/* @(#) tmpfile.c 96/05/08 1.2 */

#include <stdio.h>
#include <string.h>
#include "stdioerrs.h"

FILE *tmpfile(void)
{
FILE *file;
char  name[L_tmpnam];

    tmpnam(name);
    if (file = fopen(name, "wb+"))
    {
        file->siof_Flags |= SIOF_AUTODELETE;
        return file;
    }
	return(NULL);
}

