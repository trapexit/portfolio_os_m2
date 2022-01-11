/******************************************************************************
**
**  @(#) ta_patchcmd.c 96/09/03 1.44
**
**  Simple PatchCmd test.
**
**  By: Bill Barton
**
**  Excerpt from "Pinstripe Penguin" copyright (c) 1988, Bill Barton and Colin Aiken.
**  Used by permission.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950725 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/


#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <audio/patch.h>
#include <kernel/mem.h>
#include <stdio.h>


    /* MIDI pitch stuff */
enum MIDINote {
    NOTE_C,
    NOTE_Cs,
    NOTE_D,
    NOTE_Ds,
    NOTE_E,
    NOTE_F,
    NOTE_Fs,
    NOTE_G,
    NOTE_Gs,
    NOTE_A,
    NOTE_As,
    NOTE_B,
};

    /* enharmonics */
#define NOTE_Db NOTE_Cs
#define NOTE_Eb NOTE_Ds
#define NOTE_Gb NOTE_Fs
#define NOTE_Ab NOTE_Gs
#define NOTE_Bb NOTE_As

    /* generate a pitch based on a note number and octave C0..G10. C5 is middle C. */
#define MakeMIDIPitch(note,octave) ((octave) * 12 + (note))

    /* durations (192 parts per quarter note) */
#define DUR_WHOLE               (DUR_QUARTER*4)
#define DUR_HALF                (DUR_QUARTER*2)
#define DUR_QUARTER             192
#define DUR_EIGHTH              (DUR_QUARTER/2)
#define DUR_TRIPLET_EIGHTH      (DUR_QUARTER/3)
#define DUR_SIXTEENTH           (DUR_QUARTER/4)
#define DUR_TRIPLET_SIXTEENTH   (DUR_QUARTER/6)


#if 0
#define printavail(desc) \
    do { \
        MemInfo meminfo; \
        \
        AvailMem (&meminfo, MEMTYPE_ANY); \
        printf ("%s: sys=%lu:%lu task=%lu:%lu total=%lu\n", desc, meminfo, meminfo.minfo_SysFree + meminfo.minfo_TaskFree); \
    } while(0)
#else
#define printavail(desc)
#endif

void DemoPatch (Item (*makepatchfn)(void), const char *name, int octave);

Item makepatch1 (void);
Item makepatch2 (void);
Item makepatch3 (void);
Item makepatch4 (void);
Item makepatch5 (void);
Item makepatch6 (void);
Item makepatch7 (void);
Item makepatch8 (void);
Item makepatch9 (void);

Err PlayPatch (Item demoInsTemplate, int octave);

static Item melodyClock;
static Item melodyCue;
static Item outInstrument;
static AudioTime startTime;

int main (void)
{
    Err errcode;

    printavail("start");
  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) goto clean;
    if ((errcode = OpenAudioPatchFolio()) < 0) goto clean;

    if ((errcode = melodyClock = CreateAudioClock(NULL)) < 0) goto clean;
    if ((errcode = melodyCue = CreateCue(NULL)) < 0) goto clean;
    if ((errcode = outInstrument = LoadInstrument ("line_out.dsp", 0, 100)) < 0) goto clean;
    if ((errcode = StartInstrument (outInstrument, NULL)) < 0) goto clean;

    DemoPatch (makepatch1, "example1", 7);
    DemoPatch (makepatch3, "example3", 5);
    DemoPatch (makepatch6, "example6", 5);
    DemoPatch (makepatch7, "example7", 5);
/*
    DemoPatch (makepatch2, "example2", 5);
    DemoPatch (makepatch4, "example4", 5);
    DemoPatch (makepatch5, "example5", 5);
    DemoPatch (makepatch8, "example8", 5);
    DemoPatch (makepatch9, "example9", 5);
*/

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);
    if (outInstrument > 0) UnloadInstrument (outInstrument);
    if (melodyCue > 0) DeleteItem (melodyCue);
    if (melodyClock > 0) DeleteItem (melodyClock);
    CloseAudioPatchFolio();
    CloseAudioFolio();

  #ifdef MEMDEBUG
    DumpMemDebugVA (
        DUMPMEMDEBUG_TAG_SUPER, TRUE,
        TAG_END);
    DeleteMemDebug();
  #endif
    printavail("end");

    return 0;
}

