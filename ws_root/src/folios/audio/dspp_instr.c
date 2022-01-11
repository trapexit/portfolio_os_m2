/* @(#) dspp_instr.c 96/08/09 1.104 */
/* $Id: dspp_instr.c,v 1.124 1995/03/13 20:53:48 peabody Exp phil $ */
/****************************************************************
**
** DSPP Instrument Manager
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921112 PLB Merge DRSC_OUTPUT and DRSC_I_MEM
** 921113 PLB Add DSPPFindResource, add DSPPConnectInstruments
** 921116 PLB DSPP Code now threaded, Fix bad DSPPRelocate
** 921119 PLB Use AllocThings and FreeThings for proper resource alloc.
**          Call DSPPStopInstrument in DSPPFreeInstrument.
** 921216 PLB Link code in DSPP in same order as in host list.
**          Stop sample after unlinking code.
** 921218 PLB Added AllocAmplitude and FreeAmplitude
** 921221 PLB Added DisconnectInstruments
** 921228 PLB Check for relocation offsets out of bounds to detect LF->CR
**            mangling of xxx.ofx by Mac.
** 930104 PLB Use SampleAttachments list to handle multiple attachments.
**            AttachSample now uses FIFOName.
** 930112 PLB Validate *Alloc in DSPGetRsrcAlloc
** 930126 PLB Change to use .dsp files instead of .ofx
** 930302 PLB StopInstrument does not set Amplitude=0 anymore.
** 930306 PLB Support compound 3INS instruments.
** 930311 PLB Free TemplateSample structures.
** 930323 PLB Switch to generic Attachment structure.
** 930326 PLB Use DSPPStartAttachment instead of DSPP_StartSample
** 930415 PLB Track connections between Items
** 930607 PLB Add tuning.
** 930612 PLB Added IMem access.
** 930616 PLB Added DSP Data Initialization
** 930703 PLB Added Release Loop Support
** 930728 PLB Do not play note if no sample in range.
** 930830 PLB Allow disconnection from Knobs
** 930907 PLB Add FIFOInit to DSPPStartSampleAttachment
** 931116 PLB Clear head of DSPP FIFO to prevent pops.
** 931117 PLB Instrument may have gotten stopped by DSPPReleaseEnvAttachment
**            so check before setting status to AF_RELEASED.   This was
**            causing a crash if the ReleasePoint on an envelope
**            was set to the last point.  Crash was due to double RemNode
**            of instrument when stopped the second time.
** 931130 PLB Fix bug with premature STOP for looping samples with FATLADYSINGS
** 931201 PLB Rewrote DSPPReleaseSampleAttachment() to fix FATLADYSINGS problems
**				and to fix problem with missing samples between sustain loop and release loop.
** 931211 PLB Implemented PauseInstrument and ResumeInstrument
** 931216 PLB Check for input DMA Type before clearing FIFO HEAD
** 931220 PLB Added DSPPCalcSampleStart, rewrote DSPPStartSampleAttachmant and
**               DSPPNextAttachment to use it.
** 931221 PLB Trap Amplitude < 0 in AllocAmplitude() and FreeAmplitude()
** 940203 PLB Fix check for sample loop > 0
** 940502 PLB Add support for audio input.
** 940608 PLB Added DSPPAdjustTicksPerFrame()
** 940609 PLB Added shared library template support.
** 940713 PLB Fixed DMA Channel for DSPPStopInstrument for Output channels.
**            It used to turn off any Input channel with the same index.
**            Same for DSPPReleaseInstrument().
** 940901 PLB Change to new dsph prefix names for Anvil/Bulldog
** 940908 PLB Rewrote DumpList() so it doesn't crash on empty list. DEBUG mode only.
** 941024 PLB Trap negative DMA counts that can occur if AF_TAG_START_AT
**            is past loop end.
** 941031 PLB Clean up handling of dins_ActivityLevel flag.  It now
**            indicates whether an instrument's code is executing.
** 941101 PLB Prevent negative loop sizes from hanging DMA
** 941121 PLB Stop sample attachment if running when started.
** 941121 PLB Don't enable DMA interrupt if release loop in sample.
** 941130 PLB Restore Knob connection on DisconnectInstrument(), CR3763
** 950112 PLB Clear head of FIFO to prevent expansion bus hardware bug
**            from causing the playback of garbage data.
** 950113 WJB Now using DSPP opcode definitions from dspp_instructions.h.
** 950118 WJB Privatized PatchCode16().
** 950119 WJB Made dsppRelocate() fixupFunc() callback system more general.
** 950120 WJB Fixed percent-Q in ConnectInstrument() usage of dsppRelocate().
** 950120 WJB Cleaned up a triple bang and a few warnings.
** 950120 WJB Further localized a local variable in DSPPStopCodeExecution().
** 950126 PLB Overwrite SLEEP at beginning of HEAD code to enable execution.
**            This was moved from dspp_loader where it was causing premature
**            execution of "head.dsp".  Fixes CR4215.
** 950130 WJB Replaced opcode defines with new ones from dspp_instructions.h.
** 950131 WJB Cleaned up includes.
** 950206 WJB Moved resource code to dspp_resource.c.
** 950208 WJB Updated DSPGetInsRsrcUsed() to be aware of DRSC_IMPORT/EXPORT flags.
** 950224 WJB Renamed dspp_ExternalList to dspp_ExportedResourceList.
** 950228 PLB Use dsphStart() and dsphHalt() when TAIL started or stopped to
**            control DSPP.
** 950307 WJB Added DSPPTemplate arg to dsppRelocate().
** 950308 WJB Removed DSPPAdjustTicksPerFrame() and related avail tick code.
** 950309 WJB Restored initialization of dspp_SampleRate (accidentally removed in previous changed).
** 950313 WJB Surrounded HACK_TAG_READ_EO support with #ifdef DSPP_MODE_OPERA_RESOURCES.
** 950412 WJB Added knowledge of DRSC_F_BIND.
** 950420 WJB Changed swiTestHack() HACK_TAG_READ_EO to call dsphReadEOMem().
**            Added placeholder for special EO memory emulation.
** 950420 WJB Added calls to dsph...() functions for swiTestHack() HACK_TAG_READ_EO cases.
** 950425 WJB Moved dspp_ExportedResourceList to dspp_resources.c.
** 950427 WJB Commented out HACK_TAG_READ_DMA support - will fix soon.
** 950502 WJB Replaced usage of dsppGetResourceAttribute() with dsppGetResourceRelocationValue().
** 950508 WJB Moved I-Mem stuff into dspp_imem_anvil.c.
** 950628 PLB Changed ConnectInstruments() to ConnectInstrumentParts()
** 960220 PLB Added range check for LowVelocity and HighVelocity for multisamples.
** 960522 PLB Fix velocity selection when Pitch not specified. CR5506
** 960617 PLB If we don't start DMAChan then we should clear FIFO of old junk. CR6036
****************************************************************/

#include <dspptouch/dspp_instructions.h>    /* DSPP opcodes */
#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

#include "audio_folio_modes.h"      /* AF_ modes (!!! no longer used) */
#include "audio_internal.h"
#include "dspp_resources.h"         /* DSPP resource allocation */


/* -------------------- Debug */

#define DEBUG_Connect   0
#if DEBUG_Connect
#define DBUGCON(x)   PRT(x)
#else
#define DBUGCON(x)   DBUG(x)
#endif

/* #define DEBUG */
#define DBUG(x)      /* PRT(x) */
#define DBUGOFX(x)   DBUG(x)
#define DBUGNOTE(x)  DBUG(x)
#define DBUGSAMP(x)  DBUG(x)
#define DBUGLIST(x)  DBUG(x)
#define DBUGATT(x)   DBUG(x)
#define DBUGTUNE(x)  DBUG(x)
#define DBUGINIT(x)  DBUG(x)


/* -------------------- Misc defines */

#define CHECK_BEFORE_DISABLE   (0)
#define DSPP_MAX_AMPLITUDE     (0x7FFF)

/* #define COMPILE_DSPP_TRACE */
#define MIN_DMA_COUNT (4)   /* Still valid? !!! */


/* -------------------- Silence */
/* Dummy DMA buffers to use when no real buffer is available */

#define DSPH_SILENCE_SIZE   16      /* size in DSPP words */

	/* Buffers must be even-address aligned, so declare them as uint16, which
	** is conveniently the same size as a DSPP word. */
static uint16 DMASilenceBuf [DSPH_SILENCE_SIZE];    /* Zero data to play silence. */
static uint16 DMAScratchBuf [DSPH_SILENCE_SIZE];    /* Scratch target of output DMA. */

	/* Macro to get correct silence ram buffer */
#define dsphGetSilenceAddress(DMAChan) \
	((AudioGrain *)((dsphGetChannelDirection(DMAChan) == OUTPUT_FIFO) ? DMAScratchBuf : DMASilenceBuf))


/* -------------------- Misc DSPP-related data */

/* !!! use as many static initializers as we can here (e.g. INITLIST()) */
DSPPStatic DSPPData;


/* -------------------- Local functions */

static DSPPCodeList *DSPPSelectRunningList( DSPPInstrument *dins );
static Err   DSPPLinkCodeToStartList( DSPPInstrument *dins, DSPPCodeList *dcls );
static Err   DSPPUnlinkCodeToStopList( DSPPInstrument *dins, DSPPCodeList *dcls );
static void  dsppGetPrevNextAddresses( DSPPInstrument *dins, DSPPCodeList *dcls,
		int32 *PrevAddressPtr, int32 *NextAddressPtr );
static Err   DSPPStopCodeExecution( DSPPInstrument *dins );
static Err   DSPPStartCodeExecution( DSPPInstrument *dins );
static int32 dsppConvertFramesToCount( int32 Frames,AudioSample *asmp, uint8 SubType );
static Err   dsppInitConnectionMoves( void );
static void  dsppJumpToAddress ( int32 FromAddress, int32 ToAddress );
static int32 DSPPGetResourceChannel( DSPPResource *drsc );

/*****************************************************************/
/******* Supervisor Level Code ***********************************/

Err DSPP_Init( void )
{
	DBUGINIT(("DSPP_init: DMASilenceBuf=0x%x DMAScratchBuf=0x%x\n", DMASilenceBuf, DMAScratchBuf));

	dsphInitDSPP();

	PrepList(&DSPPData.dspp_FullRateInstruments.dcls_InsList);
	PrepList(&DSPPData.dspp_HalfRateInstruments.dcls_InsList);
	PrepList(&DSPPData.dspp_EighthRateInstruments.dcls_InsList);

/* Set DAC frame rate to default */
	AF_SAMPLE_RATE = DEFAULT_SAMPLERATE;

/* Initialize resources */
DBUGINIT(("DSPP_Init: call dsppInitResources()\n"));
	dsppInitResources();
DBUGINIT(("DSPP_Init: returned from dsppInitResources()\n"));

/* Initialize MOVE block for ConnectInstruments() */
	dsppInitConnectionMoves();

	return 0;
}

/*****************************************************************/
void DSPP_Term( void )
{
	dsphTermDSPP();
}

