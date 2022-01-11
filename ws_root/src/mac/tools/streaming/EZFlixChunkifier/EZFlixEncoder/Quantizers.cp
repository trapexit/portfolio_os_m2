/*
 **
 **  @(#) Quantizers.cp 96/03/06 1.3
 **
	File:		Quantizers.cp
	
				Quantization and Encoding classes

	Written by:	Donn Denman

	To Do:
		Failure handling
*/

#include "Quantizers.h"
#include <stddef.h>

//============================
// Error Quatization functions
//============================


//===========================================
// MultiQuantizer
//===========================================

//
// Quantizer Construction
// 
//-------------------------------------------
MultiQuantizer::MultiQuantizer(unsigned char whichQuantizer,
								Boolean yQuantizations)
//-------------------------------------------
{
	long i;
	long* levels;

	Huffman3Encoder encoder3;
	Huffman3x2Encoder encoder3x2;
	Huffman4Encoder encoder4;
	Huffman5Encoder encoder5;
	Huffman6Encoder encoder6;
	Fixed2BitEncoder encoderF2;
	
	fQuantizer = whichQuantizer;
	
	switch (whichQuantizer)
	{
		/* Huffman5 Trees */
		case kUnballanced5Tree:		/* HuffmanTree shape: 5-level most unballanced */
		case kSemiBallanced5Tree:	/* Semi ballanced: 0, 100, 101, 110, 111 */
		case kMostBallanced5Tree:	/* Most ballanced: 00, 01, 10, 110, 111 */
		case kUnballanced5p5Tree:	/* Unballanced half-scale tree */
			fQuantLevels = (unsigned char) encoder5.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoder5.HasSecondTree();
			break;
		
		/* Opera compatible 2-bit codes */
		case kFixedTwoBit:			
			fQuantLevels = (unsigned char) encoderF2.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoderF2.HasSecondTree();
			break;
		
		/* Huffman4 unballanced tree */
		case kUnballanced4Tree:		
			fQuantLevels = (unsigned char) encoder4.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoder4.HasSecondTree();
			break;
		
		/* RESERVED!  Flags that no symbols are present. */
		case kUnUsedEncoding:		
			return;
		
		/* Huffman3 level tree */
		case kUnballanced3Tree:		
			fQuantLevels = (unsigned char) encoder3.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoder3.HasSecondTree();
			break;
			
		/* 3-level Dithered tree */
		case kDithered3x2Tree:
			fQuantLevels = (unsigned char) encoder3x2.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoder3x2.HasSecondTree();
			break;
		
		/* Huffman6 level tree */
		default:
			fQuantLevels = (unsigned char) encoder6.QuantLevels(whichQuantizer, yQuantizations, levels);
			fHasSecondTree = encoder6.HasSecondTree();
	}
	
	for (i=0; i<fQuantLevels; i++)
		if (fQuantizer == kMostlyBallanced6p5Tree
			|| fQuantizer == kUnballanced5p5Tree)
			fQuant[i] = (SInt8)(*levels++);
		else
			fQuant[i] = (SInt8)(*levels++ * 2);

	fDivision[0] = 0;
	for (i=1; i<fQuantLevels; i++) 
		fDivision[i] = *levels++;

	if (fHasSecondTree)
	{
		for (i=0; i<fQuantLevels; i++) 
			fSecondQuant[i] = (SInt8)(*levels++ * 2);
	
		fSecondDivision[0] = 0;
		for (i=1; i<fQuantLevels; i++) 
			fSecondDivision[i] = *levels++;
	}
	
	for (i=0; i<fQuantLevels; i++) 
		fEncoding[i] = *levels++;

	for (i=0; i<fQuantLevels; i++) 
		fEncodeBits[i] = *levels++;

	for (i=0; i<fQuantLevels; i++) 
		feq_hist[i]=0;
} // MultiQuantizer

//
// BumpQuantDown - bumps Quantization down one level
//

//-------------------------------------------
SInt8 MultiQuantizer::BumpQuantDown(SInt8 eqIn, long variantCounter, long /*layerNumber*/, long* overUnderFlow, long* quantChanged)
//-------------------------------------------
{
	SInt8	result = eqIn;

	if (! fHasSecondTree || ! variantCounter) {
		// normal case
		if (eqIn == fQuant[0])
		{
			*overUnderFlow = 0;
			return eqIn;
		}
		else
		{
			*quantChanged = 1;
			for (long i=1; i<fQuantLevels; i++) {
				if (eqIn == fQuant[i])
					return fQuant[i-1];
			}
		}
	} else {
		// Dithered case
		if (eqIn == fSecondQuant[0]) {
			*overUnderFlow = 0;
			return eqIn;
		}
		else
		{
			*quantChanged = 1;
			for (long i=1; i<fQuantLevels; i++) {
				if (eqIn == fSecondQuant[i])
					return fSecondQuant[i-1];
			}
		}
		RAISE_ERROR(kErrBumpQuantProblem); // fell off the end without finding our eq.
	}
	
	return eqIn;
}

