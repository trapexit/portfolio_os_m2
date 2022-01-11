/* @(#) calloc.c 95/09/04 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


void *callocDebug(size_t nelem, size_t elsize, const char *sourceFile, int sourceLine)
{
    return AllocMemDebug(nelem * elsize, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL,
                         sourceFile, (uint32)sourceLine);
}
