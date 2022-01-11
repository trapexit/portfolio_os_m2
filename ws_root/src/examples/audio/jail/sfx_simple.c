
/******************************************************************************
**
**  @(#) sfx_simple.c 96/08/27 1.10
**
******************************************************************************/

/**
|||	!!! disabled AUTODOC -public -class examples -group Audio -name sfx_simple
|||	Simple sound effects manager.
|||
|||	  Format
|||
|||	    sfx_simple
|||
|||	  Description
|||
|||	    This examples demonstrates how to make a simple sound effect manager
|||	    using only the audio folio (as opposed to sfx_score, which uses the
|||	    libmusic.a score player package).
|||
|||	    This sound effects manager depends on a number of things being
|||	    true in order to work:
|||	        1) all sound effects are composed of a single sample,
|||	        2) all samples are of the same format (i.e. a sample player
|||	           instrument template (e.g. sampler.dsp) can play all of the
|||	           samples).
|||	    See sfx_score for a more complex sound effects manager that isn't
|||	    bound by the above rules.
|||
|||	    !!! improve this doc a bit.
|||
|||	  Location
|||
|||	    Examples/Audio
|||
|||	  See Also
|||
|||	    sfx_score
|||
**/

/*
!!! Not quite done:
    . Add Rate and Amplitude knobs for continuous sounds.
    . Don't steal if protected.
    . fix sfxDeleteSound()
*/

#include <audio/audio.h>
#include <audio/handy_tools.h>      /* Choose() */
#include <kernel/list.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>

#define  VERSION "V25.0"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/******************************************************************
** defines to control compilation
******************************************************************/
#define STANDALONE_MODE    /* compile main() for standalone testing */

/******************************************************************
** This section of code could be put in a ".h" file.
******************************************************************/
/* There is one of these for a global context. */
typedef struct SoundEffectsContext
{
	Item      sfxc_InsTemplate;    /* Template for all instruments. */
	int32     sfxc_NumInstruments; /* Number of instruments allocated. */
	Item     *sfxc_Instruments;    /* Array of instruments allocated. */
	Item     *sfxc_LeftGains;      /* Mixer knobs for left side. */
	Item     *sfxc_RightGains;     /* Mixer knobs for right side. */
	Item      sfxc_MixerIns;       /* Mixer to connect instruments to. */
	List      sfxc_SoundList;      /* List of associated sound effect structures. */
} SoundEffectsContext;

/* Each sample has a corresponding SoundEffect structure. */
typedef struct SoundEffect
{
	Node      sfct_Node;
	SoundEffectsContext  *sfct_Context;
	Item      sfct_Sample;
	int32     sfct_Channel;     /* -1 if unassigned. */
	Item     *sfct_Attachments; /* Points to an array of attachments for this sample */
} SoundEffect;

SoundEffectsContext *sfxCreateContext( const char *InstrumentName, int32 NumInstruments );
Err          sfxLoadMixer ( SoundEffectsContext *sfxc, const char *MixerName );
SoundEffect *sfxCreateSound ( SoundEffectsContext *sfxc, Item Sample );
Err          sfxStartSound( SoundEffect *sfct, int32 Frequency, int32 LeftGain, int32 RightGain );

Err          sfxReleaseSound( SoundEffect *sfct );
Err          sfxDeleteSound( SoundEffect *sfct );
Err          sfxUnloadSound( SoundEffect *sfct );
Err          sfxUnloadMixer( SoundEffectsContext *sfxc );
Err          sfxDeleteContext( SoundEffectsContext *sfxc );
Err sfxDeleteAttachment( Item Attachment );
Item sfxCreateAttachment( Item Instrument, Item Sample, const char *FIFOName, uint32 Flags);

#define  SFX_UNASSIGNED_CHANNEL   (-1)
#define  SFCT_F_DONT_STEAL        (1)

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("Try %s\n", name )); \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(NULL, name, NULL, Result); \
		goto cleanup; \
	}

/* Macro to simplify error checking. */
#define CHECKPTR(ptr,name) \
	if (ptr == NULL) \
	{ \
		Result = -1; \
		ERR(("NULL returned from %s\n", name )); \
		goto cleanup; \
	}

