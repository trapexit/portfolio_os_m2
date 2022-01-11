/*
 **
 **      @(#) EZFlixDefines.h 96/03/04 1.6
 **
	File:		EZFlixDefines.h

	Written by:	Donn Denman and Greg Wallace

	To Do:
*/

#ifndef __EZFlixDefines__
#define __EZFlixDefines__


/* Version of a stream header for Opera */
#define kEZFlixFormatVersion		0		/* version of EZFL chunk format */
#define kEZFlixM2FormatVersion		2		/* version of EZFL chunk format */

/* These are the available quality settings */
#define kQualitySetting1			10
#define kQualitySetting2			25
#define kQualitySetting3			40
#define kQualitySetting4			60
#define kQualitySetting5			75

/* Associations between the QuickTime quality slider */
/*	and the resulting EZFlix compression quality selected */

#define kHuffman3Quality			(kQualitySetting1 * codecLosslessQuality/100)
#define kFixedTwoBitQuality			(kQualitySetting2 * codecLosslessQuality/100)
#define kHuffman4Quality			(kQualitySetting3 * codecLosslessQuality/100)
#define kHuffman5Quality			(kQualitySetting4 * codecLosslessQuality/100)
#define kHuffman6Quality			(kQualitySetting5 * codecLosslessQuality/100)

/*
FlixFrameHeader - This header is put on each compressed frame to mark
	the version, the type of encoding for the frame, and most important
	the QuantizeSymbols used for this frame.

*/
typedef struct {
	unsigned char headerVersion; /* version of this header */
	unsigned char UVHuffmanTree; /* shape of UV Huffman trees */
	unsigned char Y1HuffmanTree; /* shape of Y1 Huffman tree */
	unsigned char Y2HuffmanTree; /* shape of Y2 Huffman tree */
	unsigned short chunk1Offset; /* offset to end of this record */
	unsigned short chunk2Offset; /* offset to second Chunk of huffman bits in longwords */
	unsigned short chunk3Offset; /* offset to Y huffman bits in longwords */
	signed char QuantizeSymbols[24]; /* ! VARIABLE LENGTH !  Quantization Symbols */
} FlixFrameHeader;


#endif /* __EZFlixDefines__ */

