/* @(#) ferror.c 96/05/08 1.2 */

#include <stdio.h>
#include "stdioerrs.h"

/*****************************************************************************/


int ferror(FILE *file)
{
	FileInfo	i;
	Err 		r;

	if (file->siof_Flags & SIOF_RAWFILE)
	{	
		r = GetRawFileInfo(file->siof_RawFile, &i, sizeof(FileInfo));
		if (r >= 0)
		{
			if (i.fi_Error)
				return(TRUE);
			else
				return(FALSE);
		}
	}
	return(FALSE);
}

