/* @(#) fence.c 96/07/18 1.33 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>


/*****************************************************************************/

/*#define DEBUG*/
#define DBUG(x)	printf x


static void LoadFenceBits(Task *t)
{
#ifdef DEBUG
     int i;
     uint32 *p;
     DBUG(("Loading fence for %s\n",t->t.n_Name));
     p = t->t_PagePool->pp_WritablePages;
     for (i=0; i < 16 ; i++)
     {
	printf("%08x ",*p++);
	if ( (i & 7) == 7) printf("\n");
     }
#endif
#ifndef	NO_FENCE
     KB_FIELD(kb_WriteFencePtr) = t->t_PagePool->pp_WritablePages;
     _mtsr13((uint32)KB_FIELD(kb_WriteFencePtr));
     LoadDTLBs(KB_FIELD(kb_WriteFencePtr), t->t_PagePool->pp_MemRegion->mr_MemBase);
#endif
}


/*****************************************************************************/


/* Warning: be careful changing this code; there are compiler bugs
 * which occur if you get too fancy with uint64 operations. */
static void UserAccessPCMCIA(bool userOK)
{
    uint64 bat;
    uint32 ubat;
    uint32 lbat;
    static bool UserPCMCIA = FALSE;

    if (userOK == UserPCMCIA)
	/* BATs are already set correctly. */
	return;

    bat = _mfdbat3();
    lbat = bat;
    ubat = bat >> 32;
    ubat &= ~(UBAT_VS | UBAT_VP);
    if (userOK)
	ubat |= UBAT_VS | UBAT_VP;
    else
	ubat |= UBAT_VS;
    _mtdbat3(ubat, lbat);

    bat = _mfibat3();
    lbat = bat;
    ubat = bat >> 32;
    ubat &= ~(UBAT_VS | UBAT_VP);
    if (userOK)
	ubat |= UBAT_VS | UBAT_VP;
    else
	ubat |= UBAT_VS;
    _mtibat3(ubat, lbat);

    UserPCMCIA = userOK;
}


/*****************************************************************************/


void LoadFence(Task *t)
{
    UserAccessPCMCIA((t->t_Flags & TASK_PCMCIA_PERM) != 0);

    if (t->t_Flags & TASK_SUPERVISOR_ONLY)
    {
        /* This task only ever runs in supervisor mode, it therefore doesn't
         * care about the fence bits.
         */
        return;
    }

    /* get the container task if a thread */
    t = (t->t_ThreadTask) ? t->t_ThreadTask : t;

    if (t == KB_FIELD(kb_CurrentFenceTask))
        return;

    KB_FIELD(kb_CurrentFenceTask) = t;
    LoadFenceBits(t);
}


/*****************************************************************************/


void UpdateFence(void)
{
PagePool *pp;

    if (CURRENTTASK)
    {
        if (KB_FIELD(kb_CurrentFenceTask))
        {
            if (IsSameTaskFamily(CURRENTTASK,KB_FIELD(kb_CurrentFenceTask)))
                LoadFenceBits(CURRENTTASK);
        }
        else
        {
            LoadFenceBits(CURRENTTASK);
        }

        if (KB_FIELD(kb_CurrentSlaveTask) >= 0)
        {
            if (IsSameTaskFamily(CURRENTTASK, TASK(KB_FIELD(kb_CurrentSlaveTask))))
            {
                pp = CURRENTTASK->t_PagePool;
                WriteBackDCache(0, pp->pp_WritablePages, pp->pp_MemRegion->mr_NumPages / 8);
                SuperSlaveRequest(SLAVE_UPDATEMMU, (uint32)pp->pp_WritablePages, 0, 0);
            }
        }
    }
}
