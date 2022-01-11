/****************************************************************************
**
**  @(#) datachannels.h 96/11/27 1.4
**
** DATASubscriber private defines/prototypes
**
*****************************************************************************/
#ifndef __STREAMING_DATACHANNELS_H
#define __STREAMING_DATACHANNELS_H


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef	__GRAPHICS_GRAPHICS_H
#include <graphics/graphics.h>
#endif

#ifndef __MISC_COMPRESSION_H
#include <misc/compression.h>
#endif

#include <streaming/mempool.h>
#include <streaming/subscriberutils.h>

#include <streaming/datasubscriber.h>
#include <streaming/dsstreamdefs.h>
#include <streaming/subscribertraceutils.h>


/* the compression folio has a bug (in Portfolio 2.0 and earlier), which sometimes
 *  causes it to output one extra word of data when the decompressor is deleted
 *  (described in more detail in a note in the datachannels.c).  define this 
 *  symbol as long as this error exists, to workaround the bug
 */
#define	DEAL_WITH_COMPFOLIO_ERROR

/* there is currently "standard" subscriber code included which isn't
 * actually used.  it is (or should...) be surrounded by 
 * "#ifdef DATA_SUBSCRIBER_UNUSED_CMDS"
 */
#undef	DATA_SUBSCRIBER_UNUSED_CMDS

#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
/************************************************/
/* Skeleton data structures for passing special */
/* control requests to the Data Subscriber.		*/
/* No control requests are implemented, so this */
/* is just a copy of the declarations found in 	*/
/* the Proto Subscriber.						*/
/************************************************/
enum DataControlOpcode 
{
		kDataControlOpTest = 0
};

/* Data accompanying control request */
typedef union DataControlBlock 
{
		struct	 					/* kDataControlOpTest */
		{
			int32	channelNumber;	/* logical channel to control */
			int32	aValue;			/* argument value */
		} test;
} DataControlBlock, *DataControlBlockPtr;
#endif	/* DATA_SUBSCRIBER_UNUSED_CMDS */



/**************************************/
/* Subscriber context, one per stream */
/**************************************/

enum DataContextFlags
{
	DATA_MEM_ALLOCED	= (1 << 1)	/* memory has been allocated, can't change */
									/*  the memory allocation fcns */
};


/* forward declaration for fussy compiler */
typedef struct _DataHead DataHead, *DataHeadPtr;

/* structure we allocate and use to keep track of a data block as it is
 *  being assembled from the stream 
 */
typedef struct _DataHead
{
	DataHeadPtr		nextHeadPtr;	/* linked list of partial blocks */
	uint32			userData[2];	/* user data field copied from data header */
	uint32			chunkType;		/* what type of data block is this? */
	uint32			channel;		/* channel number */
	Decompressor	*decompressor;	/* decompressor engine to use for this block */
	Err				decompErr;		/* error encountered while decompressing data */
	uint32			signature;		/* unique chunk signature, must be the same */
									/*  in all pieces. */
#ifdef DEBUG
	uint32			checkSum;		/* checksum we calculate as data comes in from the stream */
#endif
	uint8			*dataPtr;		/* begining of partial block */
	uint8			*dataWriteAddr;	/* location in dataPtr to copy next chunk of data */
	int32			totalDataSize;	/* the total size of the UNCOMPRESSED data in all chunks */
	int32			pieceCount;		/* number of pieces in this block */
	int32			nextChunkNum;	/* number of next partial block chunk we're */
									/*  expecting, -1 if we're not waiting for */
									/*  anything at all */
#ifdef DEAL_WITH_COMPFOLIO_ERROR
	uint32			badWriteCount;	/* state to deal with compession folio error */
#endif
} DataHead, *DataHeadPtr;



/* a few defines to make bit access into the channel bitmap a bit easier.  if
 *  you change the constant 'kDATA_SUB_MAX_CHANNELS', be aware that we always
 *  use an even number of long words for the bit map so you may as well make
 *  the number of channels a multiple of MAP_SUBSET (32) 
 */
#define	BITS_PER_CHAR		8
#define	MAP_SUBSET			uint32
#define	BITS_PER_SUBSET		(BITS_PER_CHAR * sizeof(MAP_SUBSET))
#define	LONGS_IN_BITMAP	\
	(1 + ((kDATA_SUB_MAX_CHANNELS - 1) / BITS_PER_SUBSET))
	
#ifndef SetFlag
#define SetFlag(val, flag)		((val)|=(flag))
#define ClearFlag(val, flag)	((val)&=~(flag))
#define FlagIsSet(val, flag)	((Boolean)(((val)&(flag))!=0))
#define FlagIsClear(val, flag)	((Boolean)(((val)&(flag))==0))
#endif

