/******************************************************************************
**
**  @(#) audio_trigger.c 96/06/26 1.22
**
**  Audio Trigger system
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950504 WJB  Created skeleton.
**  950511 WJB  Added Init/TermAudioTrigger().
**              Added HandleTriggerSignal() skeleton.
**  950512 WJB  Implemented TriggerCueTable manager and HandleTriggerSignal().
**              Added skeleton for MonitorTrigger().
**  950515 WJB  Implemented MonitorTrigger().
**  950515 WJB  Added argument validation to MonitorTrigger().
**              Added default value for TriggerName in MonitorTrigger().
**              Added Cue override optimization to SetTriggerCue().
**  950516 WJB  Added UnmonitorInstrumenTriggers().
**  950516 WJB  Now allocating TriggerCueTable.
**              Removed some triple bangs.
**  950518 WJB  Cleaned up Init/TermAudioTrigger().
**              Added lots of maintenance notes.
**  950525 WJB  Changed from MonitorTrigger() to Arm/DisarmTrigger().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include "audio_internal.h"
#include "dspp_resources.h"     /* DSPR_NUM_TRIGGERS, DSPR_MAX_TRIGGERS */


/* -------------------- Debug */

#define DEBUG_HandleTriggerSignal   0
#define DEBUG_ArmTrigger            0   /* printf() in ArmTrigger() and related functions */
#define DEBUG_SignalTriggerCue      0   /* printf() inside SignalTriggerCue() */


/* -------------------- Local data */

/*
    TriggerCueEntry TriggerCueTable[DSPR_MAX_TRIGGERS];

        Trigger -> Cue map. Each entry is a TiggerCueEntry structure.
*/
typedef struct TriggerCueEntry {
    Item  tce_Cue;              /* Cue to send when trigger goes off. 0 when not disarmed. */
    uint8 tce_Flags;            /* ArmTrigger() Flags ANDed with TCE_FLAGS */
    uint8 tce_pad0[3];
} TriggerCueEntry;

#define TCE_FLAGS               (AF_F_TRIGGER_CONTINUOUS)
#define AF_F_TRIGGER_VALID_FLAGS (AF_F_TRIGGER_CONTINUOUS | AF_F_TRIGGER_RESET)

static TriggerCueEntry TriggerCueTable[DSPR_MAX_TRIGGERS];


/* -------------------- Local functions */

static void InternalArmTrigger (int32 trignum, Item cue, uint32 flags);
static void InternalRearmTrigger (int32 trignum);
static void InternalDisarmTrigger (int32 trignum);
static void InternalResetTrigger (int32 trignum);

static void SetTriggerCue (int32 trignum, Item cue, uint8 flags);
#define ClearTriggerCue(trignum) SetTriggerCue ((trignum), 0, 0)


/* -------------------- Init/Term */

/******************************************************************
**
**  Initialize Audio Trigger system. Called by InitAudioFolio()
**  (which is part of the audio daemon).
**
**  Notes
**      . Expects caller to clean up.
**      . The signal allocated by this function is never explicitly
**        freed, because task deletion automatically takes care of
**        that.
**      . Caller is expected to clear and disable all soft ints
**        as part of clean up from this (dsphTermDSPP() does this).
**
******************************************************************/

Err InitAudioTrigger (void)
{
        /* Allocate signal for trigger interrupt to signal audio daemon. */
    {
        const int32 sig = AllocSignal(0);

        if (sig <= 0) return sig ? sig : AF_ERR_NOSIGNAL;
        AB_FIELD(af_TriggerSignal) = sig;
    }
  #if DEBUG_HandleTriggerSignal
    printf ("InitAudioTrigger: af_TriggerSignal = 0x%08lx\n", AB_FIELD(af_TriggerSignal));
  #endif

    return 0;
}


/* -------------------- Monitor Trigger */

