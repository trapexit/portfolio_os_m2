#ifndef __AUDIO_INTERNAL_H
#define __AUDIO_INTERNAL_H

/* @(#) audio_internal.h 96/10/16 1.115 */
/* $Id: audio_internal.h,v 1.79 1995/03/14 06:40:54 phil Exp phil $ */

/*
** Audio Folio Internal Includes
** By:  Phil Burk
*/

/*
** 930816 PLB Removed redefinition of SuperInternalFreeSignal
** 940304 WJB Added stdio.h.
** 940407 PLB Added DEBUG_AUDIO to select between dev and debug.
** 940506 PLB Changed SAMPLERATE to DEFAULT_SAMPLERATE
** 940511 WJB Removed local prototype for isUser().  Including clib.h instead.
** 940802 WJB Added support for demand loading.
** 940811 PLB Added ezmem_tools.h and table_alloc.h
** 940912 PLB Added Write16(), Read16() macros.
** 940915 PLB Call IsItemOpened(), not ItemOpened()
** 940927 WJB Removed redundant Create/DeleteProbe() prototypes.
** 941011 WJB Added prototype for GetMasterTuning() (no longer in audio.h).
** 941012 WJB Syncronized swiAbandonInstrument() prototype in audio.h
** 941024 PLB Removed "UNSUPPORTED" macro, only used by GetKnob()
** 950106 WJB Made Read16() take a const pointer.
** 950109 WJB Fixed Write16() to take a uint32 - saves a few
**            instructions for caller and callee.
** 950131 WJB Cleaned up includes.
**            Added ufrac16FromIeee() prototype.
** 950505 WJB Minor tweaks to recover from the great split.
** 950512 WJB Added trigger function prototypes.
** 950515 WJB Added swiMonitorTrigger() prototype.
** 950516 WJB Added UnmonitorInstrumenTriggers().
** 950516 WJB Removed UnmonitorInstrumenTriggers().
**            Added ResetAllInstrumentTriggers() and DisarmAllInstrumentTriggers().
**            Renamed swiMonitorTrigger() to swiArmTrigger().
** 950717 WJB Added more packed string macros.
** 950725 WJB Removed REALTIME macro.
*/

#include <audio/audio.h>
#include <audio/handy_macros.h> /* convenience */
#include <kernel/item.h>
#include <kernel/kernel.h>      /* for PRTDBUGINFO() */
#include <kernel/mem.h>         /* convenience */
#include <kernel/super.h>       /* convenience */
#include <kernel/tags.h>        /* convenience */
#include <kernel/task.h>        /* for PRTDBUGINFO() */
#include <kernel/types.h>
#include <stdio.h>              /* printf() */

#include "audio_folio_modes.h"
#include "audio_structs.h"
#include "dspp.h"               /* convenience */

#define PRT(x)          { printf x; }
#define PRTDBUGINFO()   { PRT(("audio (task=0x%x '%s' pri=%d %s %s): ", CURRENTTASKITEM, CURRENTTASK->t.n_Name, CURRENTTASK->t.n_Priority, IsPriv(CURRENTTASK) ? "priv" : "nonpriv", IsSuper() ? "super" : "user")); }
/* Turn on different levels of debug.
/* Level 1 is API level LoadInstrument(), StartInstrument(), etc. */
/* Level 2 is commonly needed internal debug like allocated addresses. */
/* Level 3 is more detailed. */
#define DBUG3(x)  /* PRT(x) */
#define DBUG2(x)  DBUG3(x)
#define DBUG1(x)  DBUG2(x)

/**********************************************************************/
/**************************** Debug Support  **************************/
/**********************************************************************/
#ifndef BUILD_STRINGS
	#define ERR(x)  /* PRT(x) */
	#define ERRDBUG(x)  /* PRT(x) */
#else
	#define ERR(x)  PRT(x)
	#ifdef DEBUG_AUDIO
		#define ERRDBUG(x)  PRT(x)
	#else
		#define ERRDBUG(x)  /* PRT(x) */
	#endif
#endif

/* TRACE* are no longer used.  Remove code references or convert to DBUG - FIXME */
#define TRACEE(andmask,ormask,msg) /* */
#define TRACEB(andmask,ormask,msg) /* */
#define TRACER(andmask,ormask,msg) /* */

#define TRACKMEM(x) /* { PRT(x); afi_ReportMemoryUsage(); } */

