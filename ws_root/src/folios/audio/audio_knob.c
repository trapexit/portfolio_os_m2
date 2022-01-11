/* @(#) audio_knob.c 96/08/09 1.35 */
/* $Id: audio_knob.c,v 1.33 1995/02/02 18:51:32 peabody Exp phil $ */
/***************************************************************
**
** Audio Knobs
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************
** 930816 PLB Removed call to CHECKAUDIOOPEN
** 930828 PLB Return proper error code for Bad Item in internalCreateAudioKnob,
**            TweakKnob and TweakRawKnob.
** 930830 PLB Started to implement AF_TAG_CURRENT for knobs.
** 931222 PLB Fully implemented AF_TAG_CURRENT for knobs.
**            Optimised by calling DSPPPutKnob() directly.
** 941024 PLB Removed GetKnob() which was dead code.
** 950628 PLB Converted to M2 floating point knob style.
***************************************************************/

#include "audio_internal.h"
#include <dspptouch/dspp_touch.h>   /* dsph...() */

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */

/******************************************************************/
/***** USER MODE **************************************************/
/******************************************************************/

 /**
 |||	AUTODOC -public -class Items -group Audio -name Knob
 |||	An item for adjusting an Instrument(@)'s parameters.
 |||
 |||	  Description
 |||
 |||	    A Knob is an Item for adjusting an Instrument's parameters. Each knob has
 |||	    one or more parts (as defined by the instrument), all of which can be
 |||	    addressed from its knob item. Each knob has a signal type which defines the
 |||	    units of the knob's value. Knob have a default signal type as defined by the
 |||	    instrument, but this can be overridden by using AF_TAG_TYPE when creating
 |||	    the knob. Knobs have a default value per part, which can be read using
 |||	    ReadKnobPart() before that part has been set with SetKnobPart().
 |||
 |||	    Multiple knob items for the same instrument knob can coexist without
 |||	    conflict, even if they have different signal types. The last value set by
 |||	    SetKnobPart() takes precedence.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_KNOB_NODE
 |||
 |||	  Create
 |||
 |||	    CreateKnob()
 |||
 |||	  Delete
 |||
 |||	    DeleteKnob()
 |||
 |||	  Query
 |||
 |||	    GetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    SetKnobPart(), ReadKnobPart()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_INSTRUMENT (Item) - Create
 |||	        Specifies instrument containing knob to create.
 |||
 |||	    AF_TAG_MAX_FP (float32) - Query
 |||	        Returns maximum value of knob in the units appropriate for the signal
 |||	        type specified by AF_TAG_TYPE, and taking into consideration
 |||	        execution rate.
 |||
 |||	    AF_TAG_MIN_FP (float32) - Query
 |||	        Returns minimum value of knob in the units appropriate for the signal
 |||	        type specified by AF_TAG_TYPE, and taking into consideration
 |||	        execution rate.
 |||
 |||	    AF_TAG_NAME (const char *) - Create, Query
 |||	        Knob name. On creation, specifies name of knob belonging to the
 |||	        instrument to create. On query, returns a pointer to knob's name.
 |||
 |||	    AF_TAG_TYPE (uint8) - Create
 |||	        Determines the signal type of the knob. Must be one of the
 |||	        AF_SIGNAL_TYPE_* defined in <audio/audio.h>. Defaults to the default
 |||	        signal type of the knob (for standard DSP instruments, this is described
 |||	        in the instrument documentation).
 |||
 |||	  See Also
 |||
 |||	    Instrument(@), Probe(@), GetInstrumentPortInfoByName(),
 |||	    GetAudioSignalInfo()
 **/

 /**
 |||	AUTODOC -public -class audio -group Knob -name CreateKnob
 |||	Gain direct access to one of an Instrument(@)'s Knob(@)s.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateKnob (Item instrument, const char *name, const TagArg *tagList)
 |||
 |||	    Item CreateKnobVA (Item instrument, const char *name, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure creates a Knob item that provides a fast connection between
 |||	    a task and one of an Instrument's parameters. You can then call
 |||	    SetKnobPart() to rapidly modify that parameter.
 |||
 |||	    See the Instrument Template pages for complete listings of each Instrument
 |||	    Templates knobs.
 |||
 |||	    Call DeleteKnob() to relinquish access to this knob. All Knob Items
 |||	    grabbed for an Instrument are deleted when the Instrument is deleted.
 |||	    This can save you a bunch of calls to DeleteKnob().
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        The item number of the Instrument.
 |||
 |||	    name
 |||	        The name of the knob to grab. The knob name is matched
 |||	        case-insensitively.
 |||
 |||	  Tag Arguments
 |||
 |||	    AF_TAG_TYPE (uint8)
 |||	        Determines the signal type of the knob. Must be one of the
 |||	        AF_SIGNAL_TYPE_* defined in <audio/audio.h>. Defaults to the default
 |||	        signal type of the knob (for standard DSP instruments, this is described
 |||	        in the instrument documentation).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the Knob (a positive value) if
 |||	    successful or an error code (a negative value) if an error occurs.
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
 |||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_KNOB_NODE),
 |||	                      AF_TAG_INSTRUMENT, instrument,
 |||	                      AF_TAG_NAME, name,
 |||	                      TAG_JUMP, tagList);
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    DeleteKnob(), SetKnobPart(), Knob(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Knob -name DeleteKnob
 |||	Deletes a Knob(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteKnob (Item knob)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the Knob Item.
 |||
 |||	  Arguments
 |||
 |||	    knob
 |||	        Item number of knob to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h>.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateKnob()
 **/

