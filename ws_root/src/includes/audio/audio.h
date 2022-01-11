#ifndef __AUDIO_AUDIO_H
#define __AUDIO_AUDIO_H


/****************************************************************************
**
**  @(#) audio.h 96/09/11 1.115
**  $Id: audio.h,v 1.94 1995/03/14 06:39:17 phil Exp phil $
**
**  Audio Folio Includes
**
**  NOTE: Please consider the contents of all Audio Folio data structures as PRIVATE.
**  Use GetAudioItemInfo() and SetAudioItemInfo() to change these structures.
**  We reserve the right to change the definition of these structures.
**
****************************************************************************/

#ifndef EXTERNAL_RELEASE
/*
** 930103 PLB Removed include of inthard.h
** 930208 PLB Added support for new Audio Timer, AudioCue
** 930210 PLB Added support for compressed samples
** 930527 PLB Changed PurgeInstruments to ScavengeInstrument
** 930604 PLB Add envelopes.
** 930614 PLB Moved structures to private file. Made tags enums.
** 940304 WJB Cleaned up includes.
** 940419 WJB Restored #include <> back to "" in order to keep examples
**            building until new compiler is released.
** 940427 PLB Added DRSC_LEFT_ADC and other hardware resources for Anvil
** 940506 PLB Added Get/SetAudioFolioInfo() and tags
** 940607 WJB Added AudioTime macros.
** 940609 PLB Added AF_TAG_USED_BY
** 940614 PLB Added AF_TAG_INTERNAL_1
** 940713 WJB Added varargs glue function prototypes.
** 941011 WJB Made ReservedAudioSWI1(), TestHack(), PrivateInternal(),
**            and IncrementGlobalIndex() internal-only functions.
** 941011 WJB Removed FindAudioDevice(), ControlAudioDevice(), GetNumInstruments(),
**            Get/SetMasterTuning(), and DSPGetMaxTicks().
** 941018 WJB Added SendAttachmentVA().
** 941024 PLB Changed AF_TAG_CALCRATE_SHIFT to AF_TAG_CALCRATE_DIVIDE
** 941116 PLB Removed SendAttachment
** 941121 PLB Added AF_ERR_TAGCONFLICT;
** 950308 WJB Added AF_TAG_DMA_GRANULARITY.
** 950313 WJB Added AF_ERR_BAD_DSP_CODE and AF_ERR_BAD_DSP_ABSOLUTE.
** 950313 PLB Made GetAudioTime a SWI.
** 950414 WJB Added AF_ERR_BAD_RSRC_BINDING.
** 950515 WJB Added MonitorTrigger() prototype. Commented out AUDIOSWI.
** 950525 WJB Changed MonitorTrigger() to Arm/DisarmTrigger() pair.
** 950525 WJB Added AF_F_TRIGGER_ defines.
**            Tidied slightly.
** 950531 WJB Changed some redundant non-standard errors into standard errors.
** 950627 PLB Changed Knob and Connect prototypes to M2 style.
** 950628 PLB Removed include of iff_fs_tools.h. MAKE_ID now in types.h
** 950711 WJB Removed AF_TAG_DMA_GRANULARITY. Not needed with new compatibility strategy.
** 950711 WJB Added AF_SIGNAL_TYPEs and AF_PORT_TYPEs.
** 950711 WJB Added NEXTPATCHCMD function number and AF_TAG_PATCH_CMDS.
** 950717 PLB Added CreateMixerTemplate() and MakeMixerSpec()
** 950721 WJB Added PatchCmd parser errors.
** 950725 WJB Retired SleepAudioTicks().
** 950807 WJB Added AF_ERR_PORT_NOT_USED.
** 950920 PLB Converted AF_KNOB_TYPE_... to AF_SIGNAL_TYPE_...
** 950921 DLD Protected include of audio_obsolete.h
** 960105 WJB Added resource query stuff.
** 960310 DLD Fixed protection of the include of audio_signals.h
*/
#endif

#ifndef __AUDIO_AUDIO_SIGNALS_H
#include <audio/audio_signals.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TAGS_H
#include <kernel/tags.h>        /* convenience: ConvertFP_TagData() */
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/**********************************************************************/
/**************************** Constants  ******************************/
/**********************************************************************/

