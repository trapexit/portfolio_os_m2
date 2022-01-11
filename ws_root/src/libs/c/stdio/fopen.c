/* @(#) fopen.c 96/05/09 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

FILE *fopen(const char *filename, const char *mode)
{
    FILE            *f;

    /* Create a File Handle with default buffering */
    f = CreateFH(_IOLBF, NULL, BUFSIZ);
    if (f)
    {
        FILE        *rf;

        /* Use freopen() for the core of the open */
        rf = freopen(filename, mode, f);

        /* If it worked, great!  Exit. */
        if (rf)
            return(rf);

        /* It didn't.  Clean up. */
        DeleteFH(f);
    }
    return NULL;
}

FILE *freopen(const char *filename, const char *mode, FILE *f)
{
    char            *plus;
    bool             seektoend = FALSE;
    FileOpenModes    orfmode;
    Err              r;

    /* If someone tries to freopen() on a system-owned descriptor, 
     * like stdin, stdout, stderr, complain.
     */
    if (f->siof_Flags & SIOF_OWNEDSYSTEM)
    {
        errno = C_ERR_SYSTEMDESCRIPT;
        return(NULL);
    }

    /* Close any part of f that's still open */
    closenodelete(f);

    /* Parse out the mode string */
    plus = strchr(mode,'+');
    if (strchr(mode,'r'))
    {
        if (plus)
            orfmode = FILEOPEN_READWRITE;
        else
            orfmode = FILEOPEN_READ;
    }
    else if (strchr(mode, 'w'))
    {
        if (plus)
            orfmode = FILEOPEN_WRITE_NEW;
        else
            orfmode = FILEOPEN_WRITE;
    }
    else if (strchr(mode, 'a'))
    {
        if (plus)
            orfmode = FILEOPEN_WRITE;
        else
            orfmode = FILEOPEN_WRITE;
        seektoend = TRUE;
    }
    else
    {
        /* If we couldn't make any sense out of the mode string */
        errno = C_ERR_SYSTEMDESCRIPT;
        return(NULL);
    }

    /* In the event this file should get autodeleted, they'll need */
    /* The source filename ... */
    strncpy(&f->siof_AutoDeleteFilename[0], filename, 79);
    f->siof_AutoDeleteFilename[79]=0;

    /* For the moment, presume that they're trying to fopen() a 
     * RawFile.
     */
    r = OpenRawFile(&f->siof_RawFile, filename, orfmode);
    if (r >= 0)
    {
        f->siof_Mode = orfmode;
        f->siof_Flags |= SIOF_RAWFILE;

        /* If an append mode, move to the end of the file */
        if (seektoend)
            r = SeekRawFile(f->siof_RawFile, 0, FILESEEK_END);

        if (r >= 0)
        {
            return(f);
        }
    }
    errno = r;

    return NULL;
}

