/* @(#) audio_envelopes.c 96/08/16 1.65 */
/* $Id: audio_envelopes.c,v 1.38 1995/02/02 18:51:32 peabody Exp phil $ */
/****************************************************************
**
** Audio Internals to support Envelopes
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************
** 930804 PLB make KnobName bigger to avoid going past end.
** 930807 PLB added AF_TAG_RELEASEJUMP
** 930830 PLB Check for number of points in envelope.
** 931117 PLB Send signal for MonitorAttachment() for envelopes.
** 940505 PLB Delay at least 1 audio timer tick between envelope segments.
**            This was added after fixing the audio timer event handler on 940413.
**            The timer fix had a side effect of causing all events for a given time
**            to be processed, even if they were added to the list as a result
**            of processing another event at that time.  Before the fix, the event
**            would be delayed until the next tick.  The result was that envelopes
**            with very short attacks (1 msec) would not have a chance to rise.
**            This affected the laser sound for ShockWave.  It now works as before.
** 940602 WJB Moved default error code set to just after afi_IsRamAddr() to avoid
**            being clobbered by same.
** 940616 PLB COmmented out DebugEnvelope
** 940707 PLB Use default name "Env" if NULL name passed.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 941224 PLB Take DSP instrument execution rate into account for envelope timing.
** 950612 WJB Added ASIC-specific data memory access.
** 950818 WJB Published ENV_SUFFIX_ string defines in audio_structs.h.
** 950818 WJB Using new aeva_ DSPI field names. Cleaned up DSPPAttachEnvelope().
** 950821 WJB Added ENV_DRSC_TYPE_ defines.
** 950822 WJB Added dsppFindEnvHookResources().
** 950912 PLB make Envelope times relative to next pair.
** 950929 WJB Reorganized in preparation for AF_TAG_AUTO_DELETE_DATA.
**            Revised autodocs.
**            Added AF_NODE_F_AUTO_FREE_DATA support in internalDeleteAudioEnvelope().
** 950929 WJB Rewrote internalCreateAudioEnvelope() and internalSetEnvelopeInfo() around common tag handler.
**            Implemented AF_TAG_AUTO_FREE_DATA.
**            Removed AF_ENVF_FOLIO_OWNS.
** 951109 WJB Changed AF_TAG_AUTO_FREE_DATA to use MEMTYPE_TRACKSIZE.
** 951115 WJB Now clipping envelope values prior to writing to DSPI registers.
** 951116 WJB Added support for envelopes with signal types.
** 960129 WJB Added GetAudioItemInfo() support.
** 960522 PLB Apply aeva->aeva_TimeScale to SUSTAINTIME and RELEASETIME, CR6154
** 960522 PLB Use faster af_EnvelopeClock to fix CR6164
****************************************************************/

/****************
Envelope design:

X- Need signal driven event buffer.
Insertion sort events into pending list.

Internal Event Structure
	Node minimal
	Time to Execute
	Execution Vector
	User Data....

Envelope Attachment contains single node used for pending events.
	Allows simple removal from pending list if deleted.

Envelope Service:
	Do normal audio timer stuff then,
	Check Env Time
	Update EnvPhinc before ReqTarget because we should be near end of
	segment anyway and it will just clip if much bigger.
	Signal Envelope Servicer

May need to update in interrupt cuz signal latency could really mess up timing.

Attachment points to "class" method dispatcher for Start/Release/Stop/Update

Need to update EI via interrupt, calculate next and queue for interrupt service.
Envelope calculation:
	Delta = NewTarget-OldTarget
	EnvPhinc = (Delta * 32768) / (dT(sec) * SamplePerSecond) rounded up one
	Calculate actual NewTarget based on EnvPhinc
	Calc time to update

How to handle level segments when we don't quite get there on last seg,
	minimum update time.

Nasty Issues:
    How do we handle Attachment Names when envelopes use two knobs?
         SVFENV.target    user passes SVFENV
         SVFENV.phinc

*******************/

#include <dspptouch/dspp_touch.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"


/* Macros for debugging. */

#define DEBUG_Item          0       /* debug create/delete */
#define DEBUG_DumpEnvelope  0       /* enable DumpEnvelope() */

#if DEBUG_Item
#define DBUGITEM(x) PRT(x)
#else
#define DBUGITEM(x)
#endif

#define DBUG(x)    /* PRT(x) */
#define NODBUG(x)  /* */

#define CHECKMEM(msg) /* { PRT(msg); ReportMemoryUsage(); } */