/* This unique number matches number assigned by kernel. */
#define AUDIONODE   4
#define AUDIOFOLIONAME "audio"

/*
** Item type numbers for Audio Folio
*/
#define AUDIO_TEMPLATE_NODE    (1)
#define AUDIO_INSTRUMENT_NODE  (2)
#define AUDIO_KNOB_NODE        (3)
#define AUDIO_SAMPLE_NODE      (4)
#define AUDIO_CUE_NODE         (5)
#define AUDIO_ENVELOPE_NODE    (6)
#define AUDIO_ATTACHMENT_NODE  (7)
#define AUDIO_TUNING_NODE      (8)
#define AUDIO_PROBE_NODE       (9)
#define AUDIO_CLOCK_NODE       (10)

/* Instrument Flags */
#define AF_INSF_AUTOABANDON     (0x0001)    /* Abandon when stopped. */
#define AF_INSF_LEGALFLAGS      (AF_INSF_AUTOABANDON)

/* Envelope Flags */
#define AF_ENVF_LOCKTIMESCALE   (0x0001)
#define AF_ENVF_FATLADYSINGS    (0x0002)
#define AF_ENVF_LEGALFLAGS      (AF_ENVF_LOCKTIMESCALE | AF_ENVF_FATLADYSINGS)

/* Attachment Flags */
#define AF_ATTF_NOAUTOSTART     (0x0001)
#define AF_ATTF_FATLADYSINGS    (0x0002)
#define AF_ATTF_LEGALFLAGS      (AF_ATTF_NOAUTOSTART | AF_ATTF_FATLADYSINGS)

/* Sample Flags */
#define AF_SAMPF_CLEARONSTOP    (0x0001)
#define AF_SAMPF_LEGALFLAGS     (AF_SAMPF_CLEARONSTOP)

/* Special Cue Values for Attachments */
#define CUE_AT_LOOP    (-1)
#define CUE_AT_END     (-2)

/* Trigger Flags */
#define AF_F_TRIGGER_RESET      (0x01)
#define AF_F_TRIGGER_CONTINUOUS (0x02)

/* Misc Defines */
#define AF_MIDDLE_C_PITCH       (60)
#define AF_A440_PITCH           (69)
#define AF_DEFAULT_CLOCK_RATE   (44100.0/184.0)

/* Status levels for Instruments and Attachments */
#define AF_ABANDONED     (0)
#define AF_STOPPED       (1)
#define AF_RELEASED      (2)
#define AF_STARTED       (3)

/* Used by MakeMixerSpec() */
#define AF_MIXER_MIN_INPUTS   (1)
#define AF_MIXER_MAX_INPUTS  (32)
#define AF_MIXER_MIN_OUTPUTS  (1)
#define AF_MIXER_MAX_OUTPUTS  (8)
#define AF_F_MIXER_WITH_AMPLITUDE  (0x01)
#define AF_F_MIXER_WITH_LINE_OUT   (0x02)
#define AF_MIXER_LEGAL_FLAGS  (AF_F_MIXER_WITH_AMPLITUDE | AF_F_MIXER_WITH_LINE_OUT)

/* Standard part numbers for use with stereo connections */
#define AF_PART_LEFT    0
#define AF_PART_RIGHT   1

/* Instrument Port types (patch construction and port query) */
typedef enum AudioPortType {
    AF_PORT_TYPE_INPUT,
    AF_PORT_TYPE_OUTPUT,
    AF_PORT_TYPE_KNOB,
    AF_PORT_TYPE_IN_FIFO,
    AF_PORT_TYPE_OUT_FIFO,
    AF_PORT_TYPE_TRIGGER,
    AF_PORT_TYPE_ENVELOPE
#ifndef EXTERNAL_RELEASE
    , AF_PORT_TYPE_MANY             /* # of port types defined here (private) */
#define AF_PORT_TYPE_MAX (AF_PORT_TYPE_MANY-1)
#endif
} AudioPortType;

