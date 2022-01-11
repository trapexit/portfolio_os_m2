/******************************************************************************
**
**  @(#) playsf.c 96/08/28 1.11
**
**  playsf.c
**  An example of using the Sound Player to play back, pause and
**  stop a soundfile from the CD.
**
**  Author: rnm
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name playsf
|||	Play, pause and stop playback of a sound file from disk using the Sound Player.
|||
|||	  Format
|||
|||	    playsf [filename]
|||
|||	  Description
|||
|||	    This program plays back a sound file from disk using a background
|||	    thread.  The foreground task monitors the control pad; you can pause,
|||	    restart and stop playback using the play/pause and stop buttons
|||	    respectively.  The example uses Messages to send commands from the
|||	    foreground task to the background thread.
|||
|||	  Arguments
|||
|||	    [filename]
|||	        File name of an AIFF/AIFC file.  If you don't provide one, the example
|||	        file "spA.cbd2" in the MarkovMusic(@) directory will  be used.
|||
|||	  Associated Files
|||
|||	    spA.cbd2
|||
|||	  Caveats
|||
|||	    The control pad handling function uses GetControlPad() with wait enabled,
|||	    which means the foreground task sits and waits for button presses even
|||	    after the soundfile has finished playing and the background thread has
|||	    politely cleaned up after itself.  You need to hit the Stop button to
|||	    terminate the program.
|||
|||	  Location
|||
|||	    Examples/Audio/Spooling
|||
|||	  See Also
|||
|||	    tsp_spoolsoundfile(@)
**/

#include <audio/audio.h>
#include <audio/soundplayer.h>
#include <audio/parse_aiff.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/msgport.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PRT(x)  { printf x; }
#define ERR(x)  PRT(x)
#define DBUG(x) /* PRT(x) */

	/* Soundtrack thead */
#define PLAYSF_PRIORITY 150
#define PLAYSF_STACK 4096

	/* Soundtrack SPPlayer audio buffers */
#define BUF_COUNT   4
#define BUF_SIZE 22000

	/* Command set for sf player */
#define SF_COMMAND_STOP 1
#define SF_COMMAND_PAUSE 2
#define SF_COMMAND_RESUME 3
#define SF_COMMAND_START 4

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )

/*
	Macro to simplify error checking.
	expression is only evaluated once and should return an int32
	(or something that can be cast to it sensibly)
*/
#define TRAP_ERROR(_exp,_desc) \
	do { \
		errcode = (_exp); \
		DBUG((_desc)); \
		DBUG(("\n")); \
		if (errcode < 0) { \
			PrintError (NULL, _desc, NULL, errcode); \
			goto clean; \
		} \
	} while (0)

/* ---- global variables */

volatile int32 msgSignal;
char* sfSoundFilename;
SPSound* sfSound = NULL;

/* ---- functions */

Err HandleInput (Item aThread, Item parentport, Item childport, Item msgItem);
Err TellThread ( int32 msg, Item parentPort, Item childport, Item msgItem );

	/* thread */
int32 sfMain (void);
/******************************************************************************/
int main (int argc, char *argv[])
{
	Item parentMsgPort = 0, childMsgPort;
	Item msgItem = 0;
	Item soundtrackthread = 0;
	Err errcode;

	TOUCH(argc);

	printf ("%s: start.\n", argv[0]);

	/* Retrieve command line arguments */
	sfSoundFilename = (argc > 1) ? argv[1] : "../MarkovMusic/spA.cbd2";

	if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) goto clean;

		/* create soundtrack thread's reply port */
	if ((errcode = parentMsgPort = CreateMsgPort ("parentMsgPort", 0, 0)) < 0)
		goto clean;

		/* create small message to contain command */
	if ((errcode = msgItem = CreateSmallMsg("sfCommand",0,parentMsgPort)) < 0)
		goto clean;

		/* start soundtrack thread */
	if ((errcode = soundtrackthread = CreateThreadVA (
	  (void (*)())sfMain, "playsf",
	  PLAYSF_PRIORITY, PLAYSF_STACK,
	  CREATETASK_TAG_MSGFROMCHILD, parentMsgPort,
	  CREATETASK_TAG_DEFAULTMSGPORT, 0,
	  TAG_END)) < 0) goto clean;

		/* get child's message port from structure while child waits */
	childMsgPort = THREAD(soundtrackthread)->t_DefaultMsgPort;

		/* send a message to the child to proceed */
	if ((errcode = TellThread(SF_COMMAND_START, parentMsgPort, childMsgPort,
	  msgItem)) < 0) goto clean;

		/* process control pad events. returns when user presses X button */
	if ((errcode = HandleInput(soundtrackthread, parentMsgPort, childMsgPort,
	  msgItem)) < 0) goto clean;

		/* wait for completion message from soundtrack thread to arrive */
	WaitPort (parentMsgPort, 0);

