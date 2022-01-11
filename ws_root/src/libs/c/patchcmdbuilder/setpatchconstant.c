/* @(#) setpatchconstant.c 96/02/28 1.4 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name SetPatchConstant
|||	Adds a PATCH_CMD_SET_CONSTANT(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err SetPatchConstant (PatchCmdBuilder *builder, const char *blockName,
|||	                          const char *portName, uint32 partNum,
|||	                          float32 constantValue)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_SET_CONSTANT(@) PatchCmd to list under construction. This will
|||	    assign a constant value to an internal unconnected input or knob part. This
|||	    is a space-efficient way to set permanent values of things like
|||	    coefficients, biases, frequencies, etc., that would otherwise require a knob.
|||
|||	    Any block input or block knob can have up to one connection or constant.
|||	    Unused block inputs default to 0; unused block knobs default to their
|||	    default value.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    blockName
|||	        Name of a patch block containing an input or knob to set to a constant.
|||	        Patch outputs cannot be set to a constant, therefore NULL is illegal
|||	        here. This string must remain valid for the life of the PatchCmdBuilder.
|||
|||	    portName
|||	        Name of an input or knob of the block named by blockName. This string
|||	        must remain valid for the life of the PatchCmdBuilder.
|||
|||	    partNum
|||	        Part number of the input or knob specified by blockName and portName.
|||
|||	    constantValue
|||	        The constant value to set. The units depend on the thing being assigned.
|||	        For knobs, the units are determined by the knob type (e.g. oscillator
|||	        frequency in Hz, signed signal level, etc.). For inputs, this is always a
|||	        signed signal in the range of -1.0 to 1.0.
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
|||	    PatchCmd(@), PATCH_CMD_SET_CONSTANT(@), CreatePatchCmdBuilder(),
|||	    AddTemplateToPatch(), ConnectPatchPorts()
**/

Err SetPatchConstant (PatchCmdBuilder *builder, const char *blockName, const char *portName, uint32 partNum, float32 constantValue)
{
    PatchCmdSetConstant newcmd = { PATCH_CMD_SET_CONSTANT };

    newcmd.pc_BlockName     = blockName;
    newcmd.pc_PortName      = portName;
    newcmd.pc_PartNum       = partNum;
    newcmd.pc_ConstantValue = constantValue;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
