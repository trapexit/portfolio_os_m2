/* @(#) fputs.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int fputs(const char *s, FILE *file)
{
	int 	r=0;
	while (*s)
	{
		r = fputc(*s++,file);
		if (r < 0)
			break;
	}
    return (r);
}

