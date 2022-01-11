#ifndef __DSPP_H
#define __DSPP_H


/* @(#) dspp.h 96/08/15 1.94 */
/* $Id: dspp.h,v 1.87 1995/03/10 20:36:23 peabody Exp phil $ */

/*
** DSP Includes
** By:  Phil Burk
*/

/*
** 921112 PLB Remove OUTPUT RSRC cuz basically just IMEM
** 930210 PLB Add ID_AIFC
** 930215 PLB Moved PRT(x) to audio_internal.h
** 931216 PLB Corrected LAST_EI_MEM to 0x6F, added IsDMATypeInput/Output
** 940228 PLB Added CALC_SHIFT_DIVIDE_SR
** 940608 PLB Added proto for DSPPAdjustTicksPerFrame()
** 940609 PLB Added dtmp_LibraryTemplateRefs
** 940811 PLB Added ezmem_tools.h and table_alloc.h
** 950113 WJB Removed definitions and prototypes that were moved to dspp_touch.h.
** 950116 WJB Removed inclusion of dspp_touch.h - most clients of dspp.h don't need it.
** 950118 WJB Privatized PatchCode16() and DSPPRelocateAll().
** 950118 WJB Groundword changes in DSPPRelocate() to cooperate with abs address remapping.
** 950119 WJB Made dsppRelocate() fixupFunc() callback system more general.
** 950120 WJB Privatized DSPPVerifyDataInitializer(). Added const to DSPPValidateTemplate().
** 950123 WJB Rolled DSPPValidateTemplate() and DSPPCloneTemplate() into dsppCreateSuperTemplate().
** 950124 WJB Added prototype for dsppStripTemplate().
** 950124 WJB Added note about keeping DSPPTemplate and dsppCloneTemplate() in sync.
** 950130 WJB Added dsppRemapAbsoluteAddresses() prototype.
**            Fixed up includes slightly.
** 950131 WJB Added DSPPSumResourceReferences() prototype.
** 950131 WJB Added DSPP_SILICON_ version defines.
**            Added DSPPTemplate.dtmp_Header field in place of dtmp_FunctionID.
** 950206 WJB Moved resource management functions to dspp_resources.h.
** 950207 WJB Added DRSC_TYPE_MASK.
** 950214 WJB Changed drsc_Allocated from uint32 to int32.
** 950220 WJB Added some comments.
** 950220 WJB Renamed DRSC_IMPORT/EXPORT to DRSC_F_IMPORT/EXPORT.
** 950224 WJB Changed int32 drsc_Type into 4 uint8 fields.
** 950224 WJB Removed typedef Allocator and associated prototypes - not used.
** 950224 WJB Replaced DSPPExternal with DSPPExportedResource.
**            Added dins_ExportedResources[] to DSPPInstrument.
**            Retired drsc_References.
**            Renamed dspp_ExternalList to dspp_ExportedResourceList.
** 950227 WJB Added DSPP_MODE_ defines.
** 950301 WJB Moved DSPP_MODE_ defines to dspp_modes.h.
** 950307 WJB Added DSPPTemplate arg to dsppRelocate().
** 950308 WJB Removed dspp_AvailableTicks, dspp_TicksPerFrame, and DSPPAdjustTicksPerFrame().
**            Made dins_ExecFrame be present for DSPP_MODE_OPERA_RESOURCES.
** 950310 WJB Moved dsppRemapAbsoluteAddresses() prototype to dspp_remap.h.
** 950411 WJB Added DRSC_F_BOUND_TO. Updated DSPPResource docs.
** 950412 WJB Changed drsc_BoundRsrc to drsc_BindToRsrcIndex.
** 950413 WJB Added DRSC_FIFO_OSC.
** 950419 WJB Moved DSPP_InitIMemAccess() prototype to dspp_touch_anvil.h to balance dspp_touch_bulldog.h.
** 950420 WJB Moved dsph...() functions to dspp_touch.h.
** 950424 WJB Removed DSPPFindRsrcType() prototype - not defined.
**            Publicized gHeadInstrumentItem from audio_folio.c.
** 950425 WJB Moved dspp_ExportedResourceList to dspp_resources.c.
** 950428 WJB Added DRSC_TRIGGER and DRSC_HW_CPU_INT.
** 950501 WJB Added dsppGetTemplateRsrcName().
** 950501 WJB Added dsppRelocateInstrument().
** 950502 WJB Added dsppGetResourceRelocatationValue().
** 950505 WJB Moved some more stuff here from dspp_addresses.h.
**            Minor tweaks to recover from the great split.
** 950508 WJB Found homes for most of the refugees from dspptouch library.
**            Deleted prototypes for unimplemented functions.
**            Tidied up a bit.
** 950509 WJB Found home for remaining stuff from dspptouch library.
** 950512 WJB Added dsphGetReceivedTriggers() prototype.
** 950512 WJB Added dsphEnable/DisableTriggerInterrupts().
** 950525 WJB Revised for Arm/DisarmTrigger().
** 950711 WJB Removed local definitions of KNOB_TYPEs.
** 950718 WJB Added dsppCreate/DeleteUserTemplate().
** 950724 WJB Added dsppGetDataInitializerImage() and dsppNextDataInitializer().
** 950724 WJB Added dsppCreatePatchTemplate().
** 950724 WJB Added dsppClipKnobValue().
** 950809 WJB Updated DSPPRelocation field names. Cleaned up formatting a bit.
** 950814 WJB Added dsppDataInitializerSize().
** 950824 WJB Updated resource comments.
*/

