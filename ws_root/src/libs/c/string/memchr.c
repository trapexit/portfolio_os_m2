/* @(#) memchr.c 96/04/25 1.5 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


void *memchr(const void *mem, int searchChar, size_t numBytes)
{
const char *p = (char *)mem;

    while (numBytes--)
    {
        if (*p == searchChar)
            return (void *)p;

        p++;
    }

    return NULL;
}
