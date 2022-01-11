/* @(#) mem.c 96/08/09 1.145 */

#ifdef MEMDEBUG
#undef MEMDEBUG
#endif

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <kernel/semaphore.h>
#include <kernel/lumberjack.h>
#include <kernel/bitarray.h>
#include <kernel/operror.h>
#include <kernel/memlock.h>
#include <kernel/internalf.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/folio.h>
#include <kernel/panic.h>
#include <device/mp.h>
#include <hardware/PPCasm.h>
#include <dipir/hwresource.h>
#include <dipir/hw.ram.h>
#include <file/discdata.h>
#include <loader/loader3do.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "memdebug.h"


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x)	printf x
#else
#define INFO(x)
#endif

#define DBUGV(s)	/* printf s */
#define DBUGFP(s)	/* printf s */


/*****************************************************************************/


#define	MEM_ALLOC_ALIGN_SIZE  sizeof(FreeBlock)
#define	MIN_FREE_BLOCK        sizeof(FreeBlock)
#define	MEM_ALLOC_ALIGN_MASK  (sizeof(FreeBlock) - 1)
#define ROUND_ALLOC_SIZE(x)   {x += MEM_ALLOC_ALIGN_MASK; x &= ~MEM_ALLOC_ALIGN_MASK;}


/*****************************************************************************/


static MemRegion normalMR;
static PagePool  systemPP;
static FreeBlock systemFB;


/*****************************************************************************/


#ifdef DEBUG
static void DumpPagePool(const PagePool *pp)
{
FreeBlock *fb;

    LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

    printf("Page Pool     : 0x%08x\n", pp);
    printf("Owner Task    : 0x%08x (%s)\n", pp->pp_Owner, (pp->pp_Owner ? pp->pp_Owner->t.n_Name : ""));
    printf("Writable Pages: \n");
    DumpBitRange(pp->pp_WritablePages, 0, pp->pp_MemRegion->mr_NumPages - 1, NULL);

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        printf("  Free memory, range $%08x-$%08x, size %u\n",fb,(uint32)fb + (uint32)fb->fb_Size - 1,fb->fb_Size);
        fb = fb->fb_Next;
    }

    UnlockSemaphore(pp->pp_Lock);
}
#endif


/*****************************************************************************/


static bool IsMemOwnedPagePool(const PagePool *pp, const void *p, int32 size)
{
MemRegion *mr;
uint32     start;
uint32     end;
bool       ret = FALSE;

    if (size > 0)
    {
        mr    = pp->pp_MemRegion;
        start = (uint32)p;
        end   = start + (uint32)size;

        if ((start < end)
         && (start >= (uint32)mr->mr_MemBase)
         && (end <= (uint32)mr->mr_MemTop))
        {
            if (pp->pp_Lock >= 0)
                LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

            /* do we own all the needed pages? */
            if (IsBitRangeSet(pp->pp_OwnedPages,
                (start - (uint32)mr->mr_MemBase) >> mr->mr_PageShift,
                (end - (uint32)mr->mr_MemBase - 1) >> mr->mr_PageShift))
            {
                ret = TRUE;
            }

            if (pp->pp_Lock >= 0)
                UnlockSemaphore(pp->pp_Lock);
        }
    }

    return ret;
}


/*****************************************************************************/


static bool IsMemWritablePagePool(const PagePool *pp, const void *p, int32 size)
{
MemRegion *mr;
uint32     start;
uint32     end;

    if (size > 0)
    {
        mr    = pp->pp_MemRegion;
        start = (uint32)p;
        end   = start + (uint32)size;

        if ((start < end)
         && (start >= (uint32)mr->mr_MemBase)
         && (end <= (uint32)mr->mr_MemTop))
        {
            if (pp->pp_Lock >= 0)
                LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

            /* do we have write permission to all the needed pages? */
            if (IsBitRangeSet(pp->pp_WritablePages,
                              (start - (uint32)mr->mr_MemBase) >> mr->mr_PageShift,
                              (end - (uint32)mr->mr_MemBase - 1) >> mr->mr_PageShift))
            {
                if (pp->pp_Lock >= 0)
                    UnlockSemaphore(pp->pp_Lock);

                return TRUE;
            }

            if (pp->pp_Lock >= 0)
                UnlockSemaphore(pp->pp_Lock);
        }
    }

    return FALSE;
}


/*****************************************************************************/


static bool IsMemReadablePagePool(const PagePool *pp, const void *p, int32 size)
{
MemRegion *mr;
uint8     *start;
uint8     *end;

    if (size == 0)
        return TRUE;

    mr    = pp->pp_MemRegion;
    start = (uint8 *)p;
    end   = start + size;

    if (size < 0 || start > end)
	return FALSE;

    if (start >= (uint8*)mr->mr_MemBase &&
	end <= (uint8*)mr->mr_MemTop)
    {
	/* Memory is in RAM belonging to this page pool. */
        return TRUE;
    }

    if (start >= (uint8*)KB_FIELD(kb_ROMBaseAddress) &&
	end <= (uint8*)KB_FIELD(kb_ROMEndAddress))
    {
	/* Memory is in system ROM. */
        return TRUE;
    }

    /* FIXME: if task has TASK_PCMCIA_PERM, then check PCMCIA space too? */

    return FALSE;
}



/*****************************************************************************/


bool IsMemUserOwned(const void *p, int32 size, Task *t)
{
    return IsMemOwnedPagePool(t->t_PagePool, p, size);
}


/*****************************************************************************/


bool IsMemUserWritable(const void *p, int32 size, Task *t)
{
    return IsMemWritablePagePool(t->t_PagePool, p, size);
}


/*****************************************************************************/


bool IsMemOwned(const void *p, int32 size)
{
    return IsMemUserOwned(p, size, CURRENTTASK);
}


/*****************************************************************************/


bool IsMemWritable(const void *p, int32 size)
{
    return IsMemUserWritable(p, size, CURRENTTASK);
}


/*****************************************************************************/