#define CHECKRSLT(msg) \
	if (Result < 0) \
	{ \
		ERR(msg); \
		goto error; \
	}

#define ItemStructureParanoid(st,type,msg) \
{ \
	if (CheckItem(((ItemNode *) st)->n_Item, AUDIONODE, type) == NULL) \
	{ \
		ERR(("Paranoia Trap %s = 0x%x\n", msg, st)); \
		return AF_ERR_BADITEM; \
	} \
}

#ifdef PARANOID
#define ParanoidRemNode(n) \
{ \
	Node *paranode; \
	paranode = n; \
	if(( paranode->n_Next->n_Prev != n) || ( paranode->n_Prev->n_Next != n)) \
	{ \
		ERR(("Attempt to remove node 0x%x not in list!\n", paranode)); \
	} \
	else \
	{ \
		RemNode(paranode); \
	} \
}
#else
#define ParanoidRemNode(n) RemNode(n)
#endif


/**********************************************************************/
/**************************** Macros  *********************************/
/**********************************************************************/

#define OWNEDBYCALLER(ItemPtr) ((((ItemNode *)ItemPtr)->n_Owner) == CURRENTTASKITEM)

#define CHECKAUDIOOPEN \
	{ \
		const Item curTask = CURRENTTASKITEM; \
		\
		if (curTask != AB_FIELD(af_AudioDaemonItem) && IsItemOpened (curTask, AB_FIELD(af_AudioModule)) < 0) \
		{ \
			ERR(("OpenAudioFolio() must be called by each thread!\n")); \
			return AF_ERR_AUDIOCLOSED; \
		} \
	}


/**********************************************************************/
/**************************** Constants  ******************************/
/**********************************************************************/

#define DEFAULT_SAMPLERATE (44100)
/* The clock that controls the DAC may be a ceramic oscillator
** which means it may be off by many parts per million.
** This must be accounted for when calculating how many
** ticks the DSP has available to calculate a frame.
**  See TRAC# 11167.
*/
#define DAC_CLOCK_TOLERANCE_PPM  (10000)
#define MAX_DEFAULT_SAMPLERATE   (DEFAULT_SAMPLERATE + ((DEFAULT_SAMPLERATE*DAC_CLOCK_TOLERANCE_PPM)/1000000))
#define AF_SAMPLE_RATE     (DSPPData.dspp_SampleRate)
#define AF_DEFAULT_ENV_HOOK "Env"

#define	ID_SDX2			MAKE_ID('S','D','X','2') /* Opera software 2:1 OBSOLETE ID - USE ID_SQD2 */

#if 0   /* !!! old */
/* Tags for TestHack routine. */
#define HACK_TAG_LEVEL    101
#define HACK_TAG_READ_EO  102
#define HACK_TAG_READ_DMA 103
#define HACK_TAG_RSRC_NAME 104
#define HACK_TAG_RSRC_TYPE 105
#define HACK_TAG_TEST_DUCK 106
#endif

/**********************************************************************/
/**************************** Externs  ********************************/
/**********************************************************************/
extern int32 DAC_ControlValue;
extern int32 SetDAC_ControlValue;

extern AudioFolio AudioBase;
#define AB_FIELD(x) AudioBase.x

    /* Tuning to use if none other specified. Also used if DAC rate changes to update sample tuning. */
extern const AudioTuning DefaultTuning;

/**********************************************************************/
/************************** Private SWIs ******************************/
/**********************************************************************/

int32 GetDynamicLinkResourceUsage (Item insTemplate, int32 rsrcType);