static int32 FindInsTrigger (const AudioInstrument *, const char *TriggerName);

 /**
 |||	AUTODOC -public -class audio -group Trigger -name ArmTrigger
 |||	Assigns a Cue(@) to an Instrument(@)'s Trigger.
 |||
 |||	  Synopsis
 |||
 |||	    Err ArmTrigger (Item instrument, const char *triggerName, Item cue,
 |||	                    uint32 flags)
 |||
 |||	  Description
 |||
 |||	    Assigns a Cue to be sent the next time that an Instrument's Trigger goes
 |||	    off.
 |||
 |||	    Triggers can be set to either continuous or one-shot mode by this function.
 |||	    In one-shot mode, the Trigger is automatically disarmed after the assigned
 |||	    Cue is sent (as if DisarmTrigger() had been called). In this mode, the
 |||	    Trigger must be re-armed by another call to ArmTrigger() in order to have
 |||	    the Cue sent for a subsequent Trigger event.
 |||
 |||	    In continuous mode, the assigned Cue is sent every time the Trigger goes
 |||	    off. It is never automatically disarmed; there's no need to manually re-arm
 |||	    it after receiving the Cue.
 |||
 |||	    A Trigger is normally only reset when its associated Cue is sent. This
 |||	    means if you use a one-shot Trigger, you won't lose Trigger events that
 |||	    happen while the Trigger is disarmed. If a Trigger event has already
 |||	    occured by the time the Trigger is armed, the Cue is sent immediately upon
 |||	    arming the Trigger. You can manually reset the Trigger when you arm it if
 |||	    you want to have the Cue sent the _next_ time the Trigger goes off, and not
 |||	    for any Trigger events that may have occured while the Trigger was disarmed.
 |||
 |||	    A Trigger can be armed, re-armed, or disarmed at any time. The Trigger mode
 |||	    can be changed between one-shot and continuous at any time. The most recent
 |||	    call to ArmTrigger() takes precendence for any given Trigger. Use
 |||	    DisarmTrigger() (a convenience macro) to disarm the trigger prior to
 |||	    disposing of the Cue assigned to the Trigger.
 |||
 |||	    The audio folio automatically disarms the Trigger when either the owning
 |||	    Instrument or assigned Cue are deleted while the Trigger is armed.
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        Instrument Item containing trigger to arm.
 |||
 |||	    triggerName
 |||	        Name of Instrument's Trigger. A NULL pointer causes the default name
 |||	        "Trigger" to be used.
 |||
 |||	    cue
 |||	        Cue Item to assign to Trigger.
 |||
 |||	    flags
 |||	        Optional set of flags described below, which may be used in any
 |||	        combination.
 |||
 |||	    AF_F_TRIGGER_CONTINUOUS
 |||	        When set, causes Trigger to be armed in continuous mode. Otherwise,
 |||	        Trigger is armed in one-shot mode.
 |||
 |||	    AF_F_TRIGGER_RESET
 |||	        Causes Trigger to be reset before being armed. Any accumulated Trigger
 |||	        events that have occured before this call is flushed. When not set, any
 |||	        previously occuring event from this Trigger would cause the Cue to be
 |||	        sent immediately.
 |||
 |||	  Return Value
 |||
 |||	    A non-negative value if successful or an error code (a negative value) if an
 |||	    error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V27.
 |||
 |||	  Caveats
 |||
 |||	    Care should be used when using AF_F_TRIGGER_CONTINUOUS mode. Use of this
 |||	    mode causes the audio folio's high priority task to re-enable a DSP
 |||	    software interrupt immediately after it has been serviced. If this
 |||	    interrupt occurs very frequently, the only task that could get time to run
 |||	    are the audio folio's task and anything of equal or higher priority (i.e.,
 |||	    system code only).
 |||
 |||	    A good example of what NOT to do is connect schmidt_trigger.dsp(@) to the
 |||	    output of an oscillator running at a really high frequency followed by
 |||	    arming schmidt_trigger.dsp(@)'s trigger in AF_F_TRIGGER_CONTINUOUS mode.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    DisarmTrigger(), schmidt_trigger.dsp(@), CreateCue(), MonitorAttachment()
 **/

 /**
 |||	AUTODOC -public -class audio -group Trigger -name DisarmTrigger
 |||	Remove previously assigned Cue(@) from an Instrument(@)'s Trigger.
 |||
 |||	  Synopsis
 |||
 |||	    Err DisarmTrigger (Item instrument, const char *triggerName)
 |||
 |||	  Description
 |||
 |||	    Disarms a Trigger previously armed by ArmTrigger(). Call this function to
 |||	    clean up in an orderly fashion before deleting the Cue that was last given
 |||	    to ArmTrigger(). Redundant, but harmless, if Trigger is already disarmed
 |||	    (either by a previous call to DisarmTrigger() or if the Trigger is in
 |||	    one-shot mode and has already gone off).
 |||
 |||	  Arguments
 |||
 |||	    instrument
 |||	        Instrument Item containing trigger to disarm.
 |||
 |||	    triggerName
 |||	        Name of Instrument's Trigger. A NULL pointer causes the default name
 |||	        "Trigger" to be used.
 |||
 |||	  Return Value
 |||
 |||	    A non-negative value if successful or an error code (a negative value) if an
 |||	    error occurs.
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
 |||	    ArmTrigger()
 **/

