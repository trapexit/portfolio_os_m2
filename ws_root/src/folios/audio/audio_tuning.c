/* @(#) audio_tuning.c 96/08/09 1.33 */
/* $Id: audio_tuning.c,v 1.22 1995/03/23 18:58:41 phil Exp phil $ */
/****************************************************************
**
** Audio Folio support for Tunings
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930828 PLB Fixed item check in TuneInstrument
** 930830 PLB Check for negative counts in tuning
** 931129 PLB Added Convert12TET_F16
** 940406 PLB Support TuneInsTemplate()
** 940912 PLB Use Read16 to read uint16 values for NuPup
** 950323 PLB Fixed bug in PitchToFrequency. Was overindexing tuning array
**            whenever (Pitch == (NumNotes*j + BaseNote)) for j = 2,3,4...
**            The above bug is in Clarify as CR4510.
** 950706 PLB Converted tunings to float32.
** 950828 PLB Fixed OctaveShift for intervals more than an octave down.
** 950927 WJB Added support for AF_NODE_F_AUTO_FREE_DATA.
**            Added commented out support for AF_TAG_AUTO_FREE_DATA.
**            Removed DeleteTuning().
**            Now using NextTagArg().
**            Updated autodocs a bit.
** 950928 WJB Rewrote internalCreateAudioTemplate() and internalSetAudioTemplate()
**            around common TagProcessor() callback.
**            Enabled support for AF_TAG_AUTO_FREE_DATA.
**            Revised Template autodoc.
** 951109 WJB Changed AF_TAG_AUTO_FREE_DATA to use MEMTYPE_TRACKSIZE.
** 960220 PLB Use Taylor expansion to approximate 2**X.
****************************************************************/

/****************************************************************
How Tuning Works

Tuning tables are specified in float32 frequencies. The base of the table is
associated with a specific pitch (note index). When an instrument is played
at a specific pitch, it is converted to frequency via table lookup. If the
pitch is outside the table, it will extrapolate the frequency based on
octave replication. For tunings that do not replicate by octave, the entire
range must be specified. The number of notes per octave can also be
specified to balance memory usage and speed of lookup.

The frequency is then passed to the frequency knob which converts the freq
to device specific parameters. For samples, the ratio of the desired
frequency to the frequency of the sample is passed to the DSP.

****************************************************************/

#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)  /* PRT(x) */

#define SEMITONES_PER_OCTAVE  (12)


/*****************************************************************/
/****** Statically allocated default tuning table. ***************/
/*****************************************************************/

static const float32 DefaultTuningTable[SEMITONES_PER_OCTAVE] =
{
	440.000000, /* A 440 */
	466.163762,
	493.883301,
	523.251131,
	554.365262,
	587.329536,
	622.253967,
	659.255114,
	698.456463,
	739.988845,
	783.990872,
	830.609395
};

const AudioTuning DefaultTuning = {     /* @@@ depends on AudioTuning field order */
    { 0 },
    DefaultTuningTable,             /* atun_Frequencies */
    SEMITONES_PER_OCTAVE,           /* atun_NumNotes */
    AF_A440_PITCH,                  /* atun_BaseNote */
    SEMITONES_PER_OCTAVE            /* atun_NotesPerOctave */
};

