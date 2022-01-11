/* @(#) strspn.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


size_t strspn(const char *str, const char *set)
{
const char *ptr;

    ptr = str;
    while (*ptr)
    {
        if (strchr(set,*ptr) == NULL)
            break;

        ptr++;
    }

    return (ptr - str);
}
