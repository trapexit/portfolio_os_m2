/* @(#) memcmp.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


/* This code really needs to be optimized to do word compares if possible... */

int memcmp(const void *mem1, const void *mem2, size_t numBytes)
{
const uint8 *p1 = (char *)mem1;
const uint8 *p2 = (char *)mem2;

    while (TRUE)
    {
        if (!numBytes)
            return 0;

        numBytes--;

        if (*p1 != *p2)
        {
            if (*p1 < *p2)
                return -1;

            return 1;
        }

        p1++;
        p2++;
    }
}