#include <audio/dspp_template.h>        /* DSPPTemplate and related structures */
#include <dspptouch/dspp_addresses.h>   /* DSPI_NUM_DMA_CHANNELS */
#include <dspptouch/dspp_touch.h>
#include <kernel/list.h>
#include <kernel/types.h>

#include "audio_folio_modes.h"
#include "audio_structs.h"              /* Audio structures (pointed to by a bunch of stuff in here) */


/*******************************************************************/

/*
    !!! inefficient to use this structure for Instrument resource array
        . might be cooler if DSPPExportedResource and DSPPResource were intrinsically
          bound to one another rather than 2 separate allocations
        . don't necessarily need to keep Type/Many in the allocation records:
          can always refer back to the parent Template's resource list for this
          information.
*/

    /* DSPPResource.drsc_Allocated special values */
#define DRSC_ALLOC_INVALID  (-1)    /* Special value for DSPPInstrument's
                                    ** drsc_Allocated indicating no resource allocation */

typedef struct FIFOControl
{
	int32            fico_RsrcIndex;
	AudioAttachment *fico_CurrentAttachment;    /* The Sample Attachment selected when the instrument was started. May be NULL.
	                                            ** @@@ Since current attachment can change while instrument is playing,
	                                            **     this is only really valid during DSPPStartInstrument(). */
	List             fico_Attachments;
} FIFOControl;

typedef struct DSPPInstrument DSPPInstrument;   /* forward reference of typedef (DSPPExportedResource needs this) */

typedef struct DSPPExportedResource  /* Exported resource information */
{
		/* linkage */
	Node                  dexp_Node;            /* Linkage in dspp_exportedResourceList (internal to dspp_resources.c).
	                                               n_Name is resource name (which must be unique) */
		/* exporter */
	const DSPPInstrument *dexp_Owner;           /* instrument containing exported resource */
	int32                 dexp_ResourceIndex;   /* index into instrument's DSPPResource table of exported resource */
} DSPPExportedResource;

struct DSPPInstrument   /* typedef'ed above */
{
	Node          dins_Node;
	uint8         dins_ActivityLevel;
	uint8         dins_RateShift; /* execution rate shifted right 940811 */
	uint8         dins_pad0;                        /* unused byte when !defined(AF_API_OPERA) */
	uint8         dins_Specialness;  /* for kernel */
	DSPPTemplate *dins_Template;
	int32         dins_NumResources;
	DSPPResource *dins_Resources;
	int32         dins_NumFIFOs;
	FIFOControl  *dins_FIFOControls;     /* Contains references to attached samples. */
	int32         dins_EntryPoint;
	int32         dins_DSPPCodeSize;
/* Standard knob for StartInstrument */
	AudioKnob     dins_AmplitudeKnob;
	List          dins_EnvelopeAttachments;     /* List of direct attachments. */
	int32         dins_NumExportedResources;        /* Number of entries in dins_ExportedResources[]. Can be 0. */
	DSPPExportedResource *dins_ExportedResources;   /* Array of exported resources. NULL if none. */
};

typedef struct DSPPCodeList
{
	int32    dcls_JumpAddress;    /* Address to patch to jump into list. */
	int32    dcls_ReturnAddress;  /* Address to return to list. */
	List     dcls_InsList;
} DSPPCodeList;

