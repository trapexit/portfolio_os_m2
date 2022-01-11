/* @(#) rename.c 96/04/29 1.2 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <stdio.h>


int rename(const char *old, const char *new)
{
    return Rename(old, new);
}
