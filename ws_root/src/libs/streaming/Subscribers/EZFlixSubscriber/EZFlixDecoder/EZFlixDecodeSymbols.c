/*
 *	@(#) EZFlixDecodeSymbols.c 96/03/04 1.5
 *
	File:		EZFlixDecodeSymbols.c

	Contains:	EZFlix Symbol Decoders - these decode Huffman streams

	Written by:	Donn Denman

	Change History (most recent first):

		 <3>	95/07/24	DLD		Renamed the double tree huffman 3 encoder Huffman3x2Encoder
		 <2>	95/07/15	DLD		Added support for dithered 3-level encloder
		 <1>	95/07/12	DLD		first checked in

	To Do:
*/


#include "EZFlixDecodeSymbols.h"
#include "EZFlixCodec.h"
#include <stdio.h> /* in case RAISE_ERROR does this */

#ifdef TO_BE_DELETED
signed char* Huffman3DecodeSymbols(const uint32* input, 
								Byte* output, 
								signed char* symbols,
								unsigned char encodingShape,
								uint32 count);

signed char* Huffman4DecodeSymbols(const uint32* input, 
								Byte* output, 
								signed char* symbols,
								unsigned char encodingShape,
								uint32 count);

signed char* Huffman5DecodeSymbols(const uint32* input, 
								Byte* output, 
								signed char* symbols,
								unsigned char encodingShape,
								uint32 count);

signed char* Huffman6DecodeSymbols(const uint32* input, 
								Byte* output, 
								signed char* symbols,
								unsigned char encodingShape,
								uint32 count);

signed char* TwoBitDecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols, 
							unsigned char encodingShape,
							uint32 count);
#endif /* TO_BE_DELETED */

uint32 NextHuffman3Symbols(HuffmanDecodeState* hState);
uint32 NextHuffman4Symbols(HuffmanDecodeState* hState);
uint32 NextHuffman5Symbols(HuffmanDecodeState* hState);
uint32 NextHuffman6Symbols(HuffmanDecodeState* hState);
uint32 Next2BitSymbols(HuffmanDecodeState* hState);


/*
PrepSymbolDecoder - Prepare a symbol decoder state record, and scan the symbols. 
*/
signed char* PrepSymbolDecoder(const uint32* input,
							signed char* symbols, 
							unsigned char encodingShape,
							HuffmanDecodeState* stateRec)
{
	uint32 numSymbols;
	uint32 i;
	
	stateRec->inputStream = input;
	stateRec->hInStream = *stateRec->inputStream++;	/* Prep the stream by reading in the first long */
	stateRec->validInBits = 32;
	stateRec->backupBits = 0;
	stateRec->shiftHighword = 0;
	stateRec->remainingOutput = 0;
	stateRec->validOutputBits = 0;
	stateRec->ditherBit = 0;
	stateRec->encodingShape = encodingShape;
	stateRec->decodeMask = DECODEMASK;
	stateRec->shiftAmount = 1;

	switch (encodingShape)			/* Sure would be nice to use C++ instead of "switch" */
	{
		case kUnballanced5Tree:		/* HuffmanTree shape: 5-level most unballanced */
		case kUnballanced5p5Tree:	/* Unballanced half-scale tree */
			stateRec->decoder = NextHuffman5Symbols;
			numSymbols = 5;
			break;
		case kFixedTwoBit:			/* Opera compatible 2-bit codes */
			stateRec->decoder = Next2BitSymbols;
			numSymbols = 4;
			break;
		case kUnballanced4Tree:		/* Huffman4 unballanced tree */
			stateRec->decoder = NextHuffman4Symbols;
			numSymbols = 4;
			break;
		case kUnUsedEncoding:		/* RESERVED!  Flags that no symbols are present. */
			numSymbols = 0;
			break;
		case kUnballanced3Tree:		/* Huffman3 level tree */
			stateRec->decoder = NextHuffman3Symbols;
			numSymbols = 3;
			break;
		case kDithered3x2Tree:		/* Huffman3x2 dithered trees */
			stateRec->decoder = NextHuffman3Symbols;
			numSymbols = 6;
			break;
		case kMostlyBallanced6Tree:	/* Mostly: 00, 01, 10, 110, 1110, 1111 */
		case kMostlyBallanced6p5Tree:
			stateRec->decoder = NextHuffman6Symbols;
			numSymbols = 6;
			break;
		default:
			/* QuickTime will display gray if we don't supply any bits. */
			/* TBD - Put up an alert notifying the user that the stream */
			/* Is newer than we know how to play */
			numSymbols = 0;
			break;
	}

	for (i=0; i<numSymbols; i++)
	{
		if (encodingShape == kMostlyBallanced6p5Tree
					|| encodingShape == kUnballanced5p5Tree)
		{
			stateRec->symbols[i] = (*symbols++);
		}
		else
		{
			stateRec->symbols[i] = (*symbols++) << 1;
		}
	}
	
	return symbols;
}