/* Signal Types */
#define AF_SIGNAL_TYPE_GENERIC_SIGNED      AUDIO_SIGNAL_TYPE_GENERIC_SIGNED
#define AF_SIGNAL_TYPE_GENERIC_UNSIGNED    AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED
#define AF_SIGNAL_TYPE_OSC_FREQ            AUDIO_SIGNAL_TYPE_OSC_FREQ
#define AF_SIGNAL_TYPE_LFO_FREQ            AUDIO_SIGNAL_TYPE_LFO_FREQ
#define AF_SIGNAL_TYPE_SAMPLE_RATE         AUDIO_SIGNAL_TYPE_SAMPLE_RATE
#define AF_SIGNAL_TYPE_WHOLE_NUMBER        AUDIO_SIGNAL_TYPE_WHOLE_NUMBER
#ifndef EXTERNAL_RELEASE
	#define AF_SIGNAL_TYPE_MANY        AUDIO_SIGNAL_TYPE_MANY
	#define AF_SIGNAL_TYPE_MAX         (AUDIO_SIGNAL_TYPE_MANY-1)
#endif

/* Resource types used for instrument and system-wide resource queries */
typedef enum AudioResourceType {
    AF_RESOURCE_TYPE_TICKS,         /* DSP ticks */
    AF_RESOURCE_TYPE_CODE_MEM,      /* words of DSP code memory */
    AF_RESOURCE_TYPE_DATA_MEM,      /* words of DSP data memory */
    AF_RESOURCE_TYPE_FIFOS,         /* FIFOs */
    AF_RESOURCE_TYPE_TRIGGERS       /* Triggers */
#ifndef EXTERNAL_RELEASE
    , AF_RESOURCE_TYPE_MANY
#define AF_RESOURCE_TYPE_MAX (AF_RESOURCE_TYPE_MANY-1)
#endif
} AudioResourceType;

/* Maximum SIZE of audiofolio name including NUL terminator, as in sizeof(name). */
#define AF_MAX_NAME_SIZE    (32)
/* Maximum LENGTH of audiofolio name NOT including NUL terminator, as in strlen(name). */
#define AF_MAX_NAME_LENGTH  (AF_MAX_NAME_SIZE - 1)

#ifndef EXTERNAL_RELEASE
/* !!! hide internal tags in #ifndef EXTERNAL_RELEASE -
       don't forget to replace them for EXTERNAL_RELEASE w/ some place holder */
