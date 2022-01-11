#ifndef __MPEGVIDEOCHANNELS_H
#define __MPEGVIDEOCHANNELS_H

/******************************************************************************
**
**  @(#) mpegvideochannels.h 96/03/15 1.11
**
******************************************************************************/
 
#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __MISC_MPEG_H
	#include <misc/mpeg.h>
#endif

#include <streaming/datastream.h>
#include <streaming/mpegvideosubscriber.h>
#include <streaming/subscriberutils.h>
#include "fmvdriverinterface.h"
#include <streaming/mpegbufferqueues.h>


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

#ifdef __cplusplus
} 
#endif

#endif	/* __MPEGVIDEOCHANNELS_H */