bool IsMemReadable(const void *p, int32 size)
{
    /* Should look like:
     *
     *  if (CURRENTTASK)
     *      return IsMemReadablePagePool(CURRENTTASK->t_PagePool, p, size);
     *
     *  return IsMemReadablePagePool(KB_FIELD(kb_PagePool), p, size);
     *
     * But since all RAM is readable, there's no sense in having this check
     * here since the final effect would be exactly the same was the single
     * statement below.
     */
    return IsMemReadablePagePool(KB_FIELD(kb_PagePool), p, size);
}


/*****************************************************************************/


void InitPagePool(PagePool *pp)
{
    pp->pp_FreeBlocks = NULL;

    if (pp != &systemPP)
    {
        if (CURRENTTASK)
        {
            pp->pp_Lock  = internalCreateItem(MKNODEID(KERNELNODE, SEMA4NODE), NULL);
        }
        else
        {
            Semaphore *semaphore = AllocateNode((Folio *) &KB, SEMA4NODE);

            pp->pp_Lock = internalCreateSemaphore(semaphore, NULL);
        }
    }
    else
    {
        pp->pp_Lock = -1;
    }

    pp->pp_PageSize      = KB_FIELD(kb_MemRegion)->mr_PageSize;
    pp->pp_PageMask      = KB_FIELD(kb_MemRegion)->mr_PageMask;
    pp->pp_Owner         = NULL;
    pp->pp_OwnedPages    = NULL;
    pp->pp_WritablePages = NULL;
    pp->pp_MemRegion     = KB_FIELD(kb_MemRegion);
}


/*****************************************************************************/


#ifdef DEBUG
static void DumpHWResources(void)
{
HWResource dev;
uint32     r;
uint32     i;

    printf("--- Hardware devices ---\n");
    dev.hwr_InsertID = 0;
    for (;;)
    {
            r = SuperQuerySysInfo(SYSINFO_TAG_HWRESOURCE, &dev, sizeof(dev));
            if (r == SYSINFO_END) break;
            if (r != SYSINFO_SUCCESS)
            {
                    printf("DumpHWResources: sysinfo err %x\n", r);
                    break;
            }
            printf("%s in slot %d.%d\n", dev.hwr_Name, dev.hwr_Channel, dev.hwr_Slot);
            printf("    perms %x, insert %d\n", dev.hwr_Perms, dev.hwr_InsertID);
            printf("    spec: ");

            for (i = 0;  i < sizeof(dev.hwr_DeviceSpecific)/sizeof(dev.hwr_DeviceSpecific[0]);  i++)
                    printf("%x ", dev.hwr_DeviceSpecific[i]);

            printf("\n");
    }
    printf("----------------------\n");
}
#endif


/*****************************************************************************/


int MatchDeviceName(const char *name, const char *fullname, uint32 part)
{
	char c1, c2;

	/*
	 * Skip to the right "part" of the name
	 * by skipping separators.
	 */
	while (part-- > 0)
	{
		while (*fullname != DEVNAME_SEPARATOR)
		{
			if (*fullname == '\0')
				return 0;
			fullname++;
		}
		fullname++;
	}

	/*
	 * Compare the given name with the device name,
	 * up to the next separator.
	 */
	while (*fullname != DEVNAME_SEPARATOR && *fullname != '\0')
	{
		if (*name == '\0')
			return 0;
		c1 = *name++;
		c2 = *fullname++;
		if (toupper(c1) != toupper(c2))
			return 0;
	}
	if (*name != '\0')
		return 0;
	return 1;
}


/*****************************************************************************/


static void CreateFreeBlock(PagePool *pool, void *freeStart, uint32 freeSize)
{
    /* We can insert this memory into the free list simply by calling
     * FreeToPagePool.
     */
    FreeToPagePool(pool, freeStart, freeSize);
}


/*****************************************************************************/


Err SuperFreeRawMem(void *freeStart, uint32 freeSize)
{
    if (CURRENTTASK && !IsPriv(CURRENTTASK))
	return BADPRIV;

    CreateFreeBlock(&systemPP, freeStart, freeSize);
    return 0;
}


/*****************************************************************************/


