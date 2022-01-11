/******************************************************************************
**
**  @(#) mpaudiodecode.h 96/03/28 1.3
**
******************************************************************************/

/* defines for mpaudiodecode.c */

#ifndef __MISC_MPAUDIODECODE_H
#define __MISC_MPAUDIODECODE_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MISC_MPEG_H
#include <misc/mpeg.h>
#endif

#ifndef MakeMPAErr
	#define MakeMPAErr(svr,class,err)      MakeErr(ER_FOLI,ER_MPA,svr,ER_E_SSTM,class,err)
#endif
 
/* MPEG Audio Decoder Errors */
#define MPANoMemErr        			MakeMPAErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* Non standard error. */

/* Error during parsing */
#define MPAParsingErr 				MakeMPAErr(ER_SEVERE,ER_C_NSTND,1)

/* Layer not supported */
#define MPAUnsupportedLayerErr		MakeMPAErr(ER_SEVERE,ER_C_NSTND,2)

/* Result buffer is too small for decoded data */
#define MPABufrTooSmallErr			MakeMPAErr(ER_SEVERE,ER_C_NSTND,3)

/* Invalid bitrate */
#define MPAUnsupportedBitrateErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,4)

/* Invalid sampling frequency */
#define MPAUnsupportedSampleFreqErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,5)

/* Invalid bitrate and mode combination */
#define MPAInvalidBitrateModeErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,6)

/* "Only ISO/IEC 11172-3 audio supported" */
#define MPAInvalidIDErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,7)

/* Undefined or reserved layer */
#define MPAUndefinedLayerErr		MakeMPAErr(ER_SEVERE,ER_C_NSTND,8)

/* Decompressed data not yet available */
#define MPAOutputDataNotAvailErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,9)

/* Decompressed data not yet available */
#define MPAInputDataNotAvailErr	MakeMPAErr(ER_SEVERE,ER_C_NSTND,10)

/* Compressed data not available */
#define MPAEOFErr MakeMPAErr(ER_SEVERE,ER_C_NSTND,11)

/* possible return value from the callback functions. */
#define MPAPTSVALID			1
#define MPAFLUSH	    	2
#define AUDIO_FLOAT_SIZE	4

/* Size of one decompressed MPEG audio frame. */
#define BYTES_PER_MPEG_AUDIO_FRAME	\
		(MPEG_SAMPLES_PER_AUDIO_FRAME * AUDIO_FLOAT_SIZE)

#ifdef __cplusplus
extern "C" {
#endif

/* These are the callback functions provided for the MPEG Audio decoder
 * to read the compressed data. */

typedef Err (*MPACompressedBfrReadFn)(const void *theUnit,
	uint8 *buf );
typedef Err (*MPAGetCompressedBfrFn)(const void *theUnit,
  const uint8 **buf, int32 *len, uint32 *pts, uint32 *ptsIsValidFlag );

typedef struct MPACallbackFns {
	MPACompressedBfrReadFn		CompressedBfrReadFn;
	MPAGetCompressedBfrFn		GetCompressedBfrFn;
} MPACallbackFns;

typedef struct MPADecoderContext MPADecoderContext, *MPADecoderContextPtr;

/******************************
 * Public function prototypes
 ******************************/

Err CreateMPAudioDecoder( MPADecoderContextPtr *ctx, MPACallbackFns CallbackFns );
Err DeleteMPAudioDecoder( MPADecoderContextPtr ctx );
Err MPAudioDecode( void *theUnit, MPADecoderContextPtr ctx,
	uint32 *pts, uint32 *ptsIsValidFlag, uint32 *decompressedBufr,
	AudioHeader *header );
Err MPAFlush( MPADecoderContextPtr ctx );

#ifdef __cplusplus
}
#endif

#endif /* end of #ifndef __MISC_MPAUDIODECODE_H */
