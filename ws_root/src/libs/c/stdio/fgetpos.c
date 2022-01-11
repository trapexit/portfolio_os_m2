/* @(#) fgetpos.c 95/10/08 1.1 */

#include <stdio.h>


int fgetpos(FILE *file, fpos_t *pos)
{
    *pos = ftell(file);
    if (*pos < 0)
	return EOF;

    return 0;
}
