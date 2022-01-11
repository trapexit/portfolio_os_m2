/******************************************************************************
**
**  @(#) dumppatchcmdlist.c 96/02/28 1.3
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
**  951101 WJB  Split off from dumppatchcmd.c
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/patch.h>
#include <stdio.h>


/**
|||	AUTODOC -public -class AudioPatch -name DumpPatchCmdList
|||	Prints out a PatchCmd list.
|||
|||	  Synopsis
|||
|||	    void DumpPatchCmdList (const PatchCmd *patchCmdList, const char *banner)
|||
|||	  Description
|||
|||	    Prints out the contents of a PatchCmd list.
|||
|||	  Arguments
|||
|||	    patchCmdList
|||	        The list of PatchCmds to print. Can be NULL, which signifies an empty
|||	        list.
|||
|||	    banner
|||	        Description of PatchCmd list to print. Can be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libc.a V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioPatchFolio()
|||
|||	  Associated Files
|||
|||	    <audio/patch.h>, libc.a, System.m2/Modules/audiopatch
|||
|||	  See Also
|||
|||	    PatchCmd(@), DumpPatchCmd()
**/

void DumpPatchCmdList (const PatchCmd *patchCmds, const char *banner)
{
    const PatchCmd *state;
    const PatchCmd *pc;

    if (banner) printf ("%s:\n", banner);

    for (state = patchCmds; (pc = NextPatchCmd (&state)) != NULL; ) {
        DumpPatchCmd (pc, NULL);
    }
}
