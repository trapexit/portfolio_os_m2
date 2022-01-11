/******************************************************************************
**
**  @(#) audio_template.c 96/08/23 1.2
**
**  Instrument Template functions.
**
**  By: Bill Barton
**
**  Copyright (c) 1996, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960823 WJB  Split off from audio_instr.c
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <kernel/usermodeservices.h>    /* to support ir_Load */

#include "audio_internal.h"
#include "dspp.h"
#include "dspp_resources.h"     /* resource usage */


/* -------------------- Debug */

#define DEBUG_Template      0   /* debug Template Item create/delete/load/open/close operations */
#define DEBUG_TemplateOwner 0   /* debug internalSetAudioTemplateOwner() */

#if DEBUG_Template || DEBUG_TemplateOwner
#include "audio_debug.h"
#endif

#if DEBUG_Template
#define DBUGTMPL(x) PRT(x)
#else
#define DBUGTMPL(x)
#endif


/* -------------------- Local Functions */

#define FindTemplateNode(name) ((AudioInsTemplate *)FindNamedNode(&AB_FIELD(af_TemplateList), name))

static Err dsppShareTemplate (DSPPTemplate **resultDTmp, const char *shareTemplateName);
static void dsppUnshareTemplate (DSPPTemplate *);

static Err OpenDynamicLinkTemplates (Item **resultDynamicLinkTemplates, const DSPPTemplate *);
static void CloseDynamicLinkTemplates (Item *dynamicLinkTemplates);

#if DEBUG_TemplateOwner
static void DumpResources (const Task *);
#endif


/* -------------------- Template Item Documentation */

/**
|||	AUTODOC -public -class Items -group Audio -name Template
|||	A description of an audio instrument.
|||
|||	  Description
|||
|||	    A Template is the description of a DSP audio instrument (including the DSP
|||	    code, resource requirements, parameter settings, etc.) from which
|||	    Instrument items are created. A Template can either be a standard system
|||	    instrument template (e.g. envelope.dsp(@), sampler_16_v1.dsp(@), etc), a
|||	    mixer template, or a custom patch template.
|||
|||	  Folio
|||
|||	    audio
|||
|||	  Item Type
|||
|||	    AUDIO_TEMPLATE_NODE
|||
|||	  Create
|||
|||	    CreateMixerTemplate(), CreatePatchTemplate(), LoadInsTemplate()
|||
|||	  Delete
|||
|||	    DeleteItem(), DeleteMixerTemplate(), DeletePatchTemplate(),
|||	    UnloadInsTemplate()
|||
|||	  Use
|||
|||	    CreateInstrument(), AdoptInstrument(), CreateAttachment(), TuneInsTemplate()
|||
|||	  See Also
|||
|||	    Instrument(@), Attachment(@), Sample(@)
**/

/**
|||	AUTODOC -private -class Items -group Audio -name Template-private
|||	Private Instrument Template(@) tags.
|||
|||	  Folio
|||
|||	    audio
|||
|||	  Item Type
|||
|||	    AUDIO_TEMPLATE_NODE
|||
|||	  Tags
|||
|||	    Mutually exclusive creation tags:
|||
|||	    AF_TAG_MIXER_SPEC (MixerSpec) - Create (!!! to be retired)
|||	        MixerSpec of mixer template to create.
|||
|||	    AF_TAG_SHARE (void) - Create
|||	        Create another Template that shares the DSPPTemplate owned by
|||	        TAG_ITEM_NAME. Presence of this tag is sufficient. ta_Arg is ignored.
|||
|||	    AF_TAG_TEMPLATE (const DSPPTemplate *) - Create
|||	        Create new Template with supplied DSPPTemplate. Pass in user-mode
|||	        DSPPTemplate. This is validated and then cloned in supervisor memory.
|||	        Once CreateItem() has finished, caller can delete original user-mode
|||	        DSPPTemplate.
|||
|||	    Additional Tags:
|||
|||	    TAG_ITEM_NAME (const char *) - Create
|||	        Optional name for AF_TAG_MIXER_SPEC and AF_TAG_TEMPLATE. Required name
|||	        for AF_TAG_SHARE.
|||
|||	  See Also
|||
|||	    Template(@), dsppCreateUserTemplate(), dsppDeleteUserTemplate()
**/


/* -------------------- Load and Unload Standard DSP Instrument Template */

/*****************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name LoadInsTemplate
|||	Loads a standard DSP Instrument Template(@).
|||
|||	  Synopsis
|||
|||	    Item LoadInsTemplate ( const char *insName, const TagArg *tagList )
|||
|||	    Item LoadInsTemplateVA ( const char *insName, uint32 tag1, ... )
|||
|||	  Description
|||
|||	    This procedure loads an Instrument Template(@) from the specified file. Note
|||	    that the procedure doesn't create an instrument from the instrument
|||	    template. Call CreateInstrument() to create an Instrument from this Template.
|||
|||	    When you finish using the Template, call UnloadInsTemplate() to deallocate
|||	    the template's resources.
|||
|||	  Arguments
|||
|||	    insName
|||	        Name of the instrument, e.g., sawtooth.dsp(@).
|||
|||	    tagList
|||	        Tag list. None currently supported. Pass NULL.
|||
|||	  Return Value
|||
|||	    The procedure returns a Template item number if successful (a positive
|||	    value) or an error code (a negative value) if an error occurs.
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
|||	    UnloadInsTemplate(), CreateMixerTemplate(), CreatePatchTemplate(),
|||	    CreateInstrument(), LoadInstrument(), CreateAttachment(),
|||	    LoadPatchTemplate(), LoadScoreTemplate()
**/
/* -LoadInsTemplate */
Item LoadInsTemplate ( const char *InsName, const TagArg *tagList )
{
	Item result;

		/* @@@ no other tags supported yet */
		/* !!! not sure whether this trap is actually necessary */
	{
		const TagArg *tstate = tagList;

		if (NextTagArg (&tstate)) return AF_ERR_BADTAG;
	}

		/* Look to see if instrument has already been loaded. If so, share. 940224 */
	DBUGTMPL(("LoadInsTemplate: try to share '%s'\n", InsName));
	result = CreateItemVA ( MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
							TAG_ITEM_NAME,  InsName,
							AF_TAG_SHARE,   TRUE,
							TAG_JUMP,       tagList );
		/*
			If this returns AF_ERR_INTERNAL_NOT_SHAREABLE, then it must not be loaded yet.
			press on and load it.

			Otherwise, we either got a successfully cloned template, or
			CreateItem() failed. Either way, return the value we got back.
		*/
	if (result != AF_ERR_INTERNAL_NOT_SHAREABLE) {
		DBUGTMPL(("LoadInsTemplate: share '%s' returned 0x%x\n", InsName, result));
		return result;
	}

		/* If not already loaded, load template from file. */
	{
		DSPPTemplate *userdtmp;

		DBUGTMPL(("LoadInsTemplate: try to load '%s'\n", InsName));
		if ((result = dsppLoadInsTemplate (&userdtmp, InsName)) >= 0) {
			result = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
							TAG_ITEM_NAME,      InsName,
							AF_TAG_TEMPLATE,    userdtmp,
							TAG_JUMP,           tagList);
			dsppDeleteUserTemplate (userdtmp);
		}
	}
  #if DEBUG_Template || DEBUG_TemplateOwner
	PRT(("\nLoadInsTemplate: '%s' returns 0x%x\n", InsName, result ));
  #endif
  #if DEBUG_TemplateOwner
	DumpResources (CURRENTTASK);
	DumpResources (AB_FIELD(af_AudioDaemon));
  #endif

	return result;
}

/**
|||	AUTODOC -public -class audio -group Instrument -name UnloadInsTemplate
|||	Unloads an instrument Template(@) loaded by LoadInsTemplate().
|||
|||	  Synopsis
|||
|||	    Err UnloadInsTemplate (Item insTemplate)
|||
|||	  Description
|||
|||	    This procedure unloads (or deletes) an instrument template created by
|||	    LoadInsTemplate(). It unloads the Template from memory and frees its
|||	    resources. All Instrument(@)s created using the Template are deleted, with
|||	    the side effects of DeleteInstrument(). All Attachment(@)s made to this
|||	    Template are deleted. If any of those attachments were created with
|||	    { AF_TAG_AUTO_DELETE_SLAVE, TRUE }, the associated slave items are also
|||	    deleted.
|||
|||	  Arguments
|||
|||	    insTemplate
|||	        Item number of Template to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/audio.h> V29.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    LoadInsTemplate(), Template(@), Instrument(@), Attachment(@)
**/


