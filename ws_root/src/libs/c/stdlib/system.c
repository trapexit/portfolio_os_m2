/* @(#) system.c 95/10/11 1.4 */

#include <kernel/types.h>
#include <misc/script.h>
#include <errno.h>
#include <stdlib.h>


/*****************************************************************************/


int system(const char *cmdString)
{
Err err;
Err result;

    result = OpenScriptFolio();
    if (result >= 0)
    {
        result = ExecuteCmdLine(cmdString,&err,NULL);
        CloseScriptFolio();
    }

    if (result < 0)
    {
        errno = result;
        return -1;
    }

    return err;
}
