/* @(#) memdebug.c 96/09/11 1.31 */

#ifdef MEMDEBUG
#undef MEMDEBUG
#endif

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/item.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/semaphore.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <kernel/tags.h>
#include <kernel/random.h>
#include <string.h>
#include <stdio.h>
#include "memdebug.h"


#ifdef BUILD_MEMDEBUG

/****************************************************************************/


#define NUM_HASH_ENTRIES 16
#define HASH_MEM(x)      (((((uint32)x) & 0x000000f0) >> 4) & 0xf)
#define COOKIE_SIZE      16       /* must be multiple of sizeof(FreeBlock) */
#define OUTPUT(x)        printf x


/* COOKIE_SIZE should be really be adjustable by client code... */


/* statistics about memory allocations */
typedef struct AllocStats
{
    uint32 as_CurrentAllocations;
    uint32 as_MaxAllocations;
    uint32 as_TotalAllocations;
    uint32 as_CurrentAllocated;
    uint32 as_MaxAllocated;
} AllocStats;

/* one of these exists for every allocation made (yes, I know it's big) */
typedef struct MemTrack
{
    struct MemTrack *mt_Next;  /* must be at head of structure due to the sneaky way we yank things from the singly-linked list */
    void            *mt_Memory;
    int32            mt_MemorySize;
    uint32           mt_MemoryFlags;
    const char      *mt_File;
    uint32           mt_LineNum;
    uint32           mt_CookieValue;
    uint8            mt_Call;
    bool             mt_TrackSize;
} MemTrack;

/* one of these exists for every task/thread we know about */
typedef struct TaskTrack
{
    MinNode     tt_Link;
    Item        tt_Task;     /* -1 for super allocations */
    MemTrack   *tt_Memory[NUM_HASH_ENTRIES];
    AllocStats  tt_Stats;
    char        tt_TaskName[30];
} TaskTrack;


/*****************************************************************************/


static List       tasks = PREPLIST(tasks);
static Item       memDebugLock = -1;
static bool       allocPatterns;
static bool       freePatterns;
static bool       padCookies;
static bool       breakpointOnErrors;
static bool       checkAllocFailures;
static bool       keepTaskData;
static uint32     cookieValue;
static AllocStats globalStats;

static bool       ration;
static bool       rationRandom;
static bool       rationBreakpoint;
static bool       rationSuper;
static bool       rationVerbose;
static uint32     rationInterval;
static uint32     rationCountdown;
static uint32     rationMinSize;
static uint32     rationMaxSize;
static Item       rationTask;
static uint32     rationAfter;


/*****************************************************************************/


static void LockSem(void)
{
    if (CURRENTTASK)
        LockSemaphore(memDebugLock, SEM_WAIT);
}


/*****************************************************************************/


static void UnlockSem(void)
{
    if (CURRENTTASK)
        UnlockSemaphore(memDebugLock);
}


/*****************************************************************************/


/* fill memory with a 32-bit pattern */
static void FillMem(void *mem, uint32 numBytes, uint32 pattern)
{
uint32  extra;
uint32 *l;

    extra    = numBytes % 4;
    l        = (uint32 *)mem;
    numBytes = numBytes / 4;
    while (numBytes--)
        *l++ = pattern;

    if (extra)
        memcpy(l,&pattern,extra);
}


/*****************************************************************************/


/* check that a memory block contains a specific 32-bit pattern */
static bool CheckMem(const void *mem, uint32 numBytes, uint32 pattern)
{
uint32 *l;

    l = (uint32 *)mem;
    while (numBytes)
    {
        if (*l++ != pattern)
            return (FALSE);

        numBytes -= 4;
    }

    return (TRUE);
}


/*****************************************************************************/


/* invoked when an error condition is detected. Calls the debugger if needed */
static void MemError(void)
{
    if (breakpointOnErrors)
        DebugBreakpoint();
}


/*****************************************************************************/


/* we're about to say something.... */
static void OutputTitle(const char *title)
{
    OUTPUT(("\nMEMDEBUG; %s\n",title));
}


/*****************************************************************************/


/* output information about the given task */
static void OutputTask(const char *header, Item task, const TaskTrack *tt)
{
Task *t;

    OUTPUT(("  %s",header));

    if (task < 0)
    {
        OUTPUT(("SUPERVISOR"));
    }
    else
    {
        if (task == 0)
            t = CURRENTTASK;
        else
            t = (Task *)CheckItem(task,KERNELNODE,TASKNODE);

        if (t)
        {
            OUTPUT(("name %s, addr $%x, item $%x",t->t.n_Name,t,t->t.n_Item));
        }
        else
        {
            OUTPUT(("dead task %s, item $%x",tt->tt_TaskName,task));
        }
    }

    OUTPUT(("\n"));
}


/*****************************************************************************/


