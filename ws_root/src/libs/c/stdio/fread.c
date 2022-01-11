/* @(#) fread.c 96/05/09 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

size_t fread(void *ptr, size_t size, size_t nitems, FILE *file)
{
    int          readin=0, x;
    char        *ptrchar = (char *) ptr;

    /* If this descriptor is referencing a RawFile ... */
    if (file->siof_Flags & SIOF_RAWFILE)
    {
        /* I've special-cased files here, 'cause it's more efficient
         * than depending on the fall-back fgetc() based routine
         */
        for (x=0; x < nitems; x++)
        {
            Err      ret;
            
            ret = ReadRawFile(file->siof_RawFile, ptrchar, size);
            if (ret < 0)
            {
                errno = ret;
                return(EOF);
            }
    
            ptrchar += size;        /* Bump up the pointer ... */
            readin += ret;          /* Keep track of total read */
    
            if (ret != size)        /* If we didn't get as much data 
                                       as we asked for, stop now */
                break;
        }
        return(readin); 
    }

    /* If this descriptor is referencing the console ... */
    if (file->siof_Flags & SIOF_CONSOLE)
    {
        /* For the console, we simply depend on fgetc()
         * working.  (Albeit slowly)
         */
        int         r = 0;

        for (x=0; x < nitems; x++)
        {
            int     y;
    
            for (y=0; y < size; y++)
            {
                r = fgetc(file);
                if (r == EOF)
                    break;
                *ptrchar++ = (uint8) r;
                readin++;
            }
            if (r == EOF)
                break;
        }
        return(readin);
    }

    /* If descriptor doesn't represent any device we know ... */
    errno = C_ERR_NOTSUPPORTED;
    return(EOF);
}