/* -------------------- Mixer Templates */

/*****************************************************************/
/**
|||	AUTODOC -public -class audio -group Mixer -name CreateMixerTemplate
|||	Creates a custom mixer Template(@).
|||
|||	  Synopsis
|||
|||	    Item CreateMixerTemplate (MixerSpec mixerSpec, const TagArg *tags );
|||
|||	    Item CreateMixerTemplateVA (MixerSpec mixerSpec, uint32 tag1, ... );
|||
|||	  Description
|||
|||	    This function creates a custom, multi-channel Mixer Template. Use the macro
|||	    MakeMixerSpec() to generate the 32-bit packed MixerSpec that you pass to
|||	    this function. You can specify any number of "Input" channels from 1-32, and
|||	    1-8 "Output" channels. There are other options as well, see MakeMixerSpec()
|||	    for more details.
|||
|||	    The created mixer has multi-part "Gain" knob that is used to control the
|||	    amount of each Input that goes to each Output. The Gain knob part number for
|||	    Input A going to Output B for a mixer with NI Inputs is calculated as follows:
|||
|||	    GainKnobPartNumber = A + (B * NI)
|||
|||	    The macro CalcMixerGainPart() does this calculation.
|||
|||	    All parts of the Gain knob default to 0. The Amplitude knob, if requested,
|||	    defaults to 1.0.
|||
|||	    Call DeleteMixerTemplate() when done with this Template.
|||
|||	  Arguments
|||
|||	    mixerSpec
|||	        MixerSpec describing the mixer to construct. Returned by
|||	        MakeMixerSpec().
|||
|||	  Tag Arguments
|||
|||	    TAG_ITEM_NAME (const char *)
|||	        Optional name for the newly created template. Defaults to not having a
|||	        name.
|||
|||	  Return Value
|||
|||	    The procedure returns an instrument Template Item number (a non-negative
|||	    value) if successful or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V28.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    MakeMixerSpec(), Mixer(@), Template(@), CalcMixerGainPart(),
|||	    DeleteMixerTemplate(), LoadInsTemplate(), CreatePatchTemplate(),
|||	    CreateInstrument()
**/
/* -CreateMixerTemplate */
Item CreateMixerTemplate (MixerSpec mixerSpec, const TagArg *tagList)
{
		/* !!! temp until mixer creation becomes user mode */
	return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
		AF_TAG_MIXER_SPEC,  mixerSpec,
		TAG_JUMP,           tagList);

#if 0       /* !!! what this function should eventually look like */
	DSPPTemplate *userdtmp;
	Item result;

	if ((result = dsppCreateMixerTemplate (&userdtmp, mixerSpec)) >= 0) {
		result = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
						AF_TAG_TEMPLATE,    userdtmp,
						TAG_JUMP,           tagList);
		dsppDeleteUserTemplate (userdtmp);
	}

	return result;
#endif
}

/**
|||	AUTODOC -public -class audio -group Mixer -name DeleteMixerTemplate
|||	Deletes a mixer Template created by CreateMixerTemplate()
|||
|||	  Synopsis
|||
|||	    Err DeleteMixerTemplate (Item mixerTemplate)
|||
|||	  Description
|||
|||	    This macro deletes a mixer Template created by CreateMixerTemplate(). This
|||	    has the same side effects as UnloadInsTemplate() (e.g., deleting
|||	    instruments).
|||
|||	  Arguments
|||
|||	    mixerTemplate
|||	        Mixer Template Item to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns an non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/audio.h> V28.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    CreateMixerTemplate()
**/

/**
|||	AUTODOC -public -class audio -group Mixer -name MakeMixerSpec
|||	Makes a MixerSpec for use with CreateMixerTemplate().
|||
|||	  Synopsis
|||
|||	    MixerSpec MakeMixerSpec (uint8 numInputs, uint8 numOutputs, uint16 flags)
|||
|||	  Description
|||
|||	    Constructs a 32-bit packed MixerSpec to pass to CreateMixerTemplate() and
|||	    other mixer-related functions.  The mixer created from this spec will have:
|||	    an "Input" port with N parts,
|||	    an "Output" port with M parts (unless AF_F_MIXER_WITH_LINE_OUT is set),
|||	    and a "Gain" knob with N*M parts (see CalcMixerGainPart()).
|||	    If AF_F_MIXER_WITH_AMPLITUDE is set then it will also have an "Amplitude"
|||	    knob which acts as a master gain control.
|||
|||	    Output[m] = Amplitude * (Input[0]*Gain[0,m] + Input[1]*Gain[1,m] + .... )
|||
|||	  Arguments
|||
|||	    numInputs
|||	        Number of "Input" channels from 1-32.  (See caveat below!)
|||
|||	    numOutputs
|||	        Number of "Output" channels from 1-8.
|||
|||	    flags
|||	        Set of AF_F_MIXER_ flags (defined below) to control mixer
|||	        construction.
|||
|||	  Flags
|||	    AF_F_MIXER_WITH_AMPLITUDE
|||	        Mixer will have a global Amplitude knob that can be used as a master
|||	        gain control.
|||
|||	    AF_F_MIXER_WITH_LINE_OUT
|||	        Mixer will have an internal connection to the global system mixer. If
|||	        you don't set this flag the mixer has an "Output" port that can be
|||	        connected to a line_out.dsp(@) to be heard, or to a delay effect, etc.
|||	        Note: this flag is only valid if numOutputs is 2.
|||
|||	  Return Value
|||
|||	    Constructed MixerSpec.
|||
|||	  Implementation
|||
|||	    Macro implemented in audio folio V28.
|||
|||	  Caveats
|||
|||	    This macro doesn't trap bad input values, so it is possible to construct an
|||	    invalid MixerSpecs. CreateMixerTemplate() checks the validity of the
|||	    MixerSpec it is passed.
|||
|||	    Note that there is a bug in V28 that causes mixers with only 1 input to fail!
|||	    As a workaround, set the AF_F_MIXER_WITH_AMPLITUDE flag and it will work.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    CreateMixerTemplate(), CalcMixerGainPart(), MixerSpecToNumIn(),
|||	    MixerSpecToNumOut(), MixerSpecToFlags()
**/

/**
|||	AUTODOC -public -class audio -group Mixer -name CalcMixerGainPart
|||	Returns the mixer gain knob part number for a specified input and output.
|||
|||	  Synopsis
|||
|||	    int32 CalcMixerGainPart (MixerSpec mixerSpec, int32 inChannel, int32 outChannel)
|||
|||	  Description
|||
|||	    Returns the gain knob part number for a given input and output pair using
|||	    the following formula:
|||
|||	    GainKnobPartNumber = outChannel * MixerSpecToNumIn(mixerSpec) + inChannel
|||
|||	  Arguments
|||
|||	    mixerSpec
|||	        MixerSpec for the mixer for which to compute the gain knob part.
|||
|||	    inChannel
|||	        Input channel number (0..numInputs).
|||
|||	    outChannel
|||	        Output channel number (0..numOutputs).
|||
|||	  Return Value
|||
|||	    Gain knob part number.
|||
|||	  Implementation
|||
|||	    Macro implemented in audio folio V28.
|||
|||	  Caveats
|||
|||	    This macro doesn't trap bad input values. SetKnobPart() traps out of range
|||	    part numbers.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    MakeMixerSpec(), CreateMixerTemplate()
**/

/**
|||	AUTODOC -public -class audio -group Mixer -name MixerSpecToNumIn
|||	Extracts number of inputs from MixerSpec.
|||
|||	  Synopsis
|||
|||	    uint8 MixerSpecToNumIn (MixerSpec mixerSpec)
|||
|||	  Description
|||
|||	    Returns the number of inputs specified in the MixerSpec.
|||
|||	  Arguments
|||
|||	    mixerSpec
|||	        MixerSpec to examine.
|||
|||	  Return Value
|||
|||	    Number of inputs.
|||
|||	  Implementation
|||
|||	    Macro implemented in audio folio V28.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    MakeMixerSpec(), MixerSpecToNumOut(), MixerSpecToFlags()
**/