//
// BumpQuantUp - bumps Quantization up one level
//
//-------------------------------------------
SInt8 MultiQuantizer::BumpQuantUp(SInt8 eqIn, long variantCounter, long /*layerNumber*/, long* overUnderFlow, long* quantChanged)
//-------------------------------------------
{
	SInt8	result = eqIn;
	
	if (! fHasSecondTree || ! variantCounter)
	{
		// normal case
		if (eqIn == fQuant[fQuantLevels-1])
		{
			*overUnderFlow = 0;
			return eqIn;
		}
		else
		{
			*quantChanged = 1;
			for (long i=0; i<fQuantLevels-1; i++) {
				if (eqIn == fQuant[i])
					return fQuant[i+1]; 
			}
		}
	} else {
		// Dithered case
		if (eqIn == fSecondQuant[fQuantLevels-1]) {
			*overUnderFlow = 0;
			return eqIn;
		}
		else
		{
			*quantChanged = 1;
			for (long i=0; i<fQuantLevels-1; i++) {
				if (eqIn == fSecondQuant[i])
					return fSecondQuant[i+1];
			}
		}
		RAISE_ERROR(kErrBumpQuantProblem); // fell off the end without finding our eq.
	}
	
	return eqIn;
}

//-------------------------------------------
SInt8 MultiQuantizer::Quantize(long errorValue, long variantCounter, long /*layerNumber*/)
//-------------------------------------------
{
	long i;
	
	if (! fHasSecondTree || ! variantCounter)
	{
		// Normal Case
		for (i=1; i<fQuantLevels; i++) 
		{
			if (errorValue < fDivision[i])
			{
				feq_hist[i-1]++;
				return fQuant[i-1];
			}
		}
		feq_hist[i-1]++;
		return fQuant[i-1];
	} else {
		for (i=1; i<fQuantLevels; i++) 
		{
			if (errorValue < fSecondDivision[i])
			{
				feq_hist[i-1]++;
				return fSecondQuant[i-1];
			}
		}
		feq_hist[i-1]++;
		return fSecondQuant[i-1];
	}
}

//
// Encode the Quantization
//
// TBD - Merge this routine in with its caller:  QuantizeEncode.
// Note that this was split out to work around a bug in the
// MrCpp compiler.  It couldn't compile the original
// QuantizeEncode routine.
//

//-------------------------------------------
void MultiQuantizer::QuantizeEncodeGuts(
					unsigned long& huffmanBits, 
					unsigned long& huffmanLong, 
					long& bitsUsed,
					long& highWord,
					long& ditherCounter,
					unsigned long*& pEQv,
					unsigned long*& pEQv_flc)
//-------------------------------------------
{
	long n;
	
	ditherCounter = ~ditherCounter; // in case we are using multiple trees.
	unsigned long scratch0 = *pEQv++; 	// grab a chunk of four eq values
	for (long k=0; k<4; k++) {
		SInt8 scratch1 = (SInt8)(scratch0 >> ((3-k)<<3));
		for (n=0; n<fQuantLevels; n++) {
			if (! fHasSecondTree) { // normal case
				if ( scratch1 == fQuant[n]) {
					huffmanBits = (huffmanBits<<fEncodeBits[n]) | fEncoding[n];
					bitsUsed += fEncodeBits[n];
					break;
				}
			} else { // second tree case
				if ( ! ditherCounter && scratch1 == fQuant[n]) {
					huffmanBits = (huffmanBits<<fEncodeBits[n]) | fEncoding[n];
					bitsUsed += fEncodeBits[n];
					break;
				} else if (ditherCounter && scratch1 == fSecondQuant[n]) {
					huffmanBits = (huffmanBits<<fEncodeBits[n]) | fEncoding[n];
					bitsUsed += fEncodeBits[n];
					break;
				}
			}
		}
		if (n==fQuantLevels) RAISE_ERROR(-30000); // error case - didn't find it!
	}
	
	// Check for a full buffer, since four new tokens could be enough
	if (bitsUsed >= 16) {
		highWord = ! highWord; // got a high word already?
		bitsUsed -= 16;
		if ( highWord ) // no, just take a word into high word of output
			huffmanLong = (huffmanBits >> bitsUsed ) << 16;
		else { // yes, OR in the low word and output
			huffmanLong |= (huffmanBits << (16 - bitsUsed)) >> 16;
			*(pEQv_flc++) = huffmanLong;
		}
	}
} // MultiQuantizer::QuantizeEncodeGuts


		
//-------------------------------------------
void MultiQuantizer::QuantizeEncode(unsigned long* unencodedStream, 
					unsigned long inputLength, 
					unsigned long*& encodedStream)
