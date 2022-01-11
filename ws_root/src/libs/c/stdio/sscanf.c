/* @(#) sscanf.c 96/05/08 1.2 */

#include <stdio.h>
#include <stdarg.h>
#include "stdioerrs.h"

/*****************************************************************************/


static int ScanfGetC(char **str)
{
char *ptr;

    ptr = *str;
    if (*ptr)
    {
	*str = ptr + 1;
	return (*ptr);
    }

    return (EOF);
}


/*****************************************************************************/


static int ScanfUngetC(char dummy, char **str)
{
    TOUCH(dummy);

    --*str;
    return 0;
}


/*****************************************************************************/


int sscanf(const char *s, const char *fmt, ...)
{
int      result;
va_list  va;

    va_start(va, fmt);
    result = __vfscanf(fmt, va, (GetCFunc) ScanfGetC, (UnGetCFunc) ScanfUngetC, &s);
    va_end(va);

    return result;
}
