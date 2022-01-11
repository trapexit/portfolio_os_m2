/* @(#) syncstress.c 96/02/26 1.7 */

/**
|||	AUTODOC -class Shell_Commands -name SyncStress
|||	Put some stress of task synchronization code to
|||	uncover title bugs.
|||
|||	  Format
|||
|||	    SyncStress [-quit]
|||
|||	  Description
|||
|||	    This program is used to test the strength of the task
|||	    synchronization code in a title.
|||
|||	    To use this program, simply invoke it from the shell. The
|||	    program will setup conditions to cause random task switches to
|||	    occur in the system. This will likely uncover any incorrect
|||	    synchronization code that may exist in a title.
|||
|||	    It is a bad idea to loosely synchronize the execution of
|||	    multiple threads purely based on the task switching algorithm
|||	    of the system. This generally causes the code to break if a
|||	    task switch occurs at an unexpected moment. Proper sync methods
|||	    should be used, including signals, messages, and semaphores.
|||
|||	  Arguments
|||
|||	    [-quit]
|||	        Instructs a previously invoked instance of the program to exit.
|||
|||	  Implementation
|||
|||	    Command implemented in V27.
|||
|||	  Location
|||
|||	    System.m2/Programs/SyncStress
**/

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/item.h>
#include <kernel/time.h>
#include <kernel/msgport.h>
#include <kernel/random.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


#define Error(x,err)         {printf(x); PrintfSysErr(err);}
#define SYNCSTRESS_PORTNAME  "SyncStress"


/*****************************************************************************/


/* packet of options passed from main code to main loop */
typedef struct OptionPacket
{
    bool   op_Quit;
} OptionPacket;


/*****************************************************************************/


static bool quit;
static Item optPort;


/*****************************************************************************/


static void ApplyOP(OptionPacket *op)
{
    quit = op->op_Quit;
}


/*****************************************************************************/


uint32 idleCount;

static void RandomThread(void)
{
Item   timerIO;
uint32 micros;

    timerIO = CreateTimerIOReq();
    if (timerIO >= 0)
    {
        while (TRUE)
        {
            micros = (ReadHardwareRandomNumber() % 90000) + 25000;
            WaitTime(timerIO,0,micros);
        }
    }
}


/*****************************************************************************/


static void MainLoop(OptionPacket *op)
{
Item     msgItem;
Message *msg;
Item     timerIO;
uint32   seconds;
uint32   micros;
Item     child;

    TOUCH(op);

    timerIO = CreateTimerIOReq();
    child = CreateThread(RandomThread, "SyncStress Randomite", 0, 1024, NULL);

    quit = FALSE;

    SetItemPri(CURRENTTASK->t.n_Item,200);

    while (!quit)
    {
        seconds = (ReadHardwareRandomNumber() % 4);
        micros  = (ReadHardwareRandomNumber() % 1000000);
        WaitTime(timerIO,seconds,micros);

        msgItem = GetMsg(optPort);
        if (msgItem > 0)
        {
            msg = (Message *)LookupItem(msgItem);
            ApplyOP((OptionPacket *)msg->msg_DataPtr);
            ReplyMsg(msgItem,0,NULL,0);
        }

        SetItemPri(child,(ReadHardwareRandomNumber() % 20) + 90);
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int          parm;
OptionPacket op;
Item         repPort;
Item         msg;
Item         err;

    memset(&op,0,sizeof(op));

    for (parm = 1; parm < argc; parm++)
    {
        if ((strcmp("-help",argv[parm]) == 0)
         || (strcmp("-?",argv[parm]) == 0)
         || (strcmp("help",argv[parm]) == 0)
         || (strcmp("?",argv[parm]) == 0))
        {
            printf("syncstress: stress task synchronization code to find bugs\n");
            printf("  -quit - quit the program\n");
            return (0);
        }

        if (strcmp("-quit",argv[parm]) == 0)
        {
            op.op_Quit = TRUE;
        }
    }

    /* Now that the options are parsed, we need to figure out what to do
     * with them.
     *
     * This code checks to see if another version of SyncStress is already
     * running. If it is already running, then a message is sent to it with the
     * new options. If SyncStress is not already running, we initialize
     * the universe, and jump into the main loop.
     */

    optPort = FindMsgPort(SYNCSTRESS_PORTNAME);
    if (optPort >= 0)
    {
        repPort = CreateMsgPort(NULL,0,0);
        if (repPort >= 0)
        {
            msg = CreateMsg(NULL,0,repPort);
            if (msg >= 0)
            {
                err = SendMsg(optPort,msg,&op,sizeof(OptionPacket));
                if (err >= 0)
                {
                    WaitPort(repPort,msg);
                }
                else
                {
                    Error("Unable to send a message: ",err);
                }
            }
            else
            {
                Error("Unable to create a message: ",msg);
            }
            DeleteMsg(msg);
        }
        else
        {
            Error("Unable to create a message port: ",repPort);
        }
        DeleteMsgPort(repPort);
    }
    else
    {
        optPort = CreateMsgPort(SYNCSTRESS_PORTNAME,0,0);
        if (optPort >= 0)
        {
            MainLoop(&op);
            DeleteMsgPort(optPort);
        }
        else
        {
            Error("Unable to create message port: ",optPort);
        }
    }

    return 0;
}
