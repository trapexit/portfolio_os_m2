/*
 **
 **  @(#) EZFlixDecodeSymbols.h 96/03/04 1.5
 **
	File:		EZFlixDecodeSymbols.h

	Contains:	xxx put contents here xxx

	Written by:	Donn Denman

	To Do:
*/



#ifndef __EZFLIX_DECODE_SYMBOLS__
#define __EZFLIX_DECODE_SYMBOLS__

#include "EZFlixXPlat.h"
#include "EZFlixCodec.h"

struct HuffmanDecodeState; typedef struct HuffmanDecodeState HuffmanDecodeState;

typedef uint32 (*HuffmanDecodeProc)(HuffmanDecodeState* hState);

#define kMaxSymbols 6

typedef struct HuffmanDecodeState {
	HuffmanDecodeProc decoder;	/* the decoder proc that handles this struct */
	uint32 hInStream;			/* leading few bits of the huffman stream */
	int32  validInBits;			/* number of valid bits in hInStream */
	uint32 backupBits;			/* buffers input from huffman stream */
	uint32 shiftHighword;		/* if the highword of backupBits has been used */
	uint32 remainingOutput;		/* remaining output bytes from last time */
	uint32 validOutputBits;		/* number of remaining output bits that are valid */
	uint32 decodeMask;			/* mask used during decoding */
	uint32 shiftAmount;			/* amount to shift during decoding */
	const uint32* inputStream;		/* huffman input stream */
	unsigned char encodingShape;
	unsigned char ditherBit;
	signed char symbols[kMaxSymbols]; 	/* symbols for decoding */
} HuffmanDecodeState;

signed char* DecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols,
							int32 flixLayer,
							uint32 count,
							Byte encodingShape);
							
signed char* PrepSymbolDecoder(const uint32* input,
							signed char* symbols, 
							unsigned char encodingShape,
							HuffmanDecodeState* decoder);

#endif /* __EZFLIX_DECODE_SYMBOLS__ */