/**
|||	AUTODOC -public -class audio -group Mixer -name MixerSpecToNumOut
|||	Extracts number of outputs from MixerSpec.
|||
|||	  Synopsis
|||
|||	    uint8 MixerSpecToNumOut (MixerSpec mixerSpec)
|||
|||	  Description
|||
|||	    Returns the number of outputs specified in the MixerSpec.
|||
|||	  Arguments
|||
|||	    mixerSpec
|||	        MixerSpec to examine.
|||
|||	  Return Value
|||
|||	    Number of outputs.
|||
|||	  Implementation
|||
|||	    Macro implemented in audio folio V28.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    MakeMixerSpec(), MixerSpecToNumIn(), MixerSpecToFlags()
**/

/**
|||	AUTODOC -public -class audio -group Mixer -name MixerSpecToFlags
|||	Extracts mixer flags from MixerSpec.
|||
|||	  Synopsis
|||
|||	    uint16 MixerSpecToFlags (MixerSpec mixerSpec)
|||
|||	  Description
|||
|||	    Returns the AF_F_MIXER_ flags specified in the MixerSpec.
|||
|||	  Arguments
|||
|||	    mixerSpec
|||	        MixerSpec to examine.
|||
|||	  Return Value
|||
|||	    AF_F_MIXER_ flags.
|||
|||	  Implementation
|||
|||	    Macro implemented in audio folio V28.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    MakeMixerSpec(), MixerSpecToNumIn(), MixerSpecToNumOut()
**/


/* -------------------- Template Item Management */

	/* context data for CreateInsTemplateTagHandler() */
typedef struct CreateInsTemplateTagData {
	int32               td_SpecCount;
	const DSPPTemplate *td_UserDTmp;
	MixerSpec           td_MixerSpec;       /* !!! will go away */
} CreateInsTemplateTagData;

static Err CreateInsTemplateTagHandler (AudioInsTemplate *aitp, CreateInsTemplateTagData *tagdata, uint32 tag, TagData arg);
static Err ValidateInsTemplateName (const DSPPTemplate *, const char *name);

static void StripAudioInsTemplate (AudioInsTemplate *aitp);


