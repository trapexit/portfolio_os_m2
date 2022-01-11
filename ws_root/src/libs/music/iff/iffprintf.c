/****************************************************************************
**
**  @(#) iffprintf.c 96/02/23 1.2
**
****************************************************************************/


#ifdef BUILD_STRINGS    /* { */


#include <audio/music_iff.h>        /* Self */
#include <stdarg.h>
#include <stdio.h>


static void PrintIndent (const ContextNode *);


/**
|||	AUTODOC -private -class libmusic -group IFF -name IFFPrintf
|||	printf() to console with indentation determined by IFF context stack depth.
|||
|||	  Synopsis
|||
|||	    void IFFPrintf (const IFFParser *iff, const char *fmt, ...)
|||
|||	  Description
|||
|||	    Prints formatted output to console (using printf()) indented as many
|||	    instances of the string ". " as there are nodes in the context stack.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to use.
|||
|||	    fmt
|||	        printf() format.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/music_iff.h>, libmusic.a, System.m2/Modules/iff
|||
|||	  Notes
|||
|||	    This function only enabled when BUILD_STRINGS is defined.
**/

void IFFPrintf (const IFFParser *iff, const char *fmt, ...)
{
    va_list ap;

    PrintIndent (GetCurrentContext(iff));

    va_start (ap, fmt);
    vprintf (fmt, ap);
    va_end (ap);
}

static void PrintIndent (const ContextNode *cn)
{
    for (; cn; cn = GetParentContext(cn)) printf (". ");
}

#else

extern int foo;     /* avoid compiler warning */

#endif
