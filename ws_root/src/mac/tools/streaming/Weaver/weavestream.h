/******************************************************************************
**
**  @(#) weavestream.h 96/11/20 1.8
**
******************************************************************************/
/*****************************************************************************
 *	File:			WeaveStream.h
 *
 *	Contains:		definitions for WeaveStream.c
 *
 *	Written by:		Joe Buczek & friends
 *
 *
 ******************************************************************************/

#ifndef __STREAMING_WEAVESTREAM_H
#define __STREAMING_WEAVESTREAM_H

#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef __STDIO__
#include <stdio.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
#include <streaming/dsstreamdefs.h>
#endif


#define	MAX_INPUT_STREAMS		20	/* Max number of input files open using standard I/O */
									/* The actual limit set by standard I/O is 12        */

#define	INFINITY_TIME			(0x7fffffff)
#define	AUDIO_TICKS_PER_SEC		((float)(44100./184.))	/* 3DO audio clock rate */

/*************************************************/
/* Input stream descriptor, one per input stream */
/*************************************************/
typedef struct InStream InStream;
typedef struct InStream {
		bool				eof;			/* true when EOF reached */
		long				priority;		/* file data priority */
		uint32				startTime;		/* file data start time in output stream */
		char*				fileName;		/* name of file containing stream data */
		FILE*				fd;				/* Standard I/O file descriptor for reading file data */
#if CACHE_OPEN_FILES
		long				fileOffset;		/* current file offset */
#endif
		/* The following fields are used when splitting a chunk into
		 * two pieces to eliminate fill in the final stream
		 */
		uint32				preSplitSize;	/* the original size of the chunk before splitting */
		uint32				splitOffset;    /* the point in the chunk that we split */
		PackedID			splitEndSubType;/* chunk subtype for last part of split  */

		SubsChunkData		buf;			/* buffer for current header */
		InStream			*next;
	} InStream, *InStreamPtr;


typedef struct StrmChunkNode StrmChunkNode;
typedef struct StrmChunkNode {
	union {
		StreamGoToChunk		gotoChk;
		StreamStopChunk		stopChk;
		StreamTimeChunk		timeChk;
	} chunk;
	StrmChunkNode			*next;
} StrmChunkNode, *StrmChunkNodePtr;


/* Note: TBD. Replace these structs with HaltChunk1 that
   is defined in dsstreamdefs.h
 */
typedef struct SimpleChunk {
	SUBS_CHUNK_COMMON;			/* a simple chunk */
	} SimpleChunk;

typedef struct HaltChunk {
	SimpleChunk		streamerChunk;
	SimpleChunk		subscriberChunk;
	} HaltChunk;

typedef struct HaltChunks HaltChunks;
typedef struct HaltChunks {
	HaltChunk			dataChunk;
	HaltChunks			*next;
	} HaltChunks, *HaltChunksPtr;


/* The following definitions control how the weaver splits chunks
 * to disable this feature, define SPLIT_CHUNK_TYPE as zero.
 * The code below will split a <SPLIT_CHUNK_TYPE, UNSPLIT_CHUNK_SUBTYPE> chunk into
 * a <SPLIT_CHUNK_TYPE, SPLIT_START_SUBTYPE> chunk and a
 * <SPLIT_CHUNK_TYPE, SPLIT_END_SUBTYPE> chunk.
 */
typedef struct TypeSubType_tag
{
	PackedID	type;					/* original chunk type */
	PackedID	subType;				/* original chunk subtype */
	PackedID	splitStartSubType;		/* chunk subtype for firt part of split */
	PackedID	splitEndSubType;		/* chunk subtype for last part of split  */
} TypeSubType;

