
/******************************************************************************
**
**  @(#) test_sample.c 96/08/27 1.14
**
******************************************************************************/

/**
|||	AUTODOC -private -class tests -group Audio -name test_sample
|||	Plays an AIFF sample in memory using the control pad.
|||
|||	  Format
|||
|||	    test_sample {-fSample -iInstrument -rRate -nMany -dCalcRateDivider}
|||
|||	  Description
|||
|||	    This program shows how to load an AIFF sample file and play it using the
|||	    control pad. Use the A button to start the sample, the B button to release
|||	    the sample, and the C button to stop the sample. The X button quits the
|||	    program.
|||
|||	    It is similar to playsample(@) but it has more support for testing that we
|||	    didn't want to put in the clean example.
|||
|||	  Arguments
|||
|||	    <sample file>
|||	        Name of a compatible AIFF sample file. Defaults to sinewave.aiff.
|||
|||	    <instrument>
|||	        Name of a DSP instrument used to play sample. Defaults to result of
|||	        SampleItemToInsName().
|||
|||	    [rate]
|||	        Sample rate. Defaults to 44100.
|||
|||	    !!!
|||
|||	  Location
|||
|||	    Tests/Audio
**/

#include <audio/audio.h>
#include <audio/music.h>
#include <audio/parse_aiff.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>         /* strncpy() */

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
DBUG(("%s => 0x%x\n", name, val )); \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}

void PrintHelp( void )
{
	PRT(("Usage: test_sample <-sSamplefile> <-iInstrument> <-rRate>\n"));
}

#define MAX_SAMPLERS  (32)
Item gOutputIns[MAX_SAMPLERS];
Item gSamplerIns[MAX_SAMPLERS];
Item gAttachment[MAX_SAMPLERS];
Item gAmpKnobs[MAX_SAMPLERS];
float32 gCurRates[MAX_SAMPLERS];
float32 gCurAmplitudes[MAX_SAMPLERS];

#define NUM_STEPS   (1000)
/************************************************************/
Err  RampAmplitude( Item AmpKnob, float32 OldAmplitude, float32 NewAmplitude )
{
	int32 i, Result = 0;
	float32 InterpAmplitude;

	for( i=0; i<NUM_STEPS; i++ )
	{
		InterpAmplitude = OldAmplitude + (i * (NewAmplitude-OldAmplitude))/(NUM_STEPS-1);
		Result = SetKnob( AmpKnob, InterpAmplitude );
		CHECKRESULT(Result,"RampAmplitude: SetKnob");
	}
cleanup:
	return Result;
}

/************************************************************/
int main(int argc, char *argv[])
{
	char *SampleName = "sinewave.aiff";
	Item SampleItem = 0;
	float32 Rate = 44100.0;
	float32 SharedAmp,SoloAmp;
	int32 NumIns = 2;
	int32 InsIndex = 0;
	int32 calcRateDivide = 1;
	int32 Result;
	int32 DoIt = TRUE;
	int32 IfVariable = FALSE;
	char InstrumentName[AF_MAX_NAME_SIZE];
	uint32 Buttons;
	ControlPadEventData cped;


	int32 i;
	char c, *s;

	InstrumentName[0] = '\0';

	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 's':
				SampleName = s;
				break;
			case 'i':
				strncpy(InstrumentName, s, AF_MAX_NAME_SIZE);
				break;

			case 'r':
				Rate = (float32) atoi(s);
				IfVariable = TRUE;
				break;

			case 'n':
				NumIns=atoi(s);
				break;

			case 'd':
				calcRateDivide=atoi(s);
				break;

			case '?':
			default:
				PrintHelp();
				exit(1);
				break;
			}
		}
		else
		{
			PrintHelp();
			exit(1);
		}
	}

#if 0
/* Dump name of sample for file name debugging. */
	{
		char *p, cx;
		p = SampleName;
		while(*p != '\0')
		{
			cx = *p++;
			PRT(("cx = %c = 0x%x\n", cx, cx ));
		}
	}
#endif

/* Print menu of button commands. */
	PRT(("Button Menu:\n"));
	PRT(("   A    = Start\n"));
	PRT(("   B    = Release\n"));
	PRT(("   C    = Stop\n"));
	PRT(("   A+SL = UNMute\n"));
	PRT(("   B+SL = Mute\n"));
	PRT(("   C+SL = Solo\n"));
	PRT(("   Up   = Raise Freq\n"));
	PRT(("   Down = Lower Freq\n"));
	PRT(("   Right= Next\n"));
	PRT(("   Left = Previous\n"));
	PRT(("   X = Exit\n"));

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
		goto cleanup;
	}


