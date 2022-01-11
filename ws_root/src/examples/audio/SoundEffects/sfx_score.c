
/******************************************************************************
**
**  @(#) sfx_score.c 96/08/27 1.21
**  $Id: sfx_score.c,v 1.22 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name sfx_score
|||	Uses libmusic.a score player as a sound effects manager.
|||
|||	  Format
|||
|||	    sfx_score
|||
|||	  Description
|||
|||	    Demonstrate use of ScoreContext as a simple sound effects manager. This
|||	    gives you dynamic voice allocation, and a simple MIDI-like interface.
|||
|||	    To run this program:
|||
|||	    1) Install the sample library from the recent 3DO tools disk as
|||	    documented.
|||
|||	    If you don't have the sample library, edit this program to use your own
|||	    samples. Search for .aiff.
|||
|||	    2) Run the program, follow printed instructions.
|||
|||	    The idea behind this program is that one can use the virtual MIDI routines
|||	    in the music library as a simple sound effects manager. You can assign
|||	    samples to channels, and then pay those samples using StartScoreNote().
|||	    This will take advantage of the voice allocation, automatic mixer
|||	    connections, and other management code already used for playing scores.
|||	    You do not have to play sequences or load MIDI files to do this.
|||
|||	    You can load your samples based on a PIMap text file using the LoadPIMap()
|||	    routine. This is documented in the manual under score playing. You may
|||	    also load the samples yourself and put them in the PIMap which will give
|||	    you more control. This example program shows you how to do that. Samples
|||	    are assigned to Program Numbers at this step.
|||
|||	    Assigning program numbers to channels is accomplished by calling
|||	    ChangeScoreProgram(). Once you have assigned a program to a channel, you
|||	    can play the sound by calling: StartScoreNote (scon, Channel, Note,
|||	    Velocity);
|||
|||	    Channel selects the sound. Note determines the pitch and may also be used
|||	    to select parts of a multisample, e.g. a drum kit. Velocity controls
|||	    loudness. The voice allocation occurs during this call.
|||
|||	  Implementation
|||
|||	    This example was revised to take advantage of a change made for libmusic.a
|||	    V22 to permit playing multiple voices per MIDI node, which greatly
|||	    facilitates sound effects playback.
|||
|||	  Location
|||
|||	    Examples/Audio/SoundEffects
|||
|||	  Notes
|||
|||	    1) Samples that are attached to an InsTemplate are deleted when the
|||	    Template is deleted. This is caused by setting AF_TAG_AUTO_DELETE_SLAVE
|||	    to TRUE in CreateAttachment. Set it to FALSE if you don't want this to happen.
|||
|||	    2) You can share the ScoreContext with code that plays a score as long as
|||	    you don't overlap the channels or program numbers. You might restrict
|||	    scores to channels 0-7 and programs 0-19. You could then use channels 8-15
|||	    and programs 20-39 for sound effects. The advantage of this is that they
|||	    share the same voice allocation and thus resources will be shared between
|||	    them. If desired, you could instead use separate independent
|||	    ScoreContexts.
|||
|||	  See Also
|||
|||	    CreateScoreContext()
**/

#include <audio/audio.h>
#include <audio/musicerror.h>   /* ML_ERR_ */
#include <audio/parse_aiff.h>   /* LoadSample() */
#include <audio/score.h>
#include <kernel/nodes.h>       /* MKNODEID() */
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>

#define  EXAMPLE_VERSION "V25.0"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */


/* You can substitute your own sample names here.  */
#define LONGSAMPLE_NAME "/remote/Samples/PitchedL/PianoGrandFat/GrandPianoFat.C1M44k.aiff"
#define TRUMPET_NAME    "/remote/Samples/PitchedL/Trumpet/Trumpet.C4LM44k.aiff"
#define SNARE_NAME      "/remote/Samples/GMPercussion44K/Snare.M44k.aiff"
#define PUNCH_NAME      "/remote/Samples/GMPercussion44K/Handclap.M44k.aiff"
#define BANG_NAME       "/remote/Samples/GMPercussion44K/Cowbell2.M44k.aiff"

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		TOUCH(Result); \
		PrintError(NULL, name, NULL, Result); \
		goto cleanup; \
	}

