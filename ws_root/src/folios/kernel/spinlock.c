/* @(#) spinlock.c 96/09/03 1.2 */

#include <kernel/types.h>
#include <kernel/cache.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/spinlock.h>


/*****************************************************************************/


/* a handle to a spin lock */
struct SpinLock
{
    uint32  sl_Lock;
    uint32  sl_Pad0[7];		/* put the next field in a different cache line */
    uint32  sl_CPU0;
    uint32  sl_Pad1[7];		/* put the next field in a different cache line */
    uint32  sl_CPU1;
    void   *sl_Cookie;
};


/*****************************************************************************/


Err CreateSpinLock(SpinLock **lock)
{
SpinLock  *sl;
CacheInfo  ci;

    GetCacheInfo(&ci, sizeof(CacheInfo));

    sl = AllocMemAligned(ALLOC_ROUND(sizeof(SpinLock), ci.cinfo_DCacheLineSize),
                         MEMTYPE_FILL | MEMTYPE_TRACKSIZE,
                         ci.cinfo_DCacheLineSize);
    *lock = sl;

    if (!sl)
        return NOMEM;

    sl->sl_Cookie = sl;
    FlushDCache(0, sl, sizeof(SpinLock));

    return 0;
}


/*****************************************************************************/


Err DeleteSpinLock(SpinLock *lock)
{
    if (lock)
    {
        if (lock->sl_Cookie == lock)
        {
            lock->sl_Cookie = NULL;
            FreeMem(lock, TRACKED_SIZE);
        }
    }

    return 0;
}
