/******************************************************************************
**
**  @(#) preparestream.c 96/11/26 1.4
**
******************************************************************************/

#include <kernel/debug.h>		/* CHECK_NEG(), ERROR_RESULT_STATUS() */
#include <kernel/types.h>
#include <kernel/mem.h>			/* Alloc/FreeMem variations */

#include <video_cd_streaming/datastream.h>			/* DSDataBufPtr, dserror.h, dsstreamdefs.h */
#include <video_cd_streaming/preparestream.h>

/* The "nice" alignment modulo needed to optimize DMA and CPU cache access.
 * [TBD] Get this value at run-time from the OS */
#define NICE_BUFFER_ALIGNMENT		32

DSDataBufPtr	CreateBufferList(int32 numBuffers, int32 bufferSize)
	{
	int32			totalHeaderSize, eachBufferSize;
	DSDataBufPtr	firstBp;
	DSDataBufPtr	bp;
	int32			bufferNum;
	uint8			*bufferAddr;

	if ( numBuffers < 2 )
		return NULL;

	/* Compute the size needed for an array DSDataBuf[numBuffers], plus
	 * padding out to a "nice" boundary, plus an array of numBuffers
	 * data buffers, each padded out to a "nice" boundary. */
	totalHeaderSize = ALLOC_ROUND(numBuffers * sizeof(DSDataBuf),
		NICE_BUFFER_ALIGNMENT);
	eachBufferSize  = ALLOC_ROUND(bufferSize, NICE_BUFFER_ALIGNMENT);

	/* Try to allocate the needed memory. */
	firstBp = (DSDataBufPtr)AllocMemAligned(
		totalHeaderSize + numBuffers * eachBufferSize,
		MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL,
		NICE_BUFFER_ALIGNMENT);
	if ( firstBp == NULL )
		return NULL;

	/* Split the big allocation into an array DSDataBuf[numBuffers] and an
	 * array of data buffers with "nice" alignment. Link them all together. */
	for ( bp = firstBp, bufferAddr = (uint8 *)firstBp + totalHeaderSize,
			bufferNum = numBuffers;
			bufferNum > 0;
			++bp, bufferAddr += eachBufferSize, --bufferNum )
		{
		bp->next = bp + 1;
		bp->streamData = bufferAddr;
		}
	bp[-1].next = NULL;

	/* Return a pointer to the first buffer in the list  */
	return firstBp;
	}


Err	FindAndLoadStreamHeader(DSHeaderChunkPtr headerPtr, char *fileName)
{
	/* <HPP> for videocd we don't have any header chunk */
	TOUCH(headerPtr);
	TOUCH(fileName);
	return kDSHeaderNotFound;
}

