/* @(#) free.c 95/09/04 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <stdlib.h>


void freeDebug(void *ptr, const char *sourceFile, int sourceLine)
{
    FreeMemDebug(ptr, TRACKED_SIZE, sourceFile, (uint32)sourceLine);
}