void InitMem(void)
{
HWResource      hwdev;
HWResource_RAM	*ram;
Err             err;
List		*list;
BootAlloc	*ba;
PersistentMem	pm;

static char mrName[32];

    KB_FIELD(kb_MemRegion) = &normalMR;
    KB_FIELD(kb_PagePool)  = &systemPP;

#ifdef DEBUG
    DumpHWResources();
#endif

    hwdev.hwr_InsertID = 0;
    do {
        err = NextHWResource(&hwdev);
        if (err < 0)
        {
            /* !!! ACK, no memory! */
            PANIC(ER_Kr_NoMemList);
        }
    } while (!MatchDeviceName("SYSMEM", hwdev.hwr_Name, DEVNAME_TYPE));

    ram                     = (HWResource_RAM *)hwdev.hwr_DeviceSpecific;
    strcpy(mrName, hwdev.hwr_Name);
    normalMR.mr.n_Name      = mrName;
    normalMR.mr.n_Size      = sizeof(MemRegion);
    normalMR.mr_MemoryType  = ram->ram_MemTypes;
    normalMR.mr_PageSize    = ram->ram_PageSize;
    normalMR.mr_PageMask    = ram->ram_PageSize - 1;
    normalMR.mr_PageShift   = FindMSB(ram->ram_PageSize);
    normalMR.mr_NumPages    = ram->ram_Size / ram->ram_PageSize;
    normalMR.mr_MemBase     = (void *)ram->ram_Addr;
    normalMR.mr_MemTop      = (uint8 *)ram->ram_Addr + ram->ram_Size - 1;
    normalMR.mr_Lock        = -1;

    /* Remove previously allocated chunks of memory from the free list. */

    InitPagePool(&systemPP);
    systemFB.fb_Size = sizeof(FreeBlock);
    systemFB.fb_Next = NULL;
    systemPP.pp_FreeBlocks = &systemFB;

    QUERY_SYS_INFO(SYSINFO_TAG_BOOTALLOCLIST, list);
    ScanList(list, ba, BootAlloc)
    {
        /* Create a free block from the end of this allocated range
	 * to the start of the next one. */
	uint8 *startp = (uint8*)ba->ba_Start + ba->ba_Size;
	uint8 *endp = ISNODE(list, ba->ba_Next) ?
		ba->ba_Next->ba_Start : (uint8*)normalMR.mr_MemTop + 1;
	if (endp - startp >= MIN_FREE_BLOCK)
	    CreateFreeBlock(&systemPP, startp, endp - startp);
    }

    /* Remember persistent memory. */
    QUERY_SYS_INFO(SYSINFO_TAG_PERSISTENTMEM, pm);
    KB_FIELD(kb_PersistentMemStart) = pm.pm_StartAddress;
    KB_FIELD(kb_PersistentMemSize) = pm.pm_Size;
    pm.pm_StartAddress = NULL;
    pm.pm_Size = 0;
    pm.pm_AppID = 0;
    SuperSetSysInfo(SYSINFO_TAG_PERSISTENTMEM, (void *)&pm, sizeof(pm));

    normalMR.mr_PublicPages   = SuperAllocMem((normalMR.mr_NumPages+7) / 8, MEMTYPE_NORMAL | MEMTYPE_FILL);

    KB_FIELD(kb_RAMBaseAddress) = (uint32)normalMR.mr_MemBase;
    KB_FIELD(kb_RAMEndAddress)  = (uint32)normalMR.mr_MemTop;

    _mtsr11((uint32)KB_FIELD(kb_RAMBaseAddress));
    _mtsr12((uint32)KB_FIELD(kb_RAMEndAddress));

    /* Find system ROM */
    hwdev.hwr_InsertID = 0;
    for (;;)
    {
        if (NextHWResource(&hwdev) < 0)
	    break;
	if (MatchDeviceName("SYSROM", hwdev.hwr_Name, DEVNAME_TYPE))
	{
	    KB_FIELD(kb_ROMBaseAddress) = hwdev.hwr_DeviceSpecific[0];
	    KB_FIELD(kb_ROMEndAddress)  = hwdev.hwr_DeviceSpecific[0] +
						hwdev.hwr_ROMSize - 1;
	    break;
	}
    }
}


/*****************************************************************************/


static const TagArg tags[2] =
{
    TAG_ITEM_NAME, (void *)"System Page Pool",
    TAG_END,	   0,
};

void FinishInitMem(void)
{
Semaphore *sem;

    sem = (Semaphore *)AllocateNode((Folio *)&KB,SEMA4NODE);
    systemPP.pp_Lock = internalCreateSemaphore(sem,tags);
}



/*****************************************************************************/


static Err MakeMemPersistent(const uint8 *p, int32 size)
{
PersistentMem   pm;
ExtVolumeLabel *evl;

    QUERY_SYS_INFO(SYSINFO_TAG_PERSISTENTMEM, pm);
    if (pm.pm_Size)
    {
        /* only one block at a time, thank you */
        return NOSUPPORT;
    }

    QUERY_SYS_INFO(SYSINFO_TAG_APPVOLUMELABEL, evl);
    pm.pm_AppID        = evl->dl_ApplicationID;
    pm.pm_StartAddress = (void *)p;
    pm.pm_Size         = (uint32)size;

    return SuperSetSysInfo(SYSINFO_TAG_PERSISTENTMEM, (void *)&pm, sizeof(pm));
}


/*****************************************************************************/


static void RemoveWriteAccessAll(uint32 firstBit, uint32 lastBit)
{
Node *n;
Task *t;

    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        t = Task_Addr(n);
        if (!t->t_ThreadTask)
        {
            ClearBitRange(t->t_PagePool->pp_WritablePages,firstBit,lastBit);
	}
    }
}


/*****************************************************************************/


static void GrantWriteAccessAll(uint32 firstBit, uint32 lastBit)
{
Node *n;
Task *t;

    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        t = Task_Addr(n);
        if (!t->t_ThreadTask)
        {
            SetBitRange(t->t_PagePool->pp_WritablePages,firstBit,lastBit);
	}
    }
}


/*****************************************************************************/


