/* @(#) lumberjack.c 95/12/05 1.7 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/device.h>
#include <kernel/io.h>
#include <kernel/lumberjack.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


static void ThreadFunction(void)
{
    while (1)
    {
    }
}


/*****************************************************************************/


int main(void)
{
LumberjackBuffer *lb;
Item              sem;

    printf("CreateLumberjack() returned %d\n",CreateLumberjack(NULL));

    ControlLumberjack(LOGF_CONTROL_ITEMS);
    DeleteMsgPort(CreateMsgPort("TestPort",0,0));

    ControlLumberjack(LOGF_CONTROL_TASKS);
    DeleteThread(CreateThread(ThreadFunction,"TestThread",0,2048,NULL));

    ControlLumberjack(LOGF_CONTROL_PAGES);
    ControlMem(AllocMemPages(100000,MEMTYPE_ANY),100000,MEMC_GIVE,0);

    ControlLumberjack(LOGF_CONTROL_INTERRUPTS);
    ControlLumberjack(0);

    ControlLumberjack(LOGF_CONTROL_USER);
    LogEvent("Yo!");

    ControlLumberjack(LOGF_CONTROL_SEMAPHORES);
    sem = CreateSemaphore("TestSem",0);
    LockSemaphore(sem,SEM_WAIT);
    UnlockSemaphore(sem);

    ControlLumberjack(LOGF_CONTROL_IOREQS | LOGF_CONTROL_SIGNALS);
    {
    Item    ioreqItem;
    TimeVal tv;
    IOInfo  ioInfo;

            ioreqItem = CreateTimerIOReq();
            if (ioreqItem >= 0)
            {
                memset(&ioInfo,0,sizeof(ioInfo));
                ioInfo.ioi_Command         = TIMERCMD_GETTIME_USEC;
                ioInfo.ioi_Recv.iob_Buffer = &tv;
                ioInfo.ioi_Recv.iob_Len    = sizeof(tv);

                SendIO(ioreqItem,&ioInfo);
                WaitSignal(SIGF_IODONE);
                DeleteTimerIOReq(ioreqItem);
            }
    }

    ControlLumberjack(0);
    printf("Dumping Lumberjack buffers\n");

    while (lb = ObtainLumberjackBuffer())
    {
        DumpLumberjackBuffer(NULL,lb);
        ReleaseLumberjackBuffer(lb);
    }

    printf("DeleteLumberjack() returned %d\n",DeleteLumberjack());

    return 0;
}