#endif
enum audio_folio_tags
{
							/* 10 */
	XAF_TAG_AMPLITUDE = TAG_ITEM_LAST+1,     /* !!! unused */
	XAF_TAG_RATE,                /* !!! unused */
	AF_TAG_NAME,
	AF_TAG_MASTER,              /* (Item) */
	AF_TAG_PITCH,
	AF_TAG_VELOCITY,
	AF_TAG_TEMPLATE,
	AF_TAG_INSTRUMENT,
	AF_TAG_SLAVE,
	AF_TAG_MIN_FP,
							/* 20 */
	AF_TAG_MAX_FP,
	XAF_TAG_DEFAULT_FP,         /* !!! unused */
	AF_TAG_WIDTH,
	AF_TAG_CHANNELS,
	AF_TAG_FRAMES,
	AF_TAG_BASENOTE,
	AF_TAG_DETUNE,
	AF_TAG_LOWNOTE,
	AF_TAG_HIGHNOTE,
	AF_TAG_LOWVELOCITY,
							/* 30 */
	AF_TAG_HIGHVELOCITY,
	AF_TAG_SUSTAINBEGIN,
	AF_TAG_SUSTAINEND,
	AF_TAG_RELEASEBEGIN,
	AF_TAG_RELEASEEND,
	AF_TAG_NUMBYTES,
	AF_TAG_ADDRESS,
	AF_TAG_SQUARE_VELOCITY,
	AF_TAG_EXPONENTIAL_VELOCITY,
	AF_TAG_PRIORITY,
							/* 40 */
	AF_TAG_SET_FLAGS,
	AF_TAG_CLEAR_FLAGS,
	XAF_TAG_FREQUENCY,          /* !!! unused */
	AF_TAG_EXP_VELOCITY_SCALAR,
	XAF_TAG_HOOKNAME,           /* !!! unused */
	AF_TAG_START_AT,
	XAF_TAG_SAMPLE_RATE,        /* !!! unused */
	AF_TAG_COMPRESSIONRATIO,
	AF_TAG_COMPRESSIONTYPE,
	AF_TAG_NUMBITS,
							/* 50 */
	AF_TAG_NOTESPEROCTAVE,
	XAF_TAG_BASEFREQ,           /* !!! unused */
	AF_TAG_SUSTAINTIME_FP,
	AF_TAG_RELEASETIME_FP,
	XAF_TAG_MICROSPERUNIT,      /* !!! unused */
	XAF_TAG_DATA_OFFSET,        /* !!! unused */
	AF_TAG_DELAY_LINE_TEMPLATE, /* (int32) */
	AF_TAG_DELAY_LINE,          /* (int32) */
	AF_TAG_RELEASEJUMP,
	AF_TAG_CURRENT,
							/* 60 */
	AF_TAG_STATUS,
	AF_TAG_TIME_SCALE_FP,       /* (float32) */
	AF_TAG_START_TIME,
	AF_TAG_AUTO_FREE_DATA,      /* (bool) */
	AF_TAG_AUTO_DELETE_SLAVE,   /* (bool) */
	AF_TAG_SUPPRESS_WRITE_BACK_D_CACHE, /* (bool) */
	XAF_TAG_ALLOC_FUNCTION, /* !!! unused */
	XAF_TAG_FREE_FUNCTION, /* !!! unused */
	XAF_TAG_SCAN, /* !!! unused */
#ifdef EXTERNAL_RELEASE
	AF_TAG_INTERNAL69,
#else
	AF_TAG_SHARE,               /* (void) */
#endif
							/* 70 */
	XAF_TAG_USED_BY,            /* !!! unused */
	XAF_TAG_INTERNAL_1,         /* !!! unused */
	AF_TAG_CALCRATE_DIVIDE,     /* For reduced execution rate. */
	AF_TAG_SPECIAL,             /* For internal use only. */
	XAF_TAG_PATCH_CMDS,         /* !!! unused */
	AF_TAG_TYPE,
	AF_TAG_RATE_FP,
	AF_TAG_FREQUENCY_FP,
	AF_TAG_AMPLITUDE_FP,
	AF_TAG_SAMPLE_RATE_FP,
							/* 80 */
	AF_TAG_BASEFREQ_FP,         /* (float32) Base frequency of sample */
	AF_TAG_DETUNE_FP,
#ifdef EXTERNAL_RELEASE
	AF_TAG_INTERNAL82
#else
	AF_TAG_MIXER_SPEC           /* (MixerSpec) Specify that a mixer be created from scratch. (!!! will go away) */
#endif
};

/**********************************************************************/
/************************** Error Returns *****************************/
/**********************************************************************/

#define MAKEAERR(svr,class,err) MakeErr(ER_FOLI,ER_ADIO,svr,ER_E_SSTM,class,err)

/* Standard errors returned from audiofolio */
#define AF_ERR_BADITEM          MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define AF_ERR_BADPRIV          MAKEAERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define AF_ERR_BADPTR           MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define AF_ERR_BADTAG           MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define AF_ERR_BADTAGVAL        MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define AF_ERR_NOMEM            MAKEAERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define AF_ERR_BADSUBTYPE       MAKEAERR(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define AF_ERR_NOSIGNAL         MAKEAERR(ER_SEVERE,ER_C_STND,ER_NoSignals)
#define AF_ERR_NOTFOUND         MAKEAERR(ER_SEVERE,ER_C_STND,ER_NotFound)
#define AF_ERR_NOTOWNER         MAKEAERR(ER_SEVERE,ER_C_STND,ER_NotOwner)
#define AF_ERR_UNIMPLEMENTED    MAKEAERR(ER_SEVERE,ER_C_STND,ER_NotSupported)

/* Audio specific errors. */
#define AF_ERR_BASE (1)
#ifndef EXTERNAL_RELEASE
/* first non-standard error code has the value MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+0) */
#endif

/* DSP is in use by Beep folio. */
#define AF_ERR_DSP_BUSY             MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+1)

/* Mismatched Item or Type. */
#define AF_ERR_MISMATCH             MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+2)

/* Template only for patches, not CreateInstrument(). */
#define AF_ERR_PATCHES_ONLY         MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+3)