/* Structure of actual hardware DMA register block, 931220 made volatile */
typedef volatile struct DMARegisters
{
	AudioGrain *dmar_Address;
	uint32     dmar_Count;
	AudioGrain *dmar_NextAddress;
	uint32     dmar_NextCount;
} DMARegisters;

/* Used for passing around the results of sample DMA calculations. */
typedef struct SampleDMA
{
	AudioGrain *sdma_Address;
	uint32     sdma_Count;
	AudioGrain *sdma_NextAddress;
	uint32     sdma_NextCount;
} SampleDMA;

typedef struct AudioDMAControl
{
	DMARegisters *admac_ChannelAddr;    /* Address of DMA Channel */
	int32       admac_NextCountDown;    /* set DMA Next registers at zero, used to count loops */
	AudioGrain *admac_NextAddress;      /* Address of sample memory. */
	uint32      admac_NextCount;        /* number of bytes */
	int32       admac_SignalCountDown;  /* signal foreground at zero */
	int32       admac_Signal;           /* used to signal foreground from interrupt, set once */
	Item        admac_AttachmentItem;   /* Item of attachment currently playing. Used when releasing or stopping instrument */
} AudioDMAControl;

typedef struct DSPPStatic
{
	float32 dspp_SampleRate;
	AudioDMAControl dspp_DMAControls[DSPI_MAX_DMA_CHANNELS];
	int32 dspp_NumMoveConnections; /* Number of MOVE instructions in ConnectInstrument MOVE block. */
	DSPPCodeList  dspp_FullRateInstruments; /* 1/1 running DSPPInstruments.  Only used to link code. */
	DSPPCodeList  dspp_HalfRateInstruments;
	DSPPCodeList  dspp_EighthRateInstruments;
} DSPPStatic;

/**********************************************************************/

    /* Chunk IDs of .dsp (FORM DSPP) file */
#define ID_DSPP MAKE_ID('D','S','P','P')    /* FORM ID */
#define ID_DCOD MAKE_ID('D','C','O','D')    /* DCOD chunk: DSPPCodeHeader[] + code image */
#define ID_DHDR MAKE_ID('D','H','D','R')    /* DHDR chunk: DSPPHeader */
#define ID_DINI MAKE_ID('D','I','N','I')    /* DINI chunk: { DSPPDatatInitializer + data image }[] packed array */
#define ID_DLNK MAKE_ID('D','L','N','K')    /* DLNK chunk: Dynamic Link Names packed string array */
#define ID_DNMS MAKE_ID('D','N','M','S')    /* DNMS chunk: Resource Names packed string array */
#define ID_DRLC MAKE_ID('D','R','L','C')    /* DRLC chunk: DSPPRelocation[] */
#define ID_MRSC MAKE_ID('M','R','S','C')    /* MRSC chunk: DSPPResource[] */


/*Data Structures *************************************************/

extern DSPPStatic DSPPData;


/* Prototypes ****************************************************/

	/* init/term (dspp_instr.c) */
Err    DSPP_Init (void);
void   DSPP_Term (void);

	/* supervisor DSPPTemplate create/delete */
Err dsppPromoteTemplate (DSPPTemplate **resultTemplate, const DSPPTemplate *srcTemplate);
DSPPTemplate *dsppCreateSuperTemplate (void);
void dsppDeleteSuperTemplate (DSPPTemplate *);

