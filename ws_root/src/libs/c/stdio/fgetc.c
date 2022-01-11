/* @(#) fgetc.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int fgetc(FILE *file)
{

	/* First, if someone has previously done an ungetc(),
	 * return that character to 'em 
	 */

	if (file->siof_Flags & SIOF_UNGOTCHAR)
	{
		file->siof_Flags &= (~SIOF_UNGOTCHAR);
		return(file->siof_UngotChar);
	}

	if (file->siof_Flags & SIOF_RAWFILE)
	{
		char 	c;
		Err		r;
	
		r = ReadRawFile(file->siof_RawFile, &c, 1);
		if (r == 1)
		{
			errno = 0;
			return(c);
		}
		errno = r;
		return(EOF);
	}
	
	/* If descriptor doesn't represent any device we know ... */
	errno = C_ERR_NOTSUPPORTED;
	return(EOF);	
}