/* mostly private data subscriber context.  only the first two fields (mem alloc/free
 *  function pointers) are known to the client through the "DataContext" structure
 *  defined in "datasubscriber.h".  
 *
 * IF THE PUBLIC PORTION OF THIS STRUCTURE IS CHANGED, MAKE SURE "DataContext" IN
 *   "datasubscriber.h" IS CHANGED ACCORDINGLY
 */
typedef struct _PvtDataContext 
{
	DataAllocMemFcn	allocMemFcn;			/* procedure to call for memory allocation */
	DataFreeMemFcn	freeMemFcn;				/* procedure to call to free memory */

	/* all fields below this point are unknown to the client */
	uint32			cookie;					/* magic value used to verify this structure */
	Item			threadItem;				/* subscriber thread item */

	Item			requestPort;			/* message port item for subscriber requests */
	uint32			requestPortSignal;		/* signal to detect request port message arrival */

	DSStreamCBPtr	streamCBPtr;			/* stream to which this subscriber belongs */
			 
	Item			ctxLock;				/* semaphore protects data context */

	uint32			flags;					/* subscriber state */
	SubsQueue		dataMsgQueue;			/* queue of unprocessed data chunks */
	
	DataHeadPtr		completedBlockList;		/* linked list of completed blocks */
	DataHeadPtr		partialBlockList;		/* linked list of partial blocks */

	uint32			chanBitMap[LONGS_IN_BITMAP]; /* map with one bit per channel, used */
											/*  to keep track of the enable/disable */
											/*  state of subscriber channels */
} PvtDataContext, *PvtDataContextPtr;



/*****************************************************************************
 * trace compile-switched implementations
 *****************************************************************************/

/* This switch is a way to quickly change the level of tracing.
 * 	TRACE_LEVEL_1 gives you minimal trace info, 
 *	TRACE_LEVEL_2 gives you more info (includes level 1 trace),
 *	TRACE_LEVEL_3 gives you  maximal info (includes levels 1 and 2 trace).
 */	

#ifdef TRACE_DATA_SUBSCRIBER
	#define TRACE_FILENAME		"DataTraceRawDump.txt"
	#define STATS_FILENAME		"DataTraceStatsDump.txt"
	
	#define DATA_TRACE_LEVEL		3
	
	/* Locate the trace buffer.  It's declared in DataSubscriber.c. */
	extern	TraceBufferPtr	gDataTraceBufPtr;
	
	#if (DATA_TRACE_LEVEL >= 1)
		#define		ADD_DATA_TRACE_L1( bufPtr, value1, value2, value3, value4 )	\
						AddTrace( bufPtr, value1, value2, value3, (void*) value4 )
	#else
		#define		ADD_DATA_TRACE_L1( bufPtr, value1, value2, value3, value4 )
	#endif
	
	#if (DATA_TRACE_LEVEL >= 2)
		#define		ADD_DATA_TRACE_L2( bufPtr, value1, value2, value3, value4 )	\
						AddTrace( bufPtr, value1, value2, value3, (void*) value4 )
	#else
		#define		ADD_DATA_TRACE_L2( bufPtr, value1, value2, value3, value4 )
	#endif
	
	#if (DATA_TRACE_LEVEL >= 3)
		#define		ADD_DATA_TRACE_L3( bufPtr, value1, value2, value3, value4 )	\
						AddTrace( bufPtr, value1, value2, value3, (void*) value4 )
	#else
		#define		ADD_DATA_TRACE_L3( bufPtr, value1, value2, value3, value4 )
	#endif
	
#else
	
	/* Trace is off */
	#define		ADD_DATA_TRACE_L1( bufPtr, value1, value2, value3, value4 )	
	#define		ADD_DATA_TRACE_L2( bufPtr, value1, value2, value3, value4 )	
	#define		ADD_DATA_TRACE_L3( bufPtr, value1, value2, value3, value4 )	

#endif /* TRACE_DATA_SUBSCRIBER */


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* private datasubscriber functions */
extern	uint32	GetStreamClock(PvtDataContextPtr ctx);
extern	void	UnlinkHeader(DataHeadPtr headPtr, DataHeadPtr prevHeadPtr, DataHeadPtr *listFirst);
extern	Err		ProcessMessageQueue(PvtDataContextPtr ctx);
extern	Err		IsDataChanEnabled(PvtDataContextPtr ctx, int32 channelNumber);
extern	Err		SetDataChanEnabled(PvtDataContextPtr ctx, int32 channelNumber, int32 enableChannel);
extern	int32	FlushDataChannel(PvtDataContextPtr ctx, int32 channelNumber);
extern	int32	FreeCompletedDataChannel(PvtDataContextPtr ctx, int32 channelNumber);
extern	int32	CloseDataChannel(PvtDataContextPtr ctx, int32 channelNumber);
extern	int32	ProcessNewDataChunk(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __STREAMING_DATACHANNELS_H */