int32 ControlPagePool(PagePool *pp, uint8 *p, int32 size, ControlMemCmds cmd,
                      Item taskItem)
{
MemRegion *mr;
PagePool  *taskPP;
Task      *task;
uint32     firstBit;
uint32     lastBit;
uint32     delta;
Err        err;

    task = NULL;
    if (taskItem)
    {
        task = (Task *)CheckItem(taskItem,KERNELNODE,TASKNODE);
        if (!task || (cmd == MEMC_PERSISTENT))
            return BADITEM;
    }

    mr = pp->pp_MemRegion;

    if (size <= 0)
        return 0;

    if ((uint32)size > (uint32)mr->mr_MemTop - (uint32)p)
        return BADPTR;

    if (pp->pp_Lock >= 0)
        externalLockSemaphore(pp->pp_Lock, SEM_WAIT);

    /* round the buffer start and length to page boundaries */
    delta    = (int32)p & pp->pp_PageMask;
    p       -= delta;                   /* align down to beginning of page */
    size    += delta;
    size    += pp->pp_PageMask;
    size    &= ~pp->pp_PageMask;        /* round to full pages */

    firstBit = ((uint32)p - (uint32)mr->mr_MemBase) >> mr->mr_PageShift;
    lastBit  = ((uint32)p + (size - 1) - (uint32)mr->mr_MemBase) >> mr->mr_PageShift;

    if (pp->pp_OwnedPages && (!IsBitRangeSet(pp->pp_OwnedPages,firstBit,lastBit)))
    {
        /* current task family doesn't own the memory block */

        if (!IsPriv(CURRENTTASK))
        {
            if (cmd != MEMC_NOWRITE)
            {
                /* family must own the memory to do any cmd except MEMC_NOWRITE */
                UnlockSemaphore(pp->pp_Lock);
                return NOTOWNER;
            }

            if ((!task) || (!IsSameTaskFamily(task,pp->pp_Owner)))
            {
                /* Since we don't own the memory, we can't change permission
                 * state for anybody except our family.
                 */
                UnlockSemaphore(pp->pp_Lock);
                return NOTOWNER;
            }
        }
    }

    err = 0;

    if (task)
    {
	taskPP = task->t_PagePool;
        switch (cmd)
        {
            case MEMC_GIVE   : /* give the memory to a specific task */
                               if (pp->pp_OwnedPages)
                                   ClearBitRange(pp->pp_OwnedPages,firstBit,lastBit);

            		       SetBitRange(taskPP->pp_WritablePages,firstBit,lastBit);
            		       SetBitRange(taskPP->pp_OwnedPages,firstBit,lastBit);
                               break;

            case MEMC_NOWRITE: /* prevent this task's family from writing to the memory block */
			       InvokeMemLockHandlers(task,p,size);
                               ClearBitRange(taskPP->pp_WritablePages,firstBit,lastBit);
                               break;

            case MEMC_OKWRITE: /* allow this task to write to the memory block */
            		       SetBitRange(taskPP->pp_WritablePages,firstBit,lastBit);
                               break;
        }
    }
    else
    {
        switch (cmd)
        {
            case MEMC_GIVE   : /* give the memory back to the system */
                               RemoveWriteAccessAll(firstBit, lastBit);
                               InvokeMemLockHandlers(NULL,p,size);
                               ClearBitRange(pp->pp_OwnedPages, firstBit, lastBit);
                               ClearBitRange(mr->mr_PublicPages, firstBit, lastBit);
                               LogPagesFreed(p,size);
                               FreeToPagePool(KB_FIELD(kb_PagePool), p, size);
                               break;

            case MEMC_NOWRITE: /* prevent all tasks from writing to the memory block */
                               RemoveWriteAccessAll(firstBit, lastBit);
                               InvokeMemLockHandlers(NULL,p,size);
                               ClearBitRange(mr->mr_PublicPages, firstBit, lastBit);
                               break;

            case MEMC_OKWRITE: /* allow all tasks to write to the memory block */
                               GrantWriteAccessAll(firstBit, lastBit);
                               SetBitRange(mr->mr_PublicPages, firstBit, lastBit);
                               break;

            case MEMC_PERSISTENT: /* mark the memory as persistent memory */
			       err = MakeMemPersistent(p,size);
			       if (err == 0)
                                   ClearBitRange(pp->pp_OwnedPages, firstBit, lastBit);
                               break;

        }
    }

    UpdateFence();

    if (pp->pp_Lock >= 0)
        UnlockSemaphore(pp->pp_Lock);

    return err;
}


/*****************************************************************************/


Err internalGetPersistentMem(PersistentMemInfo *info, uint32 infoSize)
{
PersistentMemInfo pi;
Err               result;

    if (!IsMemWritable(info, infoSize))
        return BADPTR;

    if (KB_FIELD(kb_PersistentMemStart) == NULL)
        return NOPERSISTENTMEM;

    result = ControlPagePool(KB_FIELD(kb_PagePool),
                             KB_FIELD(kb_PersistentMemStart),
                             KB_FIELD(kb_PersistentMemSize),
                             MEMC_GIVE,
                             CURRENTTASKITEM);
    if (result >= 0)
    {
        pi.pinfo_Start    = KB_FIELD(kb_PersistentMemStart);
        pi.pinfo_NumBytes = KB_FIELD(kb_PersistentMemSize);

        KB_FIELD(kb_PersistentMemStart) = NULL;
        KB_FIELD(kb_PersistentMemSize)  = 0;

        if (infoSize > sizeof(PersistentMemInfo))
        {
            memset(info,0,sizeof(infoSize));
            infoSize = sizeof(PersistentMemInfo);
        }

        memcpy(info,&pi,infoSize);
    }

    return result;
}


/*****************************************************************************/


/* This flushes a single resource out of the system, and returns TRUE if it
 * flushes anything, and FALSE otherwise.
 */
static bool FlushResources(void)
{
    return FALSE;
}


/*****************************************************************************/


#ifdef BUILD_MEMDEBUG

static void ValidateFreeBlock(const PagePool *pp, const FreeBlock *fb,
                              const char *call)
{
uint32     start;
uint32     end;
MemRegion *mr;

    if ((KB_FIELD(kb_Flags) & KB_MEMDEBUG) == 0)
        return;

    if (((uint32)fb & MEM_ALLOC_ALIGN_MASK) == 0)
    {
        if (fb->fb_Size >= sizeof(FreeBlock))
        {
            if ((fb->fb_Size & MEM_ALLOC_ALIGN_MASK) == 0)
            {
                mr    = pp->pp_MemRegion;
                start = (uint32)fb;
                end   = start + fb->fb_Size;

                if ((start < end)
                 && (start >= (uint32)mr->mr_MemBase)
                 && (end <= (uint32)mr->mr_MemTop))
                {
                    if (pp->pp_OwnedPages == NULL)
                    {
                        /* supervisor page pool */
                        return;
                    }

                    if (IsBitRangeSet(pp->pp_OwnedPages,
                                      (start - (uint32)mr->mr_MemBase) >> mr->mr_PageShift,
                                      (end - (uint32)mr->mr_MemBase - 1) >> mr->mr_PageShift))
                    {
                        return;
                    }
                }
            }
        }
    }

    printf("WARNING: The free memory list has been corrupted\n");
    printf("         The corruption was detected when calling %s()\n",call);
    printf("         Current task/thread is '%s', ", CURRENTTASK->t.n_Name);

    if (pp->pp_Owner)
        printf("         corrupted list for task '%s'\n", pp->pp_Owner->t.n_Name);
    else
        printf("         Supervisor memory list\n");

    printf("         Triggering the debugger...\n");
    DebugBreakpoint();
}

#else

#define ValidateFreeBlock(pp,fb,call)

#endif


/*****************************************************************************/


