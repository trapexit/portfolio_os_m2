/******************************************************************************
**
**  @(#) audio_items.c 96/08/23 1.9
**
**  Audio Item management functions.
**
**  By: Bill Barton and Phil Burk
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960528 WJB  Created from audio_folio.c
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include "audio_internal.h"


/* -------------------- Debug */

#define DEBUG_Item  0   /* Item service debugging */

#if DEBUG_Item
#include <stdio.h>
#define DBUGITEM(x) PRT(x)
#else
#define DBUGITEM(x)
#endif


/* -------------------- ItemRoutines */

static Item  internalCreateAudioItem (void *n, uint8 ntype, void *args);
static int32 internalDeleteAudioItem (Item it, Task *t);
static Item  internalFindAudioItem (int32 ntype, TagArg *args);
static Item  internalOpenAudioItem (OpeningItemNode *, void *args, Task *);
static int32 internalCloseAudioItem (Item it, Task *task);
static Err   internalSetItemOwner (ItemNode *n, Item NewOwner, struct Task *t);
static Item  internalLoadAudioItem (int32 ntype, TagArg *args);


/******************************************************************/
/*
    Install Audio ItemRoutines in AudioBase

    Arguments
        ab
            AudioFolio to modify

    Results
        Sets ab->af_Folio.f_ItemRoutines.
*/

void InstallAudioItemRoutines (AudioFolio *ab)
{
    static const ItemRoutines AudioItemRoutines = {
        internalFindAudioItem,          /* ir_Find */
        internalCreateAudioItem,        /* ir_Create */
        internalDeleteAudioItem,        /* ir_Delete */
                                        /* ir_Open */
        (Item (*)(Node *, void *, struct Task *))internalOpenAudioItem,
        internalCloseAudioItem,         /* ir_Close */
        NULL,                           /* ir_SetPriority */
        internalSetItemOwner,           /* ir_SetOwner */
        internalLoadAudioItem           /* ir_Load */
    };

    *ab->af_Folio.f_ItemRoutines = AudioItemRoutines;
}


/******************************************************************/
/*
** audio folio ir_Create vector
** This routine is passed an item pointer in n.
** The item is (I think) allocated by CreateItem() in the kernel
** based on information in the AudioNodeData array.
*/
static Item internalCreateAudioItem (void *n, uint8 ntype, void *args)
{
    Item Result;

    /* @@@ args are validated by TagProcessor() called by the create vector for each node type */

    DBUGITEM(("internalCreateAudioItem: Item 0x%x (type %d) @ 0x%x\n", ((ItemNode *)n)->n_Item, ntype, n));
    DBUGITEM(("                         Caller 0x%x ('%s') @ 0x%x\n", CURRENTTASKITEM, CURRENTTASK->t.n_Name, CURRENTTASK));

    TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioItem(0x%lx, %d, 0x%lx)\n", n, ntype, args));

    CHECKAUDIOOPEN;

    TRACKMEM(("Begin internalCreateAudioItem: n=0x%x, ntype=0x%x\n", n,ntype));

    switch (ntype)
    {
        case AUDIO_TEMPLATE_NODE:
            Result = internalCreateAudioTemplate ((AudioInsTemplate *)n, (TagArg *)args);
            break;

        case AUDIO_INSTRUMENT_NODE:
            Result = internalCreateAudioIns ((AudioInstrument *)n, (TagArg *)args);
            break;

        case AUDIO_KNOB_NODE:
            Result = internalCreateAudioKnob ((AudioKnob *)n, (TagArg *)args);
            break;

        case AUDIO_SAMPLE_NODE:
            Result = internalCreateAudioSample ((AudioSample *)n, (TagArg *)args);
            break;

        case AUDIO_CUE_NODE:
            Result = internalCreateAudioCue ((AudioCue *)n, (TagArg *)args);
            break;

        case AUDIO_ENVELOPE_NODE:
            Result = internalCreateAudioEnvelope ((AudioEnvelope *)n, (TagArg *)args);
            break;

        case AUDIO_ATTACHMENT_NODE:
            Result = internalCreateAudioAttachment ((AudioAttachment *)n, (TagArg *)args);
            break;

        case AUDIO_TUNING_NODE:
            Result = internalCreateAudioTuning ((AudioTuning *)n, (TagArg *)args);
            break;

        case AUDIO_PROBE_NODE:
            Result = internalCreateAudioProbe ((AudioProbe *)n, (TagArg *)args);
            break;

        case AUDIO_CLOCK_NODE:
            Result = internalCreateAudioClock ((AudioClock *)n, (TagArg *)args);
            break;

        default:
            Result = AF_ERR_BADSUBTYPE;
            break;
    }

    TRACKMEM(("End internalCreateAudioItem: n=0x%x\n", n));

    TRACER(TRACE_INT,TRACE_ITEM,("internalCreateAudioItem returns 0x%lx\n", Result));
    return (Result);
}

