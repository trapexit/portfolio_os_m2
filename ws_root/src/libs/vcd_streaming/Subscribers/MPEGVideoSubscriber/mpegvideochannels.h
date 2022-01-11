#ifndef __MPEGVIDEOCHANNELS_H
#define __MPEGVIDEOCHANNELS_H

/******************************************************************************
**
**  @(#) mpegvideochannels.h 96/11/26 1.6
**
**	08/15/96 Ian	Added prevDeltaPTS and prevShuttleDeltaPTS fields to the
**					MPEGVideoContext structure.
******************************************************************************/
 
#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __MISC_MPEG_H
	#include <video_cd_streaming/mpeg.h>
#endif

#include <video_cd_streaming/datastream.h>
#include <video_cd_streaming/mpegvideosubscriber.h>
#include <video_cd_streaming/subscriberutils.h>
#include "fmvdriverinterface.h"
#include <video_cd_streaming/mpegbufferqueues.h>


/****************************************************************
 * Subscriber logical channel state bits
 ****************************************************************/

/* Defines for dealing with channel status bits.
 *
 * Channel Enabled 		- means that a channel is prepared to queue data msgs.
 *					 	  Channel 0 defaults to enabled, but all other channels
 *						  must be explicitly set with a DoSetChan() call.
 *
 * Channel Active  		- means that a "stream started" msg has been received.
 *
 * Channel Initialized 	- means that a header chunk has arrived for this channel.
 */

/* Define a channel status flag to indicate wheather a channel
 * has been initialized.  
 * Bits 0-15 are reserved by the streamer; bit 0 (CHAN_ENABLED) is
 * R/W by the user with DoSetChan(), but bits 1-15 are R/O to the user.  
 * See DataStream.h. */
#define	MPVD_CHAN_INITIALIZED		(1<<16)				

/* Define a mask for subscriber reserved R/O bits.
 * Make sure that the user can't set/unset these bit(s)
 * using DoSetChan().  This mask is ORed with CHAN_SYSBITS
 * in the subscriber's implementation of DoSetChan(). */
#define MPVD_CHAN_SUBSBITS			(0x10000)


#define IsMPEGVideoChanEnabled(x)		((x)->status & CHAN_ENABLED)		
#define IsMPEGVideoChanActive(x)		((x)->status & CHAN_ACTIVE)		
#define IsMPEGVideoChanInitialized(x)	((x)->status & MPVD_CHAN_INITIALIZED)

/* states of the video subscriber - the video can be in Play mode, or shuttle mode */

#define FMV_MODE			0
#define IFRAME_MODE			1

/* the maximum sizes of frames we can process when in FMV or IFRAME mode */

#define FMV_MAX_HSIZE		352
#define FMV_MAX_VSIZE		288
#define IFRAME_MAX_HSIZE	704
#define IFRAME_MAX_VSIZE	576

/******************************************************************************
 *	Action structure
 ******************************************************************************/

#define ACTION_PTS_CLOSE			0
#define ACTION_PTS_LARGER			1
#define ACTION_PTS_LESS				2
#define ACTION_PTS_EQUAL			4

typedef Err (*ActionProc)(void *actionParam);

typedef struct action {
	void*		link;				/* for linking into the list */
	int32		actionType;			/* type of action to take */
	uint32		actionPTS;			/* action time in PTS to occur */
	ActionProc	actionProc;			/* action procedure to call */
	void*		actionParam;		/* parameter to pass to the above action */
} Action, *ActionPtr;

typedef struct actionqueue {
	MemPoolPtr	actionPool;			/* pool of action structures used */
	ActionPtr	head;				/* pointer to the head of the queue */
	ActionPtr	tail;				/* pointer to the tail of the queue */
} ActionQueue, *ActionQueuePtr;

/****************************************************************
 * A subscriber logical channel
 ****************************************************************/

/* Max number of logical channels per subscription */
/* NOTE: Currently we only support one channel, so only allocate one. */
#define	MPVD_SUBS_MAX_CHANNELS		1

