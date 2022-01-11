/* @(#) savelumber.c 95/09/29 1.1 */

/* save a lumberjack log to a file in /remote */

#include <kernel/types.h>
#include <kernel/lumberjack.h>
#include <kernel/debug.h>
#include <file/fileio.h>
#include <stdio.h>


void main(void)
{
Item              timer;
LumberjackBuffer *lb;
RawFile          *file;
Err               err;

    timer = CreateTimerIOReq();

    err = OpenRawFile(&file,"/remote/Lumberjack.log",FILEOPEN_WRITE_NEW);
    if (err < 0)
    {
        printf("Could not open output file: ");
        PrintfSysErr(err);
        return;
    }

    CreateLumberjack(NULL);
    ControlLumberjack(LOGF_CONTROL_SIGNALS | LOGF_CONTROL_IOREQS);
    WaitTime(timer,2,0);
    ControlLumberjack(0);

    while (lb = ObtainLumberjackBuffer())
    {
        err = WriteRawFile(file,lb->lb_BufferData,lb->lb_BufferSize);
        if (err < 0)
        {
            printf("Error writing to file: ");
            PrintfSysErr(err);
        }
        ReleaseLumberjackBuffer(lb);
    }

    err = CloseRawFile(file);
    if (err < 0)
    {
        printf("Error closing output file: ");
        PrintfSysErr(err);
    }

    DeleteLumberjack();
    DeleteTimerIOReq(timer);
}
