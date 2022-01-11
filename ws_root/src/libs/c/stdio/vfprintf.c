/* @(#) vfprintf.c 95/10/13 1.3 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/


int vfprintf(FILE *file, const char *fmt, va_list va)
{
    return vcprintf(fmt, (OutputFunc)fputc, file, va);
}