/******************************************************************
    audio folio ir_Delete vector

    Arguments
        it
            Item to delete. Kernel guarantees item is valid and task
            has permission to delete it.

        ct
            Task on whose behalf item is being deleted. Assumed to be
            the Item's owner.

    Results
        0
            Item may be deleted

        >0
            Don't delete Item, return 0

        <0
            Don't delete Item, return Err code to caller
*/
static int32 internalDeleteAudioItem (Item it, Task *ct)
{
    ItemNode * const n = LookupItem (it);
    int32 Result;

    TRACEE(TRACE_INT,TRACE_ITEM,("DeleteAudioItem (it=0x%lx, Task=0x%lx)\n", it, t));
    TRACEB(TRACE_INT,TRACE_ITEM,("DeleteAudioItem: n = $%x, type = %d\n", n, n->n_Type));

#if DEBUG_Item
    PRT(("internalDeleteAudioItem: Item 0x%x (type %d", it, n->n_Type));
    if (n->n_Name) PRT((", name '%s'", n->n_Name));
    PRT((") @ 0x%x\n"
    /**/ "                         Owner 0x%x ('%s') @ 0x%x\n", n, ct->t.n_Item, ct->t.n_Name, ct));
#endif

    TRACKMEM(("Begin internalDeleteAudioItem: it=0x%x\n", it));

    switch (n->n_Type)
    {
        case AUDIO_INSTRUMENT_NODE:
            Result = internalDeleteAudioIns ((AudioInstrument *)n);
            break;

        case AUDIO_TEMPLATE_NODE:
            Result = internalDeleteAudioTemplate ((AudioInsTemplate *)n, ct);
            break;

        case AUDIO_SAMPLE_NODE:
            Result = internalDeleteAudioSample ((AudioSample *)n, ct);
            break;

        case AUDIO_KNOB_NODE:
            Result = internalDeleteAudioKnob ((AudioKnob *)n);
            break;

        case AUDIO_CUE_NODE:
            Result = internalDeleteAudioCue ((AudioCue *)n);
            break;

        case AUDIO_ENVELOPE_NODE:
            Result = internalDeleteAudioEnvelope ((AudioEnvelope *)n, ct);
            break;

        case AUDIO_ATTACHMENT_NODE:
            Result = internalDeleteAudioAttachment ((AudioAttachment *)n, ct);
            break;

        case AUDIO_TUNING_NODE:
            Result = internalDeleteAudioTuning ((AudioTuning *)n, ct);
            break;

        case AUDIO_PROBE_NODE:
            Result = internalDeleteAudioProbe ((AudioProbe *)n);
            break;

        case AUDIO_CLOCK_NODE:
            Result = internalDeleteAudioClock ((AudioClock *)n);
            break;

        default:
            ERR(("internalDeleteAudioItem: unrecognised type = %d\n", n->n_Type));
            Result = AF_ERR_BADITEM;
            break;
    }

    TRACKMEM(("End internalDeleteAudioItem: it=0x%x\n", it));

    TRACER(TRACE_INT,TRACE_ITEM,("DeleteAudioItem: Result = $%x\n", Result));

    return Result;
}


/******************************************************************/
/* audio folio ir_Find vector */
static Item internalFindAudioItem (int32 ntype, TagArg *args)
{
    switch (ntype)
    {
        case AUDIO_TEMPLATE_NODE:
            return internalFindInsTemplate( args );

        default:
            return AF_ERR_BADITEM;
    }
}


