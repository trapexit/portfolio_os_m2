/****************************************************************************
**
**  @(#) diagnostics.c 95/11/22 1.1
**
**  musicscript diagnostics
**
****************************************************************************/

#include <kernel/operror.h>
#include <stdarg.h>
#include <stdio.h>
#include "musicscript_internal.h"


/**
|||	AUTODOC -private -class libmusic -group MusicScript -name PrintMusicScriptError
|||	Prints music script diagnostic error message.
|||
|||	  Synopsis
|||
|||	    void PrintMusicScriptError (const MusicScriptParser *msp, Err errcode,
|||	                                const char *msgfmt, ...)
|||
|||	  Description
|||
|||	    Prints a music script diagnostic error message with custom formatting.
|||
|||	  Arguments
|||
|||	    msp
|||	        MusicScriptParser containing file and line number of offending command.
|||
|||	    errcode
|||	        Error code of problem.
|||
|||	    msgfmt
|||	        printf() format (without trailing newline) additional text to print,
|||	        optionally followed by arguments to be substituted into format.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/musicscript.h>, libmusic.a
**/

void PrintMusicScriptError (const MusicScriptParser *msp, Err errcode, const char *msgfmt, ...)
{
    #define MSERR_PREFIX "### "

    printf (MSERR_PREFIX "\"%s\" line %ld: ", msp->msp_FileName, msp->msp_LineNum);

    if (msgfmt) {
        va_list ap;

        va_start (ap, msgfmt);
        vprintf (msgfmt, ap);
        va_end (ap);
        printf ("\n" MSERR_PREFIX);
    }

    PrintfSysErr (errcode);     /* prints its own newline */
}
