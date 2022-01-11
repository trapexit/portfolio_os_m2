/******************************************************************************
**
**  @(#) mpadecoderinterface.h 96/04/12 1.2
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
#include <misc/mpeg.h>
#endif

#ifndef __MISC_MPAUDIODECODE_H
#include <misc/mpaudiodecode.h>
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
	flushReadWriteMsg		/* flush out compressed & decompressed 
							 * buffers message */
};

/* Decoder message interface */
typedef struct DecoderMsg {
	MinNode				node;
	uint32				channel;		/* active channel */
	int32				branchNumber;	/* for setting presentation clock */
	int32				messageType;	/* Type of this message */
	SubscriberMsgPtr	subMsgPtr;		/* pointer to subscriber message */
	/*Item				requestPort;	*//* Msg Port for accepting data and
									 	 * command msgs from the subscriber */
	Item				msgItem;		/* message item of this message */
	uint32				*buffer;		/* pointer to buffer */
	int32				size;			/* size of data buffer */
	uint32				presentationTime;
										/* in audio ticks */
	uint32				timeIsValidFlag;
	AudioHeader			header;
	void				*Link;			/* To link the messges */
} DecoderMsg, *DecoderMsgPtr;

typedef struct DecoderThreadCtx {
  uint32		channel;			/* channel number the data belongs to */
  uint32		branchNumber;		/* branch number used in conjunction with
									 * the presentaionTime when setting the clock */
  MPADecoderContext *decoderCtxPtr;	/* ptr to MPEG audio decoder context */
  Item			msgPort;			/* messages from the subscriber */
  uint32		msgPortSignal;
  List			CompressedBfrQueue;
									/* List of compressed data buffers */
  List			DecompressedBfrQueue;
									/* List of buffers to hold
									 * decompressed data */
  DecoderMsgPtr	curCompressedBufPtr;
									/* ptr to the current compressed buffer */
} DecoderThreadCtx, *DecoderThreadCtxPtr;

 
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
