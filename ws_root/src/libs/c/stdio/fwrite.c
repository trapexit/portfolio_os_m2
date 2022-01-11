/* @(#) fwrite.c 96/05/09 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *file)
{
    int          r=0, written=0;
    int          x, y;
    char        *ptrchar = (char *) ptr;

    for (y=0; y < nitems; y++)
    {
        for (x=0; x < size; x++)
        {
            /* Boy, if this descriptor doesn't have buffering, this'll 
             * certainly be slow.  This code could be optimized to bulk-move
             * big chunks of data into the buffer, flushing as it goes.
             * But, heck -- if they want performance, use the real OS calls.
             */
            r = fputc(*ptrchar++, file);
            if (r < 0)
                break;
            written++;
        }
    }
    if (r < 0)
        return(r);
    else
        return(written);
}

