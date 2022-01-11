/* @(#) main.c 96/04/15 1.17 */

/*************************************************
** Fractal Flyer
**
** This program generates a fractal landscape then lets you
** fly around that landscape.
**
** Author: Phil Burk
** Copyright 1995, The 3DO Company
**************************************************/

#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/item.h>
#include <kernel/random.h>
#include <kernel/time.h>
#include <misc/event.h>

#include <audio/audio.h>
#include <audio/music.h>

#include <stdio.h>

/* Application specific includes. */
#include "plane.h"

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define	DBUGQUAD(x)  /* PRT(x) */

#define VERSION     "1.0"

static int32 gIfTurnRight = 0;
#define DBUG_AUTO(x) /* PRT(x) */
#define RANGE_LIMIT_SQUARED  (kGridWidth * kGridWidth * 0.2)
#define MAX_AUTO_PITCH    (0.15)
#define MIN_AUTO_PITCH    (-0.15)

/********************************************************************************/
void RandomizeSeed( void )
{
        uint32          HardSeed;

	HardSeed = ReadHardwareRandomNumber();
	PRT(("HardSeed = 0x%x\n", HardSeed ));
	srand(HardSeed);
}

/********************************************************************************/
uint32 RunAutoPilot( PlaneStatus *plst )
{
	uint32 Buttons = 0;
	float32 range;

/* Maybe adjust pitch. */
	if( plst->plst_Altitude > 800.0 )    /* DIVE? */
	{
		if( plst->plst_Pitch > MIN_AUTO_PITCH )
		{
			Buttons |= ControlUp | ControlB;  /* Yes, UP button makes plane dive DOWN! */
		}
DBUG_AUTO(("Down, altitude = %f, pitch = %f, throttle = %f\n", plst->plst_Altitude,
	plst->plst_Pitch, plst->plst_Throttle));
	}
	else if ( plst->plst_Altitude < 200.0 )   /* CLIMB? */
	{
		if( plst->plst_Pitch < MAX_AUTO_PITCH )
		{
			Buttons |= ControlDown | ControlA;
		}
DBUG_AUTO(("Up, altitude = %f, pitch = %f, throttle = %f\n", plst->plst_Altitude,
	plst->plst_Pitch, plst->plst_Throttle));
	}

/* Maybe adjust Yaw. */
	range = (plst->plst_XPos * plst->plst_XPos) + (plst->plst_YPos * plst->plst_YPos);
	if( range > RANGE_LIMIT_SQUARED )
	{

		if( ((plst->plst_XPos * cosf( plst->plst_Yaw )) > 0.0) ||
		    ((plst->plst_YPos * sinf( plst->plst_Yaw )) > 0.0) )
		{
			Buttons |= gIfTurnRight ? ControlRight : ControlLeft;
DBUG_AUTO(("Turning %d, x = %f, y = %f, yaw = %f\n", gIfTurnRight, plst->plst_XPos, plst->plst_YPos, plst->plst_Yaw));
		}
	}
	else
	{
		gIfTurnRight = ((((uint32)rand()) & 1) == 0);
/* Maybe just randomly turn. */
		if( (((uint32)rand()) & 0xFF) < 4 )
		{
			Buttons |= gIfTurnRight ? ControlRight : ControlLeft;
		}
	}

/* Randomly change landscape. */
	if( ( ((uint32)ReadHardwareRandomNumber()) & 0x7FF ) == 0x1A5 )
	{
		Buttons |= ControlC;
	}
	return Buttons;
}

/********************************************************************************
** GetElapsedTime - Return elapsed time in seconds since last call.
*/
float32 GetElapsedTime( void )
{
	int32           elapsedUSecs;
	static TimeVal  oldTime;
	static int32    ifInit = FALSE;
	
	TimeVal         newTime, deltaTime;
	
/* Start Timer */
	if( !ifInit )
	{
		SampleSystemTimeTV( &oldTime );
		ifInit = TRUE;
	}
	
/* Figure out elapsed time since last frame and frame rate. */
	SampleSystemTimeTV(&newTime);
	SubTimes(&oldTime,&newTime,&deltaTime);
	oldTime = newTime;
	elapsedUSecs = deltaTime.tv_usec + deltaTime.tv_sec*1000000;
	return ((float32) elapsedUSecs / 1000000.0);
}

