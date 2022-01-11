/****************************************************************************
**
**  @(#) mpegvideosubscriber.h 96/03/04 1.18
**
*****************************************************************************/
#ifndef __STREAMING_MPEGVIDEOSUBSCRIBER_H
#define __STREAMING_MPEGVIDEOSUBSCRIBER_H


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>
#endif


/*************
 * Constants
 *************/

/* This subscriber supports stream data with this format version number. */
#define MPVD_STREAM_VERSION		1

/* Video mode attribute definitions */
#define NTSC_FRAME_WIDTH		320
#define NTSC_FRAME_HEIGHT		240
#define NTSC_FIELD_FREQ			 60

/* Max # of frame buffers that the subscriber can use */
#define MPVD_SUBS_MAX_FRAMEBUFFERS		4


/***************************
 * Frame buffer Descriptor
 ***************************/

/* This structure is used to pass an array of frame buffers to the subscriber. */
typedef struct FrameBufferDesc {
	uint32	numFrameBuffers;
	uint32	frameBufferSize;
	uint32	bitDepth;
	void 	*frameBuffer[MPVD_SUBS_MAX_FRAMEBUFFERS];
} FrameBufferDesc, *FrameBufferDescPtr;


/*************************************************
 * Control messages to the MPEG Video Subscriber
 *************************************************/

enum MPEGVideoControlOpcode {
	kMPEGVideoCtlOpFlushClient = 0,			/* Used to release the frame buffers and reset the
											 * client port; this command should break any ties
											 * the subscriber has with a registered client */

	kMPEGVideoCtlOpSetClientPort = 1,		/* Used to set the port on which the application
											 * will receive notification of completed frames */

	kMPEGVideoCtlOpSetFrameBuffers = 2		/* Used to send frame buffer information to the
											 * subscriber to decompress into */
};

typedef union MPEGVideoCtlBlock {
	struct {								/* kMPEGVideoCtlOpSetClientPort */
		Item			clientPort;
	} SetClientPort;
	
	struct {								/* kMPEGVideoCtlOpSetFrameBuffers */
		FrameBufferDesc	frameBuffers;
	} SetFrameBuffers;

} MPEGVideoCtlBlock, *MPEGVideoCtlBlockPtr;


/*****************************
 * Public routine prototypes
 *****************************/
#ifdef __cplusplus 
extern "C" {
#endif

Item	NewMPEGVideoSubscriber(DSStreamCBPtr streamCBPtr, int32 deltaPriority,
			Item msgItem, uint32 bitDepth);

#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_MPEGVIDEOSUBSCRIBER_H */