static void OutputCall(const char *header, MemDebugCalls call,
                       const char *sourceFile, uint32 sourceLine)
{
char *str;

    OUTPUT(("  %s",header));

    switch (call)
    {
        case MEMDEBUG_CALL_ALLOCMEM            : str = "AllocMem"; break;
        case MEMDEBUG_CALL_ALLOCMEMMASKED      : str = "AllocMemMasked"; break;
        case MEMDEBUG_CALL_FREEMEM             : str = "FreeMem"; break;
        case MEMDEBUG_CALL_REALLOCMEM          : str = "ReallocMem"; break;
        case MEMDEBUG_CALL_GETMEMTRACKSIZE     : str = "GetMemTrackSize"; break;
        case MEMDEBUG_CALL_ALLOCMEMPAGES       : str = "AllocMemPages"; break;
        case MEMDEBUG_CALL_FREEMEMPAGES        : str = "FreeMemPages"; break;
        case MEMDEBUG_CALL_CONTROLMEM          : str = "ControlMem"; break;
        case MEMDEBUG_CALL_SUPERALLOCMEM       : str = "SuperAllocMem"; break;
        case MEMDEBUG_CALL_SUPERALLOCMEMMASKED : str = "SuperAllocMemMasked"; break;
        case MEMDEBUG_CALL_SUPERFREEMEM        : str = "SuperFreeMem"; break;
        case MEMDEBUG_CALL_SUPERREALLOCMEM     : str = "SuperReallocMem"; break;
        case MEMDEBUG_CALL_SUPERALLOCUSERMEM   : str = "SuperAllocUserMem"; break;
        case MEMDEBUG_CALL_SUPERFREEUSERMEM    : str = "SuperFreeUserMem"; break;
        default                                : str = "Unknown"; break;
    }

    OUTPUT(("%s(), ",str));

    if (sourceFile)
    {
        OUTPUT(("file '%s', line %d\n",sourceFile,sourceLine));
    }
    else
    {
        OUTPUT(("<unknown source file>\n"));
    }
}


/*****************************************************************************/


static void OutputAllocator(const TaskTrack *tt, const MemTrack *mt)
{
    OutputTask("Allocation Task/Thread  : ",tt->tt_Task, tt);
    OutputCall("Allocation Call         : ",mt->mt_Call, mt->mt_File,mt->mt_LineNum);
    OUTPUT (("  Allocation Parameters   : size %d, flags $%08x\n",mt->mt_MemorySize,mt->mt_MemoryFlags));
}


/*****************************************************************************/


static void OutputNewAllocator(int32 memSize, uint32 memFlags,
                               const char *sourceFile, uint32 sourceLine,
                               MemDebugCalls call)
{
    if ((call == MEMDEBUG_CALL_SUPERALLOCMEM)
     || (call == MEMDEBUG_CALL_SUPERALLOCMEMMASKED))
    {
        OutputTask("Allocation Task/Thread  : ",-1,NULL);
    }
    else
    {
        OutputTask("Allocation Task/Thread  : ",0,NULL);
    }

    OutputCall("Allocation Call         : ",call,sourceFile,sourceLine);
    OUTPUT (("  Allocation Parameters   : size %d, flags $%08x\n",memSize,memFlags));
}


/*****************************************************************************/


static void OutputDeallocator(void *mem, int32 memSize,
                              const char *sourceFile, uint32 sourceLine,
                              MemDebugCalls call)
{
    if (call == MEMDEBUG_CALL_SUPERFREEMEM)
    {
        OutputTask("Deallocation Task/Thread: ",-1,NULL);
    }
    else
    {
        OutputTask("Deallocation Task/Thread: ",0,NULL);
    }

    OutputCall("Deallocation Call       : ", call, sourceFile, sourceLine);
    OUTPUT (("  Deallocation Parameters : addr $%08x, size %d\n",mem,memSize));
}


/*****************************************************************************/


static void OutputGetTrackSize(void *mem, const char *sourceFile, uint32 sourceLine)
{
    OutputTask("Calling Task/Thread     : ",0,NULL);
    OutputCall("Call                    : ", MEMDEBUG_CALL_GETMEMTRACKSIZE, sourceFile, sourceLine);
    OUTPUT (("  TrackSize Parameters    : addr $%08x\n",mem));
}


/*****************************************************************************/


static void OutputCookies(const MemTrack *mt, void *mem)
{
uint32  i;
uint32 *u;

    OUTPUT(("  Original Cookie         : "));
    for (i = 0; i < COOKIE_SIZE / 4; i++)
    {
        OUTPUT(("$%08x ",mt->mt_CookieValue));
    }
    OUTPUT(("\n"));

    OUTPUT(("  Modified Cookie         : "));
    u = (uint32 *)mem;
    for (i = 0; i < COOKIE_SIZE / 4; i++)
    {
        OUTPUT(("$%08x ",*u++));
    }
    OUTPUT(("\n"));
}


/*****************************************************************************/


static void OutputStats(const AllocStats *as)
{
    OUTPUT(("    Current number of allocations            : %d\n",as->as_CurrentAllocations));
    OUTPUT(("    Max number of allocations at any one time: %d\n",as->as_MaxAllocations));
    OUTPUT(("    Total number of allocation calls         : %d\n",as->as_TotalAllocations));
    OUTPUT(("    Current amount allocated                 : %d bytes\n",as->as_CurrentAllocated));
    OUTPUT(("    Max amount allocated at any one time     : %d bytes\n",as->as_MaxAllocated));
}


/*****************************************************************************/


static void FreeWithFill(PagePool *pp, void *mem, int32 size)
{
    if (freePatterns)
        FillMem(mem, size, MEMDEBUG_FREE_PATTERN);

    FreeToPagePool(pp,mem,size);
}


/*****************************************************************************/