void DemoPatch (Item (*makepatchfn)(void), const char *name, int octave)
{
    Item demoInsTemplate;
    Err errcode;

    if ((errcode = demoInsTemplate = makepatchfn()) < 0) goto clean;
    DumpInstrumentResourceInfo (demoInsTemplate, name);
    if ((errcode = PlayPatch (demoInsTemplate, octave)) < 0) goto clean;

clean:
    if (errcode < 0) PrintError (NULL, "demo patch", name, errcode);
    DeletePatchTemplate (demoInsTemplate);
}


/*
    1. Construct a 3-oscillator synth w/ each oscillator playing a different octave
*/
Item makepatch1 (void)
{
    #define MS_FSCALE MakeMixerSpec(1,3,0)
    #define MS_MIXER  MakeMixerSpec(3,1,AF_F_MIXER_WITH_AMPLITUDE)
    static const EnvelopeSegment envpoints[] = {
        { 0.9999, 0.350 },  /* sustain, release */
        { 0.0000, 0.000 },
    };
    Item env = CreateEnvelopeVA (envpoints, sizeof envpoints / sizeof envpoints[0],
        TAG_ITEM_NAME,       "example1.ampenv",
        AF_TAG_SUSTAINBEGIN, 0,
        AF_TAG_SUSTAINEND,   0,
        AF_TAG_SET_FLAGS,    AF_ENVF_FATLADYSINGS,
        TAG_END);
    const Item tmpl_saw_osc   = LoadInsTemplate ("sawtooth.dsp", NULL);
    const Item tmpl_tri_osc   = LoadInsTemplate ("triangle.dsp", NULL);
    const Item tmpl_pulse_osc = LoadInsTemplate ("pulse.dsp", NULL);
    const Item tmpl_fscale    = CreateMixerTemplate (MS_FSCALE, NULL);
    const Item tmpl_envelope  = LoadInsTemplate ("envelope.dsp", NULL);
    const Item tmpl_mixer     = CreateMixerTemplate (MS_MIXER, NULL);
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    SetPatchCoherence (pb, FALSE);      /* just testing this options, it's not actually necessary for this patch */

    DefinePatchKnob (pb, "Frequency", 1, AF_SIGNAL_TYPE_OSC_FREQ, 440);
    DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
    DefinePatchPort (pb, "Output",    1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ExposePatchPort (pb, "AmpEnv",    "env_ampl", "Env");

    AddTemplateToPatch (pb, "freq_scaler",  tmpl_fscale);
    AddTemplateToPatch (pb, "saw_osc",      tmpl_saw_osc);
    AddTemplateToPatch (pb, "tri_osc",      tmpl_tri_osc);
    AddTemplateToPatch (pb, "pulse_osc",    tmpl_pulse_osc);
    AddTemplateToPatch (pb, "env_ampl",     tmpl_envelope);
    AddTemplateToPatch (pb, "mixer",        tmpl_mixer);

    ConnectPatchPorts (pb, NULL, "Frequency", 0, "freq_scaler", "Input", 0);
    SetPatchConstant (pb, "freq_scaler", "Gain", CalcMixerGainPart(MS_FSCALE,0,0), 1.0);
    SetPatchConstant (pb, "freq_scaler", "Gain", CalcMixerGainPart(MS_FSCALE,0,1), 0.5);
    SetPatchConstant (pb, "freq_scaler", "Gain", CalcMixerGainPart(MS_FSCALE,0,2), 0.25);

    ConnectPatchPorts (pb, "freq_scaler", "Output", 0, "saw_osc", "Frequency", 0);
    ConnectPatchPorts (pb, "freq_scaler", "Output", 1, "tri_osc", "Frequency", 0);
    ConnectPatchPorts (pb, "freq_scaler", "Output", 2, "pulse_osc", "Frequency", 0);

    ConnectPatchPorts (pb, "saw_osc",   "Output", 0, "mixer", "Input", 0);
    ConnectPatchPorts (pb, "pulse_osc", "Output", 0, "mixer", "Input", 1);
    ConnectPatchPorts (pb, "tri_osc",   "Output", 0, "mixer", "Input", 2);

    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,0,0), 0.5);
    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,1,0), 0.25);
    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,2,0), 0.75);
    ConnectPatchPorts (pb, NULL, "Amplitude", 0, "env_ampl", "Amplitude", 0);
    ConnectPatchPorts (pb, "env_ampl", "Output", 0, "mixer", "Amplitude", 0);
    ConnectPatchPorts (pb, "mixer", "Output", 0, NULL, "Output", 0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example1",
        TAG_END)) < 0) goto clean;

    if ((result = CreateAttachmentVA (patch, env,
        AF_TAG_NAME,              "AmpEnv",
        AF_TAG_AUTO_DELETE_SLAVE, TRUE,
        TAG_END)) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
        DeleteEnvelope (env);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_saw_osc);
    UnloadInsTemplate (tmpl_tri_osc);
    UnloadInsTemplate (tmpl_pulse_osc);
    DeleteMixerTemplate (tmpl_fscale);
    UnloadInsTemplate (tmpl_envelope);
    DeleteMixerTemplate (tmpl_mixer);
    return result;
    #undef MS_FSCALE
    #undef MS_MIXER
}