/******************************************************************/
/* audio folio ir_Open vector */
static Item internalOpenAudioItem (OpeningItemNode *node, void *args, Task *task)
{
    TOUCH(args);

#if DEBUG_Item
    PRT(("internalOpenAudioItem: Item 0x%x (type %d", node->n_Item, node->n_Type));
    if (node->n_Name) PRT((", name '%s'", node->n_Name));
    PRT((") @ 0x%x OpenCount=%d\n"
    /**/ "                       Opener 0x%x ('%s') @ 0x%x\n", node, node->n_OpenCount, task->t.n_Item, task->t.n_Name, task));
#endif

        /* only support opening items on behalf of the audio daemon */
    if (task->t.n_Item != AB_FIELD(af_AudioDaemonItem)) {
        ERR(("internalOpenAudioItem: invalid opener 0x%x", task->t.n_Item));
        return AF_ERR_BADPRIV;
    }

    switch (node->n_Type)
    {
        case AUDIO_TEMPLATE_NODE:
            return internalOpenInsTemplate( (AudioInsTemplate *) node );

        case AUDIO_INSTRUMENT_NODE:
            return internalOpenInstrument( (AudioInstrument *) node );

        default:
            return AF_ERR_BADITEM;
    }
}


/******************************************************************/
/* audio folio ir_Close vector */
static int32 internalCloseAudioItem(Item it, Task *task)
{
    OpeningItemNode * const n = (OpeningItemNode *)LookupItem(it);

    TOUCH(task);

#if DEBUG_Item
    PRT(("internalCloseAudioItem: Item 0x%x (type %d", it, n->n_Type));
    if (n->n_Name) PRT((", name '%s'", n->n_Name));
    PRT((") @ 0x%x OpenCount=%d\n"
    /**/ "                        Opener 0x%x ('%s') @ 0x%x\n", n, n->n_OpenCount, task->t.n_Item, task->t.n_Name, task));
#endif

    switch (n->n_Type)
    {
        case AUDIO_TEMPLATE_NODE:
            return internalCloseInsTemplate( (AudioInsTemplate *) n );

        case AUDIO_INSTRUMENT_NODE:
            return internalCloseInstrument( (AudioInstrument *) n );

        default:
            return AF_ERR_BADITEM;
    }
}


/***************************************************************** 940617 */
/* audio folio ir_SetOwner vector (just a stub to permit SetItemOwner() to operate) */
static Err internalSetItemOwner(ItemNode *n, Item newOwner, struct Task *t)
{
    TOUCH(n);
    TOUCH(newOwner);
    TOUCH(t);

    DBUGITEM(("internalSetItemOwner: n->n_Owner = 0x%x, newOwner = 0x%x\n", n->n_Owner, newOwner));

    return 0;
}


/******************************************************************/
/* audio folio ir_Load vector */
static Item internalLoadAudioItem (int32 ntype, TagArg *args)
{
    switch (ntype)
    {
        case AUDIO_TEMPLATE_NODE:
            return internalLoadSharedTemplate( args );

        default:
            return AF_ERR_BADITEM;
    }
}


/* -------------------- Get/SetAudioItemInfo() */

/******************************************************************/
/**
|||	AUTODOC -public -class audio -group Miscellaneous -name SetAudioItemInfo
|||	Sets parameters of an audio item.
|||
|||	  Synopsis
|||
|||	    Err SetAudioItemInfo (Item audioItem, const TagArg *tagList)
|||
|||	    Err SetAudioItemInfoVA (Item audioItem, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This procedure sets one or more parameters of an audio item. This is the
|||	    only way to change parameter settings of an audio item. The parameters are
|||	    determined by the tag arguments.
|||
|||	    If this function fails, the resulting state of the Item you were attempting
|||	    to modify depends on the kind of item:
|||
|||	    Envelope, Sample, Tuning
|||	        The item is left unchanged if SetAudioItemInfo() fails for any reason.
|||
|||	    Attachment
|||	        All of the changes prior to the tag which failed take effect.
|||
|||	  Arguments
|||
|||	    audioItem
|||	        Item number of an audio item to modify.
|||
|||	  Tag Arguments
|||
|||	    See specific Audio Item pages for supported tags.
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
|||	  Caveats
|||
|||	    The failure behavior of attachments may be changed to be like the others.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    GetAudioItemInfo(), Attachment(@), AudioClock(@), Cue(@), Envelope(@),
|||	    Instrument(@), Knob(@), Probe(@), Sample(@), Template(@), Tuning(@)
**/
Err swiSetAudioItemInfo (Item audioItem, const TagArg *tagList)
{
    ItemNode * const n = LookupItem (audioItem);

    CHECKAUDIOOPEN;

    if (!n || n->n_SubsysType != AUDIONODE) return AF_ERR_BADITEM;
    /* !!! check owner, maybe only permit this if owned by the caller or if priveleged? */

    /* @@@ tagList is validated by use of SafeFirst/NextTagArg() called by the
    **     modify vector for each node type */

    switch (n->n_Type) {
        case AUDIO_SAMPLE_NODE:
            return internalSetSampleInfo ((AudioSample *)n, tagList);

        case AUDIO_ENVELOPE_NODE:
            return internalSetEnvelopeInfo ((AudioEnvelope *)n, tagList);

        case AUDIO_ATTACHMENT_NODE:
            return internalSetAttachmentInfo ((AudioAttachment *)n, tagList);

        case AUDIO_TUNING_NODE:
            return internalSetTuningInfo ((AudioTuning *)n, tagList);

      #if 0   /* unimplemented */
        case AUDIO_INSTRUMENT_NODE:
            return internalSetInstrumentInfo ((AudioInstrument *)n, tagList);

        case AUDIO_TEMPLATE_NODE:
            return internalSetTemplateInfo ((AudioInsTemplate *)n, tagList);

        case AUDIO_KNOB_NODE:
            return internalSetKnobInfo ((AudioKnob *)n, tagList);

        case AUDIO_CUE_NODE:
            return internalSetCueInfo ((AudioCue *)n, tagList);
      #endif

        default:
            return AF_ERR_BADITEM;
    }
}

