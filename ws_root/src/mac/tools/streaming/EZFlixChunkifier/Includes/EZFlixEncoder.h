/*******************************************************************************************
**
**        @(#) EZFlixEncoder.h 96/03/04 1.4
**
	File:			EZFlixEncoder.h

	Contains:		Interface to EZFlix compressor

	Written by:		Greg Wallace

 *******************************************************************************************/

#ifndef __EZFLIXENCODER_H__
#define __EZFLIXENCODER_H__

#include <Types.h>

/* Encoder constants */

#define kEZFlixWidthModulus		16		/* frame width must be an even multiple of this value */

/* Error codes returned by the encoder */

#define kEZFlixNoError			0
#define kEZFlixNoMemory			(-1000)		/* cannot allocate heap storage */
#define kEZFlixBadFrameSize		(-1001)		/* mismatch between source and destination image dimensions */
#define kEZFlixBadBitDepth		(-1002)		/* bit depth of source image is not supported */

/* "Global" parameters */


/* Compression parameters */

typedef struct {
	unsigned long	version;
	unsigned char	yquant[4];
	unsigned char	uvquant[4];
	unsigned long	min;
	unsigned long	max;
	unsigned long	invgamma;
} EZFlixEncoderParams;



/* Compressor functions */

#ifdef __cplusplus
extern "C" {
#endif

extern long OpenEZFlixEncoder(Boolean verbose, long quality);

extern long CloseEZFlixEncoder(void);

extern long CompressEZFlixImage (long srcWidth, long srcHeight, long srcDepth, char *srcBuffer,
						 long destWidth, long destHeight, char *destBuffer);

extern long GetEZFlixEncoderParams(EZFlixEncoderParams* params);

extern void SpinCursorOnce(void);

extern void CloseMovie(void);

#ifdef __cplusplus
}
#endif

#endif /* __EZFLIXENCODER_H__ */

