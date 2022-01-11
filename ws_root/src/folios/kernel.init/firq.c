/* @(#) firq.c 96/07/10 1.8 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/folio.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/task.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <kernel/panic.h>
#include <hardware/bda.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>


/*****************************************************************************/


extern void KernelVBLHandler(void);
extern void KernelCDEHandler(void);
extern void ServiceMasterExceptions(void);

extern uint32 _InterruptStack[INTSTACKSIZE/sizeof(uint32)];


/*****************************************************************************/


Item AllocFirq(char *name, int pri, int num, void (*code)())
{
TagArg tags[5];

    tags[0].ta_Tag = TAG_ITEM_NAME;
    tags[0].ta_Arg = (void *)name;
    tags[1].ta_Tag = TAG_ITEM_PRI;
    tags[1].ta_Arg = (void *)pri;
    tags[2].ta_Tag = CREATEFIRQ_TAG_NUM;
    tags[2].ta_Arg = (void *)num;
    tags[3].ta_Tag = CREATEFIRQ_TAG_CODE;
    tags[3].ta_Arg = (void *)code;
    tags[4].ta_Tag = TAG_END;

    return SuperInternalCreateFirq((FirqNode *)SuperAllocNode((Folio *) KernelBase, FIRQNODE),
                                   tags);
}


/*****************************************************************************/


void start_firq(void)
{
    KB_FIELD(kb_InterruptStack) = _InterruptStack + sizeof(_InterruptStack)/sizeof(uint32) - 2;

    AllocFirq("Kernel", 250, INT_BDA_CDE, KernelCDEHandler);
    AllocFirq("Kernel", 0, INT_V1, KernelVBLHandler);
    AllocFirq("Kernel", 0, 0, ServiceMasterExceptions);

    if (KB_FIELD(kb_NumCPUs) > 1)
    {
        /* once the proper handlers are in place, MP can be enabled... */
        KB_FIELD(kb_Flags) |= KB_MPACTIVE;
    }

    /* enable VBLs */
    BDA_WRITE(BDAVDU_VINT, (BDA_READ (BDAVDU_VINT) & ~BDAVLINE1_MASK) | 0x10);
}
