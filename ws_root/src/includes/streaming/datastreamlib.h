#ifndef __STREAMING_DATASTREAMLIB_H
#define __STREAMING_DATASTREAMLIB_H

/******************************************************************************
**
**  @(#) datastreamlib.h 96/03/04 1.14
**
******************************************************************************/

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>
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

bool DSClockLT(uint32 branchNumber1, uint32 streamTime1,
		uint32 branchNumber2, uint32 streamTime2);

bool DSClockLE(uint32 branchNumber1, uint32 streamTime1,
		uint32 branchNumber2, uint32 streamTime2);



#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_DATASTREAMLIB_H */
