
/******************************************************************************
**
**  @(#) long_time.c 96/08/27 1.2
**
******************************************************************************/

/*
** long_time.c - Check timer wrap-around.
*
** Author: Phil Burk
** Copyright 3DO 1995-
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <kernel/time.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc()*/

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PRT(("error code = 0x%x\n",Result)); \
			PrintError(0,"\\failure in",msg,Result); \
			goto clean; \
		} \
	} while(0)

/* Macro to simplify error checking. */
#define CALL_REPORT(_exp,msg) \
	do \
	{ \
		int32 err; \
		if ((err = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg, err); \
		} \
	} while(0)


/* Macro to simplify error checking. */
#define PASS_FAIL(_exp,msg) \
	do \
	{ \
		PRT(("============================================\n")); \
		PRT(("Please wait for test to complete. %s\n", msg)); \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\ERROR in",msg,Result); \
		} else { \
			PRT(("SUCCESS in %s\n", msg )); \
		} \
	} while(0)


/****************************************************************
** Report Elapsed Time
****************************************************************/
static void ReportTime( const char *msg, uint32 elapsedTime, float32 rate )
{
	uint32 seconds;
	uint32 minutes;
	uint32 hours;
	uint32 days;

	PRT(("%s - elapsed time in ticks = 0x%x = %d.", msg, elapsedTime, elapsedTime ));
	seconds = elapsedTime / rate;
	minutes = seconds / 60;
	seconds -= minutes * 60;

	hours = minutes / 60;
	minutes -= hours * 60;

	days = hours / 24;
	hours -= days * 24;
	PRT((" = %d days + %d hours + %d minutes + %d seconds.\n", days, hours, minutes, seconds ));
}

/****************************************************************
** Sleep on multiple Cues
****************************************************************/
static Err WaitLongTime( void )
{
	Item    MyClock;
	float32 globalRate, customRate;
	AudioTime globalStartTime, globalStopTime, globalElapsedTime;
	AudioTime customStartTime, customStopTime, customElapsedTime;
	ControlPadEventData cped;
	int32 Result;

	CALL_CHECK( (MyClock = CreateAudioClock(NULL)), "create custom clock");
	CALL_CHECK( (Result = GetAudioClockRate(MyClock, &customRate)), "get custom clock rate");
	CALL_CHECK( (Result = GetAudioClockRate(AF_GLOBAL_CLOCK, &globalRate)), "get custom clock rate");

/* Get start times. */
	CALL_CHECK( (Result = ReadAudioClock(AF_GLOBAL_CLOCK, &globalStartTime)), "read custom clock");
	CALL_CHECK( (Result = ReadAudioClock(MyClock, &customStartTime)), "read custom clock");

/* Loop on button presses. */
	PRT(("\nPress A to print elapsed time. X to quit.\n"));
	do
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","WaitLongTime",Result);
			return Result;
		}

/* Sleep until A button pressed. */
		if(cped.cped_ButtonBits & ControlA)
		{
/* check times. */
			CALL_CHECK( (Result = ReadAudioClock(AF_GLOBAL_CLOCK, &globalStopTime)), "read custom clock");
			CALL_CHECK( (Result = ReadAudioClock(MyClock, &customStopTime)), "read custom clock");
			globalElapsedTime = globalStopTime - globalStartTime;
			customElapsedTime = customStopTime - customStartTime;
			ReportTime( "Global Clock", globalElapsedTime, globalRate );
			ReportTime( "Custom Clock", customElapsedTime, customRate );
			PRT(("\nPress A to print elapsed time. X to quit.\n"));
		}
	} while( (cped.cped_ButtonBits & ControlX) == 0);

clean:
	DeleteAudioClock(MyClock);

	return Result;
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

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		return Result;
	}

	PASS_FAIL( (WaitLongTime()), "WaitLongTime" );

	CloseAudioFolio();
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
