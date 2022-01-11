/* @(#) audio_instr.c 96/09/30 1.195 */
/* $Id: audio_instr.c,v 1.111 1995/03/16 19:00:11 peabody Exp phil $ */
/****************************************************************
**
** Audio Instruments
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/****************************************************************
** 00001 PLB 11/16/92 Fixed ref to uninitialized dins in Release and Stop
** 00002 PLB 12/01/92 Added LoadInstrument
** 930311 PLB UnloadInstrument deletes the Template
** 930319 PLB Don't StripInsTemplate if error in LoadInsTempExt
** 930415 PLB Track connections between Items
** 930516 PLB Add AbandonInstrument, AdoptInstrument
** 930824 PLB Free NAME data from NAME chunk in 3INS FORM, was leaking.
** 930824 PLB Do not error on unrecognised FORMs
** 930829 PLB Reject if AudioDevice != 0 to allow future use.
** 930830 PLB Fix error checks for Instrument creation.
** 930831 PLB Reject Tags for Stop and Release cuz none currently supported.
** 930904 PLB Check for no template passed to CreateAudioInstrument.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 940304 PLB DOn't SplitFileName of template name cuz it clobbers callers string.
** 940406 PLB Support TuneInsTemplate()
** 940606 WJB Added start time comparison to ScavengeInstrument().
** 940608 WJB Replaced start time comparison with CompareAudioTimes.
** 940608 PLB Check for no template in internalCreateAudioTemplate
** 940609 PLB Added shared library template support.
** 940726 WJB Removed iffParseImage() prototype.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 940812 PLB Allow AF_TAG_LEAVE_IN_PLACE cuz works now for samples.
** 940812 PLB Added AF_TAG_SPECIAL.  Support Half Calculation Rate.
** 940818 PLB Improved error cleanup in internalCreateAudioIns
** 940818 PLB Add calls to DSPPOpenSplitIns() and DSPPCloseSplitIns() for half rate.
** 940829 DAS/PLB Change MUD name to SysInfo
** 940922 PLB Add ains_ProbeList support.
** 940927 WJB Tweaked autodocs for Create/DeleteInstrument().
** 940930 WJB Reimplemented LoadInstrument() w/ code that handles failures better.
** 941011 WJB Removed GetNumInstruments() (unimplemented function).
** 941011 WJB Added some autodocs.
** 941012 WJB Added autodocs for Adopt/AbandonInstrument().
**            Syncronized swiAbandonInstrument() prototype in audio.h
** 941024 PLB Changed AF_TAG_CALCRATE_SHIFT to AF_TAG_CALCRATE_DIVIDE
** 941031 PLB Trap NULL reference in internalDeleteAudioIns
** 941116 PLB Add hack for Enabling Audio Input.  [Removed when SYSINFO used. 941206]
** 941121 PLB StopInstrument() if StartInstrument() called and it is still running.
** 941206 PLB Use SuperSetSysInfo() to enable audio input.
** 941215 PLB Nest enables and disables properly for multiple tasks.
** 950123 WJB Rolled DSPPValidateTemplate() and DSPPCloneTemplate() into dsppCreateSuperTemplate().
** 950124 WJB Replaced StripInsTemplate() with dsppStripTemplate().
** 950131 WJB Cleaned up includes.
** 950131 WJB Replaced dtmp_FunctionID with dtmp_Header.
** 950206 WJB Added dspp_resources.h.
** 950419 WJB Added prototypes for CustomAlloc/FreeMem() functions pointers.
** 950417 PLB Added support for tracking ConnectInstruments()
** 950428 WJB Fixed potentially undefined variable in internalDisconnectInstruments().
** 950516 WJB Added UnmonitorInstrumenTriggers() to internalDeleteAudioIns().
** 950525 WJB Replaced UnmonitorInstrumentTriggers() with DisarmAllInstrumentTriggers().
**            Added ResetAllInstrumentTriggers() to internalCreateAudioIns().
** 950628 PLB Changed ConnectInstruments() to ConnectInstrumentParts()
** 950712 WJB Added support for AF_TAG_PATCH_CMDS in CreateInsTemplate().
** 950718 WJB Added local CreatePatchTemplate() function.
** 950816 PLB Eliminate AF_TAG_EXTERNAL which was used to load oscupdownfp.dsp.
** 950911 PLB Use FindAndOpenNamedItem to dynamically load shared instrument templates.
** 950913 WJB Restructured LoadInsTemplate() and CreatePatchTemplate() to share common code.
**            Fixed race condition involving shared templates in LoadInsTemplate().
** 950914 WJB internalCreateAudioTemplate() now does FindAndOpen of shared template items.
**            Rewrote internalCreate/DeleteAudioTemplate() functions.
** 950915 WJB Now using TagProcessor() callback for internalCreateAudioTemplate().
** 950918 WJB Now uses aitp_DynamicLinkTemplates instead of names to manage allocation of shared code instruments.
** 950921 WJB Added usage of SuperInternalOpen/CloseItem() for template ownership change.
** 950922 WJB Added .dsp name trap to internalCreateAudioTemplate(): required for loadable
**            templates, illegal for mixers and patches.
** 950922 WJB Added privilege trap in internalCreateAudioTemplate().
** 950922 WJB Added DHDR_F_SHARED trap in Template ir_Load, ir_Open, and ir_Close methods.
** 950927 WJB Added CreateMixerTemplate().
** 950927 WJB Removed CreateInsTemplate().
** 951030 PLB Use Open/Close instead of ResourceReferences to track instrument usage.
** 951115 CMP Remove sysinfo code associated with HACK_INPUT_ENABLE.
** 951207 WJB Removed UnloadInsTemplate().
** 960108 WJB Added GetInstrumentResourceInfo().
** 960110 WJB Corrected overestimation of subroutine resource usage by GetInstrumentResourceInfo().
** 960115 WJB Implemented GetNumInstrumentPorts(), GetInstrumentPortInfo...()
** 960119 WJB Opens of subroutine instruments and templates are now owned by audio folio task.
** 960304 PLB Trap Priorities above 200 for user instruments.
** 960812 PLB Set Start Time correctly. CR6228
** 960823 WJB Moved template item management to audio_template.c and reorganized.
** 960823 WJB Restructured Create/DeleteInstrument() to share common cleanup.
****************************************************************/

#include "audio_internal.h"
#include "dspp.h"
#include "ezmem_tools.h"


/* -------------------- Debug */

#define DEBUG_Item              0   /* debug item creation and deletion */
#define DEBUG_ItemBrief         0   /* short form of DEBUG_Item (just create, delete, open, and close) */
#define DEBUG_PropagateAtts     0   /* debug PropagateTemplateAttachments() */
#define DEBUG_DLnkInstrument    0   /* debug Open/CloseDynamicLinkInstruments() */
#define LOG_CreateInstrument    0   /* log CreateInstrument() */
#define LOG_DeleteInstrument    0   /* log DeleteInstrument() */

#define DBUG(x)     /* PRT(x) */
#define DBUGCON(x)  /* PRT(x) */
#define PRTX(x)     /* PRT(x) */

#if DEBUG_Item
#define DBUGITEM(x)   PRT(x)
#define DBUGITEMBR(x) PRT(x)
#elif DEBUG_ItemBrief
#define DBUGITEM(x)
#define DBUGITEMBR(x) PRT(x)
#else
#define DBUGITEM(x)
#define DBUGITEMBR(x)
#endif

#if DEBUG_PropagateAtts
#define DBUGPTA(x) PRT(x)
#else
#define DBUGPTA(x)
#endif

#if DEBUG_DLnkInstrument
#define DBUGDLNKINS(x) PRT(x)
#else
#define DBUGDLNKINS(x)
#endif

#if LOG_CreateInstrument
#include <kernel/lumberjack.h>
#define LOGCREATEINS(x) LogEvent(x)
#else
#define LOGCREATEINS(x)
#endif

#if LOG_DeleteInstrument
#include <kernel/lumberjack.h>
#define LOGDELETEINS(x) LogEvent(x)
#else
#define LOGDELETEINS(x)
#endif

#if 0
#define REPORTMEM(msg) \
{ \
	PRT(( msg )); \
	PRT(("App===========")); \
	ReportMemoryUsage(); \
}
#else
#define REPORTMEM(msg) /* ReportMemoryUsage */
#endif


/* -------------------- Local Functions */

static Err OpenDynamicLinkInstruments (AudioInstrument *);
static void CloseDynamicLinkInstruments (AudioInstrument *);

static Err PropagateTemplateAttachments (Item instrument, const AudioInsTemplate *);


/* -------------------- Instrument Item Documentation */

