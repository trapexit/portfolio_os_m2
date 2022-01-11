/******************************************************************************
**
**  @(#) createpatch.c 96/07/15 1.39
**
**  DSP patch intstrument template builder - main.
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
**  950717 WJB  Created.
**  950718 WJB  Moved CreatePatchTemplate() to audio_instr.c.
**  950718 WJB  Moved DumpPatchData() here from createpatch_parser.c.
**  950718 WJB  Added call to dsppLinkPatchTemplate().
**              Added dsppDumpTemplate() (which will probably move elsewhere).
**  950724 WJB  Moved dsppDump...() functions to dspp_debug.h.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/


#include <audio/audio.h>
#include <audio/patch.h>
#include <audio/dspp_template.h>

#include "createpatch_internal.h"


/* -------------------- Debug */

#define DEBUG_DumpPatchCmdList  0   /* enable dumping PatchCmdList before parsing */
#define DEBUG_DumpPatchData     0   /* enable dumping contents of PatchData after construction */
#define DEBUG_DumpPatchTemplate 0   /* enable dumping contents of resulting DSPPTemplate */

#if DEBUG_DumpPatchCmdList || DEBUG_DumpPatchData || DEBUG_DumpPatchTemplate
#include <stdio.h>
#endif


/* -------------------- Local Functions */

    /* debug */
#if DEBUG_DumpPatchData
    static void DumpPatchData (const PatchData *);
#endif


/* -------------------- CreatePatchTemplate() */

static Err dsppCreatePatchTemplate (DSPPTemplate **resultUserDTmp, const PatchCmd *patchCmdList);

/**
|||	AUTODOC -public -class AudioPatch -name CreatePatchTemplate
|||	Constructs a custom Patch Template(@) from simple Instrument Templates.
|||
|||	  Synopsis
|||
|||	    Item CreatePatchTemplate (const PatchCmd *patchCmdList, const TagArg *tagList)
|||
|||	    Item CreatePatchTemplateVA (const PatchCmd *patchCmdList, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This function creates a Patch Template from a list of PatchCmd(@)s. Patches
|||	    may consist of zero or more Instrument Templates, zero or more defined ports
|||	    and knobs, and optional internal connections between these.
|||
|||	    The PatchCmd parser makes several passes through the PatchCmd list, one per
|||	    class defined below. Within each class, PatchCmds are acted upon in order
|||	    of appearance. The PatchCmd class order is:
|||
|||	    1. PATCH_CMD_SET_COHERENCE(@)
|||
|||	    2. PATCH_CMD_ADD_TEMPLATE(@)
|||
|||	    3. PATCH_CMD_DEFINE_PORT(@) and PATCH_CMD_DEFINE_KNOB(@)
|||
|||	    4. PATCH_CMD_EXPOSE(@)
|||
|||	    5. PATCH_CMD_CONNECT(@) and PATCH_CMD_SET_CONSTANT(@)
|||
|||	    Because of the class-based parsing, some order independence is offered in
|||	    constructing a PatchCmd list. For example, it makes no difference whether
|||	    things that are to be connected (PATCH_CMD_ADD_TEMPLATE(@),
|||	    PATCH_CMD_DEFINE_PORT(@), or PATCH_CMD_DEFINE_KNOB(@)) are defined before a
|||	    connection PATCH_CMD_CONNECT(@)) is defined.
|||
|||	    The resource requirements for the compiled patch are typically a little
|||	    less than the sum of the resource requirements of the constituent templates
|||	    plus data memory for the patch's outputs and knobs (one word each). If
|||	    PATCH_CMD_SET_COHERENCE(@) is set to FALSE, the instrument code is packed
|||	    together, thus shrinking the code by a couple of words for each constituent
|||	    instrument.
|||
|||	    Call DeletePatchTemplate() when done with this Template.
|||
|||	  Arguments
|||
|||	    patchCmdList
|||	        Pointer to a PatchCmd(@) list from which to create a patch.
|||
|||	  Tag Arguments
|||
|||	    TAG_ITEM_NAME (const char *)
|||	        Optional name for the newly created patch. Defaults to not having a
|||	        name.
|||
|||	  Return Value
|||
|||	    The procedure returns an instrument Template Item number (a non-negative
|||	    value) if successful or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in AudioPatch Folio V30.
|||
|||	  Caveats
|||
|||	    Items attached to any of the constituent Template Items of a patch (i.e.
|||	    Samples, Envelopes, or Tunings), are not propagated to the Patch Template.
|||	    If you wish to do this, you must Attach these things to the compiled Patch
|||	    Template yourself. This is a bit of a gotcha when it comes to including
|||	    previously defined Patch Templates in another Patch, because it requires you
|||	    to know what, if anything, is attached to this Patch Template. (there are
|||	    audio folio services to help you identify these however: see the
|||	    documentation for GetAttachments() and the Attachment(@) tags)
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio(), OpenAudioPatchFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, System.m2/Modules/audio, System.m2/Modules/audiopatch
|||
|||	  See Also
|||
|||	    PatchCmd(@), CreatePatchCmdBuilder(), Template(@), DeletePatchTemplate(),
|||	    LoadInsTemplate(), CreateMixerTemplate(), CreateInstrument(),
|||	    LoadPatchTemplate(), makepatch(@)
**/
Item CreatePatchTemplate (const PatchCmd *patchCmdList, const TagArg *tagList)
{
    DSPPTemplate *userdtmp;
    Item result;

    if ((result = dsppCreatePatchTemplate (&userdtmp, patchCmdList)) >= 0) {
        result = CreateItemVA (MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE),
                        AF_TAG_TEMPLATE,    userdtmp,
                        TAG_JUMP,           tagList);
        dsppDeleteUserTemplate (userdtmp);
    }

    return result;
}