/* -------------------- Bit twiddling macros */

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define TestTrailingEdge(newset,oldset,mask) TestLeadingEdge ((oldset),(newset),(mask))


/*******************************************************************/
/********** Declarations for sfx routines **************************/
/*******************************************************************/


Item sfxCreateAttachment( Item Instrument, Item Sample, char *FIFOName, uint32 Flags);
Item sfxMakeSampleTemplate( Item SampleItem, int32 IfVariable );
Err sfxSetSampleZone( Item Sample, int32 LowNote, int32 BaseNote, int32 HighNote );

/*******************************************************************/
/********** Specific to this example. ******************************/
/*******************************************************************/
#define MAXPROGRAMNUM (40)  /* Could be as high as 128 */

/* You can use channels 0-15 which correspond to MIDI channels 1-16 */
#define TSFX_BASE_CHANNEL (0)

/* You can use programs 0-127. */
#define TSFX_BASE_PROGRAM (12)

/* Max # of simultaneous voices */
#define TSFX_MAX_SCORE_VOICES (8)

/* This is 2* the maximum theoretical amplitude but is safe in practice. Experiment. */
#define TSFX_MAX_AMPLITUDE (2.0/TSFX_MAX_SCORE_VOICES)

/* Use maximum velocity to retain highest fidelity of sample playback. */
#define TSFX_DEFAULT_VELOCITY (127)

/* Arbitrary notes used to select multisamples */
#define LOW_NOTE   (50)
#define HIGH_NOTE  (69)
#define BASE_NOTE  (60)    /* Middle C */

/*
** Priorities range from 0-200.
*/
#define TSFX_NORMAL_PRIORITY    100
#define TSFX_HIGH_PRIORITY      110

/* Maximum voices assigned to a given sound. */
#define TSFX_MAX_VOICES  (8)

/*
** This structure is used by the conveniance routine tsfxLoadSoundEffect().
** Variations and extensions may be useful in your application.
** It is grab bag for information related to a sound.
*/
typedef struct SoundEffect
{
	char    *sfx_SampleName;
	int32    sfx_ProgramNum;
	int32    sfx_Channel;
	int32    sfx_Priority;
	int32    sfx_MaxVoices;
	int32    sfx_IfVariable;
	int32    sfx_Pan;
	Item     sfx_Sample;
	Item     sfx_InsTemplate;
} SoundEffect;

Err tsfxUnloadSoundEffect( ScoreContext *scon, SoundEffect *sfx );
Err  tsfxLoadSoundEffect( ScoreContext *scon, SoundEffect *sfx );
ScoreContext *tsfxCreateContext( void );
int32 TestSFX( void );


/*******************************************************************/
/********** Global Data. *******************************************/
/*******************************************************************/

/* Initialise sound effects structures */
SoundEffect LongSFX =
	{ LONGSAMPLE_NAME, TSFX_BASE_PROGRAM+0, TSFX_BASE_CHANNEL+0,
	TSFX_NORMAL_PRIORITY, TSFX_MAX_VOICES, TRUE, 0 };

    /* note: this instrument has a higher priority than the others */
SoundEffect TrumpetSFX =
	{ TRUMPET_NAME, TSFX_BASE_PROGRAM+1, TSFX_BASE_CHANNEL+1,
	TSFX_HIGH_PRIORITY, TSFX_MAX_VOICES, TRUE, 40 };

SoundEffect SnareSFX =
	{ SNARE_NAME, TSFX_BASE_PROGRAM+2, TSFX_BASE_CHANNEL+2,
	TSFX_NORMAL_PRIORITY, 2, FALSE, 80 };

SoundEffect PunchSFX =
	{ PUNCH_NAME, TSFX_BASE_PROGRAM+3, TSFX_BASE_CHANNEL+3,
	TSFX_NORMAL_PRIORITY, 2, FALSE, 127 };

/*****************************************************************/
int main (int argc, char *argv[])
{
	int32 Result;

/* Prevent compiler warning. */
	TOUCH(argc);

	PRT(("%s, %s\n", argv[0], EXAMPLE_VERSION));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialise control pad. */
	Result = InitEventUtility(1, 0, TRUE);
	CHECKRESULT(Result,"InitEventUtility");

/* Run test. */
	Result = TestSFX( );
	CHECKRESULT( Result, "TestSFX" );

cleanup:

	KillEventUtility();
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));

	return (int) Result;
}

