/* @(#) vprintf.c 95/10/13 1.10 */

#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>


int vprintf(const char *fmt, va_list va)
{
    return vcprintf(fmt, NULL, NULL, va);
}
