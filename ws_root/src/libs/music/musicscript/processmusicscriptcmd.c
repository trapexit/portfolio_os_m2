/******************************************************************************
**
**  @(#) processmusicscriptcmd.c 95/11/27 1.1
**
**  musicscript command processor
**
******************************************************************************/

#include <audio/musicerror.h>
#include <string.h>

#include "musicscript_internal.h"

static const MusicScriptCmdInfo *LookupCmd (const MusicScriptCmdInfo *cmdTable, const char *matchcmd);

/**
|||	AUTODOC -private -class libmusic -group MusicScript -name ProcessMusicScriptCmd
|||	Process command in MusicScriptParser's line buffer.
|||
|||	  Synopsis
|||
|||	    Err ProcessMusicScriptCmd (const MusicScriptParser *msp,
|||	                               const MusicScriptCmdInfo *cmdTable,
|||	                               void *userData)
|||
|||	  Description
|||
|||	    Looks up the MusicScriptParser's currently buffered command (as set by last
|||	    call to ReadMusicScriptCmd()) in the supplied command table, checks for
|||	    minimum number of arguments, calls msci_Callback function.
|||
|||	    This function and the msci_Callback functions print diagnostic error
|||	    messages on failure.
|||
|||	  Arguments
|||
|||	    msp
|||	        MusicScriptParser containing buffered command line to process.
|||
|||	    cmdTable
|||	        NULL terminated array of MusicScriptCmdInfos containing command set. Can
|||	        be different with each call.
|||
|||	    userData
|||	        User data pointer to be passed to selected msci_Callback function.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, Err code on failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
|||
|||	  See Also
|||
|||	    ReadMusicScriptCmd()
**/

Err ProcessMusicScriptCmd (const MusicScriptParser *msp, const MusicScriptCmdInfo *cmdTable, void *userData)
{
    const MusicScriptCmdInfo *msci;
    Err errcode;

        /* return immediately if an error code in NumArgs or empty line */
        /* @@@ this is just a bit of paranoia really */
    if (msp->msp_NumArgs <= 0) return msp->msp_NumArgs;

  #if DEBUG_Parser
    printf ("ProcessMusicScriptCmd '%s' numargs=%d cmdtable=0x%x userdata=0x%x\n", msp->msp_ArgArray[0], msp->msp_NumArgs, cmdTable, userData);
  #endif

        /* lookup command in table */
    if (!(msci = LookupCmd (cmdTable, msp->msp_ArgArray[0]))) {
        errcode = ML_ERR_BAD_COMMAND;
        MSERR((msp, errcode, "Unknown command \"%s\"", msp->msp_ArgArray[0]));
        goto clean;
    }

        /* check # arg count */
    if (msp->msp_NumArgs < msci->msci_MinNumArgs) {
        errcode = ML_ERR_MISSING_ARG;
        MSERR((msp, errcode, "Missing required argument(s) for %s", msci->msci_Cmd));
        goto clean;
    }

        /* dispatch command processor */
        /* these can print syntax error messages, so we don't do that here */
    return msci->msci_Callback (msp, userData, msp->msp_NumArgs, msp->msp_ArgArray);

clean:
    return errcode;
}

/*
    find command. returns NULL on failure.
*/
static const MusicScriptCmdInfo *LookupCmd (const MusicScriptCmdInfo *msci, const char *matchCmd)
{
    for (; msci->msci_Cmd; msci++) {
        if (!strcasecmp (msci->msci_Cmd, matchCmd)) return msci;
    }
    return NULL;
}