/*****************************************************************/
/***** USER Level Folio Calls ************************************/
/*****************************************************************/
 /**
 |||	AUTODOC -public -class Items -group Audio -name Tuning
 |||	An item that contains information for tuning an Instrument(@).
 |||
 |||	  Description
 |||
 |||	    A tuning item contains information for converting MIDI pitch numbers to
 |||	    frequency for an Instrument(@) or Template(@). The information includes an
 |||	    array of frequencies, and 32-bit integer values indicating the number of
 |||	    pitches, notes in an octave, and the lowest pitch for the tuning
 |||	    frequency. See the CreateTuning() call in the "Audio Folio Calls" chapter
 |||	    in this manual for more information.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_TUNING_NODE
 |||
 |||	  Create
 |||
 |||	    CreateItem(), CreateTuning()
 |||
 |||	  Delete
 |||
 |||	    DeleteItem(), DeleteTuning()
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    TuneInsTemplate(), TuneInstrument()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_ADDRESS (const float32 *) - Create, Modify*
 |||	        Array of frequencies in float32 Hz to be used as a lookup table. The
 |||	        length of the array is specified with AF_TAG_FREAMES. This data must
 |||	        remain valid for the life of the Tuning or until a different address is
 |||	        set with AF_TAG_ADDRESS.
 |||
 |||	        If the Tuning Item is created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then
 |||	        this data will be freed automatically when the Tuning is deleted. Memory
 |||	        used with AF_TAG_AUTO_FREE_DATA must be allocated with MEMTYPE_TRACKSIZE
 |||	        set.
 |||
 |||	    AF_TAG_AUTO_FREE_DATA (bool) - Create
 |||	        Set to TRUE to cause data pointed to by AF_TAG_ADDRESS to be freed
 |||	        automatically when Tuning Item is deleted. If the Item isn't
 |||	        successfully created, the memory isn't freed.
 |||
 |||	        The memory pointed to by AF_TAG_ADDRESS must be freeable by
 |||	        FreeMem (Address, TRACKED_SIZE).
 |||
 |||	    AF_TAG_BASENOTE (uint8) - Create, Modify
 |||	        MIDI note number that should be given the first frequency in the
 |||	        frequency array in the range of 0..127.
 |||
 |||	    AF_TAG_FRAMES (int32) - Create, Modify*
 |||	        Number of frequencies in array pointed to by AF_TAG_ADDRESS. This value
 |||	        must be >= NotesPerOctave.
 |||
 |||	    AF_TAG_NOTESPEROCTAVE (int32) - Create, Modify
 |||	        Number of notes per octave. This is used to determine where in the
 |||	        frequency array to look for notes that fall outside the range of
 |||	        BaseNote..BaseNote+Frames-1. This value must be <= Frames.
 |||
 |||	    * These tags cannot be used to modify a Tuning created with
 |||	    { AF_TAG_AUTO_FREE_DATA, TRUE }.
 |||
 |||	  See Also
 |||
 |||	    Instrument(@), Template(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Tuning -name Convert12TET_FP
 |||	Converts a pitch bend value in semitones and cents into a float value.
 |||
 |||	  Synopsis
 |||
 |||	    Err Convert12TET_FP (int32 semitones, int32 cents, float32 *fractionPtr)
 |||
 |||	  Description
 |||
 |||	    This procedure, whose name reads (convert 12-tone equal-tempered to float)
 |||	    converts a pitch bend value expressed in semitones and cents to a float bend
 |||	    value. The bend value is used with BendInstrumentPitch() to bend the
 |||	    instrument's pitch up or down: the value multiplies the frequency of the
 |||	    instrument's output to create a resultant pitch bent up or down by the
 |||	    specified amount.
 |||
 |||	    Note that the semitones and cents values need not both be positive or both
 |||	    be negative. One value can be positive while the other value is negative.
 |||	    For example, -5 semitones and +30 cents bends pitch down by 4 semitones and
 |||	    70 cents.
 |||
 |||	    Convert12TET_FP() stores the resultant bend value in fractionPtr.
 |||
 |||	  Arguments
 |||
 |||	    semitones
 |||	        The integer number of semitones to bend up or down (can be negative
 |||	        or positive).
 |||
 |||	    cents
 |||	        The integer number of cents (from -100 to 100) to bend up or down.
 |||
 |||	    fractionPtr
 |||	        Pointer to a float32 variable where the bend value will be stored.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    BendInstrumentPitch()
 **/
Err Convert12TET_FP( int32 Semitones, int32 Cents, float32 *FractionPtr )
{
	float32 exp;

/* Calculate exponent of 2. */
	exp = (Semitones/12.0) + (Cents/1200.0);

	if(( exp < -10.0 ) || ( exp > 10.0 ))
	{
		ERR(("Convert12TET_FP: exp = %g\n", exp));
		return AF_ERR_OUTOFRANGE;
	}

	*FractionPtr = Approximate2toX(exp);

DBUG(("Convert12TET_FP: Fraction = %g\n", *FractionPtr));

	return 0;
}