/******************************************************************
** Create Score Context used to keep track of voices.
** Load a mixer large enough to play all the voices.
******************************************************************/
ScoreContext *tsfxCreateContext( void )
{
	int32 Result;
	ScoreContext *scon;

/* Create a context for interpreting a MIDI score and tracking notes. */
	scon = CreateScoreContext ( MAXPROGRAMNUM );
	if( scon == NULL )
	{
		return NULL;
	}

	Result = CreateScoreMixer( scon, MakeMixerSpec(TSFX_MAX_SCORE_VOICES,2,AF_F_MIXER_WITH_LINE_OUT),
		TSFX_MAX_SCORE_VOICES, TSFX_MAX_AMPLITUDE);
	CHECKRESULT(Result,"CreateScoreMixer");

	return scon;

cleanup:
	DeleteScoreContext( scon );
	return NULL;
}

/******************************************************************
** This macro triggers a sound effect if a button is pressed.
** It demonstrates some useful techniques including:
**
** Edge detector for ButtonMask.  Has button state just changed?
******************************************************************/
#define TRIGGER_SFX(ButtonMask,sfx) TRIGGER_MULTI_SFX(ButtonMask,sfx,BASE_NOTE)

#define TRIGGER_MULTI_SFX(ButtonMask,sfx,note) \
		if (TestLeadingEdge (Joy,OldJoy,ButtonMask)) /* Edge detect. */ \
		{ \
			Result = StartScoreNote( scon, sfx.sfx_Channel, (note), TSFX_DEFAULT_VELOCITY ); \
			CHECKRESULT( Result, "StartScoreNote" ); \
		} \
		else if (TestTrailingEdge (Joy,OldJoy,ButtonMask)) \
		{ \
			Result = ReleaseScoreNote( scon, sfx.sfx_Channel, (note), 0 ); \
			CHECKRESULT( Result, "StartScoreNote" ); \
		}

