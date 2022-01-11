/* @(#) mp.c 96/08/26 1.8 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <kernel/cache.h>
#include <kernel/task.h>
#include <hardware/PPCasm.h>


void InitMP(void)
{
CPUActionReq *mr;
CPUActionReq *sr;
SlaveState   *ss;
uint32        lineSize;
PagePool     *pp;

    if (KB_FIELD(kb_NumCPUs) > 1)
    {
        lineSize = KB_FIELD(kb_DCacheBlockSize);

        mr = SuperAllocMemAligned(ALLOC_ROUND(sizeof(CPUActionReq), lineSize),
                                  MEMTYPE_FILL,
                                  lineSize);

        sr = SuperAllocMemAligned(ALLOC_ROUND(sizeof(CPUActionReq), lineSize),
                                  MEMTYPE_FILL,
                                  lineSize);

        ss = SuperAllocMemAligned(ALLOC_ROUND(sizeof(SlaveState), lineSize),
                                  MEMTYPE_FILL,
                                  lineSize);

        pp = KB_FIELD(kb_PagePool);
        ss->ss_WritablePages = SuperAllocMemAligned(ALLOC_ROUND(pp->pp_MemRegion->mr_NumPages / 8, lineSize),
                                                    MEMTYPE_FILL,
                                                    lineSize);

        ss->ss_NumPages  = pp->pp_MemRegion->mr_NumPages;
        ss->ss_SlaveSER  = _mfser();
        ss->ss_SlaveSEBR = _mfsebr();

        /* init this to the top of the stack, with a preallocated stack frame */
        ss->ss_SuperSP = (uint8 *)((uint32)ss->ss_SuperStack
                                   + sizeof(ss->ss_SuperStack)
                                   - sizeof(SuperStackFrame));

        KB_FIELD(kb_MasterReq)  = mr;
        KB_FIELD(kb_SlaveReq)   = sr;
        KB_FIELD(kb_SlaveState) = ss;
    }

    KB_FIELD(kb_CurrentSlaveTask) = -1;
}