/******************************************************************
** Delete Score Context used to keep track of voices.
******************************************************************/
Err sfxDeleteContext( SoundEffectsContext *sfxc )
{
	Node *n;
	int32 Result = -1, i;

	if( sfxc == NULL ) return 0;

	sfxUnloadMixer( sfxc );

/*
** Scan linked list for remaining sounds and delete them.
** Each sound removes itself from list so we can just keep
** getting first one and deleting it.
*/
        n = (Node *) FirstNode( &sfxc->sfxc_SoundList );
        while (ISNODE( &sfxc->sfxc_SoundList, (Node *) n) )
        {
		sfxDeleteSound( (SoundEffect *) n ); /* Removes self from list. */
        	n = (Node *) FirstNode( &sfxc->sfxc_SoundList );
        }

/* Delete all Instruments. */
	if( sfxc->sfxc_Instruments )
	{
		for (i=0; i<sfxc->sfxc_NumInstruments; i++)
		{
			FreeInstrument( sfxc->sfxc_Instruments[i] );
		}
		free( sfxc->sfxc_Instruments );
		sfxc->sfxc_Instruments = NULL;
		sfxc->sfxc_NumInstruments = 0;
	}

/* Unload instrument template. */
	UnloadInsTemplate( sfxc->sfxc_InsTemplate );
	sfxc->sfxc_InsTemplate  = 0;

	free( sfxc );

	return Result;
}
/******************************************************************
** Create Score Context used to keep track of voices.
******************************************************************/
SoundEffectsContext *sfxCreateContext( const char *InstrumentName, int32 NumInstruments )
{
	SoundEffectsContext *sfxc;
	int32 Result, i;

/* Allocate structure and instrument array. */
	sfxc = (SoundEffectsContext *) malloc( sizeof(SoundEffectsContext) );
	if(sfxc == NULL) return NULL;
	memset (sfxc, 0, sizeof(SoundEffectsContext));

	sfxc->sfxc_Instruments = (Item *) malloc( sizeof(Item) * NumInstruments );
	if(sfxc->sfxc_Instruments == NULL)
	{
		goto freesfxc;
	}
	memset (sfxc->sfxc_Instruments, 0, sizeof(Item) * NumInstruments);
	sfxc->sfxc_NumInstruments = NumInstruments;
DBUG(("sfxCreateContext: NumIns = %d\n", sfxc->sfxc_NumInstruments ));

	InitList( &sfxc->sfxc_SoundList, "Sounds" );

/* Allocate Instrument Items */
	sfxc->sfxc_InsTemplate = LoadInsTemplate( InstrumentName, 0 );
	CHECKRESULT(sfxc->sfxc_InsTemplate, "LoadInsTemplate" );

	for( i=0; i<NumInstruments; i++ )
	{
		sfxc->sfxc_Instruments[i] = AllocInstrument( sfxc->sfxc_InsTemplate, 100 );
		CHECKRESULT(sfxc->sfxc_Instruments[i], "AllocInstrument" );
	}
DBUG(("sfxCreateContext: sfxc = 0x%x\n", sfxc ));
	return sfxc;

cleanup:
	sfxDeleteContext( sfxc );
	return NULL;

freesfxc:
	free( sfxc );
	return NULL;
}

/******************************************************************
** Unload Mixer and associated Items
******************************************************************/
Err sfxUnloadMixer( SoundEffectsContext *sfxc )
{
	Node *n;
	int32 Result = -1, i;

	if( sfxc == NULL ) return 0;

/* Delete all Knobs. */
	if( sfxc->sfxc_LeftGains )
	{
		for (i=0; i<sfxc->sfxc_NumInstruments; i++)
		{
			ReleaseKnob( sfxc->sfxc_LeftGains[i] );
		}
		free( sfxc->sfxc_LeftGains );
		sfxc->sfxc_LeftGains = NULL;
	}
	if( sfxc->sfxc_RightGains )
	{
		for (i=0; i<sfxc->sfxc_NumInstruments; i++)
		{
			ReleaseKnob( sfxc->sfxc_RightGains[i] );
		}
		free( sfxc->sfxc_RightGains );
		sfxc->sfxc_RightGains = NULL;
	}

/* Unload mixer. */
	UnloadInstrument( sfxc->sfxc_MixerIns );
	sfxc->sfxc_MixerIns  = 0;

	return Result;
}