/********************************************************************
** Use taylor expansion to approximate 2**X
*/
float32  Approximate2toX ( float32 X )
{
	int32 endShift = 0;
	float32  accume, w;

/*
** Normalize X to range of -1.0 to 1.0
** because Taylor expansion becomes very inaccurate
** outside that range.
*/
	if( X > 0.0 )
	{
		while( X > 1.0 )
		{
			X -= 1.0;
			endShift++;
		}
	}
	else
	{
		while( X < -1.0 )
		{
			X += 1.0;
			endShift--;
		}
	}

/*
** Perform Taylor expansion
** w = X * ln(2.0)
** 2**X = 1 + w + w*w/2 + w*w*w/6 + w*w*w*w/24
**      = 1 + w*(1 + w/2 + w*w/6 + w*w*w/24)
**      = 1 + w*(1 + w(1/2 + w/6 + w*w/24))
**      = 1 + w*(1 + w*(1/2 + w*(1/6 + w/24)))
*/
#define LOG_OF_2   (0.693147181)
	w = X * LOG_OF_2;
	accume = ( 1.0 / (float32)(2*3*4*5*6) );
	accume = w * accume + ( 1.0 / (float32)(2*3*4*5) );
	accume = w * accume + ( 1.0 / (float32)(2*3*4) );
	accume = w * accume + ( 1.0 / (float32)(2*3) );
	accume = w * accume + ( 1.0 / (float32)(2) );
	accume = w * accume + 1.0;
	accume = w * accume + 1.0;

/* Restore to original range. */
	if( endShift > 0 )
	{
		accume *= (float32) (1<<endShift);
	}
	else if( endShift < 0 )
	{
		accume /= (float32) (1<<(-endShift));
	}

	return accume;
}

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Tuning -name CreateTuning
 |||	Creates a Tuning(@) Item.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateTuning (const float32 *frequencies, int32 numNotes,
 |||	                       int32 notesPerOctave, int32 baseNote)
 |||
 |||	  Description
 |||
 |||	    This procedure creates a tuning item that can be used to tune an
 |||	    instrument.
 |||
 |||	    NotesPerOctave is set to 12 for a standard western tuning system. For
 |||	    every octave's number of notes up or down, the frequency will be
 |||	    doubled or halved. You can thus specify 12 notes of a 12-tone scale, then
 |||	    extrapolate up or down for the other octaves. You should specify the
 |||	    entire range of pitches if the tuning does not repeat itself every octave.
 |||	    For more information, see the Music Programmer's Guide.
 |||
 |||	    When you are finished with the tuning item, you should call DeleteTuning()
 |||	    to deallocate the resources.
 |||
 |||	  Arguments
 |||
 |||	    frequencies
 |||	        A pointer to an array of float32 frequencies, which lists a tuning
 |||	        pattern.
 |||
 |||	    numNotes
 |||	        The number of frequencies in the array. This value
 |||	        must be >= notesPerOctave.
 |||
 |||	    notesPerOctave
 |||	        The number of notes in an octave (12 in a standard tuning scale).
 |||
 |||	    baseNote
 |||	        The MIDI pitch (note) value of the first frequency in the array. If your
 |||	        array starts with the frequency of middle C, its MIDI pitch value would
 |||	        equal 60.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number for the tuning or an error code (a
 |||	    negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in libc.a V29.
 |||
 |||	  Notes
 |||
 |||	    This function is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TUNING_NODE),
 |||	            AF_TAG_ADDRESS,        frequencies,
 |||	            AF_TAG_FRAMES,         numIntervals,
 |||	            AF_TAG_NOTESPEROCTAVE, notesPerOctave,
 |||	            AF_TAG_BASENOTE,       baseNote,
 |||	            TAG_END);
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    Tuning(@), DeleteTuning(), TuneInsTemplate(), TuneInstrument()
 **/

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Tuning -name DeleteTuning
 |||	Deletes a Tuning(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteTuning (Item tuning)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the specified Tuning(@) and frees any resources
 |||	    dedicated to it. Note that if you delete a tuning that's in use for
 |||	    an existing instrument, that instrument reverts to its default tuning.
 |||
 |||	    If the Tuning was created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then the
 |||	    tuning data is also be freed when the Tuning Item is deleted. Otherwise, the
 |||	    tuning data is not freed automatically.
 |||
 |||	  Arguments
 |||
 |||	    tuning
 |||	        The item number of the Tuning to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V27.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    Tuning(@), CreateTuning()
 **/


/*****************************************************************/
/***** Create/Modify/Delete Tuning Item **************************/
/*****************************************************************/

static Err CreateTuningTagHandler (AudioTuning *, void *dummy, uint32 tag, TagData arg);
static Err TuningTagHandler (AudioTuning *, uint32 tag, TagData arg);
static Err ValidateAudioTuning (const AudioTuning *);

/*****************************************************************/
/* AUDIO_TUNING_NODE ir_Create method */
Item internalCreateAudioTuning (AudioTuning *atun, const TagArg *tagList)
{
    Err errcode;

        /* process tags and validate results */
        /* (this doesn't allocate anything, so no need to clean anything up) */
    if (errcode = TagProcessor (atun, tagList, CreateTuningTagHandler, NULL)) return errcode;
    if (errcode = ValidateAudioTuning (atun)) return errcode;

    DBUG(("internalCreateAudioTuning: item=0x%x data=0x%08x numnotes=%d notes/octave=%d basenote=%d\n",
        atun->atun_Item.n_Item,
        atun->atun_Frequencies, atun->atun_NumNotes, atun->atun_NotesPerOctave, atun->atun_BaseNote));

        /* return Item as success */
    return atun->atun_Item.n_Item;
}

/*****************************************************************/
/* AUDIO_TUNING_NODE SetAudioItemOwner() method */
Err internalSetTuningInfo (AudioTuning *atun, const TagArg *tagList)
{
    AudioTuning tempatun = *atun;
    Err errcode;

        /* process tags and validate results */
        /* (this doesn't allocate anything, so no need to clean anything up) */
        /* @@@ this must prevent any changes to tempatun that are invalid in atun
           - does blind structure copy to apply changes */
    {
        const TagArg *tag;
        int32 tagResult;

        for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
            DBUG(("internalSetTuningInfo: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));

            switch (tag->ta_Tag) {
                case AF_TAG_ADDRESS:
                case AF_TAG_FRAMES:
                    if (tempatun.atun_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
                        ERR(("internalSetTuningInfo: Created with { AF_TAG_AUTO_FREE_DATA, TRUE }; can't change data address or size\n"));
                        return AF_ERR_BADTAG;
                    }
                    /* fall thru to default */

                default:
                    if ((errcode = TuningTagHandler (&tempatun, tag->ta_Tag, tag->ta_Arg)) < 0) return errcode;
                    break;
            }
        }

            /* Catch tag processing errors */
        if ((errcode = tagResult) < 0) {
            ERR(("internalSetTuningInfo: Error processing tag list 0x%x\n", tagList));
            return errcode;
        }
    }
    if (errcode = ValidateAudioTuning (&tempatun)) return errcode;

        /* apply results */
    *atun = tempatun;

    DBUG(("internalSetTuningInfo: item=0x%x data=0x%08x numnotes=%d notes/octave=%d basenote=%d\n",
        atun->atun_Item.n_Item,
        atun->atun_Frequencies, atun->atun_NumNotes, atun->atun_NotesPerOctave, atun->atun_BaseNote));

    return 0;
}

/*
    TagProcessor() callback function for internalCreateAudioTuning()

    Arguments
        atun
            AudioTuning Item to fill out. Client must initialize it before
            calling TagProcessor().
*/
static Err CreateTuningTagHandler (AudioTuning *atun, void *dummy, uint32 tag, TagData arg)
{
    TOUCH(dummy);

    DBUG(("CreateTuningTagHandler: tag { %d, 0x%x }\n", tag, arg));

    switch (tag) {
        case AF_TAG_AUTO_FREE_DATA:
            if ((int32)arg) atun->atun_Item.n_Flags |= AF_NODE_F_AUTO_FREE_DATA;
            else            atun->atun_Item.n_Flags &= ~AF_NODE_F_AUTO_FREE_DATA;
            break;

        default:
            return TuningTagHandler (atun, tag, arg);
    }

    return 0;
}

/*
    Common tag processing for CreateTuningTagHandler() and internalSetTuningInfo()

    Arguments
        atun
            AudioTuning Item to fill out.
*/
static Err TuningTagHandler (AudioTuning *atun, uint32 tag, TagData arg)
{
    switch (tag) {
        case AF_TAG_ADDRESS:
                /* address is validated by ValidateAudioTuning() */
            atun->atun_Frequencies = (float32 *)arg;
            break;

        case AF_TAG_FRAMES:
                /* validated in ValidateAudioTuning() */
            atun->atun_NumNotes = (int32)arg;
            break;

        case AF_TAG_BASENOTE:
            {
                const int32 baseNote = (int32)arg;

              #ifdef BUILD_PARANOIA
                if (baseNote < 0 || baseNote > 127) {
                    ERR(("TuningTagHandler: Base note (%d) out of range\n", baseNote));
                    return AF_ERR_OUTOFRANGE;
                }
              #endif
                atun->atun_BaseNote = baseNote;
            }
            break;

        case AF_TAG_NOTESPEROCTAVE:
                /* validated in ValidateAudioTuning() */
            atun->atun_NotesPerOctave = (int32)arg;
            break;

        default:
            ERR(("TuningTagHandler: Unrecognized tag { %d, 0x%x }\n", tag, arg));
            return AF_ERR_BADTAG;
    }

    return 0;
}

/*
    Validate contents of an AudioTuning being considered for either creating
    as an Item or posting to an existing Item.

    Validates:
        . atun_Frequencies, atun_NumFrequences - points to real memory
        . if AF_NODE_F_AUTO_FREE_DATA is set, verifies that calling task
          actually owns this memory.
        . enforces relationship between atun_NumNotes and atun_NotesPerOctave.

    Arguments
        atun
*/
static Err ValidateAudioTuning (const AudioTuning *atun)
{
        /* Make sure atun_NotesPerOctave >= 1 */
    if (atun->atun_NotesPerOctave < 1) {
        ERR(("ValidateAudioTuning: Notes/Octave (%d) must be >= 1\n", atun->atun_NotesPerOctave));
        return AF_ERR_BADTAGVAL;
    }

        /* Make sure atun_NumNotes >= atun_NotesPerOctave */
    if (atun->atun_NumNotes < atun->atun_NotesPerOctave) {
        ERR(("ValidateAudioTuning: NumNotes (%d) must be >= Notes/Octave (%d)\n", atun->atun_NumNotes, atun->atun_NotesPerOctave));
        return AF_ERR_BADTAGVAL;
    }

        /* validate data */
    return ValidateAudioItemData (atun->atun_Frequencies, atun->atun_NumNotes * sizeof(float32), (atun->atun_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) != 0);
}


/**************************************************************/
/* AUDIO_TUNING_NODE ir_Delete method */
int32 internalDeleteAudioTuning (AudioTuning *atun, Task *ct)
{
        /* delete AF_NODE_F_AUTO_FREE_DATA data */
    if (atun->atun_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
        DBUG(("internalDeleteAudioTuning: Deleting AF_NODE_F_AUTO_FREE_DATA @ 0x%08x, 0x%x bytes\n", atun->atun_Frequencies, GetMemTrackSize(atun->atun_Frequencies)));
        SuperFreeUserMem (atun->atun_Frequencies, TRACKED_SIZE, ct);
    }

    return 0;
}


/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Tuning -name TuneInsTemplate
 |||	Applies the specified Tuning(@) to an instrument Template(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err TuneInsTemplate (Item insTemplate, Item tuning)
 |||
 |||	  Description
 |||
 |||	    This procedure applies the specified tuning to the specified instrument
 |||	    template. When an instrument is created using the template, the tuning is
 |||	    applied to the new instrument. When notes are played on the instrument,
 |||	    they play using the specified tuning system.
 |||
 |||	  Arguments
 |||
 |||	    insTemplate
 |||	        The item number of the template.
 |||
 |||	    tuning
 |||	        The item number of the tuning.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    CreateTuning(), TuneInstrument()
 **/
int32 swiTuneInsTemplate( Item InsTemplate, Item Tuning )
{
	AudioTuning *atun;
	AudioInsTemplate *aitp;

	aitp = (AudioInsTemplate *) CheckItem( InsTemplate,
			AUDIONODE, AUDIO_TEMPLATE_NODE);
	if( aitp == NULL )
	{
		return AF_ERR_BADITEM;
	}

	aitp->aitp_Tuning = 0;
	if(Tuning)
	{
		atun = (AudioTuning *) CheckItem( Tuning,  /* Fix item check. 930828 */
				AUDIONODE, AUDIO_TUNING_NODE);
		if( atun == NULL )
		{
			return AF_ERR_BADITEM;
		}
		else
		{
			aitp->aitp_Tuning = Tuning;
		}
	}
	return 0;

}

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Tuning -name TuneInstrument
 |||	Applies the specified Tuning(@) to an Instrument(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err TuneInstrument (Item instrument, Item tuning)
 |||
 |||	  Description
 |||
 |||	    This procedure applies the specified Tuning to an instrument so that notes
 |||	    played on the instrument use that tuning. If no tuning is specified, the
 |||	    default 12-tone equal-tempered tuning is used.
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        The item number of the instrument.
 |||
 |||	    tuning
 |||	        The item number of the tuning.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    CreateTuning(), TuneInsTemplate()
 **/
int32 swiTuneInstrument( Item Instrument, Item Tuning )
{
	AudioTuning *atun;
	AudioInstrument *ains;

	ains = (AudioInstrument *) CheckItem( Instrument,
			AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if( ains == NULL )
	{
		return AF_ERR_BADITEM;
	}

	ains->ains_Tuning = 0;
	if(Tuning)
	{
		atun = (AudioTuning *) CheckItem( Tuning,  /* Fix item check. 930828 */
				AUDIONODE, AUDIO_TUNING_NODE);
		if( atun == NULL )
		{
			return AF_ERR_BADITEM;
		}
		else
		{
			ains->ains_Tuning = Tuning;
		}
	}
	return 0;
}


/*****************************************************************/
/***** Internal Tuning Calculations ******************************/
/*****************************************************************/

/*****************************************************************/
const AudioTuning *GetInsTuning (const AudioInstrument *ains)
{
	const AudioTuning *atun;

		/* Use specific or default tuning. */
	if (!ains->ains_Tuning || !(atun = (AudioTuning *) CheckItem( ains->ains_Tuning, AUDIONODE, AUDIO_TUNING_NODE))) {
		atun = &DefaultTuning;
	}

	return atun;
}


/*****************************************************************/
Err PitchToFrequency (const AudioTuning *atun, int32 Pitch, float32 *FrequencyPtr)
{
	int32 Index;
	int32 OctaveShift;
	float32  Freq;

/* Look up scalar and shift by octaves if needed. */
	Index = Pitch - atun->atun_BaseNote;
	OctaveShift = 0;
	if( Index >= (int32) atun->atun_NumNotes )
	{
		do
		{
			OctaveShift++;
			Index -= atun->atun_NotesPerOctave;
		}while( Index >= (int32) atun->atun_NumNotes);
		Freq = atun->atun_Frequencies[Index] * ( 1 << OctaveShift );
	}
	else if ( Index < 0 )
	{
		do
		{
			OctaveShift++;
			Index += atun->atun_NotesPerOctave;
		}while( Index < 0);
		Freq = atun->atun_Frequencies[Index] / ( 1 << OctaveShift );
	}
	else
	{
		Freq = atun->atun_Frequencies[Index];
	}

	*FrequencyPtr = Freq;

DBUG(("PitchToFrequency: Index = %d, OctaveShift = %d, *FrequencyPtr = %g, \n", Index, OctaveShift, *FrequencyPtr));

	return 0;
}
