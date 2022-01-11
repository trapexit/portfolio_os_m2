
/******************************************************************************
**
**  @(#) loopstereosoundfile.c 95/10/05 1.8
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/soundplayer.h>
#include <kernel/task.h>        /* WaitSignal() */
#include <kernel/types.h>
#include "loopstereosoundfile.h"

#ifdef DEBUG
    #define DIAGNOSE_SYSERR(i,x)	{									\
									printf("Error ("__FILE__"): ");		\
									printf x;							\
									PrintfSysErr((Item)(i));			\
									}
#else
    #define DIAGNOSE_SYSERR(i,x)
#endif


#define NUMBUFFERS 4
#define NUMBLOCKS (32)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS*BLOCKSIZE)

Err LoopStereoSoundFile( const char* filename )
	/*
		Play a stereo sound file in a dedicated endless loop.

		Loads and connects sample player instrument to an output instrument, sets
		up a sound player to play the sound file in a continuous loop, and then
		enters an infinite loop to play the sound file.

		Returns an error code (a negative number) if any operation fails.
	*/
    {

    int32 Result;
    SPPlayer *player = NULL;
    SPSound *sound = NULL;
    Item samplerins;
    Item outputins=0;

        /* set up instruments */
    if ( (samplerins = LoadInstrument ("sampler_16_f2.dsp", 0, 100)) < 0 )
        {
        DIAGNOSE_SYSERR( samplerins, ("Can't load sampler_16_f2.dsp\n"));
		Result = samplerins;
        goto CLEANUP;
        }
    if ( (outputins = LoadInstrument ("line_out.dsp", 0, 100)) < 0 )
        {
        DIAGNOSE_SYSERR( outputins, ("Can't load line_out.dsp\n"));
		Result = outputins;
        goto CLEANUP;
        }
    if ( (Result = ConnectInstrumentParts (samplerins, "Output", AF_PART_LEFT, outputins, "Input", AF_PART_LEFT)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't connect instruments\n"));
        goto CLEANUP;
        }
    if ( (Result = ConnectInstrumentParts (samplerins, "Output", AF_PART_RIGHT, outputins, "Input", AF_PART_RIGHT)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't connect instruments\n"));
        goto CLEANUP;
        }
    if ( (Result = StartInstrument (outputins, NULL)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't start output instrument\n"));
        goto CLEANUP;
        }

        /* set up sound player */
    if ( (Result = spCreatePlayer (&player, samplerins, NUMBUFFERS, BUFSIZE, NULL)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't create player\n"));
        goto CLEANUP;
        }
    if ( (Result = spAddSoundFile (&sound, player, filename)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't add sound file\n"));
        goto CLEANUP;
        }
    if ( (Result = spLoopSound (sound)) < 0)
        {
        DIAGNOSE_SYSERR( Result, ("Can't loop sound file\n"));
        goto CLEANUP;
        }

        /* start reading and playing sound */
    if ( (Result = spStartReading (sound, SP_MARKER_NAME_BEGIN)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't start reading\n"));
        goto CLEANUP;
        }
    if ( (Result = spStartPlaying (player, NULL)) < 0 )
        {
        DIAGNOSE_SYSERR( Result, ("Can't start playing\n"));
        goto CLEANUP;
        }

        /* service sound file and keep playing forever */
    {
        const int32 playersigs = spGetPlayerSignalMask (player);

        while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE) {
            const int32 sigs = WaitSignal (playersigs);

            if ( (Result = spService (player, sigs)) < 0)
                {
                DIAGNOSE_SYSERR( Result, ("Can't service the sound file\n"));
                goto CLEANUP;
                }
        }
    }

CLEANUP:
    spDeletePlayer (player);
    UnloadInstrument (outputins);
    UnloadInstrument (samplerins);

    return Result;
    }