/******************************************************************/
Err sfxLoadMixer( SoundEffectsContext *sfxc, const char *MixerName )
{
	int32 Result = -1, i; /* !!! better error code */
	int32 ArraySize;

DBUG(("sfxLoadMixer: NumIns = %d\n", sfxc->sfxc_NumInstruments ));

/* Allocate arrays to store knob items. */
	ArraySize = sizeof(Item) * sfxc->sfxc_NumInstruments;
	sfxc->sfxc_LeftGains = (Item *) malloc( ArraySize );
	if(sfxc->sfxc_LeftGains == NULL)
	{
		goto error;
	}
	memset (sfxc->sfxc_LeftGains, 0, ArraySize);

	sfxc->sfxc_RightGains = (Item *) malloc( ArraySize );
	if(sfxc->sfxc_RightGains == NULL)
	{
		goto freeleft;
	}
	memset (sfxc->sfxc_RightGains, 0, ArraySize);

	sfxc->sfxc_MixerIns = LoadInstrument( MixerName, 0, 180);
	CHECKRESULT(sfxc->sfxc_MixerIns,"InitScoreMixer: LoadInstrument");

/* Grab a knob for each instrument and connect them. */
	for (i=0; i<sfxc->sfxc_NumInstruments; i++)
	{
		char Name[32];

		sprintf( Name, "LeftGain%d", i);   /* Build knob name. */
		sfxc->sfxc_LeftGains[i] = GrabKnob( sfxc->sfxc_MixerIns, Name );
		CHECKRESULT(sfxc->sfxc_LeftGains[i],"GrabKnob");

		sprintf( Name, "RightGain%d", i);
		sfxc->sfxc_RightGains[i] = GrabKnob( sfxc->sfxc_MixerIns, Name );
		CHECKRESULT(sfxc->sfxc_RightGains[i],"GrabKnob");

		sprintf( Name, "Input%d", i);
		Result = ConnectInstruments( sfxc->sfxc_Instruments[i], "Output", /* MONO! */
			sfxc->sfxc_MixerIns, Name );
		CHECKRESULT(Result,"ConnectInstruments");
	}

/* Mixer must be started */
	Result = StartInstrument( sfxc->sfxc_MixerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	return Result;

cleanup:
	if(sfxc->sfxc_RightGains)
	{
		free(sfxc->sfxc_RightGains);
		sfxc->sfxc_RightGains = NULL;
	}

freeleft:
	if(sfxc->sfxc_LeftGains)
	{
		free(sfxc->sfxc_LeftGains);
		sfxc->sfxc_LeftGains = NULL;
	}

error:
	return Result;
}

/******************************************************************/
Err sfxUnloadSound( SoundEffect *sfct )
{
	int32 Result;
	Item  SampleItem;

	if( sfct == NULL ) return 0;

	SampleItem = sfct->sfct_Sample;
	Result = sfxDeleteSound( sfct );
	UnloadSample( SampleItem );

	return Result;
}
/******************************************************************/
SoundEffect *sfxLoadSound( SoundEffectsContext *sfxc, const char *SampleName )
{
	Item SampleItem;
	SoundEffect *sfct = NULL;
	int32 Result;

	SampleItem = LoadSample( SampleName );
	CHECKRESULT(SampleItem, "LoadSample" );

	sfct = sfxCreateSound( sfxc, SampleItem );

cleanup:
	return sfct;
}


/******************************************************************/
Err sfxDeleteSound( SoundEffect *sfct )
{
	SoundEffectsContext *sfxc;
	int32 i;

	if( sfct == NULL ) return 0;

/* Remove from linked list in context. */
	if( sfct->sfct_Context )
	{
		RemNode( (Node *) sfct );
		sfct->sfct_Context = NULL;
	}

/* Delete all attachments. */
	if( sfct->sfct_Attachments )
	{
        #error !!! sfxDeleteSound() is broken - sfxc is never set!
		for (i=0; i<sfxc->sfxc_NumInstruments; i++)
		{
			sfxDeleteAttachment( sfct->sfct_Attachments[i] );
		}
		free( sfct->sfct_Attachments );
		sfct->sfct_Attachments = NULL;
	}

	free( sfct );

	return 0;
}

/******************************************************************/
SoundEffect *sfxCreateSound( SoundEffectsContext *sfxc, Item Sample )
{
	SoundEffect *sfct;
	int32 Result = -1, i;

/* Allocate SoundEffect structure. */
	sfct = (SoundEffect *) malloc( sizeof(SoundEffect) );
	if(sfct == NULL)
	{
		return NULL;
	}
	memset (sfct, 0, sizeof(SoundEffect));

DBUG(("sfxCreateSound: Template = 0x%x, NumIns = %d\n", sfxc->sfxc_InsTemplate, sfxc->sfxc_NumInstruments ));
	sfct->sfct_Attachments = (Item *) malloc( sizeof(Item) * sfxc->sfxc_NumInstruments );
	if(sfct->sfct_Attachments == NULL)
	{
		goto cleanup;
	}
	memset (sfct->sfct_Attachments, 0, sizeof(Item) * sfxc->sfxc_NumInstruments);

	sfct->sfct_Sample = Sample;
	sfct->sfct_Channel = SFX_UNASSIGNED_CHANNEL;

/* Attach sample to each instrument. */
	for (i=0; i<sfxc->sfxc_NumInstruments; i++)
	{
/* Set FATLADYSINGS bit so that instrument will stop when sample finishes playing. */
		sfct->sfct_Attachments[i] = sfxCreateAttachment( sfxc->sfxc_Instruments[i], Sample, NULL, AF_ATTF_FATLADYSINGS);
		CHECKRESULT(sfct->sfct_Attachments[i], "sfxCreateAttachment" );
	}

/* Link to context. */
	AddTail( &sfxc->sfxc_SoundList, (Node *) sfct );
	sfct->sfct_Context = sfxc;

DBUG(("sfxCreateSound: Template = 0x%x, NumIns = %d\n", sfxc->sfxc_InsTemplate, sfxc->sfxc_NumInstruments ));

	return sfct;

cleanup:
	sfxDeleteSound( sfct );

	return NULL;

}

/******************************************************************
** sfxAllocateChannel - allocate an instrument
*/
Err sfxAllocateChannel( SoundEffect *sfct )
{
	SoundEffectsContext *sfxc;
	Item InsToUse;
	int32 ChannelFree = -1;
	int32 ChannelOldest = -1;
	int32 Channel, i;
	int32 Result;
	int32 Status;
	uint32 StartTime, TimeNow;
	int32 LongestDur, ThisDur;

	sfxc = sfct->sfct_Context;
	TimeNow = GetAudioTime();

DBUG(("sfxAllocateChannel: sfxc = 0x%x\n", sfxc ));
DBUG(("sfxAllocateChannel: Template = 0x%x, NumIns = %d\n", sfxc->sfxc_InsTemplate, sfxc->sfxc_NumInstruments ));
DBUG(("sfxAllocateChannel: Time = %d\n", TimeNow ));

/* Is a channel already allocated? !!! */
/* Check DONT_STEAL flag, look for unassigned. */
/* Allocate an instrument by checking to see if any are done. */
/* If none done, use one started the longest time ago. */
	LongestDur = -100;
	for (i=0; i<sfxc->sfxc_NumInstruments; i++)
	{
        TagArg InsTags[] = {
            { AF_TAG_STATUS },
            { AF_TAG_START_TIME },
            TAG_END
        };

		Result = GetAudioItemInfo( sfxc->sfxc_Instruments[i], InsTags );
		CHECKRESULT( Result, "GetAudioItemInfo" );
		Status = (int32) (InsTags[0].ta_Arg);
DBUG(("sfxAllocateChannel: i = %d, Status = %d\n", i, Status ));
		if( Status <= AF_STOPPED )
		{
			ChannelFree = i;
			break;
		}
		else
		{
			StartTime = (int32) (InsTags[1].ta_Arg);
			ThisDur = TimeNow - StartTime;
DBUG(("sfxAllocateChannel: ThisDur = %d\n", ThisDur ));
			if( ThisDur > LongestDur )
			{
				LongestDur = ThisDur;
				ChannelOldest = i;
			}
		}
	}

/* If none are free, stop oldest and use it. */
	if( ChannelFree < 0 )
	{
DBUG(("sfxAllocateChannel: ChannelOldest = %d\n", ChannelOldest ));
		InsToUse = sfxc->sfxc_Instruments[ChannelOldest];
/* This would be a good place to use "depopper.dsp" */
		Result = StopInstrument( InsToUse, NULL );
		CHECKRESULT( Result, "StopInstrument" );
		Channel = ChannelOldest;
	}
	else
	{
	/*	InsToUse = sfxc->sfxc_Instruments[ChannelFree]; */    /* !!! this assignment is unused */
		Channel = ChannelFree;
	}

	return Channel;
cleanup:
	return Result;
}


/******************************************************************
** sfxStartSound - allocate an instrument and start playing sound.
*/
Err  sfxStartSound( SoundEffect *sfct, int32 Frequency, int32 LeftGain, int32 RightGain )
{
	SoundEffectsContext *sfxc;
	Item InsToUse;
	int32 Result;
	int32 Chan;

	sfxc = sfct->sfct_Context;

DBUG(("sfxStartSound: sfxc = 0x%x\n", sfxc ));
	Chan = sfxAllocateChannel( sfct );
	if( Chan < 0 )
	{
		ERR(("sfxStartSound: Could not allocate channel!\n"));
		return -1;
	}
	sfct->sfct_Channel = Chan;
DBUG(("sfxStartSound: Channel = 0x%x\n", sfct->sfct_Channel ));

/* Start Instrument then Attachment. */
	InsToUse = sfxc->sfxc_Instruments[sfct->sfct_Channel];
	Result = StartInstrumentVA (InsToUse,
                                AF_TAG_RATE,      Frequency,
                                AF_TAG_AMPLITUDE, 0x7FFF,
                                TAG_END);

	CHECKRESULT( Result, "StartInstrument" );

	Result = TweakKnob( sfxc->sfxc_LeftGains[sfct->sfct_Channel], LeftGain );
	CHECKRESULT(Result, "TweakKnob");
	Result = TweakKnob( sfxc->sfxc_RightGains[sfct->sfct_Channel], RightGain );
	CHECKRESULT(Result, "TweakKnob");
	Result = StartAttachment( sfct->sfct_Attachments[sfct->sfct_Channel], NULL );
	CHECKRESULT( Result, "StartAttachment" );
cleanup:
	return Result;
}

/******************************************************************/
Err sfxReleaseSound( SoundEffect *sfct )
{
	int32 Result;
	Result = ReleaseAttachment( sfct->sfct_Attachments[sfct->sfct_Channel], NULL );
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


Err sfxDeleteAttachment( Item Attachment )
{
	return DeleteItem( Attachment );
}

/******************************************************************
** Create an attachment between Instrument and Sample with Flags.
******************************************************************/
Item sfxCreateAttachment( Item Instrument, Item Sample, const char *FIFOName, uint32 Flags)
{
	return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE),
                         AF_TAG_HOOKNAME,   FIFOName,
                         AF_TAG_SAMPLE,     Sample,
                         AF_TAG_INSTRUMENT, Instrument,
                         AF_TAG_SET_FLAGS,  Flags,
                         TAG_END);
}