/*******************************************************************/
/*
	AUDIO_TEMPLATE_NODE ir_Create method.

	Arguments
		aitp
			Allocated and cleared AudioInsTemplate ready to be filled out.

	Mutually exclusive creation tags
		AF_TAG_MIXER_SPEC (MixerSpec)       !!! will go away
			Mixer spec of mixer to create.

		AF_TAG_SHARE (void)
			Create another Template that shares the DSPPTemplate owned by TAG_ITEM_NAME.
			Presence of this tag is sufficient. ta_Arg is ignored.

		AF_TAG_TEMPLATE (const DSPPTemplate *)
			Create new Template with supplied DSPPTemplate. Pass in user-mode DSPPTemplate.
			This is validated and then cloned in supervisor memory. Once this has
			finished, caller can delete original DSPPTemplate.

	Additional tags
		TAG_ITEM_NAME (const char *)
			Optional name for AF_TAG_MIXER_SPEC and AF_TAG_TEMPLATE.
			Required name for AF_TAG_SHARE.

	Return Value
		Item number of AudioInsTemplate on success. Err code on failure.
*/
Item internalCreateAudioTemplate (AudioInsTemplate *aitp, const TagArg *tagList)
{
	CreateInsTemplateTagData tagdata;
	DSPPTemplate *dtmp;
	Item result;

  #if DEBUG_Template
	PRT(("internalCreateAudioTemplate(0x%x,0x%x) item=0x%x\n",aitp,tagList,aitp->aitp_Item.n_Item));
  #endif

		/* init tagdata */
	memset (&tagdata, 0, sizeof tagdata);

		/* pre-init aitp so that we can call common cleanup code on failure */
	PrepList (&aitp->aitp_InstrumentList);
	PrepList (&aitp->aitp_Attachments);

		/* process tags */
	if ((result = TagProcessor (aitp, tagList, CreateInsTemplateTagHandler, &tagdata)) < 0) {
		ERR(("internalCreateAudioTemplate: TagProcessor failed with 0x%x.\n", result));
		goto clean;
	}
	DBUGTMPL(("internalCreateAudioTemplate: item name after TagProcessor = '%s'\n", aitp->aitp_Item.n_Name ));

		/* Make sure we pass one and only one spec for template. */
	if (tagdata.td_SpecCount != 1) {
	  #if BUILD_STRINGS
		if (tagdata.td_SpecCount > 1) {
			ERR (("internalCreateAudioTemplate: Too many creation tags: %d\n", tagdata.td_SpecCount));
		}
		else {
			ERR (("internalCreateAudioTemplate: No creation tags\n"));
		}
	  #endif
		result = AF_ERR_BADTAG;
		goto clean;
	}

		/* get DSPPTemplate for this operation (either shared or created) */
	if (tagdata.td_UserDTmp) {
			/* bless and promote userDTmp (@@@ reminder: userDTmp hasn't been validated yet - don't dereference) */
		if ((result = dsppPromoteTemplate (&dtmp, tagdata.td_UserDTmp)) < 0) goto clean;
	}
	else if (tagdata.td_MixerSpec) {
			/* build a mixer (!!! this will become a AF_TAG_TEMPLATE client) */
		if ((result = dsppCreateMixerTemplate (&dtmp, tagdata.td_MixerSpec)) < 0) goto clean;
	}
	else {  /* other conditions trapped, so this is the only one left */
			/* share named template, if possible */
		const char * const shareTemplateName = aitp->aitp_Item.n_Name;  /* this name is guaranteed valid if non-NULL */

		if (!shareTemplateName) {
			result = AF_ERR_BAD_NAME;
			goto clean;
		}

		if ((result = dsppShareTemplate (&dtmp, shareTemplateName)) < 0) goto clean;
	}
		/* link built or shared DSPPTemplate into aitp - permits it to be freed when something here below fails */
	aitp->aitp_DeviceTemplate = dtmp;

		/* if template is privileged, make sure task is too. */
	if ((dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED) && !(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) {
		ERR(("internalCreateAudioTemplate: must be privileged to create this template. id=%d name='%s'\n",
		dtmp->dtmp_Header.dhdr_FunctionID, aitp->aitp_Item.n_Name));
		result = AF_ERR_BADPRIV;
		goto clean;
	}

		/* trap invalid name */
	if ((result = ValidateInsTemplateName (dtmp, aitp->aitp_Item.n_Name)) < 0) goto clean;

		/* open dynamically linked library templates */
	if ((result = OpenDynamicLinkTemplates (&aitp->aitp_DynamicLinkTemplates, dtmp)) < 0) goto clean;

  #if DEBUG_Template
	DumpInsTemplate(aitp, "\ninternalCreateAudioTemplate: success");
  #endif

		/* success: add to template list and set return value */
	AddTail (&AB_FIELD(af_TemplateList), (Node *)aitp);
	result = aitp->aitp_Item.n_Item;

clean:
	if (result < 0)
	{
DBUGTMPL(("StripAudioInsTemplate() called from internalCreateAudioTemplate(), result = 0x%x\n", result));
		StripAudioInsTemplate (aitp);
	}
	return result;
}

/*
	TagProcessor() callback function for internalCreateAudioTemplate()

	Arguments
		tagdata
			Place to store results. Assumes that client has cleared it before
			calling TagProcessor().
*/
static Err CreateInsTemplateTagHandler (AudioInsTemplate *aitp, CreateInsTemplateTagData *tagdata, uint32 tag, TagData arg)
{
	TOUCH(aitp);

	DBUGTMPL(("internalCreateAudioTemplate: tag { %d, 0x%x }\n", tag, arg));

	switch (tag) {
		case AF_TAG_MIXER_SPEC:         /* build a mixer */ /* !!! will become an AF_TAG_TEMPLATE client */
			{
				const MixerSpec mixerSpec = (uint32)arg;

					/* trap 0 because we use non-zero as the signature of a
					   mixer being created in internalCreateAudioTemplate() */
				if (!mixerSpec) return AF_ERR_BAD_MIXER;

				tagdata->td_MixerSpec = mixerSpec;
				tagdata->td_SpecCount++;
				DBUGTMPL(("internalCreateAudioTemplate: mixerSpec = 0x%x\n", tagdata->td_MixerSpec));
			}
			break;

		case AF_TAG_TEMPLATE:           /* build Template Item around this DSPPTemplate */
			{
				const DSPPTemplate * const userDTmp = (DSPPTemplate *)arg;

					/* trap NULL because we use non-NULL as the signature of a
					   template being loaded in internalCreateAudioTemplate() */
				if (!userDTmp) return AF_ERR_BADTAGVAL;

				tagdata->td_UserDTmp = userDTmp;
				tagdata->td_SpecCount++;
			}
			break;

		case AF_TAG_SHARE:              /* share the DSPPTemplate belonging to template named in n_Name */
										/* @@@ this is the default case, so we don't track the fact that
											   we got it. */
										/* @@@ not checking ta_Arg, since we don't really care. If
											   anyone abuses it we can always replace it with a new tag. */
			tagdata->td_SpecCount++;
			break;

		default:
			ERR (("internalCreateAudioTemplate: Unrecognized tag { %d, 0x%x }\n", tag, arg));
			return AF_ERR_BADTAG;
	}

	return 0;
}

/*
	Check for legal name for new DSPPTemplate.

	Rules
		loadable (function id >= DFID_FIRST_LOADABLE):
			name must not be NULL and must end in .dsp

		non-loadable (function id < DFID_FIRST_LOADABLE):
			name may be NULL. otherwise name must not end in .dsp

	Arguments
		dtmp
			DSPPTemplate to test

		name
			Name to test. Can be NULL, but otherwise is assumed to point to valid memory.

	Results
		0 on success, error code on failure.
*/
static Err ValidateInsTemplateName (const DSPPTemplate *dtmp, const char *name)
{
	bool nameHasDotDSPSuffix = FALSE;

	if (name) {
		static const char dotDSPSuffix[] = ".dsp";
		const size_t nameLen = strlen(name);

		if (nameLen >= (sizeof dotDSPSuffix-1) && !strcasecmp (&name[nameLen-(sizeof dotDSPSuffix-1)], dotDSPSuffix)) {
			nameHasDotDSPSuffix = TRUE;
		}
	}

	{
		const bool loadable = (dtmp->dtmp_Header.dhdr_FunctionID >= DFID_FIRST_LOADABLE);

		if (loadable != nameHasDotDSPSuffix) {
			ERR(("internalCreateAudioTemplate: invalid name for template. id=%d name='%s'.\n", dtmp->dtmp_Header.dhdr_FunctionID, name));
			return AF_ERR_BAD_NAME;
		}
	}

	return 0;
}


/*******************************************************************/
/*
	AUDIO_TEMPLATE_NODE ir_Delete method.

	Arguments
		aitp
			Completely valid AudioInsTemplate to delete.

		ct
			Owning task (@@@ only used for debugging)
*/
int32 internalDeleteAudioTemplate (AudioInsTemplate *aitp, Task *ct)
{
	TOUCH(ct);      /* @@@ only used for debugging */

  #if DEBUG_Template || DEBUG_TemplateOwner
	DumpInsTemplate (aitp, "\ninternalDeleteAudioTemplate");
  #endif
  #if DEBUG_TemplateOwner
	DumpResources (ct);
  #endif

	ParanoidRemNode ((Node *)aitp);         /* Remove from AudioFolio list. */
	DBUGTMPL(("StripAudioInsTemplate() called from internalDeleteAudioTemplate()\n"));
	StripAudioInsTemplate (aitp);           /* free stuff hanging off of this template */

  #if DEBUG_TemplateOwner
	PRT(("internalDeleteAudioTemplate: 0x%x '%s' done\n", aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name));
	DumpResources (ct);
  #endif

	return 0;
}

/*
	Clean up everything attached to AudioInsTemplate. Common cleanup
	code for partial success of internalCreateAudioTemplate() and
	internalDeleteAudioTemplate().

	Arguments
		aitp
			Legit pointer to an AudioInsTemplate. It has at least been
			initialized. It may have attachments, instruments, and
			a tuning.
*/
static void StripAudioInsTemplate (AudioInsTemplate *aitp)
{
	DBUGTMPL(("StripAudioInsTemplate: template = %s\n", aitp->aitp_Item.n_Name));

		/* Delete Items that are hanging off of this one. */
	afi_DeleteLinkedItems (&aitp->aitp_InstrumentList);
	afi_DeleteLinkedItems (&aitp->aitp_Attachments);
	/* !!! need a flag, or maybe some kind of stewardship list, to know whether to delete tuning */

		/* close dynamic link templates */
	CloseDynamicLinkTemplates (aitp->aitp_DynamicLinkTemplates);
	aitp->aitp_DynamicLinkTemplates = NULL;     /* @@@ perhaps gratuitous, but safe */

		/* unshare/delete DSPPTemplate */
	dsppUnshareTemplate ((DSPPTemplate *)aitp->aitp_DeviceTemplate);
	aitp->aitp_DeviceTemplate = NULL;                /* @@@ perhaps gratuitous, but safe */
}


/*****************************************************************************
** AUDIO_TEMPLATE_NODE ir_Find method. Called by internalFindAudioItem()
*/
Item internalFindInsTemplate(TagArg *tags)
{
	ItemNode  inode;
	ItemNode *it;
	Item      result;

	memset(&inode,0,sizeof(inode));

	result = TagProcessorNoAlloc(&inode,tags,NULL,0);
	if (result < 0) return result;

	if (inode.n_Name == NULL) return AF_ERR_BAD_NAME;

DBUGTMPL(("internalFindInsTemplate: find %s\n", inode.n_Name ));
/* !!! Do we need something like this?    SuperInternalLockSemaphore(FontBase->ff_FontLock, SEM_WAIT); */

	it = (ItemNode *)FindTemplateNode(inode.n_Name);

/*    SuperInternalUnlockSemaphore(FontBase->ff_FontLock); !!! see above */

DBUGTMPL(("internalFindInsTemplate: found node at 0x%x item=0x%x\n", it, it ? it->n_Item : 0));

	if (it == NULL) return AF_ERR_NOTFOUND; /* @@@ magic error code (ER_NotFound) to cause ir_Load to be called */

	return it->n_Item;
}


/*****************************************************************************
** AUDIO_TEMPLATE_NODE ir_Open method. Called by internalOpenAudioItem()
**
** Permit opening a template only if the template has DHDR_F_SHARED set
** (which marks a library template)
*/
Item internalOpenInsTemplate( AudioInsTemplate *aitp )
{
	const DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;

	DBUGTMPL(("internalOpenInsTemplate: item=0x%x '%s' OpenCount=%d func=%d flags=0x%02x\n",
		aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name, aitp->aitp_Item.n_OpenCount,
		dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_Header.dhdr_Flags));

	return dtmp->dtmp_Header.dhdr_Flags & DHDR_F_SHARED
		? aitp->aitp_Item.n_Item
		: AF_ERR_BADITEM;
}

/*****************************************************************************
** AUDIO_TEMPLATE_NODE ir_Close method. Called by internalCloseAudioItem()
**
** Immediately expunge library template when usage count falls to 0.
*/
Err internalCloseInsTemplate( AudioInsTemplate *aitp )
{
	const DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;

	DBUGTMPL(("internalCloseInsTemplate: item=0x%x '%s' OpenCount=%d func=%d flags=0x%02x\n",
		aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name, aitp->aitp_Item.n_OpenCount,
		dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_Header.dhdr_Flags));

	if ((dtmp->dtmp_Header.dhdr_Flags & DHDR_F_SHARED) && (aitp->aitp_Item.n_OpenCount == 0)) {
		return SuperInternalDeleteItem (aitp->aitp_Item.n_Item);
	}

	return 0;
}


#if 0       /* { @@@ no longer used because dlnk_DynamicLinkTemplates "openness"
					 is now owned by audio daemon. just here for reference. */

/*****************************************************************************
** AUDIO_TEMPLATE_NODE ir_SetOwner method. Called by internalSetAudioItemOwner (@@@ disabled)
** Transfers ownership of dlnk_DynamicLinkTemplates "openness" to new task.
** The item still has n_Owner set to the old owner at this point.
*/
Err internalSetAudioTemplateOwner (AudioInsTemplate *aitp, Item newOwner)
{
	Item * const dlnklist = aitp->aitp_DynamicLinkTemplates;

		/* process DynamicLinkTemplates, if there are any */
	if (dlnklist) {
		Task * const oldOwnerp = LookupItem (aitp->aitp_Item.n_Owner);
		Task * const newOwnerp = LookupItem (newOwner);
		Item *dlnktmpl;
		Err errcode;

	  #if DEBUG_TemplateOwner
		PRT(("\ninternalSetAudioTemplateOwner() tmpl=0x%x '%s'\n  from task: 0x%x '%s'\n  to task:   0x%x '%s'\n",
			aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name,
			aitp->aitp_Item.n_Owner, oldOwnerp->t.n_Name,
			newOwner, newOwnerp->t.n_Name ));
		DumpResources (oldOwnerp);
		DumpResources (newOwnerp);
	  #endif

			/* open on behalf of new owner */
		for (dlnktmpl = dlnklist; *dlnktmpl > 0; dlnktmpl++) {

			if ((errcode = SuperInternalOpenItem (*dlnktmpl, NULL, newOwnerp)) < 0) {

					/* on failure, close all the things we tried to open */
				for (; --dlnktmpl >= dlnklist; ) {
					SuperInternalCloseItem (*dlnktmpl, newOwnerp);
				}

			  #if DEBUG_TemplateOwner
				PRT(("internalSetAudioTemplateOwner() failed with 0x%x\n", errcode));
			  #endif

				return errcode;
			}
		}

			/* close on behalf of old owner */
		for (dlnktmpl = dlnklist; *dlnktmpl > 0; dlnktmpl++) {
			SuperInternalCloseItem (*dlnktmpl, oldOwnerp);
		}

	  #if DEBUG_TemplateOwner
		PRT(("internalSetAudioTemplateOwner() success\n"));
		DumpResources (oldOwnerp);
		DumpResources (newOwnerp);
	  #endif
	}

	return 0;
}

#endif  /* } @@@ disabled */


/* -------------------- Share standard instrument's DSPPTemplate among several Instrument Templates */

/*
	Find and share DSPPTemplate. If it is sharable, bump its ShareCount.

	Arguments
		resultDTmp
			Buffer to store located DSPPTemplate pointer into.

		shareTemplateName
			Name of AudioInsTemplate to find which contains DSPPTemplate we
			wish to share. Name must be valid.

	Return Value
		0 on success, Err code on failure.

		Writes shared DSPPTemplate pointer to *resultDTmp on success. Writes NULL
		there on failure.

		Increments dtmp_ShareCount of shared DSPPTemplate on success.
*/
static Err dsppShareTemplate (DSPPTemplate **resultDTmp, const char *shareTemplateName)
{
	const AudioInsTemplate *masterAITP;
	DSPPTemplate *shareDTmp;

	DBUGTMPL(("dsppShareTemplate: '%s'\n", shareTemplateName));

		/* Look up Template Item with specified name */
	if (!(masterAITP = FindTemplateNode (shareTemplateName))) {
		DBUGTMPL(("dsppShareTemplate: master template not yet loaded.\n"));
		return AF_ERR_INTERNAL_NOT_SHAREABLE;   /* @@@ internal error code which tells LoadInsTemplate() that
													   template hasn't been loaded yet. */
	}

	DBUGTMPL(("dsppShareTemplate: found master template item 0x%x\n", masterAITP->aitp_Item.n_Item));

		/*
			See if we can share this DSPPTemplate. If we can't, someone tried to
			LoadInsTemplate() an instrument with the same name as a mixer or patch
			already in the system. Name trap during creation prevents making a
			mixer or patch with the name of a loadable DSP instrument. This just
			traps someone trying to LoadInsTemplate ("my.mixer"...), for example.
		*/
	shareDTmp = (DSPPTemplate *)masterAITP->aitp_DeviceTemplate;
	if (shareDTmp->dtmp_Header.dhdr_FunctionID < DFID_FIRST_LOADABLE) {
		DBUGTMPL(("dsppShareTemplate: invalid master template id=%d.\n", shareDTmp->dtmp_Header.dhdr_FunctionID));
		return AF_ERR_INTERNAL_NOT_SHAREABLE;
	}

		/* Success: bump share count, post result to caller's buffer */
	shareDTmp->dtmp_ShareCount++;
	*resultDTmp = shareDTmp;

	DBUGTMPL(("dsppShareTemplate: dtmp=0x%x function=%d dtmp->dtmp_ShareCount=%d\n", shareDTmp, shareDTmp->dtmp_Header.dhdr_FunctionID, shareDTmp->dtmp_ShareCount));

	return 0;
}

/*
	Unshare DSPPTemplate and delete if no longer in use. If ShareCount is
	0 on entry, DSPPTemplate is deleted. Otherwise, ShareCount is decremented.

	Arguments
		dtmp
			Supervisor DSPPTemplate to unshare and possibly delete. Can be NULL.

	!!! might roll together with dsppDeleteSuperTemplate()
*/
static void dsppUnshareTemplate (DSPPTemplate *dtmp)
{
	if (dtmp) {
		DBUGTMPL(("dsppUnshareTemplate: dtmp=0x%x function=%d dtmp->dtmp_ShareCount=%d\n", dtmp, dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_ShareCount));
		if (!dtmp->dtmp_ShareCount--) {
			dsppDeleteSuperTemplate (dtmp);
		}
	}
}


/* -------------------- Open/Close Dynamice Link Templates */

/*
	Open dynamic link templates named in dtmp_DynamicLinkNames. Allocates a
	0-terminated array of opened dynamic link template Items.

	Arguments
		resultDynamicLinkNames
			buffer to store resulting Item array

		dtmp
			Supervisor mode DSPPTemplate containing dtmp_DynamicLinkNames to open.

	Results
		0 on success, Err code on failure

		Writes resulting array pointer to *resultDynamicLinkTemplates on success.
		*resultDynamicLinkTemplates is set to NULL on failure. Also set to NULL
		when the dtmp contains no dynamic link names, which is not a failure.
*/
static Err OpenDynamicLinkTemplates (Item **resultDynamicLinkTemplates, const DSPPTemplate *dtmp)
{
	Err errcode;
	Item *dynamicLinkTemplates = NULL;

		/* init result */
	*resultDynamicLinkTemplates = NULL;

		/* process DynamicLinkNames */
	if (dtmp->dtmp_DynamicLinkNames) {
			/* allocate empty dynamicLinkTemplates (one for each DynamicLinkName + 0 terminator) */
		{
			const int32 numDynamicLinks = CountPackedStrings (dtmp->dtmp_DynamicLinkNames, dtmp->dtmp_DynamicLinkNamesSize);

			if (!(dynamicLinkTemplates =
				SuperAllocMem (sizeof (Item) * (numDynamicLinks + 1),
				MEMTYPE_TRACKSIZE | MEMTYPE_NORMAL | MEMTYPE_FILL)))
			{
				errcode = AF_ERR_NOMEM;
				goto clean;
			}
		}

			/* open library templates, fill out dynamicLinkTemplates */
		{
			const char *dlnkname = dtmp->dtmp_DynamicLinkNames;
			const char * const dlnkname_end = dtmp->dtmp_DynamicLinkNames + dtmp->dtmp_DynamicLinkNamesSize;
			Item *dlnktmpl = dynamicLinkTemplates;

			for (; dlnkname < dlnkname_end; dlnkname = NextPackedString(dlnkname), dlnktmpl++) {
				TagArg tags[] = {
					{ TAG_ITEM_NAME },
					TAG_END
				};
				tags[0].ta_Arg = (TagData)dlnkname;

				if ((errcode = *dlnktmpl = SuperInternalFindAndOpenItem (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE), tags, AB_FIELD(af_AudioDaemon))) < 0) goto clean;
			}
		}
	}

	*resultDynamicLinkTemplates = dynamicLinkTemplates;
	return 0;

clean:
	CloseDynamicLinkTemplates (dynamicLinkTemplates);
	return errcode;
}

