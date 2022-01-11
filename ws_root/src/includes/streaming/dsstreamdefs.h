/******************************************************************************
**
** @(#) dsstreamdefs.h 96/03/15 1.29
**
** Filed data types and constants for Data Stream streams.
**
*****************************************************************************/
#ifndef __STREAMING_DSSTREAMDEFS_H
#define __STREAMING_DSSTREAMDEFS_H


#ifndef __STDDEF_H
#include <stddef.h>					/* offsetof() */
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MISC_MPEG_H
#include <misc/mpeg.h>				/* AudioHeader */
#endif



/***************************/
/* Your basic stream chunk */
/***************************/

/* A minimal data stream chunk.
 * NOTE: streamChunkSize doesn't have to be a multiple of 4. The next stream
 *       chunk begins on the next mod 4 address after this chunk. */
typedef struct StreamChunk {
	uint32	streamChunkType;		/* data type, a 4-character-code */
	uint32	streamChunkSize;		/* chunk size in bytes INCLUDING this header */
	uint32	streamChunkData[1];		/* start of DYNAMIC SIZE data bytes, uint32-aligned */
	} StreamChunk, *StreamChunkPtr;

#define STREAMCHUNK_HEADER_SIZE		offsetof(StreamChunk, streamChunkData)


/* Practically all subscriber chunks start with these common fields. */
#define	SUBS_CHUNK_COMMON										\
	uint32	chunkType;		/* chunk data type */				\
	uint32	chunkSize;		/* chunk size including header */	\
	uint32	time;			/* position in stream time */		\
	uint32	channel;		/* logical channel number */		\
	uint32	subChunkType	/* data sub-type, should be called chunkSubtype */

typedef	struct SubsChunkData {
	SUBS_CHUNK_COMMON;
	} SubsChunkData, *SubsChunkDataPtr;


/***********************************************/
/* FILL chunk, used to fill out a stream block */
/***********************************************/

#ifndef	FILL_CHUNK_TYPE
	#define	FILL_CHUNK_TYPE			MAKE_ID('F','I','L','L')
#endif


/****************************************************************************
 * CTRL subscriber chunks.
 *
 * NOTE: The CTRL subscriber is history. Some of these chunks are replaced by
 * STRM chunks. Others are replaced by automatic Streamer features.
 ****************************************************************************/

#ifndef CTRL_CHUNK_TYPE
	#define	CTRL_CHUNK_TYPE		MAKE_ID('C','T','R','L')
#endif

#ifndef	GOTO_CHUNK_SUBTYPE
	#define	GOTO_CHUNK_SUBTYPE	MAKE_ID('G','O','T','O')
#endif

#ifndef	SYNC_CHUNK_SUBTYPE
	#define	SYNC_CHUNK_SUBTYPE	MAKE_ID('S','Y','N','C')
#endif

#ifndef	ALRM_CHUNK_SUBTYPE
	#define	ALRM_CHUNK_SUBTYPE	MAKE_ID('A','L','R','M')
#endif

#ifndef	PAUS_CHUNK_SUBTYPE
	#define	PAUS_CHUNK_SUBTYPE	MAKE_ID('P','A','U','S')
#endif

#ifndef	STOP_CHUNK_SUBTYPE
	#define	STOP_CHUNK_SUBTYPE	MAKE_ID('S','T','O','P')
#endif


typedef struct ControlChunk {
	SUBS_CHUNK_COMMON;			/* chunkType 'CTRL' */
	union {
		struct {				/* sub-type 'GOTO' */
			uint32		value;
			} marker;

		struct {				/* sub-type 'SYNC' */
			uint32		value;
			} sync;

		struct {				/* sub-type 'ALRM' */
			uint32		options;
			uint32		newTime;
			} alarm;

		struct {				/* sub-type 'PAUS' */
			uint32		options;
			uint32		amount;
			} pause;

		struct {				/* sub-type 'STOP' */
			uint32		options;
			} stop;
		} u;

	} ControlChunk, *ControlChunkPtr;


/*************************************************
 * STRM chunks: HALT, GOTO, STOP
 *************************************************/

#ifndef STREAM_CHUNK_TYPE
	#define STREAM_CHUNK_TYPE	MAKE_ID('S','T','R','M')
#endif

#ifndef HALT_CHUNK_SUBTYPE
	#define HALT_CHUNK_SUBTYPE	MAKE_ID('H','A','L','T')