//-------------------------------------------
{
	#define USE_QUANTIZE_ENCODE_GUTS
	#ifdef USE_QUANTIZE_ENCODE_GUTS
	unsigned long* pEQv;
	unsigned long* pEQv_flc;
	unsigned long huffmanBits;
	
	pEQv     = unencodedStream;
	pEQv_flc = encodedStream;
			
	long highWord = 0;
	long bitsUsed = 0;
	long ditherCounter = 0;
	unsigned long huffmanLong;
	
	huffmanBits = 0; // assemble the huffman bits here
	for (long i=0; i < (inputLength >> 4); i++) {
		for (long j=0; j<4; j++) {
			QuantizeEncodeGuts(huffmanBits, huffmanLong, bitsUsed, highWord, ditherCounter, pEQv, pEQv_flc);
		}
	}
	
	// output any leftover bits in the buffer.
	if (bitsUsed > 0 || highWord) // Fixes artifact problem in lower right
	{
		if ( ! highWord )
			huffmanLong = (huffmanBits << (32 - bitsUsed)); // empty buffer, shift all the way up
		else
			huffmanLong |= (huffmanBits << (32 - bitsUsed)) >> 16;
		*(pEQv_flc++) = huffmanLong;
	}
	encodedStream = pEQv_flc;
	
	// Debugging -- Ensure that the Huffman encoding order packs the bits tightest
#ifdef BREAK_ON_BAD_HUFFMAN_ALLOCATION
	long mostFreq1 = feq_hist[0];
	long mostFreq2 = feq_hist[0];
	long freq1 = 0;
	long freq2 = 0;
	for (long i = 0; i < fQuantLevels; i++) {
		if (mostFreq1 < feq_hist[i]) {
			mostFreq2 = mostFreq1;
			mostFreq1 = feq_hist[i];
			freq2 = freq1;
			freq1 = i;
		}
		else if (mostFreq2 < feq_hist[i]) {
			mostFreq2 = feq_hist[i];
			freq2 = i;
		}
	}
	long leastBits1 = fEncodeBits[freq1];
	long leastBits2 = fEncodeBits[freq2];
	if (leastBits2 < leastBits1)
		Debugger();
	else for (long i = 0; i < fQuantLevels; i++) {
		if (fEncodeBits[i] < leastBits1)
			Debugger();
		else if (fEncodeBits[i] < leastBits2 &&
			i != freq1)
			Debugger();
	}
#endif // BREAK_ON_BAD_HUFFMAN_ALLOCATION

#else

	unsigned long* pEQv;
	unsigned long* pEQv_flc;
	unsigned long scratch0, scratch2, scratch3;
	SInt8 scratch1;
	long i, j, k, n;
	
	pEQv     = unencodedStream;
	pEQv_flc = encodedStream;
			
	long highWord = 0;
	long bitsUsed = 0;
	const long sixteen = 16;
	long ditherCounter = 0;
	
	scratch2 = 0; // assemble the huffman bits here
	for (i=0; i < (inputLength >> 4); i++) {
		for (j=0; j<4; j++) {
			ditherCounter = ~ditherCounter; // in case we are using multiple trees.
			scratch0 = *pEQv++; 	// grab a chunk of four eq values
			for (k=0; k<4; k++) {
				scratch1 = (SInt8)(scratch0 >> ((3-k)<<3));
				for (n=0; n<fQuantLevels; n++) {
					if (! fHasSecondTree) { // normal case
						if ( scratch1 == fQuant[n]) {
							scratch2 = (scratch2<<fEncodeBits[n]) | fEncoding[n];
							bitsUsed += fEncodeBits[n];
							break;
						}
					} else { // second tree case
						if ( ! ditherCounter && scratch1 == fQuant[n]) {
							scratch2 = (scratch2<<fEncodeBits[n]) | fEncoding[n];
							bitsUsed += fEncodeBits[n];
							break;
						} else if (ditherCounter && scratch1 == fSecondQuant[n]) {
							scratch2 = (scratch2<<fEncodeBits[n]) | fEncoding[n];
							bitsUsed += fEncodeBits[n];
							break;
						}
					}
				}
				if (n==fQuantLevels) RAISE_ERROR(-30000); // error case - didn't find it!
			}
			
			// Check for a full buffer, since four new tokens could be enough
			if (bitsUsed >= sixteen) {
				highWord = ! highWord; // got a high word already?
				bitsUsed -= sixteen;
				if ( highWord ) // no, just take a word into high word of output
					scratch3 = (scratch2 >> bitsUsed ) << sixteen;
				else { // yes, OR in the low word and output
					scratch3 |= (scratch2 << (sixteen - bitsUsed)) >> sixteen;
					*(pEQv_flc++) = scratch3;
				}
			}
		}
	}
	
	// output any leftover bits in the buffer.
	if (bitsUsed > 0 || highWord) // Fixes artifact problem in lower right
	{
		if ( ! highWord )
			scratch3 = (scratch2 << (32 - bitsUsed)); // empty buffer, shift all the way up
		else
			scratch3 |= (scratch2 << (32 - bitsUsed)) >> sixteen;
		*(pEQv_flc++) = scratch3;
	}
	encodedStream = pEQv_flc;
#endif
} // MultiQuantizer::QuantizeEncode

				// Stuff the UV and Y symbols into an EZFlix frame
