/* @(#) feof.c 96/05/08 1.2 */

#include <stdio.h>
#include <errno.h>
#include <file/filesystem.h>
#include "stdioerrs.h"

extern int errno;

/*****************************************************************************/

int feof(FILE *file)
{
	errno = 0;
	if (file->siof_Flags & SIOF_RAWFILE)
	{
		FileInfo 	fi;
		Err			ret;

		ret = GetRawFileInfo(file->siof_RawFile, &fi, sizeof(FileInfo));
		if (ret >= 0)
		{
			if (fi.fi_BytePosition == fi.fi_ByteCount)
				return(TRUE);
			else
				return(FALSE);
		}
		errno = ret;
		return(EOF);
		
	}
	if (file->siof_Flags & SIOF_CONSOLE)
		return(FALSE);

	/* If descriptor doesn't represent any device we know ... */
	errno = C_ERR_NOTSUPPORTED;
	return(EOF);	
}

