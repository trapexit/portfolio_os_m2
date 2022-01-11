/* @(#) memdebug.h 96/01/21 1.6 */

#ifndef __MEMDEBUG_H
#define __MEMDEBUG_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif


/*****************************************************************************/


/* types of memory calls */
typedef enum MemDebugCalls
{
    MEMDEBUG_CALL_ALLOCMEM,
    MEMDEBUG_CALL_ALLOCMEMMASKED,
    MEMDEBUG_CALL_FREEMEM,
    MEMDEBUG_CALL_REALLOCMEM,
    MEMDEBUG_CALL_GETMEMTRACKSIZE,
    MEMDEBUG_CALL_ALLOCMEMPAGES,
    MEMDEBUG_CALL_FREEMEMPAGES,
    MEMDEBUG_CALL_CONTROLMEM,

    MEMDEBUG_CALL_SUPERALLOCMEM,
    MEMDEBUG_CALL_SUPERALLOCMEMMASKED,
    MEMDEBUG_CALL_SUPERFREEMEM,
    MEMDEBUG_CALL_SUPERREALLOCMEM,

    MEMDEBUG_CALL_SUPERALLOCUSERMEM,
    MEMDEBUG_CALL_SUPERFREEUSERMEM,

    MEMDEBUG_CALL_SANITYCHECK
} MemDebugCalls;


void *MemDebugAlloc(PagePool *pp, int32 memSize, uint32 memFlags, uint32 careBits, uint32 stateBits, const char *sourceFile, uint32 sourceLine, MemDebugCalls call);
void MemDebugFree(PagePool *pp, void *mem, int32 memSize, const char *sourceFile, uint32 sourceLine, MemDebugCalls call);
void *MemDebugRealloc(PagePool *pp, void *p, int32 oldSize, int32 newSize, uint32 memFlags, const char *sourceFile, uint32 sourceLine, MemDebugCalls call);
int32 MemDebugGetMemTrackSize(void *mem, const char *sourceFile, uint32 sourceLine);


/*****************************************************************************/


void DeleteTaskMemDebug(const struct Task *t);

bool AddAllocation(const struct MemTrack *template, Item task);
void RemAllocation(struct TaskTrack *tt, struct MemTrack *mt, struct MemTrack *prev);
bool DoMemRation(const PagePool *pp, int32 memSize);

bool internalAddAllocation(const struct MemTrack *template, Item task);
void internalRemAllocation(struct TaskTrack *tt, struct MemTrack *mt, struct MemTrack *prev);
bool internalDoMemRation(const PagePool *pp, int32 memSize);


/*****************************************************************************/


#endif /* __MEMDEBUG_H */