#endif

#define TIME_CHUNK_SUBTYPE		MAKE_ID('T','I','M','E')

/* Cf. GOTO_CHUNK_SUBTYPE and STOP_CHUNK_SUBTYPE, above. */

enum GOTO_Options {
	/* --- Branch type				Meaning of (dest1, dest2) fields */
	GOTO_OPTIONS_ABSOLUTE	= 0,	/* (byte position, time) */
	GOTO_OPTIONS_MARKER		= 1,	/* (marker number, N/A) */
	GOTO_OPTIONS_PROGRAMMED	= 2,	/* (programmed-branch number, N/A) */

	/* Bitmask to select the branch type field */
	GOTO_OPTIONS_BRANCH_TYPE_MASK	= 0xFF,

	/* --- Additional branch option flags */
	GOTO_OPTIONS_FLUSH		= 1<<8,	/* flush subscribers instead of a butt-joint */
	GOTO_OPTIONS_WAIT		= 1<<9	/* branch then wait for DSStartStream */
	};

typedef struct HaltChunk1 {
	SUBS_CHUNK_COMMON;				/* 'STRM', 'HALT' */
	StreamChunk	subscriberChunk; 	/* embeded chunk for a subscriber */
	} HaltChunk1;

typedef struct StreamGoToChunk {
	SUBS_CHUNK_COMMON;				/* 'STRM', 'GOTO' */
	uint32		options;			/* GOTO_Options */
	uint32		dest1;				/* destination info (depends on options) */
	uint32   	dest2;				/* destination info (depends on options) */
	} StreamGoToChunk, *StreamGoToChunkPtr;

typedef struct StreamStopChunk {
	SUBS_CHUNK_COMMON;				/* 'STRM', 'STOP' */
	uint32		options;			/* options [TBD] */
	} StreamStopChunk, *StreamStopChunkPtr;

typedef struct StreamTimeChunk {
	SUBS_CHUNK_COMMON;				/* 'STRM', 'TIME' */
	uint32		newTime;
	} StreamTimeChunk, *StreamTimeChunkPtr;


/*************************************************************************
 * DataAcquisition can also act as a subscriber to subscribe to marker-
 * table chunks. A marker table translates stream times to file positions.
 *************************************************************************/

#ifndef DATAACQ_CHUNK_TYPE
	#define DATAACQ_CHUNK_TYPE		MAKE_ID('D','A','C','Q')
#endif

#ifndef MARKER_TABLE_SUBTYPE
	#define MARKER_TABLE_SUBTYPE	MAKE_ID('M','T','B','L')
#endif

typedef struct MarkerRec {
	uint32   markerTime;			/* stream time */
	uint32   markerOffset;			/* file position in output stream */
	} MarkerRec, *MarkerRecPtr;

typedef struct DSMarkerChunk {
	SUBS_CHUNK_COMMON;				/* 'DACQ', 'MTBL' */
	/* MarkerRec	markerTable[]; */
	} DSMarkerChunk, *DSMarkerChunkPtr;


/****************************/
/* EZFLIX subscriber chunks	*/
/****************************/

#define EZFLIX_CHUNK_TYPE			MAKE_ID('E','Z','F','L')
#define EZFLIX_HDR_CHUNK_SUBTYPE	MAKE_ID('F','H','D','R')
#define EZFLIX_FRME_CHUNK_SUBTYPE	MAKE_ID('F','R','M','E')


/********************************/
/* MPEG video subscriber chunks */
/********************************/

#ifndef MPVD_CHUNK_TYPE 
	#define MPVD_CHUNK_TYPE					MAKE_ID('M','P','V','D')
#endif

/* An MPEG video header chunk contains one time initialization and
 * configuration stuff for a given channel.  */
#ifndef MVHDR_CHUNK_SUBTYPE
	#define MVHDR_CHUNK_SUBTYPE				MAKE_ID('V','H','D','R')
#endif

/* A compressed frame can be delivered in one 'FRAM' chunk *or* split into a
 * 'FRM[' and a 'FRM]' chunk. The first frame of a GOP must start with the
 * Video Sequence Header and the GOP header. */
#ifndef MVDAT_CHUNK_SUBTYPE
	#define MVDAT_CHUNK_SUBTYPE				MAKE_ID('F','R','A','M')
	#define MVDAT_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('F','R','M','[')
	#define MVDAT_SPLIT_END_CHUNK_SUBTYPE	MAKE_ID('F','R','M',']')