unsigned long* MultiQuantizer::StuffSymbolMap(unsigned char whichUVQuantizer,
											unsigned char whichY1Quantizer,
											unsigned char whichY2Quantizer,
											signed char* flixFrame)
{
	signed char* symbolCursor = flixFrame;
	unsigned char treeShape[3];
	Boolean doYQuantizations = false;
	
	Huffman3Encoder encoder3;
	Huffman3x2Encoder encoder3x2;
	Huffman4Encoder encoder4;
	Huffman5Encoder encoder5;
	Huffman6Encoder encoder6;
	Fixed2BitEncoder encoderF2;
	
	treeShape[0] = whichUVQuantizer;
	treeShape[1] = whichY1Quantizer;
	treeShape[2] = whichY2Quantizer;
	
	for (long index=0; index < 3; index++) 
	{
		unsigned char whichQuantizer = treeShape[index];
		switch (whichQuantizer)
		{
			case kUnballanced5Tree:		/* HuffmanTree shape: 5-level most unballanced */
			case kSemiBallanced5Tree:	/* Semi ballanced: 0, 100, 101, 110, 111 */
			case kMostBallanced5Tree:	/* Most ballanced: 00, 01, 10, 110, 111 */
			case kUnballanced5p5Tree:	/* Unballanced half-scale tree */
				symbolCursor = encoder5.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
			case kFixedTwoBit:			/* Opera compatible 2-bit codes */
				symbolCursor = encoderF2.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
			case kUnballanced4Tree:		/* Huffman4 unballanced tree */
				symbolCursor = encoder4.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
			case kUnballanced3Tree:		/* Huffman3 level tree */
				symbolCursor = encoder3.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
			case kDithered3x2Tree:		/* Dithered Huffman3 level trees */
				symbolCursor = encoder3x2.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
			case kUnUsedEncoding:		/* Unused encoding tree */
				break;
				
			default:
				symbolCursor = encoder6.StuffSymbolMap(whichQuantizer, 
														symbolCursor,
														doYQuantizations);
				break;
		}
		doYQuantizations = true;
	}
	
	// word align the whole structure
	while (((symbolCursor - flixFrame + offsetof(FlixFrameHeader, QuantizeSymbols)) & 3) != 0)
		*symbolCursor++ = 0;
			
	return (unsigned long*) symbolCursor;
} // MultiQuantizer::StuffSymbolMap
		

//======================================
//
//  INDIVIDUAL ENCODERS
//
//======================================

//-------------------------------------------
signed char* Huffman3Encoder::StuffSymbolMap(unsigned char whichQuantizer,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* There is only one shape for a 3-level huffman tree */

	signed char* levels = flixFrame;
	if (whichQuantizer == kDithered3x2Tree)  // kUnballanced3Tree etc.
		RAISE_ERROR(kErrUnknownCompressionForm); // This is a serious error
	if (doYQuantizations)
	{
		*levels++ = Y3R1;
		*levels++ = Y3R0;
		*levels++ = Y3R2;
	} else {
		*levels++ = UV3R1;
		*levels++ = UV3R0;
		*levels++ = UV3R2;
	}
	return levels;
}