/* @@@ This function assumes single threaded access. Since this function has
       "last call takes precedence" behavior, it needs some kind of protection,
       at least around or inside the InternalArm/DisarmTrigger() calls in order
       to keep a consistent state for each Trigger. */

Err swiArmTrigger (Item Instrument, const char *TriggerName, Item Cue, uint32 Flags)
{
    const AudioInstrument *ains;
    int32 trignum;
    Err errcode;

  #if DEBUG_ArmTrigger
    printf ("swiArmTrigger: ins=0x%05lx trigname='%s' cue=0x%05lx flags=0x%02lx\n", Instrument, TriggerName ? TriggerName : "<NULL>", Cue, Flags);
  #endif

        /* Validate and process args */
    if (!(ains = (AudioInstrument *)CheckItem (Instrument, AUDIONODE, AUDIO_INSTRUMENT_NODE))) return AF_ERR_BADITEM;
    if (TriggerName) {
        if ((errcode = afi_IsRamAddr (TriggerName, 1)) < 0) return errcode;     /* !!! There ought to be a ValidateString() function somewhere! */
    }
    else {
        TriggerName = "Trigger";        /* Default trigger name */
    }
    if (Cue != 0 && !CheckItem (Cue, AUDIONODE, AUDIO_CUE_NODE)) return AF_ERR_BADITEM;
    if (Flags & ~AF_F_TRIGGER_VALID_FLAGS) return AF_ERR_OUTOFRANGE;

        /* Get allocated trigger number from instrument */
    if ((errcode = trignum = FindInsTrigger (ains, TriggerName)) < 0) return errcode;

        /* Arm if Cue is non-zero; Disarm if Cue is zero */
    if (Cue) InternalArmTrigger (trignum, Cue, Flags);
    else     InternalDisarmTrigger (trignum);

    return 0;
}


/******************************************************************
**
**  Find named Trigger.
**
**  Inputs
**
**      ains - AudioInstrument pointer of Instrument to monitor.
**             Doesn't validate this pointer.
**
**      TriggerName - Name of trigger to monitor. Doesn't validate
**                    this pointer. Must not be NULL. Name doesn't
**                    need to be valid for the instrument.
**
**  Results
**
**      Allocated trigger number (0..DSPR_NUM_TRIGGERS-1) on
**      success or Err code.
**
******************************************************************/

static int32 FindInsTrigger (const AudioInstrument *ains, const char *TriggerName)
{
    const DSPPInstrument * const dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
    const DSPPTemplate * const dtmp = dins->dins_Template;
    int32 rsrcindex;
    Err errcode;

    if ((errcode = rsrcindex = DSPPFindResourceIndex (dtmp, TriggerName)) < 0) return errcode;
    if (dtmp->dtmp_Resources[rsrcindex].drsc_Type != DRSC_TYPE_TRIGGER) return AF_ERR_NAME_NOT_FOUND;

    return dins->dins_Resources[rsrcindex].drsc_Allocated;
}


/* -------------------- Instrument creation/deletion support */

/******************************************************************
**
**  Resets all of instrument's triggers as part of instrument
**  creation. (called by internalCreateAudioIns()).
**
**  Inputs
**
**      ains - AudioInstrument pointer of Instrument to reset up.
**             Doesn't validate this pointer.
**
******************************************************************/

