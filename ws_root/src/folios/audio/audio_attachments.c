/* @(#) audio_attachments.c 96/09/20 1.65 */
/* $Id: audio_attachments.c,v 1.59 1995/03/08 08:41:49 phil Exp phil $ */
/****************************************************************
**
** Audio Internals to support Attachments
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
** 930415 PLB Track connections between Items
** 930517 PLB Add NOAUTOSTART and FATLADYSINGS flags
** 930726 PLB Fixed bug in StartAt range check for NumFrames == 0
** 930829 PLB Only allow Template or Instrument in CreateAttachment
** 930828 PLB Check for illegal flags in SetAttachmentInfo
** 930828 PLB Check for Cue requests other than CUE_AT_END
** 931117 PLB Support for envelopes added to WhereAttachment
** 931130 PLB Fix bug with premature STOP for looping samples with FATLADYSINGS
** 931201 PLB Keep an attachment reference for template attachments too.
**            This fixes a bug Don found if he shared a sample on multiple templates
**            and and tried to delete one template.
** 931220 PLB Fixed cutoff of sound if Attachment2 Linked or Unlinked when
**               Attachment1 has loops.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 930404 PLB Fix trace debug, c/aitp/aatt/
** 940422 DGB Change spelling of SetMode to SETMODE, same as inthard.h
** 940623 WJB Added some potential internalGetAttachmentInfo() innards (commented out until approved).
** 940810 PLB Only set SETMODE for Input DMA channels.
** 940811 WJB Fixed AllocSignal() usage to correctly detect a 0 result.
** 940825 PLB Distinguish between Sample and Envelope Attachments in
**            StartAttachment(), ReleaseAttachment(), StopAttachment()
** 940901 PLB Use dsphEnableChannelInterrupt()
** 940922 PLB swiLinkAttachment distinguishes between samples and envelopes.
**            Change slow calls to CheckItem() to LookupItem().
** 941116 PLB Removed SendAttachment(). Redesign and implement later.
** 941121 PLB internalCreateAttachment() - Prevent memory leak if AT_TAG_HOOKNAME
**            called more than once.  Plug hole if AF_TAG_SAMPLE and AF_TAG_ENVELOPE
**            both used.
** 941121 PLB Fixed interrupt disable from interrupt.  Was improperly converting
**            from channel to interrupt.  Bug only in babs version.
** 950131 WJB Fixed includes.
** 950217 PLB Moved interrupt stuff to dspp_irq_anvil.c
** 951206 WJB Added support for AF_TAG_AUTO_DELETE_SLAVE.
**            Rewrote AUDIO_ATTACHMENT_NODE ir_Delete method.
**            Replaced AF_TAG_TIME_SCALE with AF_TAG_TIME_SCALE_FP.
**            Added some debugging code.
** 951206 WJB Template Attachment AF_TAG_START_AT now propagates to Instrument Attachments.
** 960124 WJB Added rough version of GetAttachments().
**            Implemented internalGetAttachmentInfo().
** 960229 PLB Remove TIME SCALE for attachments.
** 960311 WJB Added common cleanup code for internalCreate/DeleteAudioAttachment().
** 960618 PLB Fix CR6132 and CR6210.  Error checks for Template attachments.
** 960813 WJB Rewrote internalCreateAudioAttachment().
****************************************************************/

#include <dspptouch/dspp_touch.h>

#include "audio_internal.h"


/* -------------------- Debugging */

#define DEBUG_Item  0
#define DEBUG_Query 0
#define DBUG(x)     /* PRT(x) */

#if DEBUG_Item
#define DBUGITEM(x) PRT(x)
static void internalDumpAttachment (const AudioAttachment *, const char *banner);
#else
#define DBUGITEM(x)
#endif

#if DEBUG_Query
#define DBUGQUERY(x) PRT(x)
#else
#define DBUGQUERY(x)
#endif


/* -------------------- Local functions */

	/* hook helpers */
static int32 GetAttachmentHookType (const AudioInsTemplate *, const char *hookName);
static const char *GetDefaultAttachmentHookName (const AudioInsTemplate *, uint8 attType);
static int32 dsppFindDefaultFIFORsrcIndex (const DSPPTemplate *);


/* -------------------- Code */

/*************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name CreateAttachment
 |||	Creates and Attachment(@) between a Sample(@) or Envelope(@) to an Instrument(@)
 |||	or Template(@).
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateAttachment (Item master, Item slave, const TagArg *tagList)
 |||
 |||	    Item CreateAttachmentVA (Item master, Item slave, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure connects a Sample or Envelope (slave Item) to an Instrument
 |||	    or Template (master Item). This sample or envelope will be played when the
 |||	    instrument is started.  If the procedure is successful, it returns an
 |||	    attachment item number.
 |||
 |||	    When you finish with the attachment, you should call DeleteAttachment() to
 |||	    detach and free the attachment's resources. Deleting either the master or
 |||	    slave Item does this automatically.
 |||
 |||	    Some Instruments may have multiple places where Envelopes or Samples can be
 |||	    attached. In that case you must specify which attachment point (the hook) is
 |||	    to be used. An instrument may, for example, have an amplitude envelope and a
 |||	    filter envelope, which may be called AmpEnv and FilterEnv respectively. You
 |||	    can specify whcih hook to use by passing its name with the AF_TAG_NAME tag.
 |||
 |||	    If you do not specify a hook name, then the name "Env" is assumed for
 |||	    envelopes. For samples, it is assumed that you wish use the only sample hook
 |||	    available. Sample players have a hook called "InFIFO". Delay instruments
 |||	    have a hook called "OutFIFO". If you do not specify a hook name for a
 |||	    sample, and the instrument has more than one sample hook then
 |||	    AF_ERR_BAD_NAME is returned.
 |||
 |||	    You may use the same envelope for several instruments. The envelope data may
 |||	    be edited at any time, but be aware that it is read by a high priority task
 |||	    in the audio folio. Thus you should not leave it in a potentially "goofy"
 |||	    state if it is actively used.
 |||
 |||	    If multiple Sample(@)s are attached to a FIFO, and the instrument is started
 |||	    with a specified pitch, then the list of samples is searched for the first
 |||	    sample whose range of notes and velocities matches the desired note index
 |||	    (pitch). The LowNote and HighNote, and LowVelocity and HighVelocity are read
 |||	    from the AIFF file. The values from the file can be overwritten using
 |||	    SetAudioItemInfo() on each Sample.
 |||
 |||	    By default when an Attachment is deleted, the slave Item is unaffected. An
 |||	    Attachment created with { AF_TAG_AUTO_DELETE_SLAVE, TRUE } causes its slave
 |||	    to be automatically deleted when the Attachment is deleted. Because
 |||	    Attachments themselves are automatically deleted when either the master or
 |||	    slave is deleted, the auto-deletion effect can be triggered by deleting
 |||	    either the master or slave of such an attachment.
 |||
 |||	    Most kinds of slave Items may be attached to both Instruments and an
 |||	    Instrument Templates. The only exceptions are Delay Line Templates, which may
 |||	    be attached only to Instrument Templates.
 |||
 |||	    If an Instrument Template has any Attachments when an Instrument is created
 |||	    from that Instrument Template, then a matching set of Attachments to the
 |||	    new Instrument is automatically created. See Instrument(@) for details.
 |||
 |||	  Arguments
 |||
 |||	    master
 |||	        The item number of the Instrument or Template.
 |||
 |||	    slave
 |||	        The item number of the Sample or Envelope.
 |||
 |||	  Tag Arguments
 |||
 |||	    See Attachment(@) for legal tags.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the Attachment (a non-negative
 |||	    value) or an error code a negative value) if an error occurs.
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
 |||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE),
 |||	                      AF_TAG_MASTER, master,
 |||	                      AF_TAG_SLAVE, slave,
 |||	                      TAG_JUMP, tagList);
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    DeleteAttachment(), Attachment(@), Instrument(@), Template(@), Envelope(@), Sample(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Attachment -name DeleteAttachment
 |||	Undoes Attachment(@) between Sample(@) or Envelope(@) and Instrument(@) or
 |||	Template(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteAttachment (Item attachment)
 |||
 |||	  Description
 |||
 |||	    This procedure disconnects a Sample or an Envelope from an Instrument or
 |||	    Template, deletes the attachment item connecting the two, and frees the
 |||	    attachment's resources. If the attachment was created with
 |||	    { AF_TAG_AUTO_DELETE_SLAVE, TRUE }, then the slave Sample or Envelope is also
 |||	    deleted. Otherwise, the slave item is unaffected.
 |||
 |||	  Arguments
 |||
 |||	    attachment
 |||	        The item number of the Attachment.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
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
 |||	    CreateAttachment(), Attachment(@)
 **/


/*****************************************************************/
/**  Send a signal to any Cue that may be monitoring. ************/
/*****************************************************************/
int32 SignalMonitoringCue( AudioAttachment *aatt )
{
	AudioCue *acue;

/* Signal task that called MonitorAttachment. */
	if(aatt->aatt_CueItem)
	{
/*	940922	acue = (AudioCue *)CheckItem(aatt->aatt_CueItem,  AUDIONODE, AUDIO_CUE_NODE); */
		acue = (AudioCue *)LookupItem(aatt->aatt_CueItem );
		if (acue == NULL)
		{
			ERR(("SignalMonitoringCue: Cue deleted!\n"));
			aatt->aatt_CueItem = 0;
		}
		else
		{
DBUG(("SignalMonitoringCue: cue = 0x%x, signal = 0x%x\n", acue, acue->acue_Signal ));
			SuperinternalSignal( acue->acue_Task, acue->acue_Signal );
		}
	}
	return 0;
}

