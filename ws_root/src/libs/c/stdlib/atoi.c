/* @(#) atoi.c 95/09/04 1.6 */

#include <stdlib.h>

int atoi(const char *nsptr)
{
    return (int) strtol(nsptr, NULL, 0);
}