/*
	Close dynamic link templates opened by OpenDynamicLinkTemplates()
	and free Item array.

	Arguments
		dynamicLinkTemplates
			0-terminated array of Opened Items. May be NULL. Processing
			terminates at first Item in list whose value is <= 0, which
			handles the partial success case from OpenDynamicLinkTemplates().
*/
static void CloseDynamicLinkTemplates (Item *dynamicLinkTemplates)
{
	if (dynamicLinkTemplates) {
		Item *dlnktmpl;

		for (dlnktmpl = dynamicLinkTemplates; *dlnktmpl > 0; dlnktmpl++) {
			SuperInternalCloseItem (*dlnktmpl, AB_FIELD(af_AudioDaemon));
		}
		SuperFreeMem (dynamicLinkTemplates, TRACKED_SIZE);
	}
}


/* -------------------- Load dynamic link template (AUDIO_TEMPLATE_NODE ir_Load method) */

static Item remoteLoadInsTemplate (const char *insName);

/*****************************************************************************
** AUDIO_TEMPLATE_NODE ir_Load method. Called by internalLoadAudioItem()
**
** Load dynamic library template when it hasn't yet been loaded.
*/
Item internalLoadSharedTemplate (TagArg *tags)
{
	ItemNode  inode;
	Item      result;

		/* get name of thing to load */
	memset (&inode,0,sizeof(inode));
	result = TagProcessorNoAlloc(&inode,tags,NULL,0);
	if (result < 0) return result;
	if (inode.n_Name == NULL) return AF_ERR_BAD_NAME;

		/* load it */
	DBUGTMPL(("internalLoadSharedTemplate: call CallAsItemServer(,%s,)\n", inode.n_Name ));
	{
		const uint32 oldPriv = CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED;

			/* Temporarily set privilege bit. */
		CURRENTTASK->t.n_ItemFlags |= ITEMNODE_PRIVILEGED;

		result = (Item) CallAsItemServer(remoteLoadInsTemplate, inode.n_Name, TRUE);

			/* Clear privilege if it was clear before. */
		if( oldPriv == 0 ) CURRENTTASK->t.n_ItemFlags &= ~ITEMNODE_PRIVILEGED;
	}

	DBUGTMPL(("internalLoadSharedTemplate: returns 0x%x\n", result ));
	return result;
}