static void FreeTaskTrack(TaskTrack *tt)
{
MemTrack  *mt;
MemTrack  *next;
uint32     i;
int32      freeSize;
uint32     adjustment;
int32     *mem;
Task      *t;

    for (i = 0; i < NUM_HASH_ENTRIES; i++)
    {
        mt = tt->tt_Memory[i];
        t  = TASK(tt->tt_Task);
        while (mt)
        {
            next = mt->mt_Next;

            if (mt->mt_CookieValue)
            {
                /* free the memory used for the cookies */

                adjustment = 0;

                mem = (int32 *)mt->mt_Memory;
                if (mt->mt_TrackSize)
                {
                    if ((uint32)mem & 0x07)
                        adjustment = 4;
                    else
                        adjustment = 8;
                }

                mem         = (void *)((uint32)mem - COOKIE_SIZE - adjustment);
                freeSize    = ALLOC_ROUND(mt->mt_MemorySize + COOKIE_SIZE*2 + adjustment, sizeof(FreeBlock));

                if (t)
                {
                    FreeToPagePool(t->t_PagePool, mem, COOKIE_SIZE);
                    FreeToPagePool(t->t_PagePool, (void *)((uint32)mem + freeSize - COOKIE_SIZE), COOKIE_SIZE);
                }
            }

            FreeWithFill(KB_FIELD(kb_PagePool),mt,sizeof(MemTrack));
            mt = next;
        }
    }

    FreeWithFill(KB_FIELD(kb_PagePool),tt,sizeof(TaskTrack));
}


/*****************************************************************************/


bool internalAddAllocation(const MemTrack *template, Item task)
{
MemTrack  *mt;
TaskTrack *tt;
bool       found;

    found = FALSE;
    ScanList(&tasks,tt,TaskTrack)
    {
        if (tt->tt_Task == task)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        /* if we didn't find a task node, we must fabricate one */
        tt = AllocFromPagePool(KB_FIELD(kb_PagePool), sizeof(TaskTrack), MEMTYPE_FILL, 0, 0);
        if (tt)
        {
            tt->tt_Task = task;
            if (task < 0)
                strcpy(tt->tt_TaskName,"Supervisor");
            else
                strncpy(tt->tt_TaskName,CURRENTTASK->t.n_Name,sizeof(tt->tt_TaskName)-1);

            AddHead(&tasks,(Node *)tt);
        }

        if (!tt)
            return FALSE;
    }

    mt = AllocFromPagePool(KB_FIELD(kb_PagePool), sizeof(MemTrack), MEMTYPE_NORMAL, 0, 0);
    if (!mt)
    {
        if (!found)
        {
            /* if a brand new task node, nuke it */
            RemNode((Node *)tt);
            FreeWithFill(KB_FIELD(kb_PagePool), tt, sizeof(TaskTrack));
        }

        return FALSE;
    }

    cookieValue += 2;

    *mt = *template;
    mt->mt_Next = tt->tt_Memory[HASH_MEM(mt->mt_Memory)];
    tt->tt_Memory[HASH_MEM(mt->mt_Memory)] = mt;

    /* update stats for this task/thread */

    tt->tt_Stats.as_CurrentAllocated += mt->mt_MemorySize;
    if (tt->tt_Stats.as_CurrentAllocated > tt->tt_Stats.as_MaxAllocated)
        tt->tt_Stats.as_MaxAllocated = tt->tt_Stats.as_CurrentAllocated;

    tt->tt_Stats.as_TotalAllocations++;
    tt->tt_Stats.as_CurrentAllocations++;
    if (tt->tt_Stats.as_CurrentAllocations > tt->tt_Stats.as_MaxAllocations)
        tt->tt_Stats.as_MaxAllocations = tt->tt_Stats.as_CurrentAllocations;

    /* update global stats */

    globalStats.as_CurrentAllocated += mt->mt_MemorySize;
    if (globalStats.as_CurrentAllocated > globalStats.as_MaxAllocated)
        globalStats.as_MaxAllocated = globalStats.as_CurrentAllocated;

    globalStats.as_TotalAllocations++;
    globalStats.as_CurrentAllocations++;
    if (globalStats.as_CurrentAllocations > globalStats.as_MaxAllocations)
        globalStats.as_MaxAllocations = globalStats.as_CurrentAllocations;

    return TRUE;
}


/*****************************************************************************/


void internalRemAllocation(TaskTrack *tt, MemTrack *mt, MemTrack *prev)
{
    prev->mt_Next = mt->mt_Next;

    tt->tt_Stats.as_CurrentAllocated -= mt->mt_MemorySize;
    tt->tt_Stats.as_CurrentAllocations--;

    globalStats.as_CurrentAllocated -= mt->mt_MemorySize;
    globalStats.as_CurrentAllocations--;

    FreeWithFill(KB_FIELD(kb_PagePool), mt, sizeof(MemTrack));

    if (tt->tt_Stats.as_CurrentAllocations == 0)
    {
        /* if there are no more allocations attributed to
         * this task, zap the task node
         *
         * if keepTaskData is TRUE, then we should keep the
         * node around because the caller wants to get some
         * stats on this task's allocations.
         */

        if (!keepTaskData)
        {
            RemNode((Node *)tt);
            FreeTaskTrack(tt);
        }
    }
}


/*****************************************************************************/


bool internalDoMemRation(const PagePool *pp, int32 memSize)
{
    if ((rationTask == 0) || (rationTask == CURRENTTASKITEM))
    {
        if (pp->pp_Owner || rationSuper)
        {
            if ((memSize >= rationMinSize) && (memSize <= rationMaxSize))
            {
                if (rationCountdown == 0)
                {
                    if (rationAfter == 0)
                    {
                        if (rationRandom)
                            rationAfter = internalReadHardwareRandomNumber() % rationInterval;
                        else
                            rationAfter = rationInterval - 1;

                        return TRUE;
                    }
                    else
                    {
                        rationAfter--;
                    }
                }
                else
                {
                    rationCountdown--;
                }
            }
        }
    }

    return FALSE;
}


