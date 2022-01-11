/******************************************************************************
**
**  @(#) mpadecoderinterface.h 96/11/26 1.3
**
******************************************************************************/
#ifndef _MPAUDIODECODERINTERFACE_H
#define _MPAUDIODECODERINTERFACE_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __MISC_MPEG_H
#include <video_cd_streaming/mpeg.h>
#endif

#ifndef __MISC_MPAUDIODECODE_H
#include <video_cd_streaming/mpaudiodecode.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>
#endif

#include "sachannel.h"

enum messageType {
	compressedBfrMsg,		/* new compressed buffer message */
	closeDecoderMsg,		/* close the decoder message */
	decompressedBfrMsg,		/* new decompressed buffer message */
	flushReadMsg,			/* flush out compressed buffers message */
	flushWriteMsg,			/* flush out decompressed buffers message */
	flushReadWriteMsg		/* flush out compressed & decompressed buffers message */
};

/* Decoder message interface */
typedef struct DecoderMsg {
	MinNode				node;
	uint32				channel;			/* active channel */
	int32				branchNumber;		/* for setting presentation clock */
	int32				messageType;		/* Type of this message */
	SubscriberMsgPtr	subMsgPtr;			/* pointer to subscriber message */
	Item				msgItem;			/* message item of this message */
	uint32				*buffer;			/* pointer to buffer */
	int32				size;				/* size of data buffer */
	uint32				presentationTime;	/* in audio ticks */
	uint32				timeIsValidFlag;
	AudioHeader			header;
	void				*Link;				/* back-link to SAudioSubscriberContext */
} DecoderMsg, *DecoderMsgPtr;

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

Item NewMPADecoder( SAudioContextPtr subsCtx, int32 deltaPriority );

#ifdef __cplusplus
}
#endif

#endif /* _MPAUDIODECODERINTERFACE_H */
