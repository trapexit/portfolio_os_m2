/******************************************************************************
**
**  @(#) mpegbufferqueues.c 96/02/13 1.9
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>

#include <kernel/types.h>
#include <kernel/debug.h>

#include <streaming/mpegbufferqueues.h>
#include <streaming/subscribertraceutils.h>


#define TRACE_ENABLED	0

#if TRACE_ENABLED
	extern TraceBufferPtr	MPEGVideoTraceBufPtr;

	#define ADD_TRACE(event, chan, value, ptr)	\
			AddTrace(MPEGVideoTraceBufPtr, event, chan, value, ptr)
	#define ADD_TRACE2(event, chan, value, ptr)	\
			AddTrace(MPEGVideoTraceBufPtr, event, chan, value, ptr)
#else
	#define ADD_TRACE(event, chan, value, ptr)
	#define ADD_TRACE2(event, chan, value, ptr)
#endif


#ifndef CheckIOPtr
#define CheckIOPtr(ioReqItemPtr)	((ioReqItemPtr)->io_Flags & IO_DONE)
#endif


/*******************************************************************************************
 * Initialize a MPEGBufferQueue
 *******************************************************************************************/
void InitMPEGBufferQueue(MPEGBufferQueuePtr theQueue)
	{
	theQueue->bufferPool		= 0;
	theQueue->inuseCount		= 0;
	theQueue->inuseQueueHead	= NULL;
	theQueue->inuseQueueTailP	= &theQueue->inuseQueueHead;
	}
	
/*******************************************************************************************
 * Add a buffer onto the tail of the in use queue of buffers.
 *******************************************************************************************/
bool AddMPEGBufferToTail(MPEGBufferQueuePtr queue, MPEGBufferPtr buffer)
	{
	bool	fQueueWasEmpty;

	fQueueWasEmpty = queue->inuseQueueHead == NULL;
	
	*queue->inuseQueueTailP = buffer;
	*(queue->inuseQueueTailP = &buffer->link) = NULL;
	
	queue->inuseCount++;

	return fQueueWasEmpty;
	}

/*******************************************************************************************
 * Remove a data buffer from the head of the queue of buffers.
 *******************************************************************************************/
MPEGBufferPtr GetNextMPEGBuffer(MPEGBufferQueuePtr queue)
	{
	MPEGBufferPtr	buffer;

	if ( (buffer = queue->inuseQueueHead) != NULL )
		{
		queue->inuseQueueHead = (MPEGBufferPtr)buffer->link;
		if ( queue->inuseQueueHead == NULL )
			queue->inuseQueueTailP = &queue->inuseQueueHead;

		buffer->link = NULL;	/* just to be safe */
		queue->inuseCount--;
		}
		
	return buffer;
	}

/*******************************************************************************************
 * If there is a completed I/O request at the head of a queue, dequeue and return it.
 * Else return NULL.
 *******************************************************************************************/		
MPEGBufferPtr DequeueDoneBuffer(MPEGBufferQueuePtr queue)
	{
	MPEGBufferPtr bufferPtr;
	
	bufferPtr = queue->inuseQueueHead;
	if (bufferPtr != NULL && CheckIOPtr(bufferPtr->ioreqItemPtr))
		{
		ADD_TRACE(kFoundBuffer, 0, bufferPtr->ioreqItem, bufferPtr);
		return GetNextMPEGBuffer(queue);
		}
	
	return NULL;
	}

#if 0	/* This is no longer used. */
/*******************************************************************************************
 * Figure out which buffer completed based on the IOReqItem number returned from
 * the I/O Completion.
 *******************************************************************************************/		
MPEGBufferPtr FindBuffer(MPEGBufferQueuePtr queue, Item IOReqItem) 
	{
	MPEGBufferPtr bufferPtr, *plink;
	
	bufferPtr = *(plink = &queue->inuseQueueHead);
	
	while ( bufferPtr != NULL )
		{
		if ( bufferPtr->ioreqItem == IOReqItem )
			{
			ADD_TRACE(kFoundBuffer, 0, bufferPtr->ioreqItem,
				(long)bufferPtr);
			*plink = bufferPtr->link;		/* unlink buffer */
			/* check for removal of last buffer */
			if (queue->inuseQueueTailP == &bufferPtr->link)
				{
				queue->inuseQueueTailP = plink;
				}
			bufferPtr->link = NULL;
			queue->inuseCount--;
			return bufferPtr; 
			}
		bufferPtr = *(plink = &bufferPtr->link);
		}

	ADD_TRACE2(kFoundWrongBuffer, 0, IOReqItem, (long)queue->inuseQueueHead);
	PERR(("FindBuffer: An MPEG buffer completion was received that's not in the queue!\n"));

	return NULL;
	}
#endif

/*******************************************************************************************
 * Clear out data-specific state from the MPEGBuffer structure. 
 *******************************************************************************************/
