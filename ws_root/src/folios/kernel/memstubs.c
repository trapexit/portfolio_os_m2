/* @(#) memstubs.c 96/08/16 1.14 */

#ifdef MEMDEBUG
#undef MEMDEBUG
#endif

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/internalf.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <stdio.h>
#include "memdebug.h"


/*****************************************************************************/


void *AllocUserMem(int32 size, uint32 flags, Task *t)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return MemDebugAlloc(t->t_PagePool, size, flags, 0, 0, NULL, 0, MEMDEBUG_CALL_ALLOCMEM);
#endif

    return AllocFromPagePool(t->t_PagePool, size, flags, 0, 0);
}


/*****************************************************************************/


void *AllocMem(int32 size, uint32 flags)
{
    if (CURRENTTASK)
        return AllocUserMem(size, flags, CURRENTTASK);

    return SuperAllocMem(size, flags);
}


/*****************************************************************************/


void *AllocMemMasked(int32 size, uint32 flags, uint32 careBits, uint32 stateBits)
{
    if (CURRENTTASK)
    {
#ifdef BUILD_MEMDEBUG
        if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
            return MemDebugAlloc(CURRENTTASK->t_PagePool, size, flags, careBits, stateBits, NULL, 0, MEMDEBUG_CALL_ALLOCMEMMASKED);
#endif

        return AllocFromPagePool(CURRENTTASK->t_PagePool, size, flags, careBits, stateBits);
    }

    return SuperAllocMemMasked(size, flags, careBits, stateBits);
}


/*****************************************************************************/


void *ReallocMem(void *oldMem, int32 oldSize, int32 newSize, uint32 flags)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return ReallocMemDebug(oldMem, oldSize, newSize, flags, NULL, 0);
#endif

    return ReallocFromPagePool(CURRENTTASK->t_PagePool, oldMem, oldSize, newSize, flags);
}


/*****************************************************************************/


void FreeUserMem(void *p, int32 size, Task *t)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
    {
        MemDebugFree(t->t_PagePool, p, size, NULL, 0, MEMDEBUG_CALL_FREEMEM);
        return;
    }
#endif

    FreeToPagePool(t->t_PagePool, p, size);
}


/*****************************************************************************/


void FreeMem(void *p, int32 size)
{
    if (CURRENTTASK)
        FreeUserMem(p, size, CURRENTTASK);
    else
        SuperFreeMem(p, size);
}


/*****************************************************************************/


void *SuperAllocMem(int32 size, uint32 flags)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return MemDebugAlloc(KB_FIELD(kb_PagePool), size, flags, 0, 0, NULL, 0, MEMDEBUG_CALL_SUPERALLOCMEM);
#endif

    return AllocFromPagePool(KB_FIELD(kb_PagePool), size, flags, 0, 0);
}


/*****************************************************************************/


void *SuperAllocMemMasked(int32 size, uint32 flags, uint32 careBits, uint32 stateBits)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return MemDebugAlloc(KB_FIELD(kb_PagePool), size, flags, careBits, stateBits, NULL, 0, MEMDEBUG_CALL_SUPERALLOCMEMMASKED);
#endif

    return AllocFromPagePool(KB_FIELD(kb_PagePool), size, flags, careBits, stateBits);
}


/*****************************************************************************/


void *SuperReallocMem(void *oldMem, int32 oldSize, int32 newSize, uint32 flags)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return SuperReallocMemDebug(oldMem, oldSize, newSize, flags, NULL, 0);
#endif

    return ReallocFromPagePool(KB_FIELD(kb_PagePool), oldMem, oldSize, newSize, flags);
}


/*****************************************************************************/


void SuperFreeMem(void *p, int32 size)
{
#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
    {
        MemDebugFree(KB_FIELD(kb_PagePool), p, size, NULL, 0, MEMDEBUG_CALL_SUPERFREEMEM);
        return;
    }
#endif

    FreeToPagePool(KB_FIELD(kb_PagePool), p, size);
}


/*****************************************************************************/


void *externalAllocMemPages(int32 size, uint32 flags)
{
    return internalAllocMemPages(size,flags);
}


/*****************************************************************************/


void externalFreeMemPages(void *p, int32 size)
{
    internalFreeMemPages(p,size);
}


#ifdef BUILD_MEMDEBUG

/*****************************************************************************/


void *AllocMemDebug(int32 memSize, uint32 memFlags,
                    const char *sourceFile, int32 sourceLine)
{
    if (CURRENTTASK == NULL)
        return AllocMem(memSize, memFlags);

    return MemDebugAlloc(CURRENTTASK->t_PagePool, memSize, memFlags, 0, 0, sourceFile, sourceLine, MEMDEBUG_CALL_ALLOCMEM);
}


/*****************************************************************************/