/*****************************************************************************/


static void CheckMemTrack(const TaskTrack *tt, const MemTrack *mt,
                          const char *sourceFile, uint32 sourceLine,
                          MemDebugCalls call,
                          void **realAddr, int32 *realSize,
                          const char *banner)
{
int32      freeSize;
uint32     adjustment;
int32     *mem;
int32     *originalMem;

    adjustment = 0;

    mem = (int32 *)mt->mt_Memory;
    if (mt->mt_CookieValue)
    {
        if (mt->mt_TrackSize)
        {
            if ((uint32)mem & 0x07)
                adjustment = 4;
            else
                adjustment = 8;
        }

        originalMem = mem;
        mem         = (void *)((uint32)mem - COOKIE_SIZE - adjustment);
        freeSize    = mt->mt_MemorySize + COOKIE_SIZE*2 + adjustment;

        if (!CheckMem(mem,COOKIE_SIZE,mt->mt_CookieValue))
        {
            if (banner)
                OUTPUT(("\n%s:",banner));

            OutputTitle("cookie before allocation was modified");
            OutputAllocator(tt,mt);

            if (call == MEMDEBUG_CALL_GETMEMTRACKSIZE)
                OutputGetTrackSize(originalMem,sourceFile,sourceLine);
            else if (call != MEMDEBUG_CALL_SANITYCHECK)
                OutputDeallocator(originalMem,mt->mt_MemorySize,sourceFile,sourceLine,call);
            else
                OUTPUT(("  Memory Block            : addr $%08x\n",originalMem));

            OutputCookies(mt,mem);
            MemError();
        }

        if (!CheckMem((void *)((uint32)mem + freeSize - COOKIE_SIZE),COOKIE_SIZE,mt->mt_CookieValue))
        {
            if (banner)
                OUTPUT(("\n%s:",banner));

            OutputTitle("cookie after allocation was modified");
            OutputAllocator(tt,mt);

            if (call == MEMDEBUG_CALL_GETMEMTRACKSIZE)
                OutputGetTrackSize(originalMem,sourceFile,sourceLine);
            else if (call != MEMDEBUG_CALL_SANITYCHECK)
                OutputDeallocator(originalMem,mt->mt_MemorySize,sourceFile,sourceLine,call);
            else
                OUTPUT(("  Memory Block            : addr $%08x\n",originalMem));

            OutputCookies(mt,(void *)((uint32)mem + freeSize - COOKIE_SIZE));
            MemError();
        }
        freeSize -= adjustment;
    }
    else
    {
        freeSize = mt->mt_MemorySize;
    }

    if (realAddr)
    {
        if (mt->mt_TrackSize)
            *realAddr = (void *)((uint32)mem + adjustment);
        else
            *realAddr = mem;
    }

    if (realSize)
        *realSize = freeSize;
}


/*****************************************************************************/


