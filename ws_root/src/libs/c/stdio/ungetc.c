/* @(#) ungetc.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

int ungetc(int c, FILE *file)
{

	/* If c == EOF, do nothing */
	if (c == EOF)
		return(EOF);

	/* If one is already there, fail */
	if (file->siof_Flags & SIOF_UNGOTCHAR)
	{
		return(EOF);
	}
	/* otherwise, store it away and set the flag */
	file->siof_UngotChar = c;
	file->siof_Flags |= SIOF_UNGOTCHAR;

    return(c);
}

