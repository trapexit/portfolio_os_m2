/****************************************************************************
**
**  @(#) mpegaudiosubscriber.h 96/03/28 1.1
**
*****************************************************************************/
#ifndef __STREAMING_MPEGAUDIOSUBSCRIBER_H
#define __STREAMING_MPEGAUDIOSUBSCRIBER_H


#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
	#include <video_cd_streaming/datastream.h>
#endif


/*************
 * Constants
 *************/

/* This subscriber supports stream data with this format version number. */
#define MPAU_STREAM_VERSION				1

/* The default number of audio buffer to allocate for holding MPEG
 * decompressed data. This number matches the default used by the
 * preptool Audiochunkifier. */
#define NUMBER_MPEG_AUDIO_BUFFER		8

/* --- Values for SAudioContext.userAudChoice --- */
enum MPEGUserAudioChoice { /* these are the only choices */
	kMPEGAudDefault=0,     /* User has not chosen a preference */
	kMPEGAudChooseLeft=1,  /* Mix left source channel to both output channels */
	kMPEGAudChooseRight=2, /* Mix right source channel to both output channels */
	kMPEGAudChooseMono=3,  /* Mix both source channels to both output channels */
	kMPEGAudChooseStereo=4 /* left and right source channels to respective output channels */
};

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

Item	NewMPEGAudioSubscriber( DSStreamCBPtr streamCBPtr, int32 deltaPriority, Item msgItem, uint32 numberOfBuffers );

#ifdef __cplusplus
}
#endif

#endif /* __STREAMING_MPEGAUDIOSUBSCRIBER_H */

