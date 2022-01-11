/* @(#) mpegdebug.h 96/07/17 1.2 */

#ifndef _MPEGDEBUG_H
#define _MPEGDEBUG_H

#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef _MPEGAUDIOTYPES_H
#include "mpegaudiotypes.h"
#endif


#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


void DEBUGprintparse(FrameInfo *fi, AUDIO_FLOAT *matrixInputSamples );
void DEBUGprintmatrix(FrameInfo *fi, AUDIO_FLOAT *matrixOutputSamples );
void DEBUGprintmatrix2(FrameInfo *fi, AUDIO_FLOAT *matrixOutputSamples );
void DEBUGprintwindow(void);

#endif /* end of #ifndef _MPEGDEBUG_H */