#if 0
/*
    2. sample player + sawtooth. single frequency control, export both
       amplitude controls, separate outputs.
*/
Item makepatch2 (void)
{
    const Item tmpl_sampler  = LoadInsTemplate ("sampler_16_f1.dsp", NULL);
    const Item tmpl_sawtooth = LoadInsTemplate ("sawtooth.dsp", NULL);
    const Item samp_sinewave = LoadSystemSample ("sinewave.aiff");
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "osc_sampler", tmpl_sampler);
    AddTemplateToPatch (pb, "osc_sawtooth", tmpl_sawtooth);

    DefinePatchKnob (pb, "Frequency", 1, AF_SIGNAL_TYPE_OSC_FREQ, 440.0);
    DefinePatchKnob (pb, "Amplitude", 2, AF_SIGNAL_TYPE_GENERIC_SIGNED, 0.25);
    DefinePatchPort (pb, "Output", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ExposePatchPort (pb, "InFIFO", "osc_sampler", "InFIFO");

    ConnectPatchPorts (pb, NULL, "Frequency", 0, "osc_sawtooth", "Frequency", 0);
    ConnectPatchPorts (pb, NULL, "Amplitude", 0, "osc_sawtooth", "Amplitude", 0);

#if 0
    AddTemplateToPatch (pb, "sampler_freqmath", tmpl_multiply);
    ConnectPatchPorts (pb, NULL, "Frequency", 0, "sampler_freqmath", "InputA", 0);
    SetPatchConstant (pb, "sampler_freqmath", "InputB", 0, 0.5);
    ConnectPatchPorts (pb, "sampler_freqmath", "Output", 0, "osc_sampler", "SampleRate", 0);
#endif

    ConnectPatchPorts (pb, NULL, "Amplitude", 1, "osc_sampler", "Amplitude", 0);

    ConnectPatchPorts (pb, "osc_sawtooth", "Output", 0, NULL, "Output", 0);
    ConnectPatchPorts (pb, "osc_sampler", "Output", 0, NULL, "Output", 1);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example2",
        TAG_END)) < 0) goto clean;

    if ((result = CreateAttachmentVA (patch, samp_sinewave,
        AF_TAG_NAME,                "InFIFO",
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END )) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
        UnloadSample (samp_sinewave);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);
    UnloadInsTemplate (tmpl_sawtooth);

    return result;
}
#endif


