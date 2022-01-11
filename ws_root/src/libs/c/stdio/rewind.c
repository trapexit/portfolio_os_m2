/* @(#) rewind.c 95/10/08 1.1 */

#include <stdio.h>


void rewind(FILE *file)
{
    fseek(file, 0, SEEK_SET);
    clearerr(file);
}
