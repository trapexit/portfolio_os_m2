/****************************************************************************
**
**  @(#) mpegbufferqueues.h 96/03/04 1.7
**
*****************************************************************************/
#ifndef __STREAMING_MPEGBUFFERQUEUES_H
#define __STREAMING_MPEGBUFFERQUEUES_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __DEVICE_MPEGVIDEO_H
#include <device/mpegvideo.h>		/* for FMVIOReqOptions, and indirectly IOReq, IOInfo */
#endif

#ifndef	__STREAMING_MEMPOOL_H
#include <streaming/mempool.h>		/* for MemPoolPtr */
#endif

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>	/* for SubscriberMsgPtr */
#endif


/*****************************************************************
 * Stuff necessary to queue a data chunk to the FMV driver.
 *****************************************************************/

typedef struct MPEGBuffer {
	struct	MPEGBuffer*	link;					/* for linking into lists */
	Item				ioreqItem;				/* I/O request item for queuing I/O for this buffer */
	IOReq*				ioreqItemPtr;			/* pointer to the Item in system space */
 	SubscriberMsgPtr	pendingMsg;				/* ptr to msg so we can reply when this chunk is done */
	uint32				bufSize;				/* size of the buffer associated with this request */
	void*				bufPtr;					/* the data buffer for this request */
	FMVIOReqOptions		pts;					/* a place for the FMV Driver to put PTS info */
	Boolean				fUserAbort;				/* Set if I/O was aborted */
	void*				subscriberCtx;			/* back-pointer to the subscriber context */

	Item				forwardMsg;				/* a message item for forwarding this buffer to another thread */
	uint32				displayTime;			/* Time to display this frame relative to the audio clock */

	void*				userData;				/* a client-defined field */
} MPEGBuffer, *MPEGBufferPtr;

typedef struct MPEGBufferQueue {
	MemPoolPtr		bufferPool;					/* pool of MPEGBuffers for use in the queue */

	int32			inuseCount;					/* number of buffers currently in thequeue */
	MPEGBufferPtr	inuseQueueHead;				/* pointer to head of buffers queued */
	MPEGBufferPtr*	inuseQueueTailP;			/* pointer to tail ptr of buffers queued */
} MPEGBufferQueue, *MPEGBufferQueuePtr;

void				InitMPEGBufferQueue (MPEGBufferQueuePtr theQueue);

bool				AddMPEGBufferToTail( MPEGBufferQueuePtr queue, MPEGBufferPtr buffer );
MPEGBufferPtr 		GetNextMPEGBuffer( MPEGBufferQueuePtr queue );
MPEGBufferPtr		DequeueDoneBuffer(MPEGBufferQueuePtr queue);
void				ClearBufferForReUse( MPEGBufferPtr bufferPtr );
void				ClearAndReturnBuffer(MPEGBufferQueuePtr queue, MPEGBufferPtr bufferPtr);
void				AbortQueue(MPEGBufferQueuePtr queue);
int32				CheckQueue(MPEGBufferQueuePtr queue);

					/* [TBD] This procedure is temporary, for coping with AbortIO() problems. */
void				MarkQueueAborted(MPEGBufferQueuePtr queue);

#endif /* __STREAMING_MPEGBUFFERQUEUES_H */

