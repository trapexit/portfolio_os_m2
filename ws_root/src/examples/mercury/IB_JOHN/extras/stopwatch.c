/*
 *
 *
 */
#include "stopwatch.h"

/*
 *
 */
stopWatch* StopWatch_Construct( void )
{
	stopWatch	*self;

    self = (stopWatch*)AllocMem(sizeof(stopWatch), MEMTYPE_NORMAL);
	assert(self);

	self->frameCount = 0;
	self->frameNextReport = 150;
	self->frameReport = self->frameNextReport;

	self->totalFrames = 0;
	self->totalTime = 0.0f;

	return self;
}

/*
 *
 */
void StopWatch_Destruct( stopWatch *self )
{
	assert( self );
	free( self );
}

/*
 *
 */
void StopWatch_Start( stopWatch *self )
{
	SampleSystemTimeTV(&self->startTime);
	self->frameReport = self->frameNextReport;
}

/*
 *
 */
void StopWatch_Cycle( stopWatch *self )
{
	float 		deltaSecs;

	self->frameCount += 1;
	if ( self->frameCount == self->frameReport ) {
		self->frameReport += self->frameNextReport;

		SampleSystemTimeTV(&self->endTime);

		SubTimes(&self->startTime, &self->endTime, &self->deltaTime);
		deltaSecs = ((float)(self->deltaTime.tv_usec
				+ self->deltaTime.tv_sec*1000000)) / 1000000.0;

		self->totalTime += deltaSecs;
		self->totalFrames += self->frameNextReport;

		printf("Frames/Secound: Average( %3.3f ) Recent ( %3.3f )\n",
					((float)self->totalFrames)/self->totalTime,
					((float)self->frameNextReport)/deltaSecs );

		SampleSystemTimeTV(&self->startTime);
	}
}

/* End of File */