/*****************************************************************/
/**  Process signal from DMA interrupt. ***************************/
/*****************************************************************/
int32 ProcessAttachmentDMA( AudioAttachment *PrevAtt )
{
	AudioAttachment *CurAtt;
	AudioDMAControl *admac;
	int32 Result;
	int32 DMAChan;

	Result=0;

	DMAChan = PrevAtt->aatt_Channel;
	admac = &DSPPData.dspp_DMAControls[DMAChan];

DBUG(("ProcessAttachmentDMA( 0x%x ) SegmentCount = 0x%x\n", PrevAtt, PrevAtt->aatt_SegmentCount));
DBUG(("PrevAtt->aatt_ActivityLevel = 0x%x\n", PrevAtt->aatt_ActivityLevel));
	if((PrevAtt->aatt_SegmentCount > 0) &&
		(--PrevAtt->aatt_SegmentCount == 0))  /* 931130 */
	{
		PrevAtt->aatt_ActivityLevel = AF_STOPPED;
/* This will prevent the HandleDMASignal() routine from calling us again.
** DSPPGoToAttachment() will set this to the next attachment. */
		admac->admac_AttachmentItem = 0;
		ClearDelayLineIfStopped ((AudioSample *)PrevAtt->aatt_Structure);

		if (PrevAtt->aatt_Flags & AF_ATTF_FATLADYSINGS)
		{
DBUG(("ProcessAttachmentDMA: FATLADYSINGS=>STOP\n"));
			Result = swiStopInstrument( PrevAtt->aatt_HostItem, NULL );
			if (Result < 0)
			{
				ERRDBUG(("ProcessAttachmentDMA: error stopping 0x%x\n",
					PrevAtt->aatt_HostItem));
				return Result;
			}
		}
		else if ( PrevAtt->aatt_NextAttachment )
		{
/*	940922		CurAtt = (AudioAttachment *)CheckItem(PrevAtt->aatt_NextAttachment,
**				AUDIONODE, AUDIO_ATTACHMENT_NODE);
*/
			CurAtt = (AudioAttachment *)LookupItem(PrevAtt->aatt_NextAttachment);
			if (CurAtt == NULL)
			{
/* Just eliminate the reference and point to silence. */
				ERR(("ProcessAttachmentDMA: Attachment deleted!\n"));
				 PrevAtt->aatt_NextAttachment = 0;
				DSPP_SilenceDMA(PrevAtt->aatt_Channel);
			}
			else
			{
DBUG(("ProcessAttachmentDMA: Call DSPPStartAttachment()\n"));
				DSPPStartSampleAttachment( CurAtt, FALSE );
			}
		}
		else
		{
			DSPP_SilenceDMA(PrevAtt->aatt_Channel);
DBUG(("ProcessAttachmentDMA: Silence.\n"));
		}

/* Signal task that called MonitorAttachment. */
DBUG(("ProcessAttachmentDMA: Send monitoring signal.\n"));
		SignalMonitoringCue( PrevAtt );
	}
	else
	{
		EnableAttSignalIfNeeded( PrevAtt );
	}

	return Result;
}
/*****************************************************************/
/* Scan DMAControls to see which one(s) signalled.
** Send Cue to those.
*/

int32 HandleDMASignal (void)
{
	uint32 Channels;
	AudioDMAControl *admac;
	AudioAttachment *PrevAtt;

DBUG(("\nHandleDMASignal()\n"));

	Channels = dsphGetCompletedDMAChannels();
DBUG(("\nHandleDMASignal: Channels = 0x%08x\n", Channels ));

	admac = &DSPPData.dspp_DMAControls[0];
/* Shift Channels to right, checking each bit as it passes through LSB position. */
/* !!! use FindMSB() loop instead */
	while( Channels != 0)
	{
		if (Channels & 1)
		{
			if(admac->admac_AttachmentItem)
			{
/* PrevAtt just finished */
				PrevAtt = (AudioAttachment *) LookupItem(admac->admac_AttachmentItem);
				if(PrevAtt == NULL)
				{
					ERR(("HandleDMASignal: warning, Attachment deleted.\n"));
				}
				else
				{
					ProcessAttachmentDMA( PrevAtt );
				}
			}
		}
		Channels = Channels >> 1;
		admac++;
	}
	return 0;
}


/* -------------------- Item managment */

static void StripAudioAttachment (AudioAttachment *aatt);


/*****************************************************************/
 /**
 |||	AUTODOC -public -class Items -group Audio -name Attachment
 |||	The binding of a Sample(@) or an Envelope(@) to an Instrument(@) or Template(@).
 |||
 |||	  Description
 |||
 |||	    An Attachment is the item which binds a Sample or an Envelope (slave Item)
 |||	    to a particular Instrument or Instrument Template (master Item).
 |||
 |||	    An Attachment is associated with precisely one master Item and one slave
 |||	    Item. An Attachment is said to be an Envelope Attachment if its slave is an
 |||	    Envelope, or a Sample Attachment if its slave is any kind of Sample
 |||	    (ordinary, delay line, or delay line template).
 |||
 |||	    Sample Attachments actually come in two kinds: input and output, defined by
 |||	    the whether the Hook to which the Sample is attached is an input FIFO or
 |||	    output FIFO. Both kinds are considered Sample Attachments and no distinction
 |||	    is made between them.
 |||
 |||	    A master Item can have one Envelope Attachment per Envelope hook and one
 |||	    Sample Attachment per Output FIFO. A master Item can have multiple Sample
 |||	    Attachments per Input FIFO, but only one will be selected to be played when
 |||	    the instrument is started. This is useful for creating multi-sample
 |||	    instruments, where the sample selected to be played depends on the pitch to
 |||	    be played.
 |||
 |||	    A slave Item can have any number of Attachments made to it.
 |||
 |||	    Attachments are automatically deleted when either its master or slave Item
 |||	    is deleted.
 |||
 |||	    By default when an Attachment is deleted, the slave Item is unaffected. An
 |||	    Attachment created with { AF_TAG_AUTO_DELETE_SLAVE, TRUE } causes its slave
 |||	    to be automatically deleted when the Attachment is deleted. Because
 |||	    Attachments themselves are automatically deleted when either the master or
 |||	    slave is deleted, the auto-deletion effect can be triggered by deleting
 |||	    either the master or slave of such an attachment.
 |||
 |||	    Most kinds of slave Items may be attached to both Instruments and an
 |||	    Instrument Templates. The only exceptions are Delay Line Templates, which may
 |||	    be attached only to Instrument Templates.
 |||
 |||	    If an Instrument Template has any Attachments when an Instrument is created
 |||	    from that Instrument Template, then a matching set of Attachments to the
 |||	    new Instrument is automatically created. See Instrument(@) for details.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_ATTACHMENT_NODE
 |||
 |||	  Create
 |||
 |||	    CreateAttachment()
 |||
 |||	  Delete
 |||
 |||	    DeleteAttachment()
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
 |||	    LinkAttachments(), MonitorAttachment(), ReleaseAttachment(),
 |||	    StartAttachment(), StopAttachment(), WhereAttachment()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_AUTO_DELETE_SLAVE (bool) - Create
 |||	        Set to TRUE to cause the slave Item to be automatically deleted when the
 |||	        Attachment Item is deleted. Otherwise the slave Item is left intact
 |||	        when the Attachment Item is deleted. Defaults to FALSE.
 |||
 |||	        There is no harm in setting this flag for multiple Attachments to the
 |||	        same slave, nor is there any harm for manually deleting the slave of
 |||	        such an Attachment.
 |||
 |||	    AF_TAG_CLEAR_FLAGS (uint32) - Modify
 |||	        Set of AF_ATTF_ flags to clear. Clears every flag for which a 1 is set
 |||	        in ta_Arg.
 |||
 |||	    AF_TAG_MASTER (Item) - Create, Query
 |||	        Instrument or Template item to attach envelope or sample to. Must be
 |||	        specified when creating an Attachment.
 |||
 |||	    AF_TAG_NAME (const char *) - Create, Query
 |||	        The name of the sample or envelope hook in the instrument to attach to.
 |||	        NULL, the default, means to use the default hook, which depends on the
 |||	        type of slave Item.
 |||
 |||	        For envelopes, the default envelope hook is "Env".
 |||
 |||	        For samples, the default may only be used for master items with one FIFO
 |||	        (either input or output). In this case, NULL selects the one and only
 |||	        FIFO. If the master item instrument has more than one FIFO, you must
 |||	        specify the name of the FIFO to which to attach.
 |||
 |||	    AF_TAG_SLAVE (Item) - Create, Query
 |||	        Envelope or Sample Item to attach to Instrument.
 |||
 |||	    AF_TAG_SET_FLAGS (uint32) - Create, Query, Modify
 |||	        Set of AF_ATTF_ flags to set. Sets every flag for which a 1 is set in
 |||	        ta_Arg.
 |||
 |||	    AF_TAG_START_AT (int32) - Create, Query, Modify
 |||	        Specifies the point at which to start when the attachment is started
 |||	        (with StartAttachment() or StartInstrument()).
 |||
 |||	        For sample attachments, specifies a sample frame number in the sample at
 |||	        which to begin playback.
 |||
 |||	        For envelopes attachments, specifies the segment index at which to start.
 |||
 |||	  Flags
 |||
 |||	    AF_ATTF_FATLADYSINGS
 |||	        If set, causes the instrument to stop when the attachment finishes
 |||	        playing. This flag can be used to mark the one or more Envelope(s) or
 |||	        Sample(s) that are considered to be the determiners of when the
 |||	        instrument is done playing and should be stopped.
 |||
 |||	        For envelopes, the default setting for this flag comes from the
 |||	        AF_ENVF_FATLADYSINGS flag. Defaults to cleared for samples.
 |||
 |||	    AF_ATTF_NOAUTOSTART
 |||	        When set, causes StartInstrument() to not automatically start this
 |||	        attachment. This allows later starting of the attachment by using
 |||	        StartAttachment(). This is useful for sound spooling applications.
 |||	        Defaults to cleared (attachment defaults to starting when instrument is
 |||	        started).
 |||
 |||	  Caveats
 |||
 |||	    When creating an envelope attachment to an envelope with
 |||	    AF_ENVF_FATLADYSINGS set, AF_ATTF_FATLADYSINGS is automatically ORed into
 |||	    the set of attachment flags passed in with AF_TAG_SET_FLAGS. A subsequent
 |||	    call to SetAudioItemInfo() is required to clear this flag.
 |||
 |||	  See Also
 |||
 |||	    Envelope(@), Instrument(@), Sample(@), Template(@)
 **/

