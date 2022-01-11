/* @(#) commands.c 96/10/08 1.34 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/folio.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/device.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/debug.h>
#include <kernel/bitarray.h>
#include <kernel/lumberjack.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "commands.h"


/*****************************************************************************/


static uint32 ConvertNum(char *str)
{
    if (*str == '$')
    {
        str++;
        return strtoul(str,0,16);
    }

    return strtoul(str,0,0);
}


/*****************************************************************************/


static Task *SpecialFindTask(char *s)
{
Item  it;
int32 n;
Task *task;

    it   = FindNamedItem(MKNODEID(KERNELNODE,TASKNODE),s);
    task = (Task *)CheckItem(it,KERNELNODE,TASKNODE);

    if (!task)
    {
        n = ConvertNum(s);

        /* was input an Item? */
        task = (Task *)CheckItem((Item)n,KERNELNODE,TASKNODE);
    }

    return (task);
}


/*****************************************************************************/


static Err Help(ScriptContext *sc, char *args)
{
    TOUCH(sc);
    TOUCH(args);

    printf("3DO Shell Built-In Commands:\n\n");

    printf("alias <name> [str] : create a file-path alias\n");
    printf("sleep <seconds>    : sleep for <seconds>\n");

    printf("setbg              : set shell's default to not wait for tasks (&)\n");
    printf("setfg              : set shell's default to wait for tasks (#)\n");
    printf("setpri [priority]  : set shell's priority\n");
    printf("setcd [directory]  : change current directory\n");
    printf("setminmem          : set free mem to minimum guaranteed\n");
    printf("setmaxmem          : set free mem to maximum available\n\n");

#ifdef BUILD_LUMBERJACK
    printf("log {events}       : control the logging of events\n");
    printf("dumplogs           : display the contents of the event logs\n\n");
#endif

    printf("expunge                   : purge unused demand-loaded items from memory\n");
    printf("showavailmem              : display available memory in the system\n");
    printf("showfreeblocks [item|name]: display the list of free blocks\n");
    printf("showcd                    : print current directory\n");
    printf("showerror <error #>       : print an error string from a system error code\n");
    printf("showmemmap [item|name]    : display memory page owners\n");
    printf("showmemusage [address]    : display memory usage info\n");
    printf("showtask [item|name]      : print task information\n\n");

    printf("killtask <item|name>      : Kill a task\n\n");
    printf("sendsignal <task> <sigs>  : Send signals to a task\n");

    return 0;
}


/*****************************************************************************/


static Err ShowError(ScriptContext *sc, char *args)
{
    TOUCH(sc);

    PrintfSysErr(ConvertNum(args));

    return 0;
}


/*****************************************************************************/


static Err Sleep(ScriptContext *sc, char *args)
{
Item  timerIO;
int32 secs;

    TOUCH(sc);

    secs = ConvertNum(args);

    timerIO = CreateTimerIOReq();
    if (timerIO >= 0)
    {
        WaitTime(timerIO,secs,0);
        DeleteTimerIOReq(timerIO);
    }

    return timerIO;
}


/*****************************************************************************/


static Err SetBGMode(ScriptContext *sc, char *args)
{
    TOUCH(args);

    sc->sc_DefaultBackgroundMode = TRUE;
    return 0;
}


/*****************************************************************************/


static Err SetFGMode(ScriptContext *sc, char *args)
{
    TOUCH(args);

    sc->sc_DefaultBackgroundMode = FALSE;
    return 0;
}


/*****************************************************************************/


static Err SetPri(ScriptContext *sc, char *args)
{
uint32  newpri;
Err     result;

    TOUCH(sc);

    result = 0;
    if (args && args[0])
    {
        newpri = ConvertNum(args);
        if (newpri < 256)
        {
            printf("Old priority: %u\n",SetItemPri(CURRENTTASKITEM,(uint8)newpri));
            printf("New priority: %u\n",newpri);
        }
        else
        {
            printf("SetPri: Priority out of range 0..255\n");
            result = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadPriority);
        }

    }
    else
    {
        printf("Current priority: %u\n",CURRENTTASK->t.n_Priority);
    }

    return result;
}


/*****************************************************************************/


static Err ShowCD(ScriptContext *sc, char *args)
{
int32 result;
char  path[256];

    TOUCH(sc);
    TOUCH(args);

    result = GetDirectory(path, 256);
    if (result >= 0)
        printf("Current directory is '%s'\n", path);

    return result;
}


/*****************************************************************************/


static Err SetCD(ScriptContext *sc, char *args)
{
Err result;

    TOUCH(sc);

    if (args[0] != 0)
    {
        result = ChangeDirectory(args);
    }
    else
    {
        ShowCD(sc, NULL);
        result = 0;
    }

    return result;
}


/*****************************************************************************/


static Err MakeAlias(ScriptContext *sc, char *args)
{
char *aliasname;
char *aliastext;

    TOUCH(sc);

    aliasname = args;
    if (*aliasname == 0)
    {
        printf("Alias: missing alias name\n");
        return BADNAME;
    }

    aliastext = aliasname+1;
    while (*aliastext)
    {
        if (*aliastext == ' ')
        {
            *aliastext++ = '\0';
            break;
        }
        aliastext++;
    }

    while (*aliastext)
    {
        if (*aliastext != ' ')
            break;

        aliastext++;
    }

    return CreateAlias(aliasname, aliastext);
}


/*****************************************************************************/


