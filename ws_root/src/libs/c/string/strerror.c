/* @(#) strerror.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <string.h>


/*****************************************************************************/


static char errBuf[128];


char *strerror(int errorCode)
{
    if (errorCode >= 0)
        return NULL;

    GetSysErr(errBuf, sizeof(errBuf), errorCode);
    return errBuf;
}