/*
    3. sawtooth + envelope.
*/
Item makepatch3 (void)
{
    static const EnvelopeSegment envpoints[] = {
        { 0.0000, 0.010 },
        { 0.9999, 0.020 },  /* attack */
        { 0.5000, 0.220 },  /* decay, sustain */
        { 0.0000, 0.000 },  /* release */
    };
    Item env = CreateEnvelopeVA (envpoints, sizeof envpoints / sizeof envpoints[0],
        TAG_ITEM_NAME,       "example2.ampenv",
        AF_TAG_SUSTAINBEGIN, 2,
        AF_TAG_SUSTAINEND,   2,
        AF_TAG_SET_FLAGS,    AF_ENVF_FATLADYSINGS,
        TAG_END);
    Item tmpl_sawtooth = LoadInsTemplate ("sawtooth.dsp", NULL);
    Item tmpl_envelope = LoadInsTemplate ("envelope.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    AddTemplateToPatch (pb, "env_ampl", tmpl_envelope);
    AddTemplateToPatch (pb, "osc_sawtooth", tmpl_sawtooth);

    ExposePatchPort (pb, "AmpEnv", "env_ampl", "Env");
    DefinePatchKnob (pb, "Frequency", 1, AF_SIGNAL_TYPE_OSC_FREQ, 440.0);
    DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);

    ConnectPatchPorts (pb,   NULL,           "Amplitude", 0,   "env_ampl",     "Amplitude", 0);
    ConnectPatchPorts (pb,   "env_ampl",     "Output",    0,   "osc_sawtooth", "Amplitude", 0);
    ConnectPatchPorts (pb,   NULL,           "Frequency", 0,   "osc_sawtooth", "Frequency", 0);
    ConnectPatchPorts (pb,   "osc_sawtooth", "Output",    0,   NULL,           "Output",    0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example2",
        TAG_END)) < 0) goto clean;

    if ((result = CreateAttachmentVA (patch, env,
        AF_TAG_NAME,              "AmpEnv",
        AF_TAG_AUTO_DELETE_SLAVE, TRUE,
        TAG_END)) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
        DeleteEnvelope (env);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sawtooth);
    UnloadInsTemplate (tmpl_envelope);

    return result;
}


#if 0

/*
    stress test trivial patches
*/
Item makepatch4 (void)
{
    const Item tmpl_add   = LoadInsTemplate ("add.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    DefinePatchPort (pb, "Input", 1, AF_PORT_TYPE_INPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ConnectPatchPorts (pb, NULL, "Input", 0, NULL, "Output", 0);
#if 0
    DefinePatchKnob (pb, "Knob", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 0.0);
    ConnectPatchPorts (pb, NULL, "Knob", 0, NULL, "Output", 0);
    AddTemplateToPatch (pb, "add", tmpl_add);
#endif

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example4",
        TAG_END);

clean:
    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_add);

    return result;
}

#endif

