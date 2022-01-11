/* @(#) putc.c 95/10/09 1.1 */

#include <stdio.h>


/*****************************************************************************/


int putc(int c, FILE *file)
{
    return fputc(c, file);
}