/*****************************************************************/
/***** Create Attachment Item for Folio **************************/
/*****************************************************************/
/* AUDIO_ATTACHMENT_NODE ir_Create method */
Item internalCreateAudioAttachment (AudioAttachment *aatt, const TagArg *tagList)
{
	const char *hookName = NULL;        /* !!! will become part of tagdata for attachments */
	ItemNode *masterNode;
	ItemNode *slaveNode;
	AudioInsTemplate *aitp;
	Err errcode;

	/* !!! this function could use a bit more work */

		/* process tags */
		/* !!! use custom callback rather than afi_DummyProcessor + NextTagArg() loop,
		** and share code with internalSetAttachmentInfo(). */
		/* !!! propagate AF_ENVF_FATLADYSINGS to attachment flag before processing tags
		**     to permit clearing it with AF_TAG_CLEAR_FLAGS during attachment creation
		**     (might need to scan tag list more than once or keep separate set/clear flags variables) */
	errcode = TagProcessor (aatt, tagList, afi_DummyProcessor, NULL);
	if(errcode < 0)
	{
		ERR(("internalCreateAudioAttachment: TagProcessor failed.\n"));
		goto clean;
	}

	{
		const TagArg *tstate, *t;

		for (tstate = tagList; (t = NextTagArg (&tstate)) != NULL;) {
			DBUGITEM(("internalCreateAudioAttachment: tag { %d, 0x%x }\n", t->ta_Tag, t->ta_Arg));

			switch (t->ta_Tag) {
				case AF_TAG_MASTER:
					if( aatt->aatt_HostItem ) {
						errcode = AF_ERR_TAGCONFLICT;
						goto clean;
					}
					aatt->aatt_HostItem = (Item) t->ta_Arg;
					break;

				case AF_TAG_SLAVE:
					if( aatt->aatt_SlaveItem ) {
						errcode = AF_ERR_TAGCONFLICT;
						goto clean;
					}
					aatt->aatt_SlaveItem = (Item) t->ta_Arg;
					break;

				case AF_TAG_NAME:
					hookName = (char *)t->ta_Arg;
					break;

				case AF_TAG_SET_FLAGS:
					{
						const uint32 setFlags =  (uint32)t->ta_Arg;

						if (setFlags & ~AF_ATTF_LEGALFLAGS) {
							ERR(("internalCreateAudioAttachment: Illegal attachment flags (0x%x)\n", setFlags));
							errcode = AF_ERR_BADTAGVAL;
							goto clean;
						}
						aatt->aatt_Flags |= setFlags;
					}
					break;

				case AF_TAG_START_AT:
					aatt->aatt_StartAt = (int32) t->ta_Arg;
					break;

				case AF_TAG_AUTO_DELETE_SLAVE:
					if ((int32)t->ta_Arg) aatt->aatt_Item.n_Flags |= AF_NODE_F_AUTO_DELETE_SLAVE;
					else                  aatt->aatt_Item.n_Flags &= ~AF_NODE_F_AUTO_DELETE_SLAVE;
					break;

				default:
					if(t->ta_Tag > TAG_ITEM_LAST) {
						ERR(("internalCreateAudioAttachment: Unrecognized tag { %d, 0x%x }\n", t->ta_Tag, t->ta_Arg));
						errcode = AF_ERR_BADTAG;
						goto clean;
					}
					break;
			}
		}
	}

		/* Clone hook name if given */
		/* !!! ideally point to resource or port name in template instead of cloning user-supplied name */
	if (hookName) {
			/* validate pointer */
		if ((errcode = afi_IsRamAddr (hookName, 16)) < 0) goto clean;   /* !!! wrong length */

			/* allocate copy */
		if (!(aatt->aatt_HookName = SuperAllocMem (strlen(hookName)+1, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
			errcode = AF_ERR_NOMEM;
			goto clean;
		}
		strcpy (aatt->aatt_HookName, hookName);
	}

		/* Look up master, validate its type */
	masterNode = LookupItem (aatt->aatt_HostItem);
	if ( !( masterNode &&
			masterNode->n_SubsysType == AUDIONODE &&
			(masterNode->n_Type == AUDIO_TEMPLATE_NODE || masterNode->n_Type == AUDIO_INSTRUMENT_NODE) ) )
	{
		ERR(("internalCreateAudioAttachment: Master Item 0x%x is not an Instrument or Template\n", aatt->aatt_HostItem));
		errcode = AF_ERR_BADITEM;
		goto clean;
	}
		/* Get template associated with master */
	aitp = masterNode->n_Type == AUDIO_INSTRUMENT_NODE
		? ((AudioInstrument *)masterNode)->ains_Template
		: (AudioInsTemplate *)masterNode;

		/* Look up slave, validate its type */
	slaveNode = LookupItem (aatt->aatt_SlaveItem);
	if ( !( slaveNode &&
			slaveNode->n_SubsysType == AUDIONODE &&
			(slaveNode->n_Type == AUDIO_SAMPLE_NODE || slaveNode->n_Type == AUDIO_ENVELOPE_NODE) ) )
	{
		ERR(("internalCreateAudioAttachment: Slave Item 0x%x is not a Sample or Envelope\n", aatt->aatt_SlaveItem));
		errcode = AF_ERR_BADITEM;
		goto clean;
	}
	aatt->aatt_Structure = slaveNode;
	aatt->aatt_SlaveRef.arnd_RefItem = aatt->aatt_Item.n_Item;

	/* @@@ From here on, slave and master are both expected to be valid types */

		/* Slave type-specific processing */
		/* !!! Some of this could belong in ValidateAudioAttachment()
		**     (common validation between create and modify) */
		/* !!! Consider making template-only traps available only when BUILD_PARANOIA is defined */
	switch (slaveNode->n_Type) {
		case AUDIO_SAMPLE_NODE:
			{
				AudioSample * const asmp = (AudioSample *)slaveNode;

					/* Set attachment type */
				aatt->aatt_Type = AF_ATT_TYPE_SAMPLE;

					/* Validate "start at" against sample */
					/* @@@ checking for non-zero asmp->asmp_NumFrames permits creating
					**     attachments for empty samples */
				if ( aatt->aatt_StartAt < 0 ||
					 aatt->aatt_StartAt >= asmp->asmp_NumFrames && asmp->asmp_NumFrames != 0 )
				{
					ERR(("internalCreateAudioAttachment: AF_TAG_START_AT value (%d) out of range\n", aatt->aatt_StartAt));
					errcode = AF_ERR_BADTAGVAL;
					goto clean;
				}

					/* Master type-specific processing for Samples */
					/* !!! could share more code between instrument and template by moving
					**     some of DSPPAttachSample() to this function */
				switch (masterNode->n_Type) {
					case AUDIO_TEMPLATE_NODE:
							/* Validate sample hook */
						{
							const DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;
							int32 fifoRsrcIndex;

								/* look up resource */
							fifoRsrcIndex = aatt->aatt_HookName
								? dsppFindResourceIndex (dtmp, aatt->aatt_HookName)
								: dsppFindDefaultFIFORsrcIndex (dtmp);

							if ((errcode = fifoRsrcIndex) < 0) {
							  #ifdef BUILD_STRINGS
								if (aatt->aatt_HookName) {
									ERR(("internalCreateAudioAttachment: Sample hook '%s' not found in master Item 0x%x\n", aatt->aatt_HookName, aatt->aatt_HostItem));
								}
								else {
									ERR(("internalCreateAudioAttachment: Master Item 0x%x has no default sample hook\n", aatt->aatt_HostItem));
								}
							  #endif
								goto clean;
							}

								/* check resource type */
							switch (dtmp->dtmp_Resources[fifoRsrcIndex].drsc_Type) {
								case DRSC_TYPE_IN_FIFO:
									/* nothing special */
									break;

								case DRSC_TYPE_OUT_FIFO:
										/* Trap attaching an ordinary sample to an output FIFO */
									if (!IsDelayLine(asmp)) {
										ERR(("internalCreateAudioAttachment: Cannot attach an ordinary sample (0x%x) to an output FIFO ('%s')\n", aatt->aatt_SlaveItem, dsppGetTemplateRsrcName(dtmp, fifoRsrcIndex)));
										errcode = AF_ERR_SECURITY;
										goto clean;
									}
									break;

								default:
									ERR(("internalCreateAudioAttachment: Master Item 0x%x port '%s' is not a sample hook\n", aatt->aatt_HostItem, aatt->aatt_HookName));
									errcode = AF_ERR_BAD_PORT_TYPE;
									goto clean;
							}
						}

							/* Add attachment to template's attachment list */
							/* !!! commonize with envelope? */
						AddTail (&aitp->aitp_Attachments, (Node *)aatt);
						break;

					case AUDIO_INSTRUMENT_NODE:
						{
							AudioInstrument * const ains = (AudioInstrument *)masterNode;

								/* Trap attaching a delay line template to instrument */
							if (IsDelayLineTemplate(asmp)) {
								ERR(("internalCreateAudioAttachment: Cannot attach a delay line template (0x%x) to an instrument (0x%x)\n", aatt->aatt_SlaveItem, aatt->aatt_HostItem));
								errcode = AF_ERR_BADITEM;
								goto clean;
							}

								/* Bind attachment to FIFO */
							if ((errcode =  DSPPAttachSample ((DSPPInstrument *)ains->ains_DeviceInstrument, asmp, aatt)) < 0) goto clean;
						}
						break;
				}

				/* @@@ Must not fail from here on, or else StripAudioAttachment() must
				**     remove aatt from the list it has been added to. */

					/* Link attachment reference node to Sample */
				AddTail (&asmp->asmp_AttachmentRefs, (Node *)&aatt->aatt_SlaveRef);
			}
			break;

		case AUDIO_ENVELOPE_NODE:
			{
				AudioEnvelope * const aenv = (AudioEnvelope *)slaveNode;
				DSPPEnvHookRsrcInfo deri;

					/* set attachment type */
				aatt->aatt_Type = AF_ATT_TYPE_ENVELOPE;

					/* copy FATLADYSINGS bit from envelope. */
				if (aenv->aenv_Flags & AF_ENVF_FATLADYSINGS) {
					aatt->aatt_Flags |= AF_ATTF_FATLADYSINGS;
				}

					/* Find envelope resources */
				{
						/* Substitute default envelope hook name for NULL */
					const char * const envHookName = aatt->aatt_HookName ? aatt->aatt_HookName : AF_DEFAULT_ENV_HOOK;

					if ((errcode = dsppFindEnvHookResources (&deri, sizeof deri, (DSPPTemplate *)aitp->aitp_DeviceTemplate, envHookName)) < 0) {
						ERR(("internalCreateAudioAttachment: Envelope hook '%s' not found in master Item 0x%x\n", envHookName, aatt->aatt_HostItem));
						goto clean;
					}
				}

					/* Validate "start at" against envelope */
					/* @@@ assumes that aenv_NumPoints is always > 0 */
				if (aatt->aatt_StartAt < 0 || aatt->aatt_StartAt >= aenv->aenv_NumPoints) {
					ERR(("internalCreateAudioAttachment: AF_TAG_START_AT value (%d) out of range\n", aatt->aatt_StartAt));
					errcode = AF_ERR_BADTAGVAL;
					goto clean;
				}

					/* Master type-specific processing for Envelopes */
				switch (masterNode->n_Type) {
					case AUDIO_TEMPLATE_NODE:
							/* Add attachment to template's attachment list */
							/* !!! commonize with sample? */
						AddTail (&aitp->aitp_Attachments, (Node *)aatt);
						break;

					case AUDIO_INSTRUMENT_NODE:
						{
							AudioInstrument * const ains = (AudioInstrument *)masterNode;
							DSPPInstrument * const dins = (DSPPInstrument *)ains->ains_DeviceInstrument;

								/* Add extension for envelope management */
							{
								AudioEnvExtension *aeva;

									/* alloc/init aeva */
								if (!(aatt->aatt_Extension = aeva = (AudioEnvExtension *)SuperAllocMem (sizeof (AudioEnvExtension), MEMTYPE_NORMAL | MEMTYPE_FILL))) {
									errcode = AF_ERR_NOMEM;
									goto clean;
								}
								aeva->aeva_Parent = aatt;

									/* set clock for envelope to use */
								aeva->aeva_Event.aevt_Clock = AB_FIELD(af_EnvelopeClock);

									/* get allocations for each of the envelope's resources */
								aeva->aeva_RequestDSPI = dins->dins_Resources [ deri.deri_RequestRsrcIndex ].drsc_Allocated;
								aeva->aeva_IncrDSPI    = dins->dins_Resources [ deri.deri_IncrRsrcIndex    ].drsc_Allocated;
								aeva->aeva_TargetDSPI  = dins->dins_Resources [ deri.deri_TargetRsrcIndex  ].drsc_Allocated;
								aeva->aeva_CurrentDSPI = dins->dins_Resources [ deri.deri_CurrentRsrcIndex ].drsc_Allocated;
							}

								/* Bind attachment to envelope attachment list (stored in DSPPInstrument) */
							AddTail (&dins->dins_EnvelopeAttachments, (Node *)aatt);
						}
						break;
				}

				/* @@@ Must not fail from here on, or else StripAudioAttachment() must
				**     remove aatt from the list it has been added to. */

					/* Link attachment reference node to Envelope */
				AddTail (&aenv->aenv_AttachmentRefs, (Node *)&aatt->aatt_SlaveRef);
			}
			break;
	}

#if DEBUG_Item
	internalDumpAttachment (aatt, "internalCreateAudioAttachment");
#endif

		/* success: return Item number */
	return aatt->aatt_Item.n_Item;

clean:
		/* failure: clean up from partial success */
	StripAudioAttachment (aatt);
	return errcode;
}

/*****************************************************************/
/* AUDIO_ATTACHMENT_NODE GetAudioItemInfo() method */
Err internalGetAttachmentInfo (const AudioAttachment *aatt, TagArg *tagList)
{
	TagArg *tag;
	int32 tagResult;

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUGITEM(("internalGetAttachmentInfo: %d\n", tag->ta_Tag));

		switch (tag->ta_Tag) {
			case AF_TAG_MASTER:
				tag->ta_Arg = (TagData)aatt->aatt_HostItem;
				break;

			case AF_TAG_NAME:
				tag->ta_Arg = (TagData)aatt->aatt_HookName;
				break;

			case AF_TAG_SLAVE:
				tag->ta_Arg = (TagData)aatt->aatt_SlaveItem;
				break;

			case AF_TAG_SET_FLAGS:
					/* filter off AF_ATTF_LEGALFLAGS in case there are internal flags stored here too */
				tag->ta_Arg = (TagData)(aatt->aatt_Flags & AF_ATTF_LEGALFLAGS);
				break;

			case AF_TAG_START_AT:
				tag->ta_Arg = (TagData)aatt->aatt_StartAt;
				break;

		  #if 0     /* @@@ not supported yet */
			case AF_TAG_STATUS:
				tag->ta_Arg = (TagData)aatt->aatt_ActivityLevel;
				break;
		  #endif

			default:
				ERR (("internalGetAttachmentInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if (tagResult < 0) {
		ERR(("internalGetAttachmentInfo: Error processing tag list 0x%x\n", tagList));
		return tagResult;
	}

	return 0;
}

/*****************************************************************/
/* AUDIO_ATTACHMENT_NODE SetAudioItemInfo() method */
Err internalSetAttachmentInfo (AudioAttachment *aatt, const TagArg *tagList)
{
	const TagArg *tag;
	int32 tagResult;

	for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
		DBUGITEM(("internalSetAttachmentInfo: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));

		switch (tag->ta_Tag) {
			case AF_TAG_START_AT:
				{
					const int32 StartAt = (int32)tag->ta_Arg;
					uint32 NumFrames = 0;

					switch(aatt->aatt_Type) {
						case AF_ATT_TYPE_SAMPLE:
							{
								const AudioSample * const asmp = (AudioSample *) aatt->aatt_Structure;

								NumFrames = asmp->asmp_NumFrames;
							}
							break;

						case AF_ATT_TYPE_ENVELOPE:
							{
								const AudioEnvelope * const aenv = (AudioEnvelope *) aatt->aatt_Structure;

								NumFrames = aenv->aenv_NumPoints;
							}
							break;
					}
					if((StartAt < 0) || (StartAt >= NumFrames))
					{
						ERR(("internalSetAttachmentInfo: Attachment START_AT out of range (%d)\n", StartAt));
						return AF_ERR_BADTAGVAL;
					}
					aatt->aatt_StartAt = StartAt;
				}
				break;

			case AF_TAG_SET_FLAGS:
				{
					const uint32 setFlags = (uint32)tag->ta_Arg;

					if (setFlags & ~AF_ATTF_LEGALFLAGS) {
						ERR(("internalSetAttachmentInfo: Illegal attachment flags (0x%x)\n", setFlags));
						return AF_ERR_BADTAGVAL;
					}
					aatt->aatt_Flags |= setFlags;
				}
				break;

			case AF_TAG_CLEAR_FLAGS:
				{
					const uint32 clearFlags = (uint32)tag->ta_Arg;

					if (clearFlags & ~AF_ATTF_LEGALFLAGS) {
						ERR(("internalSetAttachmentInfo: Illegal attachment flags (0x%x)\n", clearFlags));
						return AF_ERR_BADTAGVAL;
					}
					aatt->aatt_Flags &= ~clearFlags;
				}
				break;

			default:
				ERR(("internalSetAttachmentInfo: Unrecognized tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
				return AF_ERR_BADTAG;
		}
	}

		/* Catch tag processing errors */
	if (tagResult < 0) {
		ERR(("internalSetAttachmentInfo: Error processing tag list 0x%x\n", tagList));
		return tagResult;
	}

#if DEBUG_Item
	internalDumpAttachment (aatt, "internalSetAttachmentInfo");
#endif

	return 0;
}

/**************************************************************/
/* AUDIO_ATTACHMENT_NODE ir_Delete method */
int32 internalDeleteAudioAttachment (AudioAttachment *aatt, Task *ct)
{
	/*
		If Attachment deleted first:
			if (Instrument) StopInstrument
			Remove AudioReferenceNode
		If Master deleted first:
			Remove AudioReferenceNode
		If Slave deleted first:
			if (Instrument) StopInstrument
	*/

	TOUCH(ct);

	DBUGITEM(("internalDeleteAudioAttachment: att=0x%x, master=0x%x, slave=0x%x\n", aatt->aatt_Item.n_Item, aatt->aatt_HostItem, aatt->aatt_SlaveItem));

	TRACEE(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT, ("internalDeleteAudioAttachment (aatt=0x%x)\n", aatt));
	TRACKMEM(("internalDeleteAudioAttachment: aatt = 0x%x\n",aatt));
	TRACEB(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT, ("internalDeleteAudioAttachment: Host=0x%x, Slave=0x%x\n", aatt->aatt_HostItem, aatt->aatt_SlaveItem));

		/* process master */
	{
		const Item Host = aatt->aatt_HostItem;
		ItemNode * const masterNode = LookupItem (Host);

		if (masterNode && masterNode->n_SubsysType == AUDIONODE &&
			(masterNode->n_Type == AUDIO_INSTRUMENT_NODE || masterNode->n_Type == AUDIO_TEMPLATE_NODE)) {

				/* if master is an instrument, stop it */
			if (masterNode->n_Type == AUDIO_INSTRUMENT_NODE) {
				AudioInstrument * const ains = (AudioInstrument *)masterNode;

				if (aatt->aatt_ActivityLevel > AF_STOPPED)
				{
					swiStopInstrument( Host, NULL );
					    /* 960628: make sure it's a sample attachment before caling DSPPStopSampleAttachment() */
					    /* !!! this second stop seems redundant */
					if (aatt->aatt_Type == AF_ATT_TYPE_SAMPLE) DSPPStopSampleAttachment( aatt );
				}
					/* Clear reference in ains. */
				if (ains->ains_StartingAAtt == aatt) ains->ains_StartingAAtt = 0;

				TRACKMEM(("internalDeleteAudioAttachment: after StopAttachment\n"));
			}

				/* remove master's reference */
			RemoveAndMarkNode ((Node *)aatt);
			TRACKMEM(("internalDeleteAudioAttachment: after RemNode\n"));
		}
		else {
			ERR(("internalDeleteAudioAttachment: host item 0x%x invalid\n", Host));
		}
	}

		/* process slave */
	{
		const Item slave = aatt->aatt_SlaveItem;
		ItemNode * const slaveNode = LookupItem (slave);

			/* If slave is already gone, don't worry about it. */
			/* !!! add paranoia check for AUDIO_SAMPLE_NODE and AUDIO_ENVELOPE_NODE? */
		if (slaveNode && slaveNode->n_SubsysType == AUDIONODE) {

				/* remove reference node */
			RemoveAndMarkNode ((Node *)&aatt->aatt_SlaveRef);

				/* Auto-delete slave Item if requested and owners match (@@@ must be done after removing references to avoid recursion) */
			if (aatt->aatt_Item.n_Flags & AF_NODE_F_AUTO_DELETE_SLAVE && aatt->aatt_Item.n_Owner == slaveNode->n_Owner) {
			#if DEBUG_Item
				DBUGITEM(("internalDeleteAudioAttachment: Auto-deleting slave item 0x%x (type %d", slave, slaveNode->n_Type));
				if (slaveNode->n_Name) DBUGITEM((", name '%s'", slaveNode->n_Name));
				DBUGITEM((") @ 0x%x\n", slaveNode));
			#endif
				SuperInternalDeleteItem (slave);
			}
		}
	}

	TRACKMEM(("internalDeleteAudioAttachment: after afi_RemoveReferenceNode\n"));

	StripAudioAttachment (aatt);

	TRACKMEM(("internalDeleteAudioAttachment: after StripAudioAttachment\n"));
	TRACER(TRACE_INT,TRACE_ITEM|TRACE_ATTACHMENT, ("internalDeleteAudioAttachment returns 0x%x\n", Result));

	return 0;
}

/*
	Common cleanup code for partial success of internalCreateAudioAttachment()
	and internalDeleteAudioAttachment(). Simply frees extra stuff allocated
	during item creation: hook name, env extension. Does not decouple aatt_Item
	or aatt_SlaveRef from any lists that they may be in (caller is expected to
	deal with that).

	Arguments
		aatt
*/
static void StripAudioAttachment (AudioAttachment *aatt)
{
	DBUGITEM(("StripAudioAttachment: att=0x%x extension=0x%x hookname=0x%x\n", aatt->aatt_Item.n_Item, aatt->aatt_Extension, aatt->aatt_HookName));

		/* free envelope extension */
	SuperFreeMem (aatt->aatt_Extension, sizeof (AudioEnvExtension));
	aatt->aatt_Extension = NULL;

		/* free hook name */
	SuperFreeMem (aatt->aatt_HookName, TRACKED_SIZE);
	aatt->aatt_HookName = NULL;
}


/**************************************************************/
/***** SWIs ***************************************************/
/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name StartAttachment
 |||	Starts an Attachment(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err StartAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err StartAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure starts playback of an attachment, which may be an attached
 |||	    envelope or sample. This function is useful to start attachments that
 |||	    aren't started by StartInstrument() (e.g. attachments with the
 |||	    AF_ATTF_NOAUTOSTART flag set).
 |||
 |||	    An attachment started with StartAttachment() should be released with
 |||	    ReleaseAttachment() and stopped with StopAttachment() if necessary.
 |||
 |||	    Template attachments are not supported by this call.
 |||
 |||	  Arguments
 |||
 |||	    Attachment
 |||	        The item number for the attachment.
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
 |||	  Caveats
 |||
 |||	    Prior to V24, StartAttachment() did not support envelope attachments.
 |||
 |||	  See Also
 |||
 |||	    ReleaseAttachment(), StopAttachment(), LinkAttachments(),
 |||	    CreateAttachment(), StartInstrument()
 **/
int32 swiStartAttachment( Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;

	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;

/* Make sure we only do this with Instrument attachments, not templates. */
	{
		AudioInstrument *ains;
		ains = (AudioInstrument *)CheckItem(aatt->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
		if( ains == NULL ) return AF_ERR_BADITEM;
	}

/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPStartSampleAttachment( aatt, TRUE );
			break;

		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPStartEnvAttachment( aatt );
			break;
	}
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name ReleaseAttachment
 |||	Releases an Attachment(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err ReleaseAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure releases an attachment and is commonly used to release
 |||	    attachments started with StartAttachment(). ReleaseAttachment() causes an
 |||	    attachment in a sustain loop to enter release phase. Has no effect on
 |||	    attachment with no sustain loop, or not in its sustain loop.
 |||
 |||	    Template attachments are not supported by this call.
 |||
 |||	  Arguments
 |||
 |||	    Attachment
 |||	        The item number for the attachment.
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
 |||	  Caveats
 |||
 |||	    Prior to V24, ReleaseAttachment() did not support envelope attachments.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), StopAttachment(), LinkAttachments(), ReleaseInstrument()
 **/
int32 swiReleaseAttachment( Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;

	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;

/* Make sure we only do this with Instrument attachments, not templates. */
	{
		AudioInstrument *ains;
		ains = (AudioInstrument *)CheckItem(aatt->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
		if( ains == NULL ) return AF_ERR_BADITEM;
	}

/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPReleaseSampleAttachment( aatt );
			break;

		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPReleaseEnvAttachment( aatt );
			break;
	}
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name StopAttachment
 |||	Stops an Attachment(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err StopAttachment (Item Attachment, TagArg *tagList)
 |||
 |||	    Err StopAttachmentVA (Item Attachment, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure abruptly stops an attachment. The attachment doesn't
 |||	    go into its release phase when stopped this way.
 |||
 |||	    Template attachments are not supported by this call.
 |||
 |||	  Arguments
 |||
 |||	    Attachment
 |||	        The item number for the attachment.
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
 |||	  Caveats
 |||
 |||	    Prior to V24, StopAttachment() did not support envelope attachments.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), ReleaseAttachment(), LinkAttachments(), StopInstrument()
 **/
int32 swiStopAttachment(  Item Attachment, TagArg *tp )
{
	AudioAttachment *aatt;
	int32 Result = 0;

	aatt = (AudioAttachment *)CheckItem(Attachment, AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;
	if (tp) return AF_ERR_BADTAG;

/* Make sure we only do this with Instrument attachments, not templates. */
	{
		AudioInstrument *ains;
		ains = (AudioInstrument *)CheckItem(aatt->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
		if( ains == NULL ) return AF_ERR_BADITEM;
	}

/* Distinguish between Sample and Envelope Attachments. 940825 */
	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			Result = DSPPStopSampleAttachment( aatt );
			break;

		case AF_ATT_TYPE_ENVELOPE:
			Result = DSPPStopEnvAttachment( aatt );
			break;
	}
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name LinkAttachments
 |||	Connects Sample Attachment(@)s for sequential playback.
 |||
 |||	  Synopsis
 |||
 |||	    Err LinkAttachments (Item Att1, Item Att2)
 |||
 |||	  Description
 |||
 |||	    This procedure specifies that the attachment Att2 will begin playing when
 |||	    attachment Att1 finishes. This is useful if you want to connect
 |||	    discontiguous sample buffers that are used for playing interleaved audio
 |||	    and video data. It is also a good way to construct big sound effects from
 |||	    a series of small sound effects.
 |||
 |||	    If Att1's sample has a sustain loop, but no release loop, you can follow
 |||	    this with a call to ReleaseAttachment(Att1,NULL) to smoothly transition
 |||	    to Att2.
 |||
 |||	    If Att1's sample has no loops, Att2 will automatically start as soon as
 |||	    Att1 completes (assuming that it has not completed prior to this function
 |||	    being called).
 |||
 |||	    If, after linking Att1 to Att2, StopAttachment() is called on Att1 before
 |||	    Att1 finishes, Att2 will not be automatically started. StopAttachment() on
 |||	    Att1 after Att1 finishes has no effect.
 |||
 |||	    All links remain in effect for multiple calls to StartInstrument() or
 |||	    StartAttachment(). That is, if you call LinkAttachments(Att1,Att2),
 |||	    Att1 will flow into Att2 upon completion of Att1 for every subsequent
 |||	    call to StartAttachment(Att1,NULL) (or StartInstrument() on the instrument
 |||	    belonging to Att1) if Att1 would normally be automatically started by
 |||	    starting the instrument).
 |||
 |||	    An attachment (Att1) can link to no more than one attachment. An attachment
 |||	    (Att2) can be linked to multiple attachments. The most recent call to
 |||	    LinkAttachments() for Att1 takes precedence.
 |||
 |||	    The pair of Attachments passed to this function must satisfy all of these
 |||	    requirements:
 |||
 |||	    * Both Attachments must be Sample Attachments.
 |||
 |||	    * Both Attachments must be attached to the same Instrument.
 |||
 |||	    * Both Attachments must be attached to the same FIFO of that Instrument.
 |||
 |||	    * The Attachments cannot be attached to an Instrument TEMPLATE.
 |||
 |||	    Call LinkAttachments(Att1,0) to remove a previous link from Att1.
 |||
 |||	    A link does not interfere with a Cue associated with an attachment.
 |||
 |||	    Deleting either Attachment, either Attachment's Sample, or the Instrument
 |||	    to which the Attachment belongs, breaks the link.
 |||
 |||	  Arguments
 |||
 |||	    Att1
 |||	        The item number for the attachment that is to finish. Must be a Sample
 |||	        Attachment.
 |||
 |||	    Att2
 |||	        The item number for the attachment that is to begin playing. Must be a
 |||	        Sample Attachment attached to the same FIFO on the same Instrument as
 |||	        Att1. Can be 0 to remove a link from Att1.
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
 |||	    In order for Att2 to start, Att1 must either be currently playing or
 |||	    yet to be played. If it has already completed by the time of this call,
 |||	    Att2 will not be started.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    StartAttachment(), ReleaseAttachment()
 **/

int32 swiLinkAttachments( Item Att1, Item Att2 )
{
	AudioAttachment *aatt1;
	AudioInstrument *ains1;
	int32 Result = 0;

/* Are they both valid Attachments? */
DBUG(("LinkAttachment: Att1 = 0x%x, Att2 = 0x%x\n", Att1, Att2 ));
	aatt1 = (AudioAttachment *)CheckItem(Att1,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if ((aatt1 == NULL) || (aatt1->aatt_Type != AF_ATT_TYPE_SAMPLE)) return AF_ERR_BADITEM;

	ains1 = (AudioInstrument *)CheckItem(aatt1->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if( ains1 == NULL ) return AF_ERR_MISMATCH;

/* Are we forming a new link? */
	if( Att2 )
	{
		AudioAttachment *aatt2;
		AudioInstrument *ains2;

		aatt2 = (AudioAttachment *)CheckItem(Att2,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
		if ((aatt2 == NULL) || (aatt2->aatt_Type != AF_ATT_TYPE_SAMPLE)) return AF_ERR_BADITEM;

/* Are the hosts both the same instrument? Templates not allowed. */
		ains2 = (AudioInstrument *)CheckItem(aatt2->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
		if( ains1 != ains2 ) return AF_ERR_MISMATCH;

/* Are they attached to different channels? */
		if( aatt1->aatt_Channel != aatt2->aatt_Channel ) return AF_ERR_MISMATCH;
	}

/* Link them. */
	aatt1->aatt_NextAttachment = Att2;

/* Update Sample DMA and Interrupts. */
	switch( aatt1->aatt_ActivityLevel )
	{
		case AF_STARTED:
			Result = DSPPStartSampleAttachment( aatt1,  FALSE );
			break;

		case AF_RELEASED:
			Result = DSPPReleaseSampleAttachment( aatt1 );
			break;
	}

	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name WhereAttachment
 |||	Returns the current playing location of an Attachment(@).
 |||
 |||	  Synopsis
 |||
 |||	    int32 WhereAttachment (Item Attachment)
 |||
 |||	  Description
 |||
 |||	    This procedure is useful for monitoring the progress of a sample or
 |||	    envelope that's being played. It returns a value indicating where playback
 |||	    is located in the attachment's sample or envelope. For sample attachments,
 |||	    returns the currently playing byte offset within the sample. For
 |||	    envelope attachments, returns the currently playing segment index.
 |||
 |||	    A sample's offset starts at zero. Note that the offset is not measured in
 |||	    sample frames. You must divide the byte offset by the number of bytes per
 |||	    frame, then discard the remainder to find out which frame is being played.
 |||
 |||	  Arguments
 |||
 |||	    Attachment
 |||	        Item number of the attachment.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value indicating position (byte offset of sample or
 |||	    segment index of envelope) on success, negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    If a sample attachment has finished, the return value may be negative, or
 |||	    greater than the length of the sample. This is because the DMA hardware is
 |||	    pointing to a different sample. To determine whether an Attachment has
 |||	    finished, use MonitorAttachment().
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    MonitorAttachment()
 **/
int32 swiWhereAttachment( Item Attachment )
{
	AudioAttachment	*aatt;
	AudioSample     *asmp;
/*	AudioEnvelope	*aenv; */
	int8            *SampleBase;
	int32            currentAddress;
	AudioEnvExtension *aeva;
	int32            Where;

DBUG(("swiWhereAttachment( Att = 0x%x)\n", Attachment ));
	aatt = (AudioAttachment *)CheckItem(Attachment,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;

/* Make sure we only do this with Instrument attachments, not templates. */
	{
		AudioInstrument *ains;
		ains = (AudioInstrument *)CheckItem(aatt->aatt_HostItem,  AUDIONODE, AUDIO_INSTRUMENT_NODE);
		if( ains == NULL ) return AF_ERR_BADITEM;
	}

	switch(aatt->aatt_Type)
	{
		case AF_ATT_TYPE_SAMPLE:
			asmp = (AudioSample *) aatt->aatt_Structure;
			SampleBase = (int8 *) asmp->asmp_Data;
/* Get value in DMA register. */
/* !!! - move DSPX_DMA_ADDRESS_MASK to dspp_addresses.h */
#define DSPX_DMA_ADDRESS_NUMBITS (25)
#define DSPX_DMA_ADDRESS_MASK   ((1<<DSPX_DMA_ADDRESS_NUMBITS)-1)
			currentAddress = (int32) dsphReadChannelAddress(aatt->aatt_Channel);
/* DMA Register only has 25 bits so mask Samplebase as well. */
			Where = currentAddress - (((int32)SampleBase) & DSPX_DMA_ADDRESS_MASK);
/* If sample is done and Where is negative, set to -1 for consistency. !!! - review API */
			if( Where < 0 ) Where = -1;
			break;

/* 931117 Support for envelopes added. */
		case AF_ATT_TYPE_ENVELOPE:
		 /* aenv = (AudioEnvelope *) aatt->aatt_Structure; @@@ result not used */
			aeva = (AudioEnvExtension *) aatt->aatt_Extension;
			Where = aeva->aeva_CurIndex;
			break;

		default:
			Where = AF_ERR_BADITEM;
			break;
	}

DBUG(("swiWhereAttachment: Where = 0x%x\n", Where));
	return Where;
}

/**************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Attachment -name MonitorAttachment
 |||	Monitors an Attachment(@), sends a Cue(@) at a specified point.
 |||
 |||	  Synopsis
 |||
 |||	    Err MonitorAttachment (Item Attachment, Item Cue, int32 Index)
 |||
 |||	  Description
 |||
 |||	    This procedure sends a Cue to the calling task when the specified
 |||	    Attachment reaches the specified point. The procedure is often used
 |||	    with a sample Attachment to send a cue when the sample has been fully
 |||	    played.
 |||
 |||	    There can be only one Cue per Attachment. The most recent call to
 |||	    MonitorAttachment() takes precedence.
 |||
 |||	    To remove the current Cue from an Attachment, call
 |||	    MonitorAttachment(Attachment,0,0).
 |||
 |||	  Arguments
 |||
 |||	    Attachment
 |||	        The item number for the Attachment.
 |||
 |||	    Cue
 |||	        Item number for the Cue to be associated with this attachment. Can be 0
 |||	        to remove a the current Cue from the Attachment.
 |||
 |||	    Index
 |||	        Value indicating the point to be monitored. At this time, the only value
 |||	        that can be passed is CUE_AT_END, which asks that a Cue be sent at the
 |||	        end of an Attachment.
 |||
 |||	        Index is ignored if Cue is 0.
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
 |||	    GetCueSignal(), StartAttachment(), LinkAttachments(), WhereAttachment()
 **/
int32 swiMonitorAttachment( Item Attachment, Item Cue, int32 CueAt )
{
	AudioAttachment *aatt;
	AudioCue *acue;

DBUG(("swiMonitorAttachment( Att = 0x%x, Cue = 0x%x )\n", Attachment, Cue ));
	aatt = (AudioAttachment *)CheckItem(Attachment,  AUDIONODE, AUDIO_ATTACHMENT_NODE);
	if (aatt == NULL) return AF_ERR_BADITEM;


	if (Cue == 0)
	{
		aatt->aatt_CueItem = 0;
	}
	else
	{
		acue = (AudioCue *)CheckItem(Cue,  AUDIONODE, AUDIO_CUE_NODE);
		if (acue == NULL) return AF_ERR_BADITEM;
		if(CueAt != CUE_AT_END) return AF_ERR_UNIMPLEMENTED; /* 930929 */
		aatt->aatt_CueAt = CueAt;
		aatt->aatt_CueItem = Cue;
	}

	return 0;
}

/**************************************************************/
int32 IsAttSignalNeeded( AudioAttachment *aatt )
{

DBUG(("IsAttSignalNeeded( aatt = 0x%x )\n", aatt ));
DBUG(("IsAttSignalNeeded: Cue = 0x%x, Next = 0x%x",
	aatt->aatt_CueItem, aatt->aatt_NextAttachment, aatt->aatt_Flags ));
DBUG((", Flags = 0x%x, Count = %d\n",
	aatt->aatt_Flags, aatt->aatt_SegmentCount ));

	return (aatt->aatt_CueItem ||
		aatt->aatt_NextAttachment ||
		((aatt->aatt_Flags & AF_ATTF_FATLADYSINGS) &&
			 (aatt->aatt_SegmentCount > 0)));
}
/**************************************************************/
void EnableAttSignalIfNeeded( AudioAttachment *aatt )
{
	if (IsAttSignalNeeded(aatt))
	{
		EnableAttachmentSignal( aatt );
	}
}

/***************************************************************
** Set interrupt so that it will send a signal to folio when complete.
***************************************************************/
static int32  EnableAttachmentSignal ( AudioAttachment *aatt )
{
	AudioDMAControl *admac;
	int32 chan;

	chan = aatt->aatt_Channel;
DBUG(("EnableAttachmentSignal( aatt = 0x%x ) Channel = 0x%x \n", aatt, chan ));

	admac = &DSPPData.dspp_DMAControls[chan];
	admac->admac_SignalCountDown = 1;  /*  So it can go to zero and trigger signal */
	dsphEnableChannelInterrupt( chan );
	return 0;
}

/***************************************************************
** Set interrupt so that it will update NextAddr, NextCount.
***************************************************************/
int32  SetDMANextInt ( int32 DMAChan, AudioGrain *Address, int32 Cnt )
{
	AudioDMAControl *admac;

DBUG(("SetDMANextInt( chan = %d, addr = 0x%x, cnt = %d )\n", DMAChan, Address, Cnt ));
	if( Address == NULL )
	{
		ERR(("SetDMANextInt: Address = NULL\n"));
		return -1;
	}

	admac = &DSPPData.dspp_DMAControls[DMAChan];

	dsphDisableChannelInterrupt( DMAChan );
	admac->admac_NextCountDown = 1;  /*  So it can go to zero and trigger signal */
	admac->admac_NextAddress = Address;
	admac->admac_NextCount = Cnt;
	dsphEnableChannelInterrupt( DMAChan );
	return 0;
}

/*************************************************************/
int32  DisableAttachmentSignal ( AudioAttachment *aatt )
{
	AudioDMAControl *admac;
	int32 i;
DBUG(("DisableAttachment( aatt = 0x%x )\n", aatt ));
	i = aatt->aatt_Channel;
DBUG(("DisableAttachment: Channel = 0x%x )\n", i ));
	dsphDisableChannelInterrupt(i);
	admac = &DSPPData.dspp_DMAControls[i];
	admac->admac_SignalCountDown = 0;
	admac->admac_NextCountDown = 0;
	return 0;
}


/* -------------------- Attachment query */

static int32 GetAttachmentsFromList (Item *attachments, int32 maxAttachments, const List *attList, const char *hookName, uint8 attType, bool matchDefaultHook);

/**************************************************************/
/**
|||	AUTODOC -public -class audio -group Attachment -name GetNumAttachments
|||	Returns the number of Attachment(@)s to a sample or envelope hook.
|||
|||	  Synopsis
|||
|||	    int32 GetNumAttachments (Item insOrTemplate, const char *hookName)
|||
|||	  Description
|||
|||	    This function returns the number of Attachment(@) Items made to a sample or
|||	    envelope hook of an Instrument or instrument Template.
|||
|||	  Arguments
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	    hookName
|||	        Envelope(@) or Sample(@) hook of instrument to query. Unlike
|||	        CreateAttachment(), GetNumAttachments() does not accept NULL to
|||	        indicate a default hook.
|||
|||	  Return Value
|||
|||	    Non-negative value indicating the number of attachments to the specified
|||	    hook, or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/audio.h> V30.
|||
|||	  Notes
|||
|||	    This macro is equivalent to:
|||
|||	  -preformatted
|||
|||	        GetAttachments (NULL, 0, insOrTemplate, hookName);
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    GetAttachments(), GetInstrumentPortInfoByName()
**/

/**
|||	AUTODOC -public -class audio -group Attachment -name GetAttachments
|||	Returns list of Attachment(@)s to a sample or envelope hook.
|||
|||	  Synopsis
|||
|||	    int32 GetAttachments (Item *attachmentsr, int32 maxAttachments,
|||	                          Item insOrTemplate, const char *hookName)
|||
|||	  Description
|||
|||	    This function returns an array of Attachment(@) Items made to a sample or
|||	    envelope hook of an Instrument or instrument Template.
|||
|||	  Arguments
|||
|||	    attachments
|||	        A pointer to an array of Items where the attachment list will be stored.
|||	        Can be NULL if maxAttachments is 0.
|||
|||	    maxAttachments
|||	        Maximum number of attachments to write to the attachments array. Can be
|||	        0, in which case no attachments will be written to the attachments array.
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	    hookName
|||	        Envelope(@) or Sample(@) hook of instrument to query. Unlike
|||	        CreateAttachment(), GetAttachments() does not accept NULL to indicate
|||	        a default hook.
|||
|||	  Return Value
|||
|||	    >= 0
|||	        Number of attachments to hook. The number of elements written to
|||	        attachments is MIN(return value,maxAttachments).
|||
|||	    < 0
|||	        Error code. attachments array state is undefined.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    GetNumAttachments(), GetInstrumentPortInfoByName(), insinfo(@)
**/

int32 GetAttachments (Item *attachments, int32 maxAttachments, Item insOrTemplate, const char *hookName)
{
	int32 attType;      /* @@@ an int32 instead of uint8 to permit errcode = attType = Get...() */
	bool matchDefaultHookName;
	Err errcode;

	DBUGQUERY(("GetAttachments: item=0x%x hook='%s'\n", insOrTemplate, hookName));

		/* find template, determine attachment type */
	{
		const AudioInsTemplate *aitp;
		const char *defaultHookName;

		if (!(aitp = LookupAudioInsTemplate (insOrTemplate))) return AF_ERR_BADITEM;
		if ((errcode = attType = GetAttachmentHookType (aitp, hookName)) < 0) return errcode;
		matchDefaultHookName =
			(defaultHookName = GetDefaultAttachmentHookName (aitp, attType)) != NULL &&
			!strcasecmp (defaultHookName, hookName);

		DBUGQUERY(("GetAttachments: aitp=0x%x attType=%d matchdefault=%d ('%s')\n", aitp, attType, matchDefaultHookName, defaultHookName));
	}

		/* scan appropriate attachment list */
	{
		const ItemNode *n;

		if (!(n = LookupItem (insOrTemplate))) return AF_ERR_BADITEM;

		switch (n->n_Type) {
			case AUDIO_TEMPLATE_NODE:
				{
					const AudioInsTemplate * const aitp = (AudioInsTemplate *)n;

					return GetAttachmentsFromList (attachments, maxAttachments, &aitp->aitp_Attachments, hookName, attType, matchDefaultHookName);
				}

			case AUDIO_INSTRUMENT_NODE:
				{
					/* !!! a lot of this probably ought to be in a dspp_ module */

					const AudioInstrument * const ains = (AudioInstrument *)n;
					const DSPPInstrument * const dins = (DSPPInstrument *)ains->ains_DeviceInstrument;

					switch (attType) {      /* @@@ assumed to handle all AF_ATT_TYPE_ */
						case AF_ATT_TYPE_SAMPLE:
							{
								const FIFOControl *fico;

									/* @@@ This doesn't need to handle hookName==NULL case, because
									**     that is filtered out above. Could get away with simpler system
									**     for finding the FIFOControls. */
								if ((errcode = dsppFindFIFOByName (dins, hookName, &fico)) < 0) return errcode;

									/* @@@ Somewhat redundant because it checks hookName for each of these,
									**     even though this list only applies to the selected hook. */
								return GetAttachmentsFromList (attachments, maxAttachments, &fico->fico_Attachments, hookName, attType, matchDefaultHookName);
							}

						case AF_ATT_TYPE_ENVELOPE:
							return GetAttachmentsFromList (attachments, maxAttachments, &dins->dins_EnvelopeAttachments, hookName, attType, matchDefaultHookName);
					}
				}
				/* @@@ falls thru here if attType isn't caught by switch() */

			default:
				return AF_ERR_BADITEM;      /* @@@ redundant because we filtered item type in LookupAudioInsTemplate() */
		}
	}
}

/*
	Scan list of attachments for those which match the hook name.

	Arguments
		attachments, numAttachments
			Array pointer and number of elements to fill out. numAttachments
			can be 0, which means that attachments[] is not written to.

		attList
			List containing AudioAttachments to scan.

		hookName
			hook name to match. NULL isn't legal here.

		attType
			AF_ATT_TYPE_ of attachment to match.

		matchDefaultHook
			TRUE if aatt_HookName==NULL is to be considered the same as matching hookName.
			!!! this is to cope with aatt_HookName==NULL in the default case. this might
				become unnecessary at some point.

	Results
		Number of matching attachments (a non-negative value).
		Fills out MIN(result,maxAttachment) elements of attachments[]
*/
static int32 GetAttachmentsFromList (Item *attachments, int32 maxAttachments, const List *attList, const char *hookName, uint8 attType, bool matchDefaultHook)
{
	const AudioAttachment *aatt;
	int32 numAttachments = 0;

	SCANLIST (attList, aatt, AudioAttachment) {

	    DBUGQUERY(("  0x%x type=%d hook='%s'\n", aatt->aatt_Item.n_Item, aatt->aatt_Type, aatt->aatt_HookName));

		if (aatt->aatt_Type == attType &&
			(aatt->aatt_HookName
				? !strcasecmp (aatt->aatt_HookName, hookName)
				: matchDefaultHook))
		{
			if (numAttachments < maxAttachments) {
				attachments[numAttachments] = aatt->aatt_Item.n_Item;
			}
			numAttachments++;
		}
	}

	return numAttachments;
}


/* -------------------- hook helpers */
/* !!! some of these probably ought to be in a dspp_ module */

/*
	Returns the AF_ATT_TYPE_ of the named attachment hook.

	Arguments
		aitp
			AudioInsTemplate to scan

		hookName
			Name of hook to find. NULL isn't legal (and is trapped in
			BUILD_PARANOIA mode).

	Results
		Attachment type (AF_ATT_TYPE_) of hook, or Err code if no hook of
		the requested name is found.
*/
static int32 GetAttachmentHookType (const AudioInsTemplate *aitp, const char *hookName)
{
	const DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;
	int32 rsrcIndex;
	DSPPEnvHookRsrcInfo deri;

#if BUILD_PARANOIA
	if (!hookName) return AF_ERR_BAD_NAME;
#endif

		/* look for FIFO resource with the specified name */
	if ((rsrcIndex = DSPPFindResourceIndex (dtmp, hookName)) >= 0) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];

		return (drsc->drsc_Type == DRSC_TYPE_IN_FIFO || drsc->drsc_Type == DRSC_TYPE_OUT_FIFO)
			? AF_ATT_TYPE_SAMPLE
			: AF_ERR_BAD_PORT_TYPE;
	}
		/* look for envelope hook with the specified name */
	else if (dsppFindEnvHookResources (&deri, sizeof deri, dtmp, hookName) >= 0) {
		return AF_ATT_TYPE_ENVELOPE;
	}
		/* no match */
	else {
		return AF_ERR_NAME_NOT_FOUND;
	}
}

/*
	Returns default hook name for the specified hook type.

	Arguments
		aitp
			AudioInsTemplate to scan

		attType
			Attachment type (AF_ATT_TYPE_) to find default name. Assumed to be valid.

	Results
		Pointer to default hook name for the requested attachment type, or NULL if there
		is no default hook of this type. If a name is returned, it is guaranteed to exist.
*/
static const char *GetDefaultAttachmentHookName (const AudioInsTemplate *aitp, uint8 attType)
{
	const DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;

	switch (attType) {
		case AF_ATT_TYPE_ENVELOPE:
			{
				DSPPEnvHookRsrcInfo deri;

				return dsppFindEnvHookResources (&deri, sizeof deri, dtmp, AF_DEFAULT_ENV_HOOK) >= 0
					? AF_DEFAULT_ENV_HOOK
					: NULL;
			}

		case AF_ATT_TYPE_SAMPLE:
			{
					/* find resource index of default FIFO */
				const int32 fifoRsrcIndex = dsppFindDefaultFIFORsrcIndex (dtmp);

					/* if there is one, get its name */
				return fifoRsrcIndex >= 0
					? dsppGetTemplateRsrcName (dtmp, fifoRsrcIndex)
					: NULL;
			}
	}

	return NULL;    /* @@@ redundant, but keeps compiler happy */
}

/*
	Find resource index of default Sample hook (FIFO) for Template, if there is one.

	Arguments
		dtmp
			DSPPTemplate to scan

	Results
		Non-negative resource index, if default FIFO exists. Negative error code otherwise.
*/
static int32 dsppFindDefaultFIFORsrcIndex (const DSPPTemplate *dtmp)
{
	int32 fifoRsrcIndex = AF_ERR_BAD_NAME;  /* convenient default */
	int32 i;

	for (i=0; i<dtmp->dtmp_NumResources; i++) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[i];

		if (drsc->drsc_Type == DRSC_TYPE_IN_FIFO || drsc->drsc_Type == DRSC_TYPE_OUT_FIFO) {
				/* if more than one FIFO found, there's no default */
			if (fifoRsrcIndex >= 0) return AF_ERR_BAD_NAME;

				/* keep index to this resource for later */
			fifoRsrcIndex = i;
		}
	}

	return fifoRsrcIndex;
}


/* -------------------- Debugging */

#if DEBUG_Item      /* { */

#define DUMPDESCFMT "  %-18s "
#define REPORT_ATT(format,member) printf(DUMPDESCFMT format "\n", #member, aatt->member)

static void internalDumpAttachment (const AudioAttachment *aatt, const char *banner)
{
	PRT(("--------------------------\n"));
	if (banner) PRT(("%s: ", banner));
	PRT(("Attachment 0x%x ('%s') @ 0x%08x\n", aatt->aatt_Item.n_Item, aatt->aatt_Item.n_Name, aatt));
	{
		const ItemNode * const n = LookupItem (aatt->aatt_HostItem);

		PRT((DUMPDESCFMT "0x%x ", "aatt_HostItem", aatt->aatt_HostItem));
		if (n) {
			PRT(("(type %d", n->n_Type));
			if (n->n_Name) PRT((", name '%s'", n->n_Name));
			PRT((") @ 0x%x\n", n));
		}
		else {
			PRT((" --- invalid!\n"));
		}
	}
	{
		const ItemNode * const n = LookupItem (aatt->aatt_SlaveItem);

		PRT((DUMPDESCFMT "0x%x ", "aatt_SlaveItem", aatt->aatt_SlaveItem));
		if (n) {
			PRT(("(type %d", n->n_Type));
			if (n->n_Name) PRT((", name '%s'", n->n_Name));
			PRT((") @ 0x%x\n", n));
		}
		else {
			PRT((" --- invalid!\n"));
		}
	}
	REPORT_ATT("0x%02x",aatt_Item.n_Flags);
	REPORT_ATT("'%s'",aatt_HookName);
	REPORT_ATT("%d",aatt_StartAt);
	REPORT_ATT("%d",aatt_Type);
	REPORT_ATT("%d",aatt_SubType);
	REPORT_ATT("0x%02x",aatt_Flags);

	/* @@@ more? */
}

#endif      /* } */