#ifdef STANDALONE_MODE

/*******************************************************************/
/********** Specific to this example. ******************************/
/*******************************************************************/

/* You can substitute your own sample names here.  */
#if 1
#define PIANO_NAME      "$samples/PitchedL/PianoGrandFat/GrandPianoFat.C1M44k.aiff"
#define TRUMPET_NAME    "$samples/PitchedL/Trumpet/Trumpet.C4LM44k.aiff"
#define BELL_NAME       "$samples/GMPercussion44K/Cowbell2.M44k.aiff"
#else
#define PIANO_NAME      "sinewave.aiff"
#define TRUMPET_NAME    "sinewave.aiff"
#define BELL_NAME       "sinewave.aiff"
#endif

/* This should match the mixer that you choose. For 8 voices use mixer8x2.dsp */
#define TSFX_NUM_VOICES (12)
#define TSFX_MIXER_NAME "mixer12x2.dsp"

/* This is 2* the maximum theoretical amplitude but is safe in practice. Experiment. */
#define TSFX_MAX_AMPLITUDE (2*(MAXDSPAMPLITUDE/TSFX_MAX_SCORE_VOICES))

int32 TestSFX( void );

/* -------------------- Bit twiddling macros */

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define TestTrailingEdge(newset,oldset,mask) TestLeadingEdge ((oldset),(newset),(mask))