void *AllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits,
                          const char *sourceFile, int32 sourceLine)
{
    if (CURRENTTASK == NULL)
        return AllocMemMasked(memSize, memFlags, careBits, stateBits);

    return MemDebugAlloc(CURRENTTASK->t_PagePool, memSize, memFlags, careBits, stateBits, sourceFile, sourceLine, MEMDEBUG_CALL_ALLOCMEMMASKED);
}


/*****************************************************************************/


void *ReallocMemDebug(void *p, int32 oldSize, int32 newSize, uint32 memFlags,
                      const char *sourceFile, int32 sourceLine)
{
    return MemDebugRealloc(CURRENTTASK->t_PagePool, p, oldSize, newSize, memFlags,
                           sourceFile, sourceLine, MEMDEBUG_CALL_REALLOCMEM);
}


/*****************************************************************************/


void FreeMemDebug(void *p, int32 memSize,
                  const char *sourceFile, int32 sourceLine)
{
    if (CURRENTTASK == NULL)
        FreeMem(p, memSize);
    else
        MemDebugFree(CURRENTTASK->t_PagePool, p, memSize, sourceFile, sourceLine, MEMDEBUG_CALL_FREEMEM);
}


/*****************************************************************************/


int32 GetMemTrackSizeDebug(const void *mem, const char *sourceFile, int32 sourceLine)
{
    return MemDebugGetMemTrackSize(mem, sourceFile, sourceLine);
}


/*****************************************************************************/


void *externalAllocMemPagesDebug(int32 memSize, uint32 memFlags,
                                 const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);

    /* no debugging for this call yet */
    return internalAllocMemPages(memSize, memFlags);
}


/*****************************************************************************/


void externalFreeMemPagesDebug(void *mem, int32 memSize,
                               const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);

    /* no debugging for this call yet */
    internalFreeMemPages(mem, memSize);
}


/*****************************************************************************/


void *SuperAllocMemDebug(int32 memSize, uint32 memFlags,
                         const char *sourceFile, int32 sourceLine)
{
    return MemDebugAlloc(KB_FIELD(kb_PagePool), memSize, memFlags, 0, 0, sourceFile, sourceLine, MEMDEBUG_CALL_SUPERALLOCMEM);
}


/*****************************************************************************/


void *SuperAllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits,
                               const char *sourceFile, int32 sourceLine)
{
    return MemDebugAlloc(KB_FIELD(kb_PagePool), memSize, memFlags, careBits, stateBits, sourceFile, sourceLine, MEMDEBUG_CALL_SUPERALLOCMEMMASKED);
}


/*****************************************************************************/


void *SuperReallocMemDebug(void *p, int32 oldSize, int32 newSize, uint32 memFlags,
                           const char *sourceFile, int32 sourceLine)
{
    return MemDebugRealloc(KB_FIELD(kb_PagePool), p, oldSize, newSize, memFlags,
                           sourceFile, sourceLine, MEMDEBUG_CALL_SUPERREALLOCMEM);
}


/*****************************************************************************/


void SuperFreeMemDebug(void *p, int32 memSize,
                       const char *sourceFile, int32 sourceLine)
{
    MemDebugFree(KB_FIELD(kb_PagePool), p, memSize, sourceFile, sourceLine, MEMDEBUG_CALL_SUPERFREEMEM);
}


/*****************************************************************************/


#else /* BUILD_MEMDEBUG */


/*****************************************************************************/


void *AllocMemDebug(int32 memSize, uint32 memFlags,
                    const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return AllocMem(memSize, memFlags);
}

void *AllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits,
                          const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return AllocMemMasked(memSize, memFlags, careBits, stateBits);
}

void *ReallocMemDebug(void *p, int32 oldSize, int32 newSize, uint32 memFlags,
                      const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return ReallocMem(p, oldSize, newSize, memFlags);
}

void FreeMemDebug(void *p, int32 memSize,
                  const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    FreeMem(p, memSize);
}

int32 GetMemTrackSizeDebug(const void *mem, const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return GetMemTrackSize(mem);
}

void *SuperAllocMemDebug(int32 memSize, uint32 memFlags,
                         const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return SuperAllocMem(memSize, memFlags);
}

void *SuperAllocMemMaskedDebug(int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits,
                               const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return SuperAllocMemMasked(memSize, memFlags, careBits, stateBits);
}

void *SuperReallocMemDebug(void *p, int32 oldSize, int32 newSize, uint32 memFlags,
                           const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    return SuperReallocMem(p, oldSize, newSize, memFlags);
}

void SuperFreeMemDebug(void *p, int32 memSize,
                       const char *sourceFile, int32 sourceLine)
{
    TOUCH(sourceFile);
    TOUCH(sourceLine);
    SuperFreeMem(p, memSize);
}


/*****************************************************************************/


#endif /* BUILD_MEMDEBUG */
