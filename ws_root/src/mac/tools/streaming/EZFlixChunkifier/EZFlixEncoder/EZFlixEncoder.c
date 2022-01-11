/*******************************************************************************************
 **
 **  @(#) EZFlixEncoder.c 96/08/19 1.4
 **
	File:			EZFlixEncoder.c

	Contains:		Interface to EZFlix compressor

	Written by:		Greg Wallace


 *******************************************************************************************/

#include <Types.h>
#include <Memory.h>
#include <StandardFile.h>
#include <errors.h>
#include <Quickdraw.h>
#include <lowmem.h>
#include <time.h>
#include <Math.h>
#include <Memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <CursorCtl.h>
#include <Events.h>
#include "EZFlixEncoder.h"
#include "EZFlixCompress.h"
#include "EZFlixCodec.h"

/* When compiling for debugging, error messages are printed */ 
#if DEBUG
#define DIAGNOSE(x)			{									\
							printf("Error ("__FILE__"): ");		\
							printf x;							\
							}
#define NOTIFY(x)												\
							{									\
							printf x;							\
							}
#else
#define DIAGNOSE(x)
#define NOTIFY(x)
#endif

/* Alignment to 16 pixels */
#define ALIGN16(x) ((x+15) & ~15)

/* Globals */

static RGBTriple* gInternalBuffer = nil;	/* Internal image buffer.  Images to be compressed	*/
											/* are converted from their native pixel format and	*/
											/* depth to a standard 32-bit (8,8,8) RGB image in	*/
											/* this buffer before being passed to the lower		*/
											/* level of the compressor.							*/

static EZFlixParams gFlixParams;			/* Contains a PPMHeader which points to gInternalBuffer	*/
static PPMHeader* gPPMHeader = &gFlixParams.pixelMapHeader;

extern Boolean gVerbose;
extern long gQuality;


/* Set up global state of the EZFlix compressor */

long OpenEZFlixEncoder(Boolean verbose, long quality)
{
	/* Tell the low-level compressor whether to print
	 * diagnostic information.
	 */
	gFlixParams.verbose = verbose;
	gFlixParams.quality = quality;

	/* No other work to do.  Internal image buffer is allocated
	 * on first call to compress a frame.
	 */
	return kEZFlixNoError;
}



/* Take down global state of the EZFlix compressor */

long CloseEZFlixEncoder(void)
{
	/* Free the internal buffer */
	 if (gInternalBuffer != nil)
	 {
		 DisposePtr((Ptr)gInternalBuffer);
		 gInternalBuffer = nil;
	 }

	return kEZFlixNoError;
}



/* Compress raw pixels pointed to by srcBuffer.
 * Raw pixels are in the format specified by srcDepth.
 * The dimensions of the destination frame must be less than or
 * equal to the dimensions of the source frame, and must agree
 * with any additional restrictions imposed by the low-level
 * compressor.  When the destination frame is smaller, the source
 * frame will be cropped to fit into the destination frame.
 * Returns number of bytes compressed, or (negative) error value.
 */