/**
|||	AUTODOC -public -class Items -group Audio -name Instrument
|||	DSP Instrument Item.
|||
|||	  Description
|||
|||	    Instrument items are created from Instrument Template(@) items. They
|||	    represent actual instances of DSP code running on the DSP. Instrument
|||	    ports can be connected together. Instruments can be played (by starting
|||	    them), and controlled (by setting knobs). An Instrument is monophonic in the
|||	    sense that it corresponds to a single voice (not mono vs stereo). Playing
|||	    multiple voices requires creating an Instrument per voice.
|||
|||	    If the Instrument's Template has any Attachment(@)s when an Instrument is
|||	    created, then a matching set of Attachments to the Instrument is
|||	    automatically created. How each Attachment is created depends on the kind
|||	    of each slave Item attached to the Instrument Template:
|||
|||	    Envelope(@), Ordinary Sample(@), Delay Line (Sample(@))
|||	        A new Attachment is created between the new Instrument and the slave
|||	        Item attached to the Instrument Template. All Attachment parameters are
|||	        the copied from the Template's Attachment with the following exceptions:
|||	        AF_TAG_MASTER is replaced by the new Instrument, and
|||	        AF_TAG_AUTO_DELETE_SLAVE is set to FALSE.
|||
|||	    Delay Line Template (Sample(@))
|||	        A new Delay Line is created using the Delay Line Template's parameters
|||	        (e.g., length, channels, etc). A new Attachment is created between the
|||	        new Instrument and the new Delay Line, instead of the Delay Line
|||	        Template. All Attachment parameters are the copied from the Template's
|||	        Attachment with the following exceptions: AF_TAG_MASTER is replaced by
|||	        the new Instrument, AF_TAG_SLAVE is replaced by the new Delay Line, and
|||	        AF_TAG_AUTO_DELETE_SLAVE is set to TRUE.
|||
|||	    If the Instrument Template has a custom Tuning(@) (set by
|||	    TuneInsTemplate()), then the Tuning is automatically applied to the new
|||	    Instrument (as if TuneInstrument() had been called).
|||
|||	    New instruments are always created in the AF_STOPPED state, regardless of
|||	    the setting of AF_INSF_ABANDONED.
|||
|||	  Folio
|||
|||	    audio
|||
|||	  Item Type
|||
|||	    AUDIO_INSTRUMENT_NODE
|||
|||	  Create
|||
|||	    CreateInstrument(), CreateItem(), LoadInstrument()
|||
|||	  Delete
|||
|||	    DeleteInstrument(), DeleteItem(), UnloadInstrument()
|||
|||	  Query
|||
|||	    GetAudioItemInfo()
|||
|||	  Use
|||
|||	    AbandonInstrument(), BendInstrumentPitch(), ConnectInstrumentParts(),
|||	    CreateAttachment(), CreateKnob(), CreateProbe(),
|||	    DisconnectInstrumentParts(), PauseInstrument(), ReleaseInstrument(),
|||	    ResumeInstrument(), StartInstrument(), StopInstrument(), TuneInstrument()
|||
|||	  Tags
|||
|||	    General:
|||
|||	    AF_TAG_CALCRATE_DIVIDE (uint32) - Create, Query
|||	        Specifies the denominator of the fraction of the total DSP cycles on
|||	        which this instrument is to run. The valid settings for this are:
|||
|||	        1 - Full rate execution (44,100 cycles/sec)
|||
|||	        2 - Half rate (22,050 cycles/sec)
|||
|||	        8 - 1/8 rate (5,512.5 cycles/sec) (new for M2)
|||
|||	        Defaults to 1 on creation.
|||
|||	    AF_TAG_PRIORITY (uint8) - Create, Query
|||	        The priority of execution in DSP in the range of 0..200, where 200 is
|||	        the highest priority. Defaults to 100 on creation.
|||
|||	    AF_TAG_SET_FLAGS (uint32) - Create
|||	        AF_INSF_ flags to set at creation time. Defaults to all cleared.
|||
|||	    AF_TAG_START_TIME (AudioTime) - Query
|||	        Returns the AudioTime value of when the instrument was last started.
|||
|||	    AF_TAG_STATUS (uint32) - Query
|||	        Returns the current instrument status: AF_STARTED, AF_RELEASED,
|||	        AF_STOPPED, or AF_ABANDONED.
|||
|||	    AF_TAG_TEMPLATE (Item) - Create, Query
|||	        DSP Template Item used from which to create instrument.
|||
|||	    Amplitude:
|||
|||	    These tags apply to instruments that have an unconnected Amplitude knob.
|||	    They are mutually exclusive except for AF_TAG_EXP_VELOCITY_SCALAR. All
|||	    MIDI velocity tags also perform multi-sample selection based on Sample(@)
|||	    velocity ranges.
|||
|||	    AF_TAG_AMPLITUDE_FP (float32) - Start
|||	        Value to set instrument's Amplitude knob to before starting instrument
|||	        (for instruments that have an Amplitude knob). Valid range -1.0..1.0.
|||
|||	    AF_TAG_VELOCITY (uint8) - Start
|||	        Linear MIDI key velocity to amplitude mapping:
|||
|||	        Amplitude = Velocity / 127.0
|||
|||	        MIDI key velocity is specified in the range of 0..127.
|||
|||	    AF_TAG_SQUARE_VELOCITY (uint8) - Start
|||	        Like AF_TAG_VELOCITY except:
|||
|||	        Amplitude = (Velocity/127.0)**2.
|||
|||	        This gives a more natural response curve than the linear version.
|||
|||	    AF_TAG_EXPONENTIAL_VELOCITY (uint8) - Start
|||	        Like AF_TAG_VELOCITY except:
|||
|||	        Amplitude = 2.0**((Velocity-127)*expVelocityScalar).
|||
|||	        where expVelocityScalar is set using AF_TAG_EXP_VELOCITY_SCALAR.
|||	        This gives a more natural response curve than the linear version.
|||
|||	    AF_TAG_EXP_VELOCITY_SCALAR (float32) - Start
|||	        Sets expVelocityScalar used by AF_TAG_EXPONENTIAL_VELOCITY. These two
|||	        tags are used together with StartInstrument() and can be in either
|||	        order. The default value is (1.0/20.0), which means that the Amplitude
|||	        will double for every 20 units of velocity.
|||
|||	    Frequency:
|||
|||	    These tags apply to instruments that support frequency settings (e.g.,
|||	    oscillators, LFOs, sample players). They have no effect if the Frequency or
|||	    SampleRate knob is connected to another instrument. They are mutually
|||	    exclusive.
|||
|||	    AF_TAG_FREQUENCY_FP (float32) - Start
|||	        Play instrument at a specific output frequency.
|||
|||	        Oscillator or LFO: Sets the Frequency knob to the specified value.
|||
|||	        Sample player with a tuned sample: Adjusts the SampleRate knob to play
|||	        the sample at the desired frequency. For example, to play sinewave.aiff
|||	        at 440Hz, set this tag to 440.
|||
|||	    AF_TAG_PITCH (uint8) - Start
|||	        MIDI note number of the pitch to play instrument at. The range is 0 to
|||	        127; 60 is middle C. Once the output frequency is determined applies the
|||	        frequency in the same manner as AF_TAG_FREQUENCY_FP.
|||
|||	        This tag also performs multi-sample selection based on Sample(@) note
|||	        ranges, and Envelope(@) pitch-based time scaling.
|||
|||	    AF_TAG_RATE_FP (float32) - Start
|||	        Set raw frequency control. For samplers, this is a fraction of DAC rate.
|||	        For oscillators, this is a fractional phase increment.
|||
|||	    AF_TAG_SAMPLE_RATE_FP (float32) - Start
|||	        Sample rate in Hz to set sample player's SampleRate knob to.
|||
|||	    AF_TAG_DETUNE_FP (float32) - Start
|||	        For samplers, play at a fraction of original recorded sample rate.
|||	        For oscillators or LFOs, play at a fraction of default frequency.
|||
|||	    Time Scaling:
|||
|||	    AF_TAG_TIME_SCALE_FP (float32) - Start, Release
|||	        Scales times for all Envelope(@)s attached to this Instrument which do
|||	        not have the AF_ENVF_LOCKTIMESCALE flag set. Defaults to 1.0 when
|||	        instrument is started. Defaults to start setting when released.
|||
|||	  Flags
|||
|||	    AF_INSF_AUTOABANDON
|||	        When set, causes instrument to automatically go to the AF_ABANDONED
|||	        state when stopped either automatically because of an
|||	        AF_ATTF_FATLADYSINGS flag, or manually because of a StopInstrument()
|||	        call). Otherwise, the instrument goes to the AF_STOPPED state when
|||	        stopped. Note that regardless of the state of this flag, instruments are
|||	        created in the AF_STOPPED state.
|||
|||	  See Also
|||
|||	    Template(@), Attachment(@), Knob(@), Probe(@)
**/

/**
|||	AUTODOC -private -class Items -group Audio -name Instrument-private
|||	Private Instrument(@) tags.
|||
|||	  Folio
|||
|||	    audio
|||
|||	  Item Type
|||
|||	    AUDIO_INSTRUMENT_NODE
|||
|||	  Tags
|||
|||	    AF_TAG_SPECIAL (int32) - Create
|||	        AF_SPECIAL_ value. Defaults to AF_SPECIAL_NOT (i.e., not special).
|||
|||	  See Also
|||
|||	    Instrument(@)
**/


/* -------------------- Load/UnloadInstrument() */

/**************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name LoadInstrument
|||	Loads a DSP instrument Template(@) and creates an Instrument(@) from it in one
|||	call.
|||
|||	  Synopsis
|||
|||	    Item LoadInstrument (const char *name, uint8 calcRateDivider,
|||	                         uint8 priority)
|||
|||	  Description
|||
|||	    This functions combines the actions of LoadInsTemplate() and
|||	    CreateInstrument(), and returns the resulting Instrument item.
|||
|||	    Call UnloadInstrument() (not DeleteInstrument()) to free an Instrument
|||	    created by this function. Calling DeleteInstrument() deletes the
|||	    Instrument, but not the Template, leaving you with an unaccessible Template
|||	    Item that you can't delete.
|||
|||	  Arguments
|||
|||	    name
|||	        Name of the file containing the instrument template.
|||
|||	    calcRateDivider
|||	        Specifies the denominator of the fraction of the total DSP cycles on
|||	        which this instrument is to run. The valid settings for this are:
|||
|||	        1 - Full rate execution (44,100 cycles/sec)
|||
|||	        2 - Half rate (22,050 cycles/sec)
|||
|||	        8 - 1/8 rate (5,512.5 cycles/sec) (new for M2)
|||
|||	        Zero is treated as one.
|||
|||	    priority
|||	        Determines order of execution in DSP. Set from 0 to 200. A typical
|||	        value would be 100. This also determines the priority over other
|||	        instruments when voices are stolen for dynamic voice allocation.
|||
|||	  Return Value
|||
|||	    The procedure returns an Instrument item number (a positive value) if
|||	    successful or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in audio folio V20.
|||
|||	  Caveats
|||
|||	    This function creates a separate Template Item for each Instrument it
|||	    creates. If you intend to create multiple Instruments of the same type,
|||	    it is more efficient to use LoadInsTemplate() to create the Template
|||	    once and multiple calls to CreateInstrument().
|||
|||	    This function can only be used to load standard DSP instrument templates;
|||	    it can't load patch files or create mixers.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    UnloadInstrument(), LoadInsTemplate(), CreateInstrument()
**/
/* -LoadInstrument() */
Item LoadInstrument (const char *insName, uint8 calcRateDivider, uint8 priority)
{
	Item insTemplate;
	Item instrument;
	Err errcode;

	if ( (errcode = insTemplate = LoadInsTemplate (insName, NULL)) < 0 ) goto clean;
	if ( (errcode = instrument = CreateItemVA( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
		AF_TAG_TEMPLATE,    insTemplate,
		AF_TAG_CALCRATE_DIVIDE, (calcRateDivider == 0) ? 1 : calcRateDivider,
		AF_TAG_PRIORITY,    priority,
		TAG_END )) < 0 ) goto clean;

	DBUG(("LoadInstrument: returns 0x%x\n", instrument));
	return instrument;

clean:
	UnloadInsTemplate (insTemplate);
	return errcode;
}

/**************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name UnloadInstrument
|||	Unloads an instrument loaded with LoadInstrument().
|||
|||	  Synopsis
|||
|||	    Err UnloadInstrument (Item instrument)
|||
|||	  Description
|||
|||	    This procedure frees the Instrument and unloads the Template loaded by
|||	    LoadInstrument(). This has the same side effects as DeleteInstrument() and
|||	    UnloadInsTemplate().
|||
|||	    Do not confuse this function with DeleteInstrument(), which deletes an
|||	    Instrument created by CreateInstrument(). Calling DeleteInstrument() for an
|||	    instrument created by LoadInstrument() deletes the Instrument, but not the
|||	    Template, leaving you with an unaccessible Template Item that you can't
|||	    delete. Calling UnloadInstrument() for and Instrument created by
|||	    CreateInstrument() deletes Template for that Instrument along with all
|||	    other Instruments created from that Template.
|||
|||	  Arguments
|||
|||	    instrument
|||	        Item number of the instrument.
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
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    LoadInstrument(), DeleteInstrument(), UnloadInsTemplate()
**/
/* !!! might be able to mark the Instrument structure in LoadInstrument() to
	   cause deleting it to delete it's template as well */
int32 UnloadInstrument ( Item InstrumentItem  )
{
	int32 Result;
	AudioInstrument *ains;
	AudioInsTemplate *aitp;

DBUG(("UnloadInstrument( 0x%x )\n", InstrumentItem));

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;

	aitp = ains->ains_Template;

	Result = DeleteItem ( InstrumentItem );
	if (Result < 0) goto done;
	Result = UnloadInsTemplate ( aitp->aitp_Item.n_Item );
done:
DBUG(("UnloadInstrument: returns 0x%x\n", Result));
	return Result;
}


/* -------------------- Instrument Item Management */

static void StripAudioInstrument (AudioInstrument *ains);
static void DisconnectRemaining (Item insItem);

