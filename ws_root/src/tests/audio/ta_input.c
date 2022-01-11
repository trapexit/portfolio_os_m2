/* %Z ta_input.c 96/08/27 1.6 */
/* $Id: ta_input.c,v 1.4 1994/12/23 07:34:15 phil Exp $ */

/**
|||	AUTODOC -private -class tests -group Audio -name ta_input
|||	Tests the audio input.
|||
|||	  Format
|||
|||	    ta_input
|||
|||	  Description
|||
|||	    Tests the audio input by connecting a line_in.dsp(@) to a line_out.dsp(@).
|||	    Also test various illegal calls.
|||
|||	  Location
|||
|||	    tests/Audio
**/

#include <audio/audio.h>
#include <audio/music.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		TOUCH(Result); \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

Err TestAudioInput( void );

int main( int32 argc, char *argv[])
{
	int32 Result;

	TOUCH(argc);

	PRT(("%s V3\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		return Result;
	}

	TestAudioInput();

	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();

	PRT(("%s all done.\n", argv[0]));

	return Result;
}

Err TestAudioInput( void )
{
	Item InputIns;
	Item OutputIns = 0;
	int32 Result;
	TagArg Tags[2];
	ControlPadEventData cped;
	int32 err = 0;

/*  Load ADC input instrument. */
	PRT(("\n======================== Test 1 ===================\n"));
	PRT(("Try loading line_in.dsp without calling EnableAudioInput().\n"));
	InputIns = LoadInstrument( "line_in.dsp", 0, 100);
	if( InputIns < 0 )
	{
		PRT(("LoadInstrument properly rejected line_in.dsp without enable.\n"));
	}
	else
	{
		ERR(("Oops! We allowed a line_in.dsp to be loaded without enabling!\n"));
		err--;
	}

	PRT(("\n======================== Test 2 ===================\n"));
	PRT(("Try passing tags to EnableAudioInput().\n"));
	Tags[0].ta_Tag = TAG_END;
	Result = EnableAudioInput( TRUE, Tags );
	if( Result < 0 )
	{
		PRT(("EnableAudioInput properly barfed on non-zero tags.\n"));
	}
	else
	{
		ERR(("Oops! We allowed tags to be passed to EnableAudioInput()!\n"));
		err--;
	}

	PRT(("\n======================== Test 3 ===================\n"));
	PRT(("Try calling EnableAudioInput() correctly.\n"));
	Result = EnableAudioInput( TRUE, NULL );
	CHECKRESULT(Result,"EnableAudioInput");

/* Returns 1 if enabled. */
	PRT(("Audio Input Enable = %d\n", Result ));

	if( Result == 0 )
	{
		PRT(("Audio input NOT enabled.\n"));
		PRT(("Try to load a line_in.dsp anyway.\n"));
		InputIns = LoadInstrument( "line_in.dsp", 0, 100);
		if( InputIns < 0 )
		{
			PRT(("LoadInstrument properly rejected line_in.dsp without enable.\n"));
		}
		else
		{
			ERR(("Oops! We allowed a line_in.dsp to be loaded without enabling!\n"));
			err--;
		}
		return err;
	}


	PRT(("Audio input enabled.\n"));
	InputIns = LoadInstrument( "line_in.dsp", 0, 100);
	CHECKRESULT(InputIns,"LoadInstrument");

/* Load "line_out" for connecting to DAC. */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Connect output of ADC to DAC. */
	Result = ConnectInstrumentParts (InputIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts 0");
	Result = ConnectInstrumentParts (InputIns, "Output", 1, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts 1");

	Result = StartInstrument( InputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	PRT(("line_in instrument loaded and connected to line_out.\n"));
	PRT(("Signal applied to analog input should be echoed to output.\n"));
	PRT(("Press any button to continue.\n"));

	do
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","TestAudioInput",Result);
			return Result;
		}
	} while( cped.cped_ButtonBits );


	PRT(("\n======================== Test 4 ===================\n"));
	PRT(("Try disabling Audio Input()\n"));
	Result = EnableAudioInput( FALSE, NULL );
	PRT(("Input should now have stopped. Result = %d\n", Result));
	PRT(("!!! NOTE: in reality the input is still going.\n"));
	PRT(("This will be fixed later.\n"));
	PRT(("Press any button to continue.\n"));

	do
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","TestAudioInput",Result);
			return Result;
		}
	} while( cped.cped_ButtonBits );

	StopInstrument(InputIns, NULL);
	StopInstrument(OutputIns, NULL);

cleanup:
	UnloadInstrument( InputIns );
	UnloadInstrument( OutputIns );
	return((int) err);
}



