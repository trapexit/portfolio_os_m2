
/******************************************************************************
**
**  @(#) tsp_rooms.c 96/08/27 1.19
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name tsp_rooms
|||	Room-sensitive soundtrack example using the Sound Player.
|||
|||	  Format
|||
|||	    tsp_rooms
|||
|||	  Description
|||
|||	    Creates a thread to playback a sound track based on a global room variable
|||	    gRoom.
|||
|||	    The parent task, tsp_rooms, gets control port events and updates gRoom as
|||	    described below.
|||
|||	    The soundtrack thread, Soundtrack, uses the Sound Player to play
|||	    a unique sound file for each room. When the main task changes rooms, the
|||	    soundtrack thread adapts the soundtrack to the change in room at a
|||	    musically convenient location.
|||
|||	    The way this is done is that each room's soundfile is designed so it can
|||	    play in a loop. In addition to this, the end of each sound file, and
|||	    additional optional marked locations within the sound file, must be
|||	    musically sensible locations to transition to the beginning of any of the
|||	    other room's sound files. When playback reaches the next marked position
|||	    in, or the end of, the current room's sound, a default decision function
|||	    is called to check to see if the main task changed room. If the room is
|||	    still the same, then the current sound continues to play, or loops if at
|||	    the end.
|||
|||	    If the room has changed, the SPPlayer is instructed to begin playing the
|||	    sound for the new room.
|||
|||	    Note that the soundtrack adapts to room changes by the main task instead
|||	    of making a sharp transition as soon as the room changes. It is possible
|||	    for the main task to make several room changes before the soundtrack
|||	    thread discovers that the room has changed at all. This results in smooth
|||	    soundtrack transitions that occur at seemingly random locations, which is
|||	    far less likely to become annoying to a game player than a soundtrack that
|||	    predictably changes the instant the player crosses a given threshold.
|||
|||	  Controls
|||
|||	    A
|||	        Enter room 0.
|||
|||	    B
|||	        Enter room 1.
|||
|||	    C
|||	        Enter room 2.
|||
|||	    X (Stop)
|||	        Exit when the sound for the current room gets to a marker.
|||
|||	    Shift-X
|||	        Exit immediately.
|||
|||	  Associated Files
|||
|||	    tsp_rooms.c, words.aiff
|||
|||	  Location
|||
|||	    Examples/Audio/Spooling
|||
|||	  See Also
|||
|||	    spCreatePlayer(), tsp_switcher(@), tsp_algorithmic(@)
**/

#include <audio/audio.h>
#include <audio/soundplayer.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/msgport.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>


/* -------------------- Soundtrack specifications */

    /* Soundtrack thead */
#define SOUNDTRACK_PRIORITY 150
#define SOUNDTRACK_STACK    4096

    /* Soundtrack SPPlayer audio buffers */
#define BUF_COUNT   4
#define BUF_SIZE    22000

    /* Room IDs */
enum {
    ROOM_A,
    ROOM_B,
    ROOM_C,
    ROOM_NROOMS
};

    /* pseudo-room IDs */
#define ROOM_EXIT -1

    /* macro to test if room ID is a real room */
#define IsRoom(room) ((room) >= 0)

    /* Sound file associated with each room. */
const char * const SoundtrackRoomFileNames[ROOM_NROOMS] = {
    "/remote/Samples/PitchedL/PianoGrandFat/GrandPianoFat.C1M44k.aiff",    /* ROOM_A */
    "words.aiff",                                                   /* ROOM_B */
    "/remote/Samples/GMPercussion44k/CuicaOpen.M44k.aiff",                 /* ROOM_C */
};


/* -------------------- Shared data between main and room sound thread */

volatile int32 gRoom = ROOM_A;


/* -------------------- local functions */

    /* macros */
#define MakeUErr(svr,class,err) MakeErr(ER_USER,0,svr,ER_E_USER,class,err)
#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define TestTrailingEdge(newset,oldset,mask) \
    TestLeadingEdge ((oldset),(newset),(mask))

    /* thread */
int32 SoundtrackMain (void);



/* -------------------- main */

Err HandleInput (void);
void EnterRoom (int32 room);