#ifdef TO_BE_DELETED

/*
DecodeSymbols - Decode 
*/
signed char*  DecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols,
							int32 flixLayer,
							uint32 count,
							Byte encodingShape)
{
	TOUCH(flixLayer); /* avoid compiler warning */

	switch (encodingShape)			/* Sure would be nice to use C++ instead of "switch" */
	{
		case kUnballanced5Tree:		/* HuffmanTree shape: 5-level most unballanced */
		case kUnballanced5p5Tree:	/* Unballanced half-scale tree */
			return Huffman5DecodeSymbols(input, output, symbols, encodingShape, count);
			
		case kFixedTwoBit:			/* Opera compatible 2-bit codes */
			return TwoBitDecodeSymbols(input, output, symbols, encodingShape, count);
		
		case kUnballanced4Tree:		/* Huffman4 unballanced tree */
			return Huffman4DecodeSymbols(input, output, symbols, encodingShape, count);
		
		case kUnUsedEncoding:		/* RESERVED!  Flags that no symbols are present. */
			return symbols;
		
		case kUnballanced3Tree:		/* Huffman3 level tree */
		case kDithered3x2Tree:		/* Huffman3x2 dithered trees */
			return Huffman3DecodeSymbols(input, output, symbols, encodingShape, count);
		
		case kMostlyBallanced6Tree:	/* Mostly: 00, 01, 10, 110, 1110, 1111 */
		case kMostlyBallanced6p5Tree:
			return Huffman6DecodeSymbols(input, output, symbols, encodingShape, count);

		default:
			/* QuickTime will display gray if we don't supply any bits. */
			/* TBD - Put up an alert notifying the user that the stream */
			/* Is newer than we know how to play */
			break;
	}
	return symbols;
}

signed char* TwoBitDecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols, 
							unsigned char encodingShape,
							uint32 count)
{
	uint32		bitDecode[256];

	TOUCH(encodingShape);

	/* Convert the "nibbles" to bytes.  The use of nibble here is */
	/*	in the broadest sense: a small piece of data less than a byte. */
	/* Nibbles in EZFlix are always two bits. */
	/* The flix data stream consists of a series of two-bit nibbles. */
	/* The stream needs to be decoded into byte values. */
	
	/* First build the decode table, a 256 entry table that converts */
	/* a byte of four nibbles to a long of four bytes. */
	{
		uint32 byteIndex;
		int32			R[4];			/* Quantizer R levels */
		
		/* Quant levels */
		R[0] = (*symbols++)<<1;
		R[1] = (*symbols++)<<1;
		R[2] = (*symbols++)<<1;
		R[3] = (*symbols++)<<1;
		
		for (byteIndex=0; byteIndex < 256; byteIndex++)
		{
			uint32 value = 0;
			int32 j;
			for (j=6; j>=0; j-=2)
			{
				value <<= 8;
				value |= (unsigned char) (R[(byteIndex>>j) & 3]);
			}
			bitDecode[byteIndex] = value;
		}
	}

	/* Now decode the nibbles to bytes in the output buffer */
	{
		int32 j;
		int32* longOutput = (int32*) output;
		unsigned char* byteInput = (unsigned char*) input;
		int32 limit = count >> 2;
		for (j=0; j<limit; j++)
		{
			*longOutput++ = bitDecode[*byteInput++];
		}
	}
	return symbols;
}

