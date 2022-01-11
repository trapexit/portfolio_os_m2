/******************************************************************************
**
**  @(#) sound3d_patch.c 96/02/29 1.8
**
**  3D Sound Patch Builder
**
**-----------------------------------------------------------------------------
**
**  Initials:
**
**  PLB: Phil Burk (phil)
**  WJB: Bill Barton (peabody)
**  RNM: Robert Marsanyi (rnm)
**
*****************************************************************/

#include <audio/audio.h>
#include <audio/patch.h>
#include <audio/sound3d.h>
#include <stdio.h>

#define	PRT(x) { printf x; }
#define	ERR(x) PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define	DBUG2(x) /* PRT(x) */

Item MakeSound3DPatchTemplate (uint32 s3dFlags)
{
	Item tmpl_Send=-1, tmpl_Tap=-1, tmpl_Filter=-1, tmpl_Shift=-1,
	  tmpl_TimesPlus=-1, tmpl_Minus=-1, tmpl_Amp=-1, tmpl_EnvAmp=-1;
	PatchCmdBuilder *pb = NULL;
	Item dl_Buffer=-1;
	Item result, patch=-1;

	/* Open AudioPatch Folio() (note special clean up to avoid closing if we didn't open it) */
	if ((result = OpenAudioPatchFolio()) < 0) return result;

	/* Create PatchCmdBuilder */
	if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

/*-----------------------------------------------------------------------------

Delay Block:

Delay line feeds two channels.  For each channel, sampler drift minus desired
Delay is scaled by 256, then multiplied by DelayRate.  After subtracting 1.0,
the result is fed back to the SampleRate knob of the sampler.  This control
feedback loop allows the sampler to move to the desired tap point at a
controllable rate.

Amplitude panning may be provided using the sampler Amplitude knobs.  If
S3D_F_SMOOTH_AMPLITUDE is enabled, envelopes are used to generate ramped
changes in amplitude. 

Knobs

  Delay (2 channels)
    Desired tap length for each channel.
  
  DelayRate (1 channel)
    Rate at which SampleRate is changed to generate desired delay.
  
  Amplitude (2 channels)
    Output amplitude of delay block.  Used for directional sound cues.

  EnvRate (2 channel, optional)
    If S3D_F_SMOOTH_AMPLITUDE is enabled, this knob controls the rate at which
    amplitude changes.

Probes

  DriftProbe (2 channels)
    The current drift value for each sampler.
  
  AmpProbe (2 channels, optional)
    If S3D_F_SMOOTH_AMPLITUDE is enabled, this is current amplitude value for
    each sampler.
    
Inputs

  Input (1 channel)
    Mono source input

Outputs

  Output (2 channels)
    Left and Right delayed signals

-----------------------------------------------------------------------------*/

	/* Load, add and connect constituent templates, knobs, etc */
	if (s3dFlags & S3D_F_PAN_DELAY)
	{
		if ((result = tmpl_Send	=       LoadInsTemplate	("delay_f1.dsp", NULL))	< 0) goto clean;
		if ((result = tmpl_Tap =        LoadInsTemplate	("sampler_drift_v1.dsp", NULL))	< 0) goto clean;
		if ((result = tmpl_Shift =      LoadInsTemplate	("times_256.dsp", NULL)) < 0) goto clean;
		if ((result = tmpl_TimesPlus =  LoadInsTemplate	("timesplus_noclip.dsp", NULL))	< 0) goto clean;
		if ((result = tmpl_Minus =      LoadInsTemplate	("subtract.dsp", NULL))	< 0) goto clean;

		/* "Declare" all the patch delay components */
		AddTemplateToPatch (pb,	"Send",	tmpl_Send);
		AddTemplateToPatch (pb,	"LeftTap", tmpl_Tap);
		AddTemplateToPatch (pb,	"RightTap", tmpl_Tap);
		AddTemplateToPatch (pb,	"LShift", tmpl_Shift);
		AddTemplateToPatch (pb,	"LTimesPlus", tmpl_TimesPlus);
		AddTemplateToPatch (pb,	"LMinus", tmpl_Minus);
		AddTemplateToPatch (pb,	"RShift", tmpl_Shift);
		AddTemplateToPatch (pb,	"RTimesPlus", tmpl_TimesPlus);
		AddTemplateToPatch (pb,	"RMinus", tmpl_Minus);

		/* Expose FIFOs	for attaching delay line */
		ExposePatchPort	(pb, "SendFIFO", "Send", "OutFIFO");
		ExposePatchPort	(pb, "LeftTapFIFO", "LeftTap", "InFIFO");
		ExposePatchPort	(pb, "RightTapFIFO", "RightTap", "InFIFO");

		/* Declare the probe ports */
		DefinePatchPort	(pb, "Drift", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_WHOLE_NUMBER);

		/* Declare the delay control knobs */
		DefinePatchKnob	(pb, "DelayRate", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, -1.0);
		DefinePatchKnob	(pb, "Delay", 2, AF_SIGNAL_TYPE_WHOLE_NUMBER, 0.0);
		DefinePatchKnob	(pb, "Amplitude", 2, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);

		/* Extra patch components for enveloped	amplitude */
		if (s3dFlags & S3D_F_SMOOTH_AMPLITUDE)
		{
			if ((result = tmpl_EnvAmp = LoadInsTemplate ("envelope.dsp", NULL)) < 0) goto clean;

			AddTemplateToPatch (pb,	"REnvAmp", tmpl_EnvAmp);
			AddTemplateToPatch (pb,	"LEnvAmp", tmpl_EnvAmp);

			DefinePatchPort	(pb, "AmpProbe", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
			DefinePatchKnob	(pb, "EnvRate",	1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
		}

		/* Connect the delay feedback loop */
		ConnectPatchPorts (pb, "LeftTap", "Drift", 0, "LMinus",	"InputA", 0);
		ConnectPatchPorts (pb, "LMinus", "Output", 0, "LShift",	"Input", 0);
		ConnectPatchPorts (pb, "LShift", "Output", 0, "LTimesPlus", "InputA", 0);
		ConnectPatchPorts (pb, NULL, "DelayRate", 0, "LTimesPlus", "InputB", 0);
		SetPatchConstant (pb, "LTimesPlus", "InputC", 0, -1.0);
		ConnectPatchPorts (pb, "LTimesPlus", "Output", 0, "LeftTap", "SampleRate", 0);

		ConnectPatchPorts (pb, "RightTap", "Drift", 0, "RMinus", "InputA", 0);
		ConnectPatchPorts (pb, "RMinus", "Output", 0, "RShift",	"Input", 0);
		ConnectPatchPorts (pb, "RShift", "Output", 0, "RTimesPlus", "InputA", 0);
		ConnectPatchPorts (pb, NULL, "DelayRate", 0, "RTimesPlus", "InputB", 0);
		SetPatchConstant (pb, "RTimesPlus", "InputC", 0, -1.0);
		ConnectPatchPorts (pb, "RTimesPlus", "Output", 0, "RightTap", "SampleRate", 0);

		/* Attach knobs	and probes */
		ConnectPatchPorts (pb, NULL, "Delay", AF_PART_LEFT, "LMinus", "InputB",	0);
		ConnectPatchPorts (pb, NULL, "Delay", AF_PART_RIGHT, "RMinus", "InputB", 0);

		if (s3dFlags & S3D_F_SMOOTH_AMPLITUDE)
		{
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_LEFT,	"LEnvAmp", "Env.Request", 0);
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_RIGHT, "REnvAmp", "Env.Request", 0);
			ConnectPatchPorts (pb, NULL, "EnvRate",	0, "LEnvAmp", "Env.Incr", 0);
			ConnectPatchPorts (pb, NULL, "EnvRate",	0, "REnvAmp", "Env.Incr", 0);
			ConnectPatchPorts (pb, "LEnvAmp", "Output", 0, "LeftTap", "Amplitude", 0);
			ConnectPatchPorts (pb, "REnvAmp", "Output", 0, "RightTap", "Amplitude",	0);
			ConnectPatchPorts (pb, "LEnvAmp", "Output", 0, NULL, "AmpProbe", AF_PART_LEFT);
			ConnectPatchPorts (pb, "REnvAmp", "Output", 0, NULL, "AmpProbe", AF_PART_RIGHT);
		}
		else
		{
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_LEFT,	"LeftTap", "Amplitude",	0);
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_RIGHT, "RightTap", "Amplitude", 0);
		}

		ConnectPatchPorts (pb, "LeftTap", "Drift", 0, NULL, "Drift", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RightTap", "Drift", 0, NULL, "Drift", AF_PART_RIGHT);

		/* Touch amplitude Item so compiler doesn't complain */
		TOUCH(tmpl_Amp);
	}

