/*
 **
 **  @(#) EZFlixDecodeFrame.c 96/03/06 1.10
 **
	File:		EZFlixDecodeFrame.c

	Contains:	Guts of the EZFlix Decoder.
					¥ Converts Huffman coded symbols into error values.	(DecodeFrame)
					¥ Converts error values into pixels. 				(DecodeFlixLayer)
					¥ Converts YUV planes into RBG.				(end of EZFlixDecodeFrame)

	Written by:	Donn Denman and Greg Wallace

	To Do:
*/

/* 						*/
/*	EZFlixDecodeFrame.c */
/*						*/

#include "EZFlixDecodeFrame.h"
#include "EZFlixDecodeSymbols.h"
#include "EZFlixXPlat.h"
#include "EZFlixCodec.h"

/* Control variables */

const int32 depth = 32;
#define kRGB555Shift 5
#define kRGB555Truncate (8 - kRGB555Shift)

/* */
/* Decode to a Pixmap */
/* */
psErr EZFlixDecodeFrame(Ptr inEZFlixFrame,
						uint32 destRowBytes,
						Ptr destBaseAddr,
						uint32 destWidth,
						uint32 destHeight,
						Boolean use16BitPixels,
						DecoderBuffers* buffers)
{
	uint32*			pY;
	FlixFrameHeader* frameHeader = (FlixFrameHeader*) inEZFlixFrame;
	uint32*			frameHeaderPtr = (uint32*) frameHeader;
	uint32*			huffmanBits = frameHeaderPtr + frameHeader->chunk1Offset;
	uint32*			huffmanBits2 = frameHeaderPtr + frameHeader->chunk2Offset;
	uint32*			huffmanBits3 = frameHeaderPtr + frameHeader->chunk3Offset;
	uint32			i;
	unsigned char*	stripPtr;

#ifdef __MWERKS__
	unsigned char	postproc[256];
#else
	const unsigned char	postproc[256] = {
		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 4,
		 5, 7, 8, 9,10,12,13,15,17,19,20,22,24,26,28,30,
		32,34,36,38,40,43,45,47,49,51,53,56,58,60,63,65,
		67,70,72,74,77,79,82,84,87,89,92,94,97,99,102,104,
		107,109,112,115,117,120,122,125,128,130,133,136,139,141,144,147,
		150,152,155,158,161,164,166,169,172,175,178,181,183,186,189,192,
		195,198,201,204,207,210,213,216,219,222,225,228,231,234,237,240,
		243,246,249,252,255,255,255,255,255,255,255,255,255,255,255,255,
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,  
		255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255 };
#endif

	/* Allocate a block big enough for one component of the decoded image and */
	/*	a row of gray on the top and left edges. */
	/* This buffer has one byte per pixel - the decoded Y component. */
	uint32 imageWidth = (destWidth + 15) & ~15;

	unsigned char* outputBuffer;
	unsigned char* yStripBuffer;	
	unsigned char* uStripBuffer;	
	unsigned char* vStripBuffer;	
	unsigned char* uBuffer;
	unsigned char* vBuffer;
	signed char* symbols;

	HuffmanDecodeState hYState;
	HuffmanDecodeState hUState;
	HuffmanDecodeState hVState;
	
	psErr err;
	
#ifdef __MWERKS__
	#define __INCLUDE_POSTPROC__
	#include "PostProcArray.h"
#endif

	/* Check the version of the stream, and give an error if it is newer than we understand */
	if (frameHeader->headerVersion > kHeaderVersion0)
		/* return codecDataVersErr; */
		return psNoErr;
	
		/* allocate the output buffers, unless we already have one. */
	err = AllocateBuffers(buffers, imageWidth);
	if (err != psNoErr) return err;

	/* set up my local buffer pointers */
	outputBuffer = buffers->outputBuffer;
	uBuffer = buffers->uBuffer;
	vBuffer = buffers->vBuffer;
	yStripBuffer = buffers->yStripBuffer;
	uStripBuffer = buffers->uStripBuffer;
	vStripBuffer = buffers->vStripBuffer;
	
		/* Initialize the strip buffers */	
	stripPtr = yStripBuffer;
	for (i=0; i<buffers->yStripBuffSize; i++) 		/* Write imaginary reference row */
	{
		*stripPtr++ = REFVAL; 
	}	

	stripPtr = uStripBuffer;
	for (i=0; i<buffers->uvStripBuffSize; i++) 		/* Write imaginary reference row */
	{
		*stripPtr++ = REFVAL; 
	}

	stripPtr = vStripBuffer;
	for (i=0; i<buffers->uvStripBuffSize; i++) 		/* Write imaginary reference row */
	{
		*stripPtr++ = REFVAL; 
	}

	/*	Start with the U and V buffers. */
	/*	Rows are encoded in sets of four. */
	/*	The first 3 pixels of each row in a set are packaged with the */
	/*		first 2 pixels of the second row and the last pixel of the second row, and */
	/*		the first pixel of the third row with the last two pixels of the third row... */
	/* */
	/*		XXX00000000000000000 */
	/*		XX00000000000000000X */
	/*		X00000000000000000XX */
	/*		00000000000000000XXX */
	/* */
	/*	The map above illustrates this point, with the X pixels treated differently.	 */
	
	/* Decode the U stream of bits into a byte buffer */
	/* UV symbols, by order of most common to least common */
	PrepSymbolDecoder(huffmanBits, frameHeader->QuantizeSymbols,
							frameHeader->UVHuffmanTree, &hUState);

	/*	Continue at the V buffer.  The V bits follow the U bits. */
	/*	Rows are encoded in sets of four, just like the U values. */

	symbols = PrepSymbolDecoder(huffmanBits2, frameHeader->QuantizeSymbols,
							frameHeader->UVHuffmanTree, &hVState);
	
	/*	Continue at the Y buffer.  The Y bits follow the V bits. */
	/*	Rows are encoded in sets of four, just like the V values. */
	/*	However, the bitmap is 4 times the size in each dimension. */

	PrepSymbolDecoder(huffmanBits3, symbols,
							frameHeader->Y1HuffmanTree, &hYState);
	
	
	/* Reset the height and width variables to the normal size setting */
	/* in preparation for YUV decoding. */
	
	pY = (uint32*)outputBuffer;

	/* */
	/* Decode YUV into RGB */
	/* */

	{
		/*
		// Simple YUV decoding is done on a scan-line by scan-line basis.
		// For each scan line we check if we have pixel data yet.
		// We decode pixel data in strips, on demand.
		// We decode a strip of Y data for every four scan lines,
		//	and a strip of U and V data for each sixteen scan lines
		//	(since U and V data are one quarter as dense).
		// Then we skip through the Y, U and V values, which are packed
		//	with a four-value diagonal per long, building up the data
		//	in Y, U and V needed to generate RGB.
		//
		// The following diagram shows an example packing.
		//
		//
		//	Given pixels labeled below:
		//		
		//		A1 B1 C1 D1 E1 F1
		//		A2 B2 C2 D2 E2 F2
		//		A3 B3 C3 D3 E3 F3
		//		A4 B4 C4 D4 E4 F4
		//		
		//	gets encoded as:
		//	
		//	F2 E3 D4 A1		F3 E4 B1 A2		F4 C1 B2 A3		D1 C2 B3 A4		E1 D2 C3 B4		F1 E2 D3 C4
		//	
		//	DecodeFlixLayer will unpack it to:
		//	
		//	A1 xx xx xx		B1 A2 xx xx		C1 B2 A3 xx		D1 C2 B3 A4		E1 D2 C3 B4		
		//					F1 E2 D3 C4		xx F2 E3 D4		xx xx D3 E4		xx xx xx F4
		//
		//
		// Essentially each long represents a diagonal, with the top byte being from
		//	the top row, the second byte being from the pixel down and left one, the
		//	third byte being from the third row, and the low byte of the long being
		//	from the bottom row.
		// The complicated part involves the packing of corner pixels, which are
		//	incomplete diagonals, together to form three longs.
		*/
		
		Byte				UVSHORTOFFSET = 0x40;		/* DC offset of 64 for u & v samples */
		Byte				GRNBYTEOFFSET = 0x60;		/* G-channel vector offset - 96 per byte */

		int32				i, k;	/* loop variables */
		uint32				RGB;
		uint16				RGB16;
		uint32				pixmapWidth; /* width of dest in pixels */

		Byte* pYBase = (Byte*) pY;
		Byte* pUBase = uBuffer;
		Byte* pVBase = vBuffer;
		Byte* pYSample = pYBase;
		Byte* pUSample = pUBase;
		Byte* pVSample = pVBase;
		Byte* pYStart = pYBase;
		Byte* pUStart = pUBase;
		Byte* pVStart = pVBase;
		Byte USample  = 0;
		Byte VSample  = 0;
		Byte YSample;
		Byte Rvalue, Gvalue, Bvalue;
		
		uint32* pixelBase = (uint32*) destBaseAddr;		/* 32-bit output-RGB frame buffer */
		uint32* pixel = pixelBase;
		uint16* pixel16Base = (uint16*) destBaseAddr;
		uint16* pixel16 = pixel16Base;
		
		if (use16BitPixels)
			pixmapWidth	= destRowBytes >> 1; /* width of dest in pixels */
		else
			pixmapWidth	= destRowBytes >> 2; /* width of dest in pixels */
				
		/* for each scan line in the destination */
		for (k=0; k<destHeight; k++) {
			int32 fourthScanLine = k & 3;
			int32 sixteenthScanLine = k & 0xF;
			
			/* Starting a set of sixteen scan lines? Recalculate U, V */
			if (sixteenthScanLine == 0)
			{
				uint32 uvwidth = (imageWidth>>2)+3;
			
				/* decode into the u buffer */
				DecodeFlixLayer((uint32*) uBuffer, uvwidth, k, 
								1, uStripBuffer, &hUState);
			
				DecodeFlixLayer((uint32*) vBuffer, uvwidth, k,
								2, vStripBuffer, &hVState);
			}
			
			/* Starting a set of four scan lines? Recalculate Y */
			if (fourthScanLine == 0)
			{
				/* decode luminance */
				DecodeFlixLayer((uint32*) outputBuffer, 
									imageWidth+3, k,
									3, yStripBuffer, &hYState);
			}
			
			/* RGB convert a row of pixels. */
			if (use16BitPixels)
			{
				/* for each pixel in the destination, pick the pixel
					from the right spot in the set of four pixels */
				for (i=0; i < destWidth; i++)
				{
					int32 fourthPixel = i & 3;
				
					if (fourthPixel == 0)
					{
						USample = *pUSample + UVSHORTOFFSET; /* fetch a U and V input */
						pUSample += 4;
						VSample = *pVSample + UVSHORTOFFSET;
						pVSample += 4;
					}
	
					/* Calculate the RGB values */
					YSample = *pYSample; /* fetch a Y input */
					pYSample += 4;
					Rvalue = (YSample + VSample) & RGBBYTEMASK;
					Gvalue = (YSample + GRNBYTEOFFSET) - (VSample>>1) - (USample>>2);
					Gvalue &= RGBBYTEMASK;
					Bvalue = (YSample + USample) & RGBBYTEMASK;
				
				/* Reverse Gamma the values, and merge into a pixel */
					RGB16 = postproc[Rvalue] >> kRGB555Truncate;
					RGB16 = (RGB16<<kRGB555Shift) | (postproc[Gvalue] >> kRGB555Truncate);
					RGB16 = (RGB16<<kRGB555Shift) | (postproc[Bvalue] >> kRGB555Truncate);
					*pixel16++ = RGB16;
				}
			
			/* bump the output down one line */
				pixel16Base += pixmapWidth;
				pixel16 = pixel16Base;
				
			} else {
				/* for each pixel in the destination, pick the pixel
					from the right spot in the set of four pixels */
				for (i=0; i < destWidth; i++)
				{
					int32 fourthPixel = i & 3;
				
					if (fourthPixel == 0)
					{
						USample = *pUSample + UVSHORTOFFSET; /* fetch a U and V input */
						pUSample += 4;
						VSample = *pVSample + UVSHORTOFFSET;
						pVSample += 4;
					}
	
					/* Calculate the RGB values */
					YSample = *pYSample; /* fetch a Y input */
					pYSample += 4;
					Rvalue = (YSample + VSample) & RGBBYTEMASK;
					Gvalue = (YSample + GRNBYTEOFFSET) - (VSample>>1) - (USample>>2);
					Gvalue &= RGBBYTEMASK;
					Bvalue = (YSample + USample) & RGBBYTEMASK;
					
					/* Reverse Gamma the values, and merge into a pixel */
					RGB = postproc[Rvalue];
					RGB = (RGB<<8) | postproc[Gvalue];
					RGB = (RGB<<8) | postproc[Bvalue];
					*pixel++ = RGB;
				}
			
				/* bump the output down one line */
				pixelBase += pixmapWidth;
				pixel = pixelBase;
			}
			
			/* Bump the input pixel pointers */
			if (fourthScanLine != 3)
			{
				/* point to the next row of Y bytes, same U, V rows */
				pYStart += 5;
				pYSample = pYStart;
				pUSample = pUStart;
				pVSample = pVStart;
			}
			else
			{
				/* done four rows.  Move down a row in the input */
				pYStart = pYBase;
				pYSample = pYStart;
				
				/* check if done with four-line set in UV. */
				if (sixteenthScanLine != 0xF)
				{
					/* point to the next row of U and V; */
					pUStart += 5;
					pUSample = pUStart;
					pVStart += 5;
					pVSample = pVStart;
				}
				else
				{
					pUStart = pUBase;
					pUSample = pUStart;
					pVStart = pVBase;
					pVSample = pVStart;
				}
			}
		}
	}
	return psNoErr;
}