/* This is Huffman3DecodeSymbols with an extra processing step */
/*	at the end to convert symbols from one tree to multiple trees. */
signed char* Huffman3DecodeSymbols(const uint32* input, 
										Byte* output, 
										signed char* symbols, 
										unsigned char encodingShape,
										uint32 count)
{
	uint32	hStream, validBits;
	uint32	backup = 0;
	int32	shiftHighword = 0;
	int32	bytesOut = 0;
	
	Byte nibble;
	Byte* originalOutput = output;
	
		/* load symbols */
	{	
		Byte symbol0 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol11 = (*symbols++)<<1;
		
			/* kick off the stream */
		hStream = *input++; /* it might be faster to move backwards */
		validBits = thirtyTwo;
		
		do
		{
			nibble = ((uint32) hStream) >> twentyEight;
			switch (nibble) {
				case 0: /* 0000 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;		/* bits processed */
					validBits-= four;		/* bits left to process */
					bytesOut += four;		/* symbols produced */
					break;	
				case 1: /* 0001 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += three;
					break;	
				case 2: /* 0010 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 3: /* 0011 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol11;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 4: /* 0100 */
					*output++ = symbol0;
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 5: /* 0101 */
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 6: /* 0110 */
					*output++ = symbol0;
					*output++ = symbol11;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 7: /* 0111 */
					*output++ = symbol0;
					*output++ = symbol11;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 8: /* 1000 */
					*output++ = symbol10;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 9: /* 1001 */
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 0xA: /* 1010 */
					*output++ = symbol10;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xB: /* 1011 */
					*output++ = symbol10;
					*output++ = symbol11;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xC: /* 1100 */
					*output++ = symbol11;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 0xD: /* 1101 */
					*output++ = symbol11;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 0xE: /* 1110 */
					*output++ = symbol11;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xF: /* 1111 */
					*output++ = symbol11;
					*output++ = symbol11;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
			}
			
			/* check if we're running low on valid bits */
			if (validBits < sixteen) 
			{
				/* shifted highword already? */
				shiftHighword = ! shiftHighword;
				if (shiftHighword) /* time to fetch fresh backup? */
				{
					backup = *input++; /* OR in the high word of backup */
					hStream |= (backup >> sixteen) << (sixteen - validBits);
					if (bytesOut > count) /* past the end? */
					{
						break; /* exit the while loop */
					}
				} 
				else 
				{
					hStream |= (backup << sixteen) >> validBits;
				}
				validBits += 16;
			}
	
			/* keep looping till we go past the end */
		} while (true);
	

		/* DO_DITHERING */
		if (encodingShape == kDithered3x2Tree) 
		{
			/* for Dithered trees we must convert symbols of one		*/
			/*	tree to another.  We do this the brute force way by		*/
			/*	scanning the stream, with intimate knowledge of which	*/
			/*	symbols are which.										*/
			int32 ditherCounter = 0;
			uint32 workBuffer;
			char temp;
			int32 i;
			uint32 workResult;
			int32 doneBytesCount;
			int32* convertScanner = (int32*) originalOutput;
			signed char signedSymbol0 = (signed char) symbol0;
			signed char signedSymbol10 = (signed char) symbol10;
			signed char signedSymbol11 = (signed char) symbol11;
			signed char newSymbol0 = (*symbols++)<<1;
			signed char newSymbol10 = (*symbols++)<<1;
			signed char newSymbol11 = (*symbols++)<<1;

			for (doneBytesCount=0; doneBytesCount < count; doneBytesCount+=4)
			{
				ditherCounter = ~ditherCounter;
				if (ditherCounter) 
				{
					workBuffer = *convertScanner;
					workResult = 0;
					for (i=0; i<4; i++)
					{
						if ((signed char) workBuffer == signedSymbol0)
							temp = newSymbol0;
						else if ((signed char) workBuffer == signedSymbol10)
							temp = newSymbol10;
						else if ((signed char) workBuffer == signedSymbol11)
							temp = newSymbol11;
						else {
							temp = newSymbol11;
							RAISE_ERROR(kErrDitherDecodingProblem); /* encountered an unknown eq. */
							}
						workBuffer >>= 8;
						workResult = (workResult >> 8) | (temp << 24);
					}
					*convertScanner = workResult;
				}
				convertScanner++;
			}
		}
		/* DO_DITHERING */
	}
	return symbols;
}