/********************************************************************************
** Callback function for operator's Item Server thread. Used to load dynamic
** library templates.
*/
static Item remoteLoadInsTemplate (const char *insName)
{
	Item insTemplate = -1;
	int32 result;

	DBUGTMPL(("remoteLoadInsTemplate: %s\n", insName));

		/* open audio folio for item server (necessary to pass by CHECKAUDIOOPEN) */
	if ((result = OpenItem (AB_FIELD(af_AudioModule), NULL)) < 0) goto clean;

	DBUGTMPL(("remoteLoadInsTemplate: LoadInsTemplate(%s)\n", insName));

		/* load template */
	if ((result = insTemplate = LoadInsTemplate (insName, NULL)) < 0) goto clean;

		/*
			Trap attempt to get thru here with a non-library template. If we don't trap
			this here, the item will get created and remain in existence. only the
			ir_Close vector causes expunge, and that doesn't (and shouldn't) get called
			on open failure. So trap it here before it gets returned.
		*/
	{
		AudioInsTemplate * const aitp = LookupItem (insTemplate);   /* @@@ trusts that item returned is actually a template */
		DSPPTemplate * const dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;

		DBUGTMPL(("internalLoadSharedTemplate: item=0x%x '%s' OpenCount=%d func=%d flags=0x%02x\n",
			aitp->aitp_Item.n_Item, aitp->aitp_Item.n_Name, aitp->aitp_Item.n_OpenCount,
			dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_Header.dhdr_Flags));

		if (!(dtmp->dtmp_Header.dhdr_Flags & DHDR_F_SHARED)) {
			result = AF_ERR_BADITEM;
			goto clean;
		}
	}

		/* Give ownership to Audio Daemon */
	if ((result = SetItemOwner (insTemplate, AB_FIELD(af_AudioDaemonItem))) < 0) goto clean;

		/* success */
	result = insTemplate;

clean:
		/* clean up on failure */
	if (result < 0) {
		UnloadInsTemplate (insTemplate);
	}

	CloseItem (AB_FIELD(af_AudioModule));
	return result;
}


/* -------------------- Get Instrument / Template Resource Information */

static int32 AddAndMarkDLinkResources (const Item *dlnkitem, uint32 rsrcType);
static void UnmarkDLinks (const Item *dlnkitem);

/**************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name GetInstrumentResourceInfo
|||	Get audio resource usage information for an Instrument(@).
|||
|||	  Synopsis
|||
|||	    Err GetInstrumentResourceInfo (InstrumentResourceInfo *info,
|||	                                   uint32 infoSize, Item insOrTemplate,
|||	                                   uint32 rsrcType)
|||
|||	  Description
|||
|||	    This function returns information about the resource usage of the supplied
|||	    Instrument, or any Instrument created from the supplied Instrument Template.
|||	    The information is returned in an InstrumentResourceInfo structure.
|||
|||	    The InstrumentResourceInfo structure contains the following fields:
|||
|||	    rinfo_PerInstrument
|||	        Indicates the amount of the resource required for each instrument created
|||	        from the same template.
|||
|||	    rinfo_MaxOverhead
|||	        Indicates the worst case overhead shared among all instruments from the
|||	        same template. This typically represents the size of shared code or data
|||	        used by the instrument.
|||
|||	        A shared resource is allocated when the first instrument which uses it
|||	        is created, and freed when the last instrument which uses it is deleted.
|||	        Many different templates can share the same shared resource. Because of
|||	        this, less than this amount may actually be required to allocate the
|||	        first instrument from this template.
|||
|||	    The total amount of a resource required by all instruments from a template
|||	    (worst case) is computed with the formula:
|||
|||	    rinfo_MaxOverhead + rinfo_PerInstrument * <number of instruments>
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to an InstrumentResourceInfo structure where the information
|||	        will be stored.
|||
|||	    infoSize
|||	        The size in bytes of the InstrumentResourceInfo structure.
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	    rsrcType
|||	        The resource type to query. This must be one of the AF_RESOURCE_TYPE_
|||	        values defined in <audio/audio.h>. See below.
|||
|||	  Resource Types
|||
|||	    AF_RESOURCE_TYPE_TICKS
|||	        Number of DSP ticks / frame that instrument uses.
|||
|||	    AF_RESOURCE_TYPE_CODE_MEM
|||	        Words of DSP code memory that instrument uses.
|||
|||	    AF_RESOURCE_TYPE_DATA_MEM
|||	        Words of DSP data memory that instrument uses.
|||
|||	    AF_RESOURCE_TYPE_FIFOS
|||	        Number of FIFOs (input and output counted together) that instrument
|||	        uses.
|||
|||	    AF_RESOURCE_TYPE_TRIGGERS
|||	        Number of triggers that instrument uses.
|||
|||	  Return Value
|||
|||	    Non-negative if successful or an error code (a negative value) if an error
|||	    occurs. On failure, the contents of info are left unchanged.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Caveats
|||
|||	    Tick amounts returned by this function are in ticks per frame regardless of
|||	    the instrument's AF_TAG_CALCRATE_DIVIDE setting. Tick amounts returned by
|||	    GetAudioResourceInfo() are in ticks per batch (8 frame set). So an
|||	    instrument created with full rate execution will reduce the available ticks
|||	    reported by GetAudioResourceInfo() by eight times the amount returned by
|||	    GetInstrumentResourceInfo().
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    DumpInstrumentResourceInfo(), insinfo(@), GetAudioResourceInfo()
**/
Err GetInstrumentResourceInfo (InstrumentResourceInfo *resultInfo, uint32 resultInfoSize, Item insOrTemplate, uint32 rsrcType)
{
	const AudioInsTemplate * const aitp = LookupAudioInsTemplate (insOrTemplate);
	InstrumentResourceInfo info;
	Err errcode;

		/* init info */
	memset (&info, 0, sizeof info);

		/* validate template */
	if (!aitp) return AF_ERR_BADITEM;

#ifdef BUILD_PARANOIA
		/* validate resource type */
	if (rsrcType > AF_RESOURCE_TYPE_MAX) return AF_ERR_BADRSRCTYPE;
#endif

		/* get total DynamicLinkTemplates overhead */
		/* @@@ assumes that rinfo_MaxOverhead is an int32 */
		/* @@@ passing Item number to GetDynamicLinkResourceUsage() for security reasons */
	if ((errcode = info.rinfo_MaxOverhead = GetDynamicLinkResourceUsage (aitp->aitp_Item.n_Item, rsrcType)) < 0) return errcode;

		/* get per-instrument usage */
		/* !!! include contribution to code size by possible connection moves? */
	info.rinfo_PerInstrument = dsppGetTemplateResourceUsage (aitp->aitp_DeviceTemplate, rsrcType);

		/* success: copy to client's buffer */
	memset (resultInfo, 0, resultInfoSize);
	memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));

	return 0;
}

