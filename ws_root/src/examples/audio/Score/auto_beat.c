/******************************************************************************
**
**  @(#) auto_beat.c 96/08/27 1.17
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name auto_beat
|||	Automatic rhythm demo that uses lots of AIFF library samples.
|||
|||	  Format
|||
|||	    auto_beat [pimap file]
|||
|||	  Description
|||
|||	    Play LOTS of notes pseudo-randomly. Demonstrate direct use of ScoreContext.
|||
|||	  Arguments
|||
|||	    [pimap file]
|||	        Name of a PIMAP file to use. Defaults to auto_beat.pimap.
|||
|||	  Associated Files
|||
|||	    auto_beat.c, auto_beat.pimap, /remote/Samples
|||
|||	  Location
|||
|||	    Examples/Audio/Score
**/

#include <audio/audio.h>
#include <audio/music.h>
#include <kernel/operror.h>
#include <kernel/random.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>             /* rand() */

#define  VERSION "V0.3"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("try %s\n", name )); \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

#define kMaxProgramNum   ( 32)  /* Could be as high as 128 */
#define kMaxScoreVoices  ( 16)
#define kNumChannelsBeat   ( 0)
#define kNumChannelsWacky  ( 16)
#define kNumChannels       (kNumChannelsBeat + kNumChannelsWacky)

/* These parameters affect the musical outcome. */
#define kTicksPerBeat    (  50)
#define kNumSlots        ( 200)
#define kMaxProb         (1000)
#define kSleepProb       ( 200)
#define kNotesDesired    ((kNumSlots*3)/8)
#define kChangeNoteProb  (  30)
#define kChangeSleepProb (  20)
#define kNoteRange       (  24)
#define kLoudness        ( 2.0)

#define MIDI_PAN_CONTROLLER   ( 10)

#ifndef ABS
	#define ABS(x)      ((x<0)?(-(x)):(x))
#endif
static int8 gChannelSlots[kNumSlots];
static int8 gNoteSlots[kNumSlots];
static int8 gVelocitySlots[kNumSlots];
static int8 gSleepSlots[kNumSlots];
static int8 gPanSlots[kNumSlots];

int32 PlayCacophony( char *mapfile );

static void DumpNoteArrays( void )
{
	int32 i;
	for( i=0; i<kNumSlots; i++ )
	{
		PRT(("%3d - Chan = %2d, Note = %2d",
			i, gChannelSlots[i], gNoteSlots[i] ));
		PRT((", Vel = %3d, Pan = %3d\n", gVelocitySlots[i], gPanSlots[i] ));
	}
}

/*****************************************************************/
int main (int argc, char *argv[])
{
	char *mapfile = "auto_beat.pimap";
	int32 Result;

	PRT(("Cacophony %s\n", VERSION));
	PRT(("Usage: %s <PIMapFilename>\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}

/* Get files from command line. */
	if (argc > 1 ) mapfile = argv[1];

	Result = PlayCacophony(  mapfile );
	CHECKRESULT( Result, "PlayMIDIFile" );

cleanup:
/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));
	return (int) Result;
}

/*******************************************************************/
int32 Choose ( int32 range )
{
        int32 val, r16;

        r16 = rand() & 0xFFFF;
        val = (r16*range) >> 16;
        return val;
}