static int32 ScavengePagePool(PagePool *pp)
{
int32      total;
FreeBlock *fb;
FreeBlock *prev;
FreeBlock *new;
uint32     size;
uint32     headSlop;
uint32     tailSlop;

    total = 0;

    LockSemaphore(pp->pp_Lock,SEM_WAIT);

    /* Look for blocks of memory in the free list which contain whole
     * pages. Remove all these whole pages from the free list, and return
     * then to the system page pool.
     */

    prev = pp->pp_FreeBlocks;
    fb   = prev->fb_Next;
    while (fb)
    {
        ValidateFreeBlock(pp,fb,"ScavengeMem");

        if (fb->fb_Size >= pp->pp_PageSize)
        {
            if ((uint32)fb & pp->pp_PageMask)
            {
                /* if the node is not page-aligned, we don't
                 * want to include the slop before the next page boundary.
                 */
                headSlop = (pp->pp_PageSize - ((uint32)fb & pp->pp_PageMask));
                size     = fb->fb_Size - headSlop;
            }
            else
            {
                headSlop = 0;
                size     = fb->fb_Size;
            }

            if (size >= pp->pp_PageSize)
            {
                if (headSlop)
                {
                    /* make the headSlop area into its own node */
                    new          = (FreeBlock *)((uint32)fb + headSlop);
                    new->fb_Next = fb->fb_Next;
                    new->fb_Size = fb->fb_Size - headSlop;
                    fb->fb_Next  = new;
                    fb->fb_Size  = headSlop;
                    prev         = fb;
                    fb           = new;
                }

                tailSlop = size & pp->pp_PageMask;
                if (tailSlop)
                {
                    /* make the tail slop area into its own node */
                    new           = (FreeBlock *)((uint32)fb + fb->fb_Size - tailSlop);
                    new->fb_Next  = fb->fb_Next;
                    new->fb_Size  = tailSlop;
                    fb->fb_Next   = new;
                    fb->fb_Size  -= tailSlop;
                }

                /* remove the page-aligned node from the list */
                prev->fb_Next = fb->fb_Next;
                total        += fb->fb_Size;

                /* return the pages */
                ControlMem(fb,fb->fb_Size,MEMC_GIVE,0);

                fb = prev;
            }
        }

        prev = fb;
        fb   = fb->fb_Next;
    }

    UnlockSemaphore(pp->pp_Lock);

    return total;
}


/*****************************************************************************/


static void MergeBlocks(PagePool *pp, FreeBlock *prev, FreeBlock *next,
                        void *p, int32 size)
{
FreeBlock *fb;

    /* Be sure to never merge with the first FreeBlock, since it is a
     * dummy anchor node.
     */

    if ((prev != pp->pp_FreeBlocks) &&
       ((uint32)prev + (uint32)prev->fb_Size == (uint32)p))
    {
        /* The end of the previous block corresponds with the beginning of
         * the new block. We can just extend the previous block's size to
         * encompass the new block.
         */
        prev->fb_Size += size;

        if ((uint32)prev + (uint32)prev->fb_Size == (uint32)next)
        {
            /* The previous block now extends to the next block.
             * We can therefore merge the two into one bigger block.
             */
            prev->fb_Next  = next->fb_Next;
            prev->fb_Size += next->fb_Size;
        }
    }
    else if ((uint32)p + (uint32)size == (uint32)next)
    {
        /* The end of the new block corresponds to the beginning of the
         * next block. So we just suck the next block into the new block.
         */
        fb            = (FreeBlock *)p;
        fb->fb_Next   = next->fb_Next;
        fb->fb_Size   = size + next->fb_Size;
        prev->fb_Next = fb;
    }
    else
    {
        /* Plain insertion with no coallessing */
        fb            = (FreeBlock *)p;
        fb->fb_Next   = next;
        fb->fb_Size   = size;
        prev->fb_Next = fb;
    }
}


/*****************************************************************************/


void FreeToPagePool(PagePool *pp, void *p, int32 size)
{
FreeBlock *next;
FreeBlock *prev;

    if ((size == 0) || (p == NULL))
        return;

    if (size < 0)
    {
        /* deal with allocations done with MEMTYPE_TRACKSIZE */

        size = *(int32 *)((uint32)p - sizeof(int32));
        if ((uint32)p & MEM_ALLOC_ALIGN_MASK)
        {
            /* If the allocation is not aligned on an eight byte boundary,
             * it means there is 4 bytes of extra space before the supplied
             * pointer.
             */
            p     = (void *)((uint32)p - sizeof(int32));
            size += 4;
        }
        else
        {
            /* If the allocation is aligned on an eight byte boundary,
             * it means there is 8 bytes of extra space before the supplied
             * pointer.
             */
            p     = (void *)((uint32)p - sizeof(FreeBlock));
            size += sizeof(FreeBlock);
        }
    }

    ROUND_ALLOC_SIZE(size);

    DBUGFP(("Free 0x%x bytes @ 0x%x\n", p, size));

    /* find the right spot to insert this new free block */

    if (pp->pp_Lock >= 0)
        LockSemaphore(pp->pp_Lock, SEM_WAIT);

    prev = pp->pp_FreeBlocks;
    next = prev->fb_Next;
    while (next)
    {
        ValidateFreeBlock(pp,next,"FreeMem");

        if ((uint32)next > (uint32)p)
            break;

        prev = next;
        next = next->fb_Next;
    }

    /* now actually add the memory to the free block list */
    MergeBlocks(pp,prev,next,p,size);

    if (pp->pp_Lock >= 0)
        UnlockSemaphore(pp->pp_Lock);
}


/*****************************************************************************/


