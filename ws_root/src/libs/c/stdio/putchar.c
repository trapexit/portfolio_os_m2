/* @(#) putchar.c 95/10/09 1.1 */

#include <stdio.h>


/*****************************************************************************/


int putchar(int c)
{
    return fputc(c, stdout);
}
