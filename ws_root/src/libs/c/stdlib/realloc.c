/* @(#) realloc.c 95/08/29 1.7 */
/* $Id: realloc.c,v 1.1 1994/04/04 23:03:50 vertex Exp $ */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>
#include <string.h>


/* WARNING: This code doesn't handle the horrible case of being able to
 *          call realloc() on the last freed pointer. This would require
 *          free() not to free the memory immediately, and keep it around.
 */


void *realloc(void *oldBlock, size_t newSize)
{
void   *newBlock;
size_t  oldSize;

    oldSize = 0;
    if (oldBlock)
    {
        oldSize = GetMemTrackSize(oldBlock);
    }
    else if (newSize == 0)
    {
        return NULL;
    }

    newBlock = AllocMem(newSize, MEMTYPE_ANY | MEMTYPE_TRACKSIZE);
    if (newBlock == NULL)
        return NULL;

    if (oldBlock)
        memcpy(newBlock,oldBlock,newSize < oldSize ? newSize : oldSize);

    FreeMem(oldBlock,-1);

    return newBlock;
}