long CompressEZFlixImage (long srcWidth, long srcHeight, long srcDepth, char *srcBuffer,
						 long destWidth, long destHeight, char *destBuffer)
{
	 long			bytesPerPixel;		/* source pixel size in bytes */
	 long			x, y;				/* row and column of destination image */
	 unsigned char	*srcPtr;			/* next pixel component in source image */
	 RGBTriple		*internalPixel;		/* next pixel in internal image buffer */
	 long			result;				/* count of bytes compressed or error return */
	 long			firstX, firstY;		/* pixel offsets in source image define top-left */
	 long			bufferSize;			/*  corner of destination image */

	result = kEZFlixNoError;
		
	firstX = (srcWidth - destWidth)/2;
	firstY = (srcHeight - destHeight)/2;

	/* Source image must be at least as big as the
	 * destination image. This function does not implement
	 * upsampling.  A destination image bigger than
	 * the source constitutes an error.
	 */
	if (firstX < 0 || firstY < 0)
		{
		DIAGNOSE(("Source frame %d x %d smaller than destination frame %d x %d\n",
				  srcWidth, srcHeight, destWidth, destHeight));
		result = kEZFlixBadFrameSize;
		goto Return;
		}

	/* Calculate bytes per pixel in source image */
	
	switch (srcDepth)
		{
		case 16:
			bytesPerPixel = 2;
			break;
		case 24:
			bytesPerPixel = 3;
			break;
		case 32:
			bytesPerPixel = 4;
			break;
		case 40:
			bytesPerPixel = 1;
			break;
		default:
			DIAGNOSE(("Unsupported bit depth %d in source image\n", srcDepth));
			result = kEZFlixBadBitDepth;
			goto Return;
		}
	
	/* About to convert source image to a 32-bit-per-pixel image in 
	 * the internal buffer.  Make sure the internal buffer is big enough.
	 */
	bufferSize = ALIGN16(destWidth) * ALIGN16(destHeight);
	if ((gInternalBuffer != nil) &&
		(gPPMHeader->width * gPPMHeader->height < bufferSize))
	{
		/* Internal buffer isn't big enough; have to reallocate it. */
		DisposePtr((Ptr)gInternalBuffer);
		gInternalBuffer = nil;
	}

	/* If an internal buffer is not allocated, allocate it now */

	if (gInternalBuffer == nil)
	{
		gInternalBuffer = (RGBTriple *)NewPtrClear(sizeof(gPPMHeader->pixels[0]) *
											  bufferSize);
		if (gInternalBuffer == nil)
		{
			DIAGNOSE(("Cannot allocate storage for internal buffer (%d x %d pixels)\n",
					  destWidth, destHeight));
			result = kEZFlixNoMemory;
			goto Return;
		}
		/* Set up the Flix params the way we want them */
		gFlixParams.tempBuf = NULL;
		gFlixParams.tempBufWidth = 0;
		gFlixParams.tempBufHeight = 0;
	}
				
	/* Set up header describing the image in the internal buffer */

	gPPMHeader->width = destWidth;
	gPPMHeader->height = destHeight;
	gPPMHeader->depth = 32;
	gPPMHeader->pixels = (RGBTriple *)gInternalBuffer;

	/* Visit every pixel in the destination image (in the internal buffer)
	 * and assign its value from the corresponding pixel in the source image.
	 * The dimensions of the destination image are less than or equal to the
	 * dimensions of the source image.  If less than, the destination image will
	 * be a cropped (NOT REDUCED) version of the source image.  The "firstX" and
	 * "firstY" variables define the pixel in the source image that will be the
	 * top-left pixel of the destination image.
	 */
	
	for (y = 0; y < destHeight; y++)
	{
		SpinCursorOnce();
		for (x = 0,
				srcPtr		  = (unsigned char*) &srcBuffer[((y+firstY)*srcWidth+firstX)*bytesPerPixel],
				internalPixel = &gPPMHeader->pixels[y*destWidth];
			x < destWidth;
			x++,
				srcPtr += bytesPerPixel,
				internalPixel++)
		{

			internalPixel->whole = 0;

			/* Assign the 32-bit destination pixel from the source pixel,
			 * based on the format of the source pixel.
			 */
			switch(srcDepth)
			{
				case 16:	/* 1 bit alpha; 5,5,5 RGB */
					/* No support for alpha channel */
					internalPixel->part.red = ((*((unsigned short *)srcPtr)) >> 10) & 0x1F;
					internalPixel->part.green = ((*((unsigned short *)srcPtr)) >> 5) & 0x1F;
					internalPixel->part.blue = ((*((unsigned short *)srcPtr)) >> 0) & 0x1F;
					break;
				case 24:	/* 8,8,8 RGB */
					internalPixel->part.red = srcPtr[0];
					internalPixel->part.green = srcPtr[1];
					internalPixel->part.blue = srcPtr[2];
					break;
				case 32:	/* 8-bit alpha; 8,8,8 RGB */
					/* No support for alpha channel */
					internalPixel->part.red = srcPtr[1];
					internalPixel->part.green = srcPtr[2];
					internalPixel->part.blue =	srcPtr[3];
					break;
				case 40:	/* 8-bit grey-scale */
					internalPixel->part.red = srcPtr[0];
					internalPixel->part.green = internalPixel->part.red;
					internalPixel->part.blue = internalPixel->part.red;
					break;
			}
		} /* for x */
	}	/* for y */

	/* Compress the 32-bit image in the internal buffer by calling
	 * the low-level compression function.
	 */
	{
		OSErr err;
		unsigned long compressedSize;

		err = GWcompress_frame(&gFlixParams, (unsigned long *)destBuffer, &compressedSize);
		if (err != noErr)
		{
			DIAGNOSE(("ERROR during compression: %d\n", err));
			result = err;
		}
		else
			result = compressedSize;
	}
	
Return:
	return result;
}

/*
 * Fill in a record of information that should
 * be carried along with the compressed data and
 * ultimately provided to the EZFlix decompressor.
 */
long GetEZFlixEncoderParams (EZFlixEncoderParams* params)

{
	params->version = (((unsigned long) MAJORVERSION) << 8) | 
					   ((unsigned long) MINORVERSION);

	params->yquant[0] = ((unsigned long)(2*Y2R0)) & 0xFF;
	params->yquant[1] = ((unsigned long)(2*Y2R1)) & 0xFF;
	params->yquant[2] = ((unsigned long)(2*Y2R2))  & 0xFF;
	params->yquant[3] = ((unsigned long)(2*Y2R3))  & 0xFF;

	params->uvquant[0] = ((unsigned long)(2*UV2R0)) & 0xFF;
	params->uvquant[1] = ((unsigned long)(2*UV2R1)) & 0xFF;
	params->uvquant[2] = ((unsigned long)(2*UV2R2)) & 0xFF;
	params->uvquant[3] = ((unsigned long)(2*UV2R3)) & 0xFF;

	params->min = FLOOR;
	params->max = CEILING;
	params->invgamma = ((INVGAMMA - 1.0) * 256.0);

	return kEZFlixNoError;
}

Boolean KeyBit(KeyMap keyMap, char keyCode);
void exithandler(void);

void SpinCursorOnce(void)
{
	const char	kAbortKeyCode = 47;	/* US "Period" key code.	*/
	const char	kCmdKeyCode = 55;	
	KeyMap	theKeyMap;	
	GetKeys(theKeyMap);
	
	if ( KeyBit(theKeyMap, kCmdKeyCode) && KeyBit(theKeyMap, kAbortKeyCode))
	{
		atexit(exithandler);
		exit(-9); /* conventional status value for user abort */
	}
	SpinCursor(1);
}

Boolean KeyBit(KeyMap keyMap, char keyCode)
{
	char* keyMapPtr = (char*)keyMap;
	char keyMapByte = keyMapPtr[keyCode / 8];
	return ((keyMapByte & (1 << (keyCode % 8))) != 0);
}

void exithandler(void)
{
	CloseMovie();
}
