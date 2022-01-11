
/******************************************************************************
**
**  @(#) ta_framecount.c 96/03/11 1.3
**
******************************************************************************/

/*
** Run for a long time and make sure that AudioFrameCount stays coherent.
*
** Author: Phil Burk
** Copyright 3DO 1995-
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc()*/

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

Err TestAudioFrameCount( int32 seconds )
{
	int32 Previous, Current;
	int32 Elapsed, Min = 1000000, Max = -1000000;
	uint32 StartFrame, StartTime, ElapsedTime = 0, Duration;
	float32 rate;

	GetAudioClockRate(AF_GLOBAL_CLOCK, &rate);
	Duration = (int32)(rate*seconds);
	StartTime = GetAudioTime();
	StartFrame = Previous = Current = GetAudioFrameCount();

/* Do this for N seconds. */
	while( ElapsedTime < Duration )
	{
		Current = GetAudioFrameCount();
		Elapsed = Current - Previous;
		if( Elapsed > Max ) Max = Elapsed;
		if( Elapsed < Min ) Min = Elapsed;
		Previous = Current;
		ElapsedTime = GetAudioTime() - StartTime;
		Yield();
	};

	PRT(("Frames: Min = %d, Max = %d\n", Min, Max ));
	PRT(("Frames/second = %g\n", (rate*((float32)(Current - StartFrame))) / (ElapsedTime) ));
	if( Min < 0 )
	{
		PRT(("Wraparound detected!\n"));
		return -1;
	}
	return 0;
}

/************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;

	PRT(("Begin %s\n", argv[0] ));
	TOUCH(argc); /* Eliminate anal compiler warning. */

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

	while(TestAudioFrameCount( 10 ) == 0);

	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
