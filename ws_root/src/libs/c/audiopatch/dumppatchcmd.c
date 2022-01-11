/******************************************************************************
**
**  @(#) dumppatchcmd.c 96/02/28 1.15
**
**  PatchCmd debug functions.
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
**  950711 WJB  Created.
**  950727 WJB  Changed output format.
**  950811 WJB  Added output of AF_SIGNAL_TYPE_ and AF_PORT_TYPE_ constant names
**              instead of magic numbers.
**  951101 WJB  Moved DumpPatchCmdList() to separate module.
**  951101 WJB  Added printout of control PatchCmds.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>        /* AF_PORT_TYPE_, AF_SIGNAL_TYPE_ */
#include <audio/patch.h>
#include <stdio.h>


/**
|||	AUTODOC -public -class AudioPatch -name DumpPatchCmd
|||	Prints out a PatchCmd.
|||
|||	  Synopsis
|||
|||	    void DumpPatchCmd (const PatchCmd *patchCmd, const char *prefix)
|||
|||	  Description
|||
|||	    Prints out the contents of a PatchCmd.
|||
|||	  Arguments
|||
|||	    patchCmd
|||	        PatchCmd to print.
|||
|||	    prefix
|||	        Text to print on line before PatchCmd. Can be NULL.
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
|||	    PatchCmd(@), DumpPatchCmdList()
**/

void DumpPatchCmd (const PatchCmd *patchCmd, const char *prefix)
{
    static const char * const PortTypeDesc[AF_PORT_TYPE_MANY] = {   /* @@@ indexed by AF_PORT_TYPE_ */
        "AF_PORT_TYPE_INPUT",
        "AF_PORT_TYPE_OUTPUT",
    };
    static const char * const SigTypeDesc[AF_SIGNAL_TYPE_MANY] = {  /* @@@ indexed by AF_SIGNAL_TYPE_ */
        "AF_SIGNAL_TYPE_GENERIC_SIGNED",
        "AF_SIGNAL_TYPE_GENERIC_UNSIGNED",
        "AF_SIGNAL_TYPE_OSC_FREQ",
        "AF_SIGNAL_TYPE_LFO_FREQ",
        "AF_SIGNAL_TYPE_SAMPLE_RATE",
        "AF_SIGNAL_TYPE_WHOLE_NUMBER",
    };

    #define PRTSTR(str) (str) ? "\"" : "", (str) ? (str) : "NULL", (str) ? "\"" : ""

    if (prefix) printf ("%s", prefix);

    switch (patchCmd->pc_Generic.pc_CmdID) {
        case PATCH_CMD_ADD_TEMPLATE:
            {
                const PatchCmdAddTemplate * const pc = &patchCmd->pc_AddTemplate;

                printf ("{ PATCH_CMD_ADD_TEMPLATE, %s%s%s, 0x%x }\n",
                    PRTSTR(pc->pc_BlockName),
                    pc->pc_InsTemplate);
            }
            break;

        case PATCH_CMD_DEFINE_PORT:
            {
                const PatchCmdDefinePort * const pc = &patchCmd->pc_DefinePort;
                char porttypedescbuf[24];
                const char *porttypedesc;
                char sigtypedescbuf[24];
                const char *sigtypedesc;

                if (pc->pc_PortType > AF_PORT_TYPE_MAX || !(porttypedesc = PortTypeDesc[pc->pc_PortType])) {
                    sprintf (porttypedesc = porttypedescbuf, "%u", pc->pc_PortType);
                }

                if (pc->pc_SignalType > AF_SIGNAL_TYPE_MAX || !(sigtypedesc = SigTypeDesc[pc->pc_SignalType])) {
                    sprintf (sigtypedesc = sigtypedescbuf, "%u", pc->pc_SignalType);
                }

                printf ("{ PATCH_CMD_DEFINE_PORT, %s%s%s, %u, %s, %s }\n",
                    PRTSTR(pc->pc_PortName),
                    pc->pc_NumParts,
                    porttypedesc,
                    sigtypedesc);
            }
            break;

        case PATCH_CMD_DEFINE_KNOB:
            {
                const PatchCmdDefineKnob * const pc = &patchCmd->pc_DefineKnob;
                char sigtypedescbuf[24];
                const char *sigtypedesc;

                if (pc->pc_KnobType > AF_SIGNAL_TYPE_MAX || !(sigtypedesc = SigTypeDesc[pc->pc_KnobType])) {
                    sprintf (sigtypedesc = sigtypedescbuf, "%u", pc->pc_KnobType);
                }

                printf ("{ PATCH_CMD_DEFINE_KNOB, %s%s%s, %u, %s, %f }\n",
                    PRTSTR(pc->pc_KnobName),
                    pc->pc_NumParts,
                    sigtypedesc,
                    pc->pc_DefaultValue);
            }
            break;

        case PATCH_CMD_CONNECT:
            {
                const PatchCmdConnect * const pc = &patchCmd->pc_Connect;

                printf ("{ PATCH_CMD_CONNECT, %s%s%s, %s%s%s, %u, %s%s%s, %s%s%s, %u }\n",
                    PRTSTR(pc->pc_FromBlockName),
                    PRTSTR(pc->pc_FromPortName),
                    pc->pc_FromPartNum,
                    PRTSTR(pc->pc_ToBlockName),
                    PRTSTR(pc->pc_ToPortName),
                    pc->pc_ToPartNum);
            }
            break;

        case PATCH_CMD_EXPOSE:
            {
                const PatchCmdExpose * const pc = &patchCmd->pc_Expose;

                printf ("{ PATCH_CMD_EXPOSE, %s%s%s, %s%s%s, %s%s%s }\n",
                    PRTSTR(pc->pc_PortName),
                    PRTSTR(pc->pc_SrcBlockName),
                    PRTSTR(pc->pc_SrcPortName));
            }
            break;

        case PATCH_CMD_SET_CONSTANT:
            {
                const PatchCmdSetConstant * const pc = &patchCmd->pc_SetConstant;

                printf ("{ PATCH_CMD_SET_CONSTANT, %s%s%s, %s%s%s, %u, %f }\n",
                    PRTSTR(pc->pc_BlockName),
                    PRTSTR(pc->pc_PortName),
                    pc->pc_PartNum,
                    pc->pc_ConstantValue);
            }
            break;

        case PATCH_CMD_SET_COHERENCE:
            {
                const PatchCmdSetCoherence * const pc = &patchCmd->pc_SetCoherence;

                printf ("{ PATCH_CMD_SET_COHERENCE, %d }\n",
                    pc->pc_State);
            }
            break;

        case PATCH_CMD_JUMP:
            {
                const PatchCmdJump * const pc = &patchCmd->pc_Jump;

                printf ("{ PATCH_CMD_JUMP, 0x%x }\n", pc->pc_NextPatchCmd);
            }
            break;

        case PATCH_CMD_NOP:
            printf ("{ PATCH_CMD_NOP }\n");
            break;

        case PATCH_CMD_END:
            printf ("{ PATCH_CMD_END }\n");
            break;

        default:
            {
                const PatchCmdGeneric * const pc = &patchCmd->pc_Generic;

                printf ("{ /* PATCH_CMD_??? */ 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x }\n",
                    pc->pc_CmdID,
                    pc->pc_Args[0],
                    pc->pc_Args[1],
                    pc->pc_Args[2],
                    pc->pc_Args[3],
                    pc->pc_Args[4],
                    pc->pc_Args[5]);
            }
            break;
    }

    #undef PRTSTR
}