#endif

/* MPEG video HALT chunk (for future use). */
#ifndef MVHLT_CHUNK_SUBTYPE
	#define MVHLT_CHUNK_SUBTYPE				MAKE_ID('V','H','L','T')
#endif


typedef	struct MPEGVideoHeaderChunk {
	SUBS_CHUNK_COMMON;					/* 'MPVD', 'VHDR' */
	uint32		version;				/* format version number, currently 1 */
	uint32		maxPictureArea;			/* to preallocate ref frame buffers */
	uint32		framePeriod;			/* nominal, in 90 kHz ticks */
	} MPEGVideoHeaderChunk, *MPEGVideoHeaderChunkPtr;

typedef struct MPEGVideoFrameChunk {
	SUBS_CHUNK_COMMON;					/* 'MPVD', 'FRAM' or 'FRM[' */
	uint32		pts;					/* The frame's presentation time stamp,
										 * expressed in 90kHz clock ticks */
	uint32		compressedVideo[1];		/* start of DYNAMIC SIZE MPEG video data bytes */
	} MPEGVideoFrameChunk, *MPEGVideoFrameChunkPtr;

#if 0	/* This is just a placeholder definition */
typedef struct MPEGVideoHaltChunk {
	SUBS_CHUNK_COMMON;					/* 'MPVD', 'VHLT' */
	int32		haltDuration;			/* How long to wait before replying */
	} MPEGVideoHaltChunk, *MPEGVideoHaltChunkPtr;
#endif

	
/********************************
 * MPEG audio subscriber chunks
 * NOTE: The 2-, 3-, and 4-frame chunks and the Silence chunk are not
 * yet supported and might never be needed.
 ********************************/

#define MPAU_CHUNK_TYPE						MAKE_ID('M','P','A','U')

#define MAHDR_CHUNK_SUBTYPE					MAKE_ID('M','H','D','R')
#define MA1FRME_CHUNK_SUBTYPE				MAKE_ID('1','F','R','M')
#define MA2FRME_CHUNK_SUBTYPE				MAKE_ID('2','F','R','M')
#define MA3FRME_CHUNK_SUBTYPE				MAKE_ID('3','F','R','M')
#define MA4FRME_CHUNK_SUBTYPE				MAKE_ID('4','F','R','M')

#define MA1FRME_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('1','F','R','{')
#define MA2FRME_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('2','F','R','{')
#define MA3FRME_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('3','F','R','{')
#define MA4FRME_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('4','F','R','{')

#define MA1FRME_SPLIT_END_CHUNK_SUBTYPE		MAKE_ID('1','F','R','}')
#define MA2FRME_SPLIT_END_CHUNK_SUBTYPE		MAKE_ID('2','F','R','}')
#define MA3FRME_SPLIT_END_CHUNK_SUBTYPE		MAKE_ID('3','F','R','}')
#define MA4FRME_SPLIT_END_CHUNK_SUBTYPE		MAKE_ID('4','F','R','}')

#define MASILENCE_CHUNK_SUBTYPE				MAKE_ID('S','L','N','C')


typedef struct MPEGAudioHeaderChunk {
	SUBS_CHUNK_COMMON;					/* 'MPAU', 'MHDR' */
	uint32		version; 				/* format version number, currently 1 */
	AudioHeader	audioHdr; 				/* MPEG audio sequence header (32 bits) */
	} MPEGAudioHeaderChunk;

typedef struct MPEGAudioFrameChunk {
	SUBS_CHUNK_COMMON;					/* 'MPAU', '1FRM' to '4FRM' */
	uint32		compressedAudio[1];		/* start of DYNAMIC SIZE bytes of 1 to 4 MPEG audio access units */
	} MPEGAudioFrameChunk;

#define MPEGAUDIO_FRAME_CHUNK_HEADER_SIZE	\
	offsetof(MPEGAudioFrameChunk, compressedAudio)

typedef struct MPEGAudioSilenceChunk {
	SUBS_CHUNK_COMMON;					/* 'MPAU', 'SLNC' */
	uint32		cntPresentationUnits;	/* number of silent presentation units */
	} MPEGAudioSilenceChunk;


/****************************/
/* SAudio subscriber chunks */
/****************************/

