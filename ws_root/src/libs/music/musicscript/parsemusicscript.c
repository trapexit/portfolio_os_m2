/******************************************************************************
**
**  @(#) parsemusicscript.c 95/11/27 1.2
**
**  musicscript convenience call
**
******************************************************************************/

#include <kernel/operror.h>         /* PrintError() */

#include "musicscript_internal.h"


/**
|||	AUTODOC -private -class libmusic -group MusicScript -name ParseMusicScript
|||	All-in-one music script processing convenience call.
|||
|||	  Synopsis
|||
|||	    Err ParseMusicScript (const char *fileName, int32 lineBufferSize,
|||	                          int32 argArrayElts,
|||	                          const MusicScriptCmdInfo *cmdTable,
|||	                          void *userData)
|||
|||	  Description
|||
|||	    Opens script, reads line, looks up commands and calls command handler
|||	    functions in a single function call.
|||
|||	    This function prints diagnostic error messages.
|||
|||	  Arguments
|||
|||	    fileName
|||	        Name of file to parse.
|||
|||	    lineBufferSize
|||	        The size of the line buffer to allocate. The maximum legal line limit
|||	        after EOL and comment stripping is lineBufferSize - 2.
|||
|||	    argArrayElts
|||	        Number of elements to allocate for the argument array. When filled out,
|||	        this array is NULL terminated, so the maximum number of arguments for a
|||	        script command is argArrayElts - 1.
|||
|||	    cmdTable
|||	        NULL terminated array of MusicScriptCmdInfos containing command set for
|||	        script.
|||
|||	    userData
|||	        User data pointer to be passed to msci_Callback functions.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, Err code on failure.
|||
|||	  Implementation
|||
|||	    Convenience library call implemented in libmusic.a V29.
|||
|||	  Notes -preformatted
|||
|||	    This function is roughly equivalent to:
|||
|||	    Err ParseMusicScript (const char *fileName, int32 lineBufferSize,
|||	        int32 argArrayElts, const MusicScriptCmdInfo *cmdTable, void *userData)
|||	    {
|||	        MusicScriptParser *msp;
|||	        Err errcode;
|||
|||	        if ((errcode = OpenMusicScript (&msp, fileName, lineBufferSize, argArrayElts)) < 0)
|||	            goto clean;
|||
|||	        while ((errcode = ReadMusicScriptCmd (msp)) > 0) {
|||	            if ((errcode = ProcessMusicScriptCmd (msp, cmdTable, userData)) < 0) goto clean;
|||	        }
|||
|||	    clean:
|||	        CloseMusicScript (msp);
|||	        return errcode;
|||	    }
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
|||
|||	  See Also
|||
|||	    OpenMusicScript(), ReadMusicScriptCmd(), ProcessMusicScriptCmd(),
|||	    CloseMusicScript()
**/

Err ParseMusicScript (const char *fileName, int32 lineBufferSize, int32 argArrayElts, const MusicScriptCmdInfo *cmdTable, void *userData)
{
    MusicScriptParser *msp;
    Err errcode;

  #if DEBUG_Parser
    printf ("ParseMusicScript '%s' linebufsize=%d numargs=%d cmdtable=0x%x userdata=0x%x\n", fileName, lineBufferSize, argArrayElts, cmdTable, userData);
  #endif

        /* open file */
    if ((errcode = OpenMusicScript (&msp, fileName, lineBufferSize, argArrayElts)) < 0) {
        PrintError (NULL, "open script", fileName, errcode);
        goto clean;
    }

        /* read lines until 0 (end of file) or error */
        /* (these functions print their own error messages) */
    while ((errcode = ReadMusicScriptCmd (msp)) > 0) {
        if ((errcode = ProcessMusicScriptCmd (msp, cmdTable, userData)) < 0) goto clean;
    }

clean:
    CloseMusicScript (msp);
    return errcode;
}
