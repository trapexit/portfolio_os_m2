/* @(#) fgets.c 96/05/08 1.1 */

#include "stdioerrs.h"
#include <kernel/debug.h>
#include <file/filefunctions.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*****************************************************************************/

char *fgets(char *s, int n, FILE *file)
{
	char 	*out=s;

	*out=0;					/* initialize string to 0 length */
	while ( (n) && (!feof(file)) )
	{
		char 	c;
	
		c = fgetc(file);
		*out++ = c;
		*out = 0;
		if (c == '\n')
			break;
		n--;
	}
	if (feof(file))
		return(NULL);
	else
		return(s);
}