signed char* Huffman4DecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols, 
							unsigned char encodingShape,
							uint32 count)
{
	uint32	hStream, validBits;
	uint32	backup = 0;
	int32	shiftHighword = 0;
	int32	bytesOut = 0;
	
	Byte nibble;
	
	/* avoid compiler warning */
	TOUCH(encodingShape);

		/* load symbols */
	{	
		Byte symbol0 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol110 = (*symbols++)<<1;
		Byte symbol111 = (*symbols++)<<1;
		
			/* kick off the stream */
		hStream = *input++; /* it might be faster to move backwards */
		validBits = thirtyTwo;
		
		do
		{
			nibble = ((uint32) hStream) >> twentyEight;
			switch (nibble) {
				case 0: /* 0000 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += four;
					break;	
				case 1: /* 0001 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += three;
					break;	
				case 2: /* 0010 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 3: /* 0011 */
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= two;
					validBits-= two;
					bytesOut += two;
					break;	
				case 4: /* 0100 */
					*output++ = symbol0;
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 5: /* 0101 */
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 6: /* 0110 */
					*output++ = symbol0;
					*output++ = symbol110;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 7: /* 0111 */
					*output++ = symbol0;
					*output++ = symbol111;
					hStream <<= four;
					validBits-= four;
					bytesOut += 2;
					break;	
				case 8: /* 1000 */
					*output++ = symbol10;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 9: /* 1001 */
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 0xA: /* 1010 */
					*output++ = symbol10;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xB: /* 1011 */
					*output++ = symbol10;
					hStream <<= two;
					validBits-= two;
					bytesOut++;
					break;	
				case 0xC: /* 1100 */
					*output++ = symbol110;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xD: /* 1101 */
					*output++ = symbol110;
					hStream <<= three;
					validBits-= three;
					bytesOut++;
					break;	
				case 0xE: /* 1110 */
					*output++ = symbol111;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xF: /* 1111 */
					*output++ = symbol111;
					hStream <<= three;
					validBits-= three;
					bytesOut++;
					break;	
			}
			
				/* check if we're running low on valid bits */
			if (validBits < sixteen) {
	
					/* shifted highword already? */
				shiftHighword = ! shiftHighword;
				if (shiftHighword) /* time to fetch fresh backup? */
				{
					backup = *input++; /* OR in the high word of backup */
					hStream |= (backup >> sixteen) << (sixteen - validBits);
					if (bytesOut > count) /* past the end? */
					{
						break; /* exit the while loop */
					}
				} 
				else 
				{
					hStream |= (backup << sixteen) >> validBits;
				}
				validBits += 16;
			}
	
			/* keep looping till we go past the end */
		} while (true);
	}
	return symbols;
}

#endif /* TO_BE_DELETED */

uint32 NextHuffman3Symbols(HuffmanDecodeState* hState)
{

/*  Symbols are:
	Byte symbol0 = (*symbols++)<<1;
	Byte symbol10 = (*symbols++)<<1;
	Byte symbol11 = (*symbols++)<<1;
*/
		/* load handy numbers into registers */
	int32 sixteen = 16;
	
	uint32 result = 0;
	uint32 byteCount;
	uint16 inStream = hState->hInStream >> sixteen;
	uint32 validInBits = hState->validInBits;
	signed char* ditherSymbols = &hState->symbols[0];
	
	if (hState->encodingShape == kDithered3x2Tree) {
		hState->ditherBit = 1 - hState->ditherBit;
		ditherSymbols += 3 * hState->ditherBit;
	}

	for (byteCount=0; byteCount<4; byteCount++) {		
		result <<= 8;
		
		if (inStream < 0x8000) {		
			inStream <<= 1;						
			validInBits--;
			result |= (Byte) ditherSymbols[0];
		} else {
			if (inStream < 0xC000) {
				result |= (Byte) ditherSymbols[1];
			} else {
				result |= (Byte) ditherSymbols[2];		
			}
			inStream <<= 2;
			validInBits -= 2;
		}
	}
	
	/* shift away the bits we used */
	hState->hInStream <<= hState->validInBits - validInBits;

		/* check if we're running low on valid input bits */
	if (validInBits < sixteen) {
		uint16* wordInput = (uint16*) hState->inputStream;
		uint16 nextWord = *wordInput++;
		hState->inputStream = (uint32*) wordInput;
		hState->hInStream |= (uint32) nextWord << (sixteen - validInBits);
		validInBits += sixteen;
	}
	
		/* update our bitstream state record entry */
	hState->validInBits = validInBits;
	
	return result;
}

