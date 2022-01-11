/******************************************************************************
**
**  @(#) patchcmd.c 96/02/28 1.19
**
**  PatchCmd support functions.
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
**  950824 WJB  Updated PATCH_CMD_ autodocs.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/patch.h>

/**
|||	AUTODOC -public -class AudioPatch -name NextPatchCmd
|||	Finds the next PatchCmd(@) in a PatchCmd list.
|||
|||	  Synopsis
|||
|||	    PatchCmd *NextPatchCmd (const PatchCmd **cmdState)
|||
|||	  Description
|||
|||	    This function iterates through a PatchCmd list, skipping and chaining as
|||	    dictated by control commands. There are three control commands:
|||
|||	    PATCH_CMD_NOP(@)
|||	        Ignores that single entry and moves to the next one.
|||
|||	    PATCH_CMD_JUMP(@)
|||	        Has a pointer to another array of PatchCmds.
|||
|||	    PATCH_CMD_END(@)
|||	        Marks the end of the PatchCmd list.
|||
|||	    This function only returns PatchCmds which are not control commands. Each call
|||	    returns either the next PatchCmd you should examine, or NULL when the end of
|||	    the list has been reached.
|||
|||	  Arguments
|||
|||	    cmdState
|||	        This is a pointer to a storage location used by the iterator to keep
|||	        track of its current location in the PatchCmd list. The variable that this
|||	        parameter points to should be initialized to point to the first PatchCmd
|||	        in the list, and should not be changed thereafter.
|||
|||	  Return Value
|||
|||	    This function returns a pointer to a PatchCmd structure, or NULL if all the
|||	    PatchCmds have been visited. None of the control commands are ever returned
|||	    to you, they are handled transparently by this function.
|||
|||	  Implementation
|||
|||	    Folio call implemented in AudioPatch Folio V30.
|||
|||	  Example
|||
|||	    void WalkPatchCmdList (const PatchCmd *patchCmdList)
|||	    {
|||	        const PatchCmd *state, *pc;
|||
|||	        for (state = patchCmdList; pc = NextPatchCmd(&state);) {
|||
|||	            // do something with pc
|||
|||	        }
|||	    }
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioPatchFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, System.m2/Modules/audiopatch
|||
|||	  See Also
|||
|||	    PatchCmd(@), CreatePatchTemplate(), NextTagArg(), PATCH_CMD_JUMP(@),
|||	    PATCH_CMD_NOP(@), PATCH_CMD_END(@)
**/
PatchCmd *NextPatchCmd (const PatchCmd **cmdState)
{
    const PatchCmd *pc = *cmdState;

    while (pc) {
        switch (pc->pc_Generic.pc_CmdID) {
            case PATCH_CMD_END:
                pc = NULL;
                break;

            case PATCH_CMD_NOP:
                pc++;
                break;

            case PATCH_CMD_JUMP:
                pc = pc->pc_Jump.pc_NextPatchCmd;   /* @@@ not safe for supervisor code */
                break;

            default:
                *cmdState = &pc[1];
                return (PatchCmd *)pc;
        }
    }

    return NULL;
}