void *MemDebugAlloc(PagePool *pp, int32 memSize, uint32 memFlags, uint32 careBits,
                    uint32 stateBits, const char *sourceFile, uint32 sourceLine,
                    MemDebugCalls call)
{
Item        task;
MemTrack    mt;
void       *mem;
bool        doCookie;
uint32      adjustment;
bool        result;
void       *originalMem;
int32       originalSize;

    LockSem();

    if (memSize <= 0)
    {
        OutputTitle("attempt to allocate a block of <= 0 bytes in size");
        OutputNewAllocator(memSize,memFlags,sourceFile,sourceLine,call);
        MemError();
        UnlockSem();

        return NULL;
    }

    if (memFlags & MEMTYPE_ILLEGAL)
    {
        OutputTitle("illegal allocation flags specified");
        OutputNewAllocator(memSize,memFlags,sourceFile,sourceLine,call);
        MemError();
        UnlockSem();

        return NULL;
    }

    if (ration)
    {
        if (DoMemRation(pp, memSize))
        {
            if (rationVerbose)
            {
                OutputTitle("allocation denied due to rationing");
                OutputNewAllocator(memSize,memFlags,sourceFile,sourceLine,call);
            }

            if (rationBreakpoint)
                DebugBreakpoint();

            return NULL;
        }
    }

    /* allocate the memory that the caller wants */

    if ((careBits > COOKIE_SIZE) || !padCookies)
    {
        /* We don't do cookies if the user doesn't want 'em, or
         * if the alignment needs exceed the cookie size, since the
         * cookie would ruin the alignment.
         */

        doCookie     = FALSE;
        originalSize = memSize;
        mem          = AllocFromPagePool(pp, memSize, memFlags, careBits, stateBits);
    }
    else
    {
        doCookie     = TRUE;
        originalSize = memSize + COOKIE_SIZE*2;
        mem          = AllocFromPagePool(pp, memSize + COOKIE_SIZE*2, memFlags, careBits, stateBits);
    }

    originalMem = mem;

    if ((KB_FIELD(kb_Flags) & KB_MEMDEBUG) == 0)
    {
        UnlockSem();
        return mem;
    }

    if (mem)
    {
        if (doCookie)
        {
            /* put our cookies in place */

            if (memFlags & MEMTYPE_TRACKSIZE)
            {
                if ((uint32)mem & 0x07)
                    adjustment = 4;
                else
                    adjustment = 8;

                mem = (void *)((uint32)mem - adjustment);
                FillMem(mem,COOKIE_SIZE,cookieValue);

                mem = (void *)((uint32)mem + COOKIE_SIZE + adjustment);
                FillMem((void *)((uint32)mem + memSize), COOKIE_SIZE, cookieValue);

                ((int32 *)mem)[-1] = memSize;
            }
            else
            {
                FillMem(mem,COOKIE_SIZE,cookieValue);
                mem = (void *)((uint32)mem + COOKIE_SIZE);
                FillMem((void *)((uint32)mem + memSize), COOKIE_SIZE, cookieValue);
            }
        }

        if (allocPatterns)
        {
            /* fill the allocated memory with a pattern, but
             * only if the user didn't ask for it to be filled
             * by the allocator
             */
            if (!(MEMTYPE_FILL & memFlags))
            {
                if (call != MEMDEBUG_CALL_REALLOCMEM)
                {
                    FillMem(mem,memSize,MEMDEBUG_ALLOC_PATTERN);
                }
                else
                {
                    /* when expanding an allocation, we should
                     * zap the newly created region...
                     */
                }
            }
        }

        mt.mt_Next        = NULL;
        mt.mt_Memory      = mem;
        mt.mt_MemorySize  = memSize;
        mt.mt_MemoryFlags = memFlags;
        mt.mt_File        = sourceFile;
        mt.mt_LineNum     = sourceLine;
        mt.mt_Call        = call;
        mt.mt_TrackSize   = (memFlags & MEMTYPE_TRACKSIZE) ? TRUE : FALSE;

        if (doCookie)
            mt.mt_CookieValue = cookieValue;
        else
            mt.mt_CookieValue = 0;

        if (pp->pp_Owner)
        {
            task = pp->pp_Owner->t.n_Item;
        }
        else
        {
            task = -1;
        }

        if (CURRENTTASK)
            result = AddAllocation(&mt,task);
        else
            result = internalAddAllocation(&mt,task);

        if (!result)
        {
            if (memFlags & MEMTYPE_TRACKSIZE)
                ((int32 *)originalMem)[-1] = memSize;

            if (freePatterns)
                FillMem(originalMem,originalSize,MEMDEBUG_FREE_PATTERN);

            FreeToPagePool(pp,originalMem,originalSize);

            mem = NULL;
        }
    }

    if (!mem && checkAllocFailures)
    {
        OutputTitle("unable to allocate memory");
        OutputNewAllocator(memSize,memFlags,sourceFile,sourceLine,call);
        MemError();
    }

    UnlockSem();

    return mem;
}


/*****************************************************************************/


void MemDebugFree(PagePool *pp, void *mem, int32 memSize,
                  const char *sourceFile, uint32 sourceLine,
                  MemDebugCalls call)
{
Task      *t;
TaskTrack *tt;
MemTrack  *mt;
MemTrack  *prev;
int32      testSize;

    if (!mem)
        return;

    LockSem();

    if (memSize < 0)
        testSize = ((int32 *)mem)[-1];
    else
        testSize = memSize;

    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
    {
        /* find the node we have created for this task/thread */
        ScanList(&tasks,tt,TaskTrack)
        {
            mt   = tt->tt_Memory[HASH_MEM(mem)];
            prev = (MemTrack *)&tt->tt_Memory[HASH_MEM(mem)];  /* sneaky... */
            while (mt)
            {
                if (mt->mt_Memory == mem)
                {
                    if (tt->tt_Task >= 0)
                    {
                        t = (Task *)CheckItem(tt->tt_Task,KERNELNODE,TASKNODE);
                        if (t)
                        {
                            if  (!IsSameTaskFamily(t,pp->pp_Owner))
                            {
                                OutputTitle("memory being freed by a different task family than when allocated");
                                OutputAllocator(tt,mt);
                                OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
                                MemError();
                            }
                        }
                    }

                    /* We've found a memory tracking node, free it */
                    if (mt->mt_TrackSize)
                    {
                        if (memSize != TRACKED_SIZE)
                        {
                            OutputTitle("size value other than TRACKED_SIZE given for deallocation of MEMTYPE_TRACKSIZE");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
                            MemError();
                        }

                        if (((int32 *)mem)[-1] != mt->mt_MemorySize)
                        {
                            OutputTitle("word before allocation was modified");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
                            OUTPUT(("  Original Value          : $%08x\n",memSize));
                            OUTPUT(("  Modified Value          : $%08x\n",((int32 *)mem)[-1]));
                            MemError();
                        }
                    }
                    else
                    {
                        if (memSize != mt->mt_MemorySize)
                        {
                            OutputTitle("inconsistent size for deallocation compared to allocation");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
                            MemError();
                        }
                    }

                    CheckMemTrack(tt,mt,sourceFile,sourceLine,call,
                                  &mem,&memSize, NULL);

                    if (freePatterns)
                        FillMem(mem,memSize,MEMDEBUG_FREE_PATTERN);

                    if (mt->mt_TrackSize)
                    {
                        ((int32 *)mem)[-1] = memSize;
                        memSize = TRACKED_SIZE;
                    }

                    FreeToPagePool(pp,mem,memSize);

                    if (CURRENTTASK)
                        RemAllocation(tt,mt,prev);
                    else
                        internalRemAllocation(tt,mt,prev);

                    UnlockSem();

                    return;
                }
                else if (((uint32)mem + (uint32)testSize - 1 >= (uint32)mt->mt_Memory)
                      && ((uint32)mem < (uint32)mt->mt_Memory + mt->mt_MemorySize))
                {
                    OutputTitle("address/size overlaps other allocation");
                    OutputAllocator(tt,mt);
                    OUTPUT(("  Overlapped Range        : addr $%08x, size %d\n",mt->mt_Memory,mt->mt_MemorySize));
                    OutputDeallocator(mem, testSize, sourceFile, sourceLine, call);
                    MemError();
                    UnlockSem();

                    return;
                }

                prev = mt;
                mt   = mt->mt_Next;
            }
        }
    }

    if (!IsMemReadable(mem, testSize))
    {
        OutputTitle("memory range not in valid RAM");
        OutputDeallocator(mem,testSize,sourceFile,sourceLine,call);
        MemError();
    }
    else if ((memSize < 0) && (memSize != TRACKED_SIZE))
    {
        OutputTitle("negative size supplied for deallocation");
        OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
        MemError();
    }
    else if (((uint32)mem & (sizeof(FreeBlock) - 1))
          && ((memSize != TRACKED_SIZE) || ((uint32)mem & 0x3)))
    {
        OutputTitle("memory pointer incorrectly aligned");
        OutputDeallocator(mem,memSize,sourceFile,sourceLine,call);
        MemError();
    }
    else
    {
        /* NOTE: This unfortunately won't catch the error of freeing a previously
         *       freed block.
         */

        /* We've never seen this allocation before, but the pointer is
         * suitably aligned and within RAM, and doesn't fall within the
         * range of an allocation we know about, so it appears like a good
         * value.
         *
         * This case happens when an allocation was made before MemDebug
         * was activated.
         */

        if (freePatterns)
            FillMem(mem, testSize, MEMDEBUG_FREE_PATTERN);

        FreeToPagePool(pp, mem, memSize);
    }

    UnlockSem();
}