void *AllocFromPagePool(PagePool *pp, int32 usersize, uint32 flags,
                        uint32 careBits, uint32 stateBits)
{
int32     *result;
uint32     reqsize;
uint32     nstart, nend;
uint32     pstart, pend;
uint32     leftover;
FreeBlock *fb;
FreeBlock *prev;
uint32     extra;
uint32     shortfall;

    if (flags & MEMTYPE_ILLEGAL)
        return NULL;

    if (usersize <= 0)
    {
        /* negative allocation size, can't have that */
        return NULL;
    }

    reqsize = (uint32)usersize;

    extra = 0;
    if (flags & MEMTYPE_TRACKSIZE)
    {
        if (careBits)
        {
            /* If doing an aligned and tracked allocation, pad the size
             * suitably.
             */
            extra    = sizeof(FreeBlock);
            reqsize += extra;
        }
        else
        {
            /* If doing a tracked allocation, pad out the size */
            reqsize += sizeof(int32);
        }
    }

    ROUND_ALLOC_SIZE(reqsize);
    stateBits &= careBits;	/*  Don't let unimportant bits skew tests.  */

    if (careBits)
    {
        if (stateBits & 0x7)
        {
            /* can't return a pointer with these bits set */
            return NULL;
        }
        careBits |= 0x7;	/*  Internal restriction.  */
    }

    if (pp->pp_Lock >= 0)
        LockSemaphore(pp->pp_Lock, SEM_WAIT);

restart:

    shortfall = 0;
    prev      = pp->pp_FreeBlocks;
    fb        = prev->fb_Next;
    while (TRUE)
    {
        if (!fb)
        {
            /* we didn't find a big enough free block */

            if (pp->pp_Owner)
            {
                /* if we aren't looking in the supervisor pool, scavenge... */

                /* clean up anything we can */
                ScavengePagePool(pp);

                /* try and get more pages for the free list */
                result = AllocMemPages(reqsize + shortfall,
                                       flags & MEMTYPE_TYPESMASK);
                if (result)
                {
                    FreeToPagePool(pp, result, *result);
                    goto restart;
                }
            }
            else
            {
                /* if we're looking in the supervisor pool, then try to flush
                 * some resources to free up some space.
                 */
                if (FlushResources())
                {
                    goto restart;
                }
            }

            if (pp->pp_Lock >= 0)
                UnlockSemaphore(pp->pp_Lock);

            return NULL;
        }

        ValidateFreeBlock(pp,fb,"AllocMem");

        if (fb->fb_Size >= reqsize)
        {
            nstart   = (uint32)fb + extra;
            nend     = nstart + fb->fb_Size - extra;
            pstart   = nstart;
            pend     = pstart + reqsize - extra;

            /* WARNING: In the case of a tracked and aligned allocation,
             *          nstart and pstart will be offset from the
             *          actual start of the block. Be careful!
             */

            if ((pstart & careBits) != stateBits)
            {
                /* Compute smallest integer greater than pstart that satisfies
                 * bit state requirements. This is done as follows:
                 *
                 *      o Compute "error" bits.  This is:
                 *
                 *        error = (pstart ^ stateBits) & careBits
                 *
                 *      o Find the LSB in ~careBits whose value is greater
                 *        than the error. This is the MSB of the expression:
                 *
                 *        term = (~(notcare - error)) & notcare
                 *
                 *        This value will be used to increment pstart if
                 *        necessary.
                 *
                 *      o Compute the bits we may safely change. This is:
                 *
                 *        safe = notcare & ~(term - 1)
                 *
                 *      o Force the address bits to the desired state. If
                 *        this yields a smaller value, then increment the
                 *        address such that only the safe bits are affected.
                 *        This is done by OR-ing in the careBits so that
                 *        carries out of intermediate safe regions will
                 *        propogate across the regions we care about, then
                 *        masking them out again.
                 *
                 * My most profuse thanks to Tom Rokicki for figuring this
                 * out.
                 */
                uint32 notcare, term, safe, r;

                /* compute LSB that encompasses erroneous bits */
                notcare = ~careBits;

                term = notcare & ~(notcare - ((pstart ^ stateBits) & careBits));
                term = 1 << FindMSB(term);

                /* compute bits we may safely diddle */
                safe = notcare & ~(term - 1);

                /* try forcing bit state */
                r = (pstart & safe) | stateBits;
                if (r < pstart)
                {
                    /* That didn't work. Add term to roll to next candidate
                     * value, OR in careBits to propogate carries.
                     */
                    r = (((pstart | careBits) + term) & safe & notcare) | stateBits;
                }

                pstart = r;
                pend   = pstart + reqsize - extra;

                if (pend > nend)
                {
                    /* The block was big enough, until we tried to align it.
                     * Then it got too small. Figure out how much to add to
                     * fb_Size to make it big enough, and request that
                     * additional amount the next time we allocate pages
                     * from the supervisor pool.
                     */
                    term = fb->fb_Size + pend - nend - reqsize;
                    if (shortfall < term)
                        shortfall = term;

                    prev = fb;
                    fb   = fb->fb_Next;
                    continue;
                }

                /* handle the bytes left before the block */
                fb->fb_Size = pstart - nstart;
                prev        = fb;
            }
            else
            {
                /* unlink the block */
                prev->fb_Next = fb->fb_Next;
            }

            /* If we have a trailing free area, add
             * it back into the free list.
             */
            leftover = nend - pend;
            if (leftover)
            {
                fb            = (FreeBlock *)pend;
                fb->fb_Next   = prev->fb_Next;
                fb->fb_Size   = leftover;
                prev->fb_Next = fb;

		DBUGFP(("Freesplit 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
            }

            result = (int32 *)(pstart - extra);
            break;
        }

        prev = fb;
        fb   = fb->fb_Next;
    }

    if (pp->pp_Lock >= 0)
        UnlockSemaphore(pp->pp_Lock);

    /* The allocation worked. Deal with some of the flags */

    if (flags & MEMTYPE_TRACKSIZE)
    {
        if (careBits)
        {
            result   = (int32 *)((uint32)result + extra);
            reqsize -= extra;
        }
        else
        {
            result++;
            reqsize -= sizeof(int32);
        }
        result[-1] = usersize;
    }

    if (flags & MEMTYPE_FILL)
        memset(result,(int)(flags & MEMTYPE_FILLMASK),reqsize);
    else
        *(uint32 *)result = reqsize;	/*  Not 'usersize'?  */

    return result;
}


/*****************************************************************************/


void *ReallocFromPagePool(PagePool *pp, void *oldMem, int32 oldSize,
                          int32 newSize, uint32 flags)
{
int32 reqSize;

    if (flags & MEMTYPE_ILLEGAL)
        return NULL;

    if ((oldSize == 0) || (oldMem == NULL))
        return NULL;

    if (newSize <= 0)
        return NULL;

    if (oldSize < 0)
    {
        if ((flags & MEMTYPE_TRACKSIZE) == 0)
        {
            /* can't alter track state for now */
            return NULL;
        }

        /* deal with allocations done with MEMTYPE_TRACKSIZE */
        oldMem  = (void *)((uint32)oldMem - sizeof(int32));
        oldSize = *(int32 *)oldMem + sizeof(int32);
        reqSize = newSize + sizeof(int32);
    }
    else
    {
        if (flags & MEMTYPE_TRACKSIZE)
        {
            /* can't alter track state for now */
            return NULL;
        }

        reqSize = newSize;
    }

    ROUND_ALLOC_SIZE(reqSize);
    ROUND_ALLOC_SIZE(oldSize);

    if (reqSize < oldSize)
    {
        FreeToPagePool(pp,(void *)((uint32)oldMem + reqSize),oldSize - reqSize);
    }
    else if (reqSize > oldSize)
    {
        /* we don't yet support making an allocation bigger */
        return NULL;
    }

    if (flags & MEMTYPE_TRACKSIZE)
    {
        *(int32 *)oldMem = newSize;
        oldMem           = (void *)((uint32)oldMem + sizeof(int32));
    }

    return oldMem;
}


/*****************************************************************************/


static uint32 CountSetBits(uint32 *array, uint32 firstBit, uint32 lastBit)
{
uint32 i;
uint32 count;

    count = 0;
    for (i = firstBit; i <= lastBit; i++)
        if (IsBitSet(array,i))
            count++;

    return count;
}


/*****************************************************************************/


static void GetPagePoolInfo(const PagePool *pp, MemInfo *minfo, uint32 flags)
{
uint32     largest;
uint32	   size;
MemRegion *mr;
FreeBlock *fb;
uint32     headSlop;
Node      *n;
Task      *t;
uint32     taskPages;
uint32     allTaskPages;
uint32     freePages;
uint32     freeBytes;

    TOUCH(flags);

    freeBytes = 0;

    LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        ValidateFreeBlock(pp,fb,"GetMemInfo");

        freeBytes += fb->fb_Size;
        fb         = fb->fb_Next;
    }
    taskPages = CountSetBits(pp->pp_OwnedPages,0,pp->pp_MemRegion->mr_NumPages - 1);

    UnlockSemaphore(pp->pp_Lock);

    minfo->minfo_TaskAllocatedPages = taskPages;
    minfo->minfo_TaskAllocatedBytes = (taskPages * pp->pp_PageSize) - freeBytes;

    pp           = KB_FIELD(kb_PagePool);
    mr           = pp->pp_MemRegion;
    largest      = 0;
    allTaskPages = 0;
    freePages    = 0;
    freeBytes    = 0;

    LockSemaphore(pp->pp_Lock,SEM_WAIT | SEM_SHAREDREAD);

    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        t = Task_Addr(n);
	if (!t->t_ThreadTask)
	    allTaskPages += CountSetBits(t->t_PagePool->pp_OwnedPages,0,t->t_PagePool->pp_MemRegion->mr_NumPages - 1);
    }

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        ValidateFreeBlock(pp,fb,"GetMemInfo");

        if (fb->fb_Size >= pp->pp_PageSize)
        {
            if ((uint32)fb & pp->pp_PageMask)
            {
                /* if the node is not page-aligned, we don't
                 * want to include the slop before the next page boundary.
                 */
                headSlop = (pp->pp_PageSize - ((uint32)fb & pp->pp_PageMask));
                size     = fb->fb_Size - headSlop;
            }
            else
            {
                size = fb->fb_Size;
            }

            size = size & (~pp->pp_PageMask);

            if (size > largest)
                largest = size;

            freePages += size / pp->pp_PageSize;
        }
        freeBytes += fb->fb_Size;
        fb = fb->fb_Next;
    }

    UnlockSemaphore(pp->pp_Lock);

    minfo->minfo_FreePages            = freePages;
    minfo->minfo_LargestFreePageSpan  = largest;
    minfo->minfo_SystemAllocatedPages = mr->mr_NumPages - freePages - allTaskPages;
    minfo->minfo_SystemAllocatedBytes = (minfo->minfo_SystemAllocatedPages * mr->mr_PageSize) - freeBytes;
    minfo->minfo_OtherAllocatedPages  = allTaskPages - taskPages;
}


