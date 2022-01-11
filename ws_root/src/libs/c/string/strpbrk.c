/* @(#) strpbrk.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


char *strpbrk(const char *str, const char *breaks)
{
const char *ptr;

    while (*str)
    {
	ptr = breaks;
	while (*ptr)
        {
	    if (*ptr == *str)
		return (char *)str;

	    ptr++;
	}
	str++;
    }

    return NULL;
}
