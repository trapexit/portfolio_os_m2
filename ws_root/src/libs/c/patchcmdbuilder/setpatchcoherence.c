/* @(#) setpatchcoherence.c 96/02/29 1.2 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name SetPatchCoherence
|||	Adds a PATCH_CMD_SET_COHERENCE(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err SetPatchCoherence (PatchCmdBuilder *builder, bool state)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_SET_COHERENCE(@) PatchCmd to list under construction. This
|||	    controls whether the patch is to be built in such a way as to guarantee
|||	    signal phase coherence along all internal connections. With coherence set
|||	    to FALSE, signals output from one constituent instrument may not propagate
|||	    into the destination constituent instrument until the next audio frame.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    state
|||	        When set to TRUE, the default case, the patch is constructed with
|||	        signal phase coherence guaranteed along internal connections. When set
|||	        to FALSE, there is no guarantee of signal phase coherence.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, negative error code on failure.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), PATCH_CMD_SET_COHERENCE(@), CreatePatchCmdBuilder(),
|||	    AddTemplateToPatch(), ConnectPatchPorts()
**/

Err SetPatchCoherence (PatchCmdBuilder *builder, bool state)
{
    PatchCmdSetCoherence newcmd = { PATCH_CMD_SET_COHERENCE };

    newcmd.pc_State = state;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
