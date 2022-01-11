/* @(#) scanf.c 96/05/08 1.2 */

#include <stdio.h>
#include <stdarg.h>
#include "stdioerrs.h"


/*****************************************************************************/


int scanf(const char *fmt, ...)
{
int     result;
va_list va;

    va_start(va, fmt);
    result = __vfscanf(fmt, va, (GetCFunc) fgetc, (UnGetCFunc) ungetc, stdin);
    va_end(va);

    return result;
}