//-------------------------------------------
long Huffman3Encoder::QuantLevels(unsigned char whichQuantizer,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{

/* There are two variants of 3-level huffman trees */
/* One with an alternate 3-level tree to dither with */

	if (whichQuantizer == kDithered3x2Tree)  // kUnballanced3Tree etc.
		RAISE_ERROR(kErrUnknownCompressionForm);
	if (doYQuantizations) {
		fQuant[0] = Y3R0;
		fQuant[1] = Y3R1;
		fQuant[2] = Y3R2;
		
		fDivision[0] = Y3D1;
		fDivision[1] = Y3D2;
	} else {
		fQuant[0] = UV3R0;
		fQuant[1] = UV3R1;
		fQuant[2] = UV3R2;
		
		fDivision[0] = UV3D1;
		fDivision[1] = UV3D2;
	}
	
	fEncoding[0] = Encoding10;
	fEncoding[1] = Encoding0;
	fEncoding[2] = Encoding11;
	
	fEncodeBits[0] = Encoding10Bits;
	fEncodeBits[1] = Encoding0Bits;
	fEncodeBits[2] = Encoding11Bits;
	
	encoderLevels = &fQuant[0];
	return kHuffmanLevels;
}

//-------------------------------------------
signed char* Huffman3x2Encoder::StuffSymbolMap(unsigned char whichQuantizer,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* There is only one shape for a 3-level huffman tree */

	signed char* levels = flixFrame;
	if (whichQuantizer != kDithered3x2Tree)  // kUnballanced3Tree etc.
	{
		RAISE_ERROR(kErrUnknownCompressionForm);
	} else {
	// kDithered3x2Tree shape actually has 6 quant levels!
		if (doYQuantizations)
		{
			*levels++ = Y3x2R1;
			*levels++ = Y3x2R0;
			*levels++ = Y3x2R2;
			*levels++ = Y3x2R4;
			*levels++ = Y3x2R3;
			*levels++ = Y3x2R5;
		} else {
			*levels++ = UV3x2R1;
			*levels++ = UV3x2R0;
			*levels++ = UV3x2R2;
			*levels++ = UV3x2R4;
			*levels++ = UV3x2R3;
			*levels++ = UV3x2R5;
		}
	}
	return levels;
}

//-------------------------------------------
long Huffman3x2Encoder::QuantLevels(unsigned char whichQuantizer,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{

/* There are two variants of 3-level huffman trees */
/* One with an alternate 3-level tree to dither with */

	if (whichQuantizer != kDithered3x2Tree)  // kUnballanced3Tree etc.
		RAISE_ERROR(kErrUnknownCompressionForm);
	{
	// kDithered3x2Tree - Dithered tree has two 3-level trees.
		if (doYQuantizations) {
			fQuant[0] = Y3x2R0;
			fQuant[1] = Y3x2R1;
			fQuant[2] = Y3x2R2;

			fSecondQuant[0] = Y3x2R3;
			fSecondQuant[1] = Y3x2R4;
			fSecondQuant[2] = Y3x2R5;
			
			fDivision[0] = Y3x2D1;
			fDivision[1] = Y3x2D2;
			
			fSecondDivision[0] = Y3x2D4;
			fSecondDivision[1] = Y3x2D5;
		} else {
			fQuant[0] = UV3x2R0;
			fQuant[1] = UV3x2R1;
			fQuant[2] = UV3x2R2;
			
			fSecondQuant[0] = UV3x2R3;
			fSecondQuant[1] = UV3x2R4;
			fSecondQuant[2] = UV3x2R5;
			
			fDivision[0] = UV3x2D1;
			fDivision[1] = UV3x2D2;
			
			fSecondDivision[0] = UV3x2D4;
			fSecondDivision[1] = UV3x2D5;
		}
	}
	
	fEncoding[0] = Encoding10;
	fEncoding[1] = Encoding0;
	fEncoding[2] = Encoding11;
	
	fEncodeBits[0] = Encoding10Bits;
	fEncodeBits[1] = Encoding0Bits;
	fEncodeBits[2] = Encoding11Bits;
	
	encoderLevels = &fQuant[0];
	return kHuffmanLevels;
}

//======================================
//
//  Huffman4Encoder
//
//======================================

//-------------------------------------------
signed char* Huffman4Encoder::StuffSymbolMap(unsigned char /*whichQuantizer*/,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* There is only one usefull shape for a 4-level huffman tree */

	signed char* levels = flixFrame;

	if (doYQuantizations)
	{
		*levels++ = Y4R2;
		*levels++ = Y4R1;
		*levels++ = Y4R0;
		*levels++ = Y4R3;
	} else {
		*levels++ = UV4R2;
		*levels++ = UV4R1;
		*levels++ = UV4R0;
		*levels++ = UV4R3;
	}
	return levels;
}

//-------------------------------------------
long Huffman4Encoder::QuantLevels(unsigned char /*whichQuantizer*/,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{

/* There is only one usefull shape for a 4-level huffman tree */

	if (doYQuantizations) {
		fQuant[0] = Y4R0;
		fQuant[1] = Y4R1;
		fQuant[2] = Y4R2;
		fQuant[3] = Y4R3;
		
		fDivision[0] = Y4D1;
		fDivision[1] = Y4D2;
		fDivision[2] = Y4D3;
	} else {
		fQuant[0] = UV4R0;
		fQuant[1] = UV4R1;
		fQuant[2] = UV4R2;
		fQuant[3] = UV4R3;
		
		fDivision[0] = UV4D1;
		fDivision[1] = UV4D2;
		fDivision[2] = UV4D3;
	}
	
	fEncoding[0] = Encoding110;
	fEncoding[1] = Encoding10;
	fEncoding[2] = Encoding0;
	fEncoding[3] = Encoding111;
	
	fEncodeBits[0] = Encoding110Bits;
	fEncodeBits[1] = Encoding10Bits;
	fEncodeBits[2] = Encoding0Bits;
	fEncodeBits[3] = Encoding111Bits;
	
	encoderLevels = &fQuant[0];
	return kHuffmanLevels;
}

//======================================
//
//  Huffman5Encoder
//
//======================================

//-------------------------------------------
signed char* Huffman5Encoder::StuffSymbolMap(unsigned char whichQuantizer,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* TBD - Cases for each Huffman5 tree shape */
/* For now, all shapes of Huffman5 trees use the same quantization  */


/* These values are listed using a leftmost tree scan ordering. */
/* This ordering must be the inverse of the ordering listed under */
/*   QuantLevels switched by tree shape (below). */
/* Thus, for an unballanced 5 level tree, we have the following */
/*	node order: 0, 10, 110, 1110, 1111.  The symbols for this ordering */
/*	are stuffed in the header here, for use by the decoder.  */
/* The Encoding table has the inverse ordering:               */
/*	The encodings are listed in ascending order within the encoding */
/*	array:  The encoding for symbol 0 (UV5R0) is listed first, */
/*	followed by symbol 1 (UV5R1) etc.  							*/
/* In essence, the symbol map is a mapping from an encoding to */
/*	a symbol.  The quantizer's QuantLevels routine must provide the */
/*	inverse mapping from Quantization Symbol to encoding.  */

	signed char* levels = flixFrame;
	
	if (whichQuantizer != kUnballanced5Tree
		&& whichQuantizer != kUnballanced5p5Tree)
		RAISE_ERROR(kErrUnknownCompressionForm);
	else
	{
		if (doYQuantizations)
		{
			// Y Symbol Map
			*levels++ = Y5R3;	// Encoding0
			*levels++ = Y5R1;	// Encoding10
			*levels++ = Y5R2;	// Encoding110
			*levels++ = Y5R0;	// Encoding1110
			*levels++ = Y5R4;	// Encoding1111
			/*
						fEncoding[0] = Encoding1110;       // From Y section below
						fEncoding[1] = Encoding10;
						fEncoding[2] = Encoding110;
						fEncoding[3] = Encoding0;
						fEncoding[4] = Encoding1111;
			*/
		} else {
			*levels++ = UV5R3;	// Encoding0
			*levels++ = UV5R2;	// Encoding10
			*levels++ = UV5R1;	// Encoding110
			*levels++ = UV5R4;	// Encoding1110
			*levels++ = UV5R0;	// Encoding1111
			/*
						fEncoding[0] = Encoding1111;       // From UV section below
						fEncoding[1] = Encoding110;
						fEncoding[2] = Encoding10;
						fEncoding[3] = Encoding0;
						fEncoding[4] = Encoding1110;
			*/
		}
	}
	return levels;
}

//-------------------------------------------
long Huffman5Encoder::QuantLevels(unsigned char whichQuantizer,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{
	if (whichQuantizer != kUnballanced5Tree
		&& whichQuantizer != kUnballanced5p5Tree)
		RAISE_ERROR(kErrUnknownCompressionForm);
	else
	{
		if (doYQuantizations) {
			fQuant[0] = Y5R0;
			fQuant[1] = Y5R1;
			fQuant[2] = Y5R2;
			fQuant[3] = Y5R3;
			fQuant[4] = Y5R4;
			
			fDivision[0] = Y5D1;
			fDivision[1] = Y5D2;
			fDivision[2] = Y5D3;
			fDivision[3] = Y5D4;
		} else {
			fQuant[0] = UV5R0;
			fQuant[1] = UV5R1;
			fQuant[2] = UV5R2;
			fQuant[3] = UV5R3;
			fQuant[4] = UV5R4;
			
			fDivision[0] = UV5D1;
			fDivision[1] = UV5D2;
			fDivision[2] = UV5D3;
			fDivision[3] = UV5D4;
		}
	}
	
/* There are three usefull shapes for a 5-level huffman tree */

	switch (whichQuantizer)
	{
		case kUnballanced5Tree:		/* HuffmanTree shape: 5-level most unballanced */
		case kUnballanced5p5Tree:	/* Unballaced half-scale tree */
			if (doYQuantizations) {
				fEncoding[0] = Encoding1110;
				fEncoding[1] = Encoding10;
				fEncoding[2] = Encoding110;
				fEncoding[3] = Encoding0;
				fEncoding[4] = Encoding1111;
				
				fEncodeBits[0] = Encoding1110Bits;
				fEncodeBits[1] = Encoding10Bits;
				fEncodeBits[2] = Encoding110Bits;
				fEncodeBits[3] = Encoding0Bits;
				fEncodeBits[4] = Encoding1111Bits;
			} else {
				fEncoding[0] = Encoding1111;       // UV section
				fEncoding[1] = Encoding110;
				fEncoding[2] = Encoding10;
				fEncoding[3] = Encoding0;
				fEncoding[4] = Encoding1110;
				
				fEncodeBits[0] = Encoding1111Bits;
				fEncodeBits[1] = Encoding110Bits;
				fEncodeBits[2] = Encoding10Bits;
				fEncodeBits[3] = Encoding0Bits;
				fEncodeBits[4] = Encoding1110Bits;
			}
			break;
			
		case kSemiBallanced5Tree:	/* Semi ballanced: 0, 100, 101, 110, 111 */
			fEncoding[0] = Encoding110;
			fEncoding[1] = Encoding100;
			fEncoding[2] = Encoding0;
			fEncoding[3] = Encoding101;
			fEncoding[4] = Encoding111;
			
			fEncodeBits[0] = Encoding110Bits;
			fEncodeBits[1] = Encoding100Bits;
			fEncodeBits[2] = Encoding0Bits;
			fEncodeBits[3] = Encoding101Bits;
			fEncodeBits[4] = Encoding111Bits;
			break;

		case kMostBallanced5Tree:	/* Most ballanced: 00, 01, 10, 110, 111 */
			fEncoding[0] = Encoding110;
			fEncoding[1] = Encoding01;
			fEncoding[2] = Encoding00;
			fEncoding[3] = Encoding10;
			fEncoding[4] = Encoding111;
			
			fEncodeBits[0] = Encoding110Bits;
			fEncodeBits[1] = Encoding01Bits;
			fEncodeBits[2] = Encoding00Bits;
			fEncodeBits[3] = Encoding10Bits;
			fEncodeBits[4] = Encoding111Bits;
			break;
	}
	
	encoderLevels = &fQuant[0];
	return kHuffmanLevels;
}

//======================================
//
//  Huffman6Encoder
//
//======================================

//-------------------------------------------
signed char* Huffman6Encoder::StuffSymbolMap(unsigned char whichQuantizer,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* For now, all shapes of Huffman6 trees use the same quantization  */

	signed char* levels = flixFrame;

	if (whichQuantizer != kMostlyBallanced6Tree &&
		whichQuantizer != kMostlyBallanced6p5Tree)
		RAISE_ERROR(kErrUnknownCompressionForm);
	else
	{
		if (doYQuantizations)
		{
			*levels++ = Y6R4;
			*levels++ = Y6R2;
			*levels++ = Y6R3;
			*levels++ = Y6R1;
			*levels++ = Y6R0;
			*levels++ = Y6R5;
	/*		fEncoding[0] = Encoding1110;
			fEncoding[1] = Encoding110;
			fEncoding[2] = Encoding01;
			fEncoding[3] = Encoding10;
			fEncoding[4] = Encoding00;
			fEncoding[5] = Encoding1111; */
		} else {
			*levels++ = UV6R4;
			*levels++ = UV6R1;
			*levels++ = UV6R3;
			*levels++ = UV6R2;
			*levels++ = UV6R0;
			*levels++ = UV6R5;
	/*		fEncoding[0] = Encoding1110;
			fEncoding[1] = Encoding01;
			fEncoding[2] = Encoding110;
			fEncoding[3] = Encoding10;
			fEncoding[4] = Encoding00;
			fEncoding[5] = Encoding1111; */
		}
	}
	return levels;
}

//-------------------------------------------
long Huffman6Encoder::QuantLevels(unsigned char whichQuantizer,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{
	if (whichQuantizer != kMostlyBallanced6Tree &&
		whichQuantizer != kMostlyBallanced6p5Tree)
		RAISE_ERROR(kErrUnknownCompressionForm);
	else
	{
		if (doYQuantizations) {
			fQuant[0] = Y6R0;
			fQuant[1] = Y6R1;
			fQuant[2] = Y6R2;
			fQuant[3] = Y6R3;
			fQuant[4] = Y6R4;
			fQuant[5] = Y6R5;
			
			fDivision[0] = Y6D1;
			fDivision[1] = Y6D2;
			fDivision[2] = Y6D3;
			fDivision[3] = Y6D4;
			fDivision[4] = Y6D5;
		} else {
			fQuant[0] = UV6R0;
			fQuant[1] = UV6R1;
			fQuant[2] = UV6R2;
			fQuant[3] = UV6R3;
			fQuant[4] = UV6R4;
			fQuant[5] = UV6R5;
			
			fDivision[0] = UV6D1;
			fDivision[1] = UV6D2;
			fDivision[2] = UV6D3;
			fDivision[3] = UV6D4;
			fDivision[4] = UV6D5;
		}
		
	/* TBD -- other useful shapes for a 6-level huffman tree */
	/* There are at least six usefull shapes for a 6-level huffman tree */
	
		/* Mostly: 00, 01, 10, 110, 1110, 1111 */
		
		if (doYQuantizations) {
			fEncoding[0] = Encoding1110;
			fEncoding[1] = Encoding110;
			fEncoding[2] = Encoding01;
			fEncoding[3] = Encoding10;
			fEncoding[4] = Encoding00;
			fEncoding[5] = Encoding1111;
			
			fEncodeBits[0] = Encoding1110Bits;
			fEncodeBits[1] = Encoding110Bits;
			fEncodeBits[2] = Encoding01Bits;
			fEncodeBits[3] = Encoding10Bits;
			fEncodeBits[4] = Encoding00Bits;
			fEncodeBits[5] = Encoding1111Bits;
		} else {
			fEncoding[0] = Encoding1110;
			fEncoding[1] = Encoding01;
			fEncoding[2] = Encoding110;
			fEncoding[3] = Encoding10;
			fEncoding[4] = Encoding00;
			fEncoding[5] = Encoding1111;
			
			fEncodeBits[0] = Encoding1110Bits;
			fEncodeBits[1] = Encoding01Bits;
			fEncodeBits[2] = Encoding110Bits;
			fEncodeBits[3] = Encoding10Bits;
			fEncodeBits[4] = Encoding00Bits;
			fEncodeBits[5] = Encoding1111Bits;
		}
	}
	encoderLevels = &fQuant[0];
	return kHuffmanLevels;
}

//======================================
//
//  Fixed2BitEncoder
//
//======================================

//-------------------------------------------
signed char* Fixed2BitEncoder::StuffSymbolMap(unsigned char /*whichQuantizer*/,
												signed char* flixFrame,
												Boolean doYQuantizations)
//-------------------------------------------
{

/* We only have one quantization for our 2-Bit Fixed encoding  */

	signed char* levels = flixFrame;

	if (doYQuantizations)
	{
		*levels++ = Y2R0;
		*levels++ = Y2R1;
		*levels++ = Y2R2;
		*levels++ = Y2R3;
	} else {
		*levels++ = UV2R0;
		*levels++ = UV2R1;
		*levels++ = UV2R2;
		*levels++ = UV2R3;
	}
	return levels;
}

//-------------------------------------------
long Fixed2BitEncoder::QuantLevels(unsigned char /*whichQuantizer*/,
								Boolean doYQuantizations,
								long*& encoderLevels)
//-------------------------------------------
{
	if (doYQuantizations) {
		fQuant[0] = Y2R0;
		fQuant[1] = Y2R1;
		fQuant[2] = Y2R2;
		fQuant[3] = Y2R3;
		
		fDivision[0] = Y2D1;
		fDivision[1] = Y2D2;
		fDivision[2] = Y2D3;
	} else {
		fQuant[0] = UV2R0;
		fQuant[1] = UV2R1;
		fQuant[2] = UV2R2;
		fQuant[3] = UV2R3;
		
		fDivision[0] = UV2D1;
		fDivision[1] = UV2D2;
		fDivision[2] = UV2D3;
	}
	
	fEncoding[0] = Encoding00;
	fEncoding[1] = Encoding01;
	fEncoding[2] = Encoding10;
	fEncoding[3] = Encoding11;
	
	fEncodeBits[0] = Encoding00Bits;
	fEncodeBits[1] = Encoding01Bits;
	fEncodeBits[2] = Encoding10Bits;
	fEncodeBits[3] = Encoding11Bits;
	
	encoderLevels = &fQuant[0];
	return kLevels;
}


