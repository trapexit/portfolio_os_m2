/* @(#) malloc.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


void *malloc(size_t size)
{
    return AllocMem(size,MEMTYPE_ANY | MEMTYPE_TRACKSIZE);
}
