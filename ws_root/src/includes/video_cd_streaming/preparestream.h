/******************************************************************************
**
**  @(#) preparestream.h 96/06/04 1.1
**  example stream playback preparation
**
******************************************************************************/
#ifndef	_PREPARESTREAM_H_
#define	_PREPARESTREAM_H_


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAMLIB_H
#include <streaming/datastreamlib.h>
#endif

#ifndef __DSSTREAMDEFS_H
#include <dsstreamdefs.h>
#endif


/*********************
 * Default constants
 *
 * ASSUMES: These thread priorities assume the app is displaying the video frames
 * produced by the video subscriber.
 *********************/

#define kDefaultBlockSize	32768	/* size of stream buffers */
#define kDefaultNumBuffers	4		/* suggested number of stream buffers to use */
#define kStreamerDeltaPri	-10		/* streamer thread delta priority */
#define kDataAcqDeltaPri	-9		/* data acquisition thread delta priority */
#define kNumSubsMsgs		200		/* number of subscriber messages to allocate */

#define kAudioClockChan		0		/* logical channel number of audio clock channel */
#define kEnableAudioChanMask 1		/* mask of audio channels to enable
									 * (enable channel 1) */

#define kSoundsPriority		10		/* audio subscriber thread delta priority */
#define kVideoPriority		-2		/* video subscriber thread delta priority (was kCPakPriority) */


/*****************************/
/* Public routine prototypes */
/*****************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err				FindAndLoadStreamHeader(DSHeaderChunkPtr headerPtr,
					char *streamFileName);
DSDataBufPtr	CreateBufferList(int32 numBuffers, int32 bufferSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _PREPARESTREAM_H_ */
