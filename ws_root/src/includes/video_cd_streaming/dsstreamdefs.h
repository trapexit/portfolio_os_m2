/******************************************************************************
**
** @(#) dsstreamdefs.h 96/06/05 1.2
**
** Filed data types and constants for Data Stream streams.
**
*****************************************************************************/
#ifndef __DSSTREAMDEFS_H
#define __DSSTREAMDEFS_H


#ifndef __STDDEF_H
#include <stddef.h>					/* offsetof() */
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MPEG_H__
#include <video_cd_streaming/mpeg.h>				/* AudioHeader */
#endif

/**************************************/
/* Your basic white book stream chunk */
/**************************************/

#define MAX_WB_CHUNKS		32

/* A minimal data stream chunk.
 * NOTE: streamChunkSize doesn't have to be a multiple of 4. The next stream
 *       chunk begins on the next mod 4 address after this chunk. */

#define WBCHUNK_COMMON															\
	uint32			chunkType;			/* 0:VIDEO 1:AUDIO */					\
	const void*		buffer;				/* ptr to start of data */				\
	int32			bufferSize;			/* size of data in BYTES */				\
	int32			pts;				/* 33-bit wraparound PTS; negative => none */ \
	int8			ptsValid;			/* true if this pts value is valid */	\
	uint8			packetStart;		/* is this buffer the start of a packet's data? */

/********************************/
/* MPEG video subscriber chunks */
/********************************/
/* This is a hack for now */
/* This subscriber supports stream data with this format version number. */
#ifndef MPVD_STREAM_VERSION
#define MPVD_STREAM_VERSION		1
#endif

#ifndef MPVD_CHUNK_TYPE 
	#define MPVD_CHUNK_TYPE					MAKE_ID('M','P','V','D')
#endif

typedef	struct MPEGVideoHeaderChunk {
	uint32			version;			/* format version number, currently 1 */
	uint32			maxPictureArea;		/* to preallocate ref frame buffers */
	uint32			framePeriod;		/* nominal, in 90 kHz ticks */
} MPEGVideoHeaderChunk, *MPEGVideoHeaderChunkPtr;
	
/********************************
 * MPEG audio subscriber chunks
 * NOTE: The 2-, 3-, and 4-frame chunks and the Silence chunk are not
 * yet supported and might never be needed.
 ********************************/

#define MPAU_CHUNK_TYPE						MAKE_ID('M','P','A','U')

typedef struct SAudioSampleDescriptor
	{
	uint32	dataFormat;				/* 4 char identifier (e.g. 'raw ') */
	int32	sampleSize;				/* bits per sample, typically 16 */
	int32 	sampleRate;    			/* 44100 or 22050 samples/sec */
	int32	numChannels;			/* channels per frame,1=mono,2=stereo */
	uint32  compressionType;  		/* eg. SDX2, 930210 */
	int32	compressionRatio;
	int32	sampleCount;			/* Number of samples in sound */
	} SAudioSampleDescriptor, *SAudioSampleDescriptorPtr;

typedef	struct SAudioHeaderChunk {
	uint32	version;				/* format version number */
	int32	numBuffers;				/* max # of buffers that can be */
									/* queued to the AudioFolio */
	int32	initialAmplitude;		/* initial volume of the stream */
	int32	initialPan;				/* initial pan of the stream */
	SAudioSampleDescriptor sampleDesc;	/* sound format info */
	} SAudioHeaderChunk, *SAudioHeaderChunkPtr;

typedef struct SAudioDataChunk {
	int32	actualSampleSize;		/* actual number of bytes in the sample data */
	uint32	samples[1];				/* start of DYNAMIC SIZE audio sample data bytes */
	} SAudioDataChunk, *SAudioDataChunkPtr;

/* The size of the header information that preceeds every audio data chunk.
 * Use this instead of sizeof(SAudioDataChunk). */ 
#define SAUDIO_DATA_CHUNK_HDR_SIZE	(offsetof(SAudioDataChunk, samples))


/***********************/
/* Stream header chunk */
/***********************/

#ifndef HEADER_CHUNK_TYPE
	#define	HEADER_CHUNK_TYPE	 MAKE_ID('S','H','D','R')
#endif

#define	DS_HDR_MAX_PRELOADINST	16
#define	DS_HDR_MAX_SUBSCRIBER	16

#define	DS_STREAM_VERSION	2	/* Stream header version number */


typedef struct DSHeaderSubs {
	int32		subscriberType;		/* a subscriber data type */
	int32		deltaPriority;		/* the delta priority for its thread */
	} DSHeaderSubs, *DSHeaderSubsPtr;

typedef struct DSHeaderChunk {
	uint32	headerVersion;		/* format version number = DS_STREAM_VERSION */

	int32	streamBlockSize;	/* size of stream buffers in this stream */
	int32	streamBuffers;		/* suggested number of stream bfrs to use */
	int32	streamerDeltaPri;	/* delta priority for streamer thread */
	int32	dataAcqDeltaPri;	/* delta priority for data acq thread */
	int32	numSubsMsgs;		/* number of subscriber msgs to allocate */

	int32	audioClockChan;		/* logical channel # of audio clock channel */
	int32	enableAudioChan;	/* mask of audio channels to enable */
	
		/* NULL terminated array of instruments to preload */
	int32	preloadInstList[DS_HDR_MAX_PRELOADINST];

	DSHeaderSubs	subscriberList[DS_HDR_MAX_SUBSCRIBER];
	} DSHeaderChunk, *DSHeaderChunkPtr;


#endif	/* __STREAMING_DSSTREAMDEFS_H */
