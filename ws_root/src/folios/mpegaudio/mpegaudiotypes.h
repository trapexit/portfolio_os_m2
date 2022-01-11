/* @(#) mpegaudiotypes.h 96/11/25 1.4 */
#ifndef _MPEGAUDIOTYPES_H
#define _MPEGAUDIOTYPES_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MISC_MPEG_H
#include <misc/mpeg.h>
#endif

#ifndef __MISC_MPEGAUDIODECODE_H
#include <misc/mpaudiodecode.h>
#endif

#define AUDIO_FLOAT float
#define AUDIO_INT_SIZE 4

#define SIZE_OF_SYNCWORD_PLUS_A_NIBBLE	2
#define SYNCWORD_SHIFTLEFT_FOUR			(MPEG_AUDIO_SYNCWORD << 4)

#define MAX_CHAN	2

/* [TBD] Replace this with AudioHeader defined in mpeg.h */
typedef struct Header {
  uint32 id;
  uint32 layer;
  uint32 protection_bit;
  uint32 bitrate_index;
  uint32 sampling_frequency;
  uint32 padding_bit;
  uint32 private_bit;
  uint32 mode;
  uint32 mode_extension;
  uint32 copyright;
  uint32 original;
  uint32 emphasis;
} Header;

/* stuff associated with the Audio frame */
typedef struct FrameInfo {
  AudioHeader	*header;	/* raw header information */
  uint32	crc_check;	/* crc check word */
  uint32	nch;		/* number of channels (1 or 2) */
  uint32	sblimit; 	/* total number of sub bands */
  uint32	bound;		/* first band of joint stereo coding */
  uint32	table;		/* bit allocation table selected */
  uint32	allocation[32*2]; /* bit allocation per channel, subband */
  uint32  	scalefactor_index[2*32*3];
} FrameInfo;

/* stuff associated with the Driver interaction (buffers) */
typedef struct BufferInfo {
  void				*theUnit;
  MPACallbackFns	CallbackFns;
  uint32			timeStamp;	/* current buffer's timestamp */
  uint32			PTSIsValid;
  bool				wholeFrame; /* Set for the first whole frame of the
								 * bit stream buffer */
  uint8				*readBuf;	/* pointer to beginning of bit stream buffer */
  uint8				*curBytePtr;/* current byte ptr into the bit stream buffer */
  uint8				readCount;
  uint32			readWord;	/* top 32 bits in readBuf */
  int32				readBits; 	/* number of valid bits in readWord */
  int32				readLen;	/* length of readBuf */
  uint8				*writeBuf;	/* pointer to beginning of write buffer */
  Err				readStatus;	/* status of last call to MPAGetCompressedBfr (persistant) */
} BufferInfo;

typedef struct AudioDecodingBlks {
  int32       windowTempArray[MPEG_SAMPLES_PER_AUDIO_FRAME];
  uint32      samplecode[MAX_CHAN][32][4];	/* 32 samples per sub-blocks */
  AUDIO_FLOAT matrixOutputSamples[MAX_CHAN][MPEG_SAMPLES_PER_AUDIO_FRAME];
  AUDIO_FLOAT matrixInputSamples[MAX_CHAN][MPEG_SAMPLES_PER_AUDIO_FRAME];
  AUDIO_FLOAT windowPartialSums[MAX_CHAN][512];	/* 32 samples x 16 sub-blocks */
  AUDIO_FLOAT scale;
} AudioDecodingBlks, *AudioDecodingBlksPtr;

struct MPADecoderContext {
	AudioDecodingBlks	matrices;
	BufferInfo			bi;
};
 
#endif /* _MPEGAUDIOTYPES_H */