static Err KillTask(ScriptContext *sc, char *args)
{
Task *task;
Item  it;
Err   result;

    TOUCH(sc);

    task = SpecialFindTask(args);
    if (!task)
    {
        printf("KillTask: task not found\n");
        return BADNAME;
    }

    it = task->t.n_Item;
    result = DeleteItem(it);

    if (LookupItem(it) == 0)
    {
        printf("KillTask: task %s now removed\n",args);
    }
    else
    {
        printf("KillTask: task %s survived the assault!\n",args);
    }

    return result;
}


/*****************************************************************************/


static uint32 CheckStack(uint32* base,uint32 size)
{
  uint32 *p;
  uint32 i;
  uint32 depth=0;

  if(base) {
    p = base;
    if(*p != 0xDEADCAFE)
      depth = 0xFFFF; /* may have blown stack */
    else {
      for(i=0;*p==0xDEADCAFE;p++,i++)
	;
      depth = size-(i*4);
    }
  }
  return depth;
}


/*****************************************************************************/


static void DumpTask(Task *task)
{
uint32   s;
uint32   u;
TimeVal  tv;

    ConvertTimerTicksToTimeVal((TimerTicks *)&task->t_ElapsedTime,&tv);
    s = tv.tv_Seconds;
    u = tv.tv_Microseconds;

    printf(" $%05x $%08x $%08x $%08x $%08x %3d %6d %2d:%02d:%02d.%03d $%04x $%04x",
           task->t.n_Item,
           task,
           task->t_ThreadTask,
           task->t_WaitBits,
           task->t_SigBits,
           task->t.n_Priority,
           task->t_NumTaskLaunch,
           s/3600,
           (s/60)%60,
           s%60,
           u/1000,
           CheckStack(task->t_StackBase,task->t_StackSize),
           CheckStack(task->t_SuperStackBase,task->t_SuperStackSize));

    printf(" %c %c %s\n",
          task->t.n_ItemFlags & ITEMNODE_PRIVILEGED ? 'P' : 'U',
          task->t.n_Flags & TASK_RUNNING ? 'X' :
          task->t.n_Flags & TASK_READY ? 'R' :
          task->t.n_Flags & TASK_WAITING ? 'W' : ' ',
          task->t.n_Name);
}


/*****************************************************************************/


/* defines the maximum number of tasks/threads on a single task list
 * that DumpTaskList() will deal with. Warning: increasing this will
 * increase the amount of stack used by DumpTaskList()!
 */
#define MAX_TASK_LIST 128

static void DumpTaskList(List *l)
{
ItemNode *in;
Item      tasks[MAX_TASK_LIST];
uint32    taskCnt;
uint32    i;
Task     *task;
MinNode  *n;

    taskCnt = 0;
    SCANLIST(l,n,MinNode)
    {
        in = (ItemNode *)Task_Addr(n);

        tasks[taskCnt++] = in->n_Item;
        if (taskCnt == MAX_TASK_LIST)
            break;
    }

    for (i = 0; i < taskCnt; i++)
    {
        task = (Task *)LookupItem(tasks[i]);
        if (task)
            DumpTask(task);
    }
}


/*****************************************************************************/


static void DumpResources(Task *task)
{
int   i;
int   pi = 0;
Item *p = task->t_ResourceTable;

    for (i = 0; i < task->t_ResourceCnt; i++)
    {
        if (*p >= 0)
        {
            if (*p & ITEM_WAS_OPENED)
            {
                printf("($%05x) ",*p & ~ITEM_WAS_OPENED);
            }
            else
            {
                printf(" $%05x  ",*p);
            }
            pi++;

            if (pi > 7)
            {
                pi = 0;
                printf("\n");
            }
        }
        p++;
    }

    if (pi)
        printf("\n");
}


/*****************************************************************************/


static void DumpPagePool(const PagePool *pp)
{
uint32     unused;
int32      i;
uint32     largest;
int32      pages;
FreeBlock *fb;
MemInfo    mi;

    LockSemaphore(pp->pp_Lock,SEM_WAIT | SEM_SHAREDREAD);

    if (pp->pp_OwnedPages)
    {
        i     = pp->pp_MemRegion->mr_NumPages / 32;
        pages = 0;

        while (i--)
            pages += CountBits(pp->pp_OwnedPages[i]);
    }
    else
    {
        GetMemInfo(&mi,sizeof(mi),MEMTYPE_NORMAL);
        pages = mi.minfo_SystemAllocatedPages;
    }

    unused  = 0;
    largest = 0;

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        if (largest < fb->fb_Size)
            largest = fb->fb_Size;

        unused += fb->fb_Size;
        fb = fb->fb_Next;
    }

    UnlockSemaphore(pp->pp_Lock);

    printf("num pages %d, unused %u ($%x), largest unused %u ($%x)\n",pages,unused,unused,largest,largest);
}


/*****************************************************************************/


static Err ShowFreeBlocks(ScriptContext *sc, char *args)
{
Task      *task;
FreeBlock *fb;
PagePool  *pp;

    TOUCH(sc);

    if (args[0])
    {
        task = SpecialFindTask(args);
        if (!task)
        {
            printf("ShowFreeBlocks: unable to find task '%s'\n",args);
            return BADNAME;
        }
        pp = task->t_PagePool;
        printf("Free memory blocks for task '%s':\n",task->t.n_Name);
    }
    else
    {
        pp = KernelBase->kb_PagePool;
        printf("Free memory blocks for system:\n");
    }

    LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        printf("  Free memory, range $%08x-$%08x, size %u\n",fb,(uint32)fb + (uint32)fb->fb_Size - 1,fb->fb_Size);
        fb = fb->fb_Next;
    }

    UnlockSemaphore(pp->pp_Lock);

    return 0;
}