/*****************************************************************
** Scan list of samples for first that matches range.
** If Note and Velocity are both -1, then just pick first sample that doesn't have NOAUTOSTART set.
** If Note is -1, then ignore it.
** If Velocity is -1, then ignore it.
*/
static AudioAttachment *ScanMultiSamples( List *AttList, int32 Note, int32 Vel )
{
  	AudioSample *samp;
	AudioAttachment *aatt, *ChosenAAtt;

DBUGNOTE(("ScanMultiSamples: Note = %d, Vel = %d\n", Note, Vel ));

	aatt = (AudioAttachment *)FirstNode(AttList);
	ChosenAAtt = NULL; /* default */
	if( (Note < 0) && (Vel < 0) )
	{

		while (ISNODE(AttList,aatt))
		{
			if ((aatt->aatt_Flags & AF_ATTF_NOAUTOSTART) == 0)
			{
				ChosenAAtt = aatt;
				break;
			}
			aatt = (AudioAttachment *)NextNode((Node *)aatt);
		}
	}
	else
	{
		while (ISNODE(AttList,aatt))
		{
			samp = (AudioSample *) aatt->aatt_Structure;
DBUGNOTE(("ScanMultiSamples: aatt = 0x%x\n", aatt ));
			if( ( (Note < 0) || ((Note >= samp->asmp_LowNote) && (Note <= samp->asmp_HighNote)) ) &&
			    ( (Vel < 0)  || ((Vel >= samp->asmp_LowVelocity) && (Vel <= samp->asmp_HighVelocity)) ) )
			{
					ChosenAAtt = aatt;
					break;
			}
			aatt = (AudioAttachment *)NextNode((Node *)aatt);
		}
	}

DBUGNOTE(("ScanMultiSamples: ChosenAAtt = 0x%x\n", ChosenAAtt ));
	return ChosenAAtt;
}

/*****************************************************************/
static DSPPCodeList *DSPPSelectRunningList( DSPPInstrument *dins )
{
	DSPPCodeList *dcls;

	if( dins->dins_RateShift == 0 )
	{
DBUGLIST(("DSPPSelectRunningList: use dspp_FullRateInstruments\n"));
		dcls = &DSPPData.dspp_FullRateInstruments;
	}
	else if( dins->dins_RateShift == 1 )
	{
DBUGLIST(("DSPPSelectRunningList: use dspp_HalfRateInstruments\n"));
		dcls = &DSPPData.dspp_HalfRateInstruments;
	}
	else if( dins->dins_RateShift == 3 )
	{
DBUGLIST(("DSPPSelectRunningList: use dspp_EighthRateInstruments\n"));
		dcls = &DSPPData.dspp_EighthRateInstruments;
	}
	else
	{
		ERR(("DSPPSelectRunningList: bad rate=%d, dins=0x%x\n", dins->dins_RateShift, dins ));
		dcls = &DSPPData.dspp_FullRateInstruments;
	}
	return dcls;
}

/********************************************************************
** Start execution on DSPP of instrument.   M2 VERSION
** Handle special case of nanokernel.dsp.
** This version assumes that the special splits are loaded before
** any regular instruments.
********************************************************************/
static Err DSPPStartCodeExecution( DSPPInstrument *dins )
{
	int32 Result = 0;

DBUGLIST(("DSPPStartCodeExecution: dins = 0x%x, Specialness = %d\n", dins, dins->dins_Specialness ));
/* Activate code if not already. 941031 */
	if (dins->dins_ActivityLevel > AF_STOPPED) return 0;

	switch( dins->dins_Specialness )
	{
		case AF_SPECIAL_NOT:
			Result = DSPPLinkCodeToStartList( dins, DSPPSelectRunningList( dins ) );  /* Most common. */
			break;

		case AF_SPECIAL_KERNEL:
/* Import special addresses. */
			if( (DSPPData.dspp_FullRateInstruments.dcls_JumpAddress = dsppImportResource("FullRateJump")) < 0 ) return AF_ERR_BADOFX;
			if( (DSPPData.dspp_FullRateInstruments.dcls_ReturnAddress = dsppImportResource("FullRateReturn")) < 0 ) return AF_ERR_BADOFX;

			if( (DSPPData.dspp_HalfRateInstruments.dcls_JumpAddress = dsppImportResource("HalfRateJump")) < 0 ) return AF_ERR_BADOFX;
			if( (DSPPData.dspp_HalfRateInstruments.dcls_ReturnAddress = dsppImportResource("HalfRateReturn")) < 0 ) return AF_ERR_BADOFX;

			if( (DSPPData.dspp_EighthRateInstruments.dcls_JumpAddress = dsppImportResource("EighthRateJump")) < 0 ) return AF_ERR_BADOFX;
			if( (DSPPData.dspp_EighthRateInstruments.dcls_ReturnAddress = dsppImportResource("EighthRateReturn")) < 0 ) return AF_ERR_BADOFX;

			dsphStart();   /* Don't start until we get this instrument. */
			dsphEnableADIO();   /* Now enable ADIO so DSPP can proceed. */
			break;

	}

	dins->dins_ActivityLevel = AF_STARTED;

	return Result;
}

/*************************************************************************
** dsppApplyStartFreq()
**
** Scan resources in instrument and tweak all that are pitch following.
** Convert to internal value based on KnobType and FreqType
** We need to check all resources because some patch instruments may have
** combinations of synthetic oscillators and samplers that need different
** setting to result in the same perceived pitch.
** !!! Eventually we want to set a FOLLOWS_PITCH flag in resource so we know
** whether to tweak it or not.
** !!! We also need some way to associate SampleRate knobs with a FIFO
** in case an instrument has two FIFOs with different sample rate samples, ie. XFADE.
** !!! Doesn't deal w/ multi-part sample rate or frequency knobs.
*/
/* Define different ways we can specify frequency */
#define FREQ_TYPE_DEFAULT     (0)
#define FREQ_TYPE_RATE        (1)
#define FREQ_TYPE_HERTZ       (2)
#define FREQ_TYPE_SAMPLE_RATE (3)
#define FREQ_TYPE_DETUNE      (4)
#define FREQ_TYPE_PITCH       (5)

void dsppApplyStartFreq( AudioInstrument *ains, int32 FreqType, float32 FreqRequested, int32 PitchNote, AudioAttachment *ChosenAAtt )
{
	int32 i;
	DSPPInstrument *dins;
	DSPPResource *drsc, *drscarray;
	float32 FreqRate = 1.0;

DBUGNOTE(("dsppApplyStartFreq: FreqType = %d, FreqRequested = %g, PitchNote = %d\n",
	FreqType, FreqRequested, PitchNote ));

	dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
	drscarray = dins->dins_Resources;

	for (i=0; i<dins->dins_NumResources; i++)
	{
		int32 IfSetFreq;

		drsc = &drscarray[i];
DBUGNOTE(("dsppApplyStartFreq: Resource name = %s, SubType = %d\n",
	DSPPGetRsrcName( dins, i ), drsc->drsc_SubType ));


		IfSetFreq = FALSE;
		if( drsc->drsc_Type == DRSC_TYPE_KNOB )
		{
DBUGNOTE(("dsppApplyStartFreq: ChosenAAtt = 0x%x\n", ChosenAAtt ));
			if( drsc->drsc_SubType == AF_SIGNAL_TYPE_SAMPLE_RATE )
			{
				FreqRate = ConvertF15_FP( drsc->drsc_Default );
				IfSetFreq = TRUE;
				switch( FreqType )
				{
				case FREQ_TYPE_DEFAULT:
			/* Play at recorded rate. but what if already set by knob? !!! */
					if(ChosenAAtt != NULL)
					{
						FreqRate = ((AudioSample *) ChosenAAtt->aatt_Structure)->asmp_SampleRate / AF_SAMPLE_RATE;
					}
					break;
				case FREQ_TYPE_HERTZ:
	/* This one compiles badly unless we enable DBUGNOTE and print. !!! */
					if(ChosenAAtt != NULL)
					{
						FreqRate = FreqRequested / (((AudioSample *) ChosenAAtt->aatt_Structure)->asmp_BaseFreq) ;
					}
					break;
				case FREQ_TYPE_SAMPLE_RATE:
					FreqRate = FreqRequested / AF_SAMPLE_RATE;
					break;
				case FREQ_TYPE_DETUNE:
					if(ChosenAAtt != NULL)
					{
						FreqRate = (FreqRequested * ((AudioSample *) ChosenAAtt->aatt_Structure)->asmp_SampleRate) / AF_SAMPLE_RATE;
					}
					break;
				case FREQ_TYPE_PITCH:
					if(ChosenAAtt != NULL)
					{
						if( PitchToFrequency( GetInsTuning(ains), PitchNote, &FreqRequested) == 0)
						{
							FreqRate = FreqRequested / ((AudioSample *) ChosenAAtt->aatt_Structure)->asmp_BaseFreq ;
						}
					}
					break;
				case FREQ_TYPE_RATE:
					FreqRate = FreqRequested;
					break;
				}
			}
	/* Synthetic oscillator type knob, sawtooth, triangle, etc. */
			else if( drsc->drsc_SubType == AF_SIGNAL_TYPE_OSC_FREQ )
			{
				FreqRate = ConvertF15_FP( drsc->drsc_Default );
				IfSetFreq = TRUE;
				switch( FreqType )
				{
				case FREQ_TYPE_DEFAULT:
					IfSetFreq = FALSE;
					break;
				case FREQ_TYPE_HERTZ:
					FreqRate = (FreqRequested * 2.0)/ AF_SAMPLE_RATE;
					break;
				case FREQ_TYPE_SAMPLE_RATE:
					break;
				case FREQ_TYPE_DETUNE:
					FreqRate = (FreqRequested * FreqRate);
					break;
				case FREQ_TYPE_PITCH:
					PitchToFrequency( GetInsTuning(ains), PitchNote, &FreqRequested);
					FreqRate = (FreqRequested * 2.0) / AF_SAMPLE_RATE;
					break;
				case FREQ_TYPE_RATE:
					FreqRate = FreqRequested;
					break;
				}
			}
	/* LFO's  !!! This is way too big and wasteful.  Rewrite in a more compact way. use dspp_signals.c*/
			else if( drsc->drsc_SubType == AF_SIGNAL_TYPE_LFO_FREQ )
			{
#define LFO_SCALAR   (256.0)
				FreqRate = ConvertF15_FP( drsc->drsc_Default );
				IfSetFreq = TRUE;
				switch( FreqType )
				{
				case FREQ_TYPE_DEFAULT:
					IfSetFreq = FALSE;
					break;
				case FREQ_TYPE_HERTZ:
					FreqRate = (FreqRequested * 2.0 * LFO_SCALAR )/ AF_SAMPLE_RATE;
					break;
				case FREQ_TYPE_SAMPLE_RATE:
					break;
				case FREQ_TYPE_DETUNE:
					FreqRate = (FreqRequested * FreqRate  * LFO_SCALAR );
					break;
				case FREQ_TYPE_PITCH:
					PitchToFrequency( GetInsTuning(ains), PitchNote, &FreqRequested);
					FreqRate = (FreqRequested * 2.0  * LFO_SCALAR) / AF_SAMPLE_RATE;
					break;
				case FREQ_TYPE_RATE:
					FreqRate = FreqRequested;
					break;
				}
			}
		}

DBUGNOTE(("dsppApplyStartFreq: IfSetFreq = %d, FreqRate = %g\n", IfSetFreq, FreqRate ));

/* Apply current pitch bend, and set knob. */
		if( IfSetFreq )
		{
			if(ains->ains_Bend != 1.0)
			{
				FreqRate = FreqRate * ains->ains_Bend;
			}
/* Account for Execution Rate. */
			if( dins->dins_RateShift != 0)
			{
				FreqRate *= ((float32) (1<<dins->dins_RateShift));
			}
			dsppSetKnob( drsc, 0, FreqRate, drsc->drsc_SubType );
		}

	}

/* Set so that we can update them using Pitch bend */
	ains->ains_StartingFreqType = FreqType;
	ains->ains_StartingFreqReq = FreqRequested;
	ains->ains_StartingPitchNote = PitchNote;
	ains->ains_StartingAAtt = ChosenAAtt;
}

