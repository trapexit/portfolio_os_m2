/* @(#) strcspn.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


size_t strcspn(const char *str, const char *set)
{
char *ptr;

    ptr = strpbrk(str, set);
    if (ptr)
	return (ptr - str);

    return strlen(str);
}
