/* @(#) cfree.c 95/08/29 1.5 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


int cfree(char *ptr)
{
    free(ptr);
    return 0;
}