/*****************************************************************
** DSPPStartInstrument - start instrument playing.
** Set pitch various ways as a convenience.
*/
int32 DSPPStartInstrument( AudioInstrument *ains, const TagArg *tagList )
{
	DSPPInstrument *dins;
	AudioAttachment *ChosenAAtt;
	int32 PitchNote = -1, Velocity = -1;
	int32 FreqType = FREQ_TYPE_DEFAULT;
	uint32 ampTag = TAG_END;    /* Using TAG_END to indicate that no amplitude tag has been encountered. */
	float32 TimeScale = 1.0;
	int32 i;
	int32 Result;
	float32 Amplitude = 0.0;
	float32 FreqRequested = 0.0;
	float32 expVelocityScalar = (1.0/20.0);

	dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
	ChosenAAtt = NULL;

DBUGNOTE(("DSPPStartInstrument ( dins=0x%lx, tagList=0x%lx\n",
	dins, tagList));

/* Stop the executing DSP code. 941031 */
	Result = DSPPStopCodeExecution( dins );
	if( Result < 0 ) return Result;

/* Process tags */
	{
		const TagArg *tag;

		for (Result = SafeFirstTagArg (&tag, tagList); Result > 0; Result = SafeNextTagArg (&tag))
		{
DBUGNOTE(("DSPPStartInstrument: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
			switch (tag->ta_Tag)
			{
			case AF_TAG_AMPLITUDE_FP:
				if( ampTag != TAG_END ) return AF_ERR_TAGCONFLICT;
				Amplitude = ConvertTagData_FP(tag->ta_Arg);
				ampTag = tag->ta_Tag;
				break;

			case AF_TAG_VELOCITY:
				if( ampTag != TAG_END ) return AF_ERR_TAGCONFLICT;
				Velocity = (int32)tag->ta_Arg;
				Amplitude = (float32) Velocity / 128.0;  /* max MIDI velocity */
				ampTag = tag->ta_Tag;
				break;

			case AF_TAG_SQUARE_VELOCITY:
				if( ampTag != TAG_END ) return AF_ERR_TAGCONFLICT;
				Velocity = (int32)tag->ta_Arg;
				Amplitude = (float32) Velocity / 127.0;  /* max MIDI velocity */
				Amplitude *= Amplitude;  /* Square amplitude to give better response curve. */
				ampTag = tag->ta_Tag;
				break;

			case AF_TAG_EXPONENTIAL_VELOCITY:
				if( ampTag != TAG_END ) return AF_ERR_TAGCONFLICT;
				Velocity = (int32)tag->ta_Arg;
				ampTag = tag->ta_Tag;
				break;

			case AF_TAG_EXP_VELOCITY_SCALAR:
				expVelocityScalar = ConvertTagData_FP(tag->ta_Arg);
				break;

			case AF_TAG_FREQUENCY_FP:
				if( FreqType != FREQ_TYPE_DEFAULT ) return AF_ERR_TAGCONFLICT;
				FreqRequested = ConvertTagData_FP(tag->ta_Arg);
				FreqType = FREQ_TYPE_HERTZ;
				break;

			case AF_TAG_RATE_FP:
				if( FreqType != FREQ_TYPE_DEFAULT ) return AF_ERR_TAGCONFLICT;
				FreqRequested = ConvertTagData_FP(tag->ta_Arg);
				FreqType = FREQ_TYPE_RATE;
				break;

			case AF_TAG_SAMPLE_RATE_FP:
				if( FreqType != FREQ_TYPE_DEFAULT ) return AF_ERR_TAGCONFLICT;
				FreqRequested = ConvertTagData_FP(tag->ta_Arg);
				FreqType = FREQ_TYPE_SAMPLE_RATE;
				break;

			case AF_TAG_DETUNE_FP:
				if( FreqType != FREQ_TYPE_DEFAULT ) return AF_ERR_TAGCONFLICT;
				FreqRequested = ConvertTagData_FP(tag->ta_Arg);
				FreqType = FREQ_TYPE_DETUNE;
				break;

			case AF_TAG_PITCH:
				if( FreqType != FREQ_TYPE_DEFAULT ) return AF_ERR_TAGCONFLICT;
				PitchNote = (int32)tag->ta_Arg;
				FreqType = FREQ_TYPE_PITCH;
				break;

			case AF_TAG_TIME_SCALE_FP:
				TimeScale = ConvertTagData_FP(tag->ta_Arg);
				break;

			default:
				ERR(("StartInstrument: Unrecognized tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
				return AF_ERR_BADTAG;
			}
		}

			/* Catch tag processing errors */
		if (Result < 0) {
			ERR(("StartInstrument: Error processing tag list 0x%x\n", tagList));
			return Result;
		}
	}

DBUGNOTE(("DSPPStartInstrument: FreqType = %d, FreqRequested = %g, PitchNote = %d\n",
	FreqType, FreqRequested, PitchNote ));

/* Calculate exponential amplitude from velocity. */
	if(ampTag == AF_TAG_EXPONENTIAL_VELOCITY)
	{
		Amplitude = Approximate2toX((Velocity - 127)*expVelocityScalar);
	}

/* Scan all FIFOs and select samples for each but don't start them yet.
** We may ultimately select samples based on AF_TAG_FREQUENCY_FP as well. !!!
*/
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		List *AttList = &dins->dins_FIFOControls[i].fico_Attachments;
/* Select Sample based on Pitch. If Pitch or Velocity still -1 then it will ignore them. */
		ChosenAAtt = ScanMultiSamples( AttList, PitchNote, Velocity );
/* Save selection, may be NULL */
		dins->dins_FIFOControls[i].fico_CurrentAttachment = ChosenAAtt;
DBUGNOTE(("DSPPStartInstrument: ChosenAAtt = 0x%x for fifo #%d\n", ChosenAAtt, i ));
	}

/* Set starting frequency. */
/* !!! doesn't deal w/ multiple FIFOs */
	if( FreqType != FREQ_TYPE_DEFAULT )
	{
		dsppApplyStartFreq( ains, FreqType, FreqRequested, PitchNote, ChosenAAtt );
	}

DBUGNOTE(("DSPPStartInstrument: Finished frequency setting.\n"));

/* Scan all FIFOs and start selected samples. */
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		ChosenAAtt = dins->dins_FIFOControls[i].fico_CurrentAttachment;

		if(ChosenAAtt != NULL)
		{
DBUGNOTE(("DSPPStartInstrument: start 0x%lx\n", ChosenAAtt));
DBUGNOTE(("DSPPStartInstrument: channel =  0x%lx\n", ChosenAAtt->aatt_Channel));
			DSPPStartSampleAttachment( ChosenAAtt, TRUE );
		}
		else
		{
/* If we don't start it then we should clear FIFO of old junk. 960617 */
			int32 DMAChan;
			int32 ri = dins->dins_FIFOControls[i].fico_RsrcIndex;
			DMAChan = DSPPGetResourceChannel( &dins->dins_Resources[ri] ); /* Correct 940713 */
			dsphResetFIFO( DMAChan );
		}
	}

	{
		AudioKnob *aknob = &dins->dins_AmplitudeKnob;
DBUGNOTE(("DSPPStartInstrument: ampTag = %d\n", ampTag));
DBUGNOTE(("DSPPStartInstrument: aknob->aknob_DeviceInstrument =0x%x\n", aknob->aknob_DeviceInstrument));
		if((aknob->aknob_DeviceInstrument != 0) && ampTag != TAG_END)
		{
			DSPPResource *drsc = dsppKnobToResource(aknob);
DBUGNOTE(("DSPPStartInstrument: Set amplitude to %g\n", Amplitude));
			dsppSetKnob( drsc, 0, Amplitude, drsc->drsc_SubType );
		}
	}


/* Start all envelopes. */
	{
		List *AttList = &dins->dins_EnvelopeAttachments;
		Node *n = FirstNode( AttList );
		while ( ISNODE(AttList,n))
		{
			AudioAttachment *aatt = (AudioAttachment *) n;
			SetEnvAttTimeScale( aatt, PitchNote, TimeScale );
			n = NextNode(n);
			if ((aatt->aatt_Flags & AF_ATTF_NOAUTOSTART) == 0)
			{
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPStartInstrument: env start 0x%lx\n", aatt));
				DSPPStartEnvAttachment( aatt );
			}
		}
	}

/* Initialize Instrument I Memory for start */
	Result = DSPPInitInsMemory( dins, DINI_F_AT_START );
	if (Result != 0) return Result;

/* Link code into execution chain of DSPP. */
	Result = DSPPStartCodeExecution( dins );

TRACER(TRACE_INT,TRACE_DSP|TRACE_NOTE,("DSPPStartInstrument returns 0\n"));

	return Result;
}

/*****************************************************************/
int32 DSPPPauseInstrument( DSPPInstrument *dins )
{
/* Stop the executing DSP code. */
	if( dins->dins_Specialness > 0 )
	{
		ERR(("DSPPPauseInstrument: called specialness = %d\n", dins->dins_Specialness ));
		return AF_ERR_BADPRIV;
	}

	return DSPPStopCodeExecution( dins );
}
/*****************************************************************/
int32 DSPPResumeInstrument( DSPPInstrument *dins )
{
/* Link code into execution chain. */
	if( dins->dins_Specialness > 0 )
	{
		ERR(("DSPPResumeInstrument: called specialness = %d\n", dins->dins_Specialness ));
		return AF_ERR_BADPRIV;
	}
	return DSPPStartCodeExecution( dins );
}

/********************************************************************/
/* M2 DMA uses Sample Count instead of byte count. */
static int32 dsppConvertFramesToCount( int32 Frames, AudioSample *asmp, uint8 subType )
{
	int32 Cnt;

/* Calculate number of bytes. */
	Cnt = Frames * asmp->asmp_Channels * asmp->asmp_Width /
	          asmp->asmp_CompressionRatio;

/* If not hardware decompressed, then sample count is byteCnt/2   */
	if( (subType != DRSC_INFIFO_SUBTYPE_SQS2) &&
	    (subType != DRSC_INFIFO_SUBTYPE_8BIT) )
	{
		Cnt = Cnt>>1;
	}
DBUG(("dsppConvertFramesToCount: Frames = %d, Cnt = %d\n", Frames, Cnt));
	return Cnt;
}

/********************************************************************
**  Calculate DMA values for startup.
**
**   Loop? Rels? => Do
**     Y     x      Set Cur and Next to This
**     N     Y      Set Cur to This, Set Next to RlsLoop
**     N     N      Set Cur to This, Set Next to NULL
*/
/*******************************************************************/
int32	DSPPCalcSampleStart ( AudioAttachment *aatt, SampleDMA *sdma )
{
	AudioSample *asmp;

	asmp = (AudioSample *) aatt->aatt_Structure;

/* Calculate starting address. */
	if(aatt->aatt_StartAt)
	{
		sdma->sdma_Address = (AudioGrain *) (((char *) asmp->asmp_Data) +
			CvtFrameToByte(aatt->aatt_StartAt, asmp));
	}
	else
	{
		sdma->sdma_Address = (AudioGrain *) asmp->asmp_Data;
	}

	if (asmp->asmp_SustainBegin >= 0)
	{
/* Sustain Loop */
		sdma->sdma_Count = dsppConvertFramesToCount(asmp->asmp_SustainEnd - aatt->aatt_StartAt, asmp, aatt->aatt_SubType );
		sdma->sdma_NextAddress = (AudioGrain *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainBegin, asmp));
		sdma->sdma_NextCount  = dsppConvertFramesToCount((asmp->asmp_SustainEnd -
						asmp->asmp_SustainBegin), asmp, aatt->aatt_SubType );
	}
	else if (asmp->asmp_ReleaseBegin >= 0)
	{
/* No Sustain Loop, but a Release Loop */
		sdma->sdma_Count      = dsppConvertFramesToCount(asmp->asmp_ReleaseEnd - aatt->aatt_StartAt, asmp, aatt->aatt_SubType );
		sdma->sdma_NextAddress = (AudioGrain *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_ReleaseBegin, asmp));
		sdma->sdma_NextCount  = dsppConvertFramesToCount((asmp->asmp_ReleaseEnd -
						asmp->asmp_ReleaseBegin), asmp, aatt->aatt_SubType );
	}
	else
	{
/* No Loop, play whole thing. */
		sdma->sdma_Count      =  dsppConvertFramesToCount(asmp->asmp_NumFrames - aatt->aatt_StartAt, asmp, aatt->aatt_SubType );
		sdma->sdma_NextAddress = NULL;
		sdma->sdma_NextCount = 0;
	}

/* Prevent negative counts that can occur if StartAt is beyond loop end! 941024 */
/* Prevent negative counts that can occur if SustainBegin == SustainEnd! 941101 */
DBUG(("sdma->sdma_Count = %d\n", sdma->sdma_Count ));
	if( (int32) sdma->sdma_Count < 0 )
	{
		sdma->sdma_Address = (AudioGrain *)(((char *)sdma->sdma_Address) + sdma->sdma_Count);

/* Prevent sample access out of range. 941101 */
		if( (uint32) sdma->sdma_Address < (uint32) asmp->asmp_Data )
		{
			sdma->sdma_Address = (AudioGrain *)asmp->asmp_Data;
		}
		sdma->sdma_Count = 0;
	}

/* Prevent negative counts that can occur if ReleaseBegin == ReleaseEnd 941101 */
DBUG(("sdma->sdma_NextCount = %d\n", sdma->sdma_NextCount ));
	if( (int32) sdma->sdma_NextCount < 0 )
	{
		sdma->sdma_NextAddress = (AudioGrain *)(((char *)sdma->sdma_NextAddress) + sdma->sdma_NextCount);
/* Prevent sample access out of range. 941101 */
		if( (uint32) sdma->sdma_NextAddress < (uint32) asmp->asmp_Data )
		{
			sdma->sdma_NextAddress = (AudioGrain *)asmp->asmp_Data; /* Prevent sample access out of range. */
		}
		sdma->sdma_NextCount = 0;
	}

	return(0);
}