typedef struct MPEGVideoChannel {
	uint32					status;				/* state bits */
	SubsQueue				dataMsgQueue;		/* queued data chunks */
	uint32					frameDurationPTS;	/* frame period @ 90 kHz */
	uint32					frameDurationTicks;	/* frame period @ ~ 240 Hz */
	} MPEGVideoChannel, *MPEGVideoChannelPtr;


/*********************************************************
 * A subscriber private context structure (instance data)
 *********************************************************/

typedef struct MPEGVideoContext {
	Item				requestPort;			/* message port item for subscriber requests */
	uint32				requestPortSignal;		/* signal to detect request port messages */

	DSStreamCBPtr		streamCBPtr;			/* stream this subscriber belongs to */
			 
	MPEGVideoChannel	channel[MPVD_SUBS_MAX_CHANNELS];	/* channel objects */
	uint32				activeChannel;			/* the currently active channel number */

	Item				frameClientPort;		/* Message port to send decompressed frames off to */															 
	FrameBufferDesc 	newframeBuffers;		/* FrameBuffers to decompress into */

	FMVDeviceHandle		fmvDevice;				/* handle to FMV device resources */
	uint32				frameBufferDepth;		/* the depth of the frame buffers to decompress into */
	uint32				frameWidth;				/* the width of the frame buffer in pixels */
	uint32				frameHeight;			/* the height of the frame buffer in pixels */
	uint32				frameSize;				/* the size in bytes of the frame buffer */
	
	MPEGBufferQueue		writeQueue;				/* outstanding compressed data writes to FMV driver */
	uint32				writeDoneSignal;		/* MPEG compressed data write completion signal mask */

	MPEGBufferQueue 	readQueue;				/* outstanding decompressed frame reads to FMV driver */
	uint32				readDoneSignal;			/* MPEG decompressed data read completion signal mask */
	
	MPEGBufferQueue		forwardQueue;			/* a queue for frames forwarded to the client thread */
	Item				forwardReplyPort;		/* a message port for replies from the client */
	uint32				forwardReplyPortSignal;	/* a signal associated with forwardReplyPort */

	int32				initFrameNumber;		/* used in associating frame buffers with readQueue pool members */

	uint32				prevBranchNumber;		/* the branchNumber of the previous decompressed frame buffer */
	uint32				prevPTS;				/* the PTS of the previous decompressed frame buffer */
	int32				prevDeltaPTS;			/* the delta of the previous PTS from the one before that. */
	int32				prevShuttleDeltaPTS;	/* the delta of the previous PTS from the one before that when shuttling. */
	
	uint32				mode;					/* 0: Play, 1: Shuttle */
	bool				fShuttling;				/* TRUE: if shuttling */
	
	ActionQueue			actionQueue;			/* responds to end-of-stream or trigger-bit actions */

	uint32				curHSize;				/* current frame hSize (as told to the MPEG Driver) */
	uint32				curVSize;				/* current frame vSize (as told to the MPEG Driver) */

} MPEGVideoContext, *MPEGVideoContextPtr;


/******************************
 * Public function prototypes
 ******************************/

#ifdef __cplusplus 
extern "C" {
#endif

Err  	InitMPEGVideoChannel(MPEGVideoContextPtr ctx,
			MPEGVideoHeaderChunkPtr headerPtr);
Err		StartMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber);
Err		StopMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber);
Err		FlushMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber,
			bool deactivate);
Err		CloseMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber);
Err		ProcessNewMPEGVideoDataChunk(MPEGVideoContextPtr ctx,
			SubscriberMsgPtr subMsg);
Err		EnqueueBranchMPEGVideoChannel(MPEGVideoContextPtr ctx,
			uint32 channelNumber, SubscriberMsgPtr subMsg);


void 	InitActionQueue(ActionQueuePtr theQueue);
void 	AddActionToTail(ActionQueuePtr queue, ActionPtr action);
ActionPtr GetNextAction(ActionQueuePtr queue);
void 	ClearAndReturnAction(ActionQueuePtr queue, ActionPtr actionPtr);
void 	ClearAndReturnAllActions(ActionQueuePtr queue);

#ifdef __cplusplus
} 
#endif

#endif	/* __MPEGVIDEOCHANNELS_H */
