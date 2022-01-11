/* @(#) MPEGVideoBuffers.h 96/12/11 1.1 */
/* file: MPEGVideoBuffers.h */
/* definitions for buffer management */
/* 12/06/96 Ian Lepore */
/* The 3DO Company Copyright © 1993 */

#ifndef MPEGBUFFERS_HEADER
#define MPEGBUFFERS_HEADER

#include <device/mpegvideo.h>
#include "videoDeviceContext.h"
#include "MPEGVideoParse.h"
#include "M2MPEGUnit.h"


#define DEFAULT_STRIPBUFFERSIZE 		(1024 * 32)
#define DEFAULT_SBSIZECODE 				STRIPBUFFERSIZE32K

Err		CalcBufferInfo(MPEGBufferInfo *bi, int32 fmvWidth, int32 fmvHeight, int32 stillWidth);

int32	AllocDecodeBuffers(tVideoDeviceContext *theUnit, MPEGStreamInfo *si);
void	FreeDecodeBuffers(tVideoDeviceContext* theUnit);

Err		UseStripBuffer(tVideoDeviceContext* theUnit, void *address, uint32 size);
void	UnUseStripBuffer(tVideoDeviceContext* theUnit);

int32	AllocGlobalStripBuffer(tVideoDeviceContext* theUnit);
void	FreeGlobalStripBuffer(void);

Err		AllocStripBuffer(tVideoDeviceContext *theUnit, MPEGBufferInfo *bi);
void	FreeStripBuffer(tVideoDeviceContext *theUnit);

Err		UseReferenceBuffers(tVideoDeviceContext* theUnit, void *address, uint32 size);
void	UnUseReferenceBuffers(tVideoDeviceContext* theUnit);

Err		AllocReferenceBuffers(tVideoDeviceContext *theUnit, MPEGBufferInfo *bi);
void	FreeReferenceBuffers(tVideoDeviceContext* theUnit);

#endif
