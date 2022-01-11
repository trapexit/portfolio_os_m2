/******************************************************************************
**
**  @(#) saudiosubscriber.h 96/03/04 1.7
**
******************************************************************************/
#ifndef __STREAMING_SAUDIOSUBSCRIBER_H
#define __STREAMING_SAUDIOSUBSCRIBER_H

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
	#include <video_cd_streaming/datastream.h>
#endif


/**********************/
/* Internal constants */
/**********************/

/* stream version supported by subscriber.  Used for sanity */
/* checking when a header chunk is received.                */
#define SAUDIO_STREAM_VERSION	0


/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif

int32	NewSAudioSubscriber( DSStreamCBPtr streamCBPtr, int32 deltaPriority, Item msgItem );

#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_SAUDIOSUBSCRIBER_H */