/********************************************************************
**  Start playing sample.
**  Setup DMA channel to play sample and loop if appropriate.
**  IfFullStart means that the DMA is starting from a dead stop
**      so we need to set initial DMA pointers. Otherwise we
**      just update the Next addresses and Interrupts.
*/
/*******************************************************************/
int32	DSPPStartSampleAttachment ( AudioAttachment *aatt, int32 IfFullStart )
{
	AudioSample *asmp;
	SampleDMA SDMA;
	AudioDMAControl *admac;
	int32 DMAChan;

#ifdef PARANOID
	ItemStructureParanoid(aatt, AUDIO_ATTACHMENT_NODE,
		"DSPPStartSampleAttachment");
#endif

	asmp = (AudioSample *) aatt->aatt_Structure;

#ifdef PARANOID
	ItemStructureParanoid(asmp, AUDIO_SAMPLE_NODE,
		"DSPPStartSampleAttachment");
#endif

	DMAChan = aatt->aatt_Channel;

#ifdef PARANOID
	if((DMAChan < 0) || (DMAChan >= DSPI_NUM_DMA_CHANNELS))
	{
		ERR(("Paranoid: DSPPStartSampleAttachment: DMAChan = %d !!\n", DMAChan));
	}
#endif

	if (asmp->asmp_Data == NULL)
	{
		ERR(("Sample has NULL data address.\n"));
		return AF_ERR_NULLADDRESS;
	}
	if (asmp->asmp_NumBytes < MIN_DMA_COUNT)
	{
		ERR(("Sample has NumBytes = 0.\n"));
		return AF_ERR_OUTOFRANGE;
	}

/* Stop if not already. 941121 */
/* AND with IfFullStart. 950711 */
	if( (aatt->aatt_ActivityLevel > AF_STOPPED) && IfFullStart )
	{
		DSPPStopSampleAttachment( aatt );
	}

	aatt->aatt_SegmentCount = 0;  /* 931130 Clear initially */
	admac = &DSPPData.dspp_DMAControls[DMAChan];
	admac->admac_AttachmentItem = aatt->aatt_Item.n_Item;
#if 0
#define REPORT_ADMAC(name,member) PRT(("admac->%s = 0x%lx\n", name, admac->member));
	REPORT_ADMAC("admac_ChannelAddr", admac_ChannelAddr);
	REPORT_ADMAC("admac_NextCountDown", admac_NextCountDown);
	REPORT_ADMAC("admac_NextCount", admac_NextCount);
	REPORT_ADMAC("admac_SignalCountDown", admac_SignalCountDown);
	REPORT_ADMAC("admac_Signal", admac_Signal);
	REPORT_ADMAC("admac_AttachmentItem", admac_AttachmentItem);
#endif

/* Figure out sample addresses, return in SDMA. */
	DSPPCalcSampleStart( aatt, &SDMA );

	if( IfFullStart )
	{

/* start full dma */
		dsphDisableDMA( DMAChan );
		dsphResetFIFO( DMAChan );
		dsphSetInitialDMA( DMAChan, SDMA.sdma_Address, SDMA.sdma_Count);
DBUG(("Start ATT: 0x%lx, DMAChan = 0x%lx\n", aatt, DMAChan ));
	}

DBUG(("SSA: NA=0x%lx, NC=0x%lx\n", SDMA.sdma_NextAddress, SDMA.sdma_NextCount));
	if( SDMA.sdma_NextAddress != NULL )
	{
/* Set loop registers to be picked up on next dma completion. */
		dsphSetNextDMA( DMAChan, SDMA.sdma_NextAddress, SDMA.sdma_NextCount, DSPH_F_DMA_LOOP );
	}
	else
	{
		AudioAttachment *nextaatt;
/* No loop so set up silence or scratch for later DMA. */
		dsphSetNextDMA( DMAChan, dsphGetSilenceAddress( DMAChan ), DSPH_SILENCE_SIZE, 0 );
		aatt->aatt_SegmentCount = 1; /* 931130 */

/* No loop so go ahead and setup next attachment if there is one. */
		nextaatt = afi_CheckNextAttachment( aatt );
		if(nextaatt)
		{
			DSPPNextAttachment( nextaatt );
		}
		EnableAttSignalIfNeeded(aatt);
	}

	if( IfFullStart ) dsphEnableDMA( DMAChan );

	aatt->aatt_ActivityLevel = AF_STARTED;
	return(0);
}

/********************************************************************/
void DSPP_SilenceDMA( int32 DMAChan )
{
	AudioGrain *Ptr = dsphGetSilenceAddress(DMAChan);
	dsphSetInitialDMA( DMAChan, Ptr, DSPH_SILENCE_SIZE );
	dsphSetNextDMA( DMAChan, Ptr, DSPH_SILENCE_SIZE, DSPH_F_DMA_LOOP );

}
/********************************************************************/
void DSPP_SilenceNextDMA( int32 DMAChan )
{
	AudioGrain *Ptr = dsphGetSilenceAddress(DMAChan);
DBUG(("DSPP_SilenceNextDMA: channel = 0x%x\n", DMAChan));
	dsphSetNextDMA(DMAChan, Ptr, DSPH_SILENCE_SIZE, DSPH_F_DMA_LOOP);
}

/********************************************************************/
void dsppQueueSilenceDMA( int32 DMAChan )
{
	AudioGrain *Ptr = dsphGetSilenceAddress(DMAChan);
DBUG(("dsppQueueSilenceDMA: Ptr = 0x%08x\n", Ptr));
	SetDMANextInt(DMAChan, Ptr, DSPH_SILENCE_SIZE);
}

/********************************************************************/
/* Queue up for interrupt */
void DSPPQueueAttachment( AudioAttachment *aatt )
{
	SampleDMA SDMA;

	DSPPCalcSampleStart( aatt, &SDMA );
DBUG(("DSPPQueueAttachment: sdma_Address = 0x%08x\n", SDMA.sdma_Address));
	if( SDMA.sdma_Address != NULL )
	{
		SetDMANextInt( aatt->aatt_Channel, SDMA.sdma_Address, SDMA.sdma_Count );
	}
}

/********************************************************************/
/* Make this Attachment the next DMA block. */
void DSPPNextAttachment( AudioAttachment *aatt )
{
	SampleDMA SDMA;

	DSPPCalcSampleStart( aatt, &SDMA );

	if( SDMA.sdma_Address != NULL )
	{
		dsphSetNextDMA ( aatt->aatt_Channel, SDMA.sdma_Address, SDMA.sdma_Count, DSPH_F_DMA_LOOP );
		if( SDMA.sdma_NextAddress != NULL )
		{
DBUG(("DSPPNextAttachment: sdma_NextAddress = 0x%08x\n", SDMA.sdma_NextAddress));
			SetDMANextInt ( aatt->aatt_Channel, SDMA.sdma_NextAddress, SDMA.sdma_NextCount );
		}
	}

DBUG(("DSPPNextAttachment: 0x%lx,0x%lx\n", SDMA.sdma_Address, SDMA.sdma_Count));
}

/*******************************************************************/
AudioAttachment *afi_CheckNextAttachment( AudioAttachment *aatt )
{
	AudioAttachment *nextaatt;

/* Is there a valid next attachment? */
	if ( aatt->aatt_NextAttachment )
	{
		nextaatt = (AudioAttachment *)CheckItem(aatt->aatt_NextAttachment,
			AUDIONODE, AUDIO_ATTACHMENT_NODE);
		if (nextaatt == NULL)
		{
/* Just eliminate the reference. */
			aatt->aatt_NextAttachment = 0;
		}
	}
	else
	{
		nextaatt = NULL;
	}
	return nextaatt;
}