int main (int argc, char *argv[])
{
    Item replyport = 0;
    Item soundtrackthread = 0;
    Err errcode;

    TOUCH(argc);

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    printf ("%s: start.\n", argv[0]);

    if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) goto clean;

        /* create soundtrack thread's completion reply port */
    if ((errcode = replyport = CreateMsgPort ("SoundtrackReply", 0, 0)) < 0)
        goto clean;

        /* start soundtrack thread */
    if ((errcode = soundtrackthread = CreateThreadVA (
        (void (*)())SoundtrackMain, "Soundtrack",
        SOUNDTRACK_PRIORITY, SOUNDTRACK_STACK,
            /* Instruct thread to send message on completion. */
        CREATETASK_TAG_MSGFROMCHILD, replyport,
        TAG_END)) < 0) goto clean;

        /* process control pad events. returns when user presses X button */
    if ((errcode = HandleInput()) < 0) goto clean;

        /* wait for completion message from soundtrack thread to arrive */
    WaitPort (replyport, 0);

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

        /* clean up */
    DeleteThread (soundtrackthread);    /* forcibly kill soundtrack thread in
                                           case of failure from above */
    DeleteMsgPort (replyport);
    KillEventUtility();

    printf ("%s: done.\n", argv[0]);

  #ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
  #endif

    return 0;
}

/*
    This function processes control pad events.

    A, B, C - selects a room to enter
    X       - exits gracefully
    Shift-X - terminates forcefully
*/
Err HandleInput (void)
{
    ControlPadEventData cped;
    int32 joy, oldjoy = 0;
    Err errcode;

    for (;;) {

            /* wait for a control pad event */
        if ((errcode = GetControlPad (1, TRUE, &cped)) < 0) goto clean;
        joy = cped.cped_ButtonBits;

            /* X (Stop) button: exit */
        if (TestLeadingEdge (joy, oldjoy, ControlX)) {
            EnterRoom (ROOM_EXIT);

                /* If either shift button is down, return an error code
                   to cause main() to exit without waiting for thread
                   to shut down. */
            errcode = (joy & (ControlLeftShift | ControlRightShift))
                ? MakeUErr (ER_INFO, ER_C_STND, ER_Aborted)
                : 0;

            goto clean;
        }

            /* If A, B, or C was pressed, enter the corresponding room */
        if (TestLeadingEdge (joy, oldjoy, ControlA)) EnterRoom (ROOM_A);
        if (TestLeadingEdge (joy, oldjoy, ControlB)) EnterRoom (ROOM_B);
        if (TestLeadingEdge (joy, oldjoy, ControlC)) EnterRoom (ROOM_C);

        oldjoy = joy;
    }

clean:
    return errcode;
}

/*
    Enters a new room.

    Simply sets the global gRoom variable that the soundtrack thread
    checks at musically sensible times.
*/
void EnterRoom (int32 newroom)
{
    printf ("tsp_rooms: entering room %ld\n", newroom);
    gRoom = newroom;
}


/* -------------------- Room Sound Thread */

    /* data structure to pass to default decision function */
typedef struct SoundtrackDecisionData {
        /* static */
    SPSound *stdd_Sounds[ROOM_NROOMS];  /* array of SPSounds associated
                                           with each room */

        /* variable */
    int32   stdd_CurRoom;               /* ROOM_ id of current room */
} SoundtrackDecisionData;

Err PlaySoundtrack (void);
Err SoundtrackDecisionFunction (SPAction *, SoundtrackDecisionData *, SPSound *,
                                const char *markername);

/*
    Soundtrack thread's entry point.
*/
int32 SoundtrackMain (void)
{
    Err errcode;

    if ((errcode = OpenAudioFolio()) < 0) goto clean;

    printf ("Soundtrack: start\n");

    errcode = PlaySoundtrack();

clean:
    if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

    CloseAudioFolio();

    printf ("Soundtrack: done\n");

    return 0;
}