/*****************************************************************************/


static void DumpPPCRegisters(const Task *t)
{
uint32 i,j,k;

    for (i = 0; i < 8; i++)
    {
        printf("  ");
        for (j = 0; j < 4; j++)
        {
            k = i + (j*8);
            printf("GPR[%2d] %08lx  ",k,t->t_RegisterSave.rb_GPRs[k]);
        }
        printf("\n");
    }

    printf("  PC  $%08x, LR $%08x, MSR $%08x\n  CTR $%08x, CR $%08x, XER $%08x\n",
            t->t_RegisterSave.rb_PC,
            t->t_RegisterSave.rb_LR,
            t->t_RegisterSave.rb_MSR,
            t->t_RegisterSave.rb_CTR,
            t->t_RegisterSave.rb_CR,
            t->t_RegisterSave.rb_XER);

    printf("\n");
}


/*****************************************************************************/


static Err ShowTask(ScriptContext *sc, char *args)
{
Task   *task;
uint32	s;
uint32	u;
Task   *p;

    TOUCH(sc);

    if (args[0])
    {
        task = SpecialFindTask(args);
        if (!task)
        {
            printf("ShowTask: task not found\n");
            return BADNAME;
        }

        {
        TimeVal tv;

            ConvertTimerTicksToTimeVal((TimerTicks *)&task->t_ElapsedTime,&tv);
            s = tv.tv_Seconds;
            u = tv.tv_Microseconds;
        }

	p = task->t_ThreadTask;

	printf("Name            : %s\n",task->t.n_Name);
	printf("Item            : $%05x\n",task->t.n_Item);
	printf("Address         : $%08x\n",task);
	printf("Priority        : %d\n",task->t.n_Priority);

	printf("n_Flags         : $%02x ( ",task->t.n_Flags);
	if (task->t.n_Flags & TASK_READY)   printf("READY ");
	if (task->t.n_Flags & TASK_WAITING) printf("WAITING ");
	if (task->t.n_Flags & TASK_RUNNING) printf("RUNNING ");
	if (task->t.n_Flags & TASK_QUANTUM_EXPIRED)  printf("QUANTUM_EXPIRED ");
	printf(")\n");

	printf("t_ItemFlags     : $%02x ( ",task->t.n_ItemFlags);
	if (task->t.n_ItemFlags & ITEMNODE_PRIVILEGED)   printf("PRIVILEGED ");
	printf(")\n");

#define TASK_SUPERVISOR_ONLY   0x00000010  /* The task only runs in super mode */
#define TASK_PCMCIA_PERM       0x00000020  /* Permission to access PCMCIA space */
#define TASK_EXCEPTION         0x00000100  /* the task triggered an exception   */

	printf("t_Flags         : $%08x ( ",task->t_Flags);
	if (task->t_Flags & TASK_DATADISCOK)       printf("DATADISCOK ");
	if (task->t_Flags & TASK_FREE_STACK)       printf("FREE_STACK ");
	if (task->t_Flags & TASK_EXITING)          printf("EXITING ");
	if (task->t_Flags & TASK_SINGLE_STACK)     printf("SINGLE_STACK ");
	if (task->t_Flags & TASK_SUPERVISOR_ONLY)  printf("SUPERVISOR_ONLY ");
	if (task->t_Flags & TASK_PCMCIA_PERM)      printf("PCMCIA_PERM ");
	if (task->t_Flags & TASK_EXCEPTION)        printf("EXCEPTION ");
	printf(")\n");

	printf("Forbid          : %d\n",task->t_Forbid);

        if (p)
            printf("Thread Of Task  : %s (item $%05x)\n",p->t.n_Name,p->t.n_Item);

	printf("Time Quantum    : %dms\n",task->t_MaxUSecs / 1000);
	printf("Launch Count    : %d\n",task->t_NumTaskLaunch);
	printf("Accumulated Time: %d:%02d", s/3600, (s/60)%60);
	printf(":%02d.%03d\n", s%60, u/1000);

	printf("Sigs Allocated  : $%08x\n",task->t_AllocatedSigs);
	printf("Sigs Received   : $%08x\n",task->t_SigBits);
	printf("Sigs Wait       : $%08x\n",task->t_WaitBits);
	printf("Item Wait       : $%08x\n",task->t_WaitItem);

	printf("Stack Base      : $%08x\n",task->t_StackBase);
	printf("Stack Size      : %d\n",task->t_StackSize);
	printf("Super Stack Base: $%08x\n",task->t_SuperStackBase);
	printf("Super Stack Size: %d\n",task->t_SuperStackSize);
        printf("Task Memory     : ");
        DumpPagePool(task->t_PagePool);

        printf("\nTask Item Table:\n");
        DumpResources(task);

        printf("\nTask Registers:\n");
        DumpPPCRegisters(task);
    }
    else
    {
        printf("  Item    Task     Parent   WaitSigs  RecvSigs  Pri Launch     Time      USD   SSD  T S Name\n");
        DumpTaskList(&KernelBase->kb_Tasks);
    }

    return 0;
}


/*****************************************************************************/


static Err SetMinMem(ScriptContext *sc, char *args)
{
    TOUCH(sc);
    TOUCH(args);

    return NOSUPPORT;
}


/*****************************************************************************/


static Err SetMaxMem(ScriptContext *sc, char *args)
{
    TOUCH(sc);
    TOUCH(args);

    return NOSUPPORT;
}


/*****************************************************************************/