#if kDEBUGME  
#define DISPLAY_ERRORVALUES
#ifndef macintosh
#undef DISPLAY_ERRORVALUES
#endif
#endif

#ifdef DISPLAY_ERRORVALUES
Boolean KeyBit(KeyMap keyMap, char keyCode);
Boolean KeyBit(KeyMap keyMap, char keyCode)
{
	char* keyMapPtr = (char*)keyMap;
	char keyMapByte = keyMapPtr[keyCode / 8];
	return ((keyMapByte & (1 << (keyCode % 8))) != 0);
}
#endif


void DecodeFlixLayer(uint32* pixelBuffer, 
					uint32 widthInLongs,
					uint32 height,
					uint32 layerNumber,
					unsigned char* stripBuffer,
					HuffmanDecodeState* decoderState)
{
	uint32* pY = pixelBuffer;
	int32	n;
	uint32	Av,Bv,Cv,EQv,Pv,Yv;
	uint32	EQvLineEnd[3];
	const uint32		widthInLongsMinus3 = widthInLongs-3;
	uint32				previousRow = -4;
	unsigned char* 		lastRow;
	
#ifdef DISPLAY_ERRORVALUES
	const char	kYKeyCode = 16;
	const char	kShiftLockKeyCode = 57;
	const char	kUKeyCode = 32;
	const char	kVKeyCode = 9;
		
	KeyMap	theKeyMap;
	Boolean keyYIsDown = false;
	Boolean shiftLockIsDown = false;
	Boolean keyUIsDown = false;	
	Boolean keyVIsDown = false;	
	Boolean aKeyIsDown = false;	
	GetKeys(theKeyMap);
	
	if ( KeyBit(theKeyMap, kYKeyCode))
		keyYIsDown = true;
	if ( KeyBit(theKeyMap, kShiftLockKeyCode))
		shiftLockIsDown = true;
	if ( KeyBit(theKeyMap, kUKeyCode))
		keyUIsDown = true;
	if ( KeyBit(theKeyMap, kVKeyCode))
		keyVIsDown = true;
	if (keyYIsDown || keyUIsDown || keyVIsDown)
		aKeyIsDown = true;

#endif /* DISPLAY_ERRORVALUES */

	TOUCH(height); /* prevent compiler warnings */
	TOUCH(layerNumber);
	
	/*	Rows are encoded in sets of four, with each long representing a four-pixel diagonal. */
	/*	The first row contains the MSB values, and the last row contains */
	/*		the LSB values. 			*/
	/* 									*/
	/*		XXX00000¥00000000000 MSB	*/
	/*		XX00000¥00000000000X 		*/
	/*		X00000¥00000000000XX 		*/
	/*		00000¥00000000000XXX LSB	*/
	/*									*/
	/* The stripBuffer represents the bottom row of the previous set of four pixels */

	/*
		Given pixels labeled below:
		
			A1 B1 C1 D1 E1 F1
			A2 B2 C2 D2 E2 F2
			A3 B3 C3 D3 E3 F3
			A4 B4 C4 D4 E4 F4
			
		gets encoded as:
		
		F2 E3 D4 A1		F3 E4 B1 A2		F4 C1 B2 A3		D1 C2 B3 A4		E1 D2 C3 B4		F1 E2 D3 C4
		
		DecodeFlixLayer will unpack it to:
		
		A1 xx xx xx		B1 A2 xx xx		C1 B2 A3 xx		D1 C2 B3 A4		E1 D2 C3 B4		
						F1 E2 D3 C4		xx F2 E3 D4		xx xx D3 E4		xx xx xx F4
	*/

	{
		Yv  = REFVALLONG;						/* Initializations */
		Cv  = Yv;
		lastRow = stripBuffer;
		
		for (n=0; n<3; n++) {					/** Compute first 3 partial IDPCM vectors **/
			Av = Yv;							/* Compute partial p vector */
			Bv = Cv;
			Cv = (Av>>8) | (*lastRow++<<24);
			Pv = ((Av + Cv)>>1) + Av + Cv - Bv;
			EQv = decoderState->decoder(decoderState); /* get the next huffman-decoded long */
			EQvLineEnd[n] = EQv;				/* save line starting values for the line end */

			/* Output only the left end */
			if (n == 0)							/* Shift away the portions from the end */
				EQv <<= 24;
			else if (n == 1)
				EQv <<= 16;
			else
				EQv <<= 8;
			Yv  = (Pv + EQv) >> decoderState->shiftAmount;
			Yv &= decoderState->decodeMask;
			*pY++ = Yv;
		}
		
	#ifdef DISPLAY_ERRORVALUES
		
			/* */
			/* Inner Loop of the Decoder */
			/* */
		for (n=3; n<widthInLongsMinus3; n++)		/** Compute next IDPCM vector **/
		{
			Av = Yv;							/* Compute next p vector */
			Bv = Cv;
			Cv = (Av>>8) | (*lastRow++<<24);
			EQv = decoderState->decoder(decoderState); 	/* get the next huffman-decoded long */
			Pv = ((Av + Cv)>>1) + Av + Cv - Bv;
			Yv  = (Pv + EQv) >> decoderState->shiftAmount; /* Compute decompressed-sample vector */
			if (aKeyIsDown)
			{
				if (layerNumber == 1)					/* U layer.  Mask if U key not down */
				{
					if (keyUIsDown) {
						if (shiftLockIsDown)
							Yv  = (REFVALLONG + EQv + EQv);	/* Colored Error values */
					} else
						Yv  = REFVALLONG;				/* Neutral Gray */
				}
				else if (layerNumber == 2)
				{
					if (keyVIsDown) {
						if (shiftLockIsDown)
							Yv  = (REFVALLONG + EQv + EQv);	/* Colored Error values */
					} else
						Yv  = REFVALLONG;				/* Neutral Gray */
				}
				else
				{
					if (keyYIsDown) {
						if (shiftLockIsDown)
							Yv  = (REFVALLONG + EQv + EQv);	/* Shades of Gray Error values */
					} else
						Yv  = REFVALLONG;				/* Neutral brightness */
				}
			}
			Yv &= decoderState->decodeMask;
			*pY++ = Yv;
			lastRow[previousRow] = (unsigned char) Yv;
		}
		
	#else /* DISPLAY_ERRORVALUES */
		
			/* */
			/* Inner Loop of the Decoder */
			/* */
		for (n=3; n<widthInLongsMinus3; n++)	/** Compute next IDPCM vector **/
		{
			Av = Yv;							/* Compute next p vector */
			Bv = Cv;
			Cv = (Av>>8) | (*lastRow++<<24);
			Pv = ((Av + Cv)>>1) + Av + Cv - Bv;
			EQv = decoderState->decoder(decoderState); /* get the next huffman-decoded long */
			Yv  = (Pv + EQv) >> 1;				/* Compute decompressed-sample vector */
			Yv &= decoderState->decodeMask;
			*pY++ = Yv;
			lastRow[previousRow] = (unsigned char) Yv;
		}
		
	#endif /* DISPLAY_ERRORVALUES */
		
		
		for (n=0; n<3; n++)						/** Compute last 3 partial IDPCM vectors **/
		{
			Av = Yv;							/* Compute partial p vector */
			Bv = Cv;
			Cv = (Av>>8);
			Pv = ((Av + Cv)>>1) + Av + Cv - Bv;
			EQv = EQvLineEnd[n];

			/* Output only the right end */
			if (n == 0)							/* Shift away the portions from the end */
				EQv >>= 8;
			else if (n == 1)
				EQv >>= 16;
			else
				EQv >>= 24;
			Yv  = (Pv + EQv) >> decoderState->shiftAmount;
			Yv &= decoderState->decodeMask;
			*pY++ = Yv;
			(++lastRow)[previousRow] = (unsigned char) Yv;
		}
	}
}