#if 0
/*
    5. Test bed for doing input reduction
*/
Item makepatch5 (void)
{
    const Item tmpl_mixer = CreateMixerTemplate (MakeMixerSpec(3,1,0,NULL));
    PatchCmdBuilder *pb = NULL;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    DefinePatchPort (pb, "Input",  3, AF_PORT_TYPE_INPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    DefinePatchKnob (pb, "Gain",   3, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);

    AddTemplateToPatch (pb, "mixer", tmpl_mixer);

    ConnectPatchPorts (pb, NULL, "Input", 0, "mixer", "Input", 0);
/*
    ConnectPatchPorts (pb, NULL, "Input", 1, "mixer", "Input", 1);
    ConnectPatchPorts (pb, NULL, "Input", 2, "mixer", "Input", 2);
*/
    SetPatchConstant (pb, "mixer", "Gain", 2, 0.9);
    SetPatchConstant (pb, "mixer", "Gain", 1, 1.0);

    ConnectPatchPorts (pb, "mixer", "Output", 0, NULL, "Output", 0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

    result = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example5",
        TAG_END);

clean:
    DeletePatchCmdBuilder (pb);
    DeleteMixerTemplate (tmpl_mixer);
    return result;
}
#endif


#if 1
/*
    2 sample players
*/
Item makepatch6 (void)
{
    #define MS_MIXER  MakeMixerSpec(3,1,AF_F_MIXER_WITH_AMPLITUDE)
    const Item tmpl_sampler  = LoadInsTemplate ("sampler_16_v1.dsp", NULL);
    const Item tmpl_mult     = LoadInsTemplate ("multiply_unsigned.dsp", NULL);
    const Item tmpl_mixer    = CreateMixerTemplate (MS_MIXER, NULL);
    const Item samp_sinewave = LoadSystemSample ("sinewave.aiff");
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

        /* templates */
    AddTemplateToPatch (pb, "freq_mult1",  tmpl_mult);
    AddTemplateToPatch (pb, "freq_mult2",  tmpl_mult);
    AddTemplateToPatch (pb, "sampler0",    tmpl_sampler);
    AddTemplateToPatch (pb, "sampler1",    tmpl_sampler);
    AddTemplateToPatch (pb, "sampler2",    tmpl_sampler);
    AddTemplateToPatch (pb, "mixer",       tmpl_mixer);

        /* ports */
    DefinePatchKnob (pb, "SampleRate", 1, AF_SIGNAL_TYPE_SAMPLE_RATE, 44100.0);
    DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ExposePatchPort (pb, "InFIFO0", "sampler0", "InFIFO");
    ExposePatchPort (pb, "InFIFO1", "sampler1", "InFIFO");
    ExposePatchPort (pb, "InFIFO2", "sampler2", "InFIFO");

        /* sample rate signal path */
#if 0
    ConnectPatchPorts (pb, NULL, "SampleRate", 0, "freq_scaler", "Input", 0);
    SetPatchConstant (pb, "freq_scaler", "Gain", CalcMixerGainPart(MS_FSCALE,0,0), 1.0);
    SetPatchConstant (pb, "freq_scaler", "Gain", CalcMixerGainPart(MS_FSCALE,0,1), 0.8);
    ConnectPatchPorts (pb, "freq_scaler", "Output", 0, "sampler0", "SampleRate", 0);
    ConnectPatchPorts (pb, "freq_scaler", "Output", 1, "sampler1", "SampleRate", 0);
#endif
    ConnectPatchPorts (pb, NULL, "SampleRate", 0, "sampler0", "SampleRate", 0);

    ConnectPatchPorts (pb, NULL, "SampleRate", 0, "freq_mult1", "InputA", 0);
    SetPatchConstant (pb, "freq_mult1", "InputB", 0, 0.5);
    ConnectPatchPorts (pb, "freq_mult1", "Output", 0, "sampler1", "SampleRate", 0);

    ConnectPatchPorts (pb, NULL, "SampleRate", 0, "freq_mult2", "InputA", 0);
    SetPatchConstant (pb, "freq_mult2", "InputB", 0, 0.25);
    ConnectPatchPorts (pb, "freq_mult2", "Output", 0, "sampler2", "SampleRate", 0);

        /* output signal path */
    ConnectPatchPorts (pb, "sampler0", "Output", 0, "mixer", "Input", 0);
    ConnectPatchPorts (pb, "sampler1", "Output", 0, "mixer", "Input", 1);
    ConnectPatchPorts (pb, "sampler2", "Output", 0, "mixer", "Input", 2);

    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,0,0), 0.3);
    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,1,0), 0.3);
    SetPatchConstant (pb, "mixer", "Gain", CalcMixerGainPart(MS_MIXER,2,0), 0.3);
    ConnectPatchPorts (pb, NULL, "Amplitude", 0, "mixer", "Amplitude", 0);
    ConnectPatchPorts (pb, "mixer", "Output", 0, NULL, "Output", 0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

        /* make patch */
    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example6",
        TAG_END)) < 0) goto clean;

        /* attach things */
    if ((result = CreateAttachmentVA (patch, samp_sinewave,
        AF_TAG_NAME,                "InFIFO0",
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END )) < 0) goto clean;
    if ((result = CreateAttachmentVA (patch, samp_sinewave,
        AF_TAG_NAME,                "InFIFO1",
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END )) < 0) goto clean;
    if ((result = CreateAttachmentVA (patch, samp_sinewave,
        AF_TAG_NAME,                "InFIFO2",
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END )) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
        UnloadSample (samp_sinewave);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);
    UnloadInsTemplate (tmpl_mult);
    DeleteMixerTemplate (tmpl_mixer);

    return result;
    #undef MS_MIXER
}
#endif

#if 1