/*************************************************************************/
/* AUDIO_INSTRUMENT_NODE ir_Create method */
Item internalCreateAudioIns (AudioInstrument *ains, const TagArg *tagList)
{
	AudioInsTemplate *aitp;
	DSPPTemplate *dtmp;
	uint8 calcRateShift = 0;
	uint8 specialness = AF_SPECIAL_NOT;
	Err errcode;

#if DEBUG_Item
	PRTDBUGINFO(); PRT(("\n  "));
	DBUGITEM(("internalCreateAudioIns(0x%lx) is 0x%x\n", ains, ains->ains_Item.n_Item ));
	DBUGITEM(("  stackbase=0x%08x frame near 0x%08x\n", CURRENTTASK->t_SuperStackBase, &aitp));
#endif

#if LOG_CreateInstrument
	{
		char b[80];

		sprintf (b, "  internalCreateAudioIns 0x%x", ains->ains_Item.n_Item);
		LogEvent (b);
	}
#endif

	REPORTMEM(("Entered internalCreateAudioIns ----------------\n"));

		/* Init non-zero fields */
	ains->ains_Status = AF_STOPPED;
	ains->ains_Bend   = 1.0;
	PrepList (&ains->ains_KnobList);
	PrepList (&ains->ains_ProbeList);

		/* Process tags */
	{
		Item templateItem = 0;
		uint32 priority = 100;

		if ((errcode = TagProcessor (ains, tagList, afi_DummyProcessor, NULL)) < 0) {
			ERR(("internalCreateAudioIns: TagProcessor failed.\n"));
			goto clean;
		}

		REPORTMEM(("After TagProcessor\n"));

		{
			const TagArg *tstate, *t;

			for (tstate = tagList; t = NextTagArg (&tstate); ) {
				DBUGITEM(("internalCreateAudioIns: tag { %d, 0x%x }\n", t->ta_Tag, t->ta_Arg));

				switch (t->ta_Tag) {
					case AF_TAG_TEMPLATE:
						templateItem = (Item) t->ta_Arg;
						break;

					case AF_TAG_CALCRATE_DIVIDE:
						{
							const int32 calcRateDivide = (int32)t->ta_Arg;

								/* Convert divisor to shift */
							switch (calcRateDivide) {
								case 1:
									calcRateShift = 0;
									break;

								case 2:
									calcRateShift = 1;
									break;

								case 8:
									calcRateShift = 3;
									break;

								default:
									ERR(("internalCreateAudioIns: AF_TAG_CALCRATE_DIVIDE value (%d) out of range\n", calcRateDivide));
									errcode = AF_ERR_BADTAGVAL;
									goto clean;
							}
						}
						break;

					case AF_TAG_SPECIAL:
						{
							const uint32 tSpecialness = (uint32)t->ta_Arg;

								/* reject this tag if caller isn't privileged */
							if (!IsPriv(CURRENTTASK)) {
								ERR(("internalCreateAudioIns: AF_TAG_SPECIAL may only be used by priveleged tasks\n"));
								errcode = AF_ERR_SECURITY;
								goto clean;
							}

							if (tSpecialness > AF_SPECIAL_MAX) {
								ERR(("internalCreateAudioIns: AF_TAG_SPECIAL value (%d) out of range\n", tSpecialness));
								errcode = AF_ERR_BADTAGVAL;
								goto clean;
							}
							specialness = tSpecialness;
						}
						break;

					case AF_TAG_PRIORITY:
						priority = (uint32) t->ta_Arg;      /* validated later */
						break;

					case AF_TAG_SET_FLAGS:
						{
							const uint32 setFlags = (uint32) t->ta_Arg;

							if (setFlags & ~AF_INSF_LEGALFLAGS) {
								ERR(("internalCreateAudioIns: Illegal instrument flags (0x%x)\n", setFlags));
								errcode = AF_ERR_BADTAGVAL;
								goto clean;
							}
							ains->ains_Flags = setFlags;
						}
						break;

					default:
						if(t->ta_Tag > TAG_ITEM_LAST)
						{
							ERR(("internalCreateAudioIns: Unrecognized tag { %d, 0x%x }\n", t->ta_Tag, t->ta_Arg));
							errcode = AF_ERR_BADTAG;
							goto clean;
						}
						break;
				}
			}
		}

			/* Validate template */
		DBUGITEM(("internalCreateAudioIns: Template = 0x%x, Ins = 0x%x\n", templateItem, ains->ains_Item.n_Item));
		aitp = (AudioInsTemplate *)CheckItem(templateItem, AUDIONODE, AUDIO_TEMPLATE_NODE);
		if (aitp == NULL)
		{
			ERR(("internalCreateAudioIns: Item 0x%x is not a Template\n", templateItem));
			errcode = AF_ERR_BADITEM;  /* 930830 */
			goto clean;
		}
		dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;
		DBUGITEM(("internalCreateAudioIns: Template Name = %s\n", aitp->aitp_Item.n_Name));

			/* Check to make sure that this template can be made into an instrument. */
		if (dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PATCHES_ONLY) {
			ERR(("internalCreateAudioIns: Template 0x%x ('%s') may be used only in a patch template\n", templateItem, aitp->aitp_Item.n_Name));
			errcode = AF_ERR_PATCHES_ONLY;
			goto clean;
		}

			/* Set template parameters */
		ains->ains_Template = aitp; /* 930311 */
		ains->ains_Tuning   = aitp->aitp_Tuning;        /* !!! should aitp_Tuning be validated first? */

			/* Validate priority. Only allow special instruments to exceed priority 200. */
		if (priority > ((specialness != AF_SPECIAL_NOT) ? 255 : 200)) /* 960304 */
		{
			ERR(("internalCreateAudioIns: Instrument priority (%u) out of range\n", priority));
			errcode = AF_ERR_BADTAGVAL;
			goto clean;
		}
		ains->ains_Item.n_Priority = (uint8)priority;
	}

	DBUGITEMBR(("internalCreateAudioIns: tmpl=0x%x '%s' ins=0x%x\n", ains->ains_Template->aitp_Item.n_Item, ains->ains_Template->aitp_Item.n_Name, ains->ains_Item.n_Item));

		/* Scan to see if the library templates have at least one instrument allocated. */
		/*
		**  !!! a non-recursive approach to this might be safer:
		**      . walk down tree of library instruments required by this instrument
		**        (possibly w/ a recursive function to construct a list, at least it
		**        wouldn't be recursive trips thru CreateItem())
		**      . alloc from the list
		**      . don't call this function when allocating a library template, require that
		**        the caller of such a library has done the above work
		*/
	DBUGITEM(("internalCreateAudioIns: OpenDynamicLinkInstruments()\n"));
	LOGCREATEINS("   OpenDynamicLinkInstruments");
	if ((errcode = OpenDynamicLinkInstruments (ains)) < 0) goto clean;

	REPORTMEM(("After OpenDynamicLinkInstruments\n"));

		/* Alloc/Init DSPPInstrument */
	{
		DSPPInstrument *dins;

		LOGCREATEINS("   DSPPAllocInstrument");
		if ((errcode = DSPPAllocInstrument (dtmp, &dins, calcRateShift)) < 0) goto clean;
		REPORTMEM(("After DSPPAllocInstrument\n"));
		dins->dins_Node.n_Priority  = ains->ains_Item.n_Priority;
		dins->dins_Specialness      = specialness;

		ains->ains_DeviceInstrument = dins;
	}


	/* Instrument Item is now complete - can pass it to functions that expect complete instruments */

		/* Propagate any attachments in the template to the new instrument */
	LOGCREATEINS("   PropagateTemplateAttachments");
	if ((errcode = PropagateTemplateAttachments (ains->ains_Item.n_Item, aitp)) < 0) goto clean;

	REPORTMEM(("After PropagateTemplateAttachments\n"));


	/* success: do last few things */

		/* Connect Instrument to Template's List */
	AddTail( &aitp->aitp_InstrumentList, (Node *) ains );

		/* Reset all of this instrument's triggers (if any) */
	ResetAllInstrumentTriggers (ains);

	REPORTMEM(("Leaving internalCreateAudioIns\n"));
	DBUGITEM(("internalCreateAudioIns: returns 0x%x\n", ains->ains_Item.n_Item ));
#if LOG_CreateInstrument
	{
		char b[80];

		sprintf (b, "  internalCreateAudioIns 0x%x done", ains->ains_Item.n_Item);
		LogEvent (b);
	}
#endif
	return ains->ains_Item.n_Item;

clean:
	StripAudioInstrument (ains);
	return errcode;
}


/******************************************************************/
/* AUDIO_INSTRUMENT_NODE ir_Delete method */
int32 internalDeleteAudioIns (AudioInstrument *ains)
{
#if LOG_DeleteInstrument
	{
		char b[80];

		sprintf (b, " internalDeleteAudioIns 0x%x", ains->ains_Item.n_Item);
		LogEvent (b);
	}
#endif

#if DEBUG_Item
	PRTDBUGINFO(); PRT(("\n  "));
#endif
	DBUGITEMBR(("internalDeleteAudioIns: tmpl=0x%x '%s' ins=0x%x\n", ains->ains_Template->aitp_Item.n_Item, ains->ains_Template->aitp_Item.n_Name, ains->ains_Item.n_Item));

	REPORTMEM(("Entering internalDeleteAudioIns ------------- \n"));

		/* Disconnect any connections that were made. */
	DisconnectRemaining (ains->ains_Item.n_Item);

		/* Delete any knobs and probes belonging to the instrument. */
	afi_DeleteLinkedItems (&ains->ains_KnobList);
	afi_DeleteLinkedItems (&ains->ains_ProbeList);

		/* Disarm all of this instrument's triggers (if any) */
	DisarmAllInstrumentTriggers (ains);

		/* Remove from template's instrument list */
		/* (use RemoveAndMarkNode() because this list can be cleared with afi_DeleteLinkedItems()) */
	RemoveAndMarkNode ((Node *)ains);

		/* Clean up the rest of the instrument */
	StripAudioInstrument (ains);

#if LOG_DeleteInstrument
	{
		char b[80];

		sprintf (b, " internalDeleteAudioIns 0x%x done", ains->ains_Item.n_Item);
		LogEvent (b);
	}
#endif
	REPORTMEM(("Leaving internalDeleteAudioIns\n"));
	DBUGITEM(("Leaving internalDeleteAudioIns\n"));

	return 0;
}

/******************************************************************/
/* Scan instrument connection list and break any connections involving this instrument. */
static void DisconnectRemaining (Item insItem)
{
	AudioConnectionNode *acnd, *succ;
	Err errcode;

	DBUGCON(("DisconnectRemaining: InsItem = 0x%x\n", insItem));

	PROCESSLIST (&AB_FIELD(af_ConnectionList), acnd, succ, AudioConnectionNode) {
		DBUGCON(("DisconnectRemaining: acnd = 0x%08x\n", acnd ));
		DBUGCON(("DisconnectRemaining: test Src = 0x%x %s, Dst = 0x%x %s\n",
			acnd->acnd_SrcIns, acnd->acnd_SrcName,
			acnd->acnd_DstIns, acnd->acnd_DstName ));

		if (acnd->acnd_SrcIns == insItem || acnd->acnd_DstIns == insItem) {

				/* !!! Might be better to have a version of disconnect
				**     which takes acnd as an arg. it would have to do a bit less
				**     validation, and would avoid another search of the connection list. */
			errcode = swiDisconnectInstrumentParts (acnd->acnd_DstIns, acnd->acnd_DstName, acnd->acnd_DstPart);

		  #ifdef BUILD_STRINGS
			if (errcode < 0) {
				const AudioInstrument * const ains = LookupItem (insItem);

				ERR(("DisconnectRemaining: Unable to break instrument connection (ins=0x%x tmpl=0x%x '%s')\n",
					ains->ains_Item.n_Item, ains->ains_Template->aitp_Item.n_Item, ains->ains_Template->aitp_Item.n_Name));
				PrintfSysErr (errcode);
			}
		  #else
			TOUCH(errcode);
		  #endif
		}
	}
}


