/*
 **
 **  @(#) EZFlixCodec.h 96/03/06 1.5
 **
	File:		EZFlixCodec.h

	Written by:	Donn Denman and Greg Wallace

	To Do:
*/

#ifndef __EZFlixCodec__
#define __EZFlixCodec__


#define DO_DITHERING

/* Control settings */

#define		kMyComponentSpec		1L
#define		kMyComponentVersion		2L

#define		kComponentSubtype			'EZFL'
#define		kComponentManufacturer		'3DO!'


/* Macros */

#define kDEBUGME 0	/* set to 0 for no debug, 1 for debug */
#define kTraceAll 0
#if (kDEBUGME) && ! defined(__3DO__)
#define SYSBREAKSTR(x)  if (kTraceAll) Debugger()
#else
#define SYSBREAKSTR(x) 
#endif

#if (kDEBUGME)
#define DEBUGSTR(x)	\
	DebugStr(x)
#else
#define DEBUGSTR(x) 
#endif

#if defined(__3DO__) || defined(MOVIETOEZFLIX)
#define RAISE_ERROR(err) printf("### Error: %d\n", err);
#else
#define RAISE_ERROR(err) Debugger()	/* TBD - figure out a decent error machanism */
#endif

/* Version of a stream header for Opera */
#define kEZFlixFormatVersion		0		/* version of EZFL chunk format */
#define kEZFlixM2FormatVersion		2		/* version of EZFL chunk format */

#define	MAX_HEIGHT		240		/* max allowed frame height */
#define MAX_WIDTH		320		/* max allowed frame width */

#define	FP_RGB			0		/* if true, use fp for YUV calc'ns */

#define MAJORVERSION	1		/* decoder major-version number */
#define MINORVERSION	0		/* decoder minor-version number */
#define CEILING			231.0	/* companded ceiling aka MAX */
#define FLOOR			24.0	/* companded floor aka MIN */
#define GAMMA			0.8		/* gamma for noise reduction */
#define INVGAMMA		1.25	/* inverse gamma for header param */

#define TAPS			8		/* number of downsampling filter taps */
#define UVMIN			3		/* min UV clamp value. (7-bit space) */
#define UVMAX			119		/* max UV clamp value. (7-bit space) */
#define YMAX			126		/* max Y clamp value. (7-bit space) */
#define UVMINM2			0		/* min UV clamp M2 quantizers. (7-bit space) */
#define UVMAXM2			127		/* max UV clamp M2.  Not really needed due to improved wrap test */
#define YMAXM2			126		/* max Y value M2.  Prevents color overflow. */
#define YMINM2			0		/* min Y value M2.  Prevents color underflow. */

#define REFVAL			64		/* ref value for top row & left column */
#define REFVALLONG		0x40404040
#define DECODEMASK		0x7E7E7E7E
#define RGBMASK			0x7F7F7F7F
#define RGBBYTEMASK		0x7F

#define WRAPTEST				/* correct for YUV->RGB wrap-around problem */
#define WTEST			84		/* threshold for wrap-around test */
#define FTEST			64		/* test for false positives */


/* DO_DITHERING */
#define ditherFrequency 2  /* ever other one */
#define	ditherAmountUV 0x04040404
#define ditherAmountY 0x08080808
#define ditherDividerUV 0x2
#define ditherDividerY 0x4
/* DO_DITHERING */

/* Quantizer Histograms */

#define kMaxQuantLevels 6		/* Huffman6 is as high as we go! */
#define kMaxBitsPerPixel 5		/* Max BPP produced by this codec */
/*    Histograms for EZFlix Classic	*/
/*		** Y quantizer **		*/
#define Y2R3			 12		/* 8 and up */
#define Y2D3	   8
#define Y2R2			  3		/* 7 through 0 inclusive */
#define Y2D2	   0
#define Y2R1			 -2		/* -7 through -1 */
#define Y2D1	  -7
#define Y2R0			-12		/* less than -7 */

/*		** UV quantizer **		*/
#define UV2R3			  8
#define UV2D3	   5
#define UV2R2			  2
#define UV2D2	   0
#define UV2R1			 -1
#define UV2D1	  -4
#define UV2R0			 -7
/*    */

#define USE_FINE_GRAIN_QUANTIZATIONS
#ifdef USE_FINE_GRAIN_QUANTIZATIONS
/* Huffman-5 histograms */
#define Y5R4			 24
#define Y5D4	   8
#define Y5R3			  5
#define Y5D3	   1
#define Y5R2			 -1
#define Y5D2	  -1
#define Y5R1			 -7
#define Y5D1	  -7
#define Y5R0			-32

#define UV5R4			 16
#define UV5D4	   5
#define UV5R3			  5
#define UV5D3	   1
#define UV5R2			 -1
#define UV5D2	  -1
#define UV5R1			 -4
#define UV5D1	  -4
#define UV5R0			-14
#else /* USE_FINE_GRAIN_QUANTIZATIONS */
/* Huffman-5 histograms */
#define Y5R4			 14
#define Y5D4	   9
#define Y5R3			  2
#define Y5D3	   1
#define Y5R2			 -1
#define Y5D2	  -2
#define Y5R1			 -6
#define Y5D1	  -7
#define Y5R0			-16

