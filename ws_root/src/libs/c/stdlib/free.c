/* @(#) free.c 95/08/29 1.5 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


void free(void *ptr)
{
    FreeMem(ptr,-1);
}
