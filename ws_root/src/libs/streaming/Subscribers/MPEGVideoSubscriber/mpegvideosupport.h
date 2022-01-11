#ifndef __MPEGVIDEOSUPPORT_H
#define __MPEGVIDEOSUPPORT_H

/******************************************************************************
**
**  @(#) mpegvideosupport.h 96/01/26 1.7
**
******************************************************************************/

#include <streaming/mpegvideosubscriber.h>
#include <streaming/mempool.h>

#include "mpegvideochannels.h"


typedef uint32 MPEGTimestamp;	/* 32-bit PTS (high-order 33rd bit not used) */


#ifdef __cplusplus 
extern "C" {
#endif


bool	InitMPEGWriteBuffer(void *ctx, void *poolEntry);
bool	InitMPEGReadBuffer(void *ctx, void *poolEntry);
bool	FreeMPEGBuffer(void *notUsed, void *poolEntry);

void Free1MPEGVideoMemPool(MemPoolPtr *memPoolPtrPtr);
void DisposeMPEGVideoPools(MPEGVideoContextPtr ctx);

Err BranchMPEGVideoDecoder(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);

Err ProcessMPEGVideoDataQueue(MPEGVideoContextPtr ctx,
		MPEGVideoChannelPtr chan);
Err PrimeMPEGVideoReadQueue(MPEGVideoContextPtr ctx);

uint32 MPEGTimestampToAudioTicks(MPEGTimestamp mpegTimestamp);
int32 MPEGDeltaTimestampToAudioTicks(MPEGTimestamp mpegTimestamp);
uint32 AudioTicksToMPEGTimestamp(uint32 audioTicks);

uint32 GetFrameRateFromSequenceHeader(VideoSequenceHeaderPtr seqHeader);


#ifdef __cplusplus
}
#endif

#endif	/* __MPEGVIDEOSUPPORT_H */
