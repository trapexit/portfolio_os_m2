/* @(#) path.c 96/09/27 1.3 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/devicecmd.h>
#include <file/fsutils.h>
#include <string.h>


/*****************************************************************************/


Err AppendPath(char *path, const char *append, uint32 numBytes)
{
uint32 pathLen;
uint32 appendLen;

    appendLen = strlen(append);
    if (append[0] == '/')
    {
        /* absolute path, replace the original */
        pathLen = 0;
    }
    else
    {
        pathLen = strlen(path);
        while ((pathLen > 1) && (path[pathLen - 1] == '/'))
        {
            /* ignore any trailing /, but not if the path is only a / */
            pathLen--;
        }
    }

    if (pathLen + 1 + appendLen + 1 >= numBytes)
        return -1;

    if (pathLen > 1)
        path[pathLen++] = '/';

    strcpy(&path[pathLen],append);

    return pathLen;
}


/*****************************************************************************/


char *FindFinalComponent(const char *path)
{
uint32 pathLen;

    pathLen = strlen(path);

    /* remove any trailing / */
    while ((pathLen > 0) && (path[pathLen - 1] == '/'))
        pathLen--;

    while (TRUE)
    {
        if (path[pathLen] == '/')
            return &path[pathLen + 1];

        if (pathLen == 0)
            return path;

        pathLen--;
    }
}


/*****************************************************************************/


Err GetPath(Item file, char *path, uint32 numBytes)
{
Item   ioreq;
IOInfo ioInfo;
Err    result;

    ioreq = result = CreateIOReq(NULL, 0, file, 0);
    if (ioreq >= 0)
    {
        memset(&ioInfo, 0, sizeof(ioInfo));
        ioInfo.ioi_Command         = FILECMD_GETPATH;
        ioInfo.ioi_Recv.iob_Buffer = path;
        ioInfo.ioi_Recv.iob_Len    = numBytes;

        result = DoIO(ioreq, &ioInfo);

        DeleteIOReq(ioreq);
    }

    return result;
}