/*-----------------------------------------------------------------------------

Amplitude Block:

If the delay block is not used, a simple gain stage for each channel can be
used to provide amplitude panning.  If S3D_F_SMOOTH_AMPLITUDE is enabled,
envelopes are used to generate ramped changes in amplitude.

Knobs

  Amplitude (2 channels)
    Output amplitude of delay block.  Used for directional sound cues.

  EnvRate (2 channel, optional)
    If S3D_F_SMOOTH_AMPLITUDE is enabled, this knob controls the rate at which
    amplitude changes.

Probes

  AmpProbe (2 channels, optional)
    If S3D_F_SMOOTH_AMPLITUDE is enabled, this is current amplitude value for
    each sampler.
    
Inputs

  Input (1 channel)
    Mono source input

Outputs

  Output (2 channels)
    Left and Right amplitude-panned signals

-----------------------------------------------------------------------------*/

	else if (s3dFlags & S3D_F_PAN_AMPLITUDE)
	{
		if ((result = tmpl_Amp = LoadInsTemplate ("multiply.dsp", NULL)) < 0) goto clean;

		AddTemplateToPatch (pb,	"LAmp",	tmpl_Amp);
		AddTemplateToPatch (pb,	"RAmp",	tmpl_Amp);

		DefinePatchKnob	(pb, "Amplitude", 2, AF_SIGNAL_TYPE_GENERIC_SIGNED, -1.0);

		/* Extra patch components for enveloped	amplitude */
		if (s3dFlags & S3D_F_SMOOTH_AMPLITUDE)
		{
			if ((result = tmpl_EnvAmp = LoadInsTemplate ("envelope.dsp",	NULL)) < 0) goto clean;

			AddTemplateToPatch (pb,	"REnvAmp", tmpl_EnvAmp);
			AddTemplateToPatch (pb, "LEnvAmp", tmpl_EnvAmp);

			DefinePatchKnob	(pb, "EnvRate",	1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
			DefinePatchPort (pb, "AmpProbe", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
		}

		if (s3dFlags & S3D_F_SMOOTH_AMPLITUDE)
		{
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_LEFT,	"LEnvAmp", "Env.Request", 0);
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_RIGHT, "REnvAmp", "Env.Request", 0);
			ConnectPatchPorts (pb, NULL, "EnvRate", 0, "LEnvAmp", "Env.Incr", 0);
			ConnectPatchPorts (pb, NULL, "EnvRate",	0, "REnvAmp", "Env.Incr", 0);
			ConnectPatchPorts (pb, "LEnvAmp", "Output", 0, "LAmp", "InputB", 0);
			ConnectPatchPorts (pb, "REnvAmp", "Output", 0, "RAmp", "InputB", 0);
			ConnectPatchPorts (pb, "LEnvAmp", "Output", 0, NULL, "AmpProbe", AF_PART_LEFT);
			ConnectPatchPorts (pb, "REnvAmp", "Output", 0, NULL, "AmpProbe", AF_PART_RIGHT);
		}
		else
		{
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_LEFT,	"LAmp",	"InputB", 0);
			ConnectPatchPorts (pb, NULL, "Amplitude", AF_PART_RIGHT, "RAmp", "InputB", 0);
		}
	}

