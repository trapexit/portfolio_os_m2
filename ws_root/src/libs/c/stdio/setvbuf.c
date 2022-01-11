/* @(#) setvbuf.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int setvbuf(FILE *file, char *buf, int mode, size_t size)
{
	fflush(file);
	
	if (SetFHBuffering(file, mode, buf, size))
		return(0);
	else
    	return EOF;
}