/********************************************************************
**  Set for "release" portion of sample.
**
** YES Release Loop............
**  If there is a release loop, then the next attachment will never be reached.
**  Rels? = Is there a gap between the end of the sustain loop and start of release loop?
**   Next? Loop? Rels? => Do
**     N     Y     Y      Set Next to Release Phase, Queue Release Loop
**     N     N     x      Set Next to Release Loop
**     N     Y     N      Set Next to Release Loop
**
** NO Release Loop............
**  Rels? = Is there a gap between the end of the sustain loop and last frame?
**   Next? Loop? Rels? => Do
**     Y     Y     Y      Set Next to Release Phase, Queue NextAttachment
**     N     Y     Y      Set Next to Release Phase, Queue Silence
**
**     Y     N     x      Set Next to NextAttachment, Queue whatever
**     Y     Y     N      Set Next to NextAttachment, Queue whatever
**
**     N     N     x      Set Next to Silence
**     N     Y     N      Set Next to Silence
**

*/
/*  Pseudocode ...................
	if(release loop)
		calc rels?
		if(sustain loop and rels?)
			Set Next to Release Phase
			Queue Release Loop, SC=0
		else
			Set Next to Release Loop, SC=0
		endif
	else
		calc rels?
		if(sustain loop and rels?)
			Set Next to Release Phase
			if(nextatt)
				Queue NextAttachment, SC=2
			else
				Queue Silence, SC=2
			endif
		else
			if(nextatt)
				Set Next to NextAttachment, Queue whatever, SC=1
			else
				Set Next to Silence, SC=1
			endif
		endif
	endif
*/
int32 DSPPReleaseSampleAttachment( AudioAttachment *aatt )
{
	AudioSample *asmp;
	AudioAttachment *nextaatt;
	AudioGrain *Addr;
	int32 Cnt;
	AudioGrain *RlsLoopAddr;
	int32 RlsLoopCnt;
	int32 DMAChan;

DBUG(("DSPPReleaseSampleAttachment: 0x%08x\n", aatt ));

	asmp = (AudioSample *) aatt->aatt_Structure;
	if ((AudioSample *) asmp->asmp_Data == NULL)
	{
		ERR(("Sample has NULL data address.\n"));
		return AF_ERR_NULLADDRESS;
	}

	nextaatt = afi_CheckNextAttachment( aatt );
	DMAChan = aatt->aatt_Channel;

	if (asmp->asmp_ReleaseBegin >= 0)    /* Release Loop? */
	{
DBUG(("DSPPReleaseSampleAttachment: Set for Release Loop\n" ));
		RlsLoopAddr = (AudioGrain *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_ReleaseBegin, asmp));
		RlsLoopCnt  = dsppConvertFramesToCount((asmp->asmp_ReleaseEnd -
						asmp->asmp_ReleaseBegin), asmp, aatt->aatt_SubType );
/* Prevent negative loop size from hanging DMA. 941101 */
		if( RlsLoopCnt < 0 ) RlsLoopCnt = 0;

/* Does this sample loop and have release gap? */
		if ((asmp->asmp_SustainBegin >= 0) &&   /* 940203 was >0 */
			(asmp->asmp_SustainEnd != asmp->asmp_ReleaseEnd) &&
			(asmp->asmp_SustainEnd != asmp->asmp_ReleaseBegin) &&
			(asmp->asmp_SustainBegin != asmp->asmp_ReleaseEnd) &&
			(asmp->asmp_SustainBegin != asmp->asmp_ReleaseBegin))
		{
/* Set Next to Release Phase */
			Addr = (AudioGrain *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainEnd, asmp));
			Cnt  = dsppConvertFramesToCount((asmp->asmp_ReleaseEnd -
						asmp->asmp_SustainEnd), asmp, aatt->aatt_SubType );

/* Queue Release Loop, SC=0 */
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetNextDMA(DMAChan, Addr, Cnt, DSPH_F_DMA_LOOP);
			}
DBUG(("DSPPReleaseSampleAttachment: RlsLoopAddr = 0x%08x\n", RlsLoopAddr));
			SetDMANextInt ( DMAChan, RlsLoopAddr, RlsLoopCnt );
		}
		else
		{
/* Set Next to Release Loop, SC=0 */
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetNextDMA(DMAChan, RlsLoopAddr, RlsLoopCnt, DSPH_F_DMA_LOOP);
			}
		}
		aatt->aatt_SegmentCount = 0;
	}
	else /* NO release loop */
	{
		Cnt  = dsppConvertFramesToCount((asmp->asmp_NumFrames -
				asmp->asmp_SustainEnd), asmp, aatt->aatt_SubType );

DBUG(("DSPPReleaseSampleAttachment: No Release Loop, Cnt after loop = %d\n", Cnt ));
		aatt->aatt_SegmentCount = 1;
/* Does this sample loop and have release gap? */
		if ((asmp->asmp_SustainBegin >= 0) && (Cnt >= MIN_DMA_COUNT))
		{
/* Set Next to Release Phase */
			Addr = (AudioGrain *) ((char *)asmp->asmp_Data +
					CvtFrameToByte(asmp->asmp_SustainEnd, asmp));
			if( aatt->aatt_ActivityLevel > AF_RELEASED )
			{
				dsphSetNextDMA(DMAChan, Addr, Cnt, DSPH_F_DMA_LOOP);
				aatt->aatt_SegmentCount = 2;
			}

			if(nextaatt)
			{
DBUG(("DSPPReleaseSampleAttachment: Queue attachment.\n", Cnt ));
				DSPPQueueAttachment( nextaatt );
			}
			else
			{
DBUG(("DSPPReleaseSampleAttachment: Queue silence.\n", Cnt ));
				dsppQueueSilenceDMA( DMAChan );
			}
		}
		else
		{
			if(nextaatt)
			{
DBUG(("DSPPReleaseSampleAttachment: next attachment.\n", Cnt ));
				DSPPNextAttachment( nextaatt );
			}
			else
			{
DBUG(("DSPPReleaseSampleAttachment: next silence.\n", Cnt ));
				DSPP_SilenceNextDMA( aatt->aatt_Channel );
			}
		}

/*
** Only enable interrupts if there is NO release loop. Release loops go forever
** so there is no point in interrupting.  It will never signal app.
*/
		EnableAttSignalIfNeeded( aatt );
	}

	aatt->aatt_ActivityLevel = AF_RELEASED;
	return(0);
}

#ifdef DEBUG
/******************************************************************
** Dump nodes in a list.
** 940908 PLB Rewrote so it doesn't crash on empty list.
******************************************************************/
void DumpList ( const List *theList )
{
	const Node *n;

	n = (Node *) FirstNode( theList );
	while (ISNODE( theList, (Node *) n))
	{
		PRT(("Node = 0x%lx\n", n));
		n = (Node *) NextNode((Node *)n);
	}
}
#endif

/******************************************************************
** Determine addresses before and after instrument in list.
**
** This routine has knowledge of how .dsp instruments are terminated.
** They end in _SLEEP  _NOP so we have to subtract 2 from size
** to index to where _SLEEP is.
** The _SLEEP and _NOP are setup by }ins in dspp_asm.fth
******************************************************************/
#define InsExitAddress(dins) (dins->dins_EntryPoint + dins->dins_DSPPCodeSize - 2)
#define InsEntryAddress(dins) (dins->dins_EntryPoint)

static void dsppGetPrevNextAddresses( DSPPInstrument *dins, DSPPCodeList *dcls,
		int32 *PrevAddressPtr, int32 *NextAddressPtr )
{
	List *RunningList;
	DSPPInstrument *FirstDins, *LastDins, *NextDins, *PrevDins;

	RunningList = &dcls->dcls_InsList;

DBUGLIST(("dsppGetPrevNextAddresses: dcls = 0x%x\n", dcls ));
	LastDins = (DSPPInstrument *) LASTNODE(RunningList);
	if (LastDins != dins )  /* nodes after this one so jump to next */
	{
		NextDins = (DSPPInstrument *) NEXTNODE(dins);
		*NextAddressPtr = InsEntryAddress(NextDins);
	}
	else
	{
		*NextAddressPtr = dcls->dcls_ReturnAddress;
	}

	FirstDins = (DSPPInstrument *) FIRSTNODE(RunningList);
	if (FirstDins != dins )  /* nodes before this one so make previous jump to new instrument */
	{
		PrevDins = (DSPPInstrument *) PREVNODE(dins);
		*PrevAddressPtr = InsExitAddress(PrevDins);
	}
	else
	{
		*PrevAddressPtr = dcls->dcls_JumpAddress;
	}
DBUGLIST(("dsppGetPrevNextAddresses: PrevAddress = 0x%x, NextAddress = 0x%x\n", *PrevAddressPtr, *NextAddressPtr ));
}

static void dsppJumpToAddress ( int32 FromAddress, int32 ToAddress )
{
DBUG(("dsppJumpToAddress( 0x%x -> 0x%x)\n", FromAddress, ToAddress ));
	dsphWriteCodeMem( FromAddress, (DSPN_OPCODE_JUMP + ToAddress) );
}

/******************************************************************
** Link Code into execution chain.
** This should only be called from DSPPStartCodeExecution() since it
** manages the dins_ActivityLevel flag.
******************************************************************/
static Err DSPPLinkCodeToStartList( DSPPInstrument *dins, DSPPCodeList *dcls )
{
	List *RunningList;
	int32 NextAddress = -1, PrevAddress = -1;

	RunningList = &dcls->dcls_InsList;

/* If list empty, add to head.
** if list has one instrument, jump first to new, add after head.
** If list has two or more, jump new to second, jump first to new,
**	add after head.
*/
TRACEB(TRACE_INT,TRACE_DSP|TRACE_NOTE,("DSPPLinkCodeToStartList: Priority = %d\n", dins->dins_Node.n_Priority));
DBUGLIST(("DSPPLinkCodeToStartList: Priority = %d\n", dins->dins_Node.n_Priority));
	InsertNodeFromHead ( RunningList, (Node *) dins );

/* Link DSP code execution in same order as it appears in the host list.
** First make this instrument point to existing which is safe cuz this is
** not executing yet.
*/
	dsppGetPrevNextAddresses( dins, dcls, &PrevAddress, &NextAddress );
	dsppJumpToAddress ( InsExitAddress(dins), NextAddress );

/* DSP CAN START EXECUTING INST HERE!!!!! */
	dsppJumpToAddress ( PrevAddress, InsEntryAddress(dins) );

#ifdef DEBUG
	PRT(("DSPPLinkCodeToStartList list--------\n"));
	DumpList(RunningList);
#endif

	return 0;
}


/*****************************************************************/
static Err DSPPUnlinkCodeToStopList( DSPPInstrument *dins, DSPPCodeList *dcls )
{
	int32 NextAddress = -1, PrevAddress = -1;

	dsppGetPrevNextAddresses( dins, dcls, &PrevAddress, &NextAddress );

/* jump over code by patching from previous code to next code. */
	dsppJumpToAddress ( PrevAddress, NextAddress );

#ifdef DEBUG
	PRT(("DSPPUnlinkCodeToStopList list--- BEFORE RemNode -----\n"));
	DumpList(&dcls->dcls_InsList);
#define Dump3UInt32(msg,p) \
{ uint32 *xp; xp=(uint32 *)p; \
PRT(("%s : 0x%x = ", msg,xp)); \
PRT(("0x%x", *xp++)); \
PRT((", 0x%x", *xp++)); \
PRT((", 0x%x\n", *xp++)); \
}
	Dump3UInt32("Prev", *(((uint32 *) dins)+1));
	Dump3UInt32("Dins", dins);
	Dump3UInt32("Next", *((uint32 *) dins));
#endif

	ParanoidRemNode ( (Node *) dins );

#ifdef DEBUG
	PRT(("DSPPUnlinkCodeToStopList list--- AFTER RemNode -----\n"));
	DumpList(&dcls->dcls_InsList);
#endif
	return 0;
}

/******************************************************************
** Get actual DMA channel for a particular resource.
** Translate for output channels.
******************************************************************/
static int32 DSPPGetResourceChannel( DSPPResource *drsc )
{
	int32 Result = AF_ERR_BADOFX;

	if( drsc->drsc_Type == DRSC_TYPE_OUT_FIFO)
	{
		Result = drsc->drsc_Allocated;
	}
	else if( drsc->drsc_Type == DRSC_TYPE_IN_FIFO)
	{
		Result = drsc->drsc_Allocated;
	}
DBUG(("DSPPGetResourceChannel: Channel = 0x%x\n", Result ));
	return Result;
}

