/* @(#) sound_wind.c 96/02/21 1.3 */
/******************************************
** Wind sound based on filtered noise.
** Does not use samples.
**
** Author: Phil Burk
** Copyright (c) 1995 3DO
** All Rights Reserved
**
*******************************************/

#include <audio/audio.h>
#include <audio/musicerror.h>
#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <audio/patch.h>
#include <stdio.h>

#include "sound_wind.h"

/* -------------------- Macros */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto error; \
	}

#define USE_PATCH

static Item  gWindAmplitudeKnob;
static Item  gWindFrequencyKnob;
static Item  gWindModDepthKnob;
static Item  gWindModRateKnob;
static Item  gWindOutputIns;

#ifdef USE_PATCH
static Item  gWindPatchTmp;
static Item  gWindPatchIns;
/****************************************************************/
Err UnloadWindSound( void )
{
	UnloadInstrument( gWindOutputIns );
	DeletePatchTemplate( gWindPatchTmp );
	return 0;
}
/****************************************************************/
static Item LoadWindPatch( void )
{
	int32 Result;
	PatchCmdBuilder *pb = NULL;
	Item  windPatchTmp;
	const Item windNoiseTmp  = LoadInsTemplate ("noise.dsp", NULL);
	const Item windFilterTmp = LoadInsTemplate ("svfilter.dsp", NULL);
	const Item windRedNoiseTmp  = LoadInsTemplate ("rednoise.dsp", NULL);
	const Item windTimesPlusTmp = LoadInsTemplate ("timesplus.dsp", NULL);

	printf ("LoadWindPatch()\n");

		/* note: special failure path to avoid closing if we didn't open */
	if ((Result = OpenAudioPatchFolio()) < 0) return Result;

	if ((Result = CreatePatchCmdBuilder (&pb)) < 0) goto error;

	DefinePatchKnob (pb, "Frequency", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 0.07);
	DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
	DefinePatchKnob (pb, "ModDepth", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
	DefinePatchKnob (pb, "ModRate", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
	DefinePatchPort (pb, "Output",    1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);

	AddTemplateToPatch (pb, "noise_ins",  windNoiseTmp);
	AddTemplateToPatch (pb, "filter_ins",  windFilterTmp);
	AddTemplateToPatch (pb, "rednoise_ins",  windRedNoiseTmp);
	AddTemplateToPatch (pb, "timesplus_ins",  windTimesPlusTmp);

/* Make internal signal connections. */
	ConnectPatchPorts (pb, "noise_ins", "Output", 0, "filter_ins", "Input", 0);
	ConnectPatchPorts (pb, "rednoise_ins", "Output", 0, "timesplus_ins", "InputA", 0);
	ConnectPatchPorts (pb, "timesplus_ins", "Output", 0, "filter_ins", "Frequency", 0);

/* Connect exported knobs to patch. */
	ConnectPatchPorts (pb, NULL, "Amplitude", 0, "filter_ins", "Amplitude", 0);
	ConnectPatchPorts (pb, NULL, "Frequency", 0, "timesplus_ins", "InputC", 0);
	ConnectPatchPorts (pb, NULL, "ModDepth", 0, "timesplus_ins", "InputB", 0);
	ConnectPatchPorts (pb, NULL, "ModRate", 0, "rednoise_ins", "Frequency", 0);

/* Connect output of filter. */
	ConnectPatchPorts (pb, "filter_ins", "Output", 0, NULL, "Output", 0);

	SetPatchConstant (pb, "filter_ins", "Resonance", 0, 0.12);
	SetPatchConstant (pb, "noise_ins", "Amplitude", 0, 0.3);

	windPatchTmp = CreatePatchTemplateVA (GetPatchCmdList (pb),
		TAG_ITEM_NAME,  "wind",
		TAG_END);
	CHECKRESULT(windPatchTmp,"CreatePatchTemplateVA");
	Result = windPatchTmp;

error:
	DeletePatchCmdBuilder (pb);
	UnloadInsTemplate (windNoiseTmp);
	UnloadInsTemplate (windFilterTmp);
	UnloadInsTemplate (windRedNoiseTmp);
	UnloadInsTemplate (windTimesPlusTmp);
	CloseAudioPatchFolio();
	return Result;
}

/****************************************************************/
Err LoadWindSound( void )
{
	int32 Result;

	gWindPatchTmp = LoadWindPatch();
	CHECKRESULT(gWindPatchTmp,"LoadWindPatch");

	gWindPatchIns = CreateInstrument( gWindPatchTmp, NULL );
	CHECKRESULT(gWindPatchIns,"CreateInstrument");

/* Load an instrument to send the sound to the DAC. */
	gWindOutputIns = LoadInstrument("line_out.dsp", 0, 100);
	CHECKRESULT(gWindOutputIns,"LoadInstrument");

/* Connect Instrument to DirectOut. */
	Result = ConnectInstrumentParts (gWindPatchIns, "Output", 0, gWindOutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (gWindPatchIns, "Output", 0, gWindOutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

	gWindAmplitudeKnob = CreateKnob( gWindPatchIns, "Amplitude", NULL );
	CHECKRESULT(gWindAmplitudeKnob, "CreateKnob");

	gWindFrequencyKnob = CreateKnob( gWindPatchIns, "Frequency", NULL );
	CHECKRESULT(gWindFrequencyKnob, "CreateKnob");

	gWindModDepthKnob = CreateKnob( gWindPatchIns, "ModDepth", NULL );
	CHECKRESULT(gWindModDepthKnob, "CreateKnob");

	gWindModRateKnob = CreateKnob( gWindPatchIns, "ModRate", NULL );
	CHECKRESULT(gWindModRateKnob, "CreateKnob");

	Result = StartInstrumentVA( gWindPatchIns,
	                            AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(0.0),
	                            AF_TAG_FREQUENCY_FP, ConvertFP_TagData(0.0),
	                            TAG_END);
	CHECKRESULT(Result,"StartInstrument gWindPatchIns");

	Result = StartInstrument( gWindOutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument output");

	PRT(("LoadWindSound complete.\n"));
	return Result;

error:
	UnloadWindSound();
	return Result;
}
#else
static Item  gWindResonanceKnob;
static Item  gWindNoiseIns;
static Item  gWindFilterIns;
static Item  gWindRedNoise;
static Item  gWindTimesPlus;
/****************************************************************/
Err UnloadWindSound( void )
{
	StopInstrument( gWindFilterIns, NULL );
	UnloadInstrument( gWindOutputIns );
	UnloadInstrument( gWindFilterIns );
	UnloadInstrument( gWindNoiseIns );
	UnloadInstrument( gWindRedNoise );
	UnloadInstrument( gWindTimesPlus );
	return 0;
}
/****************************************************************/
Err LoadWindSound( void )
{
	int32 Result;

/* Load an instrument to send the sound to the DAC. */
	gWindOutputIns = LoadInstrument("line_out.dsp", 0, 100);
	CHECKRESULT(gWindOutputIns,"LoadInstrument");

/* Load Noise instrument */
	gWindNoiseIns = LoadInstrument("noise.dsp",  0, 100);
	CHECKRESULT(gWindNoiseIns,"LoadInstrument");
/* Load Filter instrument */
	gWindFilterIns = LoadInstrument("svfilter.dsp",  0, 100);
	CHECKRESULT(gWindFilterIns,"LoadInstrument");

/* Load Red Noise generator for natural feel. */
	gWindRedNoise = LoadInstrument("rednoise.dsp",  0, 100);
	CHECKRESULT(gWindRedNoise,"LoadInstrument");
	gWindTimesPlus = LoadInstrument("timesplus.dsp",  0, 100);
	CHECKRESULT(gWindTimesPlus,"LoadInstrument");

/* Connect Noise to filter. */
	Result = ConnectInstruments (gWindNoiseIns, "Output", gWindFilterIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");

/* Add Red Noise to Wind Frequency using TimesPlus */
	Result = ConnectInstruments (gWindRedNoise, "Output", gWindTimesPlus, "InputA");
	CHECKRESULT(Result,"ConnectInstruments");

	Result = ConnectInstruments (gWindTimesPlus, "Output", gWindFilterIns, "Frequency");
	CHECKRESULT(Result,"ConnectInstruments");


/* Grab knobs for continuous control. */
	gWindAmplitudeKnob = CreateKnob( gWindFilterIns, "Amplitude", NULL );
	CHECKRESULT(gWindAmplitudeKnob, "CreateKnob");

	gWindModRateKnob = CreateKnob( gWindRedNoise, "Frequency", NULL );
	CHECKRESULT(gWindModRateKnob, "CreateKnob");

	gWindModDepthKnob = CreateKnob( gWindTimesPlus, "InputB", NULL );
	CHECKRESULT(gWindModDepthKnob, "CreateKnob");

	gWindFrequencyKnob = CreateKnob( gWindTimesPlus, "InputC", NULL );
	CHECKRESULT(gWindFrequencyKnob, "CreateKnob");

/* Connect Instrument to DirectOut. */
	Result = ConnectInstrumentParts (gWindFilterIns, "Output", 0, gWindOutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (gWindFilterIns, "Output", 0, gWindOutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* A lower number will whistle more. */
	gWindResonanceKnob = CreateKnob( gWindFilterIns, "Resonance", NULL );
	CHECKRESULT(gWindResonanceKnob, "CreateKnob");
	Result = SetKnob( gWindResonanceKnob, 0.12 );
	CHECKRESULT(Result,"SetKnob");

	Result = StartInstrument (gWindRedNoise, NULL);
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument (gWindTimesPlus, NULL);
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrumentVA (gWindNoiseIns,
	                            AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(0.3),
	                            TAG_END);
	Result = StartInstrumentVA (gWindFilterIns,
	                            AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(0.0),
	                            AF_TAG_FREQUENCY_FP, ConvertFP_TagData(0.0),
	                            TAG_END);
	CHECKRESULT(Result,"StartInstrumentVA");
	Result = StartInstrument( gWindOutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument output");

	PRT(("LoadWindSound complete.\n"));
	return Result;

error:
	UnloadWindSound( );
	return Result;
}
#endif

/****************************************************************/
Err ControlWindSound( float32 fractionalVelocity )
{
	int32 Result;
	float32 filterFreq = fractionalVelocity * 0.07;

	Result = SetKnob( gWindAmplitudeKnob, fractionalVelocity * 1.0 );
	CHECKRESULT(Result,"SetKnob gWindAmplitudeKnob");
DBUG(("Wind Frequency = 0x%x\n", Frequency ));
	Result = SetKnob( gWindFrequencyKnob, filterFreq );
	CHECKRESULT(Result,"SetKnob gWindFrequencyKnob");
	Result = SetKnob( gWindModDepthKnob, filterFreq * 0.1 );
	CHECKRESULT(Result,"SetKnob gWindModDepthKnob");
	Result = SetKnob( gWindModRateKnob, (fractionalVelocity * 10.0) + 10.0);
	CHECKRESULT(Result,"SetKnob gWindModRateKnob");

#if 0
	{
		float32 Val;
		Item myProbe = CreateProbe( gWindTimesPlus, "Output", NULL );
		Result = ReadProbe( myProbe, &Val );
		CHECKRESULT(Result,"ReadProbe");
		PRT(("Probe = %g\n", Val ));
		DeleteProbe( myProbe );
	}
#endif

error:
	return Result;
}