/* Named thing not found. */
#define AF_ERR_NAME_NOT_FOUND       MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+4)

/* Item not attached to instrument. */
#define AF_ERR_NOINSTRUMENT         MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+5)

/* External reference in instrument not satisfied. */
#define AF_ERR_EXTERNALREF          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+9)

/* Illegal DSP Resource type. (bad .dsp file) */
#define AF_ERR_BADRSRCTYPE          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+10)

/* DSPP Code Relocation error. (bad .dsp file) */
#define AF_ERR_RELOCATION           MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+12)

/* Illegal Resource Attribute. (bad .dsp file) */
#define AF_ERR_RSRCATTR             MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+13)

/* Thing is in use. */
#define AF_ERR_INUSE                MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+14)

/* Invalid DSP instrument file. */
#define AF_ERR_BADOFX               MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+17)

/* Insufficient DSP resource. */
#define AF_ERR_NORSRC               MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+18)

/* Invalid file type. */
#define AF_ERR_BADFILETYPE          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+19)

/* Invalid name */
#define AF_ERR_BAD_NAME             MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+21)

/* Value out of range. */
#define AF_ERR_OUTOFRANGE           MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+22)

/* Name too long (contains more characters than AF_MAX_NAME_LENGTH) */
#define AF_ERR_NAME_TOO_LONG        MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+23)

/* Data address NULL */
#define AF_ERR_NULLADDRESS          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+24)

/* Illegal security violation. */
#define AF_ERR_SECURITY             MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+26)

/* Invalid signal type. */
#define AF_ERR_BAD_SIGNAL_TYPE      MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+27)

/* Thread did not call OpenAudioFolio(). */
#define AF_ERR_AUDIOCLOSED          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+28)

/* Internal DSP instrument error (e.g., something wrong with nanokernel.dsp). */
#define AF_ERR_SPECIAL              MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+29)

/* Duplicated tags or tags that contradict each other. */
#define AF_ERR_TAGCONFLICT          MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+30)

/* Invalid DSP resource binding. */
#define AF_ERR_BAD_RSRC_BINDING     MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+33)

/* Specified mixer is invalid. */
#define AF_ERR_BAD_MIXER            MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+34)

/* Name is not unique. */
#define AF_ERR_NAME_NOT_UNIQUE      MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+35)

/* Invalid port type. */
#define AF_ERR_BAD_PORT_TYPE        MAKEAERR(ER_SEVERE,ER_C_NSTND,AF_ERR_BASE+36)

#ifndef EXTERNAL_RELEASE
/* Internal result code used to inform LoadInsTemplate() that requested template isn't shareable and must be loaded. */
#define AF_ERR_INTERNAL_NOT_SHAREABLE MAKEAERR(ER_INFO,ER_C_NSTND,AF_ERR_BASE+41)
#endif

/**********************************************************************/
/************************** Data Structures ***************************/
/**********************************************************************/
/*
** Please consider the contents of all other Audio Data structures as PRIVATE.
** Use GetAudioItemInfo and SetAudioItemInfo to access these structures.
** We reserve the right to change the definition of the internal structures.
*/

typedef uint32 AudioTime;

typedef uint32 MixerSpec;       /* mixer specification (packed into a uint32) */


/*
** An Envelope's function is constructed from an array of EnvelopeSegments (see
** structure definition below). When the envelope initial value of the envelope
** function is envsegs[0].envs_Value. The function value then proceeds to
** envsegs[1].envs_Value, taking envsegs[0].envs_Duration seconds to do so.
** Barring any loops, the function proceeds from one envs_Value to the next in
** this manner until the final envs_Value in the array has been reached. The
** final envs_Duration value in the array is ignored.
*/
typedef struct EnvelopeSegment
{
    float32 envs_Value;         /* Starting value of this segment of the array.
                                ** By default, a signed signal in the range of -1.0 to
                                ** 1.0.
                                */

    float32 envs_Duration;      /* Time in seconds to reach the envs_Value of
                                ** the next EnvelopeSegment in the array.
                                */
} EnvelopeSegment;