/*
    Create DSPPTemplate patch from PatchCmd list.

    The successful results of this function can be passed to CreateItem() AF_TAG_TEMPLATE
    in order to create the supervisor DSPPTemplate object. After creating the supervisor
    template, call dsppDeleteUserTemplate() to dispose of the user DSPPTemplate.

    Inputs
        resultUserDTmp
            DSPPTemplate pointer to fill out.

        patchCmdList
            PatchCmd list to parse. _Can_ be NULL, but that doesn't construct
            a very useful patch.

    Results
        Success:
            returns 0, stores pointer to allocated DSPPTemplate in *resultUserDTmp.

        Failure
            returns Err code, stores NULL in *resultUserDTmp.
*/
static Err dsppCreatePatchTemplate (DSPPTemplate **resultUserDTmp, const PatchCmd *patchCmdList)
{
    PatchData *pd = NULL;
    Err errcode;

  #if DEBUG_DumpPatchCmdList
    {
        char b[80];

        sprintf (b, "\ndsppCreatePatchTemplate: patchCmdList=0x%x", patchCmdList);
        DumpPatchCmdList (patchCmdList, b);
    }
  #endif

        /* init result */
    *resultUserDTmp = NULL;

        /* parse tags, return PatchData */
    if ((errcode = dsppParsePatchCmds (&pd, patchCmdList)) < 0) goto clean;
  #if DEBUG_DumpPatchData
    DumpPatchData (pd);
  #endif

        /* build resulting dtmp */
    if ((errcode = dsppLinkPatchTemplate (resultUserDTmp, pd)) < 0) goto clean;
  #if DEBUG_DumpPatchTemplate
    {
        char b[80];

        sprintf (b, "dsppCreatePatchTemplate() dtmp=0x%x", *resultUserDTmp);
        dsppDumpTemplate (*resultUserDTmp, b);
    }
  #endif

clean:
    /* @@@ not cleaning up resultUserDTmp on failure because dsppLinkPatchTemplate()
           cleans up after itself and sets it to NULL on failure */
    dsppDeletePatchData (pd);       /* dispose of PatchData */

    return errcode;
}


/**
|||	AUTODOC -public -class AudioPatch -name DeletePatchTemplate
|||	Deletes a custom Instrument Template created by CreatePatchTemplate()
|||
|||	  Synopsis
|||
|||	    Err DeletePatchTemplate (Item patchTemplate)
|||
|||	  Description
|||
|||	    This function deletes a Patch Template created by CreatePatchTemplate().
|||	    This has the same side effects as UnloadInsTemplate() (e.g., deleting
|||	    instruments, attachments, slaves of AF_TAG_AUTO_DELETE_SLAVE attachments,
|||	    etc.)
|||
|||	  Arguments
|||
|||	    patchTemplate
|||	        Patch Template Item to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns an non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/patch.h> V27.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@)
**/


/* -------------------- Debug */

#if DEBUG_DumpPatchData /* { */

static void DumpPatchTemplateInfo (const PatchTemplateInfo *);

