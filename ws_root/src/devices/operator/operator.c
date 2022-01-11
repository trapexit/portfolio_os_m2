/* @(#) operator.c 96/08/06 1.110 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/dipir.h>
#include <loader/loader3do.h>
#include <stdio.h>


/*****************************************************************************/


extern void OperatorInit(void);
extern void ProvideItemServices(Item serverPort);
extern Err  ReinitDevices(void);


/*****************************************************************************/


void main(void)
{
int32 sigs;
Item  serverPort;
int32 serverSignal;

    RegisterOperator();

    serverPort   = CreateMsgPort("PrivilegedItemServer", 200, 0);
    serverSignal = ((MsgPort *)LookupItem(serverPort))->mp_Signal;

    OperatorInit();

    /* Nuke unused init modules */
    FreeInitModules();

    KernelBase->kb_Flags |= KB_OPERATOR_UP;

    while (TRUE)
    {
        ScavengeMem();

        /* We listen for SIGF_DEADTASK so that we call ScavengeMem() if
         * any thread or ours dies.
         */
        sigs = WaitSignal(RESCAN_SIGNAL | serverSignal | SIGF_DEADTASK);
	if (sigs & serverSignal)
	    ProvideItemServices(serverPort);

	if (sigs & RESCAN_SIGNAL)
	    CallBackSuper(&ReinitDevices, 0, 0, 0);
    }
}