/* Instrument port info (GetInstrumentPortInfoByName(), GetInstrumentPortInfoByIndex()) */
typedef struct InstrumentPortInfo {
    char    pinfo_Name[AF_MAX_NAME_SIZE];
    uint8   pinfo_Type;         /* AF_PORT_TYPE_ */
    uint8   pinfo_SignalType;   /* AF_SIGNAL_TYPE_ for things that use signal type */
    uint8   pinfo_Reserved[2];
    int32   pinfo_NumParts;     /* Number of parts for Inputs, Outputs, and Knobs */
} InstrumentPortInfo;


/*
** Instrument or Template resource consumption (GetInstrumentResourceInfo())
**
** Worst case consumption for all instruments from this template:
**     rinfo_MaxOverhead + rinfo_PerInstrument * # of instruments
*/
typedef struct InstrumentResourceInfo {
    int32 rinfo_PerInstrument;      /* amount required for each instrument from this template */
    int32 rinfo_MaxOverhead;        /* worst case overhead shared among all instruments from this template */
} InstrumentResourceInfo;


/* System-wide resource availability (GetAudioResourceInfo()) */
typedef struct AudioResourceInfo {
    int32 rinfo_Total;              /* Total amount of resource in system */
    int32 rinfo_Free;               /* Amount currently available */
    int32 rinfo_LargestFreeSpan;    /* Largest free block of resource (only applies to code and data mem) */
} AudioResourceInfo;


/* Signal information (GetAudioSignalInfo()) */
typedef struct AudioSignalInfo {
    float32 sinfo_Min;
    float32 sinfo_Max;
    float32 sinfo_Precision;
} AudioSignalInfo;


/**********************************************************************/
/************************** Macros and Functions **********************/
/**********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* -------------------- Attachment */

Item    CreateAttachment (Item master, Item slave, const TagArg *);
Item    CreateAttachmentVA (Item master, Item slave, uint32 tag1, ...);
#define DeleteAttachment(attachment) DeleteItem(attachment)
int32   GetAttachments (Item *attachments, int32 maxAttachments, Item insOrTemplate, const char *hookName);
#define GetNumAttachments(insOrTemplate,hookName) GetAttachments (NULL, 0, (insOrTemplate), (hookName))
Err     LinkAttachments (Item fromAttachment, Item toAttachment);
Err     MonitorAttachment (Item attachment, Item cue, int32 cueAt);
Err     ReleaseAttachment (Item attachment, const TagArg *);
Err     ReleaseAttachmentVA (Item attachment, uint32 tag1, ...);
Err     StartAttachment (Item attachment, const TagArg *);
Err     StartAttachmentVA (Item attachment, uint32 tag1, ...);
Err     StopAttachment (Item attachment, const TagArg *);
Err     StopAttachmentVA (Item attachment, uint32 tag1, ...);
int32   WhereAttachment (Item attachment);


/* -------------------- Cue */

#define CreateCue(tagList) CreateItem(MKNODEID(AUDIONODE,AUDIO_CUE_NODE),(tagList))
#define DeleteCue(cue) DeleteItem(cue)
int32   GetCueSignal (Item cue);


/* -------------------- Envelope */

Item    CreateEnvelope (const EnvelopeSegment *points, int32 numPoints, const TagArg *);
Item    CreateEnvelopeVA (const EnvelopeSegment *points, int32 numPoints, uint32 tag1, ... );
#define DeleteEnvelope(envelope) DeleteItem(envelope)


/* -------------------- Instrument */

Err     AbandonInstrument (Item instrument);
Item    AdoptInstrument (Item insTemplate);
Err     BendInstrumentPitch (Item instrument, float32 bendFrac);
Err     ConnectInstrumentParts (Item srcIns, const char *srcPortName, int32 srcPartNum,
                                Item dstIns, const char *dstPortName, int32 dstPartNum);
#define ConnectInstruments(srcIns,srcPortName,dstIns,dstPortName) \
        ConnectInstrumentParts ((srcIns), (srcPortName), 0, (dstIns), (dstPortName), 0)