static void DumpPatchData (const PatchData *pd)
{
    printf ("\nDumpPatchData() pd=0x%x flags=0x%02x\n", pd, pd->pd_Flags);

    if (pd->pd_PortTemplate) {
        DumpPatchTemplateInfo (pd->pd_PortTemplate);
    }
    {
        PatchTemplateInfo *pti;

        SCANLIST (&pd->pd_TemplateList, pti, PatchTemplateInfo) {
            DumpPatchTemplateInfo (pti);
        }
    }
    if (!IsListEmpty (&pd->pd_ResourceNameList)) {
        PatchResourceNameInfo *pni;

        printf ("\nExposed names:\n");
        SCANLIST (&pd->pd_ResourceNameList, pni, PatchResourceNameInfo) {
            printf ("  flags=0x%02x '%s'\n", pni->pni.n_Flags, pni->pni.n_Name);
        }
    }

    printf ("\n");
}

static void DumpPatchTemplateInfo (const PatchTemplateInfo *pti)
{
    const DSPPTemplate * const dtmp = pti->pti_DTmp;

    if (pti->pti.n_Type == PATCH_TEMPLATE_TYPE_PORT) {
        printf ("\nPort Template:\n");
    }
    else {
        printf ("\nInstrument Template '%s':\n", pti->pti.n_Name);
        printf ("  FunctionID=%d Silicon=%d\n", dtmp->dtmp_Header.dhdr_FunctionID, dtmp->dtmp_Header.dhdr_SiliconVersion);
    }
    printf ("  NumMoves=%u\n", pti->pti_NumMoves);

    if (pti->pti_NumResources) {
        int32 rsrcIndex;

        printf ("Resources: (%u)\n", pti->pti_NumResources);

            /* resource table */
        for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
            const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];
            static const char * const ConnectabilityDesc[] = {  /* @@@ indexed by PATCH_RESOURCE_CONNECTABLE_ */
                "",
                "Src",
                "Dest",
            };

            dsppDumpResource (dtmp, rsrcIndex);
                  /* nnnn: */
            printf ("      %-4s pri_Flags=0x%02x dini=0x%08x TargetRsrcName='%s'\n",
                ConnectabilityDesc[pri->pri_Connectability], pri->pri_Flags, pri->pri_DataInitializer, pri->pri_TargetRsrcName);
        }

            /* data initializers */
        {
            bool aredinis = FALSE;

            for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
                const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

                if (pri->pri_DataInitializer) {
                    aredinis = TRUE;
                    break;
                }
            }

            if (aredinis) {
                printf ("Data Initializers:\n");
                for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
                    const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];

                    if (pri->pri_DataInitializer) dsppDumpDataInitializer (pri->pri_DataInitializer);
                }
            }
        }

            /* dest info */
        for (rsrcIndex=0; rsrcIndex<pti->pti_NumResources; rsrcIndex++) {
            const PatchResourceInfo * const pri = &pti->pti_ResourceInfo[rsrcIndex];
            const DSPPResource * const drsc = pri->pri_DRsc;
            int32 partIndex;

            if (pri->pri_Connectability == PATCH_RESOURCE_CONNECTABLE_DEST) {
                printf ("Connection Destination '%s' (%d part%s):\n", dsppGetTemplateRsrcName(dtmp,rsrcIndex), drsc->drsc_Many, drsc->drsc_Many==1 ? "" : "s");
                for (partIndex = 0; partIndex < drsc->drsc_Many; partIndex++) {
                    const PatchDestInfo * const pdi = &pri->pri_DestInfo[partIndex];

                    printf ("%4d: ", partIndex);
                    switch (pdi->pdi_State) {
                        case PATCH_DEST_STATE_UNUSED:
                            printf ("unused\n");
                            break;

                        case PATCH_DEST_STATE_CONNECTED:
                            printf ("connect: block '%s', port '%s', part %u\n",
                                pdi->pdi.ConnectInfo.SrcTemplate->pti.n_Name,
                                dsppGetTemplateRsrcName (pdi->pdi.ConnectInfo.SrcTemplate->pti_DTmp, pdi->pdi.ConnectInfo.SrcRsrcIndex),
                                pdi->pdi.ConnectInfo.SrcPartNum);
                            break;

                        case PATCH_DEST_STATE_CONSTANT:
                            printf ("constant: %6d %9.6f\n", pdi->pdi.ConstInfo.Value, ConvertF15_FP(pdi->pdi.ConstInfo.Value));
                            break;

                        default:
                            printf ("confused state=%d\n", pdi->pdi_State);
                            break;
                    }
                }
            }
        }
    }
}

#endif  /* } DEBUG_DumpPatchData */