/*
	Recursively total subroutine resource usage. This is a SWI so that it has
	the privelege to mark templates as it progresses, and to ensure that it
	runs atomically.

	This is an internal SWI, but it has sufficient security to reduce hackability
	(e.g., it is passed an Item rather than a pointer).

	Arguments
		insTemplate
			Instrument Template Item to total subroutines for. aitp_DynamicLinkTemplates
			may be NULL, in which case this function returns 0.

		rsrcType
			AF_RESOURCE_TYPE_ to query.

	Results
		>= 0 on success, < 0 on error.
*/
int32 swiGetDynamicLinkResourceUsage (Item insTemplate, uint32 rsrcType)
{
	const AudioInsTemplate * const aitp = CheckItem (insTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
	int32 rsrcUsage = 0;

	if (!aitp) return AF_ERR_BADITEM;

	if (aitp->aitp_DynamicLinkTemplates) {
		rsrcUsage = AddAndMarkDLinkResources (aitp->aitp_DynamicLinkTemplates, rsrcType);
		UnmarkDLinks (aitp->aitp_DynamicLinkTemplates);
	}

	return rsrcUsage;
}

/*
	Recursively add resources for subroutines, marking as it goes to avoid
	counting any single subroutine twice.

	Arguments
		dlnkitem
			0 terminated array of Items. Must not be NULL.

		rsrcType
			AF_RESOURCE_TYPE_ to query.

	Results
		Total resource usage (>=0).

	@@@ This function ignores errors. A template had better not contain bogus
		DynamicLinkTemplates or we're hosed anyway.
*/
static int32 AddAndMarkDLinkResources (const Item *dlnkitem, uint32 rsrcType)
{
	int32 rsrcUsage = 0;

	for (; *dlnkitem; dlnkitem++) {
		AudioInsTemplate * const aitp = LookupItem (*dlnkitem);

			/* if item isn't found (oh, well) or already marked, skip it */
		if (aitp && !(aitp->aitp_Item.n_Flags & AF_NODE_F_MARK)) {
			aitp->aitp_Item.n_Flags |= AF_NODE_F_MARK;

				/* add this template's resources */
			rsrcUsage += dsppGetTemplateResourceUsage (aitp->aitp_DeviceTemplate, rsrcType);

				/* add resource usage of any subroutines of this one */
			if (aitp->aitp_DynamicLinkTemplates) {
				rsrcUsage += AddAndMarkDLinkResources (aitp->aitp_DynamicLinkTemplates, rsrcType);
			}
		}
	}

	return rsrcUsage;
}

/*
	Recursively unmark templates marked by AddAndMarkDLinkResources()

	Arguments
		dlnkitem
			0 terminated array of Items. Must not be NULL.

	@@@ This function ignores errors. A template had better not contain bogus
		DynamicLinkTemplates or we're hosed anyway.
*/
static void UnmarkDLinks (const Item *dlnkitem)
{
	for (; *dlnkitem; dlnkitem++) {
		AudioInsTemplate * const aitp = LookupItem (*dlnkitem);

			/* if item isn't found (oh, well) skip it */
		if (aitp) {
			aitp->aitp_Item.n_Flags &= ~AF_NODE_F_MARK;
			if (aitp->aitp_DynamicLinkTemplates) UnmarkDLinks (aitp->aitp_DynamicLinkTemplates);
		}
	}
}


/* -------------------- Get Instrument / Template Port Information */

/**************************************************************/
/**
|||	AUTODOC -public -class audio -group Instrument -name GetNumInstrumentPorts
|||	Returns the number of ports of an Instrument(@).
|||
|||	  Synopsis
|||
|||	    int32 GetNumInstrumentPorts (Item insOrTemplate)
|||
|||	  Description
|||
|||	    This function returns the number of ports of an Instrument or instrument
|||	    Template. This function in combination with GetInstrumentPortInfoByIndex()
|||	    permits one to walk the port list of an instrument.
|||
|||	  Arguments
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	  Return Value
|||
|||	    Non-negative value indicating the number of ports belonging to the
|||	    instrument, or an error code (a negative value) if an error occurs.
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
|||	    GetInstrumentPortInfoByIndex(), GetInstrumentResourceInfo()
**/
int32 GetNumInstrumentPorts (Item insOrTemplate)
{
	const AudioInsTemplate *aitp;
	const DSPPTemplate *dtmp;
	int32 numPorts = 0;
	int32 rsrcIndex;
	DSPPEnvHookRsrcInfo deri;

		/* look up template */
	if (!(aitp = LookupAudioInsTemplate (insOrTemplate))) return AF_ERR_BADITEM;
	dtmp = aitp->aitp_DeviceTemplate;

		/* scan resources for ports */
	for (rsrcIndex=0; rsrcIndex<dtmp->dtmp_NumResources; rsrcIndex++) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];

			/* @@@ order of matching envelopes and ports must match GetInstrumentPortInfoByIndex() */
		if (dsppIsEnvPortRsrc(&deri,dtmp,rsrcIndex)) numPorts++;
		if (dsppIsPortRsrc(drsc)) numPorts++;
	}

		/* success: return number of ports found */
	return numPorts;
}

/**
|||	AUTODOC -public -class audio -group Instrument -name GetInstrumentPortInfoByIndex
|||	Looks up Instrument(@) port by index and returns port information.
|||
|||	  Synopsis
|||
|||	    Err GetInstrumentPortInfoByIndex (InstrumentPortInfo *info, uint32 infoSize,
|||	                                      Item insOrTemplate, uint32 portIndex)
|||
|||	  Description
|||
|||	    This function looks up an instrument port by index and returns information
|||	    about the port. The information is returned in an InstrumentPortInfo
|||	    structure.
|||
|||	    The InstrumentPortInfo structure contains the following fields:
|||
|||	    pinfo_Name
|||	        Name of the port.
|||
|||	    pinfo_Type
|||	        Port type of the port (a AF_PORT_TYPE_ value).
|||
|||	    pinfo_SignalType
|||	        Signal type of the port (AF_SIGNAL_TYPE_). This only applies to the
|||	        following port types: AF_PORT_TYPE_INPUT, AF_PORT_TYPE_OUTPUT,
|||	        AF_PORT_TYPE_KNOB, and AF_PORT_TYPE_ENVELOPE.
|||
|||	    pinfo_NumParts
|||	        Number of parts that the port has. This is in the range of 1..N. Part
|||	        numbers used to refer to this port are in the range of 0..N-1. This only
|||	        applies to the following port types: AF_PORT_TYPE_INPUT,
|||	        AF_PORT_TYPE_OUTPUT, and AF_PORT_TYPE_KNOB.
|||
|||	    This function in combination with GetNumInstrumentPorts() permits one to
|||	    walk the port list of an instrument.
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to an InstrumentPortInfo structure where the information
|||	        will be stored.
|||
|||	    infoSize
|||	        The size in bytes of the InstrumentPortInfo structure.
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	    portIndex
|||	        Index of the port in the range of 0..numPorts-1, where numPorts is
|||	        returned by GetNumInstrumentPorts().
|||
|||	  Return Value
|||
|||	    Non-negative if successful or an error code (a negative value) if an error
|||	    occurs. On failure, the contents of info are left unchanged.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Example
|||
|||	    // display instrument port info
|||	    void DumpInstrumentPortInfo (Item insOrTemplate)
|||	    {
|||	        InstrumentPortInfo info;
|||	        const int32 numPorts = GetNumInstrumentPorts (insOrTemplate);
|||	        int32 i;
|||
|||	        printf ("Instrument 0x%x has %d port(s)\n", insOrTemplate, numPorts);
|||	        for (i=0; i<numPorts; i++) {
|||	            if (GetInstrumentPortInfoByIndex (&info, sizeof info, insOrTemplate, i) >= 0) {
|||	                printf ("%2d: '%s' port type=%d parts=%d signal type=%d\n",
|||	                    i, info.pinfo_Name, info.pinfo_Type, info.pinfo_NumParts,
|||	                    info.pinfo_SignalType);
|||	            }
|||	        }
|||	    }
|||
|||	  Caveats
|||
|||	    Because envelope hooks also can be also be treated as knobs, an
|||	    AF_PORT_TYPE_ENVELOPE port will also have an associated pair of
|||	    AF_PORT_TYPE_KNOBs. For example, envelope.dsp(@) has the following
|||	    envelope related ports:
|||
|||	    Env - AF_PORT_TYPE_ENVELOPE
|||
|||	    Env.incr - AF_PORT_TYPE_KNOB
|||
|||	    Env.request - AF_PORT_TYPE_KNOB
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    GetInstrumentPortInfoByName(), GetNumInstrumentPorts(), GetAudioSignalInfo(),
|||	    insinfo(@)
**/
Err GetInstrumentPortInfoByIndex (InstrumentPortInfo *info, uint32 infoSize, Item insOrTemplate, uint32 portIndex)
{
	const AudioInsTemplate *aitp;
	const DSPPTemplate *dtmp;
	int32 rsrcIndex;
	DSPPEnvHookRsrcInfo deri;

		/* look up template */
	if (!(aitp = LookupAudioInsTemplate (insOrTemplate))) return AF_ERR_BADITEM;
	dtmp = aitp->aitp_DeviceTemplate;

		/* skip resources to get to resource for portIndex */
	for (rsrcIndex=0; rsrcIndex<dtmp->dtmp_NumResources; rsrcIndex++) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];

			/* @@@ order of matching envelopes and ports must match GetNumInstrumentPorts() */
		if (dsppIsEnvPortRsrc(&deri,dtmp,rsrcIndex) && !portIndex--) {
				/* got the right envelope hook: get info */
			dsppGetInstrumentEnvPortInfo (info, infoSize, dtmp, &deri);
			return 0;
		}
		if (dsppIsPortRsrc(drsc) && !portIndex--) {
				/* got the right port: get info */
			dsppGetInstrumentPortInfo (info, infoSize, dtmp, rsrcIndex);
			return 0;
		}
	}

		/* didn't find it: index must have been out of range */
	return AF_ERR_OUTOFRANGE;
}

