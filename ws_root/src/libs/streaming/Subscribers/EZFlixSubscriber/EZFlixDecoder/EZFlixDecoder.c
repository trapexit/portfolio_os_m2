/*******************************************************************************************
 *  @(#) EZFlixDecoder.c 96/03/26 1.11
 *
 *	File:			EZFlixDecoder.c
 *
 *	Contains:		EZFlix decompressor
 *
 *	Written by:		Greg Wallace
 *
 *******************************************************************************************/

#ifndef makeformac

#include <kernel/debug.h>
#include <kernel/mem.h>
#include <audio/audio.h>

#include <stdio.h>

#else /* makeformac */

#include "debug.h"
#include "mem.h"
#include "audio.h"
/*  #include "debug3do.h"  */

#include "stdio.h"

#endif /* makeformac */

#include "EZFlixDecoder.h"
#include "EZFlixDecodeFrame.h"
#include "EZFlixXPlat.h"

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
///	CONSTANTS
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

#define MAJORVERSION	1
#define MINORVERSION	0
#define DEC_VERSION		((MAJORVERSION<<8) + MINORVERSION)

#define	THDOWIDTH		320				/* 3DO fbuff width */
#define	THDOHEIGHT		240				/* 3DO fbuff height */
#define DECODEMOD		16				/* pixel modulus for EZFlix data */

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
///	MACROS
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

/* When compiling for debugging, error messages are printed */ 
#ifdef DEBUG
#define DIAGNOSE(x)			{									\
							PRNT(("Error (File: "__FILE__"; Line: "__LINE__"  ): "));		\
							PRNT( x );							\
							}
#else
#define DIAGNOSE(x)
#endif

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
///	GLOBALS
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
///	STATICS (INTERNAL STATE OF DECOMPRESSOR)
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

/* Decoder buffers */
static DecoderBuffers decodeBuffers = {NULL,0,NULL,NULL,NULL,NULL,0,NULL,NULL,0,0};

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
///	EZFLIX FUNCTIONS
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/


/**************************\
|| SetEZFlixDecoderParams ||
\**************************/

/*
 * Set parameters of the EZFlix decompressor.  These parameters will
 * typically be carried along with the data; for example, in the header
 * of an EZFlix segment of a data stream.
 */
int32 SetEZFlixDecoderParams (EZFlixDecoderParams* params)
{
	/* Check restrictions on the requested frame dimensions.
	 * Frame must be no greater than 320 x 240 and both width and height must be
	 * a multiple of 16 pixels.
	 */
	if ((params->width > THDOWIDTH) || (params->width < DECODEMOD) ||
		(params->height > THDOHEIGHT) || (params->height < DECODEMOD)) {
		DIAGNOSE(("Frame size out of range W=[%ld,%ld] or H=[%ld,%ld]\n",
				 (int32)DECODEMOD, (int32)THDOWIDTH, (int32)DECODEMOD, (int32)THDOHEIGHT));
		return kEZFlixBadFrameSize;
	}

	if (((params->width % DECODEMOD) != 0) || ((params->height % DECODEMOD) != 0)) {
		DIAGNOSE(("Image width or height is not a multiple of %ld\n", (int32)DECODEMOD));
		return kEZFlixBadFrameSize;
	}

	if (params->codecVersion > DEC_VERSION) {
		DIAGNOSE(("Version mismatch: decoder version is %ld.%ld, but bitstream version is %ld.%ld.\n",
			MAJORVERSION, MINORVERSION, (params->codecVersion>>8), (params->codecVersion & 0xFFFF)));
		return kEZFlixVersionError;
	}

	if (params->EZFlixVersion != kEZFlixM2FormatVersion) {
		DIAGNOSE(("Version mismatch: decoder version is %ld, but EZFlix compression version is %ld.\n",
			kEZFlixM2FormatVersion, params->EZFlixVersion));
		return kEZFlixVersionError;
	}

	return kEZFlixNoError;
}

/*/ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
/// EZFLIX-INTERFACE FUNCTIONS
///ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ*/

/***********************\
|| CreateEZFlixDecoder ||
\***********************/

/* Do the expensive fixed initialization here, before
 * the stream is started.
 */
int32 CreateEZFlixDecoder(void)
{
	return AllocateBuffers(&decodeBuffers, THDOWIDTH);
}

/************************\
|| DisposeEZFlixDecoder ||
\************************/

void DisposeEZFlixDecoder(void)
{
	FreeBuffers(&decodeBuffers);
}

/********************\
|| DecompressEZFlix ||
\********************/

int32 DecompressEZFlix (char *compressedFrame, uint32 compressedSize, char *destAddr, int32 rowBytes, 
			int32 destWidth, int32 destHeight, uint32 bytesPerPixel, Boolean *frameReturned)
{
	int32	result;


	*frameReturned = false;

	TOUCH(compressedSize);	/* prevent compiler warning */
	
	result = EZFlixDecodeFrame(compressedFrame,
						rowBytes,
						destAddr,
						destWidth,
						destHeight,
						bytesPerPixel == 2, /* Boolean use16BitPixels */
						&decodeBuffers);


	if (result != kEZFlixNoError) goto Return;

	/* Going to return a decompressed frame */
	*frameReturned = true;

Return:
	return result;
}