DBUG(("Thread finished, terminating main()\n"));

clean:
	if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

		/* clean up */
	DeleteThread(soundtrackthread);    /* forcibly kill soundtrack thread in
                                           case of failure from above */
	DeleteMsg(msgItem);
	DeleteMsgPort(parentMsgPort);
	KillEventUtility();

	printf ("%s: done.\n", argv[0]);

	return 0;
}

/******************************************************************************/
/*
This function processes control pad events.

Play    - toggle pause/resume
X       - exits gracefully
*/
Err HandleInput (Item aThread, Item parentport, Item childport, Item msgItem)
{
	ControlPadEventData cped;
	int32 joy, oldjoy = 0;
	Err errcode = 0;
	int32 lastCommand = 0;
	int32 done = FALSE;

	while(!done)
	{
		/* wait for a control pad event */
		if ((errcode = GetControlPad (1, TRUE, &cped)) < 0) goto clean;
		joy = cped.cped_ButtonBits;

		/* is the thread out there? */
		if (LookupItem(aThread))
		{
			/* X (Stop) button: exit */
			if (TestLeadingEdge (joy, oldjoy, ControlX))
			{
				/* signal a nice exit */
				if ((errcode = TellThread(SF_COMMAND_STOP, parentport,
				  childport, msgItem)) < 0) goto clean;
				PRT(("Stopped.\n"));

				done = TRUE;
			}

			/* handle pause/resume */
			if (TestLeadingEdge (joy, oldjoy, ControlStart))
			{
				if (lastCommand != SF_COMMAND_PAUSE)
				{
				    if ((errcode = TellThread(SF_COMMAND_PAUSE, parentport,
				      childport, msgItem)) < 0) goto clean;
				    lastCommand = SF_COMMAND_PAUSE;
				    PRT(("Paused..."));
				}
				else if (lastCommand != SF_COMMAND_RESUME)
				{
				    if ((errcode = TellThread(SF_COMMAND_RESUME, parentport,
				      childport, msgItem)) < 0) goto clean;
				    lastCommand = SF_COMMAND_RESUME;
				    PRT(("Playing.\n"));
				}
			}

			oldjoy = joy;
		}
		else
		{
			/* Thread has stopped, must have run out of data */
			done = TRUE;
		}
	}

clean:
	return errcode;
}
/******************************************************************************/
Err TellThread ( int32 msg, Item parentPort, Item childPort, Item msgItem )
{
	Err errcode;

DBUG(("TellThread: msg %d, parentPort %d, msgItem %d\n",
  parentPort, msgItem));

	/* send message to thread */
	if ((errcode = SendSmallMsg(childPort, msgItem, msg, 0)) < 0) goto clean;

	/* wait for reply */
	WaitPort(parentPort, msgItem);

DBUG(("Parent: received reply\n"));

	return (0);

clean:
	return (errcode);
}

/******************************************************************************/
/* ---- thread functions */

static Err sfInit( Item* samplerins, Item* outputins, SPPlayer** player );
static void sfTerm( Item samplerins, Item outputins, SPPlayer* player );
static Err sfRun( SPPlayer* player );

/******************************************************************************/
int32 sfMain ( void )
{
	Err errcode;
	Item samplerins = 0;
	Item outputins = 0;
	SPPlayer* player = NULL;
	Item startMsg;

DBUG(("Thread says: I'm alive!!\n"));

	/* wait for START message from parent */
	TRAP_ERROR( (startMsg = WaitPort(CURRENTTASK->t_DefaultMsgPort, 0)),
	  "wait for start message");
	TRAP_ERROR( ReplySmallMsg(startMsg, 0, 0, 0), "acknowledge start message");

	/* start playback */
	TRAP_ERROR( OpenAudioFolio(), "open audio folio" );
	TRAP_ERROR( sfInit( &samplerins, &outputins, &player ), "initialize playsf" );
	TRAP_ERROR( sfRun( player ), "run playsf" );

clean:
	sfTerm( samplerins, outputins, player );
	CloseAudioFolio();

	return errcode;
}