#ifndef SNDS_CHUNK_TYPE
	#define	SNDS_CHUNK_TYPE	MAKE_ID('S','N','D','S')
#endif

#ifndef SHDR_CHUNK_TYPE
	#define	SHDR_CHUNK_TYPE	MAKE_ID('S','H','D','R') /* header chunk subtype */
#endif

#ifndef SSMP_CHUNK_TYPE
	#define	SSMP_CHUNK_TYPE	MAKE_ID('S','S','M','P') /* sample chunk subtype */
#endif


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
	SUBS_CHUNK_COMMON;				/* 'SNDS', 'SHDR' */
	uint32	version;				/* format version number */
	int32	numBuffers;				/* max # of buffers that can be */
									/* queued to the AudioFolio */
	int32	initialAmplitude;		/* initial volume of the stream */
	int32	initialPan;				/* initial pan of the stream */
	SAudioSampleDescriptor sampleDesc;	/* sound format info */
	} SAudioHeaderChunk, *SAudioHeaderChunkPtr;

typedef struct SAudioDataChunk {
	SUBS_CHUNK_COMMON;				/* 'SNDS', 'SSMP' */
	int32	actualSampleSize;		/* actual number of bytes in the sample data */
	uint32	samples[1];				/* start of DYNAMIC SIZE audio sample data bytes */
	} SAudioDataChunk, *SAudioDataChunkPtr;

/* The size of the header information that preceeds every audio data chunk.
 * Use this instead of sizeof(SAudioDataChunk). */ 
#define SAUDIO_DATA_CHUNK_HDR_SIZE	(offsetof(SAudioDataChunk, samples))


/****************************/
/* DATA subscriber chunks	*/
/****************************/

/* DataSubscriber data type.   */
#define	DATA_SUB_CHUNK_TYPE				MAKE_ID('D','A','T','A')	

/* data type words we pass to DataAllocMemFcn/DataFreeMemFcn to announce what
 * we're doing with memory */
#define	DATA_HEADER_CHUNK_TYPE			MAKE_ID('D','A','H','D')	
#define	DATA_BLOCK_CHUNK_TYPE			MAKE_ID('B','L','O','K')	

/* Data chunk subtype(s) */
#define	DATA_FIRST_CHUNK_TYPE		MAKE_ID('D','A','T','1')	
#define	DATA_NTH_CHUNK_TYPE			MAKE_ID('D','A','T','n')

/* decompressor identifiers */
#define	DATA_NO_COMPRESSOR			MAKE_ID('N','O','N','E')
#define	DATA_3DO_COMPRESSOR			MAKE_ID('3','D','O','C')

/* Max number of logical channels per subscription */
#define	kDATA_SUB_MAX_CHANNELS	256
#define	kEVERY_DATA_CHANNEL		0x7FFFFFFF
 
/* Subscriber chunk definitions */
typedef struct DataChunkFirst 
{
	SUBS_CHUNK_COMMON;					/* subType = 'DAT1', the first part of a data block */
	uint32		userData[2];			/* two longwords of user data  */
	uint32		signature;				/* unique data signature. must be the same */
										/*  in all pieces (generated by chunking tool) */
	int32		totalDataSize;			/* the total size of the data in all chunks.  if */
										/*  the data is compessed, this represents the size */
										/*  of the UNCOMPESSED data */
	int32		dataSize;				/* the size of the data in this chunk (same   */
										/*  as 'totalDataSize' if only one chunk).  if the */
										/*  chunk is compressed, this represents the size of */
										/*  the COMPRESSED data */
	int32		pieceCount;				/* number of pieces in data block */
	uint32		memTypeBits;			/* what type of memory to allocate for this block */
	uint32		compressor;				/* which decompressor to use for this block */
	
	/* first part of data block goes here... */
} DataChunkFirst, *DataChunkFirstPtr;


typedef struct DataChunkPart
{
	SUBS_CHUNK_COMMON;					/* subType = 'DATn', part of a Data block  */
										/*  OTHER THAN the first */
	uint32		signature;				/* unique Data signature. must be the same */
										/*  in all pieces */
	int32		dataSize;				/* the size of the data in this chunk */
	int32		pieceNum;				/* number of this piece in sequence */

	/* 'pieceNum'th part of data block goes here... */
} DataChunkPart, *DataChunkPartPtr;



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
	SUBS_CHUNK_COMMON;			/* 'SHDR', ? */
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
