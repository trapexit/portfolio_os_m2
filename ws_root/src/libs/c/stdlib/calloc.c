/* @(#) calloc.c 95/08/29 1.5 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <string.h>


void *calloc(size_t nelem, size_t elsize)
{
    return AllocMem(nelem * elsize, MEMTYPE_ANY | MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
}