/*
	Common cleanup code for partial success of internalCreateAudioIns() and
	internalDeleteAudioIns().

	Arguments
		ains
			Legit pointer to an AudioInstrument. It has at least been
			initialized, but not necessarily complete. In particular,
			ains_Template may be valid or NULL.

			It may have attachments and open dynamic links. It is expected to
			not have knobs, probes, connections, or armed triggers.
*/
static void StripAudioInstrument (AudioInstrument *ains)
{
	DBUGITEM(("StripAudioInstrument: tmpl=0x%x '%s' ins=0x%x\n", ains->ains_Template ? ains->ains_Template->aitp_Item.n_Item : 0, ains->ains_Template ? ains->ains_Template->aitp_Item.n_Name : NULL, ains->ains_Item.n_Item));

		/* Delete DSPPInstrument and delete attachments */
	LOGDELETEINS(" dsppFreeInstrument");
	dsppFreeInstrument ((DSPPInstrument *)ains->ains_DeviceInstrument);

		/* Close dynamic library instruments. 940609 */
	LOGDELETEINS(" CloseDynamicLinkInstruments");
	CloseDynamicLinkInstruments (ains);
}


/*****************************************************************************
** AUDIO_INSTRUMENT_NODE ir_Open method. Called by internalOpenAudioItem()
**
** Permit opening an instrument only if the template has DHDR_F_SHARED set
** (which marks a library template)
*/
Item internalOpenInstrument( AudioInstrument *ains )
{
	const AudioInsTemplate * const aitp = ains->ains_Template;
	const DSPPTemplate * const dtmp = (DSPPTemplate *) aitp->aitp_DeviceTemplate;

  #if DEBUG_Item || DEBUG_DLnkInstrument
	PRTDBUGINFO(); PRT(("\n  "));
  #endif
  #if DEBUG_Item || DEBUG_ItemBrief || DEBUG_DLnkInstrument
	PRT(("internalOpenInstrument: tmpl=0x%x '%s' ins=0x%x open=%d dhdr_Flags=0x%x\n",
		ains->ains_Template->aitp_Item.n_Item, ains->ains_Template->aitp_Item.n_Name, ains->ains_Item.n_Item,
		ains->ains_Item.n_OpenCount, dtmp->dtmp_Header.dhdr_Flags));
  #endif

	return ((dtmp->dtmp_Header.dhdr_Flags & DHDR_F_SHARED)
		? ains->ains_Item.n_Item
		: AF_ERR_BADITEM);
}

/*****************************************************************************
** AUDIO_INSTRUMENT_NODE ir_Close method. Called by internalCloseAudioItem()
**
** Immediately expunge library template when usage count falls to 0.
*/
Err internalCloseInstrument( AudioInstrument *ains )
{
	const AudioInsTemplate * const aitp = ains->ains_Template;
	const DSPPTemplate * const dtmp = (DSPPTemplate *) aitp->aitp_DeviceTemplate;

  #if DEBUG_Item || DEBUG_DLnkInstrument
	PRTDBUGINFO(); PRT(("\n  "));
  #endif
  #if DEBUG_Item || DEBUG_ItemBrief || DEBUG_DLnkInstrument
	PRT(("internalCloseInstrument: tmpl=0x%x '%s' ins=0x%x open=%d dhdr_Flags=0x%x\n",
		ains->ains_Template->aitp_Item.n_Item, ains->ains_Template->aitp_Item.n_Name, ains->ains_Item.n_Item,
		ains->ains_Item.n_OpenCount, dtmp->dtmp_Header.dhdr_Flags));
  #endif

	if ((dtmp->dtmp_Header.dhdr_Flags & DHDR_F_SHARED) && (ains->ains_Item.n_OpenCount == 0))
	{
		return SuperInternalDeleteItem (ains->ains_Item.n_Item);
	}

	return 0;
}


/*****************************************************************/
/* AUDIO_INSTRUMENT_NODE GetAudioItemInfo() method */
Err internalGetInstrumentInfo (const AudioInstrument *ains, TagArg *tagList)
{
    const DSPPInstrument * const dins = ains->ains_DeviceInstrument;
	TagArg *tag;
	int32 tagResult;

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUG(("internalGetInstrumentInfo: %d\n", tag->ta_Tag));

		switch (tag->ta_Tag) {
			case AF_TAG_CALCRATE_DIVIDE:
				tag->ta_Arg = (TagData)((uint32)1 << dins->dins_RateShift);
				break;

			case AF_TAG_PRIORITY:
				tag->ta_Arg = (TagData)ains->ains_Item.n_Priority;
				break;

			case AF_TAG_STATUS:
				tag->ta_Arg = (TagData)ains->ains_Status;
				break;

			case AF_TAG_START_TIME:
				tag->ta_Arg = (TagData)ains->ains_StartTime;
				break;

			case AF_TAG_TEMPLATE:
				tag->ta_Arg = (TagData)ains->ains_Template->aitp_Item.n_Item;
				break;

			default:
				ERR (("internalGetInstrumentInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if (tagResult < 0) {
		ERR(("internalGetInstrumentInfo: Error processing tag list 0x%x\n", tagList));
		return tagResult;
	}

	return 0;
}


/* -------------------- Dynamic Link Instrument Management */

static Err OpenDynamicLinkInstrument (Item dlnktmpl);
static void CloseDynamicLinkInstrument (Item dlnktmpl);
static Item FindDynamicLinkInstrument (Item dlnktmpl);

/*
	Create if non-existent and open instruments for each of the dynamic link
	templates.

	Arguments
		ains
			AudioInstrument to bind to dynamic link instruments. Uses only
			these fields:
				ains_Item.n_Flags
				ains_Template

	Results
		0 on success, Err code on failure

		Sets AF_INS_NODE_F_DLNK_OPENED in ains_Item.n_Flags if dynamic links
		opened successfully. Not set if open failed or there were none to open
		(the latter speeds up DeleteInstrument() ever so slightly)

	Notes
		@@@ The AudioInstrument passed to this function is partially built. Only
			the fields described in Arguments are necessary.
*/
static Err OpenDynamicLinkInstruments (AudioInstrument *ains)
{
	const Item * const dynamicLinkTemplates = ains->ains_Template->aitp_DynamicLinkTemplates;
	Err errcode;

	if (dynamicLinkTemplates) {
		const Item *dlnktmpl;

		DBUGDLNKINS(("OpenDynamicLinkInstruments: ins=0x%x tmpl=0x%x\n", ains->ains_Item.n_Item, ains->ains_Template->aitp_Item.n_Item));

			/* open an instrument for each of the dynamic link templates */
		for (dlnktmpl = dynamicLinkTemplates; *dlnktmpl > 0; dlnktmpl++) {
			if ((errcode = OpenDynamicLinkInstrument (*dlnktmpl)) < 0) {

					/* on failure close all the things we tried to open */
				for (; --dlnktmpl >= dynamicLinkTemplates; ) {
					CloseDynamicLinkInstrument (*dlnktmpl);
				}
				return errcode;
			}
		}

			/* note that dynamic links opened successfully (used when deleting) */
		ains->ains_Item.n_Flags |= AF_INS_NODE_F_DLNK_OPENED;
	}

	return 0;
}

/*
	Find and open dynamic link instrument. Creates instrument if not already
	in existence.

	Arguments
		dlnktml
			Item number of dynamic link template whose instrument is to be
			opened. Validated by this function.

	Results
		0 on success, Err code on failure.
*/
static Err OpenDynamicLinkInstrument (Item dlnktmpl)
{
	bool createdIns = FALSE;
	Item dlnkins;
	Err errcode;

#if DEBUG_DLnkInstrument
	{
		const AudioInsTemplate * const dlnkaitp = LookupItem (dlnktmpl);

		DBUGDLNKINS(("OpenDynamicLinkInstrument: tmpl=0x%x '%s'\n", dlnktmpl, dlnkaitp ? dlnkaitp->aitp_Item.n_Name : NULL));
	}
#endif

		/* Find dynamic link instrument */
	if ((errcode = dlnkins = FindDynamicLinkInstrument (dlnktmpl)) < 0) goto clean;

		/* If not found, create new dynamic link instrument */
	if (!dlnkins) {
		DBUGDLNKINS(("OpenDynamicLinkInstrument: creating new ins from template 0x%x\n", dlnktmpl));

			/* Create it */
		if ((errcode = dlnkins = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
			AF_TAG_TEMPLATE, dlnktmpl,
			AF_TAG_PRIORITY, 0,
			TAG_END)) < 0) goto clean;
		createdIns = TRUE;

			/* Transfer ownership to audio daemon */
		if ((errcode = SuperSetItemOwner (dlnkins, AB_FIELD(af_AudioDaemonItem))) < 0) goto clean;
	}

		/* Open dynamic link instrument giving ownership of the "openness" to the audio daemon */
	if ((errcode = SuperInternalOpenItem (dlnkins, NULL, AB_FIELD(af_AudioDaemon))) < 0) goto clean;

#if DEBUG_DLnkInstrument
	{
		const AudioInsTemplate * const dlnkaitp = LookupItem (dlnktmpl);
		const AudioInstrument * const dlnkains = LookupItem (dlnkins);

		DBUGDLNKINS(("OpenDynamicLinkInstrument: tmpl=0x%x '%s' ins=0x%x OpenCount=%d\n",
			dlnktmpl, dlnkaitp ? dlnkaitp->aitp_Item.n_Name : NULL,
			dlnkins, dlnkains ? dlnkains->ains_Item.n_OpenCount : 0));
	}
#endif

	return 0;

clean:
	/* @@@ assumes that if we got here, the instrument wasn't opened. no need to close it */
	if (createdIns) SuperInternalDeleteItem (dlnkins);
	return errcode;
}


/*
	Close (and delete if usage count drops to 0) dynamic link instruments
	opened by OpenDynamicLinkInstruments().

	Arguments
		ains
			AudioInstrument to unbind from dynamic link instruments. Uses only
			these fields:
				ains_Item.n_Flags
				ains_Template (only if AF_INS_NODE_F_DLNK_OPENED is set)

	Results
		Clears AF_INS_NODE_F_DLNK_OPENED in ains_Item.n_Flags.
*/
static void CloseDynamicLinkInstruments (AudioInstrument *ains)
{
	if (ains->ains_Item.n_Flags & AF_INS_NODE_F_DLNK_OPENED) {
		const Item *dlnktmpl;

		DBUGDLNKINS(("CloseDynamicLinkInstruments: ins=0x%x tmpl=0x%x\n", ains->ains_Item.n_Item, ains->ains_Template->aitp_Item.n_Item));

			/* close instrument for each of the dynamic link templates */
			/* @@@ no need to make sure that dlnktmpl!=NULL, because that is a
			**     necessary condition for AF_INS_NODE_F_DLNK_OPENED to have been set. */
		for (dlnktmpl = ains->ains_Template->aitp_DynamicLinkTemplates; *dlnktmpl > 0; dlnktmpl++) {
			CloseDynamicLinkInstrument (*dlnktmpl);
		}

		ains->ains_Item.n_Flags &= ~AF_INS_NODE_F_DLNK_OPENED;
	}
}

/*
	Close dynamic link instrument. Also deletes it when the open count drops to 0.

	Arguments
		dlnktml
			Item number of dynamic link template whose instrument is to be
			closed. Validated by this function.

	Results
		0 on success, Err code on failure.
*/
static void CloseDynamicLinkInstrument (Item dlnktmpl)
{
	Item dlnkins;

		/* Find dynamic link instrument (0 means no instrument was found) */
	if ((dlnkins = FindDynamicLinkInstrument (dlnktmpl)) <= 0) {
	#ifdef BUILD_STRINGS
		ERR(("CloseDynamicLinkInstrument: Unable to find dynamic link instrument. Got 0x%x\n", dlnkins));
		if (dlnkins < 0) PrintfSysErr (dlnkins);
	#endif
		return;     /* don't return an error from here */
	}

#if DEBUG_DLnkInstrument
	{
		const AudioInsTemplate * const dlnkaitp = LookupItem (dlnktmpl);
		const AudioInstrument * const dlnkains = LookupItem (dlnkins);

		DBUGDLNKINS(("CloseDynamicLinkInstrument: tmpl=0x%x '%s' ins=0x%x OpenCount=%d\n",
			dlnktmpl, dlnkaitp ? dlnkaitp->aitp_Item.n_Name : NULL,
			dlnkins, dlnkains ? dlnkains->ains_Item.n_OpenCount : 0));
	}
#endif

		/* Close it (and delete it when open count drops to 0) */
	SuperInternalCloseItem (dlnkins, AB_FIELD(af_AudioDaemon));
}

/*
	Find the one and only Instrument belonging to dynamic link template.

	Arguments
		dlnktml
			Item number of dynamic link template. Validated by this function.

	Results
		> 0
			Item number of existing dynamic link instrument.

		0
			No dynamic link instrument exists yet.

		< 0
			Error code (e.g., dlnktmpl was bad)
*/
static Item FindDynamicLinkInstrument (Item dlnktmpl)
{
	const AudioInsTemplate *dlnkaitp;
	const AudioInstrument *dlnkains;

		/* Lookup dynamic link template */
	if (!(dlnkaitp = (AudioInsTemplate *)CheckItem (dlnktmpl, AUDIONODE, AUDIO_TEMPLATE_NODE))) {
		ERR(("FindDynamicLinkInstrument: invalid library template 0x%x\n", dlnktmpl));
		return AF_ERR_BADITEM;
	}

		/* If dynamic link instrument already exists, return its item number. Otherwise return 0. */
	dlnkains = (AudioInstrument *)FirstNode (&dlnkaitp->aitp_InstrumentList);
	return IsNode (&dlnkaitp->aitp_InstrumentList, dlnkains)
		? dlnkains->ains_Item.n_Item
		: 0;
}


/* -------------------- Propagate template attachments to instrument */

typedef struct DelayLineTemplatePromotion {
	MinNode dltp_Node;
	Item    dltp_DelayLineTemplate;     /* DelayLineTemplate Item from Template's Attachment being promoted */
	Item    dltp_DelayLine;             /* DelayLine Item created from dltp_DelayLineTemplate for Instrument's Attachment */
} DelayLineTemplatePromotion;

static Item PromoteDelayLineTemplate (List *promotionList, const AudioSample *delayLineTemplateASmp);

/*
	Propagate any attachments in the template to the new instrument. Promote
	delay line templates to delay lines. On failure expects caller to delete
	any completed attachment items added to the instrument's attachment lists.

	Arguments
		instrument
			Item number of instrument under construction.

		aitp
			Pointer to instrument template from which instrument is being created.

	Results
		Non-negative on success; negative Err code on failure.
*/
static Err PropagateTemplateAttachments (Item instrument, const AudioInsTemplate *aitp)
{
	List promotionList;         /* list of DelayLineTemplatePromotions */
	const AudioAttachment *tmplAAtt;
	Err errcode;

		/* do nothing if no template attachments */
	if (IsListEmpty (&aitp->aitp_Attachments)) return 0;

	DBUGPTA(("PropagateTemplateAttachments: template=0x%x ('%s') to ins=0x%x ('%s')\n", aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name, instrument, LookupItem(instrument) ? ((AudioInstrument *)LookupItem(instrument))->ains_Item.n_Name : NULL));

	PrepList (&promotionList);

		/* scan template attachment list */
	SCANLIST (&aitp->aitp_Attachments, tmplAAtt, AudioAttachment) {
		Item slave = tmplAAtt->aatt_SlaveItem;
		bool autoDeleteSlave = FALSE;

		DBUGPTA(("PropagateTemplateAttachments: att=0x%x slave=0x%x (type %d, '%s')\n",
			tmplAAtt->aatt_Item.n_Item,
			tmplAAtt->aatt_SlaveItem,
			((ItemNode *)tmplAAtt->aatt_Structure)->n_Type,
			((ItemNode *)tmplAAtt->aatt_Structure)->n_Name));

			/* if aatt_SlaveItem is a delay line template, promote it to a delay line */
		if (tmplAAtt->aatt_Type == AF_ATT_TYPE_SAMPLE) {
			const AudioSample * const tmplASmp = (AudioSample *)tmplAAtt->aatt_Structure;

			if (IsDelayLineTemplate (tmplASmp)) {
				if ((errcode = slave = PromoteDelayLineTemplate (&promotionList, tmplASmp)) < 0) {
					ERR((
						"PropagateTemplateAttachments: Unable to create delay line from delay line template 0x%x.\n"
						"    tmpl=0x%x att=0x%x ins=0x%x hook='%s'\n",
						tmplASmp->asmp_Item.n_Item,
						aitp->aitp_Item.n_Item, tmplAAtt->aatt_Item.n_Item, instrument, tmplAAtt->aatt_HookName));
					goto clean;
				}
				autoDeleteSlave = TRUE;
			}
		}

			/* Propagate a Template attachment to an instrument being allocated from that
			** template. Duplicates all properties of the template's attachment except
			** AF_TAG_AUTO_DELETE_SLAVE, which is set only if we promoted a delay line
			** template to a delay line. */
		if ((errcode = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE),
			AF_TAG_MASTER,      instrument,
			AF_TAG_SLAVE,       slave,
			AF_TAG_NAME,        tmplAAtt->aatt_HookName,
			AF_TAG_SET_FLAGS,   tmplAAtt->aatt_Flags,
			AF_TAG_START_AT,    tmplAAtt->aatt_StartAt,
			AF_TAG_AUTO_DELETE_SLAVE, autoDeleteSlave,
			TAG_END)) < 0)
		{
			ERR((
				"PropagateTemplateAttachments: Unable to propagate attachment 0x%x.\n"
				"    tmpl=0x%x ins=0x%x hook='%s' slave=0x%x\n",
				tmplAAtt->aatt_Item.n_Item,
				aitp->aitp_Item.n_Item, instrument, tmplAAtt->aatt_HookName, slave));

			if (autoDeleteSlave) DeleteItem (slave);
			goto clean;
		}
	}

		/* success */
	errcode = 0;

