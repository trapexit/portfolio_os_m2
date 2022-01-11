/* @(#) fprintf.c 95/10/13 1.2 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/


int fprintf(FILE *file, const char *fmt, ...)
{
va_list va;
int     result;

    va_start(va, fmt);
    result = vcprintf(fmt, (OutputFunc)fputc, file, va);
    va_end(va);

    return result;
}