uint32 NextHuffman4Symbols(HuffmanDecodeState* hState)
{
#define USE_SHORT_HUFFMAN_DECODE
#ifdef USE_SHORT_HUFFMAN_DECODE

/*  Symbols are:
	Byte symbol0 = hState->symbols[0];
	Byte symbol10 = hState->symbols[1];
	Byte symbol110 = hState->symbols[2];
	Byte symbol111 = hState->symbols[3];
*/
		/* load handy numbers into registers */
	int32 sixteen = 16;
	
	uint32 result = 0;
	uint32 byteCount;
	uint16 inStream = hState->hInStream >> sixteen;
	uint32 validInBits = hState->validInBits;
	
	for (byteCount=0; byteCount<4; byteCount++) {
		/* get at least one symbol */
		result <<= 8;
		
		if (inStream < 0xC000) {				/* 110ии */
			if (inStream < 0x8000) {			/* 100ии */
				inStream <<= 1;								/* 0 case */
				validInBits--;
				result |= (Byte) hState->symbols[0];
			} else {
				inStream <<= 2;								/* 10 case */
				validInBits -=2;
				result |= (Byte) hState->symbols[1];
			}
		} else {
			if (inStream < 0xE000) {			/* 111ии */
				result |= (Byte) hState->symbols[2];		/* 110 case */
			} else {
				result |= (Byte) hState->symbols[3];		/* 111 case */
			}
			inStream <<= 3;
			validInBits -= 3;
		}
	}
	
	/* shift away the bits we used */
	hState->hInStream <<= hState->validInBits - validInBits;

		/* check if we're running low on valid input bits */
	if (validInBits < sixteen) {
		uint16* wordInput = (uint16*) hState->inputStream;
		uint16 nextWord = *wordInput++;
		hState->inputStream = (uint32*) wordInput;
		hState->hInStream |= (uint32) nextWord << (sixteen - validInBits);
		validInBits += sixteen;
	}
	
		/* update our bitstream state record entry */
	hState->validInBits = validInBits;

#else /* USE_SHORT_HUFFMAN_DECODE */

		/* load handy numbers into registers */
	const int32 thirtyTwo = 32;
	const int32 twentyEight = 28;
	const int32 sixteen = 16;
	const int32 four = 4;
	const int32 three = 3;
	const int32 two = 2;
	const int32 one = 1;
	
	Byte symbol0 = hState->symbols[0];
	Byte symbol10 = hState->symbols[1];
	Byte symbol110 = hState->symbols[2];
	Byte symbol111 = hState->symbols[3];
		
	uint32 resultBits = hState->validOutputBits;
	uint32 result = hState->remainingOutput << (thirtyTwo - resultBits);
	uint32 output;
	
	/* load symbols */
	do
	{
		Byte nibble = hState->hInStream >> twentyEight;
		switch (nibble) {
			case 0: /* 0000 */
				output = symbol0;
				output = (output<<8) | symbol0;
				output = (output<<8) | symbol0;
				output = (output<<8) | symbol0;
				hState->validOutputBits = thirtyTwo;
				hState->hInStream <<= four;
				hState->validInBits -= four;
				break;
			case 1: /* 0001 */
				output = symbol0 = symbol0;
				output = (output<<8) | symbol0;
				output = (output<<8) | symbol0;
				hState->hInStream <<= three;
				hState->validInBits-= three;
				hState->validOutputBits = 24;
				break;	
			case 2: /* 0010 */
				output = symbol0;
				output = (output<<8) | symbol0;
				output = (output<<8) | symbol10;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = 24;
				break;	
			case 3: /* 0011 */
				output = symbol0;
				output = (output<<8) | symbol0;
				hState->hInStream <<= two;
				hState->validInBits-= two;
				hState->validOutputBits = sixteen;
				break;	
			case 4: /* 0100 */
				output = symbol0;
				output = (output<<8) | symbol10;
				output = (output<<8) | symbol0;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = 24;
				break;	
			case 5: /* 0101 */
				output = symbol0;
				output = (output<<8) | symbol10;
				hState->hInStream <<= three;
				hState->validInBits-= three;
				hState->validOutputBits = sixteen;
				break;	
			case 6: /* 0110 */
				output = symbol0;
				output = (output<<8) | symbol110;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = sixteen;
				break;	
			case 7: /* 0111 */
				output = symbol0;
				output = (output<<8) | symbol111;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = sixteen;
				break;	
			case 8: /* 1000 */
				output = symbol10;
				output = (output<<8) | symbol0;
				output = (output<<8) | symbol0;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = 24;
				break;	
			case 9: /* 1001 */
				output = symbol10;
				output = (output<<8) | symbol0;
				hState->hInStream <<= three;
				hState->validInBits-= three;
				hState->validOutputBits = sixteen;
				break;	
			case 0xA: /* 1010 */
				output = symbol10;
				output = (output<<8) | symbol10;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = sixteen;
				break;	
			case 0xB: /* 1011 */
				output = symbol10;
				hState->hInStream <<= two;
				hState->validInBits-= two;
				hState->validOutputBits = 8;
				break;	
			case 0xC: /* 1100 */
				output = symbol110;
				output = (output<<8) | symbol0;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = sixteen;
				break;	
			case 0xD: /* 1101 */
				output = symbol110;
				hState->hInStream <<= three;
				hState->validInBits-= three;
				hState->validOutputBits = 8;
				break;	
			case 0xE: /* 1110 */
				output = symbol111;
				output = (output<<8) | symbol0;
				hState->hInStream <<= four;
				hState->validInBits-= four;
				hState->validOutputBits = sixteen;
				break;	
			case 0xF: /* 1111 */
				output = symbol111;
				hState->hInStream <<= three;
				hState->validInBits-= three;
				hState->validOutputBits = 8;
				break;	
		}
		
			/* check if we're running low on valid input bits */
		if (hState->validInBits < sixteen) {

				/* TBD - check if it's faster to just input a word here */
				/* shifted highword already? */
			hState->shiftHighword = ! hState->shiftHighword;
			if (hState->shiftHighword) /* time to fetch fresh backup? */
			{
				hState->backupBits = *hState->inputStream++; /* OR in the high word of backup */
				hState->hInStream |= (hState->backupBits >> sixteen) << (sixteen - hState->validInBits);
			} 
			else 
			{
				hState->hInStream |= (hState->backupBits << sixteen) >> hState->validInBits;
			}
			hState->validInBits += sixteen;
		}

			/* shift some of the partial bytes into result */
		{
			int32 leftoverBits = hState->validOutputBits + resultBits - thirtyTwo;
			if (leftoverBits >= 0 ) {
				result |= (output >> leftoverBits);
				hState->remainingOutput = output;
				hState->validOutputBits = leftoverBits;
				break; /* done a full set, exit the doиwhile*/
			}
			else
			{
				result |= output << -leftoverBits;
				resultBits += hState->validOutputBits;
			}
		}
		
		/* keep looping till we get a full set */
	} while (true);
	
#endif /* USE_SHORT_HUFFMAN_DECODE */

	return result;
}

