/* @(#) exposepatchport.c 96/02/28 1.4 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name ExposePatchPort
|||	Adds a PATCH_CMD_EXPOSE(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err ExposePatchPort (PatchCmdBuilder *builder, const char *portName,
|||	                         const char *srcBlockName, const char *srcPortName)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_EXPOSE(@) PatchCmd to list under construction. This will
|||	    expose a FIFO, envelope hook, or trigger belonging to one of the patch
|||	    blocks, so that it may be accessed from outside the patch. For example, if
|||	    envelope.dsp is one of the blocks in the patch, use this command to expose
|||	    its envelope hook, Env, so that it may be attached to once the patch is
|||	    compiled. Unless this is done, FIFOs, envelope hooks, and triggers remain
|||	    private to the blocks of the patch and are not accessible to the clients of
|||	    the finished patch.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    portName
|||	        Name to give the exposed FIFO, envelope hook, or trigger when exposed.
|||	        Ports, knobs, and exposed things all share the same name space and must
|||	        have unique names. Names are compared case-insensitively. As with ports
|||	        and knobs, exposed names cannot contain periods. May be the same name
|||	        (or even the same pointer) as pc_SrcPortName as long as the above rules
|||	        are satisfied. This string must remain valid for the life of the
|||	        PatchCmdBuilder.
|||
|||	    srcBlockName
|||	        Name of the patch block containing the port to expose. This string must
|||	        remain valid for the life of the PatchCmdBuilder.
|||
|||	    srcPortName
|||	        Name of the port within the specified patch block to expose. This string
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
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a
|||
|||	  See Also
|||
|||	    PatchCmd(@), PATCH_CMD_EXPOSE(@), CreatePatchCmdBuilder(),
|||	    AddTemplateToPatch(), DefinePatchPort(), DefinePatchKnob()
**/

Err ExposePatchPort (PatchCmdBuilder *builder, const char *portName, const char *srcBlockName, const char *srcPortName)
{
    PatchCmdExpose newcmd = { PATCH_CMD_EXPOSE };

    newcmd.pc_PortName     = portName;
    newcmd.pc_SrcBlockName = srcBlockName;
    newcmd.pc_SrcPortName  = srcPortName;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