/*
    1 sample player
*/
Item makepatch7 (void)
{
    const Item tmpl_sampler  = LoadInsTemplate ("sampler_16_v1.dsp", NULL);
    const Item samp_sinewave = LoadSystemSample ("sinewave.aiff");
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

        /* templates */
    AddTemplateToPatch (pb, "sampler",     tmpl_sampler);

        /* ports */
    DefinePatchKnob (pb, "SampleRate", 1, AF_SIGNAL_TYPE_SAMPLE_RATE, 44100.0);
    DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);
    ExposePatchPort (pb, "InFIFO", "sampler", "InFIFO");

        /* connect player to ports */
    ConnectPatchPorts (pb, NULL, "SampleRate", 0, "sampler", "SampleRate", 0);
    ConnectPatchPorts (pb, NULL, "Amplitude", 0, "sampler", "Amplitude", 0);
    ConnectPatchPorts (pb, "sampler", "Output", 0, NULL, "Output", 0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

        /* make patch */
    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example6",
        TAG_END)) < 0) goto clean;

        /* attach things */
    if ((result = CreateAttachmentVA (patch, samp_sinewave,
     /* AF_TAG_NAME,                "InFIFO", */
        AF_TAG_AUTO_DELETE_SLAVE,   TRUE,
        TAG_END )) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
        UnloadSample (samp_sinewave);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_sampler);

    return result;
}

#endif

#if 0
/*
    testbed for bound resources
*/
Item makepatch8 (void)
{
    const Item tmpl_test  = LoadInsTemplate ("test_bind.dsp", NULL);
    PatchCmdBuilder *pb = NULL;
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

        /* templates */
    AddTemplateToPatch (pb, "sampler", tmpl_test);

        /* ports */
    DefinePatchKnob (pb, "SampleRate", 1, AF_SIGNAL_TYPE_SAMPLE_RATE, 44100.0);
    DefinePatchKnob (pb, "Amplitude", 1, AF_SIGNAL_TYPE_GENERIC_SIGNED, 1.0);
    DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_GENERIC_SIGNED);

        /* connect player to ports */
/*  ConnectPatchPorts (pb, NULL, "SampleRate", 0, "sampler", "BindKnob", 1);    */
/*  SetPatchConstant (pb, "sampler", "BindKnob", 0, 0.5);                       */
/*  ConnectPatchPorts (pb, NULL, "Amplitude", 0, "sampler", "RealKnob", 0);     */
/*  ConnectPatchPorts (pb, NULL, "Amplitude", 0, "sampler", "RealKnob", 1);     */
/*  SetPatchConstant (pb, "sampler", "RealInput", 0, 0.5);                      */
/*  ConnectPatchPorts (pb, NULL, "Amplitude", 0, "sampler", "RealInput", 1);    */
/*  SetPatchConstant (pb, "sampler", "BindInput", 0, 0.5);                      */
/*  SetPatchConstant (pb, "sampler", "BindInput", 1, 0.75);                     */
/*  ConnectPatchPorts (pb, NULL, "Amplitude", 0, "sampler", "BindInput", 1);    */
    ConnectPatchPorts (pb, "sampler", "Output", 0, NULL, "Output", 0);

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

        /* make patch */
    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example8",
        TAG_END)) < 0) goto clean;

        /* success */
    result = patch;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
    }

    DeletePatchCmdBuilder (pb);
    UnloadInsTemplate (tmpl_test);

    return result;
}
#endif

#if 0
/*
    testbed for stress testing patch compiler
*/
Item makepatch9 (void)
{
    PatchCmdBuilder *pb = NULL;
    Item tmpl_mixer = CreateMixerTemplate (MakeMixerSpec(4,2,AF_F_MIXER_WITH_AMPLITUDE), NULL);
    Item patch = -1;
    Item result;

    if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;

    DefinePatchPort (pb, "Input", 2, AF_PORT_TYPE_INPUT, AF_SIGNAL_TYPE_GENERIC_UNSIGNED);
    DefinePatchPort (pb, "Output", 2, AF_PORT_TYPE_OUTPUT, AF_SIGNAL_TYPE_WHOLE_NUMBER);
    DefinePatchKnob (pb, "Knob", 2, AF_SIGNAL_TYPE_GENERIC_SIGNED, 0);
    AddTemplateToPatch (pb, "mix1", tmpl_mixer);
    AddTemplateToPatch (pb, "mix2", tmpl_mixer);

/*
    SetPatchConstant (pb, "mix2", "Input", 3, 0);
    ConnectPatchPorts (pb, NULL, "Input", 1, "mix2", "Input", 2);
    ConnectPatchPorts (pb, NULL, "Input", 1, "mix2", "Input", 2);
    SetPatchConstant (pb, "mix2", "Input", 2, 0);
    ConnectPatchPorts (pb, "mix1", "Output", 0, "mix2", "Input", 1);
    ConnectPatchPorts (pb, "mix1", "Output", 0, NULL, "Output", 0);
    ConnectPatchPorts (pb, "mix2", "Output", 0, NULL, "Output", 1);
    ConnectPatchPorts (pb, NULL, "Input", 1, "mix2", "Input", 3);
    ConnectPatchPorts (pb, "mix1", "Output", 1, "mix2", "Input", 3);
    SetPatchConstant (pb, "mix2", "Input", 3, 0);
*/

    if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;

        /* make patch */
    if ((result = patch = CreatePatchTemplateVA (GetPatchCmdList (pb),
        TAG_ITEM_NAME,  "example9",
        TAG_END)) < 0) goto clean;

clean:
    if (result < 0) {
        DeletePatchTemplate (patch);
    }

    DeletePatchCmdBuilder (pb);

    return result;
}
#endif