void ResetAllInstrumentTriggers (const AudioInstrument *ains)
{
    const DSPPInstrument * const dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
    const DSPPTemplate * const dtmp = dins->dins_Template;
    int32 i;

  #if DEBUG_ArmTrigger
    printf ("ResetAllInstrumentTriggers: ins item = 0x%05lx\n", ains->ains_Item.n_Item);
  #endif

    for (i=0; i<dtmp->dtmp_NumResources; i++) {
        if (dtmp->dtmp_Resources[i].drsc_Type == DRSC_TYPE_TRIGGER) {
            InternalResetTrigger (dins->dins_Resources[i].drsc_Allocated);
        }
    }
}


/******************************************************************
**
**  Disarms all of instrument's triggers as part of instrument
**  deletion. (called by internalDeleteAudioIns()).
**
**  Inputs
**
**      ains - AudioInstrument pointer of Instrument to clean up.
**             Doesn't validate this pointer.
**
******************************************************************/

void DisarmAllInstrumentTriggers (const AudioInstrument *ains)
{
    const DSPPInstrument * const dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
    int32 i;

  #if DEBUG_ArmTrigger
    printf ("DisarmAllInstrumentTriggers: ins item = 0x%05lx\n", ains->ains_Item.n_Item);
  #endif

    /* @@@ will need to use DSPPTemplate for resource allocation info when
           this information is no longer stored in DSPPInstrument. Not doing
           that now because dins_Template can invalid at the time that this
           function is called (shared code bug). That must also be prevented
           before the resource table can be optimized. */

    for (i=0; i<dins->dins_NumResources; i++) {
        if (dins->dins_Resources[i].drsc_Type == DRSC_TYPE_TRIGGER) {
            const int32 trignum = dins->dins_Resources[i].drsc_Allocated;

            InternalDisarmTrigger (trignum);
            InternalResetTrigger (trignum);      /* @@@ paranoid */
        }
    }
}


/* -------------------- Internal Arm/Disarm/Reset */

/* @@@ might need a semaphore someday, or some agreement about
       client holding a semaphore around these calls. */

/******************************************************************
**
**  Arm a trigger (internal version of ArmTrigger()). Optionally
**  resets trigger, then installs Cue in TriggerCueTable and enables
**  Trigger interrupt and signal handler.
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range.
**
**      cue - Cue item to assign to trigger. Assumed to be valid and
**            non-zero.
**
**      flags - AF_F_TRIGGER_ flags passed into ArmTrigger().
**              Assumed to be valid.
**
******************************************************************/

static void InternalArmTrigger (int32 trignum, Item cue, uint32 flags)
{
  #ifdef PARANOID
    if (trignum < 0 || trignum >= DSPR_NUM_TRIGGERS) {
        ERR(("InternalArmTrigger: trignum out of range = %ld\n", trignum));
        return;
    }
  #endif

    if (flags & AF_F_TRIGGER_RESET) InternalResetTrigger (trignum);
    SetTriggerCue (trignum, cue, (uint8)flags);
    InternalRearmTrigger (trignum);
}


/******************************************************************
**
**  Re-arm a previously armed trigger. Just enables the interrupt
**  and signal handler. Cue is assumed to have been already installed.
**  Called by HandleTriggerSignal() for continuous mode triggers.
**  Also called as tail end of InternalArmTrigger().
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range.
**
******************************************************************/

static void InternalRearmTrigger (int32 trignum)
{
    const uint32 trigmask = 1UL << trignum;

    AB_FIELD(af_ArmedTriggers) |= trigmask;
    dsphEnableTriggerInterrupts (trigmask);
}


/******************************************************************
**
**  Disarm a trigger (internal version of DisarmTrigger()). Disables
**  interrupt and signal handler, then clears entry in TriggerCueTable.
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range.
**
******************************************************************/

static void InternalDisarmTrigger (int32 trignum)
{
    const uint32 trigmask = 1UL << trignum;

  #ifdef PARANOID
    if (trignum < 0 || trignum >= DSPR_NUM_TRIGGERS) {
        ERR(("InternalDisarmTrigger: trignum out of range = %ld\n", trignum));
        return;
    }
  #endif

    dsphDisableTriggerInterrupts (trigmask);
    AB_FIELD(af_ArmedTriggers) &= ~trigmask;
    ClearTriggerCue (trignum);
}


/******************************************************************
**
**  Reset a trigger. Clears the flag(s) that indicate that a trigger
**  has arrived but hasn't had a Cue sent for it yet.
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range.
**
******************************************************************/