/*****************************************************************/
int32 DSPPReleaseInstrument( DSPPInstrument *dins, const TagArg *tagList)
{
	AudioAttachment *aatt;
	int32 i, ri, chan, Result;
	float32 TimeScale = 1.0;
	int32 IfTimeScaleSet = FALSE;
	Node *n;
	Item  AttItem;

/* Process tags */
	{
		const TagArg *tag;

		for (Result = SafeFirstTagArg (&tag, tagList); Result > 0; Result = SafeNextTagArg (&tag))
		{
DBUGNOTE(("DSPPReleaseInstrument: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
			switch (tag->ta_Tag)
			{
			case AF_TAG_TIME_SCALE_FP:
				TimeScale = ConvertTagData_FP(tag->ta_Arg);
				IfTimeScaleSet = TRUE;
				break;

			default:
				ERR(("ReleaseInstrument: Unrecognized tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
				return AF_ERR_BADTAG;
			}
		}

			/* Catch tag processing errors */
		if (Result < 0) {
			ERR(("ReleaseInstrument: Error processing tag list 0x%x\n", tagList));
			return Result;
		}
	}

/* Scan all sample attachments and release them. */
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		ri = dins->dins_FIFOControls[i].fico_RsrcIndex;  /* %Q Got to be a better way. */
/*		chan = dins->dins_Resources[ri].drsc_Allocated; Did not work properly for OutputDMA 940713 */
		chan = DSPPGetResourceChannel( &dins->dins_Resources[ri] ); /* Correct 940713 */

		AttItem = DSPPData.dspp_DMAControls[chan].admac_AttachmentItem;
		aatt = (AudioAttachment *) CheckItem( AttItem,  AUDIONODE, AUDIO_ATTACHMENT_NODE);;
DBUG(("DSPPReleaseInstrument: release aatt = 0x%lx\n", aatt ));
		if (aatt) {
			Result = DSPPReleaseSampleAttachment( aatt );
			if (Result < 0) break;
		}
	}

/* Release all started envelopes. */
	n = FirstNode( &dins->dins_EnvelopeAttachments );
	while ( ISNODE(&dins->dins_EnvelopeAttachments,n))
	{
		aatt = (AudioAttachment *) n;
		if(IfTimeScaleSet) SetEnvAttTimeScale( aatt, -1, TimeScale );
		n = NextNode(n);
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPReleaseInstrument: env start 0x%lx\n", aatt));
		if( aatt->aatt_ActivityLevel == AF_STARTED)
		{
			DSPPReleaseEnvAttachment( aatt );
		}
	}

	return Result;
}

/********************************************************************
** Stop execution on DSPP of instrument.
** Handle special case of nanokernel.dsp.
********************************************************************/
static Err DSPPStopCodeExecution( DSPPInstrument *dins )
{
	int32 Result = 0;

DBUGLIST(("DSPPStopCodeExecution( dins = 0x%x )\n", dins ));

DBUGLIST(("DSPPStopCodeExecution: dins->dins_ActivityLevel = %d\n", dins->dins_ActivityLevel ));
	if (dins->dins_ActivityLevel <= AF_STOPPED) return 0; /* 941031 */

DBUGLIST(("DSPPStopCodeExecution: dins->dins_Specialness = %d\n", dins->dins_Specialness ));
	switch( dins->dins_Specialness )
	{
		case AF_SPECIAL_NOT:   /* Most common. */
			Result = DSPPUnlinkCodeToStopList( dins, DSPPSelectRunningList( dins ) );
			break;

		case AF_SPECIAL_KERNEL:
DBUGLIST(("DSPPStopCodeExecution: calling dsphHalt()\n" ));
			dsphHalt();   /* We're out of here!  Why waste code cleaning up. */
			break;

		default:
			Result = AF_ERR_SPECIAL;
ERR(("DSPPStopCodeExecution: Specialness hosed = 0x%x, dins = 0x%x\n",  dins->dins_Specialness, dins ));
			break;
	}

	dins->dins_ActivityLevel = AF_STOPPED;

	return Result;
}

#if (CHECK_BEFORE_DISABLE == 1)
#define FIFO_TRIGGER_LEVEL   (4)
static void dsphWaitForChannelService( int32 chan )
{
/* Assumes that DSPP is no longer accessing FIFO
** Make sure channel is not requesting service.
** Read DMA:
**    (Count == 0) || ((Count > 0) && (Depth > 4)) => DONE
** Write DMA:
**    (Count == 0) || ((Count > 0) && (Depth < 4)) => DONE
*/
	int32 Mask;
	vuint32  *CountPtr, *StatusPtr;
	int32 i, Depth;

	Mask = 1 << chan;
	if( (ReadHardware(DSPX_CHANNEL_ENABLE) & Mask) == 0 ) return; /* !!! Do we need this? */

	CountPtr = DSPX_DMA_STACK + (chan<<2) + DSPX_DMA_COUNT_OFFSET;
	StatusPtr = &DSPX_DATA_MEMORY[DSPI_FIFO_CONTROL(chan)];
DBUG(("dsphWaitForChannelService: chan = 0x%x, CountPtr = 0x%x, StatusPtr = 0x%x\n",
		chan, CountPtr, StatusPtr ));
	for( i=0; i<1000; i++ )
	{
/* !!! need to also check NextValid */
		if( ReadHardware(CountPtr) == 0 ) return;
		Depth = ReadHardware( StatusPtr );
		if( ReadHardware(DSPX_CHANNEL_DIRECTION_SET) & Mask )
		{
			if( Depth < FIFO_TRIGGER_LEVEL ) return;  /* Write mode DMA */
		}
		else
		{
			if( Depth > FIFO_TRIGGER_LEVEL ) return;  /* Read mode DMA */
		}
	}
	ERR(("dsphWaitForChannelService: Timed Out!!\n"));
}
#endif

/*****************************************************************/
int32 DSPPStopInstrument( DSPPInstrument *dins, const TagArg *tagList )
{
	AudioAttachment *aatt;
	int32 i, ri, chan, Result;
	Node *n;
	Item AttItem;

DBUGNOTE(("DSPPStopInstrument: >>>>>>>>>>>>>>>>>>>>>>\n"));

	TOUCH(tagList); /* to eliminate warning */

/* Stop the executing DSP code. */
/* 921216 %Q WARNING! - the code could still be running if caught in middle! */
	Result = DSPPStopCodeExecution( dins );
	if( Result < 0 ) goto error;

/* Scan all sample attachments and stop them. */
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		ri = dins->dins_FIFOControls[i].fico_RsrcIndex;  /* Got to be a better way. */

/*		chan = dins->dins_Resources[ri].drsc_Allocated; Did not work properly for OutputDMA 940713 */
		chan = DSPPGetResourceChannel( &dins->dins_Resources[ri] ); /* Correct 940713 */

#if (CHECK_BEFORE_DISABLE == 1)
		dsphWaitForChannelService( chan );
#endif
		AttItem = DSPPData.dspp_DMAControls[chan].admac_AttachmentItem;
DBUGNOTE(("DSPPStopInstrument, i = %d, ri = %d, ", i, ri ));
		aatt = (AudioAttachment *) CheckItem( AttItem,  AUDIONODE, AUDIO_ATTACHMENT_NODE);

DBUGNOTE(("   chan=0x%lx, AttItem = 0x%x, aatt = 0x%x\n", chan, AttItem, aatt));
		if (aatt)
		{
#if 0
			if( chan != aatt->aatt_Channel)
			{
				ERR(("PARANOID trap in DSPPStopInstrument, chan = %d, aatt->aatt_Channel = %d\n",
					chan, aatt->aatt_Channel));
				return -1;
			}
			PRT(("DSPPStopInstrument: valid aatt on %d, chan=0x%lx\n", i, chan));
#endif
			Result = DSPPStopSampleAttachment( aatt );
			if (Result < 0) break;
		}
		else
		{
/* This was added because ProcessDMASignal sometimes clears admac_AttachmentItem. */
DBUGNOTE(("DSPPStopInstrument: no aatt, disable channel 0x%lx\n", chan ));
			dsphDisableDMA(chan);
		}
	}

/* Stop all started envelopes. */
	n = FirstNode( &dins->dins_EnvelopeAttachments );
	while ( ISNODE(&dins->dins_EnvelopeAttachments,n))
	{
		aatt = (AudioAttachment *) n;
		n = NextNode(n);
TRACEB(TRACE_INT, TRACE_ENVELOPE|TRACE_NOTE, ("DSPPStopInstrument: env stop 0x%lx\n", aatt));
		DSPPStopEnvAttachment( aatt );
	}

DBUGNOTE(("DSPPStopInstrument: <<<<<<<<<<<<<<<<<<<<<<<<<\n"));
error:
	return Result;
}

/*******************************************************************/
int32	DSPPStopSampleAttachment( AudioAttachment *aatt )
{
	int32 DMAChan;
	AudioDMAControl *admac;

#ifdef PARANOID
	AudioAttachment *xaatt;
	xaatt = (AudioAttachment *) CheckItem(aatt->aatt_Item.n_Item,
				AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (xaatt == NULL)
	{
		ERR(("DSPPStopSampleAttachment: Attachment dead: 0x%lx\n", aatt->aatt_Item.n_Item));
		return AF_ERR_BADITEM;
	}
#endif

	if(aatt->aatt_ActivityLevel <= AF_STOPPED) return 0;

	DMAChan = aatt->aatt_Channel;
	DisableAttachmentSignal( aatt );
DBUG(("Stop ATT: 0x%x @ 0x%lx, DMAChan = 0x%lx\n", aatt->aatt_Item.n_Item, aatt, DMAChan ));
	dsphDisableDMA(DMAChan);
	admac = &DSPPData.dspp_DMAControls[DMAChan];
	admac->admac_AttachmentItem = 0;
	aatt->aatt_ActivityLevel = AF_STOPPED;
	ClearDelayLineIfStopped ((AudioSample *)aatt->aatt_Structure);

	return(0);
}


/*****************************************************************/
/*
	Find FIFO Control in instrument with matching name.

	Arguments
		dins
			DSPPInstrument containing list of FIFOControls to scan.
			The instrument can have 0 of these.

		hookName
			Name of sample hook to find. Can be NULL, which returns
			the only FIFO if there is only one. This results in
			AF_ERR_BAD_NAME if there are more than one.

		fico
			Pointer to buffer to receive FIFOControl pointer of
			located FIFO.

	Results
		0 on success, Err code on failure.
		Sets *fico to located FIFOControls on success. Not set on failure.
*/
Err dsppFindFIFOByName (const DSPPInstrument *dins, const char *hookName, FIFOControl **ficoPtrPtr)
{
	if (!hookName) {
			/* If hookName is NULL, return only FIFO present. Error if more than one FIFO. */
		if (dins->dins_NumFIFOs != 1) return AF_ERR_BAD_NAME;

		*ficoPtrPtr = &dins->dins_FIFOControls[0];
		return 0;
	}
	else {
		int32 rsrcIndex;

			/* Find resource index of hookName. */
		if ((rsrcIndex = DSPPFindResourceIndex (dins->dins_Template, hookName)) < 0) return rsrcIndex;

			/* Find FIFOControl based on resource index. Handle NULL return because
			** the above can return a rsrcIndex which isn't a FIFO. */
		return (*ficoPtrPtr = dsppFindFIFOByRsrcIndex (dins, rsrcIndex)) ? 0 : AF_ERR_BAD_PORT_TYPE;
	}
}

/*****************************************************************/
/*
	Find FIFOControl in instrument with matching resource index.

	Arguments
		dins
			DSPPInstrument containing list of FIFOControls to scan.
			The instrument can have 0 of these.

		rsrcIndex
			Resource index to match with fico_RsrcIndex.

	Results
		Pointer to FIFOControl on success, NULL if not found.
*/
FIFOControl *dsppFindFIFOByRsrcIndex (const DSPPInstrument *dins, int32 rsrcIndex)
{
	int32 i;

		/* Scan dins_FIFOControls array for matching fico_RsrcIndex */
	for (i=0; i<dins->dins_NumFIFOs; i++) {
		FIFOControl * const fico = &dins->dins_FIFOControls[i];

		if (fico->fico_RsrcIndex == rsrcIndex) return fico;
	}

	return NULL;
}

/*****************************************************************/
int32 DSPPAttachSample( DSPPInstrument *dins, AudioSample *asmp, AudioAttachment *aatt)
{
	int32 Result;
	FIFOControl *fico = NULL;
	char *FIFOName;

TRACEE(TRACE_INT,TRACE_SAMPLE,("DSPPAttachSample ( dins=0x%lx, asmp=0x%lx)\n",
	dins, asmp));
	FIFOName = aatt->aatt_HookName;

	if (FIFOName)
	{
		TRACEE(TRACE_INT,TRACE_SAMPLE,(" %s)\n", FIFOName));
	}

	Result = dsppFindFIFOByName( dins, FIFOName, &fico );
	if( Result < 0 ) return Result;

	{
		DSPPResource *drsc = &dins->dins_Resources[fico->fico_RsrcIndex];
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPAttachSample: fico->fico_RsrcIndex =  0x%lx\n",
		fico->fico_RsrcIndex));
		if( drsc->drsc_Type == DRSC_TYPE_OUT_FIFO)
		{
			if(!IsDelayLine(asmp))
			{
				ERR(("DSPPAttachSample: Must use DelaySample for Output DMA\n"));
				Result = AF_ERR_SECURITY;
				goto error;
			}
		}
		aatt->aatt_Channel = DSPPGetResourceChannel( drsc ); /* 940713 */

#ifdef PARANOID
		if((aatt->aatt_Channel < 0) || (aatt->aatt_Channel > DSPI_NUM_DMA_CHANNELS))
		{
			ERR(("Paranoid: DSPPAttachSample: aatt_Channel = %d !!\n", aatt->aatt_Channel));
		}
#endif
/* Set FIFO SubType so we know how to calculate frames based on compression ratios. */
		aatt->aatt_SubType = drsc->drsc_SubType;

TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPAttachSample: aatt_Channel =  0x%lx\n", aatt->aatt_Channel));
		AddTail(&fico->fico_Attachments, (Node *)aatt);
	}

error:
	return Result;
}

/*******************************************************************************/
/******* DSP SWIs **************************************************************/
/*******************************************************************************/

/*****************************************************************/
static void PatchCode16 (DSPPInstrument *dins_dst, uint16 offset, uint16 value)
{
	dsphWriteCodeMem (dins_dst->dins_EntryPoint + offset, value);
    DBUG(("PatchCode16: Code[$%03lx+$%03lx] = $%04lx\n", dins_dst->dins_EntryPoint, offset, value));
}

#define DSPP_CON_MOVE_TICKS      (2)
#define DSPP_CON_MOVE_RATESHIFT  (1)
#define DSPP_CON_MOVE_CODESIZE   (2)
#define DSPP_CON_MOVE_INDEX_TO_NMEM(Indx) (DSPI_CODE_MEMORY_SIZE - 3 - ((Indx)*DSPP_CON_MOVE_CODESIZE))

/*****************************************************************
** Initialize MOVE array by putting RTS at end.
*/
static Err dsppInitConnectionMoves( void )
{
	int32 Result;

	DSPPData.dspp_NumMoveConnections = 0;

/* Allocate top code memory location.  */
	Result = dsppAllocResourceHere( DRSC_TYPE_CODE, DSPI_CODE_MEMORY_SIZE - 1, 1 );
	if( Result < 0 ) return Result;

/* Put RTS at high memory. */
	dsphWriteCodeMem( DSPI_CODE_MEMORY_SIZE - 1, DSPN_OPCODE_RTS );
	DBUG(("dsppInitConnectionMoves: put RTS at 0x%x\n", DSPI_CODE_MEMORY_SIZE - 1 ));
	return 0;
}

/*****************************************************************
** Look for Move Index in list of connections and change it.
*/
static Err dsppCallConnectionMoves( int32 MoveIndex )
{
	int32 MoveAddress, CallAddress, CodeValue;

/* Calculate NMEM address of MOVE */
	MoveAddress = DSPP_CON_MOVE_INDEX_TO_NMEM(MoveIndex);

/* Find address of JSR in "m2_head.dsp" that we need to patch. !!! Optimize, do at init.*/
	CallAddress = dsppImportResource("CallConnectionMoves");
	if( CallAddress < 0 ) return AF_ERR_BADOFX;

DBUGCON(("dsppCallConnectionMoves: call 0x%x from 0x%x\n", MoveAddress, CallAddress ));

/* Point JSR to MOVE block. */
	CodeValue =  DSPN_OPCODE_JSR | ((~DSPN_OPCODEMASK_BRANCH) & MoveAddress);
	dsphWriteCodeMem( CallAddress, CodeValue);
	return 0;
}
/* Move this to dspptouch !!! */
#define dsphReadCodeMem(DSPI_Addr) ReadHardware (DSPX_CODE_MEMORY + (DSPI_Addr))

/*******************************************************************
** Look for Move Index in list of connections and change it.
*/
static void dsppChangeMoveIndex( int32 OldMoveIndex, int32 NewMoveIndex )
{
	AudioConnectionNode *acnd;
	Node *n;

DBUGCON(("dsppChangeMoveIndex: OldMoveIndex = %d, NewMoveIndex = %d\n", OldMoveIndex, NewMoveIndex ));

	n = (Node *) FirstNode( &AB_FIELD(af_ConnectionList) );
	while( ISNODE( &AB_FIELD(af_ConnectionList), n) )
	{
		acnd = (AudioConnectionNode *) n;
		if(acnd->acnd_MoveIndex == OldMoveIndex)
		{
			acnd->acnd_MoveIndex = NewMoveIndex;
			break;
		}
	     	n = (Node *) NextNode(n);  /* Get next before we kill this one. */
	}
}

/*****************************************************************/
static Err   dsppDisconnectWithMove( int32 MoveIndex )
{
	int32 MoveAddress, TopMoveAddress, CodeValue;
DBUGCON(("dsppDisconnectWithMove: starting NumConn = %d\n", DSPPData.dspp_NumMoveConnections ));

/* Calculate NMEM address of MOVE */
	MoveAddress = DSPP_CON_MOVE_INDEX_TO_NMEM(MoveIndex);
DBUGCON(("dsppDisconnectWithMove: MoveIndex = 0x%x\n", MoveIndex));
DBUGCON(("dsppDisconnectWithMove: MoveAddress = 0x%x\n", MoveAddress));

/* Are there any MOVES before the one being removed? */
	if( DSPPData.dspp_NumMoveConnections-1 > MoveIndex )
	{
/* Move MOVE from beginning of block to cover MOVE being removed. */
/* This can be done safely in a three step process:
**     Point destination of MOVE being removed to safe R/O area.
**     Replace source of MOVE being removed with source of move being moved.
**     Replace destination of MOVE being removed with source of move being moved.
*/
/* Calculate NMEM address of beginning MOVE */
		int32 TopMoveIndex = DSPPData.dspp_NumMoveConnections-1;
		TopMoveAddress = DSPP_CON_MOVE_INDEX_TO_NMEM(TopMoveIndex);
DBUGCON(("dsppDisconnectWithMove: TopMoveAddress = 0x%x\n", TopMoveAddress ));
		CodeValue =  DSPN_OPCODE_MOVEADDR | DSPI_NOISE;
DBUGCON(("dsphWriteCodeMem: write 0x%x to 0x%x\n", CodeValue, MoveAddress ));
		dsphWriteCodeMem( MoveAddress, CodeValue);

		CodeValue = dsphReadCodeMem( TopMoveAddress+1 );
DBUGCON(("dsphWriteCodeMem: write 0x%x to 0x%x\n", CodeValue, MoveAddress+1 ));
		dsphWriteCodeMem( MoveAddress+1, CodeValue);

		CodeValue = dsphReadCodeMem( TopMoveAddress );
DBUGCON(("dsphWriteCodeMem: write 0x%x to 0x%x\n", CodeValue, MoveAddress ));
		dsphWriteCodeMem( MoveAddress, CodeValue);

/* Find and update MoveIndex of AudioConnectionNode that was moved. */
		dsppChangeMoveIndex( TopMoveIndex, MoveIndex );
	}
	else
	{
		TopMoveAddress = MoveAddress;
	}

/* Free ticks and code memory from beginning of block. */
	dsppFreeTicks( DSPP_CON_MOVE_RATESHIFT, DSPP_CON_MOVE_TICKS );
	dsppFreeResource( DRSC_TYPE_CODE, TopMoveAddress, DSPP_CON_MOVE_CODESIZE );

/* Point JSR in header to call new head of list. */
	DSPPData.dspp_NumMoveConnections--;
	dsppCallConnectionMoves( DSPPData.dspp_NumMoveConnections-1 );

DBUGCON(("dsppDisconnectWithMove: ending NumConn = %d\n", DSPPData.dspp_NumMoveConnections ));
	return 0;
}
/*****************************************************************/
static int32 dsppConnectWithMove( int32 SrcAddress, int32 DstAddress )
{
	int32 MoveIndex, MoveAddress, CodeValue;
	int32 Result;

DBUGCON(("dsppConnectWithMove: starting NumConn = %d\n", DSPPData.dspp_NumMoveConnections ));
/* Check to see if there are two ticks per frame free. */
	Result = dsppAllocTicks( DSPP_CON_MOVE_RATESHIFT, DSPP_CON_MOVE_TICKS );
	if( Result < 0 ) return Result;

/* Check to see if there is room at head of MOVE block. */
	MoveIndex = DSPPData.dspp_NumMoveConnections;
	MoveAddress = DSPP_CON_MOVE_INDEX_TO_NMEM(MoveIndex);
	Result = dsppAllocResourceHere( DRSC_TYPE_CODE, MoveAddress, DSPP_CON_MOVE_CODESIZE );
	if( Result < 0 ) goto freeticks;

/* Add MOVE instruction to move data from source to destination. */
DBUGCON(("dsppConnectWithMove: Write MOVE to code address 0x%x\n", MoveAddress ));
DBUGCON(("dsppConnectWithMove: MOVE from 0x%x to 0x%x\n", SrcAddress, DstAddress ));
	CodeValue =  DSPN_OPCODE_MOVEADDR | ((~DSPN_OPCODEMASK_MOVEADDR) & DstAddress);
DBUGCON(("dsphWriteCodeMem: write 0x%x to 0x%x\n", CodeValue, MoveAddress ));
	dsphWriteCodeMem( MoveAddress, CodeValue);

	CodeValue =  DSPN_OPERAND_ADDR | ((~DSPN_OPERANDMASK_ADDR) & SrcAddress);
DBUGCON(("dsphWriteCodeMem: write 0x%x to 0x%x\n", CodeValue, MoveAddress+1 ));
	dsphWriteCodeMem( MoveAddress+1, CodeValue);

/* Point JSR in header to call new head of list. */
	DSPPData.dspp_NumMoveConnections++;
	dsppCallConnectionMoves( DSPPData.dspp_NumMoveConnections-1 );

DBUGCON(("dsppConnectWithMove: ending NumConn = %d\n", DSPPData.dspp_NumMoveConnections ));
	return MoveIndex;

freeticks:
	dsppFreeTicks( DSPP_CON_MOVE_RATESHIFT, DSPP_CON_MOVE_TICKS );
	return Result;
}

/*****************************************************************/
int32 dsppConnectInstrumentParts ( AudioConnectionNode *acnd,
	DSPPInstrument *dins_src, char *name_src, int32 SrcPart,
	DSPPInstrument *dins_dst, char *name_dst, int32 DstPart )
{
	DSPPTemplate *dtmp_dst;
	DSPPResource *drsc_src, *drsc_dst;
	int32 SrcIndex, DstIndex;
	int32  i, Result=0;
	int32 SrcAddr;
DBUGCON(("dsppConnectInstrumentParts: acnd = 0x%x\n", acnd ));

/* Find source resource then modify existing code at destination to use it. */
DBUGCON(("dsppConnectInstrumentParts ( 0x%lx, %s, %d\n",
	dins_src, name_src, SrcPart ));
DBUGCON(("    0x%lx, %s, %d\n", dins_dst, name_dst, DstPart));

/* Source resource validation. */
	SrcIndex = DSPPFindResourceIndex (dins_src->dins_Template, name_src );
	if (SrcIndex < 0)
	{
DBUGCON(("dsppConnectInstrumentParts: bad source name.\n"));
		return SrcIndex;
	}
	drsc_src = &dins_src->dins_Resources[SrcIndex];
	if(drsc_src->drsc_Type != DRSC_TYPE_OUTPUT)
	{
DBUGCON(("dsppConnectInstrumentParts: bad source type = %d\n", drsc_src->drsc_Type));
		return AF_ERR_NAME_NOT_FOUND;
	}
	if( (SrcPart < 0) || (SrcPart >= drsc_src->drsc_Many) )
	{
DBUGCON(("dsppConnectInstrumentParts: bad source partnum = %d\n", SrcPart));
		return AF_ERR_OUTOFRANGE;
	}

/* Destination resource validation. */
	DstIndex = DSPPFindResourceIndex (dins_dst->dins_Template, name_dst );
	if (DstIndex < 0)
	{
DBUGCON(("dsppConnectInstrumentParts: bad destination name.\n"));
		return DstIndex;
	}
	drsc_dst = &dins_dst->dins_Resources[DstIndex];
	if( (drsc_dst->drsc_Type != DRSC_TYPE_INPUT) && (drsc_dst->drsc_Type != DRSC_TYPE_KNOB) )
	{
DBUGCON(("dsppConnectInstrumentParts: bad dest type = %d\n", drsc_dst->drsc_Type));
		return AF_ERR_NAME_NOT_FOUND;
	}
	if( (DstPart < 0) || (DstPart >= drsc_dst->drsc_Many) )
	{
DBUGCON(("dsppConnectInstrumentParts: bad dest partnum = %d\n", DstPart));
		return AF_ERR_OUTOFRANGE;
	}

	dtmp_dst = dins_dst->dins_Template;

	SrcAddr = dsppGetResourceAttribute( drsc_src, SrcPart);
/* There are two ways to connect instruments.  One is to add a MOVE instruction
** to move data from source to destination.  The other is to change the code based
** on relocation info.
*/
	if( (drsc_dst->drsc_Flags & DRSC_F_BIND) &&
	    ((&dins_dst->dins_Resources[drsc_dst->drsc_BindToRsrcIndex])->drsc_Type == DRSC_TYPE_RBASE) )
	{
		int32 Result;
		int32 DstAddr;
		DstAddr = dsppGetResourceAttribute( drsc_dst, DstPart);
		Result = dsppConnectWithMove( SrcAddr, DstAddr );
		if( Result < 0 ) goto error;
		acnd->acnd_MoveIndex = Result;
	}
	else
	{
/* The full srcOperand is required for both DRSC_TYPE_INPUT and DRSC_TYPE_KNOB. */
		const uint16 srcOperand = DSPN_OPERAND_ADDR | SrcAddr;

/* No MOVE for this connection. */
		acnd->acnd_MoveIndex = -1;
/* Scan for all relocations in destination that use that resource. */
		for (i=0; i<dtmp_dst->dtmp_NumRelocations; i++)
		{
			const DSPPRelocation * const drlc_dst = &dtmp_dst->dtmp_Relocations[i];
			const int32 ri = drlc_dst->drlc_RsrcIndex;

/* Check to see if it matches drsc_dst, and part number. */
			if( (ri == DstIndex) && (DstPart == drlc_dst->drlc_Part) )
			{
DBUGCON(("dsppConnectInstrumentParts: resource match!\n"));
				dsppRelocate (dtmp_dst, drlc_dst, srcOperand, (DSPPFixupFunction)PatchCode16, dins_dst);
			}
		}

	#if DEBUG_Connect
		dsphDisassembleCodeMem (dins_dst->dins_EntryPoint, dins_dst->dins_DSPPCodeSize, "dsppConnectInstrumentParts");
	#endif
	}

/* Save pointers to static names in instrument. Don't save volatile names passed by caller. */
DBUGCON(("dsppConnectInstrumentParts: acnd = 0x%x\n", acnd ));
	if( acnd != NULL )
	{
		acnd->acnd_SrcName = DSPPGetRsrcName( dins_src, SrcIndex );
		acnd->acnd_DstName = DSPPGetRsrcName( dins_dst, DstIndex );
DBUGCON(("dsppConnectInstrumentParts: set SrcName = %s, DstName = %s\n", acnd->acnd_SrcName, acnd->acnd_DstName ));
	}
error:
DBUGCON(("dsppConnectInstrumentParts: Result = 0x%x\n", Result ));
	return Result;
}

/*****************************************************************/
int32 dsppDisconnectInstrumentParts ( AudioConnectionNode *acnd,
	DSPPInstrument *dins_dst, char *name_dst, int32 DstPart)
{
	DSPPTemplate *dtmp;
	DSPPRelocation *drlc;
	DSPPResource *drsc, *drsc_dst;
	int32 DstIndex;
	uint32  val, ri;
	int32  i, Result=0;

DBUGCON(("dsppDisconnectInstrumentParts: name_dst = %s\n", name_dst ));

/* If the connection was made by a MOVE instruction, remove the MOVE. */
	if( acnd->acnd_MoveIndex >= 0 )
	{
		Result = dsppDisconnectWithMove( acnd->acnd_MoveIndex );
		acnd->acnd_MoveIndex = -1;
		return Result;
	}

/* Otherwise remove it by relocation. */
	DstIndex = DSPPFindResourceIndex (dins_dst->dins_Template, name_dst );
	if (DstIndex < 0)
	{
TRACEB(TRACE_INT,TRACE_OFX,("dsppDisconnectInstruments: bad destination name.\n"));
		return DstIndex;
	}
	drsc_dst = &dins_dst->dins_Resources[DstIndex];
	if( (drsc_dst->drsc_Type != DRSC_TYPE_INPUT) && (drsc_dst->drsc_Type != DRSC_TYPE_KNOB) )
	{
TRACEB(TRACE_INT,TRACE_OFX,("dsppDisconnectInstruments: bad destination name.\n"));
		return AF_ERR_NAME_NOT_FOUND;
	}

	dtmp = dins_dst->dins_Template;

/* Scan for all relocations in destination that use that resource. */
	for (i=0; i<dtmp->dtmp_NumRelocations; i++)
	{
		drlc = &dtmp->dtmp_Relocations[i];

/* Get this relocations resource index and do bounds check. */
		ri = drlc->drlc_RsrcIndex;

/* Check to see if resource of dest reloc matches selected resource */
		if( (ri == DstIndex) && (DstPart == drlc->drlc_Part) )
		{
			drsc = &dins_dst->dins_Resources[ri];
TRACEB(TRACE_INT,TRACE_OFX,("DSPPConnectInstruments: resource match!\n"));

				/* Restore to original knob or dummy input. */
			if ((val = dsppGetResourceRelocationValue (drsc, drlc)) < 0)
			{
				Result = val;
				goto error;
			}

DBUGCON(("dsppDisconnectInstrumentParts: Relocate Type = %d, val = $%lx\n", drsc->drsc_Type, val));

			dsppRelocate (dtmp, drlc, val, (DSPPFixupFunction)PatchCode16, dins_dst);
		}
	}

#if DEBUG_Connect
	dsphDisassembleCodeMem (dins_dst->dins_EntryPoint, dins_dst->dins_DSPPCodeSize, "dsppDisconnectInstrumentParts");
#endif

error:
	return Result;
}


/* -------------------- DSPP Instrumentation functions */

	/* DSPI Addresses of system variables to support instrumentation functions */
static uint16 dspi_FrameCountLow;
static uint16 dspi_FrameCountHigh;

static int32 GetInsRsrcValue (Item insitem, const char *rsrcname);

/******************************************************************
**
**  Initialize instrumentation functions. These guys just snoop
**  global variables in m2_head.dsp and m2_tail.dsp. This is called
**  after loading system instruments.
**
**  Results
**
**      Returns 0 on success or Err code on failure.
**      Sets dspi_FrameCount.
**
**  Caveats
**
**      @@@ Requires that system instruments have already been loaded.
**
**      @@@ These functions depend on rather intimate knowledge of
**          nanokernel.dsp
**
******************************************************************/

Err dsphInitInstrumentation (void)
{
	int32 result;

	if ((result = GetInsRsrcValue (AB_FIELD(af_NanokernelInstrument), "FrameCountLow")) < 0) return result;
	dspi_FrameCountLow = (uint16)result;
	DBUGINIT(("dsphInitInstrumentation[M2]: FrameCountLow $%03x\n", dspi_FrameCountLow));
	if ((result = GetInsRsrcValue (AB_FIELD(af_NanokernelInstrument), "FrameCountHigh")) < 0) return result;
	dspi_FrameCountHigh = (uint16)result;
	DBUGINIT(("dsphInitInstrumentation[M2]: FrameCountHigh $%03x\n", dspi_FrameCountHigh));

	return 0;
}

 /**
 |||	AUTODOC -public -class audio -group Miscellaneous -name GetAudioFrameCount
 |||	Gets count of audio frames executed.
 |||
 |||	  Synopsis
 |||
 |||	    uint32 GetAudioFrameCount (void)
 |||
 |||	  Description
 |||
 |||	    Gets 32-bit value that is incremented on every DSP sample frame, typically at
 |||	    44,100 Hz. This is only useful for determining relative elapsed frame
 |||	    counts.
 |||
 |||	  Return Value
 |||
 |||	    32-bit sample frame count.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  Example
 |||
 |||	    NewFrameCount = GetAudioFrameCount();
 |||	    ElapsedFrames = NewFrameCount - OldFrameCount;
 |||	    OldFrameCount = NewFrameCount;
 |||
 |||	  Caveats
 |||
 |||	    The DSP executes asynchronously from the DAC. The fact, therefore,
 |||	    that the DSP frame counter has advanced by N samples does not imply
 |||	    that the DAC has necessarily played exactly N samples. The DSP processes
 |||	    samples in short bursts. GetAudioFrameCount() should, therefore, not
 |||	    be used as a measure of time, but rather a measure of how many frames
 |||	    the DSP has processed.
 |||
 |||	  See Also
 |||
 |||	    GetAudioTime()
 **/

uint32 dsphGetAudioFrameCount( void )
{
	uint32 High0, High1, Low;
	uint32 fc;

/*
** Read High twice until we see that High has not wrapped.
** Then we know that High and Low are coherent.
*/
	do
	{
		High0 = ReadHardware(&DSPX_DATA_MEMORY[dspi_FrameCountHigh]);
		Low = ReadHardware(&DSPX_DATA_MEMORY[dspi_FrameCountLow]);
		High1 = ReadHardware(&DSPX_DATA_MEMORY[dspi_FrameCountHigh]);
	} while( High0 != High1 );

	fc = (High0<<16) | Low;
	return fc;
}

static int32 GetInsRsrcValue (Item insitem, const char *rsrcname)
{
	const AudioInstrument *ains;
	const DSPPInstrument *dins;
	int32 rsrcindex;
	Err errcode;

	if (!(ains = (AudioInstrument *)CheckItem (insitem, AUDIONODE, AUDIO_INSTRUMENT_NODE))) return AF_ERR_BADITEM;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;

	if ((errcode = rsrcindex = DSPPFindResourceIndex (dins->dins_Template, rsrcname)) < 0) return errcode;

	return dins->dins_Resources[rsrcindex].drsc_Allocated;
}