/*****************************************************************/
/***** USER Level Folio Calls ************************************/
/*****************************************************************/

 /**
 |||	AUTODOC -public -class Items -group Audio -name Envelope
 |||	Audio envelope.
 |||
 |||	  Description
 |||
 |||	    An envelope is a time-variant function which can be used to control
 |||	    parameters of sounds that are to change over time (e.g., amplitude,
 |||	    frequency, filter characteristics, modulation amount, etc.). An Envelope
 |||	    Item is the envelope function description. In order to use an Envelope, it
 |||	    must be attached to an instrument with an envelope hook (e.g.,
 |||	    envelope.dsp(@), which simply outputs the Envelope's function as a control
 |||	    signal). Custom patches may also use envelopes.
 |||
 |||	    An Envelope's function is constructed from an array of EnvelopeSegments (see
 |||	    structure definition below). The initial value of the envelope function is
 |||	    envsegs[0].envs_Value. The function value then proceeds to
 |||	    envsegs[1].envs_Value, taking envsegs[0].envs_Duration seconds to do so.
 |||	    Barring any loops, the function proceeds from one envs_Value to the next in
 |||	    this manner until the final envs_Value in the array has been reached. The
 |||	    final envs_Duration value in the array is ignored.
 |||
 |||	    The function of each segment when used with envelope.dsp(@) is linear, but
 |||	    other instruments could interpolate between the envs_Values in other ways.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_ENVELOPE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateEnvelope(), CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DeleteEnvelope(), DeleteItem()
 |||
 |||	  Query
 |||
 |||	    GetAudioItemInfo()
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    CreateAttachment()
 |||
 |||	  Tags
 |||
 |||	    Data:
 |||
 |||	    AF_TAG_ADDRESS (const EnvelopeSegment *) - Create, Query, Modify*
 |||	        Pointer to array of EnvelopeSegments used to define the envelope. The
 |||	        length of the array is specified with AF_TAG_FRAMES. This data must
 |||	        remain valid for the life of the Envelope or until a different address
 |||	        is set with AF_TAG_ADDRESS.
 |||
 |||	        If the Envelope is created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then
 |||	        this data will be freed automatically when the Envelope is deleted.
 |||	        Memory used with AF_TAG_AUTO_FREE_DATA must be allocated with
 |||	        MEMTYPE_TRACKSIZE set.
 |||
 |||	    AF_TAG_AUTO_FREE_DATA (bool) - Create
 |||	        Set to TRUE to cause data pointed to by AF_TAG_ADDRESS to be freed
 |||	        automatically when Envelope Item is deleted. If the Item isn't
 |||	        successfully created, the memory isn't freed.
 |||
 |||	        The memory pointed to by AF_TAG_ADDRESS must be freeable by
 |||	        FreeMem (Address, TRACKED_SIZE).
 |||
 |||	    AF_TAG_FRAMES (int32) - Create, Query, Modify*
 |||	        Number of EnvelopeSegments in array pointed to by AF_TAG_ADDRESS.
 |||
 |||	    AF_TAG_TYPE (uint8) - Create, Query, Modify
 |||	        Determines the signal type of the envelope data (i.e., the units for
 |||	        envs_Value). Must be one of the AF_SIGNAL_TYPE_* defined in
 |||	        <audio/audio.h>. Defaults to AF_SIGNAL_TYPE_GENERIC_SIGNED on creation.
 |||
 |||	    * These tags cannot be used to modify an Envelope created with
 |||	    { AF_TAG_AUTO_FREE_DATA, TRUE }.
 |||
 |||	    Loops:
 |||
 |||	    AF_TAG_RELEASEBEGIN (int32) - Create, Query, Modify
 |||	        Index in EnvelopeSegment array for beginning of release loop. -1
 |||	        indicates no loop, which is the default on creation. If not -1, must <=
 |||	        the value set by AF_TAG_RELEASEEND. Must also be < the number of
 |||	        segments.
 |||
 |||	    AF_TAG_RELEASEEND (int32) - Create, Query, Modify
 |||	        Index in EnvelopeSegment array for end of release loop. -1 indicates no
 |||	        loop, which is the default on creation. If not -1, must >= the value set
 |||	        by AF_TAG_RELEASEBEGIN.  Must also be < the number of segments.
 |||
 |||	    AF_TAG_RELEASEJUMP (int32) - Create, Query, Modify
 |||	        Index in EnvelopeSegment array to jump to on release. When set, release
 |||	        causes escape from normal envelope processing to the specified index
 |||	        without disturbing the current output envelope value. From there, the
 |||	        envelope proceeds to the next EnvelopeSegment from the current value. -1
 |||	        to disable, which is the default on creation. Must be < the number of
 |||	        segments minus one.
 |||
 |||	    AF_TAG_RELEASETIME_FP (float32) - Create, Query, Modify
 |||	        The time in seconds used when looping from the end of the release loop
 |||	        back to the beginning. Defaults to 0.0 on creation.
 |||
 |||	    AF_TAG_SUSTAINBEGIN (int32) - Create, Query, Modify
 |||	        Index in EnvelopeSegment array for beginning of sustain loop. -1
 |||	        indicates no loop, which is the default on creation. If not -1, <= the
 |||	        value set by AF_TAG_SUSTAINEND. Must also be < the number of segments.
 |||
 |||	    AF_TAG_SUSTAINEND (int32) - Create, Query, Modify
 |||	        Index in EnvelopeSegment array for end of sustain loop. -1 indicates no
 |||	        loop, which is the default on creation. If not -1, >= the value set by
 |||	        AF_TAG_SUSTAINBEGIN. Must also be < the number of segments.
 |||
 |||	    AF_TAG_SUSTAINTIME_FP (float32) - Create, Query, Modify
 |||	        The time in seconds used when looping from the end of the sustain loop
 |||	        back to the beginning. Defaults to 0.0 on creation.
 |||
 |||	    Time Scaling:
 |||
 |||	    These tags define how the Envelope responds to the StartInstrument() tag
 |||	    AF_TAG_PITCH.
 |||
 |||	    AF_TAG_BASENOTE (uint8) - Create, Query, Modify
 |||	        MIDI note number of pitch at which pitch-based time scale factor is 1.0.
 |||	        Defaults to 0 on creation.
 |||
 |||	    AF_TAG_NOTESPEROCTAVE (int8) - Create, Query, Modify
 |||	        Number of semitones at which pitch-based time scale doubles. A positive
 |||	        value makes the envelope times shorter as pitch increases; a negative
 |||	        value makes the envelope times longer as pitch increases. Zero (the
 |||	        default) disables pitch-based time scaling.
 |||
 |||	    Misc:
 |||
 |||	    AF_TAG_CLEAR_FLAGS (uint32) - Create, Modify
 |||	        Set of AF_ENVF_ flags (see below) to clear. Clears every flag for which
 |||	        a 1 is set in ta_Arg.
 |||
 |||	    AF_TAG_SET_FLAGS (uint32) - Create, Query, Modify
 |||	        Set of AF_ENVF_ flags (see below) to set. Sets every flag for which a 1
 |||	        is set in ta_Arg.
 |||
 |||	  Flags
 |||
 |||	    AF_ENVF_LOCKTIMESCALE
 |||	        When set, causes the Time Scale for this envelope to
 |||	        ignore the use AF_TAG_TIME_SCALE_FP in StartInstrument()
 |||	        This is useful if you are using multiple envelopes in an
 |||	        instrument and want some to be time scaled, and some not to be.
 |||	        Envelopes used as complex LFOs are often not time scaled.
 |||
 |||	        This flag does not affect pitch-based time scaling.
 |||
 |||	    AF_ENVF_FATLADYSINGS
 |||	        The state of this flag indicates the default setting for the
 |||	        AF_ATTF_FATLADYSINGS Attachment flag whenever this Envelope is attached
 |||	        to an Instrument.
 |||
 |||	  Data Format -preformatted
 |||
 |||	    typedef struct EnvelopeSegment
 |||	    {
 |||	        float32 envs_Value;         // Starting value of this segment of the array.
 |||	                                    // The units of this depend on the AF_TAG_TYPE
 |||	                                    // setting for the Envelope Item.
 |||
 |||	        float32 envs_Duration;      // Time in seconds to reach the envs_Value of
 |||	                                    // the next EnvelopeSegment in the array.
 |||	    } EnvelopeSegment;
 |||
 |||	  Example
 |||
 |||	    The EnvelopeSegment array:
 |||
 |||	    EnvelopeSegment envsegs[] = {
 |||	       // value, duration
 |||	        { 0.0,   0.1 },
 |||	        { 1.0,   0.2 },
 |||	        { 0.5,   0.5 },
 |||	        { 0.0,   0.0 }
 |||	    };
 |||
 |||	    corresponds to the graph:
 |||
 |||	           1.00 +    O
 |||	                |    **
 |||	                |    * *
 |||	                |   *   *
 |||	                |   *    *
 |||	           0.75 +   *     *
 |||	                |   *      *
 |||	                |  *        *
 |||	                |  *         *
 |||	                |  *          *
 |||	    f(t)   0.50 +  *           O*
 |||	                | *              **
 |||	                | *                ***
 |||	                | *                   **
 |||	                | *                     ***
 |||	           0.25 +*                         **
 |||	                |*                           ***
 |||	                |*                              **
 |||	                |*                                ***
 |||	                *                                    **
 |||	           0.00 O----+----+----+----+----+----+----+---*O----+
 |||	               0.0  0.1  0.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9
 |||
 |||	                                    t (sec)
 |||
 |||	  Caveats
 |||
 |||	    Care must be taken to avoid mismatching signed and unsigned Envelopes and
 |||	    Envelope instruments. A mismatch will result in a numeric overflow in the
 |||	    envelope instrument.
 |||
 |||	  See Also
 |||
 |||	    Attachment(@), Instrument(@), Template(@), envelope.dsp(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Envelope -name CreateEnvelope
 |||	Creates an Envelope(@).
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateEnvelope (const EnvelopeSegment *points, int32 numPoints,
 |||	                         const TagArg *tagList);
 |||
 |||	    Item CreateEnvelopeVA (const EnvelopeSegment *points, int32 numPoints,
 |||	                           uint32 tag1, ...);
 |||
 |||	  Description
 |||
 |||	    Creates an Envelope Item which can be attached to an Instrument(@) or
 |||	    Template(@) with an envelope hook.
 |||
 |||	    When you are finished with the Envelope, delete it with DeleteEnvelope().
 |||
 |||	  Arguments
 |||
 |||	    points
 |||	        An array of EnvelopeSegment values giving time in seconds accompanied
 |||	        by data values. The Points array is not copied: it must remain valid
 |||	        for the life of the Envelope Item or until it is replaced with another
 |||	        array of Points by a call to SetAudioItemInfo().
 |||
 |||	    numPoints
 |||	        The number of points in the array.
 |||
 |||	  Tag Arguments
 |||
 |||	    See Envelope(@).
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the envelope (a non-negative
 |||	    value) or an error code (a negative value) if an error occurs.
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
 |||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE),
 |||	            AF_TAG_ADDRESS, points,
 |||	            AF_TAG_FRAMES,  numPoints,
 |||	            TAG_JUMP,       tagList);
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    Envelope(@), DeleteEnvelope(), CreateAttachment(), envelope.dsp(@)
 **/

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Envelope -name DeleteEnvelope
 |||	Deletes an Envelope(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteEnvelope (Item Envelope)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the specified envelope, freeing its resources. It
 |||	    also deletes any Attachment(@)s to the envelope.
 |||
 |||	    If the Envelope was created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then the
 |||	    EnvelopeSegment array is also be freed when the Envelope Item is deleted.
 |||	    Otherwise, the EnvelopeSegment array is not freed automatically.
 |||
 |||	  Arguments
 |||
 |||	    Envelope
 |||	        Item number of the Envelope to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro defined in <audio/audio.h> V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    Envelope(@), CreateEnvelope(), DeleteAttachment()
 **/


/*****************************************************************/
/***** Create/Delete/Modify Envelope Item ************************/
/*****************************************************************/

static void SetEnvelopeDefaults (AudioEnvelope *);
static Err CreateEnvelopeTagHandler (AudioEnvelope *, void *dummy, uint32 tag, TagData arg);
static Err EnvelopeTagHandler (AudioEnvelope *, uint32 tag, TagData arg);
static Err SetEnvelopeLoopParameter (int32 *resultValue, int32 val);
static Err ValidateAudioEnvelope (const AudioEnvelope *);

/*****************************************************************/
/* AUDIO_ENVELOPE_NODE ir_Create method */
Item internalCreateAudioEnvelope (AudioEnvelope *aenv, const TagArg *tagList)
{
    Err errcode;

    DBUGITEM(("internalCreateAudioEnvelope(0x%x,0x%x)\n", aenv, tagList));

        /* pre-init aenv */
    PrepList (&aenv->aenv_AttachmentRefs);
    SetEnvelopeDefaults (aenv);

        /* process tags and validate results */
        /* (this doesn't allocate anything, so no need to clean anything up) */
    if (errcode = TagProcessor (aenv, tagList, CreateEnvelopeTagHandler, NULL)) return errcode;
    if (errcode = ValidateAudioEnvelope (aenv)) return errcode;

  #if DEBUG_Item || DEBUG_DumpEnvelope
    internalDumpEnvelope (aenv, "internalCreateAudioEnvelope");
  #endif

        /* return Item as success */
    return aenv->aenv_Item.n_Item;
}

/*****************************************************************/
/* AUDIO_ENVELOPE_NODE SetAudioItemInfo() method */
Err internalSetEnvelopeInfo (AudioEnvelope *aenv, const TagArg *tagList)
{
    AudioEnvelope tempaenv = *aenv;
    Err errcode;

        /* process tags and validate results */
        /* (this doesn't allocate anything, so no need to clean anything up) */
        /* @@@ this must prevent any changes to tempaenv that are invalid in aenv
           - does blind structure copy to apply changes */
    {
        const TagArg *tag;
        int32 tagResult;

        for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
            DBUGITEM(("internalSetEnvelopeInfo: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));

            switch (tag->ta_Tag) {
                case AF_TAG_ADDRESS:
                case AF_TAG_FRAMES:
                    if (tempaenv.aenv_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
                        ERR(("internalSetEnvelopeInfo: Created with { AF_TAG_AUTO_FREE_DATA, TRUE }; can't change data address or size\n"));
                        return AF_ERR_BADTAG;
                    }
                    /* fall thru to default */

                default:
                    if ((errcode = EnvelopeTagHandler (&tempaenv, tag->ta_Tag, tag->ta_Arg)) < 0) return errcode;
                    break;
            }
        }

            /* Catch tag processing errors */
        if ((errcode = tagResult) < 0) {
            ERR(("internalSetEnvelopeInfo: Error processing tag list 0x%x\n", tagList));
            return errcode;
        }
    }
    if (errcode = ValidateAudioEnvelope (&tempaenv)) return errcode;

        /* apply results */
    *aenv = tempaenv;

  #if DEBUG_Item || DEBUG_DumpEnvelope
    internalDumpEnvelope (aenv, "internalSetEnvelopeInfo");
  #endif

    return 0;
}

/*****************************************************************/
/*
    Set envelope defaults. Assumes that caller has precleared AudioEnvelope.
*/
static void SetEnvelopeDefaults (AudioEnvelope *aenv)
{
        /* Setup Envelope field defaults. */
    aenv->aenv_SustainBegin = -1;
    aenv->aenv_SustainEnd   = -1;
/*  aenv->aenv_SustainTime  = 0.0; @@@ assuming this is 0x00000000 */
    aenv->aenv_ReleaseJump  = -1;
    aenv->aenv_ReleaseBegin = -1;
    aenv->aenv_ReleaseEnd   = -1;
/*  aenv->aenv_ReleaseTime  = 0.0; @@@ assuming this is 0x00000000 */
    aenv->aenv_SignalType   = AF_SIGNAL_TYPE_GENERIC_SIGNED;
}

/*****************************************************************/
/*
    TagProcessor() callback function for internalCreateAudioEnvelope()

    Arguments
        aenv
            AudioEnvelope Item to fill out. Client must initialize it before
            calling TagProcessor().
*/
static Err CreateEnvelopeTagHandler (AudioEnvelope *aenv, void *dummy, uint32 tag, TagData arg)
{
    TOUCH(dummy);

    DBUGITEM(("CreateEnvelopeTagHandler: tag { %d, 0x%x }\n", tag, arg));

    switch (tag) {
        case AF_TAG_AUTO_FREE_DATA:
            if ((int32)arg) aenv->aenv_Item.n_Flags |= AF_NODE_F_AUTO_FREE_DATA;
            else            aenv->aenv_Item.n_Flags &= ~AF_NODE_F_AUTO_FREE_DATA;
            break;

        default:
            return EnvelopeTagHandler (aenv, tag, arg);
    }

    return 0;
}

/*****************************************************************/
/*
    Common tag processing for CreateEnvelopeTagHandler() and internalSetEnvelopeInfo()

    Arguments
        aenv
            AudioEnvelope Item to fill out.
*/
static Err EnvelopeTagHandler (AudioEnvelope *aenv, uint32 tag, TagData arg)
{
    switch (tag) {
    /* Data */
        case AF_TAG_ADDRESS:
                /* memory is validated by ValidateAudioEnvelope() */
            aenv->aenv_Points = (EnvelopeSegment *)arg;
            break;

        case AF_TAG_FRAMES:
                /* validated by ValidateAudioEnvelope() */
            aenv->aenv_NumPoints = (int32)arg;
            break;

        case AF_TAG_TYPE:
            {
                const uint32 sigType = (uint32)arg;

                    /* always validate, because otherwise things indexed by this will be over-indexed */
                if (sigType > AF_SIGNAL_TYPE_MAX) {
                    ERR(("EnvelopeTagHandler: Illegal signal type (%u)\n", sigType));
                    return AF_ERR_BAD_SIGNAL_TYPE;
                }

                aenv->aenv_SignalType = sigType;
            }
            break;

    /* Loops */
    /* (loop points use common processing) */
        case AF_TAG_SUSTAINBEGIN:
            return SetEnvelopeLoopParameter (&aenv->aenv_SustainBegin, (int32)arg);

        case AF_TAG_SUSTAINEND:
            return SetEnvelopeLoopParameter (&aenv->aenv_SustainEnd, (int32)arg);

        case AF_TAG_SUSTAINTIME_FP:
            aenv->aenv_SustainTime = ConvertTagData_FP(arg);
            break;

        case AF_TAG_RELEASEBEGIN:
            return SetEnvelopeLoopParameter (&aenv->aenv_ReleaseBegin, (int32)arg);

        case AF_TAG_RELEASEEND:
            return SetEnvelopeLoopParameter (&aenv->aenv_ReleaseEnd, (int32)arg);

        case AF_TAG_RELEASETIME_FP:
            aenv->aenv_ReleaseTime = ConvertTagData_FP(arg);
            break;

        case AF_TAG_RELEASEJUMP:
            return SetEnvelopeLoopParameter (&aenv->aenv_ReleaseJump, (int32)arg);

    /* Time Scaling */
        case AF_TAG_BASENOTE:
            {
                const uint32 baseNote = (uint32)arg;

                if (baseNote > 127) {
                    ERR(("EnvelopeTagHandler: Base note out of range (%u)\n", baseNote));
                    return AF_ERR_BADTAGVAL;
                }

                aenv->aenv_BaseNote = baseNote;
            }
            break;

        case AF_TAG_NOTESPEROCTAVE:
            {
                const int32 notesPerDouble = (int32)arg;

              #ifdef BUILD_PARANOIA
                if (notesPerDouble > 127 || notesPerDouble < -128) {
                    ERR(("EnvelopeTagHandler: Notes per double out of range (%d)\n", notesPerDouble));
                    return AF_ERR_BADTAGVAL;
                }
              #endif

                aenv->aenv_NotesPerDouble = (int8)notesPerDouble;
            }
            break;

    /* Misc */
        case AF_TAG_SET_FLAGS:
            {
                const uint32 setFlags = (uint32)arg;

              #ifdef BUILD_PARANOIA
                if (setFlags & ~AF_ENVF_LEGALFLAGS) {
                    ERR(("EnvelopeTagHandler: Illegal envelope flags (0x%x)\n", setFlags));
                    return AF_ERR_BADTAGVAL;
                }
              #endif

                aenv->aenv_Flags |= setFlags;
            }
            break;

        case AF_TAG_CLEAR_FLAGS:
            {
                const uint32 clearFlags = (uint32)arg;

              #ifdef BUILD_PARANOIA
                if (clearFlags & ~AF_ENVF_LEGALFLAGS) {
                    ERR(("EnvelopeTagHandler: Illegal envelope flags (0x%x)\n", clearFlags));
                    return AF_ERR_BADTAGVAL;
                }
              #endif

                aenv->aenv_Flags &= ~clearFlags;
            }
            break;

        default:
            ERR(("EnvelopeTagHandler: Unrecognized tag { %d, 0x%x }\n", tag, arg));
            return AF_ERR_BADTAG;
    }

    return 0;
}

/*****************************************************************/
/*
    Check and install a loop parameter. Target is a int32.
*/
static Err SetEnvelopeLoopParameter (int32 *resultValue, int32 val)
{
    if (val < -1) {
        ERR(("EnvelopeTagHandler: Loop value (%d) out of range\n", val));
        return AF_ERR_OUTOFRANGE;
    }

    *resultValue = val;
    return 0;
}

/*****************************************************************/
/*
    Validate contents of an AudioEnvelope being considered for either creating
    as an Item or posting to an existing Item.

    Validates:
        . loop point relational bounds checking
        . aenv_Points, aenv_NumPoints points to real memory
        . if AF_NODE_F_AUTO_FREE_DATA is set, verifies that calling task
          can free this memory.

    Arguments
        aenv
*/
static Err ValidateAudioEnvelope (const AudioEnvelope *aenv)
{
        /* Make sure NumPoints >= 1 */
    if (aenv->aenv_NumPoints < 1) {
        ERR(("ValidateAudioEnvelope: NumPoints (%d) must be >= 1\n", aenv->aenv_NumPoints));
        return AF_ERR_BADTAGVAL;
    }

        /* Check sustain loop */
    if (aenv->aenv_SustainBegin != -1) {
        if (aenv->aenv_SustainEnd >= aenv->aenv_NumPoints) {
            ERR(("ValidateAudioEnvelope: SustainEnd (%d) must be < NumPoints (%d)\n",
                aenv->aenv_SustainEnd, aenv->aenv_NumPoints));
            return AF_ERR_BADTAGVAL;
        }
        if (aenv->aenv_SustainBegin > aenv->aenv_SustainEnd) {
            ERR(("ValidateAudioEnvelope: SustainBegin (%d) must be <= SustainEnd (%d)\n",
                aenv->aenv_SustainBegin, aenv->aenv_SustainEnd));
            return AF_ERR_BADTAGVAL;
        }
    }

        /* Check release loop */
    if (aenv->aenv_ReleaseBegin != -1) {
        if (aenv->aenv_ReleaseEnd >= aenv->aenv_NumPoints) {
            ERR(("ValidateAudioEnvelope: ReleaseEnd (%d) must be < NumPoints (%d)\n",
                aenv->aenv_ReleaseEnd, aenv->aenv_NumPoints));
            return AF_ERR_BADTAGVAL;
        }
        if (aenv->aenv_ReleaseBegin > aenv->aenv_ReleaseEnd) {
            ERR(("ValidateAudioEnvelope: ReleaseBegin (%d) must be <= ReleaseEnd (%d)\n",
                aenv->aenv_ReleaseBegin, aenv->aenv_ReleaseEnd));
            return AF_ERR_BADTAGVAL;
        }
    }

        /* Check release jump */
    if (aenv->aenv_ReleaseJump != -1) {
        if (aenv->aenv_ReleaseJump >= aenv->aenv_NumPoints-1) {
            ERR(("ValidateAudioEnvelope: ReleaseJump (%d) must be < NumPoints-1 (%d)\n",
                aenv->aenv_ReleaseJump, aenv->aenv_NumPoints-1));
            return AF_ERR_BADTAGVAL;
        }
    }

  #if BUILD_PARANOIA
        /* Check release jump against release loop in development mode -
           don't let it jump past the end of the loop, if there is one.
           Nothing will blow up if this isn't true, so it's just a diagnostic
           when BUILD_PARANOIA is on. */
    if (aenv->aenv_ReleaseJump != -1 && aenv->aenv_ReleaseBegin != -1) {
        if (aenv->aenv_ReleaseJump > aenv->aenv_ReleaseEnd) {
            ERR(("ValidateAudioEnvelope: ReleaseJump (%d) must be <= ReleaseEnd (%d)\n",
                aenv->aenv_ReleaseJump, aenv->aenv_ReleaseEnd));
            return AF_ERR_BADTAGVAL;
        }
    }
  #endif

    return ValidateAudioItemData (aenv->aenv_Points, aenv->aenv_NumPoints * sizeof(EnvelopeSegment), (aenv->aenv_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) != 0);
}

/**************************************************************/
/* AUDIO_ENVELOPE_NODE ir_Delete method */
int32 internalDeleteAudioEnvelope (AudioEnvelope *aenv, Task *ct)
{
    DBUGITEM(("internalDeleteAudioEnvelope(0x%x)\n", aenv->aenv_Item.n_Item));

        /* delete attachments to this item */
    afi_DeleteReferencedItems (&aenv->aenv_AttachmentRefs);

        /* delete AF_NODE_F_AUTO_FREE_DATA data */
    if (aenv->aenv_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
        DBUGITEM(("internalDeleteAudioEnvelope: Deleting AF_NODE_F_AUTO_FREE_DATA @ 0x%08x, 0x%x bytes\n", aenv->aenv_Points, GetMemTrackSize(aenv->aenv_Points)));
        SuperFreeUserMem (aenv->aenv_Points, TRACKED_SIZE, ct);
    }

    return 0;
}


/*****************************************************************/
/* AUDIO_ENVELOPE_NODE GetAudioItemInfo() method */
Err internalGetEnvelopeInfo (const AudioEnvelope *aenv, TagArg *tagList)
{
    TagArg *tag;
    int32 tagResult;

    for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
        DBUGITEM(("internalGetEnvelopeInfo: %d\n", tag->ta_Tag));

        switch (tag->ta_Tag) {
        /* Data */
            case AF_TAG_ADDRESS:
                tag->ta_Arg = (TagData)aenv->aenv_Points;
                break;

            case AF_TAG_FRAMES:
                tag->ta_Arg = (TagData)aenv->aenv_NumPoints;
                break;

            case AF_TAG_TYPE:
                tag->ta_Arg = (TagData)aenv->aenv_SignalType;
                break;

        /* Loops */
            case AF_TAG_SUSTAINBEGIN:
                tag->ta_Arg = (TagData)aenv->aenv_SustainBegin;
                break;

            case AF_TAG_SUSTAINEND:
                tag->ta_Arg = (TagData)aenv->aenv_SustainEnd;
                break;

            case AF_TAG_SUSTAINTIME_FP:
                tag->ta_Arg = ConvertFP_TagData(aenv->aenv_SustainTime);
                break;

            case AF_TAG_RELEASEBEGIN:
                tag->ta_Arg = (TagData)aenv->aenv_ReleaseBegin;
                break;

            case AF_TAG_RELEASEEND:
                tag->ta_Arg = (TagData)aenv->aenv_ReleaseEnd;
                break;

            case AF_TAG_RELEASETIME_FP:
                tag->ta_Arg = ConvertFP_TagData(aenv->aenv_ReleaseTime);
                break;

            case AF_TAG_RELEASEJUMP:
                tag->ta_Arg = (TagData)aenv->aenv_ReleaseJump;
                break;

        /* Time Scaling */
            case AF_TAG_BASENOTE:
                tag->ta_Arg = (TagData)aenv->aenv_BaseNote;
                break;

            case AF_TAG_NOTESPEROCTAVE:
                tag->ta_Arg = (TagData)aenv->aenv_NotesPerDouble;
                break;

        /* Misc */
            case AF_TAG_SET_FLAGS:
                    /* filter off AF_ENVF_LEGALFLAGS in case there are internal flags stored here too */
                tag->ta_Arg = (TagData)(aenv->aenv_Flags & AF_ENVF_LEGALFLAGS);
                break;

            default:
                ERR (("internalGetEnvelopeInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
                return AF_ERR_BADTAG;
        }
    }

        /* Catch tag processing errors */
    if (tagResult < 0) {
        ERR(("internalGetEnvelopeInfo: Error processing tag list 0x%x\n", tagList));
        return tagResult;
    }

    return 0;
}


/* -------------------- Debug */
/* !!! move to audio_debug.h? */

#if DEBUG_Item || DEBUG_DumpEnvelope /* { */

/*****************************************************************/
Err DumpEnvelope (Item EnvelopeItem, const char *banner)
{
    const AudioEnvelope * const aenv = (AudioEnvelope *)CheckItem(EnvelopeItem, AUDIONODE, AUDIO_ENVELOPE_NODE);

    if (aenv == NULL) return AF_ERR_BADITEM;

    internalDumpEnvelope (aenv, banner);

    return 0;
}

/*****************************************************************/
#define REPORT_ENVELOPE(format,member) printf("  %-18s " format "\n", #member, aenv->member)
void internalDumpEnvelope (const AudioEnvelope *aenv, const char *banner)
{
    int32 i;

    PRT(("--------------------------\n"));
    if (banner) PRT(("%s: ", banner));
    PRT(("Envelope 0x%x ('%s') @ 0x%08x\n", aenv->aenv_Item.n_Item, aenv->aenv_Item.n_Name, aenv));
    REPORT_ENVELOPE("0x%02x",aenv_Item.n_Flags);
    REPORT_ENVELOPE("0x%08x",aenv_Points);
    REPORT_ENVELOPE("%d",aenv_NumPoints);
    REPORT_ENVELOPE("%d",aenv_SustainBegin);
    REPORT_ENVELOPE("%d",aenv_SustainEnd);
    REPORT_ENVELOPE("%g",aenv_SustainTime);
    REPORT_ENVELOPE("%d",aenv_ReleaseJump);
    REPORT_ENVELOPE("%d",aenv_ReleaseBegin);
    REPORT_ENVELOPE("%d",aenv_ReleaseEnd);
    REPORT_ENVELOPE("%g",aenv_ReleaseTime);
    REPORT_ENVELOPE("0x%02x",aenv_Flags);
    REPORT_ENVELOPE("%u",aenv_SignalType);

    for (i=0; i<aenv->aenv_NumPoints; i++) {
        printf ("  %2d: { %g, %g }\n", i, aenv->aenv_Points[i].envs_Value, aenv->aenv_Points[i].envs_Duration);
    }
}
#endif  /* } */


/*****************************************************************/
/***** Envelope Operations ***************************************/
/*****************************************************************/

/*****************************************************************/
void SetEnvAttTimeScale( AudioAttachment *aatt, int32 PitchNote, float32 timeScale )
{
	AudioEnvExtension *const aeva = aatt->aatt_Extension;
	AudioEnvelope *aenv = (AudioEnvelope *) aatt->aatt_Structure;
	float32 tscale = 1.0;

/* Perform pitch based time scaling if pitch valid and scaling specified by NotesPerDouble
** TimeScale = 2**((basenote - pitch)/npd)
**    if (pitch == basenote) then scale = 1.0
**    if (basenote - pitch)/npd == 1.0 then scale = 2.0, thus length double for each npd
*/
	if( (PitchNote >= 0) && (aenv->aenv_NotesPerDouble != 0) )
	{
		tscale = Approximate2toX((float32)(aenv->aenv_BaseNote - PitchNote) / (float32)aenv->aenv_NotesPerDouble);
	}

	if( !(aenv->aenv_Flags & AF_ENVF_LOCKTIMESCALE) ) tscale *= timeScale;

	aeva->aeva_TimeScale = tscale;
	DBUG(("aeva->aeva_TimeScale = %g\n", aeva->aeva_TimeScale));
}

/* ENV_PHASERANGE used to be 32768 which was negative and caused a
** phase reversal and unexpected glitch when loop points did not
** line up, and (PhaseIncr == ENV_PHASERANGE)
*/
#define ENV_PHASERANGE (32767)
#define ENV_MAXDELTA (400)
/******************************************************************
** On entry to this routine:
**    aeva->aeva_CurIndex = index of current point
******************************************************************/
int32 NextEnvSegment( AudioEnvExtension *aeva )
{
	AudioEnvelope *aenv;
	AudioAttachment *aatt;
	int32 PhaseIncrement;
	int32 DelayTime;
	int32 Index;
	int32 Result = 0, PostIt;
	int32 ActualFrames, FramesLeft, Frames; /* These are CALCULATION frames, not SAMPLE frames. */
	float32 InterpTarget, Target;
	int32 TargetValue;
	int32 RateShift;
	AudioInstrument *ains;
	DSPPInstrument *dins;

	aatt = aeva->aeva_Parent;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;
	Index = aeva->aeva_CurIndex;

	ains = (AudioInstrument *) LookupItem(aatt->aatt_HostItem);
	if( ains == NULL ) return AF_ERR_BADITEM;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
/* Use RateShift whenever we do calculations relating Frames and Time. */
	RateShift = dins->dins_RateShift;

	CHECKMEM(("Begin NextEnvSegment"));

	if(Index < aenv->aenv_NumPoints)  /* Are we done with envelope? */
	{
/* Do we continue with a long segment or start a new one? */
		if( aeva->aeva_FramesLeft > 0 )
		{
			Frames = aeva->aeva_FramesLeft;
DBUG(("------\nNextEnvSegment: FramesLeft = %d )\n", Frames));
		}
		else
		{
/* If we RateShift by 8, then it should take fewer calc frames to reach target, so shift right. */
			Frames = ((int32) (aeva->aeva_DeltaTime * AF_SAMPLE_RATE)) >> RateShift;
DBUG(("==========\nNextEnvSegment: Delta = %g, Frames = %d, RateShift = %d )\n", aeva->aeva_DeltaTime, Frames, RateShift));
		}
		Target = dsppConvertSignalToGeneric (aenv->aenv_SignalType, 0, aenv->aenv_Points[Index].envs_Value);
DBUG(("NextEnvSegment: data[%d] = %g (raw: %g) )\n", Index, aenv->aenv_Points[Index].envs_Value, Target));

/* Calculate actual number of frames we will take to reach phase == 1.0 */

		if(Frames > ENV_PHASERANGE) /* Is Frames too many to do in one pass? */
		{
			PhaseIncrement = 1;  /* Use slowest possible phase increment. */
		}
		else
		{
			if(Frames == 0) Frames = 1;
			PhaseIncrement = (ENV_PHASERANGE + (Frames - 1)) / Frames;
		}

/* Calculate actual number of frames for this pass based on quantized phase increment. */
		ActualFrames = (ENV_PHASERANGE / PhaseIncrement);

DBUG(("\nNextEnvSegment: PhaseIncrement = %d, Frames = %d\n", PhaseIncrement, Frames ));
DBUG(("NextEnvSegment: ActualFrames = %d )\n", ActualFrames));

/* Do we break the remaining segment into pieces? */
		FramesLeft = Frames - ActualFrames;
		if( (FramesLeft << RateShift) > ENV_MAXDELTA )  /* Compare to tolerable time value. */
		{
/* Do segment in stages. */
			aeva->aeva_FramesLeft = FramesLeft;
/* Interpolate intermediate value. */
			InterpTarget = (((Target - aeva->aeva_PrevTarget) * ActualFrames) / Frames) +
					aeva->aeva_PrevTarget;
			Target = InterpTarget;
DBUG(("NextEnvSegment: InterpTarget = %g )\n", InterpTarget));
		}
		else
		{
/* Do it in one shot. */
			aeva->aeva_FramesLeft = 0;
			aeva->aeva_DeltaTime = aenv->aenv_Points[aeva->aeva_CurIndex].envs_Duration * aeva->aeva_TimeScale;
			aeva->aeva_CurIndex++;
		}
		aeva->aeva_PrevTarget = Target;
		TargetValue = dsppClipRawValue (aenv->aenv_SignalType, ConvertFP_SF15(Target));
DBUG(("NextEnvSegment: IncrDSPI=%d, RequestDSPI=%g (%d)\n", PhaseIncrement, Target, TargetValue ));
		dsphWriteDataMem( aeva->aeva_IncrDSPI, PhaseIncrement );
		dsphWriteDataMem( aeva->aeva_RequestDSPI, TargetValue );

/* Decide whether to loop and whether to post or not. */
		PostIt = TRUE;
		if(aatt->aatt_ActivityLevel == AF_STARTED)
		{
			if(aenv->aenv_SustainBegin >= 0)
			{
				if(aeva->aeva_CurIndex > aenv->aenv_SustainEnd)
				{
					aeva->aeva_CurIndex = aenv->aenv_SustainBegin;  /* Loop back. */
					aeva->aeva_DeltaTime = aenv->aenv_SustainTime * aeva->aeva_TimeScale; /* 960522 */
					if(aenv->aenv_SustainBegin == aenv->aenv_SustainEnd)
					{
						PostIt = FALSE;  /* Hold at single Sustain point. */
					}
				}
			}
		}
		if(aenv->aenv_ReleaseBegin >= 0)
		{
			if(aeva->aeva_CurIndex > aenv->aenv_ReleaseEnd)
			{
				aeva->aeva_CurIndex = aenv->aenv_ReleaseBegin;  /* Loop back. */
				aeva->aeva_DeltaTime = aenv->aenv_ReleaseTime * aeva->aeva_TimeScale;  /* 960522 */
			}
		}

		if(PostIt)
		{
			DelayTime = (ActualFrames << RateShift) / aeva->aeva_Event.aevt_Clock->aclk_Duration;
			if( DelayTime < 1 ) DelayTime = 1;
DBUG(("NextEnvSegment: post at %d + %d\n", swiGetAudioTime(), DelayTime ));
			Result = PostAudioEvent( (AudioEvent *) aeva,
				(AudioTime) GetAudioClockTime( aeva->aeva_Event.aevt_Clock ) + DelayTime);
		}
	}
	else
	{
/* Stop instrument if envelope finished and FATLADYSINGS bit set. */
		DBUG(("Env stopped, flags = 0x%x\n", aatt->aatt_Flags ));
		if( aatt->aatt_Flags & AF_ATTF_FATLADYSINGS )
		{
			DBUG(("Env stopped, FLS => StopIns\n" ));
			Result = swiStopInstrument( aatt->aatt_HostItem, NULL );
			if (Result < 0)
			{
				ERR(("NextEnvSegment: error stopping 0x%x\n", aatt->aatt_HostItem));
			}
		}

/* 931117 Signal task that called MonitorAttachment. */
		SignalMonitoringCue( aatt );

	}

	return Result;
}

/*****************************************************************/
int32 DSPPStartEnvAttachment( AudioAttachment *aatt )
{
	AudioEnvExtension *aeva;
	AudioEnvelope *aenv;
	int32 Result;
	float32 Target;
	int32 TargetValue;
	int32 CurrentValue;

DBUG(("DSPPStartEnvAttachment( attt=0x%lx )\n", aatt));
	aeva = (AudioEnvExtension *) aatt->aatt_Extension;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;

DBUG(("DSPPStartEnvAttachment: aeva = 0x%lx )\n", aeva));
/* set initial envelope value in DSP by . */
	Target = dsppConvertSignalToGeneric (aenv->aenv_SignalType, 0,
		aenv->aenv_Points[aatt->aatt_StartAt].envs_Value);
	TargetValue = dsppClipRawValue (aenv->aenv_SignalType, ConvertFP_SF15 (Target));
DBUG(("DSPPStartEnvAttachment: initial value: %g (raw: %g (%d))\n",
	aenv->aenv_Points[aatt->aatt_StartAt].envs_Value, Target, TargetValue));

/* Calculate Current value in signed range to trigger phase update. */
	if( TargetValue > 0 )
	{
		CurrentValue = TargetValue - 1;
	}
	else
	{
		CurrentValue = TargetValue + 1;
	}

	dsphWriteDataMem( aeva->aeva_CurrentDSPI, CurrentValue );
	dsphWriteDataMem( aeva->aeva_TargetDSPI, TargetValue );
	aeva->aeva_CurIndex = aatt->aatt_StartAt;  /* CR5943 */
	aeva->aeva_PrevTarget = Target;
	aeva->aeva_FramesLeft = 0;
	aeva->aeva_DeltaTime = 0.0;
	aeva->aeva_Event.aevt_Perform = (void *)NextEnvSegment;   /* Set callback function. */
	aatt->aatt_ActivityLevel = AF_STARTED;

	Result = NextEnvSegment( aeva );
DBUG(("DSPPStartEnvAttachment returns 0x%lx )\n", Result));
	return Result;
}

/*****************************************************************
** Upon release:

** If stuck and no release => advance to next

** If looping and no release => noop, AF_RELEASED will break loop.
** If no sustain and no release => continue

** If stuck and yes releasejump => advance to beginning of release
** If looping and yes releasejump => unpost, advance to beginning of release
** If no sustain and yes releasejump => unpost, advance to beginning of release
** How finish?
*/
int32 DSPPReleaseEnvAttachment( AudioAttachment *aatt )
{
	AudioEnvExtension *aeva;
	AudioEnvelope *aenv;
	int32 Result = 0;
	int32 IfRepost;

DBUG(("DSPPReleaseEnvAttachment ( attt=0x%lx )\n", aatt));

	aatt->aatt_ActivityLevel = AF_RELEASED;

	aeva = (AudioEnvExtension *) aatt->aatt_Extension;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;
	IfRepost = FALSE;

/* Jump to release if a release jump is specified. */
	if(aenv->aenv_ReleaseJump >= 0)
	{
DBUG(("DSPPReleaseEnvAttachment: Cur = %d, RJ = %d\n", aeva->aeva_CurIndex, aenv->aenv_ReleaseJump));

		if(aeva->aeva_CurIndex <= aenv->aenv_ReleaseJump)
		{
			aeva->aeva_CurIndex = aenv->aenv_ReleaseJump+1;
			IfRepost = TRUE;
		}
	}
	else if (aeva->aeva_Event.aevt_InList == NULL)
	{
/* Envelope is held at single Sustain point. */
		aeva->aeva_CurIndex++;
		IfRepost = TRUE;
	}

	if( IfRepost )
	{
		UnpostAudioEvent( (AudioEvent *) aeva );
		aeva->aeva_FramesLeft = 0;
		aeva->aeva_DeltaTime = aenv->aenv_Points[aeva->aeva_CurIndex-1].envs_Duration * aeva->aeva_TimeScale;
		Result = NextEnvSegment( aeva );
	}

	return Result;

}
/*****************************************************************/
int32 DSPPStopEnvAttachment( AudioAttachment *aatt )
{
	int32 Result;

DBUG(("DSPPStopEnvAttachment ( attt=0x%lx )\n", aatt));

	aatt->aatt_ActivityLevel = AF_STOPPED;
	Result = UnpostAudioEvent( (AudioEvent *) aatt->aatt_Extension );

	return Result;

}


/* -------------------- Find envelope resources */

static int32 dsppFindEnvHookPart (const DSPPTemplate *dtmp, const char *envHookName, const char *suffix, uint8 rsrcType);

/*****************************************************************/
/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppFindEnvHookResources
|||	Finds resources for an envelope hook.
|||
|||	  Synopsis
|||
|||	    Err dsppFindEnvHookResources (DSPPEnvHookRsrcInfo *info, uint32 infoSize,
|||	                                  const DSPPTemplate *dtmp,
|||	                                  const char *envHookName)
|||
|||	  Description
|||
|||	    Find the resource indeces of each constituent resource of a given envelope.
|||	    Only succeeds if all of the resources, with the correct types, are found.
|||
|||	  Arguments
|||
|||	    info
|||	        A Pointer to a DSPPEnvRsrcInfo structure where the information will be
|||	        stored.
|||
|||	    infoSize
|||	        The size in bytes of the DSPPEnvRsrcInfo structure.
|||
|||	    dtmp
|||	        DSPPTemplate to scan.
|||
|||	    envHookName
|||	        Name of envelope to look for. Pointer is assumed to be valid. Length is
|||	        checked against maximum (ENV_MAX_NAME_LENGTH).
|||
|||	  Return Value
|||
|||	    Returns 0 on success, error code on failure.
|||
|||	    Fills out *info, if successful with the resource indeces of each of the
|||	    constituent resources for the requested envelope. *info is left unchanged
|||	    on failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppFindResourceIndex()
**/
Err dsppFindEnvHookResources (DSPPEnvHookRsrcInfo *resultInfo, uint32 resultInfoSize, const DSPPTemplate *dtmp, const char *envHookName)
{
	DSPPEnvHookRsrcInfo info;
	int32 rsrcIndex;
	Err errcode;

		/* safety check envHookName */
		/* !!! maybe caller should be responsible for this test */
	if (strlen(envHookName) > ENV_MAX_NAME_LENGTH) return AF_ERR_NAME_TOO_LONG;

		/* init result (@@@ superfluous) */
	memset (&info, 0, sizeof info);

		/* look up each part */
	if ((errcode = rsrcIndex = dsppFindEnvHookPart (dtmp, envHookName, ENV_SUFFIX_REQUEST, ENV_DRSC_TYPE_REQUEST)) < 0) return errcode;
	info.deri_RequestRsrcIndex = rsrcIndex;

	if ((errcode = rsrcIndex = dsppFindEnvHookPart (dtmp, envHookName, ENV_SUFFIX_INCR,    ENV_DRSC_TYPE_INCR)) < 0) return errcode;
	info.deri_IncrRsrcIndex = rsrcIndex;

	if ((errcode = rsrcIndex = dsppFindEnvHookPart (dtmp, envHookName, ENV_SUFFIX_TARGET,  ENV_DRSC_TYPE_TARGET)) < 0) return errcode;
	info.deri_TargetRsrcIndex = rsrcIndex;

	if ((errcode = rsrcIndex = dsppFindEnvHookPart (dtmp, envHookName, ENV_SUFFIX_CURRENT, ENV_DRSC_TYPE_CURRENT)) < 0) return errcode;
	info.deri_CurrentRsrcIndex = rsrcIndex;

		/* success: copy to client's buffer */
	memset (resultInfo, 0, resultInfoSize);
	memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));
	return 0;
}

/*****************************************************************/
/*
	Find a constituent part of an envelope (called by dsppFindEnvHookResources())

	Inputs
		dtmp
			DSPPTemplate to scan

		envHookName
			Name of envelope hook (name w/o the suffix).
			Assumes that strlen(envHookName) <= ENV_MAX_NAME_LENGTH.

		suffix
			ENV_SUFFIX_ for envelope part

		rsrcType
			ENV_DRSC_TYPE_ of required resource type of part

	Results
		Resource index of envelope part on success, or error code on failure.
*/
static int32 dsppFindEnvHookPart (const DSPPTemplate *dtmp, const char *envHookName, const char *suffix, uint8 rsrcType)
{
	char envRsrcNameBuf[AF_MAX_NAME_SIZE];
	int32 rsrcIndex;

	strcpy (envRsrcNameBuf, envHookName);
	strcat (envRsrcNameBuf, suffix);

	if ((rsrcIndex = DSPPFindResourceIndex (dtmp, envRsrcNameBuf)) < 0) return rsrcIndex;
	if (dtmp->dtmp_Resources[rsrcIndex].drsc_Type != rsrcType) return AF_ERR_BADRSRCTYPE;

	return rsrcIndex;
}