/*****************************************************************************/


void *MemDebugRealloc(PagePool *pp, void *mem, int32 oldSize, int32 newSize, uint32 memFlags,
                      const char *sourceFile, uint32 sourceLine, MemDebugCalls call)
{
void *new;

    /* A simple implementation. Probably should be better integrated with
     * the rest of the stuff, but this is such a seldom used function that
     * it doesn't seem worth the hassle.
     */

    new = MemDebugAlloc(pp, newSize, memFlags, 0, 0, sourceFile,
                        sourceLine, call);

    if (!new)
	return NULL;

    memcpy(new, mem, newSize);

    MemDebugFree(pp, mem, oldSize, sourceFile, sourceLine, MEMDEBUG_CALL_FREEMEM);

    return new;
}


/*****************************************************************************/


int32 MemDebugGetMemTrackSize(void *mem, const char *sourceFile, uint32 sourceLine)
{
TaskTrack *tt;
MemTrack  *mt;
int32      memSize;

    if (!mem)
        return 0;

    LockSem();

    /* find the node we have created for this allocation */
    ScanList(&tasks,tt,TaskTrack)
    {
        mt = tt->tt_Memory[HASH_MEM(mem)];
        while (mt)
        {
            if (mt->mt_Memory == mem)
            {
                /* We've found a memory tracking node */
                if (!mt->mt_TrackSize)
                {
                    OutputTitle("calling GetMemTrackSize() on memory not allocated with MEMTYPE_TRACKSIZE");
                    OutputAllocator(tt,mt);
                    OutputGetTrackSize(mem,sourceFile,sourceLine);
                    MemError();
                    UnlockSem();
                    return BADPTR;
                }

                CheckMemTrack(tt,mt,sourceFile,sourceLine,MEMDEBUG_CALL_GETMEMTRACKSIZE,
                              NULL,NULL,NULL);

                memSize = mt->mt_MemorySize;

                if (memSize != ((int32 *)mem)[-1])
                {
                    OutputTitle("word before allocation was modified");
                    OutputAllocator(tt,mt);
                    OutputGetTrackSize(mem,sourceFile,sourceLine);
                    OUTPUT(("  Original Value          : $%08x\n",memSize));
                    OUTPUT(("  Modified Value          : $%08x\n",((int32 *)mem)[-1]));
                    MemError();
                }

                UnlockSem();

                return memSize;
            }
            else if (((uint32)mem >= (uint32)mt->mt_Memory)
                  && ((uint32)mem < (uint32)mt->mt_Memory + mt->mt_MemorySize))
            {
                OutputTitle("address is located in the middle of a previous allocation");
                OutputAllocator(tt,mt);
                OUTPUT(("  Previous Allocation     : addr $%08x, size %d\n",mt->mt_Memory,mt->mt_MemorySize));
                OutputGetTrackSize(mem,sourceFile,sourceLine);
                MemError();
                UnlockSem();

                return BADPTR;
            }

            mt = mt->mt_Next;
        }
    }

    if (!IsMemReadable(mem,4))
    {
        OutputTitle("memory pointer not in valid RAM");
        OutputGetTrackSize(mem,sourceFile,sourceLine);
        MemError();
    }
    else if ((uint32)mem & 0x3)
    {
        OutputTitle("memory pointer incorrectly aligned");
        OutputGetTrackSize(mem,sourceFile,sourceLine);
        MemError();
    }
    else
    {
        /* NOTE: This unfortunately won't catch the error of getting the size
         *       of a previously freed block.
         */

        /* We've never seen this allocation before, but the pointer is
         * suitably aligned and within RAM, and doesn't fall within the
         * range of an allocation we know about, so it appears like a good
         * value.
         *
         * This case happens when an allocation was made before MemDebug
         * was activated.
         */

         memSize = ((int32 *)mem)[-1];
         UnlockSem();

         return memSize;
    }

    UnlockSem();

    return BADPTR;
}


