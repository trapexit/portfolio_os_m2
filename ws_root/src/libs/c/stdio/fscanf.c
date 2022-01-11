/* @(#) fscanf.c 96/05/08 1.2 */

#include <stdio.h>
#include <stdarg.h>
#include "stdioerrs.h"


/*****************************************************************************/


int fscanf(FILE *file, const char *format, ...)
{
int      result;
va_list  va;

    va_start(va, fmt);
    result = __vfscanf(format, va, (GetCFunc) fgetc, (UnGetCFunc) ungetc, file);
    va_end(va);

    return result;
}