/******************************************************************************/
static Err sfInit( Item* samplerins, Item* outputins, SPPlayer** player )
{
	#define SAMPLER_NAME_LENGTH 40

	Err errcode;
	SampleInfo* aSampleInfo = NULL;
	char samplerName[SAMPLER_NAME_LENGTH];
	uint32 compInfo;

	/* look at sample to discover what type of playback instrument is required */
	TRAP_ERROR ( GetAIFFSampleInfo (&aSampleInfo, sfSoundFilename,
	  ML_GETSAMPLEINFO_F_SKIP_DATA), "get sample information" );
	DBUG(("GetSampleInfo: compression = 0x%x, channels = %i\n", aSampleInfo->smpi_CompressionType,
	  aSampleInfo->smpi_Channels));
	
	compInfo = aSampleInfo->smpi_CompressionType ? aSampleInfo->smpi_CompressionType : aSampleInfo->smpi_Bits;
	TRAP_ERROR ( SampleFormatToInsName (compInfo,
	  FALSE, aSampleInfo->smpi_Channels, samplerName, SAMPLER_NAME_LENGTH),
	  "derive sampler filename" );
	DBUG(("SampleFormatToInsName: instrument is %s\n", samplerName));
	
	/* make output instruments */
	TRAP_ERROR ( *samplerins = LoadInstrument (samplerName, 0, 100),
	  "Load sample player instrument" );
	TRAP_ERROR ( *outputins = LoadInstrument ("line_out.dsp", 0, 100),
	  "Load output instrument" );

	/* connect instruments */
	TRAP_ERROR ( ConnectInstrumentParts (*samplerins, "Output", AF_PART_LEFT, *outputins,
	  "Input", AF_PART_LEFT), "Connect sampler to output left" );
	if( aSampleInfo->smpi_Channels > 1 )
	{
		TRAP_ERROR ( ConnectInstrumentParts (*samplerins, "Output", AF_PART_RIGHT, *outputins,
		  "Input", AF_PART_RIGHT), "Connect stereo sampler to output right" );
	}
	else
	{
		TRAP_ERROR ( ConnectInstrumentParts (*samplerins, "Output", AF_PART_LEFT, *outputins,
		  "Input", AF_PART_RIGHT), "Connect mono sampler to output right" );
	}
	
	TRAP_ERROR ( StartInstrument (*outputins, NULL), "Start output" );

	/* create player */
	TRAP_ERROR ( spCreatePlayer (player, *samplerins, BUF_COUNT, BUF_SIZE, NULL),
	  "create SPPlayer" );

	/* add the disc-based sound files */
	TRAP_ERROR ( spAddSoundFile (&sfSound, *player, sfSoundFilename),
	  "add sound" );

	/* spDumpPlayer( *player ); */
	
clean:
	/* delete aSampleInfo* structure */
	DeleteSampleInfo( aSampleInfo );
	
	return errcode;
}

/******************************************************************************/
static void sfTerm( Item samplerins, Item outputins, SPPlayer* player )
{
DBUG(("Unloading player\n"));
	spDeletePlayer (player);
DBUG(("Unloading outputIns\n"));
	UnloadInstrument (outputins);
DBUG(("Unloading samplerIns\n"));
	UnloadInstrument (samplerins);
}

/******************************************************************************/
static Err sfRun( SPPlayer* player )
{
	Err errcode;
	int32 threadSignals, playerSignals;
	int32 done = FALSE;
	Item msgItem;
	Message *msg;

	/* start playing */
	TRAP_ERROR ( spStartReading (sfSound, SP_MARKER_NAME_BEGIN),
		"Start reading" );

DBUG(("Reading ..."));

	TRAP_ERROR ( spStartPlayingVA (player,
	  AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(1.0),
	  TAG_END),
	  "Start playing" );

DBUG(("Playing\n"));

	playerSignals = spGetPlayerSignalMask (player);
	msgSignal = MSGPORT(CURRENTTASK->t_DefaultMsgPort)->mp_Signal;

	/* service player until we're out of data, or we've gotten a STOP message */
	while (!done)
	{
		threadSignals = WaitSignal (playerSignals | msgSignal);

DBUG(("Thread: got signals 0x%x\n", threadSignals));

		if (threadSignals & playerSignals)
		{
			TRAP_ERROR (spService (player, (threadSignals & playerSignals)),
			  "service player");

			/* check to see if we've run out of data to read */
			if (!(spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE))
			  done = TRUE;
		}

		if (threadSignals & msgSignal)      /* at least one message from parent */
		{
			/* read all messages in queue */
			while ((msgItem = GetMsg (CURRENTTASK->t_DefaultMsgPort)) > 0)
			{
					msg = MESSAGE(msgItem);

DBUG(("Thread: read message containing data %d\n", msg->msg_Val1));

					if (msg->msg_Val1 == SF_COMMAND_STOP)
					{
						done = TRUE;
					}
					else if (msg->msg_Val1 == SF_COMMAND_PAUSE)
					{
						TRAP_ERROR ( spPause (player), "Pause" );
					}
					else if (msg->msg_Val1 == SF_COMMAND_RESUME)
					{
						TRAP_ERROR ( spResume (player), "Resume" );
					}

					ReplySmallMsg(msgItem,0,0,0);

DBUG(("Thread: replied to message\n"));

			}
		}
	}

	/* all done, stop player and clean up */
	TRAP_ERROR ( spStop (player), "Stop" );

DBUG(("Stopping\n"));

	return 0;

clean:
	return errcode;
}