/*****************************************************************************/


int32 GetMemTrackSize(const void *p)
{
    if (!p)
        return 0;

#ifdef BUILD_MEMDEBUG
    if (KB_FIELD(kb_Flags) & KB_MEMDEBUG)
        return MemDebugGetMemTrackSize(p, NULL, 0);
#endif

    return *(int32 *)((uint32)p - sizeof(int32));
}


/*****************************************************************************/


/* Make sure all nodes in a PagePool's free block list are valid */
static bool ValidatePagePool(PagePool *pp, Task *t)
{
FreeBlock *fb;

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        if (((uint32)fb & MEM_ALLOC_ALIGN_MASK) == 0)
        {
            if (fb->fb_Size >= sizeof(FreeBlock))
            {
                if ((fb->fb_Size & MEM_ALLOC_ALIGN_MASK) == 0)
                {
                    if (IsMemUserWritable(fb, fb->fb_Size, t))
                    {
                        if (IsMemUserOwned(fb, fb->fb_Size, t))
                        {
                            /* this block is fine, go do the next one */
                            fb = fb->fb_Next;
                            continue;
                        }
                        else
                        {
                            DBUGV(("Validate failed! NotOwned 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
                        }
                    }
                    else
                    {
                        DBUGV(("Validate failed! NotWritable 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
                    }
                }
                else
                {
                    DBUGV(("Validate failed! Bad Size 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
                }
            }
            else
            {
                DBUGV(("Validate failed! Too small 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
            }
        }
        else
        {
            DBUGV(("Validate failed! Not Aligned 0x%x bytes @ 0x%x\n", fb->fb_Size, fb));
        }

        return FALSE;
    }

    return TRUE;
}



/*****************************************************************************/


/* FIXME: This code is not secure as it doesn't protect against DMA */
int32 SuperControlUserMem(uint8 *p, int32 size, ControlMemCmds cmd, Item it,
			  Task *t)
{
int32 result;

    externalLockSemaphore(t->t_PagePool->pp_Lock,SEM_WAIT);

    if (ValidatePagePool(t->t_PagePool, t))
    {
        result = ControlPagePool(t->t_PagePool, p, size, cmd, it);
    }
    else
    {
        result = BADPTR;
    }

    externalUnlockSemaphore(t->t_PagePool->pp_Lock);

    return result;
}



/*****************************************************************************/


/* FIXME: This code is not secure as it doesn't protect against DMA */
void *SuperAllocUserMem(int32 size, uint32 flags, Task *t)
{
void *result;

    externalLockSemaphore(t->t_PagePool->pp_Lock,SEM_WAIT);

    if (ValidatePagePool(t->t_PagePool, t))
    {
        result = AllocUserMem(size,flags, t);
    }
    else
    {
        result = NULL;
    }

    externalUnlockSemaphore(t->t_PagePool->pp_Lock);

    return result;
}


/*****************************************************************************/


/* FIXME: This code is not secure as it doesn't protect against DMA */
void SuperFreeUserMem(void *p, int32 size, Task *t)
{
void *validateMem;
int32 validateSize;

    if (!p)
        return;

    externalLockSemaphore(t->t_PagePool->pp_Lock,SEM_WAIT);

    if (ValidatePagePool(t->t_PagePool, t))
    {
        /* Get pointer and size to validate: the entire span of memory blocks
         * to be affected by the FreeMem() call.
         */

        if (size < 0)
        {
            /* deal with allocations done with MEMTYPE_TRACKSIZE */

            int32 * const sizep = (int32 *)p - 1;

            if (!IsMemReadable(sizep, sizeof *sizep))
            {
                INFO(("WARNING: Unable to free user memory from supervisor mode\n"));
                INFO(("read %d\n", IsMemReadable (sizep, sizeof *sizep)));
                externalUnlockSemaphore(t->t_PagePool->pp_Lock);
                return;
            }

            validateSize = *sizep;
            if ((uint32)p & MEM_ALLOC_ALIGN_MASK)
            {
                /* If the allocation is not aligned on an eight byte boundary,
                 * it means there is 4 bytes of extra space before the supplied
                 * pointer.
                 */
                validateMem   = (void *)((uint32)p - sizeof(int32));
                validateSize += sizeof(int32);
            }
            else
            {
                /* If the allocation is aligned on an eight byte boundary,
                 * it means there is 8 bytes of extra space before the supplied
                 * pointer.
                 */
                validateMem   = (void *)((uint32)p - sizeof(FreeBlock));
                validateSize += sizeof(FreeBlock);
            }
        }
        else
        {
            /* otherwise, just check what we were given */

            validateMem  = p;
            validateSize = size;
        }
        ROUND_ALLOC_SIZE(validateSize);

        if ((((uint32)validateMem & MEM_ALLOC_ALIGN_MASK) == 0)
         && (validateSize >= sizeof(FreeBlock))
         && IsMemUserWritable (validateMem, validateSize, t)
         && IsMemUserOwned (validateMem, validateSize, t))
        {
            /* all clear, free the memory */
            FreeUserMem(p, size, t);
        }
        else
        {
            INFO(("WARNING: Unable to free user memory from supervisor mode\n"));
            INFO(("write %d, own %d\n",
                  IsMemUserWritable (validateMem, validateSize, t),
                  IsMemUserOwned (validateMem, validateSize, t)));
        }
    }
    else
    {
        INFO(("WARNING: Unable to free user memory from supervisor mode (validate failed)\n"));
    }

    externalUnlockSemaphore(t->t_PagePool->pp_Lock);
}


/*****************************************************************************/


int32 ScavengeMem(void)
{
    return ScavengePagePool(CURRENTTASK->t_PagePool);
}


/*****************************************************************************/


int32 GetPageSize(uint32 flags)
{
    if (flags)
        return PARAMERROR;

    return (int32)KB_FIELD(kb_MemRegion)->mr_PageSize;
}


/*****************************************************************************/


void GetMemInfo(MemInfo *minfo, uint32 infoSize, uint32 flags)
{
MemInfo mi;

    memset(&mi,0,sizeof(MemInfo));

    if (flags == 0)
        GetPagePoolInfo(CURRENTTASK->t_PagePool,&mi,flags);

    if (infoSize > sizeof(MemInfo))
    {
        memset(minfo,0,infoSize);
        infoSize = sizeof(MemInfo);
    }

    memcpy(minfo,&mi,infoSize);
}


/*****************************************************************************/


int32 externalControlMem(uint8 *p, int32 size, ControlMemCmds cmd, Item it)
{
    if ((cmd < 0) || (cmd > MEMC_PERSISTENT))
        return MakeKErr(ER_SEVER,ER_C_NSTND,ER_Kr_BadMemCmd);

    return ControlPagePool(CURRENTTASK->t_PagePool,p,size,cmd,it);
}


/*****************************************************************************/


void *internalAllocMemPages(int32 size, uint32 flags)
{
void  *mem;
int32  pageSize;

    if (flags & (MEMTYPE_ILLEGAL | MEMTYPE_TRACKSIZE))
        return NULL;

    pageSize = GetPageSize(flags & MEMTYPE_TYPESMASK);
    if (pageSize < 0)
        return NULL;

    mem = AllocFromPagePool(KB_FIELD(kb_PagePool), ALLOC_ROUND(size,pageSize), flags, pageSize-1, 0);
    if (mem)
    {
        if (CURRENTTASKITEM >= 0)
            ControlPagePool(KB_FIELD(kb_PagePool), mem, size, MEMC_GIVE, CURRENTTASKITEM);

        LogPagesAllocated(mem, size, FALSE);
    }

    return mem;
}


/*****************************************************************************/


void internalFreeMemPages(void *p, int32 size)
{
    ControlPagePool(CURRENTTASK->t_PagePool, p, size, MEMC_GIVE, 0);
}
