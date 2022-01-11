/* @(#) taskswitch.c 96/04/16 1.5 */

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/time.h>
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <stdio.h>


/*****************************************************************************/


static Item   thread1;
static uint32 thread1Sig;
static Item   thread2;
static uint32 thread2Sig;
static uint32 masterSig;

#define TEST_TIME 2

/*****************************************************************************/


static void SigThread1(void)
{
    thread1Sig = AllocSignal(0);
    SendSignal(CURRENTTASK->t_ThreadTask->t.n_Item,masterSig);

    while (TRUE)
    {
        WaitSignal(thread1Sig);
        SendSignal(thread2,thread2Sig);
    }
}


/*****************************************************************************/


static void SigThread2(void)
{
    thread2Sig = AllocSignal(0);
    SendSignal(CURRENTTASK->t_ThreadTask->t.n_Item,masterSig);

    while (TRUE)
    {
        WaitSignal(thread2Sig);
        SendSignal(thread1,thread1Sig);
    }
}


/*****************************************************************************/


static void YieldThread(void)
{
    while (TRUE)
    {
        Yield();
    }
}


/*****************************************************************************/


void DoTaskTest(void)
{
Item   timerIO;
uint32 launch1;
uint32 launch2;

    masterSig = AllocSignal(0);
    if (masterSig > 0)
    {
        timerIO = CreateTimerIOReq();
        if (timerIO >= 0)
        {
            printf("Testing task swap using Signals...\n");

            thread1 = CreateThread(SigThread1,"SigThread1",CURRENTTASK->t.n_Priority - 1,1024,NULL);
            if (thread1 >= 0)
            {
                WaitSignal(masterSig);

                thread2 = CreateThread(SigThread2,"SigThread2",CURRENTTASK->t.n_Priority - 1,1024,NULL);
                if (thread2 >= 0)
                {
                    WaitSignal(masterSig);

                    SendSignal(thread1,thread1Sig);
                    WaitTime(timerIO,TEST_TIME,0);

                    launch1 = THREAD(thread1)->t_NumTaskLaunch;
                    launch2 = THREAD(thread2)->t_NumTaskLaunch;
                    printf("avg time per task swap : %4.2f usec\n",((float)TEST_TIME/(float)(launch1+launch2))*1000.0*1000.0);

                    DeleteThread(thread2);
                }
                else
                {
                    printf("CreateThread() failed: ");
                    PrintfSysErr(thread2);
                }
                DeleteThread(thread1);
            }
            else
            {
                printf("CreateThread() failed: ");
                PrintfSysErr(thread1);
            }

            printf("Testing task swap using Yield...\n");

            thread1 = CreateThread(YieldThread,"YieldThread1",CURRENTTASK->t.n_Priority - 1,1024,NULL);
            if (thread1 >= 0)
            {
                thread2 = CreateThread(YieldThread,"YieldThread2",CURRENTTASK->t.n_Priority - 1,1024,NULL);
                if (thread2 >= 0)
                {
                    WaitTime(timerIO,TEST_TIME,0);

                    launch1 = THREAD(thread1)->t_NumTaskLaunch;
                    launch2 = THREAD(thread2)->t_NumTaskLaunch;
                    printf("avg time per task swap : %4.2f usec\n",((float)TEST_TIME/(float)(launch1+launch2))*1000.0*1000.0);

                    DeleteThread(thread2);
                }
                else
                {
                    printf("CreateThread() failed: ");
                    PrintfSysErr(thread2);
                }
                DeleteThread(thread1);
            }
            else
            {
                printf("CreateThread() failed: ");
                PrintfSysErr(thread1);
            }

            DeleteTimerIOReq(timerIO);
        }
        else
        {
            printf("CreateTimerIOReq() failed: ");
            PrintfSysErr(timerIO);
        }
        FreeSignal(masterSig);
    }
    else
    {
        printf("AllocSignal() failed: ");
        PrintfSysErr(masterSig);
    }
}
