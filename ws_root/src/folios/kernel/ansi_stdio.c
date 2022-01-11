/* @(#) ansi_stdio.c 96/03/29 1.1 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/


typedef struct SprintfData
{
    char *sd_Dest;
} SprintfData;


/*****************************************************************************/


static int sprintfputc(char c, SprintfData *sd)
{
    *sd->sd_Dest++ = c;
    return c;
}


/*****************************************************************************/


int sprintf(char *buff, const char *fmt, ...)
{
SprintfData sd;
va_list     va;
int         result;

    sd.sd_Dest = buff;

    va_start(va, fmt);
    result = vcprintf(fmt, (OutputFunc)sprintfputc, &sd, va);
    sprintfputc(0, &sd);
    va_end(va);

    return result;
}


/*****************************************************************************/


int printf(const char *fmt, ...)
{
va_list va;
int32   result;

    va_start(va, fmt);
    result = vcprintf(fmt, NULL, NULL, va);
    va_end(va);

    return result;
}