clean:
		/* delete contents of promotionList */
	{
		DelayLineTemplatePromotion *n,*succ;

		PROCESSLIST (&promotionList, n, succ, DelayLineTemplatePromotion) {
			SuperFreeMem (n, sizeof *n);
		}
	}

	return errcode;
}

/*
	Promote delay line template to delay line. If already promoted this delay
	line template, use the previous promotion.

	Arguments
		promotionList
			Running list of DelayLineTemplatePromotions to scan and add to.

		delayLineTemplateASmp
			Pointer to delay line template to promote.

	Results
		Non-negative Item number of delay line promoted from delay line
		template on success; negative Err code on failure.
*/
static Item PromoteDelayLineTemplate (List *promotionList, const AudioSample *delayLineTemplateASmp)
{
	Item delayLine;
	DelayLineTemplatePromotion *promotion = NULL;
	Err errcode;

	DBUGPTA(("PromoteDelayLineTemplate: delay line template 0x%x: %d bytes, %d channels, %s\n",
		delayLineTemplateASmp->asmp_Item.n_Item,
		delayLineTemplateASmp->asmp_NumBytes,
		delayLineTemplateASmp->asmp_Channels,
		delayLineTemplateASmp->asmp_ReleaseBegin >= 0 ? "loop" : "no loop"));

		/* Find previous promotion of this delay line template, if there is one.
		**
		** The patch file format groups attachments for the same slave item together,
		** and this order is preserved in the template attachment list from a patch
		** file. Therefore, chances are, that if we are going to find a previous promotion,
		** it will be the one most recently promoted. Because of this, and the fact
		** that new promotion records are added to the tail of the promotion list,
		** scan the promotion list backwards.
		*/
	{
		const Item matchDelayLineTemplate = delayLineTemplateASmp->asmp_Item.n_Item;
		const DelayLineTemplatePromotion *promotion;

		ScanListB (promotionList, promotion, DelayLineTemplatePromotion) {
			if (promotion->dltp_DelayLineTemplate == matchDelayLineTemplate) {
				DBUGPTA(("    use existing delay line 0x%x\n", promotion->dltp_DelayLine));
				return promotion->dltp_DelayLine;
			}
		}
	}

		/* If no previous promotion, promote delay line template to delay line now.
		** Copy as many attributes from delay line template as make sense. */
		/* @@@ only copies useful stuff. this is documented in Sample(@) caveats */
		/* !!! might be a better way to do this, given that the source is also an Item
		**     (e.g., { AF_TAG_CLONE, delayLineTemplateASmp->asmp_Item.n_Item } followed
		**     by tags which override certain settings) */
	if ((errcode = delayLine = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE),
		delayLineTemplateASmp->asmp_Item.n_Name
			? TAG_ITEM_NAME
			: TAG_NOP,         delayLineTemplateASmp->asmp_Item.n_Name,
		AF_TAG_DELAY_LINE,     delayLineTemplateASmp->asmp_NumBytes,
		AF_TAG_CHANNELS,       delayLineTemplateASmp->asmp_Channels,
		AF_TAG_SUSTAINBEGIN,   delayLineTemplateASmp->asmp_SustainBegin,
		AF_TAG_SUSTAINEND,     delayLineTemplateASmp->asmp_SustainEnd,
		AF_TAG_RELEASEBEGIN,   delayLineTemplateASmp->asmp_ReleaseBegin,
		AF_TAG_RELEASEEND,     delayLineTemplateASmp->asmp_ReleaseEnd,
		AF_TAG_SET_FLAGS,      delayLineTemplateASmp->asmp_Flags,
		TAG_END)) < 0) goto clean;

		/* keep track of the promotion */
	if (!(promotion = SuperAllocMem (sizeof *promotion, MEMTYPE_FILL))) {
		errcode = AF_ERR_NOMEM;
		goto clean;
	}
	promotion->dltp_DelayLineTemplate = delayLineTemplateASmp->asmp_Item.n_Item;
	promotion->dltp_DelayLine         = delayLine;
	AddTail (promotionList, (Node *)promotion);

		/* success: return delay line item number */
	DBUGPTA(("    create new delay line 0x%x\n", delayLine));
	return delayLine;

clean:
	SuperFreeMem (promotion, sizeof *promotion);
	DeleteItem (delayLine);
	return errcode;
}


/* -------------------- Instrument Execution (Start, Stop, etc) */

