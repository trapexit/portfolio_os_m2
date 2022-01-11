/****************************************************************************
**
**  @(#) e3music_recv.c 95/05/09 1.6
**
****************************************************************************/
/******************************************
** Remote Sound Communications for E3 flying demo
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
******************************************/

#include <kernel/types.h>
#include <stdio.h>
#include <hardware/mac3dolink.h>
#include <audio/audio.h>
#include "e3music.h"
#include "e3music_recv.h"

#define DBUG(x)  /* PRT(x) */
#define FRAC1    (1<<16)
/* This only works if both sides are less than 2**16, use Operamath !!!??? */
#define SCALE_DATA(x,y)  (((x*y)>>16)&0xFFFF)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto error; \
	}

static E3MusicState gLastMusicState;

/****************************************************************/
void e3PollPacket( void )
{
	E3MusicState *e3ms, E3MS;
	
	e3ms = &M3Link_Base->MusicState;

	memset( &gLastMusicState, 0, sizeof(gLastMusicState) );
	e3ms->e3ms_Updates = 0;
	e3ms->e3ms_EngineThrottle = 0;
	e3ms->e3ms_WindSpeed = 0;
	e3ms->e3ms_MusicAmplitude = MAX_MUSIC_AMPLITUDE;
	e3ms->e3ms_Flags = 0;
	while(1)
	{
		/* if( TryGrab3DOLink() && */
		if(	(e3ms->e3ms_Updates != gLastMusicState.e3ms_Updates) )
		{
			E3MS = *e3ms;     /* Make copy that isn't changing. */
		/*	Release3DOLink(); /* Release right away for Mac. */
			e3HandleMusicCommand( e3ms );
		}
		else
		{
			Yield();
		}
	}
}

/****************************************************************/
e3HandleMusicCommand ( E3MusicState *e3ms )
{
	int32 Result;
	int32 Amplitude, Frequency;
	int32 Data;

	if( gLastMusicState.e3ms_MusicAmplitude != e3ms->e3ms_MusicAmplitude )
	{
		int32 Amp;
		Amp = (e3ms->e3ms_MusicAmplitude > 0) ? MAX_MUSIC_AMPLITUDE : 0;
		Result = e3SetMusicAmplitude( Amp );
		CHECKRESULT(Result, "e3SetMusicAmplitude");
	}

/* Map throttle to engine sound. */
	if( gLastMusicState.e3ms_EngineThrottle != e3ms->e3ms_EngineThrottle )
	{
		Data = e3ms->e3ms_EngineThrottle;
		if( Data > FRAC1 ) Data = FRAC1;
		Amplitude = (Data > 10) ? MAX_ENGINE_AMPLITUDE : 0 ;
		Frequency = SCALE_DATA(Data, MAX_ENGINE_FREQUENCY);
		Result = e3ControlEngineSound( Amplitude, Frequency );
		CHECKRESULT(Result, "ControlSingleSound");
DBUG(("Data = 0x%x, Engine Amplitude = 0x%x, Frequency = 0x%x\n", Data, Amplitude, Frequency ));
	}

/* Map WindSpeed to noisy wind sound */
	if( gLastMusicState.e3ms_WindSpeed != e3ms->e3ms_WindSpeed )
	{
		Data = e3ms->e3ms_WindSpeed;
		if( Data > FRAC1 ) Data = FRAC1;
		Amplitude = SCALE_DATA(Data, MAX_WIND_AMPLITUDE);
		Frequency = SCALE_DATA(Data, MAX_WIND_FREQUENCY);
		Result = e3ControlWindSound( Amplitude, Frequency );
		CHECKRESULT(Result, "e3ControlWindSound");
DBUG(("Data = 0x%x, Wind Amplitude = 0x%x, Frequency = 0x%x\n", Data, Amplitude, Frequency ));
	}

/* Capture Edges of flag changes */
DBUG(("gLastMusicState.e3ms_Flags = 0x%x\n", gLastMusicState.e3ms_Flags ));
DBUG(("e3ms->e3ms_Flags = 0x%x\n", e3ms->e3ms_Flags ));
	if( (gLastMusicState.e3ms_Flags ^ e3ms->e3ms_Flags) & E3M_F_MACHINE_GUN )
	{
		if( e3ms->e3ms_Flags & E3M_F_MACHINE_GUN )
		{
			Result = e3StartCannonSound( CANNON_AMPLITUDE, CANNON_FREQUENCY );
		}
		else
		{
			Result = e3StopCannonSound( );
		}
	}

/* Reset on rising edge of RESET flag. */
	if( (~gLastMusicState.e3ms_Flags) & e3ms->e3ms_Flags & E3M_F_RESET )
	{
		PRT(("Resetting E3 music task.\n"));
		e3SetMusicAmplitude( 0 );
		e3ControlEngineSound( 0, 0 );
		e3ControlWindSound( 0, 0 );
		e3StopCannonSound( );
	}
error:
	gLastMusicState = *e3ms;
	return Result;
}


/****************************************************************/
Err e3InitDemoSound( void )
{
	int32 Result;
	if( (Result = e3InitMusic()) < 0) return Result;

	if( (Result = e3LoadEngineSound( E3_ENGINE_FILENAME )) < 0) return Result;
	if( (Result = e3LoadCannonSound( E3_CANNON_FILENAME )) < 0) return Result;
	if( (Result = e3LoadMusic( E3_MUSIC_FILENAME )) < 0) return Result;

	if( (Result = e3StartEngineSound( 0, 0 )) < 0) return Result;
	if( (Result = e3StartWindSound( 0, 0 )) < 0) return Result;
	if( (Result = e3StartMusic( MAX_MUSIC_AMPLITUDE )) < 0) return Result;
	return Result;
}

/****************************************************************/
int main(int32 argc, char **argv)
{

	Err                 err;
	int32  Result;

	err = e3InitDemoSound( );
	if (err < 0)
	{
		printf("Unable to e3InitDemoSound");
		PrintfSysErr(err);
        	return 0;
    	}
	else
	{
		PRT(("E3 demo music slave ready for action......\n"));
	}

	e3PollPacket();

	return Result;
}

