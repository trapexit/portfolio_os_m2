/*
 **
 **  @(#) Quantizers.h 96/03/06 1.3
 **
	File:		Quantizers.h
	
				Quantization and Encoding classes

	Written by:	Donn Denman

	To Do:
*/

#include "EZFlixCodec.h"
#include <Types.h>

#ifndef SInt8
#define SInt8 signed char
#else
#undef 	SInt8
#define SInt8 signed char
#endif

//======================================
class MultiQuantizer
//======================================
{
	public:
				// Constructor
		MultiQuantizer(unsigned char whichQuanizer, 
						Boolean yQuantizations = false);
						
				// Quantize a value
		SInt8 Quantize(long errorValue, long variantCounter, long layerNumber);
		
				// Bump down one quant level
		SInt8 BumpQuantDown(SInt8 eqIn, long variantCounter, long layerNumber, long* overUnderFlow, long* quantChanged);
		
				// Bump up one quant level
		SInt8 BumpQuantUp(SInt8 eqIn, long variantCounter, long layerNumber, long* overUnderFlow, long* quantChanged);
		
				// Encode a stream of symbols
		void QuantizeEncodeGuts(unsigned long& huffmanBits, 
					unsigned long& partialBits, 
					long& bitsUsed,
					long& highWord,
					long& ditherCounter,
					unsigned long*& pEQv,
					unsigned long*& pEQv_flc);
								
				// Encode a stream of symbols
		void QuantizeEncode(unsigned long* unencodedStream, 
								unsigned long inputLength, 
								unsigned long*& encodedStream);
								
				// Stuff the UV and Y symbols into an EZFlix frame
		unsigned long* StuffSymbolMap(unsigned char whichUVQuantizer,
											unsigned char whichY1Quantizer,
											unsigned char whichY2Quantizer,
											signed char* flixFrame);
				// Get the encoding shape
		unsigned char EncodingShape() { return fQuantizer;}
private:
		unsigned char fQuantizer;				/* which quantizer */
		unsigned char fQuantLevels;				/* number of levels used by the quantizer */
		Boolean	fHasSecondTree;
		SInt8	fQuant[kMaxQuantLevels];		/* Quantization values */
		long	fDivision[kMaxQuantLevels];		/* Quantizer Divisions */
		long	feq_hist[kMaxQuantLevels];		/* histogram of quantized pred'n error */
		long	fEncoding[kMaxQuantLevels];
		long	fEncodeBits[kMaxQuantLevels];
		SInt8	fSecondQuant[kMaxQuantLevels];		/* Secondary Quantization values */
		long	fSecondDivision[kMaxQuantLevels];	/* Secondary Quantizer Divisions */
}; // class MultiQuantizer

//======================================
//
//  INDIVIDUAL ENCODERS
//
//======================================

//======================================
class SingleTreeEncoder
//======================================
{
	public:
		/* return true if you need a secondary encoding tree */
		Boolean	HasSecondTree() { return false; }				
}; // class SingleTreeEncoder

//======================================
class Huffman3Encoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kHuffmanLevels = 3
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
	private:
		long	fQuant[kHuffmanLevels];			/* Quantization values */
		long	fDivision[kHuffmanLevels-1];		/* Quantizer Divisions */
		long	fEncoding[kHuffmanLevels];
		long	fEncodeBits[kHuffmanLevels];
}; // class Huffman3Encoder

//======================================
class Huffman3x2Encoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kHuffmanLevels = 3		// TWO trees of three.
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
		/* return true if you need a secondary encoding tree */
		Boolean	HasSecondTree() { return true; }		

	private:
		long	fQuant[kHuffmanLevels];				/* Quantization values */
		long	fDivision[kHuffmanLevels-1];		/* Quantizer Divisions */
		long	fSecondQuant[kHuffmanLevels];		/* Quantization values */
		long	fSecondDivision[kHuffmanLevels-1];	/* Quantizer Divisions */
		long	fEncoding[kHuffmanLevels];
		long	fEncodeBits[kHuffmanLevels];
}; // class Huffman3Encoder

//======================================
class Huffman4Encoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kHuffmanLevels = 4 
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
	private:
		long	fQuant[kHuffmanLevels];			/* Quantization values */
		long	fDivision[kHuffmanLevels-1];		/* Quantizer Divisions */
		long	fEncoding[kHuffmanLevels];
		long	fEncodeBits[kHuffmanLevels];
}; // class Huffman4Encoder

//======================================
class Huffman5Encoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kHuffmanLevels = 5 
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
	private:
		long	fQuant[kHuffmanLevels];			/* Quantization values */
		long	fDivision[kHuffmanLevels-1];		/* Quantizer Divisions */
		long	fEncoding[kHuffmanLevels];
		long	fEncodeBits[kHuffmanLevels];
}; // class Huffman5Encoder

//======================================
class Huffman6Encoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kHuffmanLevels = 6 
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
	private:
		long	fQuant[kHuffmanLevels];			/* Quantization values */
		long	fDivision[kHuffmanLevels-1];		/* Quantizer Divisions */
		long	fEncoding[kHuffmanLevels];
		long	fEncodeBits[kHuffmanLevels];
}; // class Huffman6Encoder

//======================================
class Fixed2BitEncoder : public SingleTreeEncoder
//======================================
{
	enum{ 
		kLevels = 4 
		};
	public:
		/* return the quantization levels info */
		long	QuantLevels(unsigned char whichQuanizer,
								Boolean doYQuantizations, long*& encoderLevels);				
				// Stuff the UV or Y symbols into an EZFlix frame
		signed char* StuffSymbolMap(unsigned char whichQuantizer,
									signed char* flixFrame,
									Boolean doYQuantizations);
	private:
		long	fQuant[kLevels];			/* Quantization values */
		long	fDivision[kLevels-1];		/* Quantizer Divisions */
		long	fEncoding[kLevels];
		long	fEncodeBits[kLevels];
}; // class Fixed2BitEncoder


