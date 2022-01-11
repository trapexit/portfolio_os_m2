#ifndef __AUDIO_STRUCTS_H
#define __AUDIO_STRUCTS_H


/***********************************************************************
**
**  @(#) audio_structs.h 96/08/28 1.71
**  $Id: audio_structs.h,v 1.36 1995/03/14 06:40:12 phil Exp $
**  Internal includes file for audio folio.
**
************************************************************************
** 940406 PLB Support TuneInsTemplate()
** 940606 WJB Changed ains_StartTime from uint32 to AudioTime.
** 940608 PLB Added dspp_TicksPerFrame
** 940614 PLB added asmp_BadDogData
** 940815 PLB Moved DSPPStatic to dspp.h
** 950131 WJB Removed a bunch of redundant includes.
** 950419 WJB Added prototype for (*aevt_Perform)().
** 950419 WJB Added prototypes for CustomAlloc/FreeMem() functions pointers.
** 950504 WJB Added trigger interrupt handshaking stuff.
** 950516 WJB Added af_TriggerCueTable to AudioFolio.
** 950525 WJB Removed af_TriggersReceived. Added af_ArmedTriggers.
** 950818 WJB Moved ENV_SUFFIX_ string defines here.
** 950818 WJB Changed aeva_ DSPI address fields to uint16 and renamed them.
** 950821 WJB Added ENV_DRSC_TYPE_ defines.
** 950905 WJB Added constants for max envelope hook name and suffix lengths.
** 951116 WJB Shortened aenv_Flags to uint8. Added aenv_SignalType.
** 951206 WJB Added AF_NODE_F_AUTO_DELETE_SLAVE.
** 951207 WJB AudioReferenceNode is now MinNode-based.
**            Embedded AudioReferenceNode in AudioAttachment to avoid extra allocation.
***********************************************************************/

#include <audio/audio.h>
#include <kernel/folio.h>
#include <kernel/list.h>
#include <kernel/task.h>
#include <kernel/types.h>

#include "audio_folio_modes.h"


/**********************************************************************/
/***************** Constants ******************************************/
/**********************************************************************/

enum af_special_types
{
	AF_SPECIAL_NOT = 0,
	AF_SPECIAL_KERNEL
};
#define AF_SPECIAL_MAX AF_SPECIAL_KERNEL

/**********************************************************************/
/***************** Internal Structures ********************************/
/**********************************************************************/

/*  Audio Item usage of non-kernel n_Flags:
**      0x07 - common to all audio items
**      0x08 - node type-specific application
**      !!! make more of these node type-specific
*/
#define AF_NODE_F_AUTO_FREE_DATA    0x01    /* automatically free this node's data when Item is deleted (envelopes, tunings, samples) */
#define AF_NODE_F_AUTO_DELETE_SLAVE 0x02    /* automatically deletes slave Item(s) when Item is deleted (attachments) */
#define AF_NODE_F_MARK              0x04    /* flag used to mark an item in an atomic scan (templates) */

typedef struct AudioReferenceNode   /* Used to store a reference to an Item in a list */
{
	MinNode arnd_Node;      /* Node to store in referring Item's Reference List (e.g., aenv_AttachmentRefs) */
	Item    arnd_RefItem;   /* Item number of Item containing AudioReferenceNode (e.g., Attachment) */
} AudioReferenceNode;

typedef struct AudioConnectionNode  /* Used to track instrument connections. */
{
	MinNode acnd_Node;
	Item    acnd_SrcIns;    /* Source instrument. */
	const char *acnd_SrcName; /* Name of source resource. Points to template names.*/
	Item    acnd_DstIns;    /* Destination instrument. */
	const char *acnd_DstName; /* Name of destination resource. Points to template names.*/
	int32   acnd_MoveIndex; /* If connection uses a MOVE, this is its index, or -1. */
	uint8   acnd_SrcPart;   /* Part number of source. */
	uint8   acnd_DstPart;   /* Part number of destination. */
} AudioConnectionNode;

/*
** Used by the application to have a clock with independant rate control.
** This is needed for MIDI scores with variable tempo that must not interfere
** with other applications.
*/
typedef struct AudioClock
{
	ItemNode    aclk_Item;
	int32       aclk_Duration;
	uint32      aclk_OldFrames;
	uint32      aclk_RemainderFrames;
	uint32      aclk_Time;
} AudioClock;

typedef struct AudioEvent
{
	ItemNode    aevt_Node;
	uint32      aevt_ListTime;  /* frame count or global time for list sort */
	AudioClock *aevt_Clock;     /* Clock that TriggerAt time is based on. */
	AudioTime   aevt_TriggerAt; /* Time passed to SignalAtTime */
	int32     (*aevt_Perform)(struct AudioEvent *);
	List       *aevt_InList;    /* List we are linked into or NULL */
} AudioEvent;