/* Allocate all the little buffers needed for decompression */
/* TBD - Allocate a single buffer, and point to portions of it */

psErr AllocateBuffers(DecoderBuffers* buffers, uint32 imageWidth)
{
	uint32 widthInLongs = imageWidth+3;
	uint32 width = widthInLongs * sizeof(int32);
	uint32 uvStripBuffSize = (imageWidth >> 2) + 3;
	uint32 buffSize;
	unsigned char* nextBuffer;
	
	/* Is the buffer big enough already? */
	if (imageWidth <= buffers->imageWidth)
		return psNoErr;
		
	/* Free the old buffer if there is one. */
	if (buffers->buffer != NULL)
	{
		FreeBuffers(buffers);
	}
	
	/* compute total buffer needs */
	/* output buffer */
	buffSize = width+3;
	/* yStripBuffer */
	buffSize += widthInLongs;
	/* uStripBuffer and vStripBuffer */
	buffSize += uvStripBuffSize << 1;
	/* uBuffer and vBuffer */
	buffSize += (uvStripBuffSize * sizeof(int32)) << 1;
	
	/* Do the allocation! */
	buffers->buffer = NewEZFlixPtr(buffSize);
	
	/* return if there was a problem */
	if (buffers->buffer == NULL)
	{
		return memFullErr;
	}
	
	/* remember the vital statistics about the buffer */
	buffers->bufferSize = buffSize;
	buffers->yStripBuffSize = widthInLongs;
	buffers->uvStripBuffSize = uvStripBuffSize;
	buffers->imageWidth = imageWidth;
	nextBuffer = (unsigned char*) buffers->buffer;
	
	/* Skip along the buffer remembering sub-buffer pointers */
	buffers->outputBuffer = nextBuffer;
	nextBuffer += width+3;

	buffers->yStripBuffer = nextBuffer;
	nextBuffer += widthInLongs;
	
	buffers->uStripBuffer = nextBuffer;
	nextBuffer += uvStripBuffSize;

	buffers->vStripBuffer = nextBuffer;
	nextBuffer += uvStripBuffSize;

	buffers->uBuffer = nextBuffer;
	nextBuffer += uvStripBuffSize * sizeof(int32);

	buffers->vBuffer = nextBuffer;

	/* Done */
	return psNoErr;
}


void FreeBuffers(DecoderBuffers* buffers)
{
	/*	Dispose of private variables. */
	if ( buffers )
	{
		/* free the temp buffer */
		if (buffers->buffer != NULL) 
			DeleteEZFlixPtr((Ptr) buffers->buffer, buffers->bufferSize);
	}
	
	buffers->buffer = NULL;
	buffers->imageWidth = 0;
}
