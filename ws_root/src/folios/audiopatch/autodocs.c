/******************************************************************************
**
**  @(#) autodocs.c 96/02/29 1.5
**
**  PATCH_CMD_ autodocs
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
**  960216 WJB  Created.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/


/*
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PatchCmd
|||	Command set used by CreatePatchTemplate().
|||
|||	  Description
|||
|||	    PatchCmds are the command set used by CreatePatchTemplate() to construct a
|||	    custom patch template. A PatchCmd list is an array of PatchCmds terminated
|||	    by PATCH_CMD_END(@) or a list of PatchCmd arrays joined by
|||	    PATCH_CMD_JUMP(@), and the last array terminated by PATCH_CMD_END(@). In
|||	    this respect PatchCmd lists are much like TagArg lists.
|||
|||	    Unlike TagArgs, each PATCH_CMD_ has a different set of arguments. So, there
|||	    is a different structure associated with each PATCH_CMD_. In order to make
|||	    an array out of these different structures, the PatchCmd typedef is defined
|||	    as a union of all of these structures.
|||
|||	    Using a union complicates defining a PatchCmd array with static
|||	    initializers, however, so there is a simple PatchCmd builder system (see
|||	    CreatePatchCmdBuilder()), which offers a set of functions to construct a
|||	    PatchCmd list one PatchCmd at a time.
|||
|||	    Names used in PatchCmds (e.g., port, block, knob) are all matched
|||	    case-insensitively and must be unique within their name space. Blocks have
|||	    their own name space. Ports, knobs, exposed things, and imported symbols
|||	    (internal to DSP instruments) occupy the same name space.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), CreatePatchCmdBuilder(), NextPatchCmd(),
|||	    DumpPatchCmdList(), NextTagArg(), makepatch(@)
**/


