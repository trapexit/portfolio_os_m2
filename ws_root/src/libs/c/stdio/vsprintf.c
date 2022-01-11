/* @(#) vsprintf.c 95/10/13 1.8 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


/*****************************************************************************/


typedef struct SprintfData
{
    char *sd_Dest;
} SprintfData;


/*****************************************************************************/


static int vsprintfputc(char c, SprintfData *sd)
{
    *sd->sd_Dest++ = c;
    return c;
}


/*****************************************************************************/


int vsprintf(char *buff, const char *fmt, va_list va)
{
SprintfData sd;
int         result;

    sd.sd_Dest = buff;

    result = vcprintf(fmt, (OutputFunc)vsprintfputc, &sd, va);
    vsprintfputc(0,&sd);

    return result;
}
