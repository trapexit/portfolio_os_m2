/****************************************************************************
**
**  @(#) datasubscriber.h 96/06/04 1.1
**
*****************************************************************************/

#ifndef __STREAMING_DATASUBSCRIBER_H__
#define __STREAMING_DATASUBSCRIBER_H__

#include <kernel/types.h>
#include <streaming/datastream.h>
#include <streaming/subscriberutils.h>


/********************/
/* Constants/macros */
/********************/

/* data type words we pass to DataAllocMemFcn/DataFreeMemFcn to announce what we're
 *  doing with memory 
 */
#define	DATA_HEADER_CHUNK_TYPE			MAKE_ID('D','A','H','D')	
#define	DATA_BLOCK_CHUNK_TYPE			MAKE_ID('B','L','O','K')	


/*************/
/*	Types	 */
/*************/

/* typedefs for functions called when subscriber needs to allocate and
 * free memory 
 */
typedef void *(*DataAllocMemFcn) (uint32 size, uint32 typeBits, uint32 chunkType, int32 channel, int32 time);
typedef void *(*DataFreeMemFcn) (void *ptr, uint32 chunkType, int32 channel, int32 time);

/* the "DataContext" structure is allocated and returned to you by "NewDataSubscriber()".
 *  it is a private sturcture which MUST NOT BE MODIFIED.  the subscriber allocates and 
 *  frees memory through the functions pointers listed, so any memory allocated by the
 *  subscriber and freed by you (ie. anything returned by a call to "GetDataChunk()")
 *  should be freed via the "freeMemFcn" function pointer.  
 */
typedef struct _DataContext 
{
	DataAllocMemFcn	allocMemFcn;			/* procedure to call for memory allocation */
	DataFreeMemFcn	freeMemFcn;				/* procedure to call to free memory */
} DataContext, *DataContextPtr;
 

/* typedefs for functions called when subscriber needs to allocate and
 * free memory 
 */
typedef void *(*DataAllocMemFcn) (uint32, uint32, uint32, int32, int32);
typedef void *(*DataFreeMemFcn) (void *, uint32, int32, int32);


/* structure passed to "GetDataChunk()" by the client when polling for new data 
 */
typedef struct DataChunk
{
	void		*dataPtr;				/* the actual data chunk */
	uint32		userData[2];			/* two longwords of user data, from the */
										/*  data chunk header */
	uint32		size;					/* size of this chunk */
	uint32		channel;				/* stream channel this data arrived on */
} DataChunk, *DataChunkPtr;


/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

Err		NewDataSubscriber(DataContextPtr *pCtx, DSStreamCBPtr streamCBPtr, 
								int32 deltaPriority, Item msgItem);
Err		SetDataMemoryFcns(DataContextPtr ctx, DataAllocMemFcn allocMemFcn, 
								DataFreeMemFcn freeMemFcn);
Err		GetDataChunk(DataContextPtr ctx, uint32 channelNumber, DataChunkPtr dataChunkPtr);


#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_DATASUBSCRIBER_H__ */