#define DSPPGetRsrcName(dins,indx) dsppGetTemplateRsrcName ((dins)->dins_Template, (indx))
int32  DSPPAllocInstrument( DSPPTemplate *dtmp, DSPPInstrument **DinsPtr, int32 RateShift);
int32  DSPPAttachSample( DSPPInstrument *dins, AudioSample *samp, AudioAttachment *aatt);
int32  dsppConnectInstrumentParts ( AudioConnectionNode *acnd, DSPPInstrument *dins_src, char *name_src, int32 SrcPart,DSPPInstrument *dins_dst, char *name_dst, int32 DstPart );
int32  dsppCreateKnobProbe(AudioKnob *aknob, DSPPInstrument *dins, char *knobname, int32 legalType);
Err    dsppCreateMixerTemplate( DSPPTemplate **dtmpPtr, MixerSpec mixerSpec );
int32  dsppDeleteKnob( AudioKnob *aknob );
int32  DSPPDetachSample( DSPPInstrument *dins, AudioSample *samp, char *FIFOName);
int32  dsppDisconnectInstrumentParts ( AudioConnectionNode *acnd, DSPPInstrument *dins_dst, char *name_dst, int32 DstPart);
void   dsppFreeInstrument( DSPPInstrument *dins );
int32  DSPPInitInsMemory( DSPPInstrument *dins, int32 AT_Mask );
Err    dsppLoadInsTemplate (DSPPTemplate **resultUserDTmp, const char *insName);
int32  DSPPPauseInstrument( DSPPInstrument *dins );
int32  dsppSetKnob( DSPPResource *drsc, int32 PartNum, float32 val, int32 type );
int32  DSPPReleaseAttachment( AudioAttachment *aatt);
int32  DSPPReleaseEnvAttachment( AudioAttachment *aatt );
int32  DSPPReleaseInstrument( DSPPInstrument *dins, const TagArg *tagList );
int32  DSPPReleaseSampleAttachment ( AudioAttachment *aatt);
int32  DSPPResumeInstrument( DSPPInstrument *dins );
int32  DSPPStartEnvAttachment( AudioAttachment *aatt );
void   dsppApplyStartFreq( AudioInstrument *ains, int32 FreqType, float32 FreqRequested, int32 PitchNote, AudioAttachment *ChosenAAtt );
int32  DSPPStartInstrument( AudioInstrument *ains, const TagArg *tagList );
int32  DSPPStartSampleAttachment ( AudioAttachment *aatt, int32 IfFullStart );
int32  DSPPStopEnvAttachment( AudioAttachment *aatt );
int32  DSPPStopInstrument( DSPPInstrument *dins, const TagArg *tagList );
int32  DSPPStopSampleAttachment( AudioAttachment *aatt);
void   DSPP_SilenceDMA( int32 DMAChan );
void   DSPP_SilenceNextDMA( int32 DMAChan );

    /* resource lookup */
Err    dsppFindFIFOByName (const DSPPInstrument *, const char *hookName, FIFOControl **ficoPtrPtr);
FIFOControl *dsppFindFIFOByRsrcIndex (const DSPPInstrument *, int32 rsrcIndex);
DSPPResource *DSPPFindResource (const DSPPInstrument *, int32 rsrcType, const char *name);
#define DSPPFindResourceIndex dsppFindResourceIndex /* old name for compatibility (!!! remove eventually) */
DSPPResource *dsppKnobToResource(AudioKnob *aknob);

    /* dspp_irq_{anvil,bulldog}.c */
Err    InitAudioDMA (void);
uint32 dsphGetCompletedDMAChannels (void);
void   dsphEnableTriggerInterrupts (uint32 TriggerMask);
void   dsphDisableTriggerInterrupts (uint32 TriggerMask);
void   dsphClearTriggerInterrupts (uint32 TriggerMask);
uint32 dsphGetReceivedTriggers (uint32 ReadTriggerMask);
Err    InitDSPPInterrupt (void);
void   TermDSPPInterrupt (void);

    /* dspp_signals.c */
float32 dsppGetSignalMin( int32 signalType, int32 rateShift );
float32 dsppGetSignalMax( int32 signalType, int32 rateShift );
#define dsppConvertSignalToGeneric(signalType,rateShift,signalValue) \
	ConvertAudioSignalToGeneric ((signalType), 1<<(rateShift), (signalValue))
#define dsppConvertGenericToSignal(signalType,rateShift,genericValue) \
	ConvertGenericToAudioSignal ((signalType), 1<<(rateShift), (genericValue))
float32 dsppConvertRawToGeneric( int32 signalType, int32 rawValue );
float32 dsppConvertRawToSignal( int32 signalType, int32 rateShift, int32 rawValue );

    /* dspp_relocator.c */
int32  dsppGetResourceRelocationValue (const DSPPResource *, const DSPPRelocation *);
Err    dsppValidateRelocation (const DSPPTemplate *, const DSPPRelocation *);
Err    dsppRelocateInstrument (DSPPInstrument *, DSPPCodeHeader *codehunk);

    /* dspp_timer.c */
void dsppSetWakeupFrame( uint32 WakeupFrame );
uint32 dsppGetCurrentFrameCount( void );

    /* low level DSPP instrumentation functions (dspp_instr.c) */
Err dsphInitInstrumentation (void);
uint32 dsphGetAudioFrameCount( void );

    /* debug */
#ifdef DEBUG
	void DumpList ( const List *theList );
#endif


/*****************************************************************************/

#endif  /* __DSPP_H */
