/* @(#) remove.c 95/10/08 1.1 */

#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <stdio.h>


int remove(const char *filename)
{
Err err;

    err = DeleteFile(filename);
    if (err == FILE_ERR_NOTAFILE)
    {
        err = DeleteDirectory(filename);
        if (err == FILE_ERR_NOTADIRECTORY)
        {
            /* if neither a file or directory, reset the original error code */
            err = FILE_ERR_NOTAFILE;
        }
    }

    if (err >= 0)
        err = 0;

    return (int)err;
}
