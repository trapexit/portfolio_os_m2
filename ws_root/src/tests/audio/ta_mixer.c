/* @(#) ta_mixer.c 96/07/29 1.10 */
/***************************************************************
**
** Create a mixer using the new M2 CreateMixerTemplate() call.
** Test it by connecting a bad tone and a good tone and fiddling
** with gains.
**
** By:  Phil Burk
**
** Copyright (c) 1994, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <audio/audio.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>

#define GOOD_INS_NAME "triangle.dsp"
#define BAD_INS_NAME "square.dsp"
#define DEFAULT_NUM_INPUTS     (8)
#define DEFAULT_NUM_OUTPUTS    (2)
#define DEFAULT_FLAGS          (0)

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


/*********************************************************************/
void PrintHelp( void )
{
	printf("Usage: ta_mixer {options}\n");
	printf("   -iNumInputs     = number of INput channels (default = %d)\n", DEFAULT_NUM_INPUTS);
	printf("   -oNumOutputs    = number of OUTput channels (default = %d)\n", DEFAULT_NUM_OUTPUTS);
	printf("   -fFlags         = MixerSpecFlags (default = 0)\n");
}


/**********************************************************************
** Capture output of instrument and do CheckSum
**********************************************************************/
int32 TestMixer( int32 numInputs, int32 numOutputs, uint32 flags )
{

/* Declare local variables */
    const MixerSpec mixerSpec = MakeMixerSpec(numInputs, numOutputs, flags);
	Item    GoodIns;
	Item    BadIns=0;
	Item    MixerTmp=0;
	Item    MixerIns=0;
	Item    LineOuts[AF_MIXER_MAX_OUTPUTS/2];
	Item    FreqKnob=0;
	Item    GainKnob;
	Item    SleepCue=0;
	int32   inputIndex, outputIndex, i;
	float32 Rate;
	int32   Result;

/* Load oscillator instruments. */
	GoodIns = LoadInstrument( GOOD_INS_NAME,  0, 100);
	CHECKRESULT(GoodIns,"LoadInstrument");
	BadIns = LoadInstrument( BAD_INS_NAME,  0, 100);
	CHECKRESULT(BadIns,"LoadInstrument");

/* Create mixer */
	MixerTmp = CreateMixerTemplate( mixerSpec, NULL );
	CHECKRESULT(MixerTmp,"CreateMixerTemplate");

/* Make instrument from template. */
	MixerIns = CreateInstrument( MixerTmp, NULL );
	CHECKRESULT(MixerIns,"CreateInstrument Mixer");

/* Create a Cue for delays. */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue,"CreateCue");

/* Load line_out instrument for DAC connection. 1 for each pair of outputs. */
	for( i=0; i<((numOutputs+1)/2); i++ )
	{
		LineOuts[i] = LoadInstrument( "line_out.dsp",  0, 100);
		CHECKRESULT(LineOuts[i],"LoadInstrument");

		Result = StartInstrument( LineOuts[i], NULL );
		CHECKRESULT(Result,"StartInstrument");
	}

