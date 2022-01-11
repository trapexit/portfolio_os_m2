/* @(#) addtemplatetopatch.c 96/02/28 1.4 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name AddTemplateToPatch
|||	Adds a PATCH_CMD_ADD_TEMPLATE(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err AddTemplateToPatch (PatchCmdBuilder *builder, const char *blockName, Item insTemplate)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_ADD_TEMPLATE(@) PatchCmd to list under construction. This will
|||	    add an Instrument Template Item with the specified name. Any Instrument
|||	    Template may be added to the patch: standard DSP instruments, patches, or
|||	    mixers. Multiple instances of the same Instrument Template Item may be added
|||	    to any patch. Each template must be given a unique block name within the patch.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    blockName
|||	        Block name for template being added. Each block in a patch must have a
|||	        unique name. Names are compared case-insensitively. Internal connections
|||	        are specified using this block name. This string must remain valid for
|||	        the life of the PatchCmdBuilder.
|||
|||	    insTemplate
|||	        Instrument Template Item to use for this block. May be a standard DSP
|||	        instrument template, a mixer, or a previously created patch. This Item
|||	        must remain valid for the life of the PatchCmdBuilder.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, negative error code on failure.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Notes
|||
|||	    To minimize propagation delay across a patch, instruments should be added
|||	    in the order of signal flow.
|||
|||	  Caveats
|||
|||	    Any Items attached to insTemplate (i.e. Samples, Envelopes, or Tunings) are
|||	    not propagated to the compiled patch.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), PATCH_CMD_ADD_TEMPLATE(@), CreatePatchCmdBuilder(),
|||	    ExposePatchPort(), ConnectPatchPorts(), SetPatchConstant(),
|||	    SetPatchCoherence()
**/

Err AddTemplateToPatch (PatchCmdBuilder *builder, const char *blockName, Item insTemplate)
{
    PatchCmdAddTemplate newcmd = { PATCH_CMD_ADD_TEMPLATE };

    newcmd.pc_BlockName   = blockName;
    newcmd.pc_InsTemplate = insTemplate;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
