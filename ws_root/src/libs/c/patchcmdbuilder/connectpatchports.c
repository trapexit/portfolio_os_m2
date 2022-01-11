/* @(#) connectpatchports.c 96/02/29 1.5 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name ConnectPatchPorts
|||	Adds a PATCH_CMD_CONNECT(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err ConnectPatchPorts (PatchCmdBuilder *builder,
|||	                           const char *fromBlockName,
|||	                           const char *fromPortName,
|||	                           uint32 fromPartNum,
|||	                           const char *toBlockName,
|||	                           const char *toPortName,
|||	                           uint32 toPartNum)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_CONNECT(@) PatchCmd to list under construction. This will
|||	    create internal signal connections between patch inputs, patch knobs, patch
|||	    outputs, block inputs, block knobs, and block outputs.
|||
|||	    Any block input or block knob can have up to one connection or constant.
|||	    Unused block inputs default to 0; unused block knobs default to their
|||	    default value. All patch outputs must be connected.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    fromBlockName
|||	        Name of a patch block containing an output to connect from. Or NULL to
|||	        indicate that the source of the connection is a patch input or patch
|||	        knob. (patch inputs and knobs appear as outputs for the purpose of
|||	        internal connections) When non-NULL, this string must remain valid for
|||	        the life of the PatchCmdBuilder.
|||
|||	    fromPortName
|||	        Name of an output of the block named by fromBlockName, or patch input or
|||	        patch knob when fromBlockName is NULL. This string must remain valid
|||	        for the life of the PatchCmdBuilder.
|||
|||	    fromPartNum
|||	        Part number of the block output, patch input, or patch knob specified by
|||	        fromBlockName and fromPortName.
|||
|||	    toBlockName
|||	        Name of a patch block containing an input or knob to connect to. Or NULL
|||	        to indicate that the destination of the connection is a patch output.
|||	        (patch outputs appear as inputs for the purpose of internal connections)
|||	        When non-NULL, this string must remain valid for the life of the
|||	        PatchCmdBuilder.
|||
|||	    toPortName
|||	        Name of an input or knob of the block named by toBlockName, or patch
|||	        output when toBlockName is NULL. This string must remain valid for the
|||	        life of the PatchCmdBuilder.
|||
|||	    toPartNum
|||	        Part number of the block input, block knob, or patch output specified by
|||	        toBlockName and toPortName.
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
|||	    Ths signal carried by a connection created by this PatchCmd(@) isn't
|||	    guaranteed to reach the destination in the same frame that it was output if
|||	    PATCH_CMD_SET_COHERENCE(@) is set to FALSE.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), PATCH_CMD_CONNECT(@), CreatePatchCmdBuilder(),
|||	    AddTemplateToPatch(), DefinePatchPort(), DefinePatchKnob(),
|||	    SetPatchConstant(), SetPatchCoherence()
**/

Err ConnectPatchPorts (PatchCmdBuilder *builder, const char *fromBlockName, const char *fromPortName, uint32 fromPartNum,
                                                 const char *toBlockName,   const char *toPortName,   uint32 toPartNum)
{
    PatchCmdConnect newcmd = { PATCH_CMD_CONNECT };

    newcmd.pc_FromBlockName = fromBlockName;
    newcmd.pc_FromPortName  = fromPortName;
    newcmd.pc_FromPartNum   = fromPartNum;
    newcmd.pc_ToBlockName   = toBlockName;
    newcmd.pc_ToPortName    = toPortName;
    newcmd.pc_ToPartNum     = toPartNum;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
