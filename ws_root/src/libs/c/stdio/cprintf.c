/* @(#) cprintf.c 95/10/13 1.1 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/


int cprintf(const char *fmt, OutputFunc of, void *userData, ...)
{
va_list va;
int     result;

    va_start(va, fmt);
    result = vcprintf(fmt, of, userData, va);
    va_end(va);

    return result;
}
