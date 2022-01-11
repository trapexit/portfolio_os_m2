/* @(#) mpVideoDecode.h 96/12/11 1.4 */
/* 	file: mpVideoDecode.h
	MPEG Video Decoder definitions
	2/28/95 George Mitsuoka
	The 3DO Company Copyright 1995 */

#ifndef MPVDECODE_HEADER
#define MPVDECODE_HEADER

#include <kernel/types.h>
#ifndef VIDEO_DEVICECONTEXT_HEADER
#include "videoDeviceContext.h"
#endif

void mpVideoDecode(tVideoDeviceContext *theUnit);

#endif

