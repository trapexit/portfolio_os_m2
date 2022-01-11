
/******************************************************************************
**
**  @(#) tsp_algorithmic.c 96/08/22 1.15
**  $Id: tsp_algorithmic.c,v 1.33 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name tsp_algorithmic
|||	Sound Player example showing algorithmic sequencing of sound playback.
|||
|||	  Format
|||
|||	    tsp_algorithmic
|||
|||	  Description
|||
|||	    Assume you have two short sounds in memory and one long sound file with the
|||	    following markers:
|||
|||	  -preformatted
|||
|||	        Marker          Description
|||	        ------          -----------
|||	        BEGIN           beginning segment ("attack")
|||	        Loop            loop segment ("sustain")
|||	        First Ending    1st ending segment ("gap")
|||	        Second Ending   2nd ending segment ("release, end")
|||	        END
|||
|||	  -normal_format
|||
|||	    algorithmically play this sequence:
|||
|||	  -preformatted
|||
|||	        Description                     What you should hear
|||	        -----------                     --------------------
|||	        long sound beginning segment    "attack"
|||	        long sound loop segment         "sustain"
|||	        long sound 1st ending segment   "gap"
|||	        long sound loop segment         "sustain"
|||	        long sound 2nd ending segment   "release, end"
|||	        short sound 1                   <honk>
|||	        long sound loop segment         "sustain"
|||	        long sound 1st ending segment   "gap"
|||	        long sound loop segment         "sustain"
|||	        long sound 2nd ending segment   "release, end"
|||	        short sound 2                   <blap>
|||
|||	  -normal_format
|||
|||	    The technique used to implement this sequence involves the use of static
|||	    branches, where one segment always leads into another, (e.g. after playing
|||	    the 1st ending always go back to the loop segment), and decision functions
|||	    where a conditional branching is required (e.g. from the end of the loop
|||	    segment either go to the 1st or 2nd ending).
|||
|||	    This also demonstrates using sounds spooled from disc and played directly
|||	    from memory.
|||
|||	  Associated Files
|||
|||	    tsp_algorithmic.c, words.aiff
|||
|||	  Location
|||
|||	    Examples/Audio/Spooling
|||
|||	  See Also
|||
|||	    spCreatePlayer(), tsp_switcher(@), tsp_rooms(@)
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <audio/soundplayer.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <stdio.h>


/* -------------------- Parameters */

    /* a long sound to be spooled off disc */
#define LONG_SOUND      "words.aiff"    /* !!! non-standard, we need to do something about this! */

    /* short sounds to be loaded in and then played from memory */
#define SHORT_SOUND_1   "/remote/Samples/PitchedL/Bassoon/Bassoon.A3LM44k.aiff"
#define SHORT_SOUND_2   "/remote/Samples/PitchedLR/TrumpetRL/Trumpet.A3LRM44k.aiff"

    /* audio buffers */
#define NUMBUFS 4
#define BUFSIZE 22000


/* -------------------- Macros */

/*
    Macro to simplify error checking.
    expression is only evaluated once and should return an int32
    (or something that can be cast to it sensibly)
*/
#define TRAP_ERROR(_exp,_desc) \
    do { \
        const int32 _result = (_exp); \
        if (_result < 0) { \
            PrintError (NULL, _desc, NULL, _result); \
            goto clean; \
        } \
    } while (0)


/* -------------------- Code */

void testplayer (void);
Err loop_decision (SPAction *resultAction, int32 *hitCount, SPSound *sound, const char *markerName);
Err secondend_decision (SPAction *resultAction, int32 *hitCount, SPSound *sound, const char *markerName);

SPSound *longSound, *shortSound1, *shortSound2;

int main (int argc, char *argv[])
{
    TOUCH(argc);

  #ifdef MEMDEBUG
    TRAP_ERROR (
        CreateMemDebug (NULL),
        "CreateMemDebug"
    );
    TRAP_ERROR (
        ControlMemDebug (MEMDEBUGF_ALLOC_PATTERNS |
                         MEMDEBUGF_FREE_PATTERNS |
                         MEMDEBUGF_PAD_COOKIES |
                         MEMDEBUGF_CHECK_ALLOC_FAILURES |
                         MEMDEBUGF_KEEP_TASK_DATA),
        "ControlMemDebug"
    );
  #endif

    printf ("%s: start.\n", argv[0]);

    TRAP_ERROR (
        OpenAudioFolio(),
        "open audio folio"
    );

    testplayer();

clean:
    CloseAudioFolio();

    printf ("%s: done.\n", argv[0]);

  #ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
  #endif

    return 0;
}

