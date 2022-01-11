/* @(#) strrchr.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


char *strrchr(const char *str, int searchChar)
{
const char *ptr;

    ptr = str;
    while (*ptr)
        ptr++;

    while (TRUE)
    {
        if (*ptr == searchChar)
            return (char *)ptr;

        if (ptr == str)
            return NULL;

        ptr--;
    }
}