#define UV5R4			  9
#define UV5D4	   5
#define UV5R3			  3
#define UV5D3	   2
#define UV5R2			  1
#define UV5D2	  -1
#define UV5R1			 -2
#define UV5D1	  -5
#define UV5R0			-10
#endif /* USE_FINE_GRAIN_QUANTIZATIONS */

/* Symbol Definitions for unballanced Huffman 5-level tree */
#define	Encoding0		0x0
#define	Encoding00		0x0
#define	Encoding01		0x1
#define	Encoding10		0x2
#define	Encoding11		0x3
#define	Encoding100		0x4
#define	Encoding101		0x5
#define	Encoding110		0x6
#define	Encoding111		0x7
#define	Encoding1110	0xE
#define	Encoding1111	0xF
#define	Encoding11110	0x1E
#define	Encoding11111	0x1F

/* Number of bits used for unballanced Huffman 5-level tree */
/* WARNING!!!  You can't just change these values!!! */
/*   That will work for the encoder, but to update the decoder */
/*		you need to rewrite HuffmanDecodeFrame's cases */
#define	Encoding0Bits		1
#define	Encoding00Bits		2
#define	Encoding01Bits		2
#define	Encoding10Bits		2
#define	Encoding11Bits		2
#define	Encoding100Bits		3
#define	Encoding101Bits		3
#define	Encoding110Bits		3
#define	Encoding111Bits		3
#define	Encoding1110Bits	4
#define	Encoding1111Bits	4
#define	Encoding11110Bits	5
#define	Encoding11111Bits	5

#ifdef USE_FINE_GRAIN_QUANTIZATIONS
/* R values (on right) are not multiplied by two as */
/*  is automatically done with all other values. */
#if 0 /* 4 LEVEL */
#define Y4R3			 12		/* 8 and up */
#define Y4D3	   8
#define Y4R2			  3		/* 7 through 0 inclusive */
#define Y4D2	   0
#define Y4R1			 -2		/* -7 through -1 */
#define Y4D1	  -7
#define Y4R0			-12		/* less than -7 */
#define UV4R3			  8
#define UV4D3	   5
#define UV4R2			  2
#define UV4D2	   0
#define UV4R1			 -1
#define UV4D1	  -4
#define UV4R0			 -7
#endif  /* 6 LEVEL */
#define Y6R5			 24
#define Y6D5	   8
#define Y6R4			  6
#define Y6D4	   3
#define Y6R3			  3
#define Y6D3	   1
#define Y6R2			 -2
#define Y6D2	  -2
#define Y6R1			 -5
#define Y6D1	  -7
#define Y6R0			-24

#if 1
#define UV6R5			 16
#define UV6D5	   5
#define UV6R4			  4
#define UV6D4	   1
#define UV6R3			  2
#define UV6D3	   0
#define UV6R2			 -1
#define UV6D2	  -1
#define UV6R1			 -3
#define UV6D1	  -4
#define UV6R0			-14
#else
/* TBD - Enable a wider set of UV quantizations to prevent bleeding near black etc */
#define UV6R5			 24
#define UV6D5	   8
#define UV6R4			  6
#define UV6D4	   3
#define UV6R3			  3
#define UV6D3	   1
#define UV6R2			 -2
#define UV6D2	  -2
#define UV6R1			 -5
#define UV6D1	  -7
#define UV6R0			-24
#endif
#else /* USE_FINE_GRAIN_QUANTIZATIONS */
/* Huffman-6 histograms */
#define Y6R5			 16
#define Y6D5	  12
#define Y6R4			  8
#define Y6D4	   5
#define Y6R3			  3
#define Y6D3	   0
#define Y6R2			 -1
#define Y6D2	  -4
#define Y6R1			 -7
#define Y6D1	  -10
#define Y6R0			-14

#define UV6R5			 10
#define UV6D5	   6
#define UV6R4			  4
#define UV6D4	   2
#define UV6R3			  2
#define UV6D3	   0
#define UV6R2			 -1
#define UV6D2	  -2
#define UV6R1			 -3
#define UV6D1	  -5
#define UV6R0			-12
#endif /* USE_FINE_GRAIN_QUANTIZATIONS */

#if 0
/* Special Offset Histogram for Dithered case */
/*		** Y quantizer 1 **	*/
#define Y3x2R2			 12
#define Y3x2D2	   8
#define Y3x2R1			  3
#define Y3x2D1	  -7
#define Y3x2R0		    -12
/*		** Y quantizer 2 **	*/
#define Y3x2R5			 12
#define Y3x2D5	   8
#define Y3x2R4			 -2
#define Y3x2D4	  -7
#define Y3x2R3		    -12