void ClearBufferForReUse(MPEGBufferPtr bufferPtr)
	{
	ADD_TRACE(kFreedBuffer, 0, bufferPtr->ioreqItem, bufferPtr);

	bufferPtr->fUserAbort 	= 0;
	
	bufferPtr->link = NULL;
	}

/***********************************************************************
 * Clear a buffer for reuse and return it to the MPEGBufferQueue's pool.
 ***********************************************************************/
void	ClearAndReturnBuffer(MPEGBufferQueuePtr queue, MPEGBufferPtr bufferPtr)
	{
	ClearBufferForReUse(bufferPtr);
	ReturnPoolMem(queue->bufferPool, bufferPtr);
	}

/*******************************************************************************************
 * Reverse the order of buffers in a queue.
 *******************************************************************************************/
static void ReverseQueue(MPEGBufferQueuePtr queue) {
	MPEGBufferPtr		prevBfr, curBfr, nextBfr;
	
	queue->inuseQueueTailP = (queue->inuseQueueHead == NULL) ?
		&queue->inuseQueueHead : &queue->inuseQueueHead->link;
	
	for (	prevBfr = NULL, curBfr = queue->inuseQueueHead;
			curBfr != NULL;
			prevBfr = curBfr, curBfr = nextBfr)
		{
		nextBfr = curBfr->link;
		curBfr->link = prevBfr;
		}
	
	queue->inuseQueueHead = prevBfr;
	}

/*******************************************************************************************
 * AbortIO() and WaitIO() on all buffers in the queue.
 * SIDE EFFECT: This reverses the order of the queue to ensure that aborting the
 *    current I/O request won't make the next one start up--just to be aborted!
 * SIDE EFFECT: This MAY soak up the I/O completion signal, and it WILL soak up their I/O
 *    completion messages, if any [thanks to WaitIO()].
 *******************************************************************************************/
void AbortQueue(MPEGBufferQueuePtr queue) {
	MPEGBufferPtr		curBufferPtr;
	
	ReverseQueue(queue);
	
	/* Abort all these I/O requests. */
	for (curBufferPtr = queue->inuseQueueHead; curBufferPtr != NULL;
			curBufferPtr = curBufferPtr->link)
		{
		ADD_TRACE(kFlushedBuffer, 0, curBufferPtr->ioreqItem, curBufferPtr);
		curBufferPtr->fUserAbort = true;
		AbortIO(curBufferPtr->ioreqItem);
		}
	
	/* Now wait for their completion. Doing this in a second pass should be much faster
	 * because aborting an in-progress request takes longer, and AbortIO(), WaitIO() on
	 * that one practically sets up the next one to be in-progress. Actually, now that
	 * we're reversing the queue, this shouldn't make all that much difference. */
	for (curBufferPtr = queue->inuseQueueHead; curBufferPtr != NULL;
			curBufferPtr = curBufferPtr->link)
		WaitIO(curBufferPtr->ioreqItem);
	}

/*******************************************************************************************
 * Do validity-checks on a queue, when compiled in DEBUG mode.
 * Returns non-zero on an error, could be positive or negative.
 *******************************************************************************************/
long CheckQueue(MPEGBufferQueuePtr queue) {
#ifdef DEBUG
	long ct = 0;
	MPEGBufferPtr endBuffer = NULL, bufferPtr = queue->inuseQueueHead;
	while (bufferPtr != NULL) {
		ct++;
		endBuffer = bufferPtr;
		bufferPtr = bufferPtr->link;
		}
	if (ct != queue->inuseCount) {
		PERR(("CheckQueue: ct (%d) != queue->inuseCount (%d)\n", ct,
			queue->inuseCount));
		return (queue->inuseCount - ct);
		}
	if (ct) {
		if (queue->inuseQueueTailP != &endBuffer->link) {
			PERR(("CheckQueue: queue->inuseQueueTailP != &endBuffer->link\n"));
			return kDSInternalStructureErr;
			}
		}
	else if (	(queue->inuseQueueTailP	!= &queue->inuseQueueHead)
			|| 	(queue->inuseQueueHead != NULL)	) {
			PERR(("CheckQueue: Problem with empty queue\n"));
			return kDSInternalStructureErr;
			}
#define CHECK_THAT_BUFFER_COUNTS_ADD_UP		0	/* are these counts really supposed to add up? */
#if CHECK_THAT_BUFFER_COUNTS_ADD_UP
	if ((queue->bufferPool != NULL) && 
		((queue->inuseCount + queue->bufferPool->numFreeInPool) != queue->bufferPool->numItemsInPool)) {
		PERR(("CheckQueue: inuseCount (%d) + numFreeInPool (%d) != numItemsInPool (%d)\n",
			queue->inuseCount, queue->bufferPool->numFreeInPool, queue->bufferPool->numItemsInPool));
		return((queue->inuseCount + queue->bufferPool->numFreeInPool) - queue->bufferPool->numItemsInPool);
		}
#endif	/* CHECK_THAT_BUFFER_COUNTS_ADD_UP */
#else
	TOUCH(queue);	/* avoid compiler warning */
#endif	/* DEBUG */
	return 0;
	}