/**
|||	AUTODOC -public -class audio -group Instrument -name StartInstrument
|||	Begins playing an Instrument(@) (Note On).
|||
|||	  Synopsis
|||
|||	    Err StartInstrument (Item Instrument, const TagArg *tagList)
|||
|||	    Err StartInstrumentVA (Item Instrument, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This procedure begins execution of an instrument. This typically starts a
|||	    sound but may have other results, depending on the nature of the
|||	    instrument. This call links the DSP code into the list of active
|||	    instruments. If the instrument has Sample(@)s or Envelope(@)s attached, they
|||	    will also be started (unless the Attachment(@)s specify otherwise). This is
|||	    equivalent to a MIDI "Note On" event.
|||
|||	    The Amplitude and Frequency (or SampleRate) knobs, of instruments that have
|||	    such, can be adjusted by some of the tags listed below before the instrument
|||	    is started. When none of the tags for a particular know are specified, that
|||	    knob is left set to its previous value. At most one tag for each knob can
|||	    be specified. Tags are ignored for Instruments without the corresponding
|||	    knob. Knobs connected to the output of another Instrument
|||	    (see ConnectInstrumentParts()), cannot be set in this manner. A Knob that
|||	    has been grabbed by CreateKnob(), can however be set in this manner.
|||
|||	    This function puts the instrument in the AF_STARTED state. If the instrument
|||	    was previous running, it is first stopped and then restarted. If the instrument
|||	    has a sustain or release loop, it stays in the AF_STARTED state until the
|||	    state is explicitly changed (e.g. ReleaseInstrument(), StopInstrument()).
|||
|||	    This function supercedes a call to PauseInstrument().
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument.
|||
|||	  Tag Arguments
|||
|||	    Amplitude:
|||
|||	    These tags apply to instruments that have an unconnected Amplitude knob.
|||	    They are mutually exclusive except for AF_TAG_EXP_VELOCITY_SCALAR. All
|||	    MIDI velocity tags also perform multi-sample selection based on Sample(@)
|||	    velocity ranges.
|||
|||	    AF_TAG_AMPLITUDE_FP (float32) - Start
|||	        Value to set instrument's Amplitude knob to before starting instrument
|||	        (for instruments that have an Amplitude knob). Valid range -1.0..1.0.
|||
|||	    AF_TAG_VELOCITY (uint8) - Start
|||	        Linear MIDI key velocity to amplitude mapping:
|||
|||	        Amplitude = Velocity / 127.0
|||
|||	        MIDI key velocity is specified in the range of 0..127.
|||
|||	    AF_TAG_SQUARE_VELOCITY (uint8) - Start
|||	        Like AF_TAG_VELOCITY except:
|||
|||	        Amplitude = (Velocity/127.0)**2.
|||
|||	        This gives a more natural response curve than the linear version.
|||
|||	    AF_TAG_EXPONENTIAL_VELOCITY (uint8) - Start
|||	        Like AF_TAG_VELOCITY except:
|||
|||	        Amplitude = 2.0**((Velocity-127)*expVelocityScalar).
|||
|||	        where expVelocityScalar is set using AF_TAG_EXP_VELOCITY_SCALAR.
|||	        This gives a more natural response curve than the linear version.
|||
|||	    AF_TAG_EXP_VELOCITY_SCALAR (float32) - Start
|||	        Sets expVelocityScalar used by AF_TAG_EXPONENTIAL_VELOCITY. These two
|||	        tags are used together with StartInstrument() and can be in either
|||	        order. The default value is (1.0/20.0), which means that the Amplitude
|||	        will double for every 20 units of velocity.
|||
|||	    Frequency:
|||
|||	    These tags apply to instruments that support frequency settings (e.g.,
|||	    oscillators, LFOs, sample players). They have no effect if the Frequency or
|||	    SampleRate knob is connected to another instrument. They are mutually
|||	    exclusive.
|||
|||	    AF_TAG_FREQUENCY_FP (float32)
|||	        Play instrument at a specific output frequency.
|||
|||	        Oscillator or LFO: Sets the Frequency knob to the specified value.
|||
|||	        Sample player with a tuned sample: Adjusts the SampleRate knob to play
|||	        the sample at the desired frequency. For example, to play sinewave.aiff
|||	        at 440Hz, set this tag to 440.
|||
|||	    AF_TAG_PITCH (uint8)
|||	        MIDI note number of the pitch to play instrument at. The range is 0 to
|||	        127; 60 is middle C. Once the output frequency is determined applies the
|||	        frequency in the same manner as AF_TAG_FREQUENCY_FP.
|||
|||	        This tag also performs multi-sample selection based on Sample(@) note
|||	        ranges, and Envelope(@) pitch-based time scaling.
|||
|||	    AF_TAG_RATE_FP (float32)
|||	        Set raw frequency control. For samplers, this is a fraction of DAC rate.
|||	        For oscillators, this is a fractional phase increment.
|||
|||	    AF_TAG_SAMPLE_RATE_FP (float32)
|||	        Sample rate in Hz to set sample player's SampleRate knob to.
|||
|||	    AF_TAG_DETUNE_FP (float32)
|||	        For samplers, play at a fraction of original recorded sample rate.
|||	        For oscillators or LFOs, play at a fraction of default frequency.
|||
|||	    Time Scaling:
|||
|||	    AF_TAG_TIME_SCALE_FP (float32)
|||	        Scales times for all Envelope(@)s attached to this Instrument which do
|||	        not have the AF_ENVF_LOCKTIMESCALE flag set, after pitch-based time
|||	        scaling is performed. Defaults to 1.0. This setting remains in effect
|||	        until the instrument is started again or is changed by
|||	        ReleaseInstrument().
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
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    ReleaseInstrument(), StopInstrument(), PauseInstrument(),
|||	    ResumeInstrument(), Instrument(@), Sample(@)
**/
Err swiStartInstrument (Item InstrumentItem, const TagArg *tagList)
{
	int32 Result;
	AudioInstrument *ains;

DBUG1(("swiStartInstrument ( 0x%x, 0x%x )\n", InstrumentItem, tagList));

	CHECKAUDIOOPEN;

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;

TRACEB(TRACE_INT, TRACE_NOTE,("swiStartInstrument: ains = 0x%x \n", ains));

/* Stop instrument if already running. */
	if( ains->ains_Status > AF_STOPPED )
	{
		Result = swiStopInstrument( InstrumentItem, NULL );
		if( Result < 0 ) return Result;
	}

/* Return positive if did not start but no error. */
	Result = DSPPStartInstrument(ains, tagList);
TRACER(TRACE_INT, TRACE_NOTE, ("swiStartInstrument returns 0x%x\n", Result));
	if (Result) return Result;

	ains->ains_Status = AF_STARTED;
	ains->ains_StartTime = GetAudioClockTime( AB_FIELD(af_GlobalClock) ); /* 960812 */

	return 0;
}


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name ReleaseInstrument
|||	Instruct an Instrument to begin to finish (Note Off).
|||
|||	  Synopsis
|||
|||	    Err ReleaseInstrument (Item Instrument, const TagArg *tagList)
|||
|||	    Err ReleaseInstrumentVA (Item Instrument, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This tells an Instrument to progress into its "release phase" if it hasn't
|||	    already done so. This is equivalent to a MIDI Note Off event. Any Samples
|||	    or Envelopes that are attached are set to their release portion, which may
|||	    or may not involve a release loop. The sound may continue to be produced
|||	    indefinitely depending on the release characteristics of the instrument.
|||
|||	    This has no audible effect on instruments that don't have some kind of
|||	    sustain loop (e.g. sawtooth.dsp(@)).
|||
|||	    Affects only instruments in the AF_STARTED state: sets them to the
|||	    AF_RELEASED state. By default if and when an instrument reaches the end of
|||	    its release phase, it stays in the AF_RELEASED state until explicitly
|||	    changed (e.g. StartInstrument(), StopInstrument()). If one of this
|||	    Instrument's running Attachments has the AF_ATTF_FATLADYSINGS flag set, the
|||	    Instrument is automatically stopped when that Attachment completes. In
|||	    that case, the instrument goes into the AF_STOPPED state, or, if the
|||	    Instrument has the AF_INSF_AUTOABANDON flag set, the AF_ABANDONED state.
|||
|||	    !!! not sure of the effect this has on a paused instrument
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument.
|||
|||	  Tag Arguments
|||
|||	    AF_TAG_TIME_SCALE_FP (float32)
|||	        Scales times for all Envelope(@)s attached to this Instrument which do
|||	        not have the AF_ENVF_LOCKTIMESCALE flag set. Defaults to value set by
|||	        StartInstrument().
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
|||	    StartInstrument(), StopInstrument(), PauseInstrument(),
|||	    ResumeInstrument(), Instrument(@)
**/
Err swiReleaseInstrument (Item InstrumentItem, const TagArg *tagList)
/*
** Allows an event to move toward completion and then free its voice.  Some
** sounds will need time to die out before stopping completely.
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;  /* 00001 */
	if (ains->ains_Status <= AF_RELEASED) return 0;

DBUG(("swiReleaseInstrument( 0x%x )\n", InstrumentItem));
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	ains->ains_Status = AF_RELEASED;
	return DSPPReleaseInstrument (dins, tagList);
}


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name StopInstrument
|||	Abruptly stops an Instrument(@).
|||
|||	  Synopsis
|||
|||	    Err StopInstrument (Item Instrument, const TagArg *tagList)
|||
|||	    Err StopInstrumentVA (Item Instrument, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This procedure, which abruptly stops an instrument, is called when you
|||	    want to abort the execution of an instrument immediately. This can cause
|||	    a click because of its suddenness. You should use ReleaseInstrument() to
|||	    gently release an instrument according to its release characteristics.
|||
|||	    Affects only instruments in the AF_STARTED or AF_RELEASED states: sets them
|||	    to the AF_STOPPED state, or, if the Instrument has the AF_INSF_AUTOABANDON
|||	    flag set, the AF_ABANDONED state.
|||
|||	    This function supercedes a call to PauseInstrument().
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument.
|||
|||	  Tag Arguments
|||
|||	    None
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
|||	    StartInstrument(), ReleaseInstrument(), PauseInstrument(),
|||	    ResumeInstrument(), Instrument(@)
**/
Err swiStopInstrument (Item InstrumentItem, const TagArg *tagList)
/*
** Immediately stop an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;

DBUG(("StopInstrument( 0x%x )\n", InstrumentItem));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if( ains == NULL) return AF_ERR_BADITEM;
	if( tagList ) return AF_ERR_BADTAG;  /* 930831 */
	if( ains->ains_Status <= AF_STOPPED) return 0;

	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	ains->ains_Status = AF_STOPPED;
	Result = DSPPStopInstrument (dins, tagList);
	if (Result < 0) return Result;
	if (ains->ains_Flags & AF_INSF_AUTOABANDON)
	{
		ains->ains_Status = AF_ABANDONED;
	}
	return Result;
}


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name PauseInstrument
|||	Pauses an Instrument's playback.
|||
|||	  Synopsis
|||
|||	    Err PauseInstrument (Item Instrument)
|||
|||	  Description
|||
|||	    This procedure pauses an instrument during playback. A paused
|||	    instrument ceases to play, but retains its position in playback so
|||	    that it can resume playback at that point. ResumeInstrument() allows
|||	    a paused instrument to continue its playback.
|||
|||	    This procedure is intended primarily for sampled-sound instruments,
|||	    where a paused instrument retains its playback position within a
|||	    sampled sound. PauseInstrument() and ResumeInstrument() used with
|||	    sound-synthesis instruments may not have effects any different from
|||	    StartInstrument() and StopInstrument().
|||
|||	    The paused state is superceded by a call to StartInstrument() or
|||	    StopInstrument().
|||
|||	    !!! unsure whether ReleaseInstrument() affects a paused instrument.
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument.
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
|||	    <audio/audio.h>
|||
|||	  Caveats
|||
|||	    This procedure will not pause an envelope attached to an instrument. The
|||	    envelope will continue to play while the instrument is paused.
|||
|||	  See Also
|||
|||	    ResumeInstrument(), StartInstrument(), StopInstrument(), ReleaseInstrument()
**/
Err swiPauseInstrument (Item InstrumentItem)
/*
** Pause an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;

PRTX(("PauseInstrument( 0x%x )\n", InstrumentItem ));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status <= AF_STOPPED) return 0;

	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Result = DSPPPauseInstrument(dins);
	return Result;
}


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name ResumeInstrument
|||	Resumes playback of a paused instrument.
|||
|||	  Synopsis
|||
|||	    Err ResumeInstrument (Item Instrument)
|||
|||	  Description
|||
|||	    This procedure resumes playback of an instrument paused using
|||	    PauseInstrument(). A resumed instrument continues playback from the point
|||	    where it was paused. It does not restart from the beginning of a note.
|||
|||	    This procedure is intended primarily for sampled-sound instruments, where
|||	    a paused instrument retains its playback position within a sampled sound.
|||	    PauseInstrument() and ResumeInstrument() used with sound-synthesis
|||	    instruments may not have effects any different than StartInstrument() and
|||	    StopInstrument().
|||
|||	    This function has no effect on an instrument that has been stopped
|||	    or restarted after being paused.
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument.
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
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    PauseInstrument(), StartInstrument(), StopInstrument(), ReleaseInstrument()
|||
**/
Err swiResumeInstrument (Item InstrumentItem)
/*
** Pause an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;

PRTX(("ResumeInstrument( 0x%x )\n", InstrumentItem ));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status <= AF_STOPPED) return 0;

	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Result = DSPPResumeInstrument(dins);
	return Result;
}


/* -------------------- Instrument Voice Stealing (Adopt/Abandon/Scavenge) */

