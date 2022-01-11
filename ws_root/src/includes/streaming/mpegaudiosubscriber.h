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
	#include <streaming/datastream.h>
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