uint32 NextHuffman5Symbols(HuffmanDecodeState* hState)
{

/*  Symbols are:
		Byte symbol0 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol110 = (*symbols++)<<1;
		Byte symbol1110 = (*symbols++)<<1;
		Byte symbol1111 = (*symbols++)<<1;
*/
		/* load handy numbers into registers */
	int32 sixteen = 16;
	
	uint32 result = 0;
	uint32 byteCount;
	uint16 inStream = hState->hInStream >> sixteen;
	uint32 validInBits = hState->validInBits;
	
	for (byteCount=0; byteCount<4; byteCount++) {
		/* get at least one symbol */
		result <<= 8;
		
		if (inStream < 0xC000) {				/* 110ии */
			if (inStream < 0x8000) {			/* 100ии */
				inStream <<= 1;								/* 0 case */
				validInBits--;
				result |= (Byte) hState->symbols[0];
			} else {
				inStream <<= 2;								/* 10 case */
				validInBits -=2;
				result |= (Byte) hState->symbols[1];
			}
		} else {
			if (inStream < 0xE000) {			/* 111ии */
				result |= (Byte) hState->symbols[2];		/* 110 case */
				inStream <<= 3;
				validInBits -= 3;
			} else {
				if (inStream < 0xF000) {
					result |= (Byte) hState->symbols[3];		/* 111 case */
				} else {
					result |= (Byte) hState->symbols[4];		/* 111 case */
				}
				inStream <<= 4;
				validInBits -= 4;
			}
		}
	}
	
	/* shift away the bits we used */
	hState->hInStream <<= hState->validInBits - validInBits;

		/* check if we're running low on valid input bits */
	if (validInBits < sixteen) {
		uint16* wordInput = (uint16*) hState->inputStream;
		uint16 nextWord = *wordInput++;
		hState->inputStream = (uint32*) wordInput;
		hState->hInStream |= (uint32) nextWord << (sixteen - validInBits);
		validInBits += sixteen;
	}
	
		/* update our bitstream state record entry */
	hState->validInBits = validInBits;
	
	return result;
}