/**
|||	AUTODOC -public -class audio -group Instrument -name GetInstrumentPortInfoByName
|||	Looks up Instrument(@) port by name and returns port information.
|||
|||	  Synopsis
|||
|||	    GetInstrumentPortInfoByName (InstrumentPortInfo *info, uint32 infoSize,
|||	                                 Item insOrTemplate, const char *portName)
|||
|||	  Description
|||
|||	    This function looks up an instrument port by name and returns information
|||	    about the port. The information is returned in an InstrumentPortInfo
|||	    structure.
|||
|||	    The InstrumentPortInfo structure contains the following fields:
|||
|||	    pinfo_Name
|||	        Name of the port.
|||
|||	    pinfo_Type
|||	        Port type of the port (a AF_PORT_TYPE_ value).
|||
|||	    pinfo_SignalType
|||	        Signal type of the port (AF_SIGNAL_TYPE_). This only applies to the
|||	        following port types: AF_PORT_TYPE_INPUT, AF_PORT_TYPE_OUTPUT,
|||	        AF_PORT_TYPE_KNOB, and AF_PORT_TYPE_ENVELOPE.
|||
|||	    pinfo_NumParts
|||	        Number of parts that the port has. This is in the range of 1..N. Part
|||	        numbers used to refer to this port are in the range of 0..N-1. This only
|||	        applies to the following port types: AF_PORT_TYPE_INPUT,
|||	        AF_PORT_TYPE_OUTPUT, and AF_PORT_TYPE_KNOB.
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to an InstrumentPortInfo structure where the information
|||	        will be stored.
|||
|||	    infoSize
|||	        The size in bytes of the InstrumentPortInfo structure.
|||
|||	    insOrTemplate
|||	        Instrument(@) or Instrument Template(@) Item to query.
|||
|||	    portName
|||	        Name of the port to query. Port names are matched case-insensitively.
|||
|||	  Return Value
|||
|||	    Non-negative if successful or an error code (a negative value) if an error
|||	    occurs. On failure, the contents of info are left unchanged.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Caveats
|||
|||	    Because envelope hooks also can be also be treated as knobs, an
|||	    AF_PORT_TYPE_ENVELOPE port will also have an associated pair of
|||	    AF_PORT_TYPE_KNOBs. For example, envelope.dsp(@) has the following
|||	    envelope related ports:
|||
|||	    Env - AF_PORT_TYPE_ENVELOPE
|||
|||	    Env.incr - AF_PORT_TYPE_KNOB
|||
|||	    Env.request - AF_PORT_TYPE_KNOB
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    GetInstrumentPortInfoByIndex(), GetNumInstrumentPorts(), GetAudioSignalInfo(),
|||	    insinfo(@)
**/
Err GetInstrumentPortInfoByName (InstrumentPortInfo *info, uint32 infoSize, Item insOrTemplate, const char *portName)
{
	const AudioInsTemplate *aitp;
	const DSPPTemplate *dtmp;
	int32 rsrcIndex;
	DSPPEnvHookRsrcInfo deri;

		/* look up template */
	if (!(aitp = LookupAudioInsTemplate (insOrTemplate))) return AF_ERR_BADITEM;
	dtmp = aitp->aitp_DeviceTemplate;

		/* find and check resource */
	if ((rsrcIndex = DSPPFindResourceIndex (dtmp, portName)) >= 0) {
		const DSPPResource * const drsc = &dtmp->dtmp_Resources[rsrcIndex];

		if (!dsppIsPortRsrc(drsc)) return AF_ERR_NAME_NOT_FOUND;
		dsppGetInstrumentPortInfo (info, infoSize, dtmp, rsrcIndex);
	}
	else if (dsppFindEnvHookResources (&deri, sizeof deri, dtmp, portName) >= 0) {
		dsppGetInstrumentEnvPortInfo (info, infoSize, dtmp, &deri);
	}
	else {
		return AF_ERR_NAME_NOT_FOUND;
	}

	return 0;
}


/* -------------------- Internal Template Lookup */

/**************************************************************/
/*
	Lookup AudioInsTemplate associated with Instrument or Template Item.

	Arguments
		insOrTemplate
			Instrument or Instrument Template Item number to look up.

	Results
		Pointer to AudioInsTemplate if found. NULL if bad item.
*/
AudioInsTemplate *LookupAudioInsTemplate (Item insOrTemplate)
{
	const ItemNode * const n = LookupItem (insOrTemplate);

	if (!n || n->n_SubsysType != AUDIONODE) return NULL;

	switch (n->n_Type) {
		case AUDIO_INSTRUMENT_NODE:
			{
				const AudioInstrument * const ains = (AudioInstrument *)n;

				return ains->ains_Template;
			}
			break;

		case AUDIO_TEMPLATE_NODE:
			return (AudioInsTemplate *)n;

		default:
			return NULL;
	}
}

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppLookupTemplate
|||	Lookup Template Item and return DSPPTemplate.
|||
|||	  Synopsis
|||
|||	    DSPPTemplate *dsppLookupTemplate (Item insTemplate)
|||
|||	  Description
|||
|||	    Looks up the specified template item and, if found, returns the associated
|||	    DSPPTemplate pointer.
|||
|||	  Arguments
|||
|||	    insTemplate
|||	        The item number for the Template.
|||
|||	  Return Value
|||
|||	    Pointer to DSPPTemplate on success, NULL if not found or not a template.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
**/
DSPPTemplate *dsppLookupTemplate (Item insTemplate)
{
	const AudioInsTemplate * const aitp = (AudioInsTemplate *)CheckItem (insTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);

	return aitp
		? aitp->aitp_DeviceTemplate
		: NULL;
}


#if DEBUG_TemplateOwner

/* -------------------- debug support */

static void DumpResources (const Task *task)
{
int   i;
Item *p = task->t_ResourceTable;

	printf ("task 0x%x '%s' templates:\n", task->t.n_Item, task->t.n_Name);

	for (i = 0; i < task->t_ResourceCnt; i++)
	{
		if (*p >= 0)
		{
			const AudioInsTemplate * const aitp = CheckItem (*p & ~ITEM_WAS_OPENED, AUDIONODE, AUDIO_TEMPLATE_NODE);

			if (aitp) {
				if (*p & ITEM_WAS_OPENED)
				{
					printf("  (0x%08x) ",*p & ~ITEM_WAS_OPENED);
				}
				else
				{
					printf("   0x%08x  ",*p);
				}
				printf ("%2d '%s'\n", aitp->aitp_Item.n_OpenCount, aitp->aitp_Item.n_Name);
			}
		}
		p++;
	}
}

#endif