/**********************************************************************/
/************************** Prototypes ********************************/
/**********************************************************************/
AudioAttachment *afi_CheckNextAttachment( AudioAttachment *aatt );
Err Convert12TET_FP( int32 Semitones, int32 Cents, float32 *FractionPtr );
Err swiBendInstrumentPitch( Item Instrument, float32 BendFrac );
float32 internalConvertKnobToGeneric( int32 knobType, int32 rateShift, float32 Value );
Item internalCreateAudioAttachment (AudioAttachment *, const TagArg *);
Item internalCreateAudioCue (AudioCue *, const TagArg *);
Item internalCreateAudioEnvelope (AudioEnvelope *, const TagArg *);
Item internalCreateAudioIns (AudioInstrument *, const TagArg *);
Item internalCreateAudioKnob (AudioKnob *, TagArg *args);
Item internalCreateAudioKnobOrProbe (AudioKnob *, int32 RsrcType, TagArg *args);
Item internalCreateAudioSample (AudioSample *, const TagArg *);
Item internalCreateAudioTemplate (AudioInsTemplate *, const TagArg *);
Item internalCreateAudioTuning (AudioTuning *, const TagArg *);
Item internalCreateAudioClock (AudioClock *, const TagArg *);
int32 internalDeleteAudioAttachment (AudioAttachment *, Task *);
int32 internalDeleteAudioCue (AudioCue *);
int32 internalDeleteAudioEnvelope (AudioEnvelope *, Task *);
int32 internalDeleteAudioIns (AudioInstrument *);
int32 internalDeleteAudioKnob (AudioKnob *);
int32 internalDeleteAudioSample (AudioSample *, Task *);
int32 internalDeleteAudioTemplate (AudioInsTemplate *, Task *);
int32 internalDeleteAudioTuning (AudioTuning *, Task *);
int32 internalDeleteAudioClock (AudioClock *);
Err  internalSetAttachmentInfo (AudioAttachment *, const TagArg *tagList);
Err  internalSetTuningInfo (AudioTuning *, const TagArg *);
Item internalFindInsTemplate(TagArg *tags);
Item internalLoadSharedTemplate( TagArg *tags);
Item internalOpenInsTemplate( AudioInsTemplate *aitp );
Err  internalCloseInsTemplate( AudioInsTemplate *aitp );
Item internalOpenInstrument( AudioInstrument *ains );
Err  internalCloseInstrument( AudioInstrument *ains );
Err  internalReadKnobOrProbePart ( AudioKnob *aknob, int32 partNum, float32 *valuePtr );
Item swiAdoptInstrument( Item InsTemplate );
Err dsppInitDuckAndCover( void );
void dsppTermDuckAndCover( void );
int32 DisableAttachmentSignal ( AudioAttachment *aatt );
int32 EnableAttachmentNext ( AudioAttachment *aatt, void *Address, uint32 Cnt );
int32 EnableAttachmentSignal ( AudioAttachment *aatt );
int32 HandleDMASignal (void);
int32 PostAudioEvent( AudioEvent *aevt, AudioTime Time );
int32 SignalMonitoringCue( AudioAttachment *aatt );
int32 UnpostAudioEvent( AudioEvent *aevt );
int32 UpdateAllSampleBaseFreqs( void );
void internalDumpEnvelope (const AudioEnvelope *, const char *banner);
void internalDumpSample (const AudioSample *, const char *banner);
Err   internalGetAttachmentInfo (const AudioAttachment *aatt, TagArg *tagList);
Err   internalGetEnvelopeInfo (const AudioEnvelope *, TagArg *tagList);
Err   internalGetInstrumentInfo (const AudioInstrument *, TagArg *tagList);
Err   internalGetKnobInfo (const AudioKnob *, TagArg *tagList);
Err   internalGetSampleInfo (const AudioSample *, TagArg *);
Err   internalSetEnvelopeInfo (AudioEnvelope *, const TagArg *);
Err   internalSetSampleInfo (AudioSample *, const TagArg *);
int32 SetDMANextInt ( int32 DMAChan, AudioGrain *Address, int32 Cnt );
Err swiAbandonInstrument( Item Instrument );
int32 swiAbortTimerCue( Item Cue );
Err   swiArmTrigger( Item Instrument, const char *TriggerName, Item Cue, uint32 Flags );
Err   swiConnectInstrumentsHack( const int32 *Params );     /* !!! hack to work around 4-arg SWI limit */
Err   swiConnectInstrumentParts (Item SrcIns, const char *SrcName, int32 SrcPart, Item DstIns, const char *DstName, int32 DstPart);
Err   swiDisconnectInstrumentParts (Item DstIns, const char *DstName, int32 DstPart);
int32 swiGetDynamicLinkResourceUsage (Item insTemplate, uint32 rsrcType);
int32 swiLinkAttachments( Item At1, Item At2 );
int32 swiMonitorAttachment( Item Attachment, Item Cue, int32 Index );
Err   swiPauseInstrument (Item InstrumentItem);
Err   swiReadKnobPart ( Item knobItem, int32 partNum, float32 *valuePtr );
int32 swiReleaseAttachment( Item Attachment, TagArg *tp );
Err   swiReleaseInstrument (Item InstrumentItem, const TagArg *);
Err   swiResumeInstrument (Item InstrumentItem);
int32 swiScavengeInstrument( Item InsTemplate, uint8 Priority, uint32 MaxActivity, int32 IfSystemWide );
int32 swiSetKnobPart ( Item KnobItem, int32 PartNum, float32 Value );
int32 swiStartAttachment( Item Attachment, TagArg *tp );
Err   swiStartInstrument (Item InstrumentItem, const TagArg *);
int32 swiStopAttachment(  Item Attachment, TagArg *);
Err   swiStopInstrument (Item InstrumentItem, const TagArg *);
int32 swiTuneInsTemplate( Item InsTemplate, Item Tuning );
int32 swiTuneInstrument( Item Instrument, Item Tuning );
int32 swiWhereAttachment( Item Attachment );
void  DSPPNextAttachment( AudioAttachment *aatt );
void  DSPPQueueAttachment( AudioAttachment *aatt );
void  EnableAttSignalIfNeeded( AudioAttachment *aatt );
void SetEnvAttTimeScale( AudioAttachment *, int32 PitchNote, float32 timeScale );

    /* items */
