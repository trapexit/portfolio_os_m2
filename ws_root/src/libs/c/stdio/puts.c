/* @(#) puts.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int puts(const char *s)
{
    DebugPutStr(s);
    DebugPutChar('\n');
    return 0;
}

