/*****************************************************************************
**
**  @(#) EZFlixDecoder.h 96/03/04 1.5
**
******************************************************************************/

#ifndef __EZFLIXDECODER_H__
#define __EZFLIXDECODER_H__


/* Error codes returned by the decoder */

#define kEZFlixNoError		0
#define kEZFlixNoMemory		(-9000)		/* cannot allocate heap storage */
#define kEZFlixBadArgument	(-9001)		/* bad argument passed to function */
#define kEZFlixBadFrameSize	(-9002)		/* frame size does not meet current restrictions */
#define kEZFlixVersionError	(-9003)		/* stream data has newer version than this decoder */


/* Settable decompressor parameters */

typedef struct {
	uint32	codecVersion;
	uint32	EZFlixVersion;
	uint32	width;
	uint32	height;
} EZFlixDecoderParams;


/* Decompressor functions */

extern int32	CreateEZFlixDecoder (void);
extern void		DisposeEZFlixDecoder (void);

extern int32	DecompressEZFlix (char *compressedFrame, 
									uint32 compressedSize, 
									char *destAddr, 
									int32 rowBytes,
									int32 destWidth,
									int32 destHeight,
									uint32 bytesPerPixel,
									Boolean *frameReturned);

extern int32	SetEZFlixDecoderParams (EZFlixDecoderParams* params);


#endif
