/* @(#) ftell.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

long int ftell(FILE *file)
{
	Err				r;

	if (file->siof_Flags & SIOF_RAWFILE)
	{
		r = SeekRawFile(file->siof_RawFile, 0, FILESEEK_CURRENT);
		if (r >= 0)
			return(r);

		errno = r;
	    return EOF;
	}

	/* If descriptor doesn't represent any device we know ... */
	errno = C_ERR_NOTSUPPORTED;
	return(EOF);	
}