static Err DoSendSignal(ScriptContext *sc, char *args)
{
Task   *task;
uint32  sigs;
uint32  i;

    TOUCH(sc);

    i = 0;
    while (args[i] && (args[i] != ' '))
        i++;

    if (args[i] == 0)
    {
        printf("SendSignal: missing signal numbers\n");
        return -1;
    }

    args[i++] = 0;

    task = SpecialFindTask(args);
    if (!task)
    {
        printf("SendSignal: task not found\n");
        return -1;
    }

    while (args[i] == ' ')
        i++;

    sigs = ConvertNum(&args[i]);
    return SendSignal(task->t.n_Item,sigs);
}


/*****************************************************************************/


static Err ShowAvailMem(ScriptContext *sc, char *args)
{
    TOUCH(sc);
    TOUCH(args);

    printf("Supervisor Memory: ");
    DumpPagePool(KernelBase->kb_PagePool);

    return 0;
}


/*****************************************************************************/


#define MAX_TASKS 26

static Err ShowMemMap(ScriptContext *sc, char *args)
{
uint32     i;
bool       found;
Task      *task;
MemRegion *mr;
uint32     pageNum;
Item       tasks[MAX_TASKS];
uint32     taskCnt;
uint32     lineLen;
Item       specialItem;
void      *ptr;
char       line[80];
uint32     index;
uint32     low, high;
FreeBlock *fb;
uint32     taskAllocated;
uint32     systemAllocated;

    TOUCH(sc);

    specialItem = -1;
    if (args[0])
    {
        task = SpecialFindTask(args);
        if (!task)
        {
            printf("ShowMemMap: unable to find task '%s'\n",args);
            return BADNAME;
        }
        specialItem = task->t.n_Item;
    }

    taskCnt = 0;
    ScanList(&KernelBase->kb_Tasks,ptr,void)
    {
        task = (Task *)((uint32)ptr - offsetof(Task,t_TasksLinkNode));
        if (!task->t_ThreadTask)
        {
            if (taskCnt >= MAX_TASKS)
                break;

            tasks[taskCnt++] = task->t.n_Item;
        }
    }

    printf("Legend: .=Free, S=System");

    lineLen = 38;
    for (i = 0; i < taskCnt; i++)
    {
        task = (Task *)LookupItem(tasks[i]);
        if (task)
        {
            if (lineLen > 70)
            {
                printf(",\n        ");
                lineLen = 8;
            }
            else
            {
                printf(", ");
                lineLen += 2;
            }

            if (tasks[i] == specialItem)
                printf("*=%s",task->t.n_Name);
            else
                printf("%c=%s",i + 'a',task->t.n_Name);

            lineLen += 2 + strlen(task->t.n_Name);
        }
    }

    printf("\n");

    systemAllocated = 0;
    taskAllocated   = 0;

    mr = KernelBase->kb_MemRegion;
    printf("Region: '%s', range $%08x-$%08x, page size %d\n",mr->mr.n_Name,mr->mr_MemBase,mr->mr_MemTop,mr->mr_PageSize);

    index   = 0;
    pageNum = 0;
    while (pageNum < mr->mr_NumPages)
    {
        found = FALSE;
        for (i = 0; i < taskCnt; i++)
        {
            task = (Task *)CheckItem(tasks[i],KERNELNODE,TASKNODE);
            if (task)
            {
                if (IsBitSet(task->t_PagePool->pp_OwnedPages,pageNum))
                {
                    if (tasks[i] == specialItem)
                        line[index++] = '*';
                    else
                        line[index++] = i + 'a';

                    taskAllocated++;
                    found = TRUE;
                    break;
                }
            }
        }

        if (!found)
        {
            /* No task owns this page, let's see if it is free, or in use
             * by system code
             */

            low  = (pageNum << mr->mr_PageShift) + (uint32)mr->mr_MemBase;
            high = low + mr->mr_PageSize - 1;

            fb = KernelBase->kb_PagePool->pp_FreeBlocks->fb_Next;
            while (fb)
            {
                if ((uint32)fb <= low)
                {
                    if ((uint32)fb + fb->fb_Size > high)
                    {
                        line[index++] = '.';
                        break;
                    }
                }

                fb = fb->fb_Next;
            }

            if (!fb)
            {
                line[index++] = 'S';
                systemAllocated++;
            }
        }

        pageNum++;
        if (pageNum % 64 == 0)
        {
            line[index] = 0;
            printf("%s\n",line);
            index = 0;
        }
        else if (pageNum % 16 == 0)
        {
            line[index++] = ' ';
	}
    }

    if (index > 0)
    {
        line[index] = 0;
        printf("%s\n",line);
    }

    printf("Pages: System %d, Tasks %d, Free %d\n",systemAllocated,taskAllocated,mr->mr_NumPages - taskAllocated - systemAllocated);

    return 0;
}


/*****************************************************************************/


List *FindNodeList(Node *n)
{
Node *lastnode;

    lastnode = NextNode(n);
    while (lastnode->n_Next)
        lastnode = NextNode(lastnode);

    return (List *)((uint32)lastnode - offsetof(List,ListAnchor.tail.links));
}


/*****************************************************************************/


typedef enum
{
    RT_CODE,
    RT_DATA,
    RT_BSS,
    RT_WASTEDDATA,
    RT_USERSTACK,
    RT_SUPERSTACK,
    RT_RESOURCETABLE,
    RT_ITEM,
    RT_ITEMNAME,
    RT_ITEMTABLE,
    RT_EXPORTTABLE,
    RT_IMPORTTABLE,
    RT_RELOCTABLE,
    RT_FREEBLOCK,
    RT_FREEPAGES,
    RT_USERDYNAMIC,
    RT_SUPERDYNAMIC,
    RT_SPECIAL,
    RT_ALL
} RangeTypes;

