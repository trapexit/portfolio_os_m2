/* @(#) MPEGVideoActions.h 96/12/11 1.6 */
/* file: MPEGVideoActions.h */
/* definitions mpeg parser actions */
/* 10/1/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef MPEGACTIONS_HEADER
#define MPEGACTIONS_HEADER

#include <kernel/types.h>

#ifndef MPEGVIDEOPARSE_HEADER
#include "MPEGVideoParse.h"
#endif

#ifndef VIDEO_DEVICECONTEXT_HEADER
#include "videoDeviceContext.h"
#endif

int32 DoSlice(tVideoDeviceContext* theUnit, MPEGStreamInfo *context);
int32 DoMPVideoExtensionData( MPEGStreamInfo *context, int32 data );
int32 DoMPVideoUserData( MPEGStreamInfo *context, int32 data );
int32 DoOpenOutputFiles( MPEGStreamInfo *context, char *filePrefix );

#endif
