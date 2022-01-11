/* @(#) placeholder.c 96/05/08 1.2 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

char *gets(char *s)
{
    TOUCH(s);
    printf("gets() not implemented\n");
    return NULL;
}