static const char * const formats[] =
{
    "%s code",
    "%s data",
    "%s bss",
    "%s wasted data",
    "%s user stack",
    "%s super stack",
    "%s resource table",
    "item",
    "item name (\"%s\")",
    "item table",
    "%s export table",
    "%s import table",
    "%s relocation table",
    "%s free block",
    "unallocated pages",
    "%s dynamic allocation",
    "%s dynamic allocation",
    "%s"
};

static const char * const sizeFormats[] =
{
    "  Code                : %7u bytes",
    "  Data                : %7u bytes",
    "  BSS                 : %7u bytes",
    "  Wasted Data         : %7u bytes",
    "  User Stacks         : %7u bytes",
    "  Super Stacks        : %7u bytes",
    "  Task Resource Tables: %7u bytes",
    "  Items               : %7u bytes",
    "  Item Names          : %7u bytes",
    "  Item Tables         : %7u bytes",
    "  Export Tables       : %7u bytes",
    "  Import Tables       : %7u bytes",
    "  Relocation Tables   : %7u bytes",
    "  Free Blocks         : %7u bytes",
    "  Free Pages          : %7u bytes",
    "  Dynamic User Alloc  : %7u bytes",
    "  Dynamic Super Alloc : %7u bytes",
    "  Special             : %7u bytes",
};

typedef struct
{
    MinNode     r;
    uint32      r_Start;
    uint32      r_NumBytes;
    RangeTypes  r_Type;
    const char *r_Description;
} Range;

typedef struct
{
    List   mu_Ranges;               /* assigned ranges       */
    List   mu_Pool;                 /* free Range structures */
    uint32 mu_SizeTotals[RT_ALL];   /* total amounts         */

    void  *mu_WhatAddress;          /* particular address to get info about */
    Range  mu_WhatRange;
    bool   mu_WhatFound;
} MemUsageState;


/*****************************************************************************/


static void Coallese(List *ranges, List *pool, Range *r)
{
Range *old;

    if (r != (Range *)FirstNode(ranges))
    {
        old = (Range *)PrevNode(r);
        if (old->r_Start + old->r_NumBytes == r->r_Start)
        {
            if (old->r_Type == r->r_Type)
            {
                old->r_NumBytes += r->r_NumBytes;
                RemNode((Node *)r);
                AddHead(pool, (Node *)r);
                r = old;
            }
        }
    }

    if (r != (Range *)LastNode(ranges))
    {
        old = (Range *)NextNode(r);
        if (r->r_Start + r->r_NumBytes == old->r_Start)
        {
            if (r->r_Type == old->r_Type)
            {
                r->r_NumBytes += old->r_NumBytes;
                RemNode((Node *)old);
                AddHead(pool, (Node *)old);
            }
        }
    }
}


/*****************************************************************************/


static void AddRange(MemUsageState *mu, void *start, uint32 numBytes,
                     RangeTypes type, const char *desc, bool coallese)
{
List   *ranges;
List   *pool;
uint32 *sizes;
Range  *r;
Range  *old;
Range  *split;
uint32  end;

    ranges = &mu->mu_Ranges;
    pool   = &mu->mu_Pool;
    sizes  = mu->mu_SizeTotals;

    r = (Range *)RemHead(pool);
    if (!r)
    {
        printf("Overflowed range pool!\n");
        return;
    }

    numBytes = ALLOC_ROUND(numBytes, 8);

    r->r_Start       = (uint32)start;
    r->r_NumBytes    = numBytes;
    r->r_Type        = type;
    r->r_Description = desc;
    sizes[type]     += numBytes;

    if (((uint32)mu->mu_WhatAddress >= (uint32)start)
     && ((uint32)mu->mu_WhatAddress < (uint32)start + numBytes))
    {
        mu->mu_WhatRange = *r;
        mu->mu_WhatFound = TRUE;
    }

    ScanList(ranges, old, Range)
    {
        if (old->r_Start >= r->r_Start + r->r_NumBytes)
        {
            /* if the range we've encountered is beyond the range we're
             * adding, we're done.
             */
            break;
        }

        if (old->r_Start + old->r_NumBytes <= r->r_Start)
        {
            /* if the range we've encountered comes fully before the new
             * range we're adding, we're not there yet.
             */
            continue;
        }

        /* there's an overlap between the range we're adding and
         * the range we've encountered.
         */

        if (old->r_Start < r->r_Start)
        {
            InsertNodeAfter((Node *)old, (Node *)r);

            sizes[old->r_Type] -= old->r_NumBytes;

            end             = old->r_Start + old->r_NumBytes - 1;
            old->r_NumBytes = r->r_Start - old->r_Start;

            sizes[old->r_Type] += old->r_NumBytes;

            if (r->r_Start + r->r_NumBytes - 1 < end)
            {
                split = (Range *)RemHead(pool);
                if (!split)
                {
                    printf("Overflowed range pool!\n");
                    return;
                }

                InsertNodeAfter((Node *)r, (Node *)split);
                split->r_Start       = r->r_Start + r->r_NumBytes;
                split->r_NumBytes    = end - split->r_Start + 1;
                split->r_Type        = old->r_Type;
                split->r_Description = old->r_Description;

                sizes[split->r_Type] += split->r_NumBytes;
            }

            if (coallese)
                Coallese(ranges, pool, r);

            return;
        }
        else
        {
            sizes[old->r_Type] -= old->r_NumBytes;

            end = old->r_Start + old->r_NumBytes - 1;
            if (end <= r->r_Start + r->r_NumBytes - 1)
            {
                InsertNodeAfter((Node *)old, (Node *)r);
                RemNode((Node *)old);
                AddHead(pool, (Node *)old);

                if (coallese)
                    Coallese(ranges, pool, r);

                return;
            }
            old->r_Start    = r->r_Start + r->r_NumBytes;
            old->r_NumBytes = end - old->r_Start + 1;

            sizes[old->r_Type] += old->r_NumBytes;
            break;
        }
    }
    InsertNodeBefore((Node *)old, (Node *)r);

    if (coallese)
        Coallese(ranges, pool, r);
}


