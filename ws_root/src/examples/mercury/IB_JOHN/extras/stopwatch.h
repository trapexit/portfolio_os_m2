/*
 *
 *
 */
#ifndef _stopwatch_h_
#define _stopwatch_h_

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <kernel:time.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#endif
#include "assert.h"

/*
 *
 */
typedef struct stopWatch {

	uint32 		frameCount;
	uint32		frameNextReport;
	uint32		frameReport;

	TimeVal 	startTime, endTime, deltaTime;

	uint32		totalFrames;
	float		totalTime;

} stopWatch, *pStopWatch;

/*
 * Prototypes
 */
stopWatch* StopWatch_Construct( void );
void StopWatch_Destruct( stopWatch *self );

void StopWatch_Start( stopWatch *self );
void StopWatch_Cycle( stopWatch *self );

#endif
/* End of File */