/*****************************************************************/
int32 TestSFX( void )
{
	int32 Result;
	static ControlPadEventData cped;
	ScoreContext *scon;
	uint32 Joy=0, OldJoy;
	int32 DoIt = TRUE;
	int32 TrumpetNote = BASE_NOTE;
	Item  TempSample;

/* Create control structures. */
	scon = tsfxCreateContext();
	if( scon == NULL ) return -1;

/* Print instructions. */
	PRT(("Buttons A,B,C,LeftShift,RightShift = trigger samples.\n"));
	PRT(("Left      = Stop sound started by A button.\n"));
	PRT(("Start     = purge unused instruments.\n"));
	PRT(("Up/Down   = Turn ON/OFF trumpet notes.\n"));
	PRT(("X         = Quit\n"));

/* Load samples and setup for playback. */
	Result = tsfxLoadSoundEffect( scon, &LongSFX );
	CHECKRESULT(Result, "sfxLoadSoundEffect");
	Result = tsfxLoadSoundEffect( scon, &TrumpetSFX );
	CHECKRESULT(Result, "sfxLoadSoundEffect");
	Result = tsfxLoadSoundEffect( scon, &SnareSFX );
	CHECKRESULT(Result, "sfxLoadSoundEffect");
	Result = tsfxLoadSoundEffect( scon, &PunchSFX );
	CHECKRESULT(Result, "sfxLoadSoundEffect");

/* Set up a multisample with zones for the PunchSFX.
** You will get different samples for different Note values:
**
**  Note Range              Sample Name
**  ----------              -----------
**  LOW_NOTE..BASE_NOTE-1   PUNCH_NAME
**  BASE_NOTE..HIGH_NOTE    BANG_NAME
*/
	Result = sfxSetSampleZone( PunchSFX.sfx_Sample, LOW_NOTE, BASE_NOTE, BASE_NOTE-1 );
	CHECKRESULT(Result, "sfxSetSampleZone");

/* Load another sample and give it a different zone.
** Attach it to an existing template to make it a multisample.
*/
	PRT(("Loading %s\n", BANG_NAME ));
	TempSample = LoadSample( BANG_NAME );
	CHECKRESULT(TempSample, "LoadSample");
	Result = sfxSetSampleZone( TempSample, BASE_NOTE, BASE_NOTE, HIGH_NOTE );
	CHECKRESULT(Result, "sfxSetSampleZone");
	Result = sfxCreateAttachment( PunchSFX.sfx_InsTemplate, TempSample, 0, AF_ATTF_FATLADYSINGS);
	CHECKRESULT(Result, "sfxCreateAttachment");

	while( DoIt )
	{
		Result = GetControlPad (1, TRUE, &cped);
		CHECKRESULT(Result,"GetControlPad");
		OldJoy = Joy;
		Joy = cped.cped_ButtonBits;

/* Trigger sound effects from buttons. */
		TRIGGER_SFX( ControlA, LongSFX );
		TRIGGER_SFX( ControlB, TrumpetSFX );
		TRIGGER_SFX( ControlC, SnareSFX );
		TRIGGER_MULTI_SFX( ControlLeftShift, PunchSFX, LOW_NOTE );
		TRIGGER_MULTI_SFX( ControlRightShift, PunchSFX, HIGH_NOTE );

/* Stop LongSFX on Left */
		if (TestLeadingEdge (Joy,OldJoy,ControlLeft))
		{
			Result = StopScoreNote( scon, LongSFX.sfx_Channel, BASE_NOTE);
			CHECKRESULT( Result, "StopScoreNote" );
		}

/*
** Use Up/Down to turn on/off lots of sustaining notes to torture
** voice allocator.
*/
		if (TestLeadingEdge (Joy,OldJoy,ControlUp))
		{
			if( ++TrumpetNote > HIGH_NOTE ) TrumpetNote = HIGH_NOTE;
			PRT(("TrumpetNote %d ON\n", TrumpetNote));
			Result = StartScoreNote( scon, TrumpetSFX.sfx_Channel, TrumpetNote, 64 );
			CHECKRESULT( Result, "StartScoreNote" );
		}
		else if (TestLeadingEdge (Joy,OldJoy,ControlDown))
		{
			Result = ReleaseScoreNote( scon, TrumpetSFX.sfx_Channel, TrumpetNote, 64 );
			CHECKRESULT( Result, "StartScoreNote" );
			PRT(("TrumpetNote %d OFF\n", TrumpetNote));
			if( --TrumpetNote < LOW_NOTE ) TrumpetNote = LOW_NOTE;
		}

/*
** PurgeScoreInstrument will delete high priority instruments from the ScoreContext
** that have stopped but are still hogging the DSPP resources.
** Call it whenever you have trouble allocating a DSPP instrument.
*/
		if (TestLeadingEdge (Joy,OldJoy,ControlStart))
		{
			int32 NumIns = 0;
			while(PurgeScoreInstrument( scon, SCORE_MAX_PRIORITY, AF_STOPPED ) > 0)
			{
				NumIns++;
			}
			PRT(("Purged %d stopped instruments from score context.\n", NumIns));
		}

/* Time to quit? */
		DoIt = !(Joy & ControlX);
	}

cleanup:

/* Unload the sound effects. */
	Result = tsfxUnloadSoundEffect( scon, &LongSFX );
	CHECKRESULT(Result, "sfxUnloadSoundEffect");
	Result = tsfxUnloadSoundEffect( scon, &TrumpetSFX );
	CHECKRESULT(Result, "sfxUnloadSoundEffect");
	Result = tsfxUnloadSoundEffect( scon, &SnareSFX );
	CHECKRESULT(Result, "sfxUnloadSoundEffect");
	Result = tsfxUnloadSoundEffect( scon, &PunchSFX );
	CHECKRESULT(Result, "sfxUnloadSoundEffect");

	DeleteScoreMixer( scon );
	DeleteScoreContext( scon );

	return Result;
}

/*****************************************************************/
Err tsfxUnloadSoundEffect( ScoreContext *scon, SoundEffect *sfx )
{
	int32	Result=0;

	if (sfx->sfx_InsTemplate)
	{
		UnloadInsTemplate( sfx->sfx_InsTemplate );
		sfx->sfx_InsTemplate = 0;
		SetPIMapEntry( scon, sfx->sfx_ProgramNum, 0, 0, 0);
	}

	sfx->sfx_Sample = 0;

	return Result;
}

