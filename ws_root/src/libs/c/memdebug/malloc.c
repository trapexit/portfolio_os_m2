/* @(#) malloc.c 95/09/04 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


void *mallocDebug(size_t size, const char *sourceFile, int sourceLine)
{
    return AllocMemDebug(size,MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE, sourceFile, (uint32)sourceLine);
}
