/* @(#) fflush.c 96/05/09 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

#define MIN(x,y) ( (x) < (y) ? (x) : (y) )

/* 
 * DebugWrite() -- Dump data to the console, but permit multiple 
 *                 strings to be dumped at once.  Presumes that all
 *                 data going to the console is textual.
 */
void DebugWrite(uint8 *data, uint32 length)
{
    int     x=0;
    char    oneline[81];

    while (x != length)
    {
        int     l;
        
        l = MIN(80, length-x);
        memcpy(&oneline[0], &data[x], l);
        oneline[l]=0;
        DebugPutStr(oneline);
        x += l;
    }   
}


int fflush(FILE *file)
{
    Err     r = 0;

    /* Is this descriptor referencing a RawFile? */
    if (file->siof_Flags & SIOF_RAWFILE)
    {
        /* Flush only if the buffer has data in it ... */
        if (file->siof_BufferActual)
        {

            /* 
             * Flushing only has meaning to this code if it's
             * a writable file of some sort.
             */
            if ( (file->siof_Mode == FILEOPEN_WRITE) ||
                 (file->siof_Mode == FILEOPEN_WRITE_NEW) ||
                 (file->siof_Mode == FILEOPEN_WRITE_EXISTING) ||
                 (file->siof_Mode == FILEOPEN_READWRITE) ||
                 (file->siof_Mode == FILEOPEN_READWRITE_NEW) ||
                 (file->siof_Mode == FILEOPEN_READWRITE_EXISTING) )
            {
                r = WriteRawFile(file->siof_RawFile, file->siof_Buffer,
                                 file->siof_BufferActual);
    
                file->siof_BufferActual = 0;
            }
        }

        /* If things didn't go well, report that ... */
        if (r < 0)
        {
            errno = r;
            return(EOF);
        }
        else
            return(0);
    }

    /* Is this descriptor referencing the Console? (stdin, stdout, stderr) */
    if (file->siof_Flags & SIOF_CONSOLE)
    {
        /* Flush only if the buffer has data in it ... */
        if (file->siof_BufferActual)
        {
                DebugWrite(file->siof_Buffer, file->siof_BufferActual);
    
                file->siof_BufferActual = 0;
        }

        return(0);
    }

    /* If descriptor doesn't represent any device we know ... */
    errno = C_ERR_NOTSUPPORTED;
    return(EOF);    
}