void InstallAudioItemRoutines (AudioFolio *);
Err swiSetAudioItemInfo (Item audioItem, const TagArg *);

    /* task */
#define GetAudioFolioTaskData(task)      (AudioFolioTaskData *)((task)->t_FolioData [ AB_FIELD(af_Folio).f_TaskDataIndex ])
#define SetAudioFolioTaskData(task,aftd) ( ((task)->t_FolioData [ AB_FIELD(af_Folio).f_TaskDataIndex ]) = (aftd) )

    /* tuning */
float32 Approximate2toX (float32 X);
const AudioTuning *GetInsTuning (const AudioInstrument *);
Err PitchToFrequency (const AudioTuning *, int32 Pitch, float32 *FrequencyPtr);

    /* instrument template */
AudioInsTemplate *LookupAudioInsTemplate (Item insOrTemplate);

    /* instrument */
#define AllocInstrumentSpecial(Template,Priority,Specialness) \
	CreateItemVA (MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), \
		AF_TAG_TEMPLATE, (Template),    \
		AF_TAG_PRIORITY, (Priority),    \
		AF_TAG_SPECIAL,  (Specialness), \
		TAG_END)

    /* Probes */
Err swiReadProbePart ( Item ProbeItem, int32 partNum, float32 *ValuePtr );
int32 internalDeleteAudioProbe (AudioProbe *aprob);
Item internalCreateAudioProbe (AudioProbe *aprobe, TagArg *args);

    /* samples (audio_samples.c) */
void ClearDelayLineIfStopped (AudioSample *);
uint32 CvtByteToFrame (uint32 byteNum, const AudioSample *);
uint32 CvtFrameToByte (uint32 frameNum, const AudioSample *);

    /* timer system (audio_timer.c) */
Err     InitAudioTimer (void);
Err     swiReadAudioClock( Item clock, AudioTime *time );
Err     swiSetAudioClockRate( Item clock, float32 hertz );
Err     GetAudioClockRate( Item clock, float32 *hertz );
Err     swiSetAudioClockDuration( Item clock, int32 numFrames );
int32   GetAudioClockDuration( Item clock );
Err     swiSignalAtAudioTime ( Item clock, Item cue, AudioTime time);
AudioTime swiGetAudioTime( void );
AudioTime GetAudioClockTime( AudioClock *aclk );
void    HandleTimerSignal (void);

    /* trigger system (audio_trigger.c) */
Err InitAudioTrigger (void);
void ResetAllInstrumentTriggers (const AudioInstrument *);
void DisarmAllInstrumentTriggers (const AudioInstrument *);
void HandleTriggerSignal (void);

    /* audio_misc.c */
void afi_DeleteLinkedItems( List *itemList );
void afi_DeleteReferencedItems( List *refList );
int32 afi_DummyProcessor( void *i, void *p, uint32 tag, uint32 arg);
/* int32 afi_ReportMemoryUsage( void ); */
int32 afi_SuperDeleteItem( Item it );
int32 afi_SuperDeleteItemNode( ItemNode *n );
Err afi_IsRamAddr( const void *p, int32 Size );
void RemoveAndMarkNode (Node *);
int32 CountPackedStrings (const char *packedStringBuf, uint32 packedStringBufSize);
Err ValidateAudioItemData (const void *dataAddr, const int32 dataSize, bool autoFree);

#endif
