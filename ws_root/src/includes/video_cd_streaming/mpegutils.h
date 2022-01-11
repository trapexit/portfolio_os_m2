#ifndef __MPEGUTILS_H
#define __MPEGUTILS_H

/******************************************************************************
**
**  @(#) mpegutils.h 96/06/05 1.1
**
******************************************************************************/

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MPEG_H__
#include <video_cd_streaming/mpeg.h>
#endif

typedef uint32 MPEGTimestamp;	/* 32-bit PTS (high-order 33rd bit not used) */

#ifdef __cplusplus 
extern "C" {
#endif

uint32 	VideoSequenceHeader_GetFrameRate(VideoSequenceHeaderPtr seqHeader);
uint32 	VideoSequenceHeader_GetVerticalSizes(VideoSequenceHeaderPtr seqHeader);
uint32 	VideoSequenceHeader_GetHorizontalSizes(VideoSequenceHeaderPtr seqHeader);
uint32 	VideoSequenceHeader_GetAspectRatio(VideoSequenceHeaderPtr seqHeader);
uint32 	VideoSequenceHeader_GetBufferSize(VideoSequenceHeaderPtr seqHeader);

uint32 	MPEGTimestampToAudioTicks(MPEGTimestamp mpegTimestamp);
int32 	MPEGDeltaTimestampToAudioTicks(MPEGTimestamp mpegTimestamp);
uint32 	AudioTicksToMPEGTimestamp(uint32 audioTicks);

#ifdef __cplusplus
}
#endif

#endif	/* __MPEGUTILS_H */

