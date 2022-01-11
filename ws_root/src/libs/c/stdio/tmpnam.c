/* @(#) tmpnam.c 95/10/08 1.1 */

#include <kernel/task.h>
#include <stdio.h>


static char name[L_tmpnam];
static int  unique;


char *tmpnam(char *buf)
{
    if (buf == NULL)
	buf = name;

    sprintf(buf, "/remote/%08x-%08x", FindTask(0), unique++);

    return buf;
}