/* Play bad sound so we know what it sounds like. */
	PRT(("This is the BAD sound.\n"));
	Result = ConnectInstrumentParts( BadIns, "Output", 0, LineOuts[0], "Input", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts( BadIns, "Output", 0, LineOuts[0], "Input", 1 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = StartInstrumentVA( BadIns, AF_TAG_FREQUENCY_FP, ConvertFP_TagData(60.0), TAG_END );
	CHECKRESULT(Result,"StartInstrument");
	SleepUntilTime( SleepCue, GetAudioTime() + 240 );
	Result = StopInstrument( BadIns, NULL );
	CHECKRESULT(Result,"StopInstrument");
	DisconnectInstrumentParts( LineOuts[0], "Input", 0 );
	DisconnectInstrumentParts( LineOuts[0], "Input", 1 );

/* Grab Frequency knob so we can change freq for each channel. */
	FreqKnob = CreateKnob( GoodIns, "Frequency", NULL );
	CHECKRESULT(FreqKnob,"CreateKnob");
/* Grab Gain knob so we can change freq for each channel. */
	GainKnob = CreateKnob( MixerIns, "Gain", NULL );
	CHECKRESULT(GainKnob,"CreateKnob");

/* Connect Mixer to LineOut */
	for( i=0; i<((numOutputs+1)/2); i++ )
	{
		Result = ConnectInstrumentParts( MixerIns, "Output", i*2, LineOuts[i], "Input", 0 );
		if( Result < 0 )
		{
			ERR(("Mixer must not have have 'Output' port.\n"));
			break;
		}
		if( (i*2+1) < numOutputs )
		{
			Result = ConnectInstrumentParts( MixerIns, "Output", i*2+1, LineOuts[i], "Input", 1 );
			if( Result < 0 )
			{
				ERR(("Mixer must not have have 'Output' port.\n"));
				break;
			}
		}
	}

	Result = StartInstrument( MixerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Connect bad sound to all other inputs to check for "bleedthrough". */
	for( inputIndex=0; inputIndex<numInputs; inputIndex++ )
	{
		Result = ConnectInstrumentParts( BadIns, "Output", 0, MixerIns, "Input", inputIndex );
		CHECKRESULT(Result,"ConnectInstrumentParts for Mixer");
	}

/* Start good instrument. */
	Result = StartInstrument( GoodIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Play good tone on each input channel, scan across outputs. */
	Rate = 100.0;
	for( inputIndex=0; inputIndex<numInputs; inputIndex++ )
	{
		Result = SetKnob( FreqKnob, Rate );
		CHECKRESULT(Result,"SetKnob");

/* Disconnect bad and connect good. */
		Result = DisconnectInstrumentParts( MixerIns, "Input", inputIndex );
		CHECKRESULT(Result,"ConnectInstrumentParts for Mixer");
		Result = ConnectInstrumentParts( GoodIns, "Output", 0, MixerIns, "Input", inputIndex );
		CHECKRESULT(Result,"ConnectInstrumentParts for Mixer");

		for( outputIndex=0; outputIndex<numOutputs; outputIndex++ )
		{
			const int32 partNum = CalcMixerGainPart (mixerSpec, inputIndex, outputIndex);

/* Turn up Gain. */
			PRT(("Hear GOOD sound. Part = %d, Rate = %g\n", partNum, Rate));
			Result = SetKnobPart( GainKnob, partNum, 1.0  );
			CHECKRESULT(Result,"SetKnobPart for Gain");
			SleepUntilTime( SleepCue, GetAudioTime() + 240 );

/* Turn down Gain. */
			Result = SetKnobPart( GainKnob, partNum, 0.0);
			CHECKRESULT(Result,"SetKnobPart for Gain");
			SleepUntilTime( SleepCue, GetAudioTime() + 120 );
		}

/* Disconnect good then reconnect bad. */
		Result = DisconnectInstrumentParts( MixerIns, "Input", inputIndex );
		CHECKRESULT(Result,"ConnectInstrumentParts for Mixer");
		Result = ConnectInstrumentParts( BadIns, "Output", 0, MixerIns, "Input", inputIndex );
		CHECKRESULT(Result,"ConnectInstrumentParts for Mixer");

		Rate = Rate * 1.1;
	}

cleanup:
	DeleteKnob( FreqKnob );
	UnloadInstrument( GoodIns );
	UnloadInstrument( BadIns );
	DeleteCue( SleepCue );
	for( i=0; i<((numOutputs+1)/2); i++ )
	{
		UnloadInstrument( LineOuts[i] );
	}
	PRT(("Result of deleting mixer instrument = 0x%x\n", DeleteInstrument( MixerIns ) ));
	PRT(("Result of deleting mixer template = 0x%x\n", DeleteMixerTemplate( MixerTmp ) ));

	return((int) Result);
}

/***********************************************************************/
int main(int argc, char *argv[])
{
	char    *s, c;
	int32   i;
	int32   Result;
	int32   numInputs, numOutputs;
	uint32  flags;

	numInputs = DEFAULT_NUM_INPUTS;
	numOutputs = DEFAULT_NUM_OUTPUTS;
	flags = DEFAULT_FLAGS;


/* Get input parameters. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 'i':
				numInputs = atoi(s);
				break;
			case 'o':
				numOutputs = atoi(s);
				break;
			case 'f':
				flags = atoi(s);
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


/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	DBUG(("ta_mixer:  numInputs = %d, numOutputs = %d, flags = 0x%x\n",
	    numInputs, numOutputs, flags ));

	Result = TestMixer( numInputs, numOutputs, flags );
	PRT(("Result = 0x%x\n", Result ));

	CloseAudioFolio();
	return (int) Result;
}
