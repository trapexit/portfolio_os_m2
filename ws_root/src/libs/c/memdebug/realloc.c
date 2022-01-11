/* @(#) realloc.c 95/09/04 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>
#include <string.h>


/* WARNING: This code doesn't handle the horrible case of being able to
 *          call realloc() on the last freed pointer. This would require
 *          free() not to free the memory immediately, and keep it around.
 */


void *reallocDebug(void *oldBlock, size_t newSize, const char *sourceFile, int sourceLine)
{
void   *newBlock;
size_t  oldSize;

    oldSize = 0;
    if (oldBlock)
    {
        oldSize = GetMemTrackSizeDebug(oldBlock, sourceFile, (uint32)sourceLine);
    }
    else if (newSize == 0)
    {
        return NULL;
    }

    newBlock = AllocMemDebug(newSize, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE, sourceFile, (uint32)sourceLine);
    if (newBlock == NULL)
        return NULL;

    if (oldBlock)
        memcpy(newBlock,oldBlock,newSize < oldSize ? newSize : oldSize);

    FreeMemDebug(oldBlock, TRACKED_SIZE, sourceFile, (uint32)sourceLine);

    return newBlock;
}