Item    CreateInstrument (Item insTemplate, const TagArg *);
Item    CreateInstrumentVA (Item insTemplate, uint32 tag1, ...);
#define DeleteInstrument(instrument) DeleteItem(instrument)
Err     DisconnectInstrumentParts (Item dstIns, const char *dstPortName, int32 dstPartNum);
void    DumpInstrumentResourceInfo (Item insOrTemplate, const char *banner);
Err     GetInstrumentPortInfoByIndex (InstrumentPortInfo *info, uint32 infoSize, Item insOrTemplate, uint32 portIndex);
Err     GetInstrumentPortInfoByName (InstrumentPortInfo *info, uint32 infoSize, Item insOrTemplate, const char *portName);
Err     GetInstrumentResourceInfo (InstrumentResourceInfo *info, uint32 infoSize, Item insOrTemplate, uint32 rsrcType);
int32   GetNumInstrumentPorts (Item insOrTemplate);
Item    LoadInsTemplate (const char *insName, const TagArg *);
Item    LoadInsTemplateVA (const char *insName, uint32 tag1, ...);
Item    LoadInstrument (const char *insName, uint8 calcRateDivider, uint8 priority);
Err     PauseInstrument (Item instrument);
Err     ReleaseInstrument (Item instrument, const TagArg *);
Err     ReleaseInstrumentVA (Item instrument, uint32 tag1, ...);
Err     ResumeInstrument (Item instrument);
Err     StartInstrument (Item instrument, const TagArg *);
Err     StartInstrumentVA (Item instrument, uint32 tag1, ...);
Err     StopInstrument (Item instrument, const TagArg *);
Err     StopInstrumentVA (Item instrument, uint32 tag1, ...);
#define UnloadInsTemplate(insTemplate) DeleteItem(insTemplate)
Err     UnloadInstrument (Item instrument);


/* -------------------- Knob */

Item    CreateKnob (Item instrument, const char *knobName, const TagArg *);
Item    CreateKnobVA (Item instrument, const char *knobName, uint32 tag1, ...);
#define DeleteKnob(knob) DeleteItem(knob)
#define ReadKnob(knob,valuePtr) ReadKnobPart ((knob), 0, (valuePtr))
Err     ReadKnobPart (Item knob, int32 partNum, float32 *valuePtr);
#define SetKnob(knob,value) SetKnobPart ((knob), 0, (value))
Err     SetKnobPart (Item knob, int32 partNum, float32 value);


/* -------------------- Mixer */

    /* MixerSpec composition/decomposition */
#define MakeMixerSpec(numIn,numOut,flags) \
    ( (MixerSpec) (((numIn)&0xFF)<<24) | (((numOut)&0xFF)<<16) | ((flags)&0xFFFF) )

#define MixerSpecToNumIn(mixerSpec)  ((mixerSpec>>24)&0xFF)
#define MixerSpecToNumOut(mixerSpec) ((mixerSpec>>16)&0xFF)
#define MixerSpecToFlags(mixerSpec)  ((mixerSpec)&0xFFFF)

    /* create/delete */
Item    CreateMixerTemplate (MixerSpec, const TagArg *);
Item    CreateMixerTemplateVA (MixerSpec, uint32 tag1, ...);
#define DeleteMixerTemplate(mixerTemplate) DeleteItem(mixerTemplate)

    /* gain knob part conversion */
#define CalcMixerGainPart(mixerSpec,inChan,outChan) ((outChan) * MixerSpecToNumIn(mixerSpec) + (inChan))


/* -------------------- Probe */

Item    CreateProbe (Item instrument, const char *outputName, const TagArg *);
Item    CreateProbeVA (Item instrument, const char *outputName, uint32 tag1, ...);
#define DeleteProbe(probe) DeleteItem(probe)
#define ReadProbe(probe,valuePtr) ReadProbePart ((probe), 0, (valuePtr))
Err     ReadProbePart (Item probe, int32 partNum, float32 *valuePtr);


/* -------------------- Sample */

Item    CreateDelayLine (int32 numBytes, int32 numChannels, bool ifLoop);
Item    CreateDelayLineTemplate (int32 numBytes, int32 numChannels, bool ifLoop);
Item    CreateSample (const TagArg *);
Item    CreateSampleVA (uint32 tag1, ...);
Err     DebugSample (Item sample);
#define DeleteDelayLine(delayLine) DeleteItem(delayLine)
#define DeleteDelayLineTemplate(delayLineTemplate) DeleteItem(delayLineTemplate)
#define DeleteSample(sample) DeleteItem(sample)


