/******************************************************************************
**
**  @(#) sasoundspoolerinterface.h 96/03/29 1.11
**
******************************************************************************/
#ifndef __SASOUNDSPOOLERINTERCE_H
#define __SASOUNDSPOOLERINTERCE_H

#include <kernel/types.h>
#include <streaming/saudiosubscriber.h>

#include "sachannel.h"
#include "mpadecoderinterface.h"

/* Function Prototypes */ 
#ifdef __cplusplus 
extern "C" {
#endif

Err		InitSoundSpooler( SAudioContextPtr ctx, uint32 channelNum,
						DSDataType datatype, int32 numBuffers,
						int32 sampleRate, int32 initialAmplitude );

Err		QueueDecompressedBfrsToSoundSpooler( SAudioContextPtr ctx,
			DecoderMsgPtr decoderMsg );

void	QueueWaitingMsgsToSoundSpooler( SAudioContextPtr ctx,
									  int32 channelNumber );
void	QueueNewAudioBuffer( SAudioContextPtr ctx,
							 SubscriberMsgPtr newSubMsg );
void	HandleCompletedBuffers( SAudioContextPtr ctx ,
								uint32 signalBits);
void	HandleCompletedMPABuffers( SAudioContextPtr ctx ,
								uint32 signalBits);
SoundBufferNode	*FirstActiveNode(SoundSpooler *spooler);


#ifdef __cplusplus
}
#endif

#endif	/* __SASOUNDSPOOLERINTERCE_H */