/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name AbandonInstrument
|||	Makes an Instrument(@) available for adoption from Template(@)
|||
|||	  Synopsis
|||
|||	    Err AbandonInstrument (Item Instrument)
|||
|||	  Description
|||
|||	    This function together with AdoptInstrument() form a simple, but efficient,
|||	    voice allocation system for a single instrument Template(@).
|||	    AbandonInstrument() adds an instrument to a pool of unused instruments;
|||	    AdoptInstrument() allocates instruments from that pool. A Template's pool
|||	    can grow or shrink dynamically.
|||
|||	    Why should you use this system when CreateInstrument() and
|||	    DeleteInstrument() also do dynamic voice allocation? The answer is that
|||	    CreateInstrument() and DeleteInstrument() create and delete Items and
|||	    allocate and free DSP resources. AdoptInstrument() and AbandonInstrument()
|||	    merely manage a pool of already existing Instrument Items belonging to a
|||	    template. Therefore they don't have the overhead of Item creation and DSP
|||	    resource management.
|||
|||	    This function stops the instrument, if it was running, and sets its status
|||	    to AF_ABANDONED (see GetAudioItemInfo() AF_TAG_STATUS). This instrument is
|||	    now available to be adopted from its Template by calling AdoptInstrument().
|||
|||	    Instruments created with the AF_INSF_AUTOABANDON flag set, are
|||	    automatically put into the AF_ABANDONED state when stopped.
|||
|||	  Arguments
|||
|||	    Instrument
|||	        The item number for the instrument to abandon.
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
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    AdoptInstrument(), StopInstrument(), GetAudioItemInfo(), Instrument(@)
**/
Err swiAbandonInstrument( Item Instrument )
{
	AudioInstrument *ains;
	Err Result = 0;

	ains = (AudioInstrument *)CheckItem(Instrument, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status > AF_STOPPED)
	{
		Result = swiStopInstrument( Instrument, NULL );
	}

	ains->ains_Status = AF_ABANDONED;
	return Result;
}


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name AdoptInstrument
|||	Adopts an abandoned Instrument(@) from a Template(@).
|||
|||	  Synopsis
|||
|||	    Item AdoptInstrument (Item instrumentTemplate)
|||
|||	  Description
|||
|||	    This function adopts an instrument from this template's abandoned
|||	    instrument pool (finds an instrument belonging to the the template
|||	    whose status is AF_ABANDONED). It then sets the instrument state to
|||	    AF_STOPPED and returns the instrument item number. If the template has
|||	    no abandoned instruments, this function returns 0.
|||
|||	    Note that this function does not create a new Instrument Item; it
|||	    returns an Instrument Item that was previously passed to
|||	    AbandonInstrument() (or became abandoned because AF_INSF_AUTOABANDON
|||	    was set).
|||
|||	    This function together with AbandonInstrument() form a simple, but
|||	    efficient, voice allocation system for a single instrument Template.
|||
|||	  Arguments
|||
|||	    instrumentTemplate
|||	        The item number for the instrument Template(@) from which to to attempt
|||	        to adopt an Instrument.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Instrument Item number if an instrument could be adopted.
|||
|||	    0
|||	        If no abandoned instruments in template.
|||
|||	    < 0
|||	        Error code on failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V20.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    AbandonInstrument(), GetAudioItemInfo(), Instrument(@)
**/
Item  swiAdoptInstrument( Item InsTemplate )
{
	AudioInstrument *ains;
	AudioInsTemplate *aitp;

	CHECKAUDIOOPEN;

	aitp = (AudioInsTemplate *)CheckItem(InsTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL) return AF_ERR_BADITEM;

/* Scan list of Instruments for next available for adoption. */
	ains = (AudioInstrument *)FirstNode(&aitp->aitp_InstrumentList);

	while (ISNODE(&aitp->aitp_InstrumentList,ains))
	{
DBUG(("Checking instrument status 0x%x = %d\n", ains, ains->ains_Status));
		if (ains->ains_Status == AF_ABANDONED)
		{
			ains->ains_Status = AF_STOPPED;
			return (ains->ains_Item.n_Item);
		}
		ains = (AudioInstrument *)NextNode((Node *)ains);
	}

	return 0;
}


/*************************************************************************/
/* !!! made this function private until its definition gets resolved */
/* !!! this function is moving out of the audio folio */
/**
|||	AUTODOC -private -class audio -group Instrument -name ScavengeInstrument
|||	Pick best instrument to steal from a template.
|||
|||	  Synopsis
|||
|||	    Item ScavengeInstrument (Item instrumentTemplate, uint8 maxPriority,
|||	                             uint32 maxActivity, int32 ifSystemWide)
|||
|||	  Description
|||
|||	    This function identifies which of the instruments created from this
|||	    template is the best to steal for a new voice. This function doesn't
|||	    actually do anything with the voice it picks (i.e. it doesn't stop
|||	    it, or change its status in any way). It merely returns its Item
|||	    number.
|||
|||	    The voice stealing logic first finds an instrument belonging to the
|||	    template with the lowest combination of priority and activity level
|||	    (i.e. AF_TAG_STATUS result: AF_ABANDONED, AF_STOPPED, AF_RELEASED,
|||	    AF_STARTED etc). (This is admittedly a bit non-deterministic, the
|||	    voice it picks depends rather heavily on the order in which instruments
|||	    are added to the template's instrument list). If this still results in
|||	    more than one instrument, the one with the earliest start time is
|||	    picked.
|||
|||	    Instruments with higher priority than MaxPriority or higher activity
|||	    level than MaxActivity are not considered valid choices for stealing.
|||
|||	    Note that this function does not create a new Instrument Item; it
|||	    returns an Instrument Item that was previously created from the template.
|||
|||	  Arguments
|||
|||	    instrumentTemplate
|||	        The item number for the instrument template from which to to attempt to
|||	        steal an instrument from.
|||
|||	    maxPriority
|||	        Maximum instrument priority to consider stealing.
|||
|||	    maxActivity
|||	        Maximum instrument activity level (AF_ABANDONED, AF_STOPPED,
|||	        AF_RELEASE, or AF_STARTED) to consider stealing.
|||
|||	    ifSystemWide
|||	        Must be set to zero. SystemWide scavenging is not supported.
|||
|||	  Return Value
|||
|||	    >0
|||	        Instrument Item number of best instrument to steal.
|||
|||	    0
|||	        If no suitable instrument in template.
|||
|||	    <0
|||	        Error code on failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V20.
|||
|||	  Caveats
|||
|||	    This function currently doesn't disregard instrument priority for
|||	    instruments whose status is AF_ABANDONED. This causes higher priority
|||	    abandoned instruments to be passed over in favor of lower priority
|||	    active instruments. We recommend that you make sure that all
|||	    instruments belonging to the template passed to this instrument be
|||	    created at the same priority.
|||
|||	  Associated Files
|||
|||	    audio.h
|||
|||	  See Also
|||
|||	    AbandonInstrument(), AdoptInstrument(), GetAudioItemInfo(),
|||	    Instrument(@)
**/

/**
	@@@ The following is the voice stealing algorithm used by the score player:

|||	    The voice stealing logic first finds the instrument belonging to the
|||	    template with the lowest activity level (i.e. AF_TAG_STATUS result:
|||	    AF_ABANDONED, AF_STOPPED, AF_RELEASED, AF_STARTED etc). If this
|||	    results in more than one instrument, the one with the lowest priority
|||	    below is picked. If this still results in more than one instrument,
|||	    the one with the earliest start time is picked.
**/

Item swiScavengeInstrument( Item InsTemplate, uint8 MaxPriority, uint32 MaxActivity, int32 IfSystemWide )
{
	Item Chosen=0;
	AudioInstrument *ains, *nextains, *bestains;
	AudioInsTemplate *aitp;
	uint8  LowestPriority;
	uint32 LowestActivity;
	AudioTime EarliestTime;

DBUG(("swiScavengeInstrument( 0x%x, %d, %d, ...)\n", InsTemplate, MaxPriority, MaxActivity));

	aitp = (AudioInsTemplate *)CheckItem(InsTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL) return AF_ERR_BADITEM;

	if( IfSystemWide ) return AF_ERR_UNIMPLEMENTED;

	bestains = NULL;
	LowestPriority = MaxPriority+1;
	LowestActivity = MaxActivity+1;
	EarliestTime = GetAudioClockTime( AB_FIELD(af_GlobalClock) );   /* 960819 */

DBUG(("swiScavengeInstrument: earliest time=%u\n", EarliestTime));

/* Scan list of Instruments for next available for scavenging. */
	ains = (AudioInstrument *)FirstNode(&aitp->aitp_InstrumentList);

/* Search for minimal fit. */
	while (ISNODE(&aitp->aitp_InstrumentList,ains))
	{
		nextains = (AudioInstrument *)NextNode((Node *)ains);
DBUG(("Considering scavenging instrument 0x%x, status %d, start time %u\n",
		ains->ains_Item.n_Item, ains->ains_Status, ains->ains_StartTime));

/* 931129 Check for either lower activity or lower priority. */
		if (((ains->ains_Status < LowestActivity) && (ains->ains_Item.n_Priority <= LowestPriority)) ||
			((ains->ains_Item.n_Priority < LowestPriority) && (ains->ains_Status <= LowestActivity)) ||
/* 940606 All other things being equal, check for earliest start time */
			((CompareAudioTimes (ains->ains_StartTime,EarliestTime) < 0) &&
					 (ains->ains_Item.n_Priority <= LowestPriority) &&
			 (ains->ains_Status <= LowestActivity)))

		{
			bestains = ains;
			LowestPriority = ains->ains_Item.n_Priority;
			LowestActivity = ains->ains_Status;
			EarliestTime   = ains->ains_StartTime;
		}

		ains = nextains;
	}

	if (bestains)
	{
		Chosen = bestains->ains_Item.n_Item;
	}

DBUG(("swiScavengeInstrument returns 0x%x\n", Chosen));
	return Chosen;
}


/* -------------------- Instrument Connections */

static AudioConnectionNode *FindConnectionNode (Item DstIns, const char *DstName, int32 DstPart);