/********************************************************************************/
void PrintHelp( void )
{
	PRT(("-< ALASKA >- fly over a fractal landscape.\n"));
	PRT((" Version %s\n", VERSION));
	PRT(("   A = increase throttle.\n"));
	PRT(("   B = decrease throttle.\n"));
	PRT(("   C = generate new landscape.\n"));
	PRT(("   To takeoff, press DOWN and A buttons until airborn,\n"));
	PRT(("     then fly using direction pad.\n\n"));
}

/********************************************************************************/

main(int argc, char **argv)
{

	int32           Buttons;
	ControlPadEventData cped;
	int32           Result;
	DemoGraphicsContext DGC;
	Terrain        *tran = NULL;
	PlaneStatus     myPlaneStatus;
	PlaneCharacteristics myPlaneCharacteristics;
	int32           ifAutoPilot = 0;
	float32         elapsedSeconds;

	PrintHelp();
	
#if USE_SOUND
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}
#endif

/* Set up ------ */
	ppInitPlaneStatus( &myPlaneStatus );
	ppCalcPlaneCharacteristics( &myPlaneCharacteristics, kPlaneMaxGs, kPlaneMass, kPlaneMaxVel );
	
/* Move plane back so we can take off easier. */
	myPlaneStatus.plst_XPos = (-0.6) * kGridWidth;

	RandomizeSeed();
	Result = CreateTerrainData( &tran, kNumRows, kNumCols );
	if( Result < 0 )
	{
		PrintError(0,"CreateTerrainData",0,Result);
		goto cleanup;
	}
	GenerateTerrain( tran );

	Result = InitDemoGraphics( &DGC, argc, argv );
	if( Result < 0 )
	{
		PrintError(0,"InitDemoGraphics",0,Result);
		goto cleanup;
	}

#if USE_SOUND
	LoadWindSound();
#endif


	Buttons = 0;
	while ((Buttons & ControlX) == 0)
	{
/* -------- GRAPHICS -------------------------------------- */
		Result = DrawWorld( &DGC, &myPlaneStatus, tran );
		if (Result < 0)
		{
			goto cleanup;
		}

/* -------- CONTROL -------------------------------------- */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","diagnostic",Result);
		}
		Buttons = cped.cped_ButtonBits;

/* ControlStart turns on AutoPilot, any other key turns it off. */
		if( ifAutoPilot)
		{
			if( Buttons & ~ControlStart )
			{
				ifAutoPilot = FALSE;
				PRT(("AutoPilot OFF\n"));
			}
			else
			{
				Buttons = RunAutoPilot( &myPlaneStatus );
			}
		}
		else
		{
			if( Buttons & ControlStart )
			{
				ifAutoPilot = TRUE;
				PRT(("AutoPilot ON\n"));
			}
		}

/* -------- PHYSICS -------------------------------------- */
		elapsedSeconds = GetElapsedTime();
		DGC.dgc_FrameRate = 1.0 / elapsedSeconds;
		
/* Apply control pad to plane. */
		ppControlPlane( &myPlaneStatus, &myPlaneCharacteristics, Buttons, elapsedSeconds );
		
/* Calculate physics of plane. */
		Result = ppDoPlanePhysics( &myPlaneStatus, &myPlaneCharacteristics, elapsedSeconds );
		if( Result < 0 )
		{
			PRT(("------=< CRASH >=--------\n"));
			goto cleanup;
		}
/*		ppReportPlane( &myPlaneStatus ); */


#if USE_SOUND
		ControlWindSound( myPlaneStatus.plst_AirVelocity / kPlaneMaxVel );
#endif

/* Make new Terrain if C hit. */
		if( Buttons & ControlC )
		{
			GenerateTerrain( tran );
		}

	}

cleanup:
	DeleteTerrainData( &tran );
	TermDemoGraphics( &DGC );
#if USE_SOUND
	UnloadWindSound();
	CloseAudioFolio();
#endif
	PRT(("%s finished.\n", argv[0] ));
	return Result;
}

