/* @(#) fsetpos.c 95/10/08 1.1 */

#include <stdio.h>


int fsetpos(FILE *file, const fpos_t *pos)
{
    if (fseek(file, *pos, 0) < 0)
	return EOF;

    return 0;
}