/*
    Plays room-sensitive soundtrack.
*/
Err PlaySoundtrack (void)
{
    SoundtrackDecisionData decisiondata;
    Err errcode;
    Item samplerins;
    Item outputins = 0;
    SPPlayer *player = NULL;

        /* Initialize decision data */
    memset (&decisiondata, 0, sizeof decisiondata);

        /* set up Instruments and Connections */
    if ((errcode = samplerins =
        LoadInstrument ("sampler_16_f1.dsp", 0, 100)) < 0) goto clean;
    if ((errcode = outputins =
        LoadInstrument ("line_out.dsp", 0, 100)) < 0) goto clean;

    if ((errcode =
        ConnectInstrumentParts (samplerins, "Output", 0,
                                outputins, "Input", 0 )) < 0) goto clean;
    if ((errcode =
        ConnectInstrumentParts (samplerins, "Output", 0,
                                outputins, "Input", 1)) < 0) goto clean;

    if ((errcode = StartInstrument (outputins, NULL)) < 0) goto clean;

        /* Set up player */
    if ((errcode =
        spCreatePlayer (&player, samplerins,
                        BUF_COUNT, BUF_SIZE, NULL)) < 0) goto clean;

    if ((errcode =
        spSetDefaultDecisionFunction (player,
            (SPDecisionFunction)SoundtrackDecisionFunction,
            &decisiondata)) < 0) goto clean;

        /* Add soundtrack file for each room */
    {
        int32 room;

        for (room = 0; room < ROOM_NROOMS; room++) {
            const char * const roomfilename = SoundtrackRoomFileNames[room];
            SPSound *roomsound;

            printf ("Soundtrack: adding room %ld sound: %s\n",
                    room, roomfilename);

                /* Add the room's sound file to the SPPlayer */
            if ((errcode =
                spAddSoundFile (&roomsound, player, roomfilename)) < 0) {

                printf ("Unable to load sound '%s'\n", roomfilename);
                goto clean;
            }

                /* Set it to loop */
            if ((errcode = spLoopSound (roomsound)) < 0) goto clean;

                /* store the SPSound in the decision data */
            decisiondata.stdd_Sounds[room] = roomsound;
        }
    }

        /*
            Start initial room's sound, if there is one.

            If the main task is trying to quit already, don't start reading.
            This causes the service loop to terminate just as if there were no
            more sound read.
        */
    if (IsRoom (decisiondata.stdd_CurRoom = gRoom)) {
        printf ("Soundtrack: starting sound for room %d\n",
            decisiondata.stdd_CurRoom);

        if ((errcode =
            spStartReading (
                decisiondata.stdd_Sounds [decisiondata.stdd_CurRoom],
                SP_MARKER_NAME_BEGIN)) < 0) goto clean;
    }

        /* start playing */
    if ((errcode = spStartPlayingVA (player,
                                     AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(1.0),
                                     TAG_END)) < 0) goto clean;

        /* service the player */
    {
        const int32 playersigs = spGetPlayerSignalMask (player);

            /* stay in this loop until there's no more sound to play */
        while (spGetPlayerStatus (player) & SP_STATUS_F_BUFFER_ACTIVE) {
            const int32 sigs = WaitSignal (playersigs);

            if ((errcode = spService (player, sigs)) < 0) goto clean;
        }
    }

        /* stop play back */
    spStop (player);

clean:
    spDeletePlayer (player);
    UnloadInstrument (outputins);
    UnloadInstrument (samplerins);
    return errcode;
}

/*
    This function is called by the sound player from within spStartReading()
    and spService() when any marker in the current room's sound is reached.
    This function then compares gRoom to its idea of the current room.

    If the room is still the same, this function does nothing, which causes the
    sound player to continue playing the current room's soundtrack, looping if
    necessary if it has reached the end.

    If the room has changed, sets the SPAction to begin playing the new room's
    sound file from the beginning. If the parent task is trying to exit, sets
    the SPAction to stop, which ultimately causes the spService() loop above to
    terminate when all of the enqueued sound has finished playing.
*/
Err SoundtrackDecisionFunction (SPAction *action,
                                SoundtrackDecisionData *decisiondata,
                                SPSound *sound, const char *markername)
{
    const int32 newroom = gRoom;
    Err errcode = 0;

    TOUCH(sound);

    if (decisiondata->stdd_CurRoom != newroom) {
        printf ("Soundtrack: detected change to room %ld in room %ld sound, "
                "marker '%s'\n",
                newroom, decisiondata->stdd_CurRoom, markername);

        decisiondata->stdd_CurRoom = newroom;

        errcode = IsRoom (newroom)
            ? spSetBranchAction (action, decisiondata->stdd_Sounds [newroom],
                                 SP_MARKER_NAME_BEGIN)
            : spSetStopAction (action);
    }

    return errcode;
}
