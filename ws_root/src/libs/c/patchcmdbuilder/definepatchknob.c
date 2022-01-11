/* @(#) definepatchknob.c 96/02/28 1.4 */

#include "patchcmdbuilder_internal.h"

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmdBuilder -name DefinePatchKnob
|||	Adds a PATCH_CMD_DEFINE_KNOB(@) PatchCmd to PatchCmdBuilder.
|||
|||	  Synopsis
|||
|||	    Err DefinePatchKnob (PatchCmdBuilder *builder, const char *knobName,
|||	                         uint32 numParts, uint32 knobType, float32 defaultValue)
|||
|||	  Description
|||
|||	    Adds a PATCH_CMD_DEFINE_KNOB(@) PatchCmd to list under construction. This will
|||	    define a knob or knob array for the patch. Once defined, ports and knobs can
|||	    be connected to blocks added with PATCH_CMD_ADD_TEMPLATE(@) or one another.
|||
|||	    All patch knobs must be connected to something, or else the patch compiler
|||	    returns an error.
|||
|||	  Arguments
|||
|||	    builder
|||	        PatchCmdBuilder to add to.
|||
|||	    knobName
|||	        Name for knob or knob array being added. Each port and knob in a patch
|||	        must have a unique name. Names are compared case-insensitively. Port and
|||	        knob names must not contain periods. This string must remain valid for
|||	        the life of the PatchCmdBuilder.
|||
|||	    numParts
|||	        Number of parts for knob. 1 for a simple knob, >1 for a knob array.
|||
|||	    knobType
|||	        Default knob signal type (AF_SIGNAL_TYPE_ value).
|||
|||	    defaultValue
|||	        Default value for all parts of knob. This value is in whichever units
|||	        are appropriate for the specified knob type.
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
|||	    PatchCmd(@), PATCH_CMD_DEFINE_KNOB(@), CreatePatchCmdBuilder(),
|||	    DefinePatchPort(), ExposePatchPort(), ConnectPatchPorts()
**/

Err DefinePatchKnob (PatchCmdBuilder *builder, const char *knobName, uint32 numParts, uint32 knobType, float32 defaultValue)
{
    PatchCmdDefineKnob newcmd = { PATCH_CMD_DEFINE_KNOB };

    newcmd.pc_KnobName     = knobName;
    newcmd.pc_NumParts     = numParts;
    newcmd.pc_KnobType     = knobType;
    newcmd.pc_DefaultValue = defaultValue;

    return AddPatchCmdToBuilder (builder, (PatchCmd *)&newcmd);
}