void testplayer (void)
{
    Item samplerins;
    Item outputins = 0;
    Item sample1=0, sample2=0;
    SPPlayer *player = NULL;
    int32 loop_count = 0, secondend_count = 0;


    /* set up instruments and connections */

    TRAP_ERROR (
        samplerins = LoadInstrument ("sampler_16_f1.dsp", 0, 100),
        "Load sample player instrument"
    );
    TRAP_ERROR (
        outputins = LoadInstrument ("line_out.dsp", 0, 100),
        "Load line_out.dsp"
    );
    TRAP_ERROR (
        ConnectInstrumentParts (samplerins, "Output", 0, outputins, "Input", 0 ),
        "Connect sampler to output"
    );
    TRAP_ERROR (
        ConnectInstrumentParts (samplerins, "Output", 0, outputins, "Input", 1),
        "Connect sampler to output"
    );
    TRAP_ERROR (
        StartInstrument (outputins, NULL),
        "Start output"
    );


    /* create player */

    TRAP_ERROR (
        spCreatePlayer (&player, samplerins, NUMBUFS, BUFSIZE, NULL),
        "create SPPlayer"
    );


    /* add long sound to player as a sound file to be read from disc */

    TRAP_ERROR (
        spAddSoundFile (&longSound, player, LONG_SOUND),
        "add sound "LONG_SOUND
    );


    /* preload short sounds and add them to player as in-memory samples */

    TRAP_ERROR (
        sample1 = LoadSample (SHORT_SOUND_1),
        "load "SHORT_SOUND_1
    );
    TRAP_ERROR (
        spAddSample (&shortSound1, player, sample1),
        "add sound "SHORT_SOUND_1
    );

    TRAP_ERROR (
        sample2 = LoadSample (SHORT_SOUND_2),
        "load "SHORT_SOUND_2
    );
    TRAP_ERROR (
        spAddSample (&shortSound2, player, sample2),
        "add sound "SHORT_SOUND_2
    );


    /*
        Set branch at end of first ending (beginning of 2nd ending) to branch
        back to beginning of Loop
    */

    TRAP_ERROR (
        spBranchAtMarker (longSound, "Second Ending", longSound, "Loop"),
        "branch long.2nd -> long.loop"
    );


    /*
        Install decision function for end of Loop segment (beginning of
        first ending) to decide whether to go to first or second ending.
    */

    TRAP_ERROR (
        spSetMarkerDecisionFunction (longSound, "First Ending",
                                     (SPDecisionFunction)loop_decision,
                                     &loop_count),
        "marker decision long.1st"
    );


    /*
        Install decision function for end of the long sound to decide
        whether to go to the short sound #1 or #2.
    */

    TRAP_ERROR (
        spSetMarkerDecisionFunction (longSound, SP_MARKER_NAME_END,
                                     (SPDecisionFunction)secondend_decision,
                                     &secondend_count),
        "marker decision long.end"
    );


    /*
        Set branch at end of short sound #1 to branch back to beginning
        of loop in long sound.
    */

    TRAP_ERROR (
        spBranchAtMarker (shortSound1, SP_MARKER_NAME_END, longSound, "Loop"),
        "branch short1.end -> long.loop"
    );


    /* print out SPPlayer contents (don't do this in production code) */

    spDumpPlayer (player);


    /* start playing */

    printf ("tsp_algorithmic: reading...");
    TRAP_ERROR (
        spStartReading (longSound, SP_MARKER_NAME_BEGIN),
        "Start reading"
    );
    printf ("playing\n");
    TRAP_ERROR (
        spStartPlayingVA (player,
                          AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(1.0),
                          TAG_END),
        "Start reading"
    );


    /* service player until it's done */

    {
        const int32 playersigs = spGetPlayerSignalMask (player);

        while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE) {
            const int32 sigs = WaitSignal (playersigs);
            /* could add signals to wait for in above WaitSignal() */

            TRAP_ERROR (
                spService (player, sigs),
                "Service"
            );

            /* could process signals for other things here */
        }
    }


    /* all done, stop player and clean up */

    printf ("tsp_algorithmic: stopping\n");
    TRAP_ERROR (
        spStop (player),
        "Stop"
    );

clean:
    spDeletePlayer (player);
    UnloadSample (sample1);
    UnloadSample (sample2);
    UnloadInstrument (samplerins);
    UnloadInstrument (outputins);
}


Err loop_decision (SPAction *resultAction, int32 *hitCount, SPSound *sound, const char *markerName)
{
    TOUCH(sound);
    TOUCH(markerName);

    printf ("tsp_algorithmic: loop_decision() count=%ld\n", *hitCount);

    if (++(*hitCount) >= 2) {
        *hitCount = 0;
        return spSetBranchAction (resultAction, longSound, "Second Ending");
    }

    return 0;
}

Err secondend_decision (SPAction *resultAction, int32 *hitCount, SPSound *sound, const char *markerName)
{
    TOUCH(sound);
    TOUCH(markerName);

    printf ("tsp_algorithmic: secondend_decision() count=%ld\n", *hitCount);

    if (++(*hitCount) >= 2) {
        *hitCount = 0;
        return spSetBranchAction (resultAction, shortSound2, SP_MARKER_NAME_BEGIN);
    }

    return spSetBranchAction (resultAction, shortSound1, SP_MARKER_NAME_BEGIN);
}