/*****************************************************************************/


#define ITEMS_PER_BLOCK	512

static ItemEntry *GetItemEntryPtr(uint32 i)
{
ItemEntry  *p;
ItemEntry **ipt;
uint32      j;

    ipt = KernelBase->kb_ItemTable;
    if (KernelBase->kb_MaxItem <= i)	return 0;
    j = i/ITEMS_PER_BLOCK;	/* which block */
    p = ipt[j];
    i -= j*ITEMS_PER_BLOCK;	/* which one in this block? */

    return p + i;
}


/*****************************************************************************/


static ItemEntry *GetItemTablePtr(uint32 i)
{
ItemEntry **ipt;
uint32      j;

    ipt = KernelBase->kb_ItemTable;
    if (KernelBase->kb_MaxItem <= i)	return 0;
    j = i/ITEMS_PER_BLOCK;	/* which block */
    return ipt[j];
}


/*****************************************************************************/


static Err ShowMemUsage(ScriptContext *sc, char *args)
{
MemUsageState  mu;
List          *l;
Module        *m;
Node          *n;
Task          *t;
char           description[200];
Range         *r;
PagePool      *pp;
FreeBlock     *fb;
uint32         i;
ItemEntry     *ie;
Item           gen;
ItemNode      *ip;
Item           itm;
int32          oldPri;
uint32         start, end, offset;
Task          *ct;
void          *where;

    TOUCH(sc);

    if (args[0])
        where = (void *)ConvertNum(args);
    else
        where = NULL;

    oldPri = SetItemPri(CURRENTTASKITEM, TASK_MAX_PRIORITY);

    PrepList(&mu.mu_Ranges);
    PrepList(&mu.mu_Pool);
    memset(mu.mu_SizeTotals, 0, sizeof(mu.mu_SizeTotals));
    mu.mu_WhatAddress = where;
    mu.mu_WhatFound   = FALSE;

    /* allocate a pool of range structures */
    for (i = 0; i < 1000; i++)
    {
        r = AllocMem(sizeof(Range), MEMTYPE_NORMAL);
        if (r)
            AddHead(&mu.mu_Pool, (Node *)r);
    }

    /* add all of RAM */
    AddRange(&mu, KernelBase->kb_PagePool->pp_MemRegion->mr_MemBase,
             (uint32)KernelBase->kb_PagePool->pp_MemRegion->mr_MemTop - (uint32)KernelBase->kb_PagePool->pp_MemRegion->mr_MemBase + 1,
             RT_SUPERDYNAMIC, "Supervisor", FALSE);

    /* start with the system free blocks and unused pages */
    pp = KernelBase->kb_PagePool;
    fb = pp->pp_FreeBlocks->fb_Next;
    while (fb)
    {
        AddRange(&mu, fb, fb->fb_Size, RT_FREEBLOCK, "Supervisor", FALSE);

        start  = (uint32)fb;
        end    = (uint32)fb + fb->fb_Size - 1;
        offset = (start & pp->pp_PageMask);

        if (offset)
        {
            /* adjust to a page boundary */
            start += (pp->pp_PageSize - offset);
        }

        if ((start < end )
         && (end - start + 1 >= pp->pp_PageSize))
        {
            AddRange(&mu, (void *)start, (end - start + 1) & (~pp->pp_PageMask),
                     RT_FREEPAGES, NULL, FALSE);
        }

        fb = fb->fb_Next;
    }

    /* now add the free blocks and pages of all the tasks in the system */
    ScanList(&KernelBase->kb_Tasks, n, Node)
    {
        t = Task_Addr(n);
        if (t->t_ThreadTask == NULL)
        {
            pp = t->t_PagePool;

            LockSemaphore(pp->pp_Lock, SEM_WAIT | SEM_SHAREDREAD);

            for (i = 0; i < pp->pp_MemRegion->mr_NumPages; i++)
            {
                if (IsBitSet(pp->pp_OwnedPages, i))
                {
                    AddRange(&mu, (void *)(i * pp->pp_MemRegion->mr_PageSize + (uint32)pp->pp_MemRegion->mr_MemBase),
                             pp->pp_MemRegion->mr_PageSize,
                             RT_USERDYNAMIC, t->t.n_Name, TRUE);
                }
            }

            fb = pp->pp_FreeBlocks->fb_Next;
            while (fb)
            {
                AddRange(&mu, fb, fb->fb_Size, RT_FREEBLOCK, t->t.n_Name, FALSE);
                fb = fb->fb_Next;
            }

            UnlockSemaphore(pp->pp_Lock);
        }
    }

    /* enter module info */
    ct = (CURRENTTASK->t_ThreadTask ? CURRENTTASK->t_ThreadTask : CURRENTTASK);
    l = FindNodeList((Node *)LookupItem(ct->t_Module));
    ScanList(l, m, Module)
    {
        if (m->li->codeLength)
            AddRange(&mu, m->li->codeBase, m->li->codeLength, RT_CODE, m->n.n_Name, FALSE);

        if (m->li->dataLength)
            AddRange(&mu, m->li->dataBase, m->li->dataLength, RT_DATA, m->n.n_Name, FALSE);

        if (m->li->bssLength)
            AddRange(&mu, (void *)((uint32)m->li->dataBase + m->li->dataLength), m->li->bssLength, RT_BSS, m->n.n_Name, FALSE);

        if (m->li->exportsSize)
            AddRange(&mu, m->li->exports, m->li->exportsSize, RT_EXPORTTABLE, m->n.n_Name, FALSE);

        if (m->li->importsSize)
        {
            AddRange(&mu, m->li->imports, m->li->importsSize, RT_IMPORTTABLE, m->n.n_Name, FALSE);
            AddRange(&mu, m->li->importedModules, m->li->imports->numImports * sizeof(Item), RT_IMPORTTABLE, m->n.n_Name, FALSE);
        }

        if (m->li->relocBufferSize)
            AddRange(&mu, m->li->relocBuffer, m->li->relocBufferSize, RT_RELOCTABLE, m->n.n_Name, FALSE);

        if ((m->n.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0)
        {
            /* this module is not privileged, which means its data segment is
             * not packed with other segments. See how much wasted space there
             * is...
             */
            offset = (m->li->dataLength + m->li->bssLength) % KernelBase->kb_PagePool->pp_PageSize;
            if (offset)
            {
                AddRange(&mu, (void *)((uint32)m->li->dataBase + m->li->dataLength + m->li->bssLength),
                         KernelBase->kb_PagePool->pp_PageSize - offset,
                         RT_WASTEDDATA, m->n.n_Name, FALSE);
            }
        }
    }

    /* enter all items */
    for (i = 0; i < KernelBase->kb_MaxItem; i++)
    {
        ie  = GetItemEntryPtr(i);
        gen = (int)(ie->ie_ItemInfo & ITEM_GEN_MASK);
        itm = (Item)(gen+i);

        ip = (ItemNode *)LookupItem(itm);
        if (ip)
        {
            AddRange(&mu, ip, ip->n_Size, RT_ITEM, NULL, TRUE);

            if ((ip->n_Flags & NODE_NAMEVALID) && (ip->n_Name))
            {
            char *n;

                n = ip->n_Name;
                n--;

                /* file system items and a few kernel items don't have
                 * name strings allocated in the standard way, so don't
                 * count these here
                 */
                if (*n != 0)
                {
                    AddRange(&mu, n, (uint32)*n,
                             RT_ITEMNAME, &n[1], TRUE);
                }
            }
        }
    }

    /* enter all item tables */
    i = 0;
    while (TRUE)
    {
        ie = GetItemTablePtr(i);
        if (!ie)
            break;

        AddRange(&mu, ie, ITEMS_PER_BLOCK * sizeof(ItemEntry), RT_ITEMTABLE, NULL, TRUE);
        i++;
    }

    /* enter task stacks and resource tables */
    ScanList(&KernelBase->kb_Tasks, n, Node)
    {
        t = Task_Addr(n);

        if (t->t_StackBase)
            AddRange(&mu, t->t_StackBase, t->t_StackSize,
                     RT_USERSTACK, t->t.n_Name, FALSE);

        if (((t->t_Flags & TASK_SINGLE_STACK) == 0)
          && (t->t_SuperStackBase))
        {
            AddRange(&mu, t->t_SuperStackBase, t->t_SuperStackSize,
                     RT_SUPERSTACK, t->t.n_Name, FALSE);
        }

        if (t->t_ResourceTable)
        {
            AddRange(&mu, t->t_ResourceTable, t->t_ResourceCnt*sizeof(Item),
                     RT_RESOURCETABLE, t->t.n_Name, FALSE);
        }
    }

    /* add known wierd things */
    AddRange(&mu, (void *)VECTORSTART,    VECTORSIZE,    RT_SPECIAL, "Interrupt vectors", FALSE);
    AddRange(&mu, (void *)BOOTDATASTART,  BOOTDATASIZE,  RT_SPECIAL, "Bootcode data", FALSE);
    AddRange(&mu, (void *)DIPIRDATASTART, DIPIRDATASIZE, RT_SPECIAL, "Dipir data", FALSE);
    AddRange(&mu, (void *)DIPIRBUFSTART,  DIPIRBUFSIZE,  RT_SPECIAL, "CD-ROM buffers", FALSE);
#ifdef BUILD_DEBUGGER
    AddRange(&mu, (void *)DEBUGGERSTART,   DEBUGGERSIZE,  RT_SPECIAL, "Debugger buffers", FALSE);
#endif

    SetItemPri(CURRENTTASKITEM, oldPri);

    if (where == NULL)
    {
        /* now dump the range info */
        ScanList(&mu.mu_Ranges, r, Range)
        {
            sprintf(description, formats[r->r_Type], r->r_Description);
            printf("[0x%08x..0x%08x] (%7u bytes) - %s\n", r->r_Start,
                                                          (uint32)r->r_Start + r->r_NumBytes - 1,
                                                          r->r_NumBytes,
                                                          description);
        }
    }
    else
    {
        if (mu.mu_WhatFound)
        {
            r = &mu.mu_WhatRange;
            sprintf(description, formats[r->r_Type], r->r_Description);
            printf("0x%08x is in the range [0x%08x..0x%08x] which is tagged as '%s'\n",
                   mu.mu_WhatAddress,
                   r->r_Start,
                   (uint32)r->r_Start + r->r_NumBytes - 1,
                   description);

            if (r->r_Type == RT_ITEM)
            {
            ItemNode *in;

                in = (ItemNode *)r->r_Start;

                printf("           (item 0x%x, subsys '%s', type %d",
                       in->n_Item,
                       ((Folio *)LookupItem(in->n_SubsysType))->fn.n_Name,
                       in->n_Type);

                if ((in->n_Flags & NODE_NAMEVALID) && (in->n_Name))
                    printf(", name '%s')\n",in->n_Name);
                else
                    printf(")\n");
            }
        }
        else
        {
            printf("ShowMemUsage: 0x%08x is not in a known range of memory\n", where);
        }
    }

    while (r = (Range *)RemHead(&mu.mu_Ranges))
        FreeMem(r, sizeof(Range));

    while (r = (Range *)RemHead(&mu.mu_Pool))
        FreeMem(r, sizeof(Range));

    if (where == NULL)
    {
        /* now dump the size info */
        printf("\nTotals\n");
        for (i = 0; i < RT_ALL; i++)
        {
            printf(sizeFormats[i], mu.mu_SizeTotals[i]);
            printf("\n");
        }
    }

    return 0;
}


/*****************************************************************************/


#ifdef BUILD_LUMBERJACK

static const char * const eventTypes[] =
{
    "User",
    "Tasks",
    "Interrupts",
    "Signals",
    "Messages",
    "Semaphores",
    "Pages",
    "Items",
    "IOReqs",
    NULL
};

static Err Log(ScriptContext *sc, char *args)
{
uint32 bit;
bool   add;
char  *start;
Err    err;

    while (*args)
    {
        while (isspace(*args))
            args++;

        add = TRUE;
        if (*args == '-')
        {
            add = FALSE;
            args++;
        }

        start = args;
        while (!isspace(*args) && *args)
            args++;

        if (*args)
            *args++ = 0;

        bit = 0;
        while (eventTypes[bit])
        {
            if (strcasecmp(eventTypes[bit],start) == 0)
                break;

            bit++;
        }

        if (eventTypes[bit])
        {
            if (add)
                sc->sc_EventsLogged |= (1 << bit);
            else
                sc->sc_EventsLogged &= ~(1 << bit);
        }
        else
        {
            printf("Log: '%s' is not a valid event type\n",start);
            printf("Valid Event Types: ");
            bit = 0;
            while (eventTypes[bit])
            {
                printf("%s ",eventTypes[bit]);
                bit++;
            }
            printf("\n");
            printf("Use -event to turn off logging of an event type\n");

            return BADNAME;
        }
    }

    printf("Events Being Logged: ");
    if (sc->sc_EventsLogged)
    {
        bit = 0;
        while (eventTypes[bit])
        {
            if (sc->sc_EventsLogged & (1 << bit))
                printf("%s ",eventTypes[bit]);

            bit++;
        }
    }
    else
    {
        printf("None");
    }
    printf("\n");

    err = 0;
    if (sc->sc_EventsLogged && !sc->sc_LumberjackCreated)
    {
        err = CreateLumberjack(NULL);
        if (err >= 0)
        {
            sc->sc_LumberjackCreated = TRUE;
        }
        else
        {
            printf("Log: Unable to create Lumberjack");
        }
    }

    ControlLumberjack(sc->sc_EventsLogged);

    return err;
}
#endif


/*****************************************************************************/


#ifdef BUILD_LUMBERJACK
static Err DumpLogs(ScriptContext *sc, char *args)
{
LumberjackBuffer *lb;

    TOUCH(args);

    ControlLumberjack(0);
    while (lb = ObtainLumberjackBuffer())
    {
        DumpLumberjackBuffer(NULL,lb);
        ReleaseLumberjackBuffer(lb);
    }
    ControlLumberjack(sc->sc_EventsLogged);

    if (sc->sc_EventsLogged == 0)
    {
        DeleteLumberjack();
        sc->sc_LumberjackCreated = FALSE;
    }

    return 0;
}
#endif


/*****************************************************************************/


static Err DoExpunge(ScriptContext *sc, char *args)
{
uint32 i;

    TOUCH(sc);
    TOUCH(args);

    /* someone'll have to change this if we ever get 2G of memory... :-) */
    for (i = 0; i < 10; i++)
        FreeMem(AllocMem(2000000000,MEMTYPE_ANY),2000000000);

    return 0;
}


/*****************************************************************************/


const ScriptCommand builtIns[] =
{
    {"alias",                   MakeAlias},
    {"cd,setcd",                SetCD},
    {"bg,setbg",                SetBGMode},
    {"fg,setfg",                SetFGMode},
    {"shellpri,setpri",         SetPri},
    {"pwd,showcd",              ShowCD},
    {"avail,showavailmem",      ShowAvailMem},
    {"memmap,showmemmap",       ShowMemMap},
    {"memusage,showmemusage",   ShowMemUsage},
    {"showfreeblocks",          ShowFreeBlocks},
    {"ps,showtask",             ShowTask},
    {"kill,killtask",           KillTask},
    {"sleep",                   Sleep},
    {"minmem,setminmem",        SetMinMem},
    {"maxmem,setmaxmem",        SetMaxMem},
    {"sendsignal",              DoSendSignal},
    {"help,?",                  Help},
    {"err,showerror",           ShowError},
    {"expunge",                 DoExpunge},
#ifdef BUILD_LUMBERJACK
    {"log",                     Log},
    {"dumplogs",                DumpLogs},
#endif
    {NULL,                      NULL}
};
