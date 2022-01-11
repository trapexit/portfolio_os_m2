/******************************************************************************
**
**  @(#) ta_trigger.c 95/10/17 1.2
**
**  Test Trigger mechanism.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950515 WJB  Created.
**  950526 WJB  Changed for Arm/DisarmTrigger().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/kernel.h>
#include <ctype.h>
#include <stdio.h>


int32 testtrigger ( void );

int main (int argc, char *argv[])
{
	Err errcode;
	int32 nerrors;

	TOUCH(argc);

	if ((errcode = OpenAudioFolio()) < 0) goto clean;

	if ((errcode = nerrors = testtrigger()) < 0) goto clean;

	if (nerrors)
		printf ("%s: test failed with %d errors.\n", argv[0], nerrors);
	else
		printf ("%s: test successful.\n", argv[0]);

clean:
	if (errcode < 0) PrintError (NULL, "execute test", NULL, errcode);
	printf ("%s: done\n", argv[0]);
	CloseAudioFolio();
	return 0;
}


/*************************************************************
** Wait some number of audio frames to allow DSPP to propagate
** values across several instruments.
************************************************************/
int32 WaitAudioFrames( int32 NumFrames )
{
	int32 Start, Current, Elapsed;

	if( NumFrames > 32767 )
	{
		printf("WaitAudioFrames: Frame count is 16 bit, give me a break!\n");
		return -1;
	}
	Start = GetAudioFrameCount();
	do
	{
		Yield();
		Current = GetAudioFrameCount();
		Elapsed = (Current - Start) & 0xFFFF;
	} while ( Elapsed < NumFrames );
	printf("Frame count = %d\n", Current);
	return 0;
}

/************************************************************/
/*
    Results
        0
            Successful test

        >0
            Received this many faults

        <0
            Test couldn't be run. returns Err code.
*/
int32 testtrigger ( void )
{
	Item TriggerIns, AddIns, AddKnob;
	Item trigcue;
	int32 cuesig;
	int32 gotsig;
	Err errcode;
	int32 ErrCnt = 0;

	if ((errcode = trigcue = CreateCue(NULL)) < 0) goto clean;
	cuesig = GetCueSignal (trigcue);

/* Setup add.dsp connected to schmidt_trigger.dsp */
	if ((errcode = AddIns = LoadInstrument("add.dsp",  0, 0)) < 0) goto clean;
	if ((errcode = TriggerIns = LoadInstrument("schmidt_trigger.dsp",  0, 0)) < 0) goto clean;

	if ((errcode = ConnectInstruments( AddIns, "Output", TriggerIns, "Input" )) < 0) goto clean;
	if ((errcode = AddKnob = CreateKnob( AddIns, "InputA", NULL )) < 0) goto clean;

	if ((errcode = StartInstrument( AddIns, NULL )) < 0) goto clean;
	if ((errcode = StartInstrument( TriggerIns, NULL )) < 0) goto clean;

/* Check top make sure clear now. */
	if ((errcode = ArmTrigger (TriggerIns, NULL, trigcue, AF_F_TRIGGER_RESET )) < 0) goto clean;
	WaitAudioFrames( 4 );
	gotsig = GetCurrentSignals() & cuesig;
	if( gotsig )
	{
		printf("ERROR - Got signal before we should have. (1)\n");
		ErrCnt++;
		WaitSignal (cuesig);
	}
	else
	{
		printf("SUCCESS - Didn't signal when we shouldn't have. (1)\n");
	}

/* Tweak knob above set level and look for signal. */
	if ((errcode = SetKnob( AddKnob, 0.06 )) < 0) goto clean;
	WaitAudioFrames( 4 );
	gotsig = GetCurrentSignals() & cuesig;
	if( gotsig == 0 )
	{
		printf("ERROR - Didn't get signal when we should have. (2)\n");
		ErrCnt++;
	}
	else
	{
		printf("SUCCESS - Got signal when we should have. (2)\n");
		WaitSignal (cuesig);
	}

/* Toggle knob and should not see signal. */
	if ((errcode = SetKnob( AddKnob, -0.06 )) < 0) goto clean;
	WaitAudioFrames( 4 );
	if ((errcode = SetKnob( AddKnob, 0.06 )) < 0) goto clean;
	WaitAudioFrames( 4 );
	gotsig = GetCurrentSignals() & cuesig;
	if( gotsig )
	{
		printf("ERROR - Got signal before we should have. (3)\n");
		ErrCnt++;
		WaitSignal (cuesig);
	}
	else
	{
		printf("SUCCESS - Didn't signal when we shouldn't have. (3)\n");
	}


/* Rearm without resetting and look for signal. */
	if ((errcode = ArmTrigger (TriggerIns, NULL, trigcue, 0 )) < 0) goto clean;
	WaitAudioFrames( 4 );
	gotsig = GetCurrentSignals() & cuesig;
	if( gotsig == 0 )
	{
		printf("ERROR - Didn't get signal when we should have. (4)\n");
		ErrCnt++;
	}
	else
	{
		printf("SUCCESS - Got signal when we should have. (4)\n");
		WaitSignal (cuesig);
	}

	return ErrCnt;

clean:
	return errcode;
}
