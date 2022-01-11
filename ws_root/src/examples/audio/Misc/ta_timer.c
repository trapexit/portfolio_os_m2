
/******************************************************************************
**
**  @(#) ta_timer.c 96/08/27 1.16
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_timer
|||	Demonstrates use of the audio timer.
|||
|||	  Format
|||
|||	    ta_timer
|||
|||	  Description
|||
|||	    This program shows how to examine and change the rate of the audio clock.
|||	    It demonstrates use of cues to signal your task at a specific time. It
|||	    also demonstrates how the audio folio deals with bad audio rate values.
|||
|||	  Controls
|||
|||	    A
|||	        From the main menu runs the audio clock at successively faster rates,
|||	        using SleepUntilAudioTime().
|||
|||	    B
|||	        From the main menu uses SignalAtTime() in conjunction with cues to wait.
|||	        While using this method, you can press A or B to abort one of two cues
|||	        prematurely.
|||
|||	    C
|||	        From the main menu feeds illegal values to SetAudioClockRate().
|||
|||	  Associated Files
|||
|||	    ta_timer.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
**/

#include <audio/audio.h>
#include <kernel/task.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>


#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

/***************************************************************/
Err TestSeveralRates( void )
{
	Item MyClock;
	float32 OriginalRate;
	int32 Duration;
	int32 Result;

	MyClock = CreateAudioClock( NULL );
	CHECKRESULT(MyClock, "CreateAudioClock");

	GetAudioClockRate( MyClock, &OriginalRate);
	PRT(("Original Rate = %g/sec\n", OriginalRate ));

#define TESTRATE(Rate) \
	Result = SetAudioClockRate( MyClock, Rate ); \
	Duration = GetAudioDuration( MyClock ); \
	PRT(("Rate = %g, Duration = %d, Result = 0x%x\n", Rate, Duration, Result ));

	TESTRATE(1050.0);
	TESTRATE(1010.0);
	TESTRATE(990.0);
	TESTRATE(400.0);
	TESTRATE(10.0);
	TESTRATE(2.0);
	TESTRATE(1.0);

	Result = DeleteAudioClock( MyClock );
	CHECKRESULT(Result, "DeleteAudioClock");

cleanup:
	return Result;
}

/***************************************************************/
Err TestRateChange( void )
{
	Item MyClock;
	float32 OriginalRate, Rate;
	Item MyCue;
	AudioTime currentTime;
	int32 Result;
	int32 i;

	MyCue = CreateCue( NULL );
	CHECKRESULT(MyCue, "CreateCue");

	MyClock = CreateAudioClock( NULL );
	CHECKRESULT(MyClock, "CreateAudioClock");

	GetAudioClockRate( MyClock, &OriginalRate);
	PRT(("Original Rate = %g/sec\n", OriginalRate ));
	PRT(("Original Duration = %d\n", GetAudioClockDuration( MyClock ) ));
	Rate = 100.0;

/* Wait for one second at increasingly fast clock rates. */
	for (i=0; i<4; i++)
	{
		SetAudioClockRate(MyClock, Rate );
		PRT(("Sleep for %d ticks.\n", Rate));
		Result = ReadAudioClock( MyClock, &currentTime);
		CHECKRESULT(Result, "ReadAudioClock");
		SleepUntilAudioTime( MyClock, MyCue, Rate + currentTime );
		Rate = Rate*2.0;
	}

	Result = DeleteAudioClock( MyClock );
	CHECKRESULT(Result, "DeleteAudioClock");

cleanup:
	DeleteCue (MyCue);
	return Result;
}

Err WaitForButtonsUp( int32 Button )
{

	ControlPadEventData cped;
	int32 Result;

	Result = GetControlPad (1, FALSE, &cped);
	if (Result < 0) {
		PrintError(0,"get control pad data in","WaitForButtonsUp",Result);
	}

	if( cped.cped_ButtonBits & Button )
	{
		do
		{
			Result = GetControlPad (1, TRUE, &cped);
			if (Result < 0)
			{
				PrintError(0,"get control pad data in","WaitForButtonsUp",Result);
			}
		} while (cped.cped_ButtonBits & Button);
	}
	return Result;
}

