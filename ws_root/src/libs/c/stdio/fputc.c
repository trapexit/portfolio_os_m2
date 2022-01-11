/* @(#) fputc.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int fputc(int cin, FILE *file)
{
	unsigned char c = cin;

	/* Buffer it? */
	if (file->siof_Buffer)
	{
		file->siof_Buffer[file->siof_BufferActual++]=c;
		/* Flush it if in line mode and a newline char */
		/* OR if the buffer is full */
		if ( (((file->siof_Flags & SIOF_TYPE_MASK)==SIOF_TYPE_LINEMODE) && (c == '\n')) ||
			 (file->siof_BufferActual == file->siof_BufferLength) )
		{
			int 	r;
			r = fflush(file);
			if (r < 0)
				return(r);
		}
	}
	else
	{
		/* No buffering, just dump the data */
		if (file->siof_Flags & SIOF_RAWFILE)
		{
			Err		r;
			r = WriteRawFile(file->siof_RawFile, &c, 1);
			if (r >= 0)
				return(0);
			errno = r;
			return(EOF);
		}
		if (file->siof_Flags & SIOF_CONSOLE)
		{
			DebugPutChar(c);
			return(0);
		}
		
		/* If descriptor doesn't represent any device we know ... */
		errno = C_ERR_NOTSUPPORTED;
		return(EOF);
	}

    return 0;
}

