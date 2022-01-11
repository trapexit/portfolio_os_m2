/* @(#) perror.c 95/10/08 1.1 */

#include <stdio.h>
#include <string.h>
#include <errno.h>


void perror(const char *str)
{
    fprintf(stderr, "%s: %s",str,strerror(errno));
}