/******************************************************************/
/**
|||	AUTODOC -public -class audio -group Miscellaneous -name GetAudioItemInfo
|||	Gets information about an audio item.
|||
|||	  Synopsis
|||
|||	    Err GetAudioItemInfo (Item audioItem, TagArg *tagList)
|||
|||	  Description
|||
|||	    This procedure queries settings of several kinds of audio items. It takes a
|||	    tag list with the ta_Tag fields set to parameters to query and fills in the
|||	    ta_Arg field of each TagArg with the parameter requested by that TagArg.
|||
|||	    SetAudioItemInfo() can be used to set audio item parameters.
|||
|||	  Arguments
|||
|||	    audioItem
|||	        Item number of an audio item to query.
|||
|||	    tagList
|||	        Pointer to tag list containing AF_TAG_ tags to query. The ta_Arg fields
|||	        of each matched tag is filled in with the corresponding setting from the
|||	        audio item. TAG_JUMP and TAG_NOP are treated as they normally are for
|||	        tag processing.
|||
|||	        See each audio Item pages for list of tags that are supported by each
|||	        item type.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	    AF_ERR_BADTAG
|||	        If this function is passed any tag that is not listed in the
|||	        appropriate item page as supporting being queried.
|||
|||	    AF_ERR_BADITEM
|||	        If this function is passed an Item of a type which it doesn't support.
|||
|||	    Fills in the ta_Arg field of each matched TagArg in tagList with the
|||	    corresponding setting from the audio item. If a bad tag is passed, all
|||	    TagArgs prior to the bad tag are filled out.
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
|||	    SetAudioItemInfo(), Attachment(@), AudioClock(@), Cue(@), Envelope(@),
|||	    Instrument(@), Knob(@), Probe(@), Sample(@), Template(@), Tuning(@)
**/
Err GetAudioItemInfo (Item audioItem, TagArg *tagList)
{
    const ItemNode * const n = LookupItem (audioItem);

    CHECKAUDIOOPEN;
    if (!n || n->n_SubsysType != AUDIONODE) return AF_ERR_BADITEM;

    switch (n->n_Type) {
        case AUDIO_INSTRUMENT_NODE:
            return internalGetInstrumentInfo ((AudioInstrument *)n, tagList);

        case AUDIO_KNOB_NODE:
            return internalGetKnobInfo ((AudioKnob *)n, tagList);

        case AUDIO_SAMPLE_NODE:
            return internalGetSampleInfo ((AudioSample *)n, tagList);

        case AUDIO_ENVELOPE_NODE:
            return internalGetEnvelopeInfo ((AudioEnvelope *)n, tagList);

        case AUDIO_ATTACHMENT_NODE:
            return internalGetAttachmentInfo ((AudioAttachment *)n, tagList);

      #if 0   /* unimplemented */
        case AUDIO_TEMPLATE_NODE:
            return internalGetTemplateInfo ((AudioInsTemplate *)n, tagList);

        case AUDIO_CUE_NODE:
            return internalGetCueInfo ((AudioCue *)n, tagList);

        case AUDIO_TUNING_NODE:
            return internalGetTuningInfo ((AudioTuning *)n, tagList);
      #endif

        default:
            return AF_ERR_BADITEM;
    }
}