uint32 NextHuffman6Symbols(HuffmanDecodeState* hState)
{

/*  Symbols are:
		Byte symbol00 = (*symbols++)<<1;
		Byte symbol01 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol110 = (*symbols++)<<1;
		Byte symbol1110 = (*symbols++)<<1;
		Byte symbol1111 = (*symbols++)<<1;
*/
		/* load handy numbers into registers */
	int32 sixteen = 16;
	
	uint32 result = 0;
	uint32 byteCount;
	uint16 inStream = hState->hInStream >> sixteen;
	uint32 validInBits = hState->validInBits;
	
	for (byteCount=0; byteCount<4; byteCount++) {
		/* get at least one symbol */
		result <<= 8;
		
		if (inStream < 0xC000) {				/* 110ии */
			if (inStream < 0x8000) {			/* 100ии */
				if (inStream < 0x4000) {		/* 01ии  */
					result |= (Byte) hState->symbols[0];	/* 00 case */
				} else {
					result |= (Byte) hState->symbols[1];	/* 01 case */
				}
			} else {
				result |= (Byte) hState->symbols[2];		/* 10 case */
			}
			inStream <<= 2;
			validInBits -= 2;
		} else if (inStream < 0xE000) {			/* 111ии */
				result |= (Byte) hState->symbols[3];		/* 110 case */
				inStream <<= 3;
				validInBits -= 3;
		} else {
			if (inStream < 0xF000) {
				result |= (Byte) hState->symbols[4];		/* 1110 case */
			} else {
				result |= (Byte) hState->symbols[5];		/* 1111 case */
			}
			inStream <<= 4;
			validInBits -= 4;
		}
	}
	
	/* shift away the bits we used */
	hState->hInStream <<= hState->validInBits - validInBits;

		/* check if we're running low on valid input bits */
	if (validInBits < sixteen) {
		uint16* wordInput = (uint16*) hState->inputStream;
		uint16 nextWord = *wordInput++;
		hState->inputStream = (uint32*) wordInput;
		hState->hInStream |= (uint32) nextWord << (sixteen - validInBits);
		validInBits += sixteen;
	}
	
		/* update our bitstream state record entry */
	hState->validInBits = validInBits;
	
	return result;
}

uint32 Next2BitSymbols(HuffmanDecodeState* hState)
{
		/* load handy numbers into registers */
	int32 sixteen = 16;
	
	uint32 result = 0;
	int32  j;
	uint16 inStream = hState->hInStream >> 24;
	uint32 validInBits = hState->validInBits;
	
	for (j=6; j>=0; j-=2) {
		/* get at least one symbol */
		result <<= 8;
		
		result |= (Byte) hState->symbols[(inStream>>j) & 3];
	}
	
	/* shift away the bits we used */
	hState->hInStream <<= 8;
	validInBits -= 8;
	
		/* check if we're running low on valid input bits */
	if (validInBits < sixteen) {
		uint16* wordInput = (uint16*) hState->inputStream;
		uint16 nextWord = *wordInput++;
		hState->inputStream = (uint32*) wordInput;
		hState->hInStream |= (uint32) nextWord << (sixteen - validInBits);
		validInBits += sixteen;
	}
	
		/* update our bitstream state record entry */
	hState->validInBits = validInBits;

	return result;
}

#ifdef  TO_BE_DELETED

signed char* Huffman5DecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols, 
							unsigned char encodingShape,
							uint32 count)
{
	uint32	hStream, validBits;
	uint32	backup = 0;
	int32	shiftHighword = 0;
	int32	bytesOut = 0;
	
	Byte nibble;
	
	/* avoid compiler warning */
	TOUCH(encodingShape);

		/* load symbols */
	{	
		Byte symbol0 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol110 = (*symbols++)<<1;
		Byte symbol1110 = (*symbols++)<<1;
		Byte symbol1111 = (*symbols++)<<1;
		
			/* kick off the stream */
		hStream = *input++; /* it might be faster to move backwards */
		validBits = thirtyTwo;
		
		do
		{
			nibble = ((uint32) hStream) >> twentyEight;
			switch (nibble) {
				case 0: /* 0000 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += four;
					break;	
				case 1: /* 0001 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += three;
					break;	
				case 2: /* 0010 */
					*output++ = symbol0;
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 3: /* 0011 */
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= two;
					validBits-= two;
					bytesOut += two;
					break;	
				case 4: /* 0100 */
					*output++ = symbol0;
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 5: /* 0101 */
					*output++ = symbol0;
					*output++ = symbol10;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 6: /* 0110 */
					*output++ = symbol0;
					*output++ = symbol110;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 7: /* 0111 */
					*output++ = symbol0;
					hStream <<= one;
					validBits-= one;
					bytesOut++;
					break;	
				case 8: /* 1000 */
					*output++ = symbol10;
					*output++ = symbol0;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += three;
					break;	
				case 9: /* 1001 */
					*output++ = symbol10;
					*output++ = symbol0;
					hStream <<= three;
					validBits-= three;
					bytesOut += two;
					break;	
				case 0xA: /* 1010 */
					*output++ = symbol10;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xB: /* 1011 */
					*output++ = symbol10;
					hStream <<= two;
					validBits-= two;
					bytesOut++;
					break;	
				case 0xC: /* 1100 */
					*output++ = symbol110;
					*output++ = symbol0;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xD: /* 1101 */
					*output++ = symbol110;
					hStream <<= three;
					validBits-= three;
					bytesOut++;
					break;	
				case 0xE: /* 1110 */
					*output++ = symbol1110;
					hStream <<= four;
					validBits-= four;
					bytesOut++;
					break;	
				case 0xF: /* 1111 */
					*output++ = symbol1111;
					hStream <<= four;
					validBits-= four;
					bytesOut++;
					break;	
			}
			
				/* check if we're running low on valid bits */
			if (validBits < sixteen) {
	
					/* shifted highword already? */
				shiftHighword = ! shiftHighword;
				if (shiftHighword) /* time to fetch fresh backup? */
				{
					backup = *input++; /* OR in the high word of backup */
					hStream |= (backup >> sixteen) << (sixteen - validBits);
					if (bytesOut > count) /* past the end? */
					{
						break; /* exit the while loop */
					}
				} 
				else 
				{
					hStream |= (backup << sixteen) >> validBits;
				}
				validBits += 16;
			}
	
			/* keep looping till we go past the end */
		} while (true);
	}
	return symbols;
}