typedef struct AudioCue
{
	AudioEvent  acue_Event;      /* Contents of acue_Event are the exclusive property of the Timer system.
	                                Other Cue clients can make use of the Cue, but not the List management
	                                or TriggerAt fields. This permits a single Cue to be used simultaneously
	                                for a Timer event and multiple ArmTrigger and MonitorAttachment() uses. */
	uint32      acue_Signal;
	Task       *acue_Task;
} AudioCue;

typedef struct AudioTuning
{
	ItemNode    atun_Item;
		/* n_Flags: AF_NODE_F_AUTO_FREE_DATA - free atun_Frequencies, TRACKED_SIZE */
	float32    *atun_Frequencies;       /* Hertz */
	int32       atun_NumNotes;          /* Number of notes in Frequencies array. */
	int32       atun_BaseNote;          /* Note corresponding to zeroth fraction. */
	int32       atun_NotesPerOctave;
} AudioTuning;

typedef struct AudioEnvelope
{
	ItemNode    aenv_Item;
		/* n_Flags: AF_NODE_F_AUTO_FREE_DATA - free aenv_Points, TRACKED_SIZE */
	List        aenv_AttachmentRefs;    /* List of AudioReferenceNodes. */

	    /* envelope definition */
	EnvelopeSegment *aenv_Points;
	int32       aenv_NumPoints;
	int32       aenv_SustainBegin;      /* Set to -1 if no sustain loop */
	int32       aenv_SustainEnd;        /* Set to -1 if no sustain loop */
	float32     aenv_SustainTime;       /* Time in seconds to get from End to Begin */
	int32       aenv_ReleaseBegin;      /* Set to -1 if no release loop */
	int32       aenv_ReleaseEnd;        /* Set to -1 if no release loop */
	float32     aenv_ReleaseTime;       /* Time to get from End to Begin */
	int32       aenv_ReleaseJump;       /* Point to jump to upon release, or -1 if no jump */
	uint8       aenv_Flags;             /* AF_ENVF_ flags */
	uint8       aenv_SignalType;        /* AF_SIGNAL_TYPE_ of envelope. envs_Value is in these units. */
	uint8       aenv_BaseNote;          /* Note at which Pitch based timescale = 1.0. */
	int8        aenv_NotesPerDouble;    /* Number of semitones at which time scale doubles. */
} AudioEnvelope;

/* Attachment Item **************************************/
/* !!! some of this stuff is certainly overkill for attachments made to Templates */
typedef struct AudioAttachment
{
	ItemNode    aatt_Item;
		/* n_Flags: AF_NODE_F_AUTO_DELETE_SLAVE - auto delete aatt_SlaveItem */
	AudioReferenceNode aatt_SlaveRef;   /* AudioReferenceNode to store in SlaveItem's reference list. */

	uint8       aatt_Type;              /* AF_ATT_TYPE_ */
	uint8       aatt_SubType;           /* Used for FIFOs for checking compression. */
	uint8       aatt_SegmentCount;      /* Counts down, when it reaches zero => at end. */
	int8        aatt_ActivityLevel;     /* Abandoned, Stopped, Started, Released.  */
	uint32      aatt_Flags;             /* AF_ATTF_ flags (!!! really doesn't need to be 32 bits) */
	Item        aatt_HostItem;          /* Master Instrument or Template Item */
	Item        aatt_SlaveItem;         /* Slave Sample or Envelope Item */
	void       *aatt_Structure;         /* Slave Sample or Envelope address (!!! ItemNode * instead?) */
	char       *aatt_HookName;          /* What we are attached to (allocated when attachment is created). !!! probably NULL when NULL is given to CreateAttachment() */
	int32       aatt_StartAt;           /* Index to Start At when started. */
	int32       aatt_Channel;           /* DMA Channel (!!! really doesn't need to be 32 bits) */
	Item        aatt_CueItem;
	int32       aatt_CueAt;
	Item        aatt_NextAttachment;
	void       *aatt_Extension;
} AudioAttachment;

    /* aatt_Type */
    /* !!! perhaps these should be replaced by AUDIO_SAMPLE_NODE and AUDIO_ENVELOPE_NODE respectively */
#define AF_ATT_TYPE_SAMPLE      1
#define AF_ATT_TYPE_ENVELOPE    2

    /* macro to convert pointer to an aatt_SlaveRef to pointer to AudioAttachment containing that aatt_SlaveRef */
#define SlaveRefToAttachment(refnode) ((AudioAttachment *)((char *)(refnode) - offsetof (AudioAttachment, aatt_SlaveRef)))