/*******************************************************************/
/********** Global Data. *******************************************/
/*******************************************************************/

SoundEffect *PianoSound;
SoundEffect *TrumpetSound;
SoundEffect *BellSound;

/*****************************************************************/
int main (int argc, char *argv[])
{
	int32 Result=0;

	PRT(("%s, %s\n", argv[0], VERSION));

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
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
** Triggers a sound effect if a button is pressed.
** It demonstrates some useful techniques including:
**
** Edge detector for ButtonMask.  Has button state just changed?
******************************************************************/
Err TriggerSFX( SoundEffect *sfct, uint32 ButtonMask, uint32 Joy, uint32 OldJoy )
{
	int32 Result = 0;
	if (TestLeadingEdge (Joy,OldJoy,ButtonMask)) /* Edge detect. */
	{
/* Pick random Frequency and Pan values for fun. */
		int32 tempRate  = Choose(0x8000) + 0x2000;
		int32 tempLeft  = Choose(0x2000) + 0x0800;
		int32 tempRight = Choose(0x2000) + 0x0800;
		Result = sfxStartSound( sfct, tempRate, tempLeft, tempRight );
		CHECKRESULT( Result, "sfxStartSound" );
	}
	else if (TestTrailingEdge (Joy,OldJoy,ButtonMask))
	{
		Result = sfxReleaseSound( sfct );
		CHECKRESULT( Result, "sfxReleaseSound" );
	}
cleanup:
	return Result;
}

/*****************************************************************/
int32 TestSFX( void )
{
	int32 Result;
	static ControlPadEventData cped;
	SoundEffectsContext *sfxc;
	uint32 Joy=0, OldJoy;
	int32 DoIt = TRUE;
	Item  TempSample;

/* Create control structures. */
	sfxc = sfxCreateContext( "sampler.dsp", TSFX_NUM_VOICES );
	if( sfxc == NULL ) return -1;

	Result = sfxLoadMixer( sfxc, TSFX_MIXER_NAME );
	CHECKRESULT( Result, "sfxLoadMixer" );

/* Print instructions. */
	PRT(("Buttons A,B,C = trigger samples.\n"));
	PRT(("X         = Quit\n"));

/* Load samples and setup for playback. */
	PRT(("Loading Sounds.\n"));
	PianoSound = sfxLoadSound( sfxc, PIANO_NAME );
	CHECKPTR(PianoSound, "sfxLoadSound");

	TrumpetSound = sfxLoadSound( sfxc, TRUMPET_NAME );
	CHECKPTR(TrumpetSound, "sfxLoadSound");

	BellSound = sfxLoadSound( sfxc, BELL_NAME );
	CHECKPTR(BellSound, "sfxLoadSound");
	PRT(("Ready.\n"));

/* Trigger sounds from button press. */
	while( DoIt )
	{
		Result = GetControlPad (1, TRUE, &cped);
		CHECKRESULT(Result,"GetControlPad");
		OldJoy = Joy;
		Joy = cped.cped_ButtonBits;

/* Trigger sound effects from buttons. */
		Result = TriggerSFX( PianoSound, ControlA, Joy, OldJoy );
		CHECKRESULT(Result,"TriggerSFX");

		Result = TriggerSFX( TrumpetSound, ControlB, Joy, OldJoy );
		CHECKRESULT(Result,"TriggerSFX");

		Result = TriggerSFX( BellSound, ControlC, Joy, OldJoy );
		CHECKRESULT(Result,"TriggerSFX");

/* Time to quit? */
		DoIt = !(Joy & ControlX);
	}

cleanup:
	sfxUnloadSound( PianoSound );
	sfxUnloadSound( TrumpetSound );
	sfxUnloadSound( BellSound );
	sfxUnloadMixer( sfxc );
	sfxDeleteContext( sfxc );

	return Result;
}
#endif /* STANDALONE_MODE */