signed char* Huffman6DecodeSymbols(const uint32* input, 
							Byte* output, 
							signed char* symbols, 
							unsigned char encodingShape,
							uint32 count)
{
	uint32	hStream, validBits;
	uint32	backup = 0;
	int32	shiftHighword = 0;
	int32	bytesOut = 0;
	
	Byte nibble;

	TOUCH(encodingShape); /* avoid compiler warning */
	
		/* load symbols */
	{	
		/* Mostly: 00, 01, 10, 110, 1110, 1111 */
		Byte symbol00 = (*symbols++)<<1;
		Byte symbol01 = (*symbols++)<<1;
		Byte symbol10 = (*symbols++)<<1;
		Byte symbol110 = (*symbols++)<<1;
		Byte symbol1110 = (*symbols++)<<1;
		Byte symbol1111 = (*symbols++)<<1;
		
			/* kick off the stream */
		hStream = *input++; /* it might be faster to move backwards */
		validBits = thirtyTwo;
		
		do
		{
			nibble = ((uint32) hStream) >> twentyEight;
			switch (nibble) {
				case 0: /* 0000 */
					*output++ = symbol00;
					*output++ = symbol00;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 1: /* 0001 */
					*output++ = symbol00;
					*output++ = symbol01;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 2: /* 0010 */
					*output++ = symbol00;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 3: /* 0011 */
					*output++ = symbol00;
					hStream <<= two;
					validBits-= two;
					bytesOut++;
					break;	
				case 4: /* 0100 */
					*output++ = symbol01;
					*output++ = symbol00;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 5: /* 0101 */
					*output++ = symbol01;
					*output++ = symbol01;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 6: /* 0110 */
					*output++ = symbol01;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 7: /* 0111 */
					*output++ = symbol01;
					hStream <<= two;
					validBits-= two;
					bytesOut++;
					break;	
				case 8: /* 1000 */
					*output++ = symbol10;
					*output++ = symbol00;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 9: /* 1001 */
					*output++ = symbol10;
					*output++ = symbol01;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xA: /* 1010 */
					*output++ = symbol10;
					*output++ = symbol10;
					hStream <<= four;
					validBits-= four;
					bytesOut += two;
					break;	
				case 0xB: /* 1011 */
					*output++ = symbol10;
					hStream <<= two;
					validBits-= two;
					bytesOut++;
					break;	
				case 0xC: /* 1100 */
					*output++ = symbol110;
					hStream <<= three;
					validBits-= three;
					bytesOut++;
					break;	
				case 0xD: /* 1101 */
					*output++ = symbol110;
					hStream <<= three;
					validBits-= three;
					bytesOut++;
					break;	
				case 0xE: /* 1110 */
					*output++ = symbol1110;
					hStream <<= four;
					validBits-= four;
					bytesOut++;
					break;	
				case 0xF: /* 1111 */
					*output++ = symbol1111;
					hStream <<= four;
					validBits-= four;
					bytesOut++;
					break;	
			}
			
				/* check if we're running low on valid bits */
			if (validBits < sixteen) {
	
					/* shifted highword already? */
				shiftHighword = ! shiftHighword;
				if (shiftHighword) /* time to fetch fresh backup? */
				{
					backup = *input++; /* OR in the high word of backup */
					hStream |= (backup >> sixteen) << (sixteen - validBits);
					if (bytesOut > count) /* past the end? */
					{
						break; /* exit the while loop */
					}
				} 
				else 
				{
					hStream |= (backup << sixteen) >> validBits;
				}
				validBits += 16;
			}
	
			/* keep looping till we go past the end */
		} while (true);
	}
	return symbols;
}

#endif /* TO_BE_DELETED */