/**********************************************************
** Envelope Attachment Extension
** Contains data needed to "play" an envelope on an instrument.
**********************************************************/
typedef struct AudioEnvExtension
{
		/* links */
	AudioEvent aeva_Event;
	AudioAttachment *aeva_Parent;

		/* DSP data memory addresses of envelope control resources */
	uint16     aeva_RequestDSPI;   /* <env>.request knob */
	uint16     aeva_IncrDSPI;      /* <env>.incr knob */
	uint16     aeva_TargetDSPI;    /* <env>.target variable */
	uint16     aeva_CurrentDSPI;   /* <env>.current variable */

		/* internal values */
	int32      aeva_CurIndex;      /* Index that matches TargetValue */
	int32      aeva_FramesLeft;    /* Frames remaining in long segment. */
	float32    aeva_DeltaTime;     /* Time between points. */
	float32    aeva_PrevTarget;    /* Target of previous segment. */
	float32    aeva_TimeScale;     /* Scaled specifically for attachment.  */
} AudioEnvExtension;

/*
** Used by the application to control parameters on an Audio Instrument.
** When a knob is attached, the device fills in function pointers for
** fast execution.
*/
typedef struct AudioKnob
{
	ItemNode    aknob_Item;
	void       *aknob_DeviceInstrument; /* Instrument that knob belongs to */
	uint8       aknob_RsrcIndex;        /* Points to device specific data. */
	uint8       aknob_Type;             /* AF_SIGNAL_TYPE_. Set to drsc_SubType when created. Can be overridden. */
} AudioKnob;

/* Knobs and probes are real similar so use same data structure and share code. */
typedef AudioKnob AudioProbe;
#define aprob_DeviceInstrument aknob_DeviceInstrument
#define aprob_RsrcIndex aknob_RsrcIndex
#define aprob_Type aknob_Type

typedef struct AudioInsTemplate
{
	OpeningItemNode aitp_Item;
		/* n_Flags: AF_NODE_F_MARK - temporary mark set by swiGetDynamicLinkResourceUsage() in an atomic operation */
	void       *aitp_DeviceTemplate;        /* Device specific information. */
	Item       *aitp_DynamicLinkTemplates;  /* 0-terminated list of Dynamic Link Templates opened during creation. NULL means no open links */
	List        aitp_InstrumentList;        /* List of Instruments allocated from this template. */
	List        aitp_Attachments;           /* List of attached items. */
	Item        aitp_Tuning;                /* Tuning for all child instruments. 0 when no tuning is assigned to Template. (940406) */
} AudioInsTemplate;

typedef struct AudioInstrument
{
	OpeningItemNode    ains_Item;
		/* n_Flags: node type-specific flags defined below */
	AudioInsTemplate  *ains_Template;
	void       *ains_DeviceInstrument;
	uint8       ains_Flags;                 /* AF_INSF_ flags */
	int8        ains_Status;                /* AF_STARTED, AF_RELEASED, etc. */
	uint8       ains_StartingFreqType;      /* For pitch bending. FREQ_TYPE_ (dspp_instr.c) selected when instrument was started. */
	uint8       ains_StartingPitchNote;     /* For pitch bending. MIDI note number of pitch instrument was started with (FREQ_TYPE_PITCH only). */
	List        ains_KnobList;              /* List of all grabbed knobs. */
	List        ains_ProbeList;             /* List of all probes. */
	Item        ains_Tuning;
	float32     ains_Bend;
	float32     ains_StartingFreqReq;       /* For pitch bending. Starting frequency requested (units depend on type indicated by ains_StartingFreqType) */
	AudioAttachment *ains_StartingAAtt;     /* For pitch bending */
	AudioTime   ains_StartTime;
} AudioInstrument;

    /* AudioInstrument type-specific n_Flags */
#define AF_INS_NODE_F_DLNK_OPENED   0x08    /* Successfully opened dynamic link instruments */


/* Structure for each task using audiofolio. */

typedef struct AudioFolioTaskData
{
	int32 aftd_InputEnables;   /* Incremented each time EnableAudioInput() is called. */
} AudioFolioTaskData;


