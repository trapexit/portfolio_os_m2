/* @(#) strstr.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <string.h>


/*****************************************************************************/


char *strstr(const char *str, const char *subString)
{
const char *s;
const char *sub;

    if (*subString == 0)
        return (char *)str;

    while (*str)
    {
        s   = str;
        sub = subString;
        while (*sub && (*s == *sub))
        {
            s++;
            sub++;
        }

        if (*sub == 0)
            return (char *)str;

	str++;
    }

    return NULL;
}
