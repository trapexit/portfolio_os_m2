/* @(#) fclose.c 96/05/09 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

/* closenodelete() is code that's used by more than one routine, so
 * it stands on it's own.  It's job is the core of fclose() -- it closes
 * any files outstanding against a FILE descriptor -- but doesn't try
 * to free the descriptor itself.  It might, for tmpfile(), try to delete
 * the file that had been open, though.
 */
int closenodelete(FILE *file)
{
    Err     r=0;

    /* Is this descriptor referencing a RawFile? */
    if (file->siof_Flags & SIOF_RAWFILE)
    {
        /* If there's any data left in the buffer, flush it out */
        fflush(file);

        /* Ask RawFile code to close the file itself */
        r = CloseRawFile(file->siof_RawFile);
        file->siof_Flags &= (~SIOF_RAWFILE);
        file->siof_RawFile = NULL;
    
        if (r < 0)
            errno = r;
    }

    /* If told to, try to delete this file */
    if (file->siof_Flags & SIOF_AUTODELETE)
        DeleteFile(file->siof_AutoDeleteFilename);

    if (r < 0)
        return(EOF);
    else
        return(0);
}

int fclose(FILE *file)
{
    Err     r;

    /* If someone tries to fclose() a system-owned descriptor, ie,
     * stdin, stdout, stderr, complain that doing so isn't permitted. 
     */
    if (file->siof_Flags & SIOF_OWNEDSYSTEM)
    {
        errno = C_ERR_SYSTEMDESCRIPT;
        return(EOF);
    }

    /* Close any files on this descriptor */
    r = closenodelete(file);

    /* Get rid of the file handle itself */
    DeleteFH(file);

    if (r < 0)
        return(EOF);
    else
        return(0);
}