static void InternalResetTrigger (int32 trignum)
{
    const uint32 trigmask = 1UL << trignum;

  #ifdef PARANOID
    if (trignum < 0 || trignum >= DSPR_NUM_TRIGGERS) {
        ERR(("InternalResetTrigger: trignum out of range = %ld\n", trignum));
        return;
    }
  #endif

    dsphClearTriggerInterrupts (trigmask);
}


/* -------------------- Trigger Signal Handler */

static void SignalTriggerCue (int32 trignum);

/******************************************************************
**
**  Handle trigger signal from trigger interrupt handler.
**  Send Cue associated with every received Trigger.
**  Re-arm continuous mode Triggers.
**
******************************************************************/

void HandleTriggerSignal (void)
{
    /* @@@ this loop may require a semaphore held around the whole thing
           (including the dsphGetReceivedTriggers() call), if Arm/DisarmTrigger()
           can ever happen at the same time as this, in order to prevent the
           TriggerCueTable[], armed state, etc, from changing while this
           function is processing it. This problem could get especially icky if the
           owning instrument of a received trigger could be deleted and replaced
           with another instrument using the same trigger number.
    */

    uint32 trigmask = dsphGetReceivedTriggers (AB_FIELD(af_ArmedTriggers));
    int32 i;

    /* Triggers are in a quasi-state here. This function (by means of
       SignalTriggerCue()) completes the re-arming or disarming of
       each received trigger */

  #if DEBUG_HandleTriggerSignal
    printf ("HandleTriggerSignal() trigmask=0x%04lx\n", trigmask);
  #endif

    /* !!! use FindMSB() loop instead */
    for (i=0; trigmask && i<DSPR_NUM_TRIGGERS; i++) {
        const uint32 testmask = 1UL << i;

        if (trigmask & testmask) {
            trigmask &= ~testmask;
            SignalTriggerCue (i);
        }
    }
}


/******************************************************************
**
**  Signal Trigger's Cue. Looks up the Cue item before doing
**  anything. If Cue is no longer valid calls InternalDisarmTrigger().
**  Otherwise, sends the Cue's signal. If a one-shot Trigger, disarms
**  it. If a continuous Trigger, re-arms it.
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range. Entry
**                in TriggerCueTable is assumed to have a Cue in it.
**                This function validates the Cue from TriggerCueTable.
**
******************************************************************/

static void SignalTriggerCue (int32 trignum)
{
    const Item cue    = TriggerCueTable[trignum].tce_Cue;
    const uint8 flags = TriggerCueTable[trignum].tce_Flags;
        /* Only need LookupItem (instead of CheckItem()) here because we
           don't put any other kind of Item in TriggerCueTable and an
           Item can't change type. */
    const AudioCue * const acue = LookupItem (cue);

  #if DEBUG_SignalTriggerCue
    printf ("SignalTriggerCue: trignum=%ld cue=0x%06lx flags=0x%02x\n", trignum, cue, flags);
  #endif

    if (acue) {
        SuperinternalSignal (acue->acue_Task, acue->acue_Signal);
        if (flags & AF_F_TRIGGER_CONTINUOUS) InternalRearmTrigger (trignum);
        else InternalDisarmTrigger (trignum);
    }
    else {
        ERR(("SignalTriggerCue: Cue 0x%06lx deleted!\n", cue));
        InternalDisarmTrigger (trignum);
    }
}


/* -------------------- Cue Table Manager */

/******************************************************************
**
**  Install a Cue in TriggerCueTable.
**
**  Inputs
**
**      trignum - trigger number. Assumed to be in range.
**
**      cue - Cue item to install. Assumed to be valid and non-zero
**
**      flags - AF_F_TRIGGER_ flags to install (Ignores flags not
**              in TCE_FLAGS).
**
******************************************************************/

static void SetTriggerCue (int32 trignum, Item cue, uint8 flags)
{
  #if DEBUG_ArmTrigger
    printf ("SetTriggerCue: trignum=%ld cue=0x%05lx flags=0x%02x\n", trignum, cue, flags);
  #endif

    TriggerCueTable[trignum].tce_Cue   = cue;
    TriggerCueTable[trignum].tce_Flags = flags & TCE_FLAGS;
}