/*****************************************************************************/


Err externalCreateMemDebug(const TagArg *tags)
{
    if (tags)
    {
        /* no tags are currently supported */
	return BADTAG;
    }

    LockSem();

    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
    {
        /* MemDebug has already been created */
        UnlockSem();
        return 1;
    }

    PrepList(&tasks);
    allocPatterns      = FALSE;
    freePatterns       = FALSE;
    padCookies         = FALSE;
    breakpointOnErrors = FALSE;
    checkAllocFailures = FALSE;
    keepTaskData       = FALSE;
    cookieValue        = 0x11111111;
    memset(&globalStats, 0, sizeof(globalStats));

    ration           = FALSE;
    rationRandom     = FALSE;
    rationBreakpoint = FALSE;
    rationSuper      = TRUE;
    rationVerbose    = TRUE;
    rationInterval   = 1;
    rationCountdown  = 0;
    rationMinSize    = 0;
    rationMaxSize    = 0xffffffff;
    rationTask       = 0;
    rationAfter      = 0;

    /* we're now doing memory debugging */
    KB_FIELD(kb_Flags) |= KB_MEMDEBUG;

    UnlockSem();

    return 0;
}


/****************************************************************************/


Err externalControlMemDebug(uint32 controlFlags)
{
    if (controlFlags & ~(MEMDEBUGF_ALLOC_PATTERNS |
                         MEMDEBUGF_FREE_PATTERNS |
                         MEMDEBUGF_PAD_COOKIES |
                         MEMDEBUGF_BREAKPOINT_ON_ERRORS |
                         MEMDEBUGF_CHECK_ALLOC_FAILURES |
                         MEMDEBUGF_KEEP_TASK_DATA))
    {
        /* illegal flags */
        return PARAMERROR;
    }

    LockSem();

    if ((KB_FIELD(kb_Flags) & KB_MEMDEBUG) == 0)
    {
        UnlockSem();
        return NOTENABLED;
    }

    allocPatterns      = FALSE;
    freePatterns       = FALSE;
    padCookies         = FALSE;
    breakpointOnErrors = FALSE;
    checkAllocFailures = FALSE;
    keepTaskData       = FALSE;

    if (controlFlags & MEMDEBUGF_ALLOC_PATTERNS)
        allocPatterns = TRUE;

    if (controlFlags & MEMDEBUGF_FREE_PATTERNS)
        freePatterns = TRUE;

    if (controlFlags & MEMDEBUGF_PAD_COOKIES)
        padCookies = TRUE;

    if (controlFlags & MEMDEBUGF_BREAKPOINT_ON_ERRORS)
        breakpointOnErrors = TRUE;

    if (controlFlags & MEMDEBUGF_CHECK_ALLOC_FAILURES)
        checkAllocFailures = TRUE;

    if (controlFlags & MEMDEBUGF_KEEP_TASK_DATA)
        keepTaskData = TRUE;

    UnlockSem();

    return (0);
}


/*****************************************************************************/


Err externalRationMemDebug(const TagArg *tags)
{
TagArg *tag;
bool    changeWhen;

    LockSem();

    changeWhen = FALSE;

    while (tag = NextTagArg(&tags))
    {
        switch (tag->ta_Tag)
        {
            case RATIONMEMDEBUG_TAG_ACTIVE    : ration = (tag->ta_Arg ? TRUE : FALSE);
                                                break;

            case RATIONMEMDEBUG_TAG_TASK      : rationTask = (Item)tag->ta_Arg;
                                                break;

            case RATIONMEMDEBUG_TAG_MINSIZE   : rationMinSize = (uint32)tag->ta_Arg;
                                                break;

            case RATIONMEMDEBUG_TAG_MAXSIZE   : rationMaxSize = (uint32)tag->ta_Arg;
                                                break;

            case RATIONMEMDEBUG_TAG_COUNTDOWN : rationCountdown = (uint32)tag->ta_Arg;
                                                break;

            case RATIONMEMDEBUG_TAG_INTERVAL  : rationInterval = (uint32)tag->ta_Arg;
                                                if (rationInterval == 0)
                                                    rationInterval = 1;

                                                changeWhen = TRUE;
                                                break;

            case RATIONMEMDEBUG_TAG_RANDOM    : rationRandom = (tag->ta_Arg ? TRUE : FALSE);
                                                changeWhen   = TRUE;
                                                break;

            case RATIONMEMDEBUG_TAG_VERBOSE   : rationVerbose = (tag->ta_Arg ? TRUE : FALSE);
                                                break;

            case RATIONMEMDEBUG_TAG_BREAKPOINT_ON_RATIONING:
                                                rationBreakpoint = (tag->ta_Arg ? TRUE : FALSE);
                                                break;

            case RATIONMEMDEBUG_TAG_SUPER     : rationSuper = (tag->ta_Arg ? TRUE : FALSE);
                                                break;

            default                           : UnlockSem();
                                                return BADTAG;
        }
    }

    if (changeWhen)
    {
        if (rationRandom)
            rationAfter = internalReadHardwareRandomNumber() % rationInterval;
        else
            rationAfter = rationInterval - 1;
    }

    UnlockSem();

    return (0);
}