/* -------------------- Constructive PatchCmds */

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_ADD_TEMPLATE
|||	Adds an instrument Template(@) to patch.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdAddTemplate {
|||	        uint32      pc_CmdID;           // PATCH_CMD_ADD_TEMPLATE
|||	        const char *pc_BlockName;
|||	        Item        pc_InsTemplate;
|||	        uint32      pc_Pad3;
|||	        uint32      pc_Pad4;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdAddTemplate;
|||
|||	  Description
|||
|||	    Add an Instrument Template Item to the patch with the specified name.
|||	    Any Instrument Template may be added to the patch: standard DSP instruments,
|||	    patches, or mixers. Multiple instances of the same Instrument Template
|||	    Item may be added to any patch. Each template must be given a unique block
|||	    name within the patch.
|||
|||	  Fields
|||
|||	    pc_BlockName
|||	        Block name for template being added. Each block in a patch must have a
|||	        unique name. Names are compared case-insensitively. Internal connections
|||	        are specified using this block name.
|||
|||	    pc_InsTemplate
|||	        Instrument Template Item to use for this block. May be a standard DSP
|||	        instrument template, a mixer, or a previously created patch.
|||
|||	  Notes
|||
|||	    To minimize propagation delay across a patch, instruments should be added
|||	    in the order of signal flow.
|||
|||	  Caveats
|||
|||	    Any Items attached to pc_InsTemplate (i.e. Sample(@)s, Envelope(@)s, or
|||	    Tuning(@)s) are not propagated to the compiled patch.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), AddTemplateToPatch(),
|||	    PATCH_CMD_EXPOSE(@), PATCH_CMD_CONNECT(@), PATCH_CMD_SET_CONSTANT(@),
|||	    PATCH_CMD_SET_COHERENCE(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_DEFINE_PORT
|||	Defines a patch input or output port.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdDefinePort {
|||	        uint32      pc_CmdID;           // PATCH_CMD_DEFINE_PORT
|||	        const char *pc_PortName;
|||	        uint32      pc_NumParts;
|||	        uint32      pc_PortType;
|||	        uint32      pc_SignalType;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdDefinePort;
|||
|||	  Description
|||
|||	    Define an input or output port or port array for the patch. Once defined,
|||	    ports and knobs can be connected to blocks added with PATCH_CMD_ADD_TEMPLATE(@)
|||	    or one another.
|||
|||	    All patch inputs and outputs must be connected to something, or else the
|||	    patch compiler returns an error.
|||
|||	  Fields
|||
|||	    pc_PortName
|||	        Port name for port or port array being added. Each port and knob in a
|||	        patch must have a unique name. Names are compared case-insensitively.
|||	        Port and knob names must not contain periods.
|||
|||	    pc_NumParts
|||	        Number of parts for port. 1 for a simple input or output, >1 for an
|||	        input or output array.
|||
|||	    pc_PortType
|||	        AF_PORT_TYPE_INPUT for an input or AF_PORT_TYPE_OUTPUT for an output.
|||
|||	    pc_SignalType
|||	        Signal type (AF_SIGNAL_TYPE_ value).
|||
|||	  Caveats
|||
|||	    Doesn't currently detect unconnected patch inputs.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, <audio/audio.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), DefinePatchPort(),
|||	    PATCH_CMD_DEFINE_KNOB(@), PATCH_CMD_EXPOSE(@), PATCH_CMD_CONNECT(@),
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_DEFINE_KNOB
|||	Defines a patch knob.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdDefineKnob {
|||	        uint32      pc_CmdID;           // PATCH_CMD_DEFINE_KNOB
|||	        const char *pc_KnobName;
|||	        uint32      pc_NumParts;
|||	        uint32      pc_KnobType;
|||	        float32     pc_DefaultValue;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdDefineKnob;
|||
|||	  Description
|||
|||	    Define a knob or knob array for the patch. Once defined, ports and knobs can
|||	    be connected to blocks added with PATCH_CMD_ADD_TEMPLATE(@) or one another.
|||
|||	    All patch knobs must be connected to something, or else the patch compiler
|||	    returns an error.
|||
|||	  Fields
|||
|||	    pc_KnobName
|||	        Name for knob or knob array being added. Each port and knob in a patch
|||	        must have a unique name. Names are compared case-insensitively. Port and
|||	        knob names must not contain periods.
|||
|||	    pc_NumParts
|||	        Number of parts for knob. 1 for a simple knob, >1 for a knob array.
|||
|||	    pc_KnobType
|||	        Default knob type (AF_SIGNAL_TYPE_ value).
|||
|||	    pc_DefaultValue
|||	        Default value for all parts of knob. This value is in whichever units
|||	        are appropriate for the specified knob type.
|||
|||	  Caveats
|||
|||	    Doesn't currently detect unconnected patch knobs.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, <audio/audio.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), DefinePatchKnob(),
|||	    PATCH_CMD_DEFINE_PORT(@), PATCH_CMD_EXPOSE(@), PATCH_CMD_CONNECT(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_EXPOSE
|||	Exposes a patch port.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdExpose {
|||	        uint32      pc_CmdID;           // PATCH_CMD_EXPOSE
|||	        const char *pc_PortName;
|||	        const char *pc_SrcBlockName;
|||	        const char *pc_SrcPortName;
|||	        uint32      pc_Pad4;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdExpose;
|||
|||	  Description
|||
|||	    Expose a FIFO, envelope hook, or trigger belonging to one of the patch
|||	    blocks, so that it may be accessed from outside the patch. For example, if
|||	    envelope.dsp is one of the blocks in the patch, use this command to expose
|||	    its envelope hook, Env, so that it may be attached to once the patch is
|||	    compiled. Unless this is done, FIFOs, envelope hooks, and triggers remain
|||	    private to the blocks of the patch and are not accessible to the clients of
|||	    the finished patch.
|||
|||	  Fields
|||
|||	    pc_PortName
|||	        Name to give the exposed FIFO, envelope hook, or trigger when exposed.
|||	        Ports, knobs, and exposed things all share the same name space and must
|||	        have unique names. Names are compared case-insensitively. As with ports
|||	        and knobs, exposed names cannot contain periods. May be the same name
|||	        (or even the same pointer) as pc_SrcPortName as long as the above rules
|||	        are satisfied.
|||
|||	    pc_SrcBlockName
|||	        Name of the patch block containing the port to expose.
|||
|||	    pc_SrcPortName
|||	        Name of the port within the specified patch block to expose.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), ExposePatchPort(),
|||	    PATCH_CMD_ADD_TEMPLATE(@), PATCH_CMD_DEFINE_PORT(@),
|||	    PATCH_CMD_DEFINE_KNOB(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_CONNECT
|||	Connects patch blocks and ports to one another.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdConnect {
|||	        uint32      pc_CmdID;           // PATCH_CMD_CONNECT
|||	        const char *pc_FromBlockName;
|||	        const char *pc_FromPortName;
|||	        uint32      pc_FromPartNum;
|||	        const char *pc_ToBlockName;
|||	        const char *pc_ToPortName;
|||	        uint32      pc_ToPartNum;
|||	    } PatchCmdConnect;
|||
|||	  Description
|||
|||	    Creates an internal signal connection between a patch input, patch knob,
|||	    or block output, and a patch output, block input, or block knob.
|||
|||	    Any block input or block knob can have up to one connection or constant.
|||	    Unused block inputs default to 0; unused block knobs default to their
|||	    default value. All patch outputs must be connected.
|||
|||	  Fields
|||
|||	    pc_FromBlockName
|||	        Name of a patch block containing an output to connect from. Or NULL to
|||	        indicate that the source of the connection is a patch input or patch
|||	        knob. (patch inputs and knobs appear as outputs for the purpose of
|||	        internal connections)
|||
|||	    pc_FromPortName
|||	        Name of an output of the block named by pc_FromBlockName, or patch input
|||	        or patch knob when pc_FromBlockName is NULL.
|||
|||	    pc_FromPartNum
|||	        Part number of the block output, patch input, or patch knob specified by
|||	        pc_FromBlockName and pc_FromPortName.
|||
|||	    pc_ToBlockName
|||	        Name of a patch block containing an input or knob to connect to. Or NULL
|||	        to indicate that the destination of the connection is a patch output.
|||	        (patch outputs appear as inputs for the purpose of internal connections)
|||
|||	    pc_ToPortName
|||	        Name of an input or knob of the block named by pc_ToBlockName, or patch
|||	        output when pc_ToBlockName is NULL.
|||
|||	    pc_ToPartNum
|||	        Part number of the block input, block knob, or patch output specified by
|||	        pc_ToBlockName and pc_ToPortName.
|||
|||	  Notes
|||
|||	    Ths signal carried by a connection created by this PatchCmd(@) isn't
|||	    guaranteed to reach the destination in the same frame that it was output if
|||	    PATCH_CMD_SET_COHERENCE(@) is set to FALSE.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), ConnectPatchPorts(),
|||	    PATCH_CMD_ADD_TEMPLATE(@), PATCH_CMD_DEFINE_PORT(@),
|||	    PATCH_CMD_DEFINE_KNOB(@), PATCH_CMD_SET_CONSTANT(@),
|||	    PATCH_CMD_SET_COHERENCE(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_SET_CONSTANT
|||	Sets a block input or knob to a constant.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdSetConstant {
|||	        uint32      pc_CmdID;           // PATCH_CMD_SET_CONSTANT
|||	        const char *pc_BlockName;
|||	        const char *pc_PortName;
|||	        uint32      pc_PartNum;
|||	        float32     pc_ConstantValue;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdSetConstant;
|||
|||	  Description
|||
|||	    Assigns a constant value to an internal unconnected input or knob part.
|||	    This is a space-efficient way to set permanent values of things like
|||	    coefficients, biases, frequencies, etc., that would otherwise require a
|||	    knob.
|||
|||	    Any block input or block knob can have up to one connection or constant.
|||	    Unused block inputs default to 0; unused block knobs default to their
|||	    default value.
|||
|||	  Fields
|||
|||	    pc_BlockName
|||	        Name of a patch block containing an input or knob to set to a constant.
|||	        Patch outputs cannot be set to a constant, therefore NULL is illegal
|||	        here.
|||
|||	    pc_PortName
|||	        Name of an input or knob of the block named by pc_BlockName.
|||
|||	    pc_PartNum
|||	        Part number of the input or knob specified by pc_BlockName and
|||	        pc_PortName.
|||
|||	    pc_ConstantValue
|||	        The constant value to set. The units depend on the thing being assigned.
|||	        For knobs, the units are determined by the knob type (e.g. oscillator
|||	        frequency in Hz, signed signal level, etc.). For inputs, this is always a
|||	        signed signal in the range of -1.0 to 1.0.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), SetPatchConstant(),
|||	    PATCH_CMD_ADD_TEMPLATE(@), PATCH_CMD_CONNECT(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_SET_COHERENCE
|||	Controls signal phase coherence along internal patch connections.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdSetCoherence {
|||	        uint32      pc_CmdID;           // PATCH_CMD_SET_COHERENCE
|||	        uint32      pc_State;           // TRUE to set, FALSE to clear
|||	        uint32      pc_Pad2;
|||	        uint32      pc_Pad3;
|||	        uint32      pc_Pad4;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdSetCoherence;
|||
|||	  Description
|||
|||	    Controls whether the patch is to be built in such a way as to guarantee
|||	    signal phase coherence along all internal connections (as defined by
|||	    PATCH_CMD_CONNECT(@)s). With PATCH_CMD_SET_COHERENCE set to FALSE, signals
|||	    output from one constituent instrument may not propagate into the
|||	    destination constituent instrument until the next audio frame.
|||
|||	    The ability to control this aspect of patch construction is provided
|||	    because guaranteeing signal phase coherence requires slightly more DSP code
|||	    words and ticks than not doing so, and it is only necessary in situations
|||	    where you need precise control over signal propagation delay (e.g., a comb
|||	    filter). Patches default to being created with PATCH_CMD_SET_COHERENCE set
|||	    to TRUE. The last PATCH_CMD_SET_COHERENCE encountered in a PatchCmd(@) list
|||	    determines the coherence of the entire patch.
|||
|||	  Fields
|||
|||	    pc_State
|||	        When set to TRUE, the default case, the patch is constructed with signal
|||	        phase coherence guaranteed along internal connections. When set to
|||	        FALSE, there is no guarantee of signal phase coherence.
|||
|||	  Notes
|||
|||	    To minimize propagation delay across a patch, instruments should be added
|||	    to the patch (using PATCH_CMD_ADD_TEMPLATE(@)) in the order of signal flow.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    CreatePatchTemplate(), PatchCmd(@), SetPatchCoherence(),
|||	    PATCH_CMD_ADD_TEMPLATE(@), PATCH_CMD_CONNECT(@)
**/


/* -------------------- Control PatchCmds */

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_JUMP
|||	Links to another list of PatchCmds.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdJump {
|||	        uint32      pc_CmdID;           // PATCH_CMD_JUMP
|||	        const PatchCmd *pc_NextPatchCmd;
|||	        uint32      pc_Pad2;
|||	        uint32      pc_Pad3;
|||	        uint32      pc_Pad4;
|||	        uint32      pc_Pad5;
|||	        uint32      pc_Pad6;
|||	    } PatchCmdJump;
|||
|||	  Description
|||
|||	    Links the end of one PatchCmd list to another.
|||
|||	  Fields
|||
|||	    pc_NextPatchCmd
|||	        Pointer to next PatchCmd list. Can be NULL, which causes this command
|||	        to behave as a PATCH_CMD_END(@).
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    PatchCmd(@), NextPatchCmd(), PATCH_CMD_NOP(@), PATCH_CMD_END(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_NOP
|||	Skip this command.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdGeneric {
|||	        uint32      pc_CmdID;           // PATCH_CMD_NOP
|||	        uint32      pc_Args[6];
|||	    } PatchCmdGeneric;
|||
|||	  Description
|||
|||	    This command is skipped.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    PatchCmd(@), NextPatchCmd(), PATCH_CMD_JUMP(@), PATCH_CMD_END(@)
**/

/**
|||	AUTODOC -public -class AudioPatch -group PatchCmd -name PATCH_CMD_END
|||	Marks the end of a PatchCmd list.
|||
|||	  Synopsis
|||
|||	    typedef struct PatchCmdGeneric {
|||	        uint32      pc_CmdID;           // PATCH_CMD_END
|||	        uint32      pc_Args[6];
|||	    } PatchCmdGeneric;
|||
|||	  Description
|||
|||	    This marks the end of a PatchCmd list.
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>
|||
|||	  See Also
|||
|||	    PatchCmd(@), NextPatchCmd(), PATCH_CMD_JUMP(@), PATCH_CMD_NOP(@)
**/


/* keep the compiler happy... */
extern int foo;