/*-----------------------------------------------------------------------------

Filter Block:

Each channel passes through a filter_3d instrument.

Knobs

  Alpha (2 channels)
    first-order IIR coefficient

  Beta (2 channels)
    third-order FIR coefficient
    
  Feed (2 channels)
    "dry" coefficient

Inputs

  Input (2 channels)

Outputs

  Output (2 channels)
    Left and Right filtered signals

-----------------------------------------------------------------------------*/

	if (s3dFlags & S3D_F_PAN_FILTER)
	{
		if ((result = tmpl_Filter = LoadInsTemplate ("filter_3d.dsp", NULL))	< 0) goto clean;

		AddTemplateToPatch (pb,	"LFilter", tmpl_Filter);
		AddTemplateToPatch (pb,	"RFilter", tmpl_Filter);

		DefinePatchKnob	(pb, "Alpha", 2, AF_SIGNAL_TYPE_GENERIC_SIGNED,	0.0);
		DefinePatchKnob	(pb, "Beta", 2,	AF_SIGNAL_TYPE_GENERIC_SIGNED, 0.0);
		DefinePatchKnob	(pb, "Feed", 2,	AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);

		ConnectPatchPorts (pb, NULL, "Alpha", AF_PART_LEFT, "LFilter", "Alpha",	0);
		ConnectPatchPorts (pb, NULL, "Alpha", AF_PART_RIGHT, "RFilter",	"Alpha", 0);
		ConnectPatchPorts (pb, NULL, "Beta", AF_PART_LEFT, "LFilter", "Beta", 0);
		ConnectPatchPorts (pb, NULL, "Beta", AF_PART_RIGHT, "RFilter", "Beta", 0);
		ConnectPatchPorts (pb, NULL, "Feed", AF_PART_LEFT, "LFilter", "Feed", 0);
		ConnectPatchPorts (pb, NULL, "Feed", AF_PART_RIGHT, "RFilter", "Feed", 0);
	}