/*****************************************************************************/


Err externalDeleteMemDebug(void)
{
TaskTrack *tt;

    LockSem();

    KB_FIELD(kb_Flags) &= ~(KB_MEMDEBUG);

    while (TRUE)
    {
        tt = (TaskTrack *)RemHead(&tasks);
        if (!tt)
            break;

        FreeTaskTrack(tt);
    }

    padCookies   = FALSE;
    keepTaskData = FALSE;

    UnlockSem();

    return 0;
}


/*****************************************************************************/


Err DumpMemDebug(const TagArg *tags)
{
TaskTrack *tt;
MemTrack  *mt;
uint32     i;
bool       header;
bool       super;
Item       task;
TagArg    *tag;

    task  = CURRENTTASKITEM;
    super = FALSE;

    while (tag = NextTagArg(&tags))
    {
        switch (tag->ta_Tag)
        {
            case DUMPMEMDEBUG_TAG_TASK : task = (Item)tag->ta_Arg;
                                         break;

            case DUMPMEMDEBUG_TAG_SUPER: super = (tag->ta_Arg ? TRUE : FALSE);
                                         break;

            default                    : return BADTAG;
        }
    }

    LockSem();

    OutputTitle("DumpMemDebug()");

    ScanList(&tasks,tt,TaskTrack)
    {
        if (((task == 0) && (tt->tt_Task >= 0))
         || (task == tt->tt_Task)
         || ((tt->tt_Task < 0) && super))
        {
            OutputTask("Task: ", tt->tt_Task, NULL);
            OutputStats(&tt->tt_Stats);

            header = FALSE;
            for (i = 0; i < NUM_HASH_ENTRIES; i++)
            {
                mt = tt->tt_Memory[i];
                while (mt)
                {
                    if (!header)
                    {
                        header = TRUE;
                        OUTPUT(("    Allocations\n"));
                        OUTPUT(("       Address      Size  Source Line  Source File\n"));
                    }

                    if (mt->mt_File)
                    {
                        OUTPUT(("      $%08x  %7d    %5d",mt->mt_Memory,mt->mt_MemorySize,mt->mt_LineNum));
                        OUTPUT(("      %s\n",mt->mt_File));
                    }
                    else
                    {
                        OUTPUT(("      $%08x  %7d    <unknown source file>\n",mt->mt_Memory,mt->mt_MemorySize));
                    }
                    mt = mt->mt_Next;
                }
            }
            OUTPUT(("\n"));
        }
    }

    OUTPUT(("  Global History\n"));
    OutputStats(&globalStats);

    UnlockSem();

    return 0;
}


/*****************************************************************************/


Err SanityCheckMemDebug(const char *banner, const TagArg *args)
{
TaskTrack *tt;
MemTrack  *mt;
uint32     i;

    if (args)
        return BADTAG;

    LockSem();

    ScanList(&tasks,tt,TaskTrack)
    {
        for (i = 0; i < NUM_HASH_ENTRIES; i++)
        {
            mt = tt->tt_Memory[i];
            while (mt)
            {
                CheckMemTrack(tt, mt, NULL, 0, MEMDEBUG_CALL_SANITYCHECK, NULL, NULL, banner);
                mt = mt->mt_Next;
            }
        }
    }

    UnlockSem();

    return 0;
}


/*****************************************************************************/


/* This function is called when a task or thread is dying. Its role is to
 * clean up any tracking information that MemDebug is maintaining for this
 * task.
 */
void DeleteTaskMemDebug(const Task *t)
{
TaskTrack *tt;

    if (CURRENTTASK->t_ThreadTask)
    {
        /* only do this for full-fledged tasks */
        return;
    }

    LockSem();

    /* only nuke the data if we've not been asked to keep the info around */
    if (!keepTaskData)
    {
        ScanList(&tasks,tt,TaskTrack)
        {
            if (tt->tt_Task == t->t.n_Item)
            {
                RemNode((Node *)tt);
                FreeTaskTrack(tt);
                break;
            }
        }
    }

    UnlockSem();
}


/*****************************************************************************/


static const TagArg tags[2] =
{
    TAG_ITEM_NAME, (void *)"MemDebug",
    TAG_END,	   0,
};

void InitMemDebug(void)
{
Semaphore *sem;

    sem = (Semaphore *)AllocateNode((Folio *)&KB,SEMA4NODE);
    memDebugLock = internalCreateSemaphore(sem,tags);
}


/*****************************************************************************/


#else /* BUILD_MEMDEBUG */


/*****************************************************************************/


Err DumpMemDebug(const TagArg *tags)
{
    TOUCH(tags);
    return NOSUPPORT;
}

Err DumpMemDebugVA(uint32 tag, ...)
{
    TOUCH(tag);
    return NOSUPPORT;
}

Err RationMemDebugVA(uint32 tag, ...)
{
    TOUCH(tag);
    return NOSUPPORT;
}

Err SanityCheckMemDebug(const char *banner, const TagArg *args)
{
    TOUCH(banner);
    TOUCH(args);
    return NOSUPPORT;
}


/*****************************************************************************/


#endif /* BUILD_MEMDEBUG */