/****************************************************************
** Start two timers and watch for their signals to come back.
** Optionally abort them using A or B button.
****************************************************************/
Err TestAbortCue( void )
{
	Item MyCue1, MyCue2=0, MyCue3=0;
	int32 Signal1, Signal2, Signals;
	ControlPadEventData cped;
	uint32 Joy;
	int32 Result;

	MyCue1 = CreateCue( NULL );
	CHECKRESULT(MyCue1, "CreateCue");
	MyCue2 = CreateCue( NULL );
	CHECKRESULT(MyCue2, "CreateCue");
	MyCue3 = CreateCue( NULL );
	CHECKRESULT(MyCue3, "CreateCue");

	Signal1 = GetCueSignal( MyCue1 );
	PRT(("Signal1 = 0x%08x\n", Signal1));

	Signal2 = GetCueSignal( MyCue2 );
	PRT(("Signal2 = 0x%08x\n", Signal2));

	PRT(("A to abort 1, B to abort 2, C to stop looping.\n"));

/* Schedule signals at one and two seconds from now. */
	Result = SignalAtTime( MyCue1, GetAudioTime() + 240 );
	CHECKRESULT(Result, "SignalAtTime");
	Result = SignalAtTime( MyCue2, GetAudioTime() + (2*240) );
	CHECKRESULT(Result, "SignalAtTime");

	do
	{
		Signals = GetCurrentSignals();
		PRT(("Signals = 0x%08x, Time = %d\n", Signals, GetAudioTime() ));

/* Read Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			PrintError(0,"get control pad data in","TestAbortCue",Result);
		}
		Joy = cped.cped_ButtonBits;

		if( Joy & ControlA )
		{
			PRT((" Abort Cue 1\n"));
			Result = AbortTimerCue( MyCue1 );
			CHECKRESULT(Result, "AbortTimerCue 1");
		}
		else if( Joy & ControlB )
		{
			PRT((" Abort Cue 2\n"));
			Result = AbortTimerCue( MyCue2 );
			CHECKRESULT(Result, "AbortTimerCue 2");
		}
		else
		{
			SleepUntilTime( MyCue3, GetAudioTime() + 24 );
		}

	} while ( (Joy & ControlC) == 0);

cleanup:
	DeleteCue (MyCue1);
	DeleteCue (MyCue2);
	DeleteCue (MyCue3);
	return Result;
}

/****************************************************************
** Menu of timer tests.
****************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;
	ControlPadEventData cped;
	uint32 Joy;
	int32 doit;
	int32 ifhelp;

	TOUCH( argc );

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		ERR(("main: error in InitEventUtility\n"));
		PrintError(0,"init event utility",0,Result);
		goto cleanup;
	}

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	PRT(("\nta_timer: Start testing Audio timer.\n"));
	PRT(("Default duration = %d\n", GetAudioDuration() ));

	/* Allow user to select test. */
	doit = TRUE;
	ifhelp = TRUE;

	do
	{
		if( ifhelp)
		{
			PRT(("---------------------------------------------------\n"));
			PRT(("Use Joypad to select timer test...\n"));
			PRT(("   A = Timer rate changes.\n"));
			PRT(("   B = Abort Timer Cue.\n"));
			PRT(("   C = Bad Rate Values.\n"));
			PRT(("   STOP = quit.\n"));
			PRT(("---------------------------------------------------\n"));
			ifhelp = FALSE;
		}

/* Read Control Pad. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"get control pad data",0,Result);
		}
		Joy = cped.cped_ButtonBits;

		switch (Joy)
		{
		case ControlA:
			WaitForButtonsUp( ControlA );
			Result = TestRateChange();
			CHECKRESULT(Result, "TestRateChange");
			ifhelp = TRUE;
			break;

		case ControlB:
			WaitForButtonsUp( ControlB );
			Result = TestAbortCue();
			CHECKRESULT(Result, "TestPoly");
			ifhelp = TRUE;
			break;

		case ControlC:
			Result = TestSeveralRates();
			CHECKRESULT(Result, "TestSeveralRates");
			break;

		case ControlX:
			doit = FALSE;
			break;
		}
	} while (doit);


	PRT(("%s: Finished!\n", argv[0] ));
cleanup:
	KillEventUtility();
	return((int) Result);
}
