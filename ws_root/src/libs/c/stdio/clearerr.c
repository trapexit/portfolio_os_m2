/* @(#) clearerr.c 96/05/08 1.2 */

#include <stdio.h>
#include <errno.h>
#include "stdioerrs.h"

/*****************************************************************************/

void clearerr(FILE *file)
{
	if (file->siof_Flags & SIOF_RAWFILE)
	{
		Err		r;

		errno = 0;
		r = ClearRawFileError(file->siof_RawFile);
		if (r < 0)
			errno = r;
		return;
	}

	/* The file descriptor doesn't reference any device type we know */
	/* since this function doesn't have any return values, we can't
	 * tell 'em we failed ...
	 */
}

