/* @(#) firq.c 96/08/23 1.67 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/nodes.h>
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
#include <kernel/panic.h>
#include <kernel/bitarray.h>
#include <kernel/internalf.h>
#include <hardware/bda.h>
#include <hardware/cde.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>


/*****************************************************************************/


#define VALIDATE

uint32 _InterruptStack[INTSTACKSIZE/sizeof(uint32)];


/*****************************************************************************/


static int32 icf_c(FirqNode *firq, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
        case CREATEFIRQ_TAG_DATA: firq->firq_Data = arg;
                                  break;

        case CREATEFIRQ_TAG_CODE: firq->firq_Code = (void *)arg;
                                  break;

        case CREATEFIRQ_TAG_NUM : firq->firq_Num = arg;
                                  break;

        default                 : return BADTAG;
    }

    return 0;
}


/*****************************************************************************/


Item internalCreateFirq(FirqNode *firq, TagArg *tagpt)
{
int32     num;
Node     *n;
Task     *t = CURRENTTASK;
uint32    oldints;
FirqList *fl;
Err       result;

    if (t && ((t->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0))
        return BADPRIV;

    firq->firq_Num = -1;
    result = TagProcessor(firq, tagpt, icf_c, 0);
    if (result < 0)
        return result;

    num = firq->firq_Num;

#ifdef BUILD_PARANOIA
    if ((firq->firq.n_Name == NULL)
     || (firq->firq_Code == NULL)
     || (num < 0)
     || (num >= INTR_MAX))
    {
        return BADTAGVAL;
    }
#endif

    n = KB_FIELD(kb_InterruptHandlers)[num];

    if (n == NULL)
    {
        KB_FIELD(kb_InterruptHandlers)[num] = (Node *) firq;
    }
    else
    {
        if (n->n_Type != FIRQLISTNODE)
        {                                       /* must allocate a list */
            fl = (FirqList *) AllocateNode ((Folio *) &KB, FIRQLISTNODE);
            if (fl == NULL)
                return NOMEM;

            PrepList(&fl->fl_Firqs);

            oldints = Disable();
            KB_FIELD(kb_InterruptHandlers)[num] = (Node *) fl;
            /* Put old node on new list */
            InsertNodeFromTail(&fl->fl_Firqs, n);
            n = (Node *)fl;
            Enable(oldints);
        }
        oldints = Disable();
        InsertNodeFromTail(&((FirqList *)n)->fl_Firqs, (Node *) firq);
        Enable(oldints);
    }

    return firq->firq.n_Item;
}


/*****************************************************************************/


int32 internalDeleteFirq(FirqNode *f, Task *t)
{
uint32  oldints;
Node   *n;

    TOUCH(t);

    n = KB_FIELD(kb_InterruptHandlers)[f->firq_Num];

    oldints = Disable();
    if (n == (Node *) f)
    {
        /* single handler */

        KB_FIELD(kb_InterruptHandlers)[f->firq_Num] = NULL;
        DisableInterrupt(f->firq_Num);
    }
    else
    {
        /* list of handlers */

        REMOVENODE((Node *)f);
        if (IsEmptyList((List *)n))
            DisableInterrupt(f->firq_Num);
    }
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


void EnableInterrupt(uint32 firqNum)
{
#ifdef BUILD_PARANOIA
    if (firqNum >= INTR_MAX)
    {
        printf("ERROR: illegal firq number given to EnableInterrupt()\n");
        return;
    }
#endif

    if (firqNum >= INT_CDE_BASE)
    {
        CDE_WRITE(KB_FIELD(kb_CDEBase), CDE_INT_ENABLE, 1 << (firqNum - INT_CDE_BASE));
	/* All CDE ints require that CDE be unmasked */
	BDA_WRITE(BDAPCTL_PBINTENSET, BDAINT_CDE_MASK);
    }
    else
    {
        /* Do a BDA int */
        BDA_WRITE(BDAPCTL_PBINTENSET, 1 << firqNum);
    }
}


/*****************************************************************************/


void DisableInterrupt(uint32 firqNum)
{
#ifdef BUILD_PARANOIA
    if (firqNum >= INTR_MAX)
    {
        printf("ERROR: illegal firq number given to DisableInterrupt()\n");
        return;
    }
#endif

    if (firqNum >= INT_CDE_BASE)
    {
        CDE_CLR(KB_FIELD(kb_CDEBase), CDE_INT_ENABLE, 1 << (firqNum - INT_CDE_BASE));
    }
    else
    {
        BDA_CLR(BDAPCTL_PBINTENSET, 1 << firqNum);
    }
}


/*****************************************************************************/


void ClearInterrupt(uint32 firqNum)
{
uint32 BDAmask;
uint32 CDEmask;

#ifdef BUILD_PARANOIA
    if (firqNum >= INTR_MAX)
    {
        printf("ERROR: illegal firq number given to ClearInterrupt()\n");
        return;
    }
#endif

    if (firqNum < INT_CDE_BASE)
    {
        BDAmask = 1 << firqNum;
        CDEmask = 0;
    } else
    {
        BDAmask = BDAINT_CDE_MASK;
        CDEmask = 1 << (firqNum - INT_CDE_BASE);
    }
    BDA_CLR(BDAPCTL_ERRSTAT, BDAmask);
    if (CDEmask)
        CDE_CLR(KB_FIELD(kb_CDEBase), CDE_INT_STS, CDEmask | CDE_INT_SENT);
}


/*****************************************************************************/


static void CallHandler(uint32 firqNum)
{
Node     *n;
FirqNode *fn;
bool      handled;

    handled = FALSE;
    if (n = KB_FIELD(kb_InterruptHandlers)[firqNum])
    {
        if (n->n_Type == FIRQNODE)
        {
            fn = (FirqNode *)n;
            (*fn->firq_Code)(fn);
            return;
        }

        ScanList(&((FirqList *)n)->fl_Firqs, fn, FirqNode)
        {
            (*fn->firq_Code)(fn);
            handled = TRUE;
        }
    }

    if (!handled)
    {
        /* no handler for this interrupt! */

        /* prevent the int from happening again */
        DisableInterrupt(firqNum);

#ifdef BUILD_PARANOIA
        printf("No interrupt handler for int %d\n", firqNum);
        printf("Int stat sez %x\n", BDA_READ(BDAPCTL_PBINTSTAT));
        printf("Int enable %x\n", BDA_READ(BDAPCTL_PBINTENSET));
#else
        PANIC(ER_Kr_NoIntHandler);
#endif
    }
}


/*****************************************************************************/


void ServiceExceptions(void)
{
uint32  intStat;
int32   firqNum;

    /* Read pending interrupts.
     *
     * Only handle interrupts that are enabled. Also mask out bit 0, which
     * is to be processed by the slave CPU.
     */
    while (intStat = (BDA_READ(BDAPCTL_PBINTSTAT)
                      & BDA_READ(BDAPCTL_PBINTENSET)
                      & (~BDAMREF_GPIO3_VALUE)))
    {
        firqNum = FindMSB(intStat);
        LogInterruptStart(firqNum, "");
        CallHandler(firqNum);
    }

    LogInterruptDone();
}


/*****************************************************************************/


/* Kernel CDE handler. This guy gets called, and it then calls handlers for the
 * particular CDE interrupt that occured.
 */
void KernelCDEHandler(void)
{
uint32 intval;
uint32 intmask;
uint32 bit;

    while (TRUE)
    {
        /* read pending interrupts */
        intval = CDE_READ(KB_FIELD(kb_CDEBase), CDE_INT_STS);
        intmask = intval & CDE_READ(KB_FIELD(kb_CDEBase), CDE_INT_ENABLE);

        if (!intmask)
        {
            /* ack the request to allow further ints from CDE */
	    ClearInterrupt(INT_CDE_INTSENT);
            return;
        }

        bit = (uint32)FindMSB(intmask & ~CDE_INT_SENT);
        CallHandler(bit + INT_CDE_BASE);
    }
}


/*****************************************************************************/


static uint64 vblankCount;

void KernelVBLHandler(void)
{
    /* acknowledge the interrupt */
    BDA_CLR(BDAVDU_VINT, BDAVINT1_MASK);
    vblankCount++;

    /* if not running with the debugger, reset the watchdog */
    if (KB_FIELD(kb_CPUFlags) & KB_NODBGR)
        SuperSetSysInfo(SYSINFO_TAG_WATCHDOG, (void *)SYSINFO_WDOGENABLE, 0);
}


/*****************************************************************************/


void SampleSystemTimeVBL(TimeValVBL *tv)
{
    *tv = (TimeValVBL)vblankCount;
}