/*		** UV quantizer 1 **	*/
#define UV3x2R2			  7
#define UV3x2D2	   4
#define UV3x2R1			  2
#define UV3x2D1	   0
#define UV3x2R0			 -1
/*		** UV quantizer 2 **	*/
#define UV3x2R5			  3
#define UV3x2D5	   1
#define UV3x2R4			 -2
#define UV3x2D4	  -3
#define UV3x2R3			 -6
#else
/* Special Offset Histogram for Dithered case */
/*		** Y quantizer 1 **	*/
#define Y3x2R2			 12
#define Y3x2D2	   8
#define Y3x2R1			  3
#define Y3x2D1	   0
#define Y3x2R0		     -2
/*		** Y quantizer 2 **	*/
#define Y3x2R5			  3
#define Y3x2D5	   0
#define Y3x2R4			 -2
#define Y3x2D4	  -7
#define Y3x2R3		    -12

/*		** UV quantizer 1 **	*/
#define UV3x2R2			  7
#define UV3x2D2	   4
#define UV3x2R1			  2
#define UV3x2D1	   0
#define UV3x2R0			 -1
/*		** UV quantizer 2 **	*/
#define UV3x2R5			  3
#define UV3x2D5	   1
#define UV3x2R4			 -2
#define UV3x2D4	  -3
#define UV3x2R3			 -6
#endif

/*    Huffman3 Histogram - NonDithered	*/
/*		** Y quantizer **	*/
#define Y3R2			 12
#define Y3D2	   8
#define Y3R1			 0
#define Y3D1	  -5
#define Y3R0		    -10

/*		** UV quantizer **		*/
#define UV3R2			  4
#define UV3D2	   2
#define UV3R1			  0
#define UV3D1	  -1
#define UV3R0			 -3
/*    */

#define ConstY3R2			5  /* From Y3R2 with DO_EQ_EXPANSION */
#define ConstUV3R2			1

#define MAX_EQ_H3Y_VALUE	(((ConstY3R2<<24)+(ConstY3R2<<16)+(ConstY3R2<<8)+(ConstY3R2))<<1)
#define MIN_EQ_H3Y_VALUE	(~MAX_EQ_H3Y_VALUE)+0x02020202
#define MAX_EQ_H3UV_VALUE	(((ConstUV3R2<<24)+(ConstUV3R2<<16)+(ConstUV3R2<<8)+(ConstUV3R2))<<1)
#define MIN_EQ_H3UV_VALUE	(~MAX_EQ_H3UV_VALUE)+0x02020202

/*    Huffman4 Histogram	*/
/*		** Y quantizer **		*/
#define Y4R3			 12		/* 8 and up */
#define Y4D3	   8
#define Y4R2			  3		/* 7 through 0 inclusive */
#define Y4D2	   0
#define Y4R1			 -2		/* -7 through -1 */
#define Y4D1	  -7
#define Y4R0			-12		/* less than -7 */

/*		** UV quantizer **		*/
#define UV4R3			  8
#define UV4D3	   5
#define UV4R2			  2
#define UV4D2	   0
#define UV4R1			 -1
#define UV4D1	  -4
#define UV4R0			 -7
/*    */

/* Tree shape definitions */

typedef enum {
	kUnballanced5Tree,		/* HuffmanTree shape: 5-level most unballanced */
	kFixedTwoBit,			/* Opera compatible 2-bit codes */
	kUnUsedEncoding,		/* RESERVED!  Flags that no symbols are present. */ 
	kUnballanced3Tree,		/* Huffman3 level tree */
	kUnballanced4Tree,		/* Huffman4 unballanced tree */
	kSemiBallanced5Tree,	/* Semi ballanced: 0, 100, 101, 110, 111 */
	kMostBallanced5Tree,	/* Most ballanced: 00, 01, 10, 110, 111 */
	kUnballanced6Tree,		/* Unballanced: 0, 10, 110, 1110, 11110, 11111 */
	kPartlyBallanced6Tree,	/* Partly: 0, 10, 1100, 1101, 1110, 1111 */
	kSemiBallanced6Tree,	/* Semi: 0, 100, 101, 110, 1110, 1111 */
	kMostlyBallanced6Tree,	/* Mostly: 00, 01, 10, 110, 1110, 1111 */
	kMostBallanced6Tree,	/* Most: 00, 01, 100, 101, 110, 111 */
	kDithered3x2Tree,		/* Huffman3 tree with two sets of quantized values */
	kMostlyBallanced6p5Tree,/* Mostly, with half-value quantizations */
	kUnballanced5p5Tree		/* Unballanced 5, with half-value quantizations */
} TEncodingShape;

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

#define kHeaderVersion0 0 		/* headerVersion */

/* From QuickTime */

#ifndef __IMAGECOMPRESSION__
enum {
	codecLosslessQuality		= 0x400L,
	codecMaxQuality				= 0x3ffL,
	codecMinQuality				= 0x000L,
	codecLowQuality				= 0x100L,
	codecNormalQuality			= 0x200L,
	codecHighQuality			= 0x300L
};
#endif

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

#define kErrUnknownCompressionForm	-23200
#define kErrBumpQuantProblem 		-23201
#define kErrDitherDecodingProblem	-23202

#endif /* __EZFlixCodec__ */