/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name ConnectInstruments
|||	Patches the output of an Instrument(@) to the input of another Instrument.
|||
|||	  Synopsis
|||
|||	    Err ConnectInstruments (Item srcInstrument, const char *srcPortName,
|||	                            Item dstInstrument, const char *dstPortName)
|||
|||	  Description
|||
|||	    This is a convenience macro that can be used to connect single-part ports
|||	    to one another.
|||
|||	  Arguments
|||
|||	    srcInstrument
|||	        Item number of the source instrument.
|||
|||	    srcPortName
|||	        Name of the output port of the source instrument to connect to. Always
|||	        connects to part 0.
|||
|||	    dstInstrument
|||	        Item number of the destination instrument.
|||
|||	    dstPortName
|||	        Name of the input port of the destination instrument to connect to.
|||	        Always connects to part 0.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	    AF_ERR_INUSE is returned if an instrument is already connected to the
|||	    destination instrument's named input. You must disconnect the old
|||	    instrument before connecting another.
|||
|||	  Implementation
|||
|||	    Macro call implemented in <audio/audio.h> v27.
|||
|||	  Notes
|||
|||	    This macro is equivalent to:
|||
|||	  -preformatted
|||
|||	        ConnectInstrumentParts (srcInstrument, srcPortName, 0,
|||	                                dstInstrument, dstPortName, 0);
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    ConnectInstrumentParts()
**/

/**
|||	AUTODOC -public -class audio -group Instrument -name ConnectInstrumentParts
|||	Patches the output of an Instrument(@) to the input of another Instrument.
|||
|||	  Synopsis
|||
|||	    Err ConnectInstrumentParts (
|||	        Item srcInstrument, const char *srcPortName, int32 srcPartNum,
|||	        Item dstInstrument, const char *dstPortName, int32 dstPartNum)
|||
|||	  Description
|||
|||	    This procedure connects an output from one instrument to an input of
|||	    another instrument. This allows construction of complex "patches"
|||	    from existing synthesis modules. Some inputs and outputs have multiple
|||	    parts, or channels, such as mixers, or stereo sample players.  You
|||	    can connect to a specific part by passing a part index. For connections
|||	    that don't require a part index, pass zero, or use ConnectInstruments()
|||
|||	    An output can be connected to one or more inputs; only one output can be
|||	    connected to any given input. If you connect an output to a knob, it
|||	    disconnects that knob from any possible control by SetKnobPart(). Unlike
|||	    Attachment(@)s, this kind of connection does not create an Item.
|||
|||	    You may call DisconnectInstrumentParts() to break a connection set up by
|||	    this function. Connections are tracked by the audio folio, so it is not
|||	    necessary to disconnect before deleting instruments.
|||
|||	    See the DSP Instrument Templates chapter for complete listings of each
|||	    template's ports.
|||
|||	  Arguments
|||
|||	    srcInstrument
|||	        Item number of the source instrument.
|||
|||	    srcPortName
|||	        Name of the output port of the source instrument to connect to.
|||
|||	    srcPartNum
|||	        Part index of output port of the source instrument to connect to.
|||	        For ports with a single part, use zero.
|||
|||	    dstInstrument
|||	        Item number of the destination instrument.
|||
|||	    dstPortName
|||	        Name of the input port of the destination instrument to connect to.
|||
|||	    dstPartNum
|||	        Part index of output port of the destination instrument to connect to.
|||	        For ports with a single part, use zero.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	    Some of the possible error codes are:
|||
|||	    AF_ERR_INUSE
|||	        If an instrument is already connected to the destination instrument's
|||	        named input. You must disconnect the old instrument before connecting
|||	        another.
|||
|||	    AF_ERR_OUTOFRANGE
|||	        If either of the part numbers is out of range.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V27.
|||
|||	  Notes
|||
|||	    Port names are matched case-insensitively.
|||
|||	  Caveats
|||
|||	    Be aware of the interactions between knob settings (even default knob
|||	    settings) and connections. A connection takes precedence over any knob
|||	    setting. When the knob is disconnected, any previous knob setting is
|||	    restored. Because of connection tracking, deleting the source instrument
|||	    also causes this sort of a restoring of knob settings.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    DisconnectInstrumentParts(), ConnectInstruments()
**/
Err swiConnectInstrumentParts  ( Item SrcIns, const char *SrcName, int32 SrcPart,
	Item DstIns, const char *DstName, int32 DstPart)
{
	AudioInstrument *ains_src, *ains_dst;
	DSPPInstrument *dins_src, *dins_dst;
	AudioConnectionNode *acnd;
	int32 Result;

DBUGCON(("swiConnectInstrumentParts: 0x%x,%s,%d    =>   0x%x,%s,%d \n", SrcIns, SrcName, SrcPart, DstIns, DstName, DstPart ));

	ains_src = (AudioInstrument *)CheckItem(SrcIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_src == NULL)
	{
		ERR(("swiConnectInstrumentParts: bad SrcIns = 0x%x\n", SrcIns));
		return AF_ERR_BADITEM;
	}
	dins_src = (DSPPInstrument *)ains_src->ains_DeviceInstrument;

	ains_dst = (AudioInstrument *)CheckItem(DstIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_dst == NULL)
	{
		ERR(("swiConnectInstrumentParts: bad DstIns = 0x%x\n", DstIns));
		return AF_ERR_BADITEM;
	}

	dins_dst = (DSPPInstrument *)ains_dst->ains_DeviceInstrument;

/* Verify name addresses. */
	Result = afi_IsRamAddr( SrcName, 1);
	if(Result < 0) return Result;
	Result = afi_IsRamAddr( DstName, 1);
	if(Result < 0) return Result;

/* Validate Part numbers in dsppConnectInstrumentParts() */

/* Check to see whether a connection has already been made. */
	acnd = FindConnectionNode( DstIns, DstName, DstPart );
	if( acnd != NULL )
	{
		ERR(("swiConnectInstrumentParts: port %s in use.\n", DstName ));
		return AF_ERR_INUSE;
	}

/* Create a node to keep track of the connection. 950417 */
	acnd = (AudioConnectionNode *) EZMemAlloc(sizeof(AudioConnectionNode), 0);
	if (acnd == NULL) return AF_ERR_NOMEM;
	acnd->acnd_SrcIns = SrcIns;
	acnd->acnd_DstIns = DstIns;
	acnd->acnd_SrcPart = SrcPart;
	acnd->acnd_DstPart = DstPart;
DBUGCON(("swiConnectInstrument: acnd = 0x%x\n", acnd));

/* Call DSPP */
	Result = dsppConnectInstrumentParts ( acnd, dins_src, SrcName, SrcPart, dins_dst, DstName, DstPart );
	if( Result < 0 )
	{
		EZMemFree( (char *) acnd );
	}
	else
	{
		AddTail( &AB_FIELD(af_ConnectionList), (Node *) acnd );
	}
	return Result;
}


/* { !!! Hack to work around 4-arg SWI limit for ConnectInstrumentParts() */

extern Err ConnectInstrumentsHack( const int32 *Params );

/* temp user-mode entry point for ConnectInstrumentParts() - packs up args into array to pass to glue SWI */
Err ConnectInstrumentParts( Item SrcIns, const char *SrcName, int32 SrcPart,
	Item DstIns, const char *DstName, int32 DstPart)
{
/* Pack parameters into array and pass to SWI */
	int32 Params[6];
	Params[0] = (int32) SrcIns;
	Params[1] = (int32) SrcName;
	Params[2] = (int32) SrcPart;
	Params[3] = (int32) DstIns;
	Params[4] = (int32) DstName;
	Params[5] = (int32) DstPart;
	return ConnectInstrumentsHack( Params );
}

/* hack SWI for ConnectInstrumentParts() which unpacks args packed up by user-mode entry point */
Err swiConnectInstrumentsHack( const int32 *Params )
{
	Item SrcIns, DstIns;
	char *SrcName, *DstName;
	int32 SrcPart, DstPart;
/* Unpack parameters from array and pass to SWI */
	SrcIns  = (Item)  *Params++;
	SrcName = (char *) *Params++;
	SrcPart = (int32)  *Params++;
	DstIns  = (Item)  *Params++;
	DstName = (char *) *Params++;
	DstPart = (int32)  *Params;
	return swiConnectInstrumentParts( SrcIns, SrcName, SrcPart,
		DstIns, DstName, DstPart);
}

/* } !!! end hack */


/*************************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name DisconnectInstrumentParts
|||	Breaks a connection made by ConnectInstrumentParts().
|||
|||	  Synopsis
|||
|||	    Err DisconnectInstrumentParts (
|||	        Item dstInstrument, const char *dstPortName, int32 dstPartNum)
|||
|||	  Description
|||
|||	    This procedure breaks a connection made by ConnectInstrumentParts() between
|||	    two instruments. If the connection was to a knob of the second instrument,
|||	    the knob is once again available for tweaking. Since only one connection can
|||	    be made to any given destination, it is not necessary to specify the Source.
|||
|||	  Arguments
|||
|||	    dstInstrument
|||	        Item number of the destination instrument.
|||
|||	    dstPortName
|||	        Name of the input port of the destination instrument to break connection
|||	        to.
|||
|||	    dstPartNum
|||	        Part index of output port of the destination instrument to connect to.
|||	        For ports with a single part, use zero.
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
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    ConnectInstrumentParts()
**/
Err swiDisconnectInstrumentParts (Item DstIns, const char *DstName, int32 DstPart)
{
	int32 Result;
	AudioInstrument  *ains_dst;
	DSPPInstrument  *dins_dst;
	AudioConnectionNode *acnd;

DBUGCON(("swiDisconnectInstrumentParts: Dst = 0x%x %s\n", DstIns, DstName ));

/* Validate destination instrument. Destination is required. */
	ains_dst = (AudioInstrument *)CheckItem(DstIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_dst == NULL) return AF_ERR_BADITEM;
	Result = afi_IsRamAddr( DstName, 1);
	if(Result < 0) return Result;

/* All connections should have a connection node! */
	acnd = FindConnectionNode( DstIns, DstName, DstPart );
DBUGCON(("swiDisconnectInstrumentParts: acnd = 0x%08x\n", acnd ));
	if( acnd == NULL )
	{
		ERR(("swiDisconnectInstrumentParts: nothing connected!\n"));
		return AF_ERR_BADPTR;  /* !!! Use new error? */
	}

/* Found connection so break it. */
	dins_dst = (DSPPInstrument *)ains_dst->ains_DeviceInstrument;
	Result = dsppDisconnectInstrumentParts ( acnd, dins_dst, DstName, DstPart );

	ParanoidRemNode( (Node *) acnd );
	EZMemFree( (char *) acnd );

	return Result;
}


/**********************************************************************
** Find node used to track ConnectInstruments() calls
*/
static AudioConnectionNode *FindConnectionNode (Item DstIns, const char *DstName, int32 DstPart)
{
	AudioConnectionNode *acnd;

	DBUGCON(("FindConnectionNode: find Dst = 0x%x %s %d\n", DstIns, DstName, DstPart ));

	ScanList (&AB_FIELD(af_ConnectionList), acnd, AudioConnectionNode) {
		DBUGCON(("FindConnectionNode: acnd = 0x%08x\n", acnd ));
		DBUGCON(("FindConnectionNode: this Dst = 0x%x %s %d\n", acnd->acnd_DstIns,
			acnd->acnd_DstName, acnd->acnd_DstPart ));

		if( (acnd->acnd_DstIns == DstIns) &&
			(acnd->acnd_DstPart == DstPart) &&
			(strcasecmp(acnd->acnd_DstName, DstName ) == 0) )
		{
			return acnd;
		}
	}
	return NULL;
}
