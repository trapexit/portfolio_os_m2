#ifndef __MPEGVIDEOSUPPORT_H
#define __MPEGVIDEOSUPPORT_H

/******************************************************************************
**
**  @(#) mpegvideosupport.h 96/06/05 1.2
**
******************************************************************************/

#include <video_cd_streaming/mpegvideosubscriber.h>
#include <streaming/mempool.h>

#include "mpegvideochannels.h"

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

#ifdef __cplusplus
}
#endif

#endif	/* __MPEGVIDEOSUPPORT_H */