/* Load digital audio AIFF Sample file from disk. */
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");

/* Look at sample information for fun. */
	DebugSample(SampleItem);

/* Select an apropriate sample playing instrument based on sample format. */
	if( InstrumentName[0] == '\0' )
	{
		Result = SampleItemToInsName( SampleItem , IfVariable, InstrumentName, AF_MAX_NAME_SIZE );
		if (Result < 0)
		{
			ERR(("No instrument to play that sample.\n"));
			goto cleanup;
		}
	}

	PRT(("Play %s on %d copies of %d at 1/%d execution rate.\n",
		SampleName, NumIns, InstrumentName, calcRateDivide ));

	SharedAmp = 1.0/NumIns;
	SoloAmp = (SharedAmp > 0.5) ? SharedAmp : 0.5;

	for( i=0; i<NumIns; i++ )
	{
/* Load an output instrument to send the sound to the DAC. */
		gOutputIns[i] = LoadInstrument("line_out.dsp", calcRateDivide, 100);
		CHECKRESULT(gOutputIns[i],"LoadInstrument");
		StartInstrument( gOutputIns[i], NULL );

/* Load Sampler instrument */
		gSamplerIns[i] = LoadInstrument(InstrumentName,  calcRateDivide, 100);
		CHECKRESULT(gSamplerIns[i],"LoadInstrument");

/* Connect Sampler Instrument to Line_Out. Works for mono or stereo. */
		Result = ConnectInstrumentParts (gSamplerIns[i], "Output", 0, gOutputIns[i], "Input", 0);
		CHECKRESULT(Result,"Connect first channel.");
		Result = ConnectInstrumentParts (gSamplerIns[i], "Output", 1, gOutputIns[i], "Input", 1);
		if( Result < 0 )
		{
/* OK, must not be a stereo instrument. */
			Result = ConnectInstrumentParts (gSamplerIns[i], "Output", 0, gOutputIns[i], "Input", 1);
			CHECKRESULT(Result,"ConnectInstruments");
		}

/* Attach the sample to the instrument for playback. */
		gAttachment[i] = CreateAttachment(gSamplerIns[i], SampleItem, 0);
		CHECKRESULT(gAttachment[i],"AttachSample");

/* Create an Amplitude so we can Mute or Solo instruments. */
		gAmpKnobs[i] = CreateKnob( gSamplerIns[i], "Amplitude", NULL );
		CHECKRESULT(gAmpKnobs[i],"CreateKnob");

		gCurRates[i] = Rate;
		Rate = 0.99 * Rate;  /* Slightly offset rates for moving comb filter. */
		PRT(("Rate = %g\n", Rate ));
		gCurAmplitudes[i] = SharedAmp;
	}


