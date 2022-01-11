
/******************************************************************************
**
**  @(#) simple_envelope.c 96/03/19 1.16
**  $Id: simple_envelope.c,v 1.6 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name simple_envelope
|||	Simple audio envelope example.
|||
|||	  Format
|||
|||	    simple_envelope <Percent Amplitude>
|||
|||	  Description
|||
|||	    Simple demonstration of an envelope used to ramp amplitude of a triangle
|||	    waveform. A 3-segment envelope is used to provide a ramp up to a specified
|||	    amplitude at the start of the sound and a ramp down when the sound is to
|||	    be stopped. Using envelopes is one technique to avoid audio pops at the
|||	    start and end of sounds.
|||
|||	  Arguments
|||
|||	    <Percent Amplitude>
|||	        Percentage of full amplitude to play at. Defaults to 100.
|||
|||	  Associated Files
|||
|||	    simple_envelope.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
**/

#include <audio/audio.h>
#include <kernel/mem.h>
#include <kernel/task.h>        /* WaitSignal() */
#include <misc/frac16.h>        /* ConvertF16_32() */
#include <stdio.h>
#include <stdlib.h>


/* -------------------- Parameters */

#define DEFAULT_AMPLITUDE   (0.999)


/* -------------------- Macros */

/*
    Macro to simplify error checking.
    expression is only evaluated once and should return an int32
    (or something that can be cast to it sensibly)
*/
#define TRAP_FAILURE(_exp,_name) \
    do { \
        const int32 _result = (_exp); \
        if (_result < 0) { \
            PrintError (NULL, _name, NULL, _result); \
            goto clean; \
        } \
    } while (0)


/* -------------------- Code */

void TestEnvelope (float32 Amplitude);

int main (int argc, char *argv[])
{
    printf ("%s: begin\n", argv[0]);

  #ifdef MEMDEBUG
    TRAP_FAILURE (CreateMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                   MEMDEBUGF_FREE_PATTERNS |
                                   MEMDEBUGF_PAD_COOKIES |
                                   MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                   MEMDEBUGF_KEEP_TASK_DATA,
                                   NULL ), "CreateMemDebug");
  #endif

    TRAP_FAILURE ( OpenAudioFolio(), "OpenAudioFolio()" );

    TestEnvelope (argc > 1 ? ((float32)atoi(argv[1])/100.0) : DEFAULT_AMPLITUDE);

clean:
    CloseAudioFolio();
    printf ("%s: end\n", argv[0]);

  #ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
  #endif

    return 0;
}

void TestEnvelope (float32 Amplitude)
{
        /* simple ramped gate envelope */
    EnvelopeSegment envpoints[] = {
/* Segment = Value, Duration */
            { 0.00,     0.54 },
            { 0.00,     6.21 },   /* fill in this data value from the amplitude argument */
            { 0.00,     0.00 },
    };

        /* misc items and such */
    Item osc_ins=0;
    Item env_ins=0;
    Item env_data=0;
    Item env_att=0;
    Item env_cue=0;
    Item out_ins=0;
    Item sleep_cue=0;
    int32 env_cuesignal;
    float32 TicksPerSecond;

        /* Get clock rate so we can wait some number of seconds. */
    TRAP_FAILURE ( GetAudioClockRate( AF_GLOBAL_CLOCK, &TicksPerSecond), "GetAudioClockRate()" );

        /* set amplitude */
    envpoints[1].envs_Value = Amplitude;

        /* Get instruments */
    TRAP_FAILURE ( out_ins = LoadInstrument ("line_out.dsp", 0, 100), "LoadInstrument() 'line_out.dsp'" );
    TRAP_FAILURE ( osc_ins = LoadInstrument ("triangle.dsp", 0, 100), "LoadInstrument() 'triangle.dsp'" );
    TRAP_FAILURE ( env_ins = LoadInstrument ("envelope.dsp", 0, 100), "LoadInstrument() 'envelope.dsp'" );

        /* Connect instruments */
    TRAP_FAILURE ( ConnectInstruments (env_ins, "Output", osc_ins, "Amplitude"), "ConnectInstruments()" );
    TRAP_FAILURE ( ConnectInstrumentParts (osc_ins, "Output", 0, out_ins, "Input", 0), "ConnectInstrumentParts()" );
    TRAP_FAILURE ( ConnectInstrumentParts (osc_ins, "Output", 0, out_ins, "Input", 1), "ConnectInstrumentParts()" );

        /* create and attach envelope */
    TRAP_FAILURE ( env_data =
        CreateItemVA ( MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE),
                       AF_TAG_ADDRESS,        envpoints,
                       AF_TAG_FRAMES,         sizeof envpoints / sizeof envpoints[0],
                       AF_TAG_SUSTAINBEGIN,   1,
                       AF_TAG_SUSTAINEND,     1,
                       AF_TAG_SET_FLAGS,      AF_ENVF_FATLADYSINGS,
                       TAG_END ), "create envelope" );
    TRAP_FAILURE ( env_att = CreateAttachment (env_ins, env_data, NULL), "CreateAttachment()" );

        /* get cue for envelope */
    TRAP_FAILURE ( env_cue = CreateCue (NULL), "CreateCue()" );
    env_cuesignal = GetCueSignal (env_cue);
    TRAP_FAILURE ( MonitorAttachment (env_att, env_cue, CUE_AT_END), "MonitorAttachment()" );

        /* create sleep cue */
    TRAP_FAILURE ( sleep_cue = CreateCue (NULL), "CreateCue()" );

        /* Start playing */
    printf ("starting...");
    TRAP_FAILURE ( StartInstrument ( out_ins, NULL ), "start output" );
    TRAP_FAILURE ( StartInstrument ( osc_ins, NULL ), "start oscillator" );
    TRAP_FAILURE ( StartInstrument ( env_ins, NULL ), "start envelope" );

        /* wait a couple of seconds */
    TRAP_FAILURE ( SleepUntilTime (sleep_cue, GetAudioTime() + (2.0 * TicksPerSecond)), "sleep" );

        /* release envelope */
    printf ("releasing...");
    TRAP_FAILURE ( ReleaseInstrument (env_ins, NULL), "release envelope" );

        /* wait until cued */
    WaitSignal (env_cuesignal);

        /*
            Stop instruments before deleting them to avoid pops.

            In this case, deleting the instruments in the order below without
            stopping them first would cause 2 loud pops. The first pop would
            occur when env_ins is deleted, which would break the connection
            between env_ins and osc_ins's Amplitude knob, which would then be
            would returned to its default setting of 1.0 (full amplitude). The
            second pop would occur when the full-amplitude osc_ins is deleted,
            which would cause out_ins's Inputs to be set to 0.
        */
    StopInstrument (env_ins, NULL);
    StopInstrument (osc_ins, NULL);
    StopInstrument (out_ins, NULL);
    printf ("done.\n");

clean:
    DeleteCue (sleep_cue);
    DeleteCue (env_cue);
    DeleteAttachment (env_att);
    DeleteItem (env_data);
    UnloadInstrument (env_ins);
    UnloadInstrument (osc_ins);
    UnloadInstrument (out_ins);
}
