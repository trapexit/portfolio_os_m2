/* @(#) definepatchport.c 96/02/28 1.4 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name DefinePatchPort
|||	Adds a PATCH_CMD_DEFINE_PORT(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err DefinePatchPort (PatchCmdBuilder *builder, const char *portName,
|||	                         uint32 numParts, uint32 portType, uint32 signalType)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_DEFINE_PORT(@) PatchCmd to list under construction. This will
|||	    define an input or output port or port array for the patch. Once defined,
|||	    ports and knobs can be connected to blocks added with PATCH_CMD_ADD_TEMPLATE(@)
|||	    or one another.
|||
|||	    All patch inputs and outputs must be connected to something, or else the
|||	    patch compiler returns an error.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    portName
|||	        Port name for port or port array being added. Each port and knob in a
|||	        patch must have a unique name. Names are compared case-insensitively.
|||	        Port and knob names must not contain periods. This string must remain
|||	        valid for the life of the PatchCmdBuilder.
|||
|||	    numParts
|||	        Number of parts for port. 1 for a simple input or output, >1 for an
|||	        input or output array.
|||
|||	    portType
|||	        AF_PORT_TYPE_INPUT for an input or AF_PORT_TYPE_OUTPUT for an output.
|||
|||	    signalType
|||	        Signal type (AF_SIGNAL_TYPE_ value).
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
|||	    PatchCmd(@), PATCH_CMD_DEFINE_PORT(@), CreatePatchCmdBuilder(),
|||	    DefinePatchKnob(), ExposePatchPort(), ConnectPatchPorts()
**/

Err DefinePatchPort (PatchCmdBuilder *builder, const char *portName, uint32 numParts, uint32 portType, uint32 signalType)
{
    PatchCmdDefinePort newcmd = { PATCH_CMD_DEFINE_PORT };

    newcmd.pc_PortName   = portName;
    newcmd.pc_NumParts   = numParts;
    newcmd.pc_PortType   = portType;
    newcmd.pc_SignalType = signalType;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