/* Interactive event loop. */
	while(DoIt)
	{
/* Get User input. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","PlaySoundFile",Result);
		}
		Buttons = cped.cped_ButtonBits;

/* Process buttons pressed. */
		if(Buttons & ControlX) /* EXIT */
		{
			DoIt = FALSE;
		}

/*
** Start the instrument at the given SampleRate.
** Note that adjusting the rate of a fixed-rate sample player has no effect.
*/
		if(Buttons & ControlA) /* START */
		{
			if( Buttons & ControlLeftShift )
			{
				PRT(("UNMute #%d\n", InsIndex ));
				Result = RampAmplitude( gAmpKnobs[InsIndex], gCurAmplitudes[InsIndex], SharedAmp );
				CHECKRESULT(Result,"RampAmplitude");
				gCurAmplitudes[InsIndex] = SharedAmp;
			}
			else if( Buttons & ControlRightShift )
			{
				PRT(("Start All\n"));
				for( i=0; i<NumIns; i++ )
				{
					Result = StartInstrumentVA (gSamplerIns[i],
                                        	AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(gCurRates[i]),
                                        	TAG_END);
					CHECKRESULT(Result,"StartInstrument");
					Result = SetKnob(gAmpKnobs[i], SharedAmp );
					CHECKRESULT(Result,"SetKnob");
					gCurAmplitudes[i] = SharedAmp;
				}
			}
			else
			{
				PRT(("Start #%d\n", InsIndex ));
				Result = StartInstrumentVA (gSamplerIns[InsIndex],
                                        AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(gCurRates[InsIndex]),
                                        TAG_END);
				CHECKRESULT(Result,"StartInstrument");
				Result = SetKnob(gAmpKnobs[InsIndex], SharedAmp );
				CHECKRESULT(Result,"SetKnob");
				gCurAmplitudes[InsIndex] = SharedAmp;

			}
		}

		if(Buttons & ControlB) /* RELEASE */
		{
			if( Buttons & ControlLeftShift )
			{
				PRT(("Mute #%d\n", InsIndex ));
				Result = RampAmplitude( gAmpKnobs[InsIndex], gCurAmplitudes[InsIndex], 0.0 );
				CHECKRESULT(Result,"RampAmplitude");
				gCurAmplitudes[InsIndex] = 0.0;
			}
			else if( Buttons & ControlRightShift )
			{

				PRT(("Release All\n"));
				for( i=0; i<NumIns; i++ )
				{
					Result = ReleaseInstrument( gSamplerIns[i], NULL );
					CHECKRESULT(Result,"ReleaseInstrument");
				}
			}
			else
			{
				PRT(("Release #%d\n", InsIndex ));
				Result = ReleaseInstrument( gSamplerIns[InsIndex], NULL );
				CHECKRESULT(Result,"ReleaseInstrument");
			}
		}

		if(Buttons & ControlC) /* STOP */
		{
			if( Buttons & ControlLeftShift )
			{
				PRT(("Solo #%d\n", InsIndex ));
				for( i=0; i<NumIns; i++ )
				{
					if( i == InsIndex )
					{
						Result = RampAmplitude( gAmpKnobs[i], gCurAmplitudes[i], SoloAmp );
						CHECKRESULT(Result,"RampAmplitude");
						gCurAmplitudes[i] = SoloAmp;
					}
					else
					{
						Result = RampAmplitude( gAmpKnobs[i], gCurAmplitudes[i], 0.0 );
						CHECKRESULT(Result,"RampAmplitude");
						gCurAmplitudes[i] = 0.0;
					}
				}
				while(cped.cped_ButtonBits & (ControlLeftShift|ControlC))
				{
					GetControlPad (1, TRUE, &cped);  /* Wait for key up. */
				}
				PRT(("UNSolo #%d\n", InsIndex ));
				for( i=0; i<NumIns; i++ )
				{
					Result = RampAmplitude( gAmpKnobs[i], gCurAmplitudes[i], SharedAmp );
					CHECKRESULT(Result,"RampAmplitude");
					gCurAmplitudes[i] = SharedAmp;
				}
			}
			else if( Buttons & ControlRightShift )
			{
				PRT(("Stop All\n"));
				for( i=0; i<NumIns; i++ )
				{
					Result = RampAmplitude( gAmpKnobs[i], gCurAmplitudes[i], 0.0 );
					CHECKRESULT(Result,"RampAmplitude");
					gCurAmplitudes[i] = 0.0;
					Result = StopInstrument( gSamplerIns[i], NULL );
					CHECKRESULT(Result,"StopInstrument");
				}
			}
			else
			{
				PRT(("Stop #%d\n", InsIndex ));
				Result = RampAmplitude( gAmpKnobs[InsIndex], gCurAmplitudes[InsIndex], 0.0 );
				CHECKRESULT(Result,"RampAmplitude");
				gCurAmplitudes[InsIndex] = 0.0;
				Result = StopInstrument( gSamplerIns[InsIndex], NULL );
				CHECKRESULT(Result,"StopInstrument");
			}
		}
		if(Buttons & ControlRight) /* Next */
		{
			InsIndex++;
			if( InsIndex >= NumIns ) InsIndex = 0;   /* Wraparound */
			PRT(("Ins #%d\n", InsIndex));
		}
		if(Buttons & ControlLeft) /* Previous */
		{
			InsIndex--;
			if( InsIndex < 0 ) InsIndex = NumIns - 1;   /* Wraparound */
			PRT(("Ins #%d\n", InsIndex));
		}
		if(Buttons & ControlUp) /* Raise Pitch */
		{
			gCurRates[InsIndex] *= (4.0/3.0);
			PRT(("Rate = %g\n", gCurRates[InsIndex]));
		}
		if(Buttons & ControlDown) /* Lower Pitch */
		{
			gCurRates[InsIndex] *= (3.0/4.0);
			PRT(("Rate = %g\n", gCurRates[InsIndex]));
		}
	}

cleanup:

	UnloadSample( SampleItem );
	for( i=0; i<NumIns; i++ )
	{
		DeleteAttachment( gAttachment[i] );
		UnloadInstrument( gSamplerIns[i] );
		UnloadInstrument( gOutputIns[i] );
	}

/* Cleanup the EventBroker. */
	KillEventUtility();
	CloseAudioFolio();
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
