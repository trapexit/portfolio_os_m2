/* @(#) setbuf.c 95/10/08 1.1 */

#include <stdio.h>


void setbuf(FILE *file, char *buf)
{
    if (buf)
	setvbuf(file, buf, _IOFBF, BUFSIZ);
    else
	setvbuf(file, NULL, _IONBF, 0);
}