/******************************************************************
** Load a sample from disk and add it to the PIMap.
******************************************************************/
Err tsfxLoadSoundEffect( ScoreContext *scon, SoundEffect *sfx )
{
	int32 Result;

	PRT(("Loading %s\n", sfx->sfx_SampleName ));
	sfx->sfx_Sample = LoadSample( sfx->sfx_SampleName );
	CHECKRESULT(sfx->sfx_Sample, "LoadSample");

	sfx->sfx_InsTemplate = sfxMakeSampleTemplate( sfx->sfx_Sample, sfx->sfx_IfVariable );
	CHECKRESULT( sfx->sfx_InsTemplate, "tsfxMakeSampleTemplate");

	Result = SetPIMapEntry( scon, sfx->sfx_ProgramNum, sfx->sfx_InsTemplate,
		sfx->sfx_MaxVoices, sfx->sfx_Priority );
	CHECKRESULT( Result, "SetPIMapEntry");

	Result = ChangeScoreProgram( scon, sfx->sfx_Channel, sfx->sfx_ProgramNum );
	CHECKRESULT( Result, "ChangeScoreProgram");

/* Set PAN which is MIDI controller #10. */
	Result = ChangeScoreControl( scon, sfx->sfx_Channel, 10, sfx->sfx_Pan );
	CHECKRESULT( Result, "ChangeScoreControl");

cleanup:
	return Result;
}

/*****************************************************************/
/********* SoundFX Source Library ********************************/
/*****************************************************************/

/******************************************************************
** Set the MIDI pitch range for a sample for multisampling.
** It will play if the Note is between LowNote and HighNote.
** BaseNote is the actual Note that was recorded.  Use 60, middle C, as a default.
******************************************************************/
Err sfxSetSampleZone( Item Sample, int32 LowNote, int32 BaseNote, int32 HighNote )
{
	return SetAudioItemInfoVA (Sample,
		AF_TAG_LOWNOTE,  LowNote,
		AF_TAG_BASENOTE, BaseNote,
		AF_TAG_HIGHNOTE, HighNote,
		TAG_END);
}


/******************************************************************
** Create an attachment between Instrument and Sample with Flags.
******************************************************************/
Item sfxCreateAttachment( Item Instrument, Item Sample, char *FIFOName, uint32 Flags)
{
	return CreateAttachmentVA (Instrument, Sample,
		AF_TAG_NAME,   FIFOName,
		AF_TAG_SET_FLAGS,  Flags,
		AF_TAG_AUTO_DELETE_SLAVE, TRUE,   /* Delete sample when template deleted. */
		TAG_END);
}


/******************************************************************
** Make a template for a sample so that it can be played by StartScoreNote()
******************************************************************/
Item sfxMakeSampleTemplate( Item SampleItem, int32 IfVariable )
{
	Item TemplateItem = 0;
	char InstrumentName[AF_MAX_NAME_SIZE];
	Item Attachment;
	int32 Result;

/* What sample is best for playing this format sample? */
	Result = SampleItemToInsName( SampleItem , IfVariable, InstrumentName, AF_MAX_NAME_SIZE );
	if (Result < 0)
	{
		ERR(("No instrument to play that sample.\n"));
		Result = ML_ERR_UNSUPPORTED_SAMPLE;
		goto cleanup;
	}

/* Load the DSP template for the instrument. */
	TemplateItem = LoadInsTemplate (InstrumentName, NULL);
	if (TemplateItem < 0)
	{
		ERR(("LoadPIMap failed for %s\n", InstrumentName));
		Result = TemplateItem;
		goto cleanup;
	}

/*
** Attach the sample to the instrument and set FATLADYSINGS bit.
** This will cause it to automatically stop executing on the DSPP
** when the sample finishes.  Thus it will be available for stealing or
** adoption by the voice allocator.
*/
	Attachment = sfxCreateAttachment( TemplateItem, SampleItem, 0, AF_ATTF_FATLADYSINGS);
	if (Attachment < 0) {
		ERR(("Attachment failed for %s\n", InstrumentName));
		Result = Attachment;
		goto cleanup;
	}

	return TemplateItem;

cleanup:
	UnloadInsTemplate (TemplateItem);
	return Result;
}