typedef struct AudioFolio
{
	Folio       af_Folio;
	Item        af_AudioModule;         /* Audio Folio's Module Item */

		/* Audio Daemon:
		** Signal handler thread which responds to signals from DMA,
		** timer, and trigger interrupts. Also owns the Audio Folio.
		*/
	Item        af_AudioDaemonItem;     /* Item number of audio daemon Task */
	Task       *af_AudioDaemon;         /* Task pointer to audio daemon */

		/* Nanokernel */
	Item        af_NanokernelInsTemplate; /* Instrument Template Item number of nanokernel.dsp */
	Item        af_NanokernelInstrument;  /* Instrument Item number of nanokernel.dsp */
	Item        af_NanokernelAmpKnob;     /* Knob Item number of nanokernel.dsp Amplitude knob */

		/* Item Lists */
	List        af_TemplateList;
	List        af_SampleList;          /* List of all existing samples. Used to update asmp_BaseFreq on DAC sample rate changes */
	List        af_ClockList;

		/* Instruments */
	List        af_ConnectionList;      /* List of connections between instruments. */
	uint32      af_InputEnableCount;    /* Number of tasks with Audio Input enabled. */

        /* Interrupts */
	Item        af_AudioFIRQ;           /* Single FIRQ shared by all DSPP interrupts. */

		/* DMA */
	int32       af_DMASignal;           /* Signal to send to af_AudioDaemon when DMA completes. */
	uint32      af_ChannelsComplete;    /* Set of completed DMA channels. Set by interrupt, read by folio task. */

		/* Timer */
	AudioClock *af_GlobalClock;         /* System-wide timer. */
	AudioClock *af_EnvelopeClock;       /* Fast clock for envelopes. */
	int32       af_TimerSignal;         /* Signal to send to af_AudioDaemon when a timer interrupt occurs. */
	List        af_FastTimerList;       /* pending Timer items */
	List        af_SlowTimerList;       /* pending Timer items */
	uint32      af_NextSyncFrame;       /* Frame at which we will resync all audio clocks. */

		/* Triggers */
	int32       af_TriggerSignal;       /* Signal to send to af_AudioDaemon when a trigger soft int is received. */
	uint32      af_ArmedTriggers;       /* Set of triggers (not soft interrupts!) that HandleTriggerSignal() checks. */

	    /* Misc */
	uint32      af_DCacheLineSize;      /* Data cache line size (used to align delay line memory) */
} AudioFolio;


/*
    AudioSample - Sample Item

    Sub types:
        - ordinary samples (ITEMNODE_PRIVILEGED is not set)
            - asmp_Data is in user memory.
            - AF_NODE_F_AUTO_FREE_DATA is supported.
        - delay lines (ITEMNODE_PRIVILEGED is set)
            - actual delay lines (asmp_Data != NULL)
                - asmp_Data points to supervisor memory, allocated with MEMTYPE_TRACKSIZE
            - delay line templates (asmp_Data == NULL)
*/
typedef struct AudioSample
{
	ItemNode    asmp_Item;
		/* n_Flags: AF_NODE_F_AUTO_FREE_DATA - free asmp_Data, TRACKED_SIZE */
		/* n_ItemFlags: ITEMNODE_PRIVILEGED - sample is a delay line */
	void       *asmp_Data;              /* Points to first sample */
	int32       asmp_NumFrames;         /* in frames */
	int32       asmp_SustainBegin;      /* Set to -1 if no sustain loop */
	int32       asmp_SustainEnd;
	int32       asmp_ReleaseBegin;      /* Set to -1 if no release loop */
	int32       asmp_ReleaseEnd;
	int32       asmp_NumBytes;          /* Size in Bytes to be played. */
	float32     asmp_BaseFreq;          /* freq (Hz) if played at 44.1 KHz */
	uint8       asmp_Bits;              /* ORIGINAL bits per sample BEFORE any compression. */
	uint8       asmp_Width;             /* ORIGINAL bytes per sample BEFORE any compression. */
	uint8       asmp_Channels;          /* channels per frame, 1 = mono, 2=stereo */
	uint8       asmp_BaseNote;          /* MIDI Note when played at original sample rate. */
	int8        asmp_Detune;            /* cents of detuning */
	uint8       asmp_LowNote;
	uint8       asmp_HighNote;
	uint8       asmp_LowVelocity;
	uint8       asmp_HighVelocity;
	uint8       asmp_CompressionRatio;  /* 2 = 2:1, 4 = 4:1 */
	uint8       asmp_Flags;             /* AF_SAMPF_ flags */
	uint8       asmp_Reserved0;
	PackedID    asmp_CompressionType;   /* eg. SDX2 , 930210 */
	List        asmp_AttachmentRefs;    /* List of AudioReferenceNodes. */
	float32     asmp_SampleRate;        /* Sample Rate recorded at */
} AudioSample;

    /* returns TRUE if sample item is a delay line or delay line template, FALSE otherwise */
#define IsDelayLine(asmp) (((asmp)->asmp_Item.n_ItemFlags & ITEMNODE_PRIVILEGED) != 0)

    /* returns TRUE if sample item is a delay line template, FALSE otherwise */
#define IsDelayLineTemplate(asmp) (IsDelayLine(asmp) && !(asmp)->asmp_Data)


/***********************************************************************/

#endif  /* __AUDIO_STRUCTS_H */