/******************************************************************/
Err internalGetKnobInfo (const AudioKnob *aknob, TagArg *tagList)
{
	const int32 rateShift = ((DSPPInstrument *) aknob->aknob_DeviceInstrument)->dins_RateShift;
	TagArg *tag;
	int32 tagResult;

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUG(("internalGetKnobInfo: %d\n", tag->ta_Tag));

		switch (tag->ta_Tag) {
			case AF_TAG_MIN_FP:
				tag->ta_Arg = (TagData) ConvertFP_TagData (dsppGetSignalMin(aknob->aknob_Type, rateShift));
				break;

			case AF_TAG_MAX_FP:
				tag->ta_Arg = (TagData) ConvertFP_TagData (dsppGetSignalMax(aknob->aknob_Type, rateShift));
				break;

			case AF_TAG_TYPE:
				tag->ta_Arg = (TagData) aknob->aknob_Type;
				break;

			case AF_TAG_NAME:
				{
					const DSPPInstrument * const dins = aknob->aknob_DeviceInstrument;

					tag->ta_Arg = (TagData) dsppGetTemplateRsrcName (dins->dins_Template, aknob->aknob_RsrcIndex);
				}
				break;

			default:
				ERR (("internalGetKnobInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if (tagResult < 0) {
		ERR(("internalGetKnobInfo: Error processing tag list 0x%x\n", tagList));
		return tagResult;
	}

	return 0;
}

 /**
 |||	AUTODOC -public -class audio -group Knob -name ReadKnob
 |||	Reads the current value of a single-part Knob(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadKnob (Item knob, float32 *valuePtr)
 |||
 |||	  Description
 |||
 |||	    This procedure reads the current value of a single-part knob.
 |||
 |||	  Arguments
 |||
 |||	    knob
 |||	        Item number of the knob to be read. Always reads part 0 of the knob.
 |||
 |||	    valuePtr
 |||	        Pointer to a variable to receive the value for that knob. Task must have
 |||	        write permission for that address. The result is in the units that apply
 |||	        to the AF_SIGNAL_TYPE_ of the Knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience macro implemented in <audio/audio.h> V27.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        ReadKnobPart (knob, 0, valuePtr)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SetKnobPart(), CreateKnob(), ReadKnobPart(), Knob(@)
 **/
 /**
 |||	AUTODOC -public -class audio -group Knob -name ReadKnobPart
 |||	Reads the current value of a Knob(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadKnobPart (Item knob, int32 partNum, float32 *valuePtr)
 |||
 |||	  Description
 |||
 |||	    This procedure reads the current value of a knob. This is either the last
 |||	    value set by SetKnobPart(), or the default knob value if SetKnobPart()
 |||	    hasn't been called for this knob yet.
 |||
 |||	    In the case of a knob which is connected to another instrument (via
 |||	    ConnectInstrumentParts()), this function still returns the last set value,
 |||	    not the signal being injected into this knob via the connection. To read the
 |||	    signal value use a Probe(@) on the source instrument's output.
 |||
 |||	  Arguments
 |||
 |||	    knob
 |||	        Item number of the knob to be read.
 |||
 |||	    partNum
 |||	        Index of the part of the knob to be read.
 |||
 |||	    valuePtr
 |||	        Pointer to a variable to receive the value for that knob. Task must have
 |||	        write permission for that address. The result is in the units that apply
 |||	        to the AF_SIGNAL_TYPE_ of the Knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V27.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    SetKnobPart(), CreateKnob(), ReadKnob(), Knob(@)
 **/

Err internalReadKnobOrProbePart ( AudioKnob *aknob, int32 partNum, float32 *valuePtr )
{
	DSPPResource *drsc;
	int32 curRaw;

	drsc = dsppKnobToResource(aknob);

/* Validate partNum */
	if( (partNum < 0) || (partNum >= drsc->drsc_Many) )
	{
DBUG(("internalReadKnobOrProbePart: bad partNum = %d\n", partNum));
		return AF_ERR_OUTOFRANGE;
	}

/* Validate valuePtr */
	if( !IsMemWritable(valuePtr,sizeof(float32)) )
	{
		return AF_ERR_SECURITY;
	}

	curRaw = dsphReadDataMem( drsc->drsc_Allocated + partNum );
	*valuePtr = dsppConvertRawToSignal( aknob->aknob_Type,
		((DSPPInstrument *) aknob->aknob_DeviceInstrument)->dins_RateShift, curRaw );

	return 0;
}

/* -ReadKnobPart */
Err swiReadKnobPart ( Item knobItem, int32 partNum, float32 *valuePtr )
{
	AudioKnob *aknob;
DBUG(("ReadKnobPart(0x%x, %d, 0x%x)\n", knobItem, PartNum, valuePtr));

	aknob = (AudioKnob *)CheckItem(knobItem, AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return AF_ERR_BADITEM;

	return internalReadKnobOrProbePart( aknob, partNum, valuePtr );
}

/******************************************************************/
/******** Folio Creation of Knobs *********************************/
/******************************************************************/

Item internalCreateAudioKnob (AudioKnob *aknob, TagArg *args)
{
	return internalCreateAudioKnobOrProbe( aknob, DRSC_TYPE_KNOB, args );
}

Item internalCreateAudioKnobOrProbe (AudioKnob *aknob, int32 RsrcType, TagArg *args)
{
	AudioInstrument *ains = NULL;
  	char *Name = NULL;
  	int32 result;
	const TagArg *tstate;
	TagArg *t;
	int32 Result;
	uint32 customType = 0;
	int32 ifTypeSet = FALSE;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioKnob(0x%x, 0x%lx)\n", aknob, args));

    Result = TagProcessor( aknob, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioKnob: TagProcessor failed.\n"));
    	return Result;
    }

	if (args)
	{

		for (tstate = args;
			 (t = NextTagArg (&tstate)) != NULL; )
		{
DBUG(("internalCreateAudioKnob: Tag = %d, Arg = 0x%x\n", t->ta_Tag, t->ta_Arg));

			switch (t->ta_Tag)
			{
			case AF_TAG_INSTRUMENT:
				ains = (AudioInstrument *)CheckItem((Item) t->ta_Arg, AUDIONODE, AUDIO_INSTRUMENT_NODE);
				break;

			case AF_TAG_TYPE:
				customType = (uint32) t->ta_Arg;
				ifTypeSet = TRUE;
				if( customType > AF_SIGNAL_TYPE_MAX )
				{
					ERR(("CreateKnob: illegal type = 0x%x\n", customType ));
					return AF_ERR_BAD_SIGNAL_TYPE;
				}
				break;

			case AF_TAG_NAME:
				Name = (char *) t->ta_Arg; /* Validate length. */
				Result = afi_IsRamAddr( Name, 1);
				if(Result < 0) return Result;
				break;

			default:
				if(t->ta_Tag > TAG_ITEM_LAST)
				{
					ERR(("internalCreateAudioKnobOrProbe: Unrecognized tag { %d, 0x%x }\n", t->ta_Tag, t->ta_Arg ));
					return AF_ERR_BADTAG;
				}
			}
		}
	}

	if (Name == NULL) return AF_ERR_NAME_NOT_FOUND;
	if (ains == NULL) return AF_ERR_BADITEM; /* 930828 */

	result = dsppCreateKnobProbe ( aknob,
		(DSPPInstrument *)ains->ains_DeviceInstrument,
		Name, RsrcType);
	if (result)
	{
		return result;
	}

/* Validate Type and use it to override default. */
	if( ifTypeSet ) aknob->aknob_Type = customType;

/* Add to instrument's list of knobs. */
	AddTail( &ains->ains_KnobList, (Node *) aknob );

	return (aknob->aknob_Item.n_Item);
}
/******************************************************************/
int32 internalDeleteAudioKnob (AudioKnob *aknob)
{

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioKnob(0x%lx)\n", aknob));

/* Remove from Instrument's List */
/* (use RemoveAndMarkNode() because this list can be cleared with afi_DeleteLinkedItems()) */
	RemoveAndMarkNode( (Node *) aknob );

	return (0);
}

/******************************************************************/
/***** SUPERVISOR MODE ********************************************/
/******************************************************************/

 /**
 |||	AUTODOC -public -class audio -group Knob -name SetKnob
 |||	Sets the value of a single-part Knob(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err SetKnob (Item knob, float32 value)
 |||
 |||	  Description
 |||
 |||	    This procedure sets the value of a single-part knob.
 |||
 |||	  Arguments
 |||
 |||	    knob
 |||	        Item number of the knob to be set. Always sets part 0.
 |||
 |||	    value
 |||	        The new value for that knob. This value is in the units apply to the
 |||	        AF_SIGNAL_TYPE_ of the Knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience macro implemented in <audio/audio.h> V27.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        SetKnobPart (knob, 0, value)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    ReadKnobPart(), CreateKnob(), SetKnobPart(), Knob(@)
 **/
 /**
 |||	AUTODOC -public -class audio -group Knob -name SetKnobPart
 |||	Sets the value of a Knob(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err SetKnobPart (Item knob, int32 partNum, float32 value)
 |||
 |||	  Description
 |||
 |||	    This procedure sets the value of a knob. This has been optimized to allow
 |||	    rapid modulation of sound parameters. The value is clipped to the allowable
 |||	    range and is converted to the appropriate internal units as necessary.
 |||
 |||	    The current value of the Knob can be read by calling ReadKnobPart().
 |||
 |||	    If this knob is connected to the output of another instrument via
 |||	    ConnectInstrumentParts(), the set value is ignored until that connection is
 |||	    broken.
 |||
 |||	  Arguments
 |||
 |||	    knob
 |||	        Item number of the knob to be set.
 |||
 |||	    partNum
 |||	        Index of the part of the knob to be set.
 |||
 |||	    value
 |||	        The new value for that knob. This value is in the units that apply to the
 |||	        AF_SIGNAL_TYPE_ of the Knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V27.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    ReadKnobPart(), CreateKnob(), SetKnob(), Knob(@)
 **/
int32 swiSetKnobPart ( Item KnobItem, int32 PartNum, float32 Value )
{
	AudioKnob *aknob;
	float32 calcval;

DBUG(("swiSetKnobPart(0x%x, %d, %g)\n", KnobItem, PartNum, Value));

	aknob = (AudioKnob *)CheckItem(KnobItem, AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return AF_ERR_BADITEM; /* 930828 */

	calcval = dsppConvertSignalToGeneric( aknob->aknob_Type,
		((DSPPInstrument *) aknob->aknob_DeviceInstrument)->dins_RateShift, Value );
	return dsppSetKnob( dsppKnobToResource(aknob), PartNum, calcval, aknob->aknob_Type );
}

 /**
 |||	AUTODOC -public -class audio -group Instrument -name BendInstrumentPitch
 |||	Bends an Instrument(@)'s output pitch up or down by a specified amount.
 |||
 |||	  Synopsis
 |||
 |||	    Err BendInstrumentPitch (Item instrument, float32 bendFrac)
 |||
 |||	  Description
 |||
 |||	    This procedure sets a new pitch bend value for an Instrument, a value that
 |||	    is stored as part of the Instrument. This function sets the resulting pitch
 |||	    to the product of the Instrument's frequency and BendFrac.
 |||
 |||	    This setting remains in effect until changed by another call to
 |||	    BendInstrumentPitch(). To restore the original pitch of an Instrument, pass
 |||	    the result of 1.0 as bendFrac.
 |||
 |||	    Use Convert12TET_FP() to compute a bendFrac value from an interval of
 |||	    semitones and cents.
 |||
 |||	    This procedure won't bend pitch above the upper frequency limit of the
 |||	    instrument.
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        The item number of an instrument
 |||
 |||	    bendFrac
 |||	        Factor to multiply Instrument's normal frequency by. This is a signed
 |||	        value; negative values result in a negative frequency value, which is
 |||	        not supported by all instruments. Specify 1.0 as bendFrac to restore the
 |||	        Instrument's original pitch.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
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
 |||	    Convert12TET_FP(), ConvertPitchBend()
 **/
Err swiBendInstrumentPitch( Item InstrumentItem, float32 BendFrac )
{
	AudioInstrument *ains;

DBUG(("swiBendInstrumentPitch(0x%x, %g)\n", InstrumentItem, BendFrac));

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;

	ains->ains_Bend = BendFrac;

/* If instrument is running, update all resources that determine pitch. */
	if( ains->ains_Status > AF_STOPPED )
	{
		dsppApplyStartFreq( ains, ains->ains_StartingFreqType,
			ains->ains_StartingFreqReq, ains->ains_StartingPitchNote,
			ains->ains_StartingAAtt);
	}

	return 0;

}

