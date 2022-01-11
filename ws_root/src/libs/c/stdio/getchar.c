/* @(#) getchar.c 95/10/09 1.1 */

#include <stdio.h>


/*****************************************************************************/


int getchar(void)
{
    return fgetc(stdin);
}