/***************************************/
/* WeaveStream() input parameter block */
/***************************************/
typedef struct WeaveParams {
		long			ioBufferSize;		/* size of I/O buffers we should allocate */
		long			mediaBlockSize;		/* physical block size of the output media */
		uint32			streamBlockSize;	/* logical buffering block size for stream */
		uint32			outputBaseTime;		/* beginning time for output file */
		char*			outputStreamName;	/* name of output stream file */
		FILE*			outputStreamFd;		/* output stream file descriptor, standard I/O */
		char*			scratchBuf;			/* scratch buffer for file copying */
		SubsChunkData	*fillerChunk;		/* 'FILL' chunk */

		long			numInputStreams;	/* number of input streams */
		InStreamPtr		inStreamList;
#if CACHE_OPEN_FILES
		InStreamPtr		currOpenInStream;	/* currently open input stream */
#endif

		long			earlyChunkMaxTime;	/* maximum amount of time that a chunk will be
											 *  pulled back in time to fill wasted space
											 *  at the end of a block (see function
											 *  "GetBestFitBelowNextMarker()"
											 */
		SubsChunkData	lastChunkWritten;	/* header of last chunk written */

		/**********************/
		/* Stream header info */
		/**********************/

		bool			fWriteStreamHeader;	/* flag: write a stream header */
		long			streamBuffers;		/* number of stream buffers to be used for playback */
		long			audioClockChan;		/* number of audio channel to be used for clock */
		long			enableAudioChan;	/* mask of audio channels to be enabled initially */
		long			streamerDeltaPri;	/* streamer delta priority */
		long			dataAcqDeltaPri;	/* data acquisition delta priority */
		long			numSubsMsgs;		/* number of subscriber messages to allocate */

		long			preloadInstList[DS_HDR_MAX_PRELOADINST];	
											/* NULL terminated table of tags of instruments 
											 * to preload for stream. Definitions of tag
											 * values in SAudioSubscriber.h
											 */

		DSHeaderSubs	subscriberList[DS_HDR_MAX_SUBSCRIBER];
											/* NULL terminated table of tags of subscribers
											 * used in this stream. Subscriber tags are
											 * defined in header files for individual subscribers.
											 */

		/***********************/
		/* Marker related info */
		/***********************/

		bool			fWriteMarkerTable;	/* flag: write marker data to output stream */
		bool			fUpdateMarkerTable;	/* flag to force update of marker table */
		long			maxMarkers;			/* physical size of marker table in # of entries */
		long			numMarkers;			/* actual number of marker entries */
		long			nextMarkerIndex;	/* offset to next marker value to use */
		long			currMarkerIndex;	/* state variable for processing markers */
		MarkerRecPtr	marker;				/* ptr to array of markers to fill in */

		/***************************/
		/* GOTO chunk related info */
		/***************************/
		bool			fWriteGotoChunk;	/* flag: write stop chunk data to output stream */
		StrmChunkNodePtr	gotoChunkList;	/* Linked list of GOTO chunks */

		/***************************/
		/* STOP chunk related info */
		/***************************/
		
		bool			fWriteStopChunk;	/* flag: write stop chunk data to output stream */
		StrmChunkNodePtr	stopChunkList;	/* Linked list of STOP chunks */

		/***************************/
		/* HALT chunk related info */
		/***************************/
		
		bool			fWriteHaltChunk;	/* flag: write halt chunk data to output stream */
		HaltChunksPtr	haltChunkList;		/* Linked list of HALT chunks */

		/***************************/
		/* Chunk Splitting related */
		/***************************/

		uint32			splitTypeCount;		/* size of split type array */
		TypeSubType		*splitTypes;		/* an array of chunk type/subtype which can be 
											 *  split to use up "FILL" chunks 
											 */

		float			timebase;			/* time values in script file are expressed as
											 *  if there are this many ticks pers second, we
											 *  convert to actual tick value.  default = 239.674
											 *  the actual number of audio ticks per second
											 */
	} WeaveParams, *WeaveParamsPtr;


/*******************/
/* Public routines */
/*******************/
bool		OpenDataStreams( WeaveParamsPtr pb );
void		CloseDataStreams( WeaveParamsPtr pb );
long		WeaveStreams( WeaveParamsPtr paramBlk );

#endif	/* __STREAMING_WEAVESTREAM_H */