/*-----------------------------------------------------------------------------

Block connections:

The final patch may consist of the following combinations:

	Flags:                            Blocks:
	AMPLITUDE|DELAY|FILTER            Delay + Filter
	DELAY|FILTER                      Delay + Filter
	AMPLITUDE|FILTER                  Amplitude + Filter
	DELAY|AMPLITUDE                   Delay
	DELAY                             Delay
	AMPLITUDE                         Amplitude
	FILTER                            Filter
	(none)                            none (Input->Outputs)

-----------------------------------------------------------------------------*/

	DefinePatchPort (pb, "Input", 2, AF_PORT_TYPE_INPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
	DefinePatchPort (pb, "Output", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);

	/* Connect the correct signal path */

	/*
	AMPLITUDE|DELAY|FILTER is the same as just DELAY|FILTER, since we use the
	tap gains
	*/

	if ((s3dFlags & (S3D_F_PAN_DELAY|S3D_F_PAN_FILTER)) == (S3D_F_PAN_DELAY|S3D_F_PAN_FILTER))
	{

DBUG(("Delay and Filter	patch: flags 0x%x\n", s3dFlags));

    		ConnectPatchPorts (pb, NULL, "Input", 0, "Send", "Input", 0);
		ConnectPatchPorts (pb, "LeftTap", "Output", 0, "LFilter", "Input", 0);
		ConnectPatchPorts (pb, "RightTap", "Output", 0,	"RFilter", "Input", 0);
		ConnectPatchPorts (pb, "LFilter", "Output", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RFilter", "Output", 0, NULL, "Output", AF_PART_RIGHT);
	}
	else if ((s3dFlags & (S3D_F_PAN_AMPLITUDE|S3D_F_PAN_FILTER)) == (S3D_F_PAN_AMPLITUDE|S3D_F_PAN_FILTER))
	{

DBUG(("Amplitude and Filter patch\n"));

    		ConnectPatchPorts (pb, NULL, "Input", 0, "LAmp", "InputA", 0);
	    	ConnectPatchPorts (pb, NULL, "Input", 0, "RAmp", "InputA", 0);
    		ConnectPatchPorts (pb, "LAmp", "Output", 0, "LFilter", "Input",	0);
	    	ConnectPatchPorts (pb, "RAmp", "Output", 0, "RFilter", "Input",	0);
		ConnectPatchPorts (pb, "LFilter", "Output", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RFilter", "Output", 0, NULL, "Output", AF_PART_RIGHT);
	}

	/* AMPLITUDE|DELAY is the same as just DELAY, since we use the tap gains */

	else if (s3dFlags & S3D_F_PAN_DELAY)
	{

DBUG(("Delay patch\n"));

		ConnectPatchPorts (pb, NULL, "Input", 0, "Send", "Input", 0);
		ConnectPatchPorts (pb, "LeftTap", "Output", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RightTap", "Output", 0,	NULL, "Output",	AF_PART_RIGHT);
	}
	else if (s3dFlags & S3D_F_PAN_AMPLITUDE)
	{

DBUG(("Amplitude patch\n"));

		ConnectPatchPorts (pb, NULL, "Input", 0, "LAmp", "InputA", 0);
		ConnectPatchPorts (pb, NULL, "Input", 0, "RAmp", "InputA", 0);
		ConnectPatchPorts (pb, "LAmp", "Output", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RAmp", "Output", 0, NULL, "Output", AF_PART_RIGHT);
	}
	else if (s3dFlags & S3D_F_PAN_FILTER)
	{

DBUG(("Filter patch\n"));

		ConnectPatchPorts (pb, NULL, "Input", 0, "LFilter", "Input", 0);
		ConnectPatchPorts (pb, NULL, "Input", 0, "RFilter", "Input", 0);
		ConnectPatchPorts (pb, "LFilter", "Output", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, "RFilter", "Output", 0, NULL, "Output", AF_PART_RIGHT);
	}
	else /* No panning at all! */
	{

DBUG(("No patch\n"));

		ConnectPatchPorts (pb, NULL, "Input", 0, NULL, "Output", AF_PART_LEFT);
		ConnectPatchPorts (pb, NULL, "Input", 0, NULL, "Output", AF_PART_RIGHT);
	}

	/* See if an error occured in one of the constructors */
	if ((result = GetPatchCmdBuilderError (pb))	< 0) goto clean;

	/* Create patch	template */
	if ((result = patch = CreatePatchTemplate (GetPatchCmdList(pb), NULL)) < 0)
	  goto clean;

	if (s3dFlags & S3D_F_PAN_DELAY)
	{
		/* Create delay	line */
		if ((result = dl_Buffer	= CreateDelayLine (200,	1, TRUE)) < 0) goto clean;

		/* Create attachments to constituents */
		if ((result = CreateAttachmentVA (patch, dl_Buffer,
		  AF_TAG_NAME, "SendFIFO",
		  AF_TAG_AUTO_DELETE_SLAVE, TRUE,
		  TAG_END )) < 0)
		  goto clean;

		if ((result = CreateAttachmentVA (patch, dl_Buffer,
		  AF_TAG_NAME, "LeftTapFIFO",
		  TAG_END )) < 0)
		  goto clean;

		if ((result = CreateAttachmentVA (patch, dl_Buffer,
		  AF_TAG_NAME, "RightTapFIFO",
		  TAG_END )) < 0)
		  goto clean;
	}

	result = patch;

clean:
	if (result < 0)
	{
		DeletePatchTemplate( patch );
		DeleteDelayLine( dl_Buffer );
	}

	DeletePatchCmdBuilder (pb);
	UnloadInsTemplate (tmpl_Send);
	UnloadInsTemplate (tmpl_Tap);
	UnloadInsTemplate (tmpl_Filter);
	UnloadInsTemplate (tmpl_Shift);
	UnloadInsTemplate (tmpl_TimesPlus);
	UnloadInsTemplate (tmpl_Minus);
	UnloadInsTemplate (tmpl_Amp);
	UnloadInsTemplate (tmpl_EnvAmp);

	CloseAudioPatchFolio();

	return result;
}