/* -------------------- Signal */

float32 ConvertAudioSignalToGeneric (int32 signalType, int32 rateDivide, float32 signalValue);
float32 ConvertGenericToAudioSignal (int32 signalType, int32 rateDivide, float32 genericValue);
Err     GetAudioSignalInfo (AudioSignalInfo *info, uint32 infoSize, uint32 signalType);


/* -------------------- Timer */

#define AF_GLOBAL_CLOCK (0)     /* magic Clock Item for indicating system-wide audio clock */

    /* create/delete */
#define CreateAudioClock(tagList) CreateItem(MKNODEID(AUDIONODE,AUDIO_CLOCK_NODE),(tagList))
#define DeleteAudioClock(clock) DeleteItem(clock)

    /* general timer operations */
Err     AbortTimerCue (Item cue);
int32   GetAudioClockDuration (Item clock);
Err     GetAudioClockRate (Item clock, float32 *hertz);
Err     ReadAudioClock (Item clock, AudioTime *time);
Err     SetAudioClockDuration (Item clock, int32 numFrames);
Err     SetAudioClockRate (Item clock, float32 hertz);
Err     SignalAtAudioTime (Item clock, Item cue, AudioTime time);
Err     SleepUntilAudioTime (Item clock, Item cue, AudioTime time);

    /* global clock operations (convenience) */
AudioTime GetAudioTime (void);
#define GetAudioDuration() GetAudioClockDuration (AF_GLOBAL_CLOCK)
#define SignalAtTime(cue,time) SignalAtAudioTime (AF_GLOBAL_CLOCK, (cue), (time))
#define SleepUntilTime(cue,time) SleepUntilAudioTime (AF_GLOBAL_CLOCK, (cue), (time))

    /* AudioTime comparison macros. These macros assume that the times to be
       compared are <= 0x7fffffff ticks apart. */
#define CompareAudioTimes(t1,t2)         ( (int32) ( (t1) - (t2) ) )
#define AudioTimeLaterThan(t1,t2)        (CompareAudioTimes((t1),(t2)) > 0)
#define AudioTimeLaterThanOrEqual(t1,t2) (CompareAudioTimes((t1),(t2)) >= 0)


/* -------------------- Trigger */

Err     ArmTrigger (Item instrument, const char *triggerName, Item cue, uint32 flags);
#define DisarmTrigger(instrument,triggerName) ArmTrigger((instrument),(triggerName),0,0)


/* -------------------- Tuning */

Err     Convert12TET_FP (int32 semitones, int32 cents, float32 *fractionPtr);
Item    CreateTuning (const float32 *frequencies, int32 numNotes, int32 notesPerOctave, int32 baseNote);
#define DeleteTuning(tuning) DeleteItem(tuning)
Err     TuneInsTemplate (Item insTemplate, Item tuning);
Err     TuneInstrument (Item instrument, Item tuning);


/* -------------------- Miscellaneous */

    /* Folio oopen/close */
Err     OpenAudioFolio (void);
Err     CloseAudioFolio (void);

    /* Generic Item control */
Err     GetAudioItemInfo (Item audioItem, TagArg *resultTagList);
Err     SetAudioItemInfo (Item audioItem, const TagArg *);
Err     SetAudioItemInfoVA (Item audioItem, uint32 tag1, ...);

    /* System-wide stuff */
int32   EnableAudioInput (int32 OnOrOff, const TagArg *);
Err     GetAudioFolioInfo (TagArg *resultTagList);
uint32  GetAudioFrameCount (void);
Err     GetAudioResourceInfo (AudioResourceInfo *info, uint32 infoSize, uint32 rsrcType);

#ifndef EXTERNAL_RELEASE
    /* supervisor functions */
Err     SetAudioFolioInfo (const TagArg *);
Err     SetAudioFolioInfoVA (uint32 tag1, ...);
#endif

#ifndef EXTERNAL_RELEASE        /* !!! bound for removal */

    /* ScavengeInstrument() was never made public. will move out of audio folio soon */
Item  ScavengeInstrument( Item InsTemplate, uint8 Priority, int32 MaxActivity, int32 IfSystemWide );

#endif  /* !defined(EXTERNAL_RELEASE) */


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_AUDIO_H */