/* -------------------- DemoPatch() */

Err PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration);
static Err Rest (int32 duration);

Err PlayPatch (Item demoInsTemplate, int octave)
{
    Item demoInstrument;
    Err errcode;

    if ((errcode = demoInstrument = CreateInstrument (demoInsTemplate, NULL)) < 0) goto clean;

    {
        InstrumentPortInfo info;

        if ((errcode = GetInstrumentPortInfoByName (&info, sizeof info, demoInstrument, "Output")) >= 0) {
            if (info.pinfo_Type == AF_PORT_TYPE_OUTPUT) {
                if ((errcode = ConnectInstrumentParts (demoInstrument, "Output", 0,
                    outInstrument, "Input", AF_PART_LEFT )) < 0) goto clean;
                if ((errcode = ConnectInstrumentParts (demoInstrument, "Output", 1 % info.pinfo_NumParts,
                    outInstrument, "Input", AF_PART_RIGHT)) < 0) goto clean;
            }
        }
        else if (errcode != AF_ERR_NAME_NOT_FOUND) goto clean;
    }

        /* prime timer */
    if (!startTime) {
        SetAudioClockRate (melodyClock, 141.0 / 60.0 * DUR_QUARTER);
        ReadAudioClock (melodyClock, &startTime);
    }

        /* play the melody */
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_D, octave),   112, DUR_QUARTER);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_A, octave-1),  64, DUR_TRIPLET_EIGHTH);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_D, octave),    64, DUR_TRIPLET_EIGHTH);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_A, octave-1),  64, DUR_TRIPLET_EIGHTH);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_Fs,octave),   112, DUR_TRIPLET_EIGHTH * 2);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_D, octave),    64, DUR_TRIPLET_EIGHTH + DUR_TRIPLET_EIGHTH * 2);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_Fs,octave),    64, DUR_TRIPLET_EIGHTH);

    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_E, octave),   112, DUR_TRIPLET_EIGHTH * 2);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_C, octave),    64, DUR_TRIPLET_EIGHTH + DUR_TRIPLET_EIGHTH * 2);
    PlayPitchNote (demoInstrument, MakeMIDIPitch (NOTE_G, octave-1), 112, DUR_TRIPLET_EIGHTH + DUR_TRIPLET_EIGHTH * 2);

    Rest (DUR_TRIPLET_EIGHTH);                  /* set delay time for note to release */
    Rest (DUR_QUARTER*3);                       /* set up next start time */

    errcode = 0;

clean:
    DeleteInstrument (demoInstrument);
    return errcode;
}

/***************************************************************/
Err PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration)
{
    SleepUntilAudioTime (melodyClock, melodyCue, startTime);
    StartInstrumentVA (Instrument,
                       AF_TAG_VELOCITY, Velocity,
                       AF_TAG_PITCH,    Note,
                       TAG_END);

    SleepUntilAudioTime (melodyClock, melodyCue, startTime + Duration * 1 / 2);
    ReleaseInstrument (Instrument, NULL);

    startTime += Duration;

    return 0;
}

static Err Rest (int32 duration)
{
    SleepUntilAudioTime (melodyClock, melodyCue, startTime);
    startTime += duration;

    return 0;
}
