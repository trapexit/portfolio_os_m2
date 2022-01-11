#ifndef __STREAMING_DATASTREAMLIB_H
#define __STREAMING_DATASTREAMLIB_H

/******************************************************************************
**
**  Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
**  All rights reserved. This material contains confidential
**  information that is the property of The 3DO Company. Any
**  unauthorized duplication, disclosure or use is prohibited.
**  
**  @(#) datastreamlib.h 96/11/26 1.6
**  Distributed with Portfolio V33.7
**
******************************************************************************/

#ifndef __STREAMING_DATASTREAM_H
#include <video_cd_streaming/datastream.h>
#endif

#ifdef __cplusplus 
extern "C" {
#endif

bool	FillPoolWithMsgItems(MemPoolPtr memPool, Item replyPort);

Err	DSSubscribe(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, DSDataType dataType,
		Item subscriberPort);

Err	DSPreRollStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, uint32 asyncBufferCnt);

Err	DSStartStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, uint32 startOptions);

Err	DSStopStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, uint32 stopOptions);

Err	DSGoMarker(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, uint32 markerValue,
		uint32 options);

Err	DSGetChannel(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, DSDataType streamType,
		uint32 channelNumber, uint32 *channelStatusPtr);

Err	DSSetChannel(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, DSDataType streamType,
		uint32 channelNumber, uint32 channelStatus, uint32 mask);

Err	DSControl(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, DSDataType streamType,
		int32 userDefinedOpcode, void *userDefinedArgPtr);

Err	DSConnect(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, Item acquirePort);

Err	DSWaitEndOfStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr);

Err	DSWaitTriggerBit(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr);

Err	DSWaitStartOfStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr);

bool DSClockLT(uint32 branchNumber1, uint32 streamTime1,
		uint32 branchNumber2, uint32 streamTime2);

bool DSClockLE(uint32 branchNumber1, uint32 streamTime1,
		uint32 branchNumber2, uint32 streamTime2);

Err	DSShuttle( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
		int32 rate );

Err	DSPlay( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr );

Err DSFrameDisplayed(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr);

Err DSDoHighResStills(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, Boolean doHighRes);

#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_DATASTREAMLIB_H */