/*****************************************************************/
int32 PlayCacophony( char *mapfile )
{
	int32 Result;
	Item MyCue=0;
	int32 i;
	ScoreContext *scon;
	int32 DoIt = TRUE;
	ControlPadEventData cped;
	uint32 Buttons, PreviousButtons=0, NewButtons;
	int32 Channel=0, Note, Velocity=0, IfSleep, Pan=0;
	AudioTime NextTime;
	int32 SlotIndex = 0;
	int32 LastTime, ThisTime, ElapsedTime, MaxTime=0;
	int32 OldSlotNote, NotesActual=0;
	int32 BeatCount = 0;
/* State variables that can be performed. */
	int32 CurNotesDesired = kNotesDesired;
	int32 IfFreezeNotes = FALSE;
	int32 IfFreezeSleeps = TRUE;
	int32 CurTicksPerBeat = kTicksPerBeat;

/* Create a context for interpreting a MIDI score and tracking notes. */
	scon = CreateScoreContext ( kMaxProgramNum );
	if( scon == NULL )
	{
		Result = AF_ERR_NOMEM;
		goto cleanup;
	}

	Result = CreateScoreMixer( scon, MakeMixerSpec(kMaxScoreVoices,2,AF_F_MIXER_WITH_LINE_OUT),
		kMaxScoreVoices, kLoudness);
	CHECKRESULT(Result,"CreateScoreMixer");

/* Load Instrument Templates from disk and fill Program Instrument Map. */
	Result = LoadPIMap ( scon, mapfile );
	CHECKRESULT(Result, "LoadPIMap");

	MyCue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(MyCue, "CreateItem Cue");

/* Initialize random number generator. */
	{
		uint32 HardSeed = ReadHardwareRandomNumber();
		PRT(("HardSeed = 0x%x\n", HardSeed ));
		srand(HardSeed);
	}

DBUG(("Cacophony: preset slots.\n"));
	for( i=0; i<kNumSlots; i++ )
	{
		gNoteSlots[i] = 0;
		gSleepSlots[i] = ( Choose(kMaxProb) < kSleepProb );
	}

/* Listen to each channel to see if we like it. */
	NextTime = GetAudioTime();
#if 0
	for( i=0; i<kNumChannels; i++ )
	{
		Result = StartScoreNote( scon, i, 60, 64 );
		CHECKRESULT(Result, "StartScoreNote");
		PRT(("Channel %d\n", i+1 ));
		SleepUntilTime( MyCue, NextTime );
		NextTime += kTicksPerBeat;
	}
#endif

	LastTime = NextTime;

	while( DoIt )
	{
		int32 TempSlotProb;

		/* Increase probability of changing a slot if the number of notes is off. */
		TempSlotProb = (kChangeNoteProb * ( CurNotesDesired + (5*ABS(CurNotesDesired-NotesActual)) )) / CurNotesDesired;

		/* Decide whether to change Slot. */
		DBUG(("Cacophony: change slot?\n"));
		if( !IfFreezeNotes && (Choose(kMaxProb) < TempSlotProb) )
		{
			/* Pick random note, or silence. */
			OldSlotNote = gNoteSlots[SlotIndex];

			/* Use 50% probability of making a note if at desired note level. */
			/* Make new notes more probable if below desired density. */
			if( Choose(kMaxProb) <
			   (( (kMaxProb/2) * ( CurNotesDesired + (2*ABS(CurNotesDesired-NotesActual)) ) ) / CurNotesDesired) )
			{
				DBUG(("Cacophony: pick new stuff.\n"));
				Channel = Choose(kNumChannelsWacky) + kNumChannelsBeat;
				/* Make odd sleepcounts sound louder. */
				Velocity = 32 + Choose( (32 * ((BeatCount&1)+1)));
				Note = Choose(kNoteRange) + 60+12 - kNoteRange;
				Pan = Choose(128);
			}
			else
			{
				Note = 0;
			}

			/* Figure out whether we added or subtracted a note. */
			if( (Note>0) && (OldSlotNote==0) )
			{
				NotesActual++;
			}
			else if( (Note==0) && (OldSlotNote>0) )
			{
				NotesActual--;
			}
			DBUG(("NotesActual = %d\n", NotesActual ));
		}
		else
		{
			Channel = gChannelSlots[SlotIndex];
			Velocity = gVelocitySlots[SlotIndex];
			Note = gNoteSlots[SlotIndex];
			Pan = gPanSlots[SlotIndex];
		}


		if( !IfFreezeSleeps && (Choose(kMaxProb) < kChangeSleepProb) )
		{
			IfSleep = ( Choose(kMaxProb) < kSleepProb );
		}
		else
		{
			IfSleep = gSleepSlots[SlotIndex];
		}

/* Play the note. */
		if( Note )
		{
                	Result = ChangeScoreControl( scon, Channel, MIDI_PAN_CONTROLLER, Pan );
			CHECKRESULT(Result, "StartScoreNote");

DBUG(("Note %d at %d on %d\n", Note, Velocity, Channel ));
			Result = StartScoreNote( scon, Channel, Note, Velocity );
			CHECKRESULT(Result, "StartScoreNote");
		}
/* Save back into Slot for repeated play. */
		gChannelSlots[SlotIndex] = Channel;
		gVelocitySlots[SlotIndex] = Velocity;
		gNoteSlots[SlotIndex] = Note;
		gPanSlots[SlotIndex] = Pan;
		gSleepSlots[SlotIndex] = IfSleep;


/* Advance to the next slot. */
		SlotIndex++;
		if( ++SlotIndex >= kNumSlots )
		{
			SlotIndex = 0;
			BeatCount = 0;
		}

/* Advance clock by sleeping if desired.
** Otherwise play more notes at this time.
*/
		if( IfSleep )
		{
/* Measure how long it's been since the last sleep. */
			ThisTime = GetAudioTime();
			ElapsedTime = ThisTime - LastTime;
			if( ElapsedTime > MaxTime )
			{
				MaxTime = ElapsedTime;
				PRT(("MaxTime = %d ticks\n", MaxTime));
			}

/* Get User input. */
			Result = GetControlPad (1, FALSE, &cped);
			if (Result < 0) {
				PrintError(0,"read control pad in","PlaySoundFile",Result);
			}
/* Edge detect for new buttons. */
			Buttons = cped.cped_ButtonBits;
			NewButtons = Buttons & ~PreviousButtons;
			PreviousButtons = Buttons;

			switch( NewButtons )
			{
			case ControlX:
				DoIt = FALSE;
				break;

			case ControlA:
				if( CurNotesDesired == kNotesDesired)
				{
					CurNotesDesired = 2*kNotesDesired;
				}
				else
				{
					CurNotesDesired = kNotesDesired;
				}
				PRT(("CurNotesDesired = %d\n", CurNotesDesired ));
				break;

			case ControlB:
				if( CurTicksPerBeat == kTicksPerBeat)
				{
					CurTicksPerBeat = kTicksPerBeat/2;
				}
				else
				{
					CurTicksPerBeat = kTicksPerBeat;
				}
				PRT(("CurTicksPerBeat = %d\n", CurTicksPerBeat ));
				break;

			case ControlC:
				IfFreezeNotes = !IfFreezeNotes;
				PRT(("IfFreezeNotes = %d\n", IfFreezeNotes ));
				break;

			case ControlRightShift:
				IfFreezeSleeps = !IfFreezeSleeps;
				PRT(("IfFreezeSleeps = %d\n", IfFreezeSleeps ));
				break;

			case ControlLeftShift:
				DumpNoteArrays();
				break;
			}

/* Sleep for a random time. */
			NextTime = NextTime + CurTicksPerBeat;
			SleepUntilTime( MyCue, NextTime );
			BeatCount++;

			LastTime = GetAudioTime();
		}
	}

cleanup:
	DeleteItem( MyCue );
	UnloadPIMap( scon );
	DeleteScoreMixer( scon );
	DeleteScoreContext( scon );
	return Result;
}
