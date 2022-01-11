/* @(#) atol.c 95/09/04 1.1 */

#include <stdlib.h>

long int atol(const char *nsptr)
{
    return (long int) strtol(nsptr, NULL, 0);
}
