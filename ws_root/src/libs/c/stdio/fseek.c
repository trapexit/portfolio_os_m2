/* @(#) fseek.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int fseek(FILE *file, long int offset, int relative)
{
	Err				r;

	/* As from ANSI 7.9.8.1, about characters put back on the
	 * stream with ungetc():
	 * Notice that any file position requests nullify the
	 * effects of ungetc().
	 */
	file->siof_Flags &= (~SIOF_UNGOTCHAR);


	if (file->siof_Flags & SIOF_RAWFILE)
	{
		r = SeekRawFile(file->siof_RawFile, offset, relative);
		if (r >= 0)
			return(0);
		else
		{
			errno = r;
			return(EOF);
		}
	}

	/* If descriptor doesn't represent any device we know ... */
	errno = C_ERR_NOTSUPPORTED;
	return(EOF);	
}

