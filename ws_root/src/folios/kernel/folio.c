/* @(#) folio.c 96/11/19 1.211 */

#ifdef MEMDEBUG
#undef MEMDEBUG
#endif

#define DBUG(x)	/* printf x */
#define DBUGOF(x)	/* printf x */
#define DBUGCF(x)	/* printf x */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/folio.h>
#include <kernel/task.h>
#include <kernel/msgport.h>
#include <kernel/semaphore.h>
#include <kernel/interrupts.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/timer.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/usermodeservices.h>
#include <kernel/tags.h>
#include <kernel/memlock.h>
#include <kernel/bitarray.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/lumberjack.h>
#include <kernel/random.h>
#include <kernel/uniqueid.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <dipir/dipirpub.h>
#include <hardware/PPCasm.h>
#include <hardware/cde.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memdebug.h"


struct KernelBase  KB;
struct KernelBase *KernelBase = &KB;


extern struct KernelBase *getKernelBase(void);

extern char   *__ctype;

#define NUM_FOLIOS_CACHED 16
static Folio *folioCache[NUM_FOLIOS_CACHED];


/* Currently, only the audio folio and the file folio have per-task data. This
 * leaves room for two more. If need arises, just bump this constant to
 * allow more. Or if you're really fond of data folios, this could be
 * made dynamic too...
 */
#define NUM_DATA_FOLIOS 4
static Folio *PerTaskDataFolio[NUM_DATA_FOLIOS];


void InitFolio(Folio *folio, char *name, int32 size, uint8 pri)
{
	/* initialize standard Node fields */
	folio->fn.n_Type = FOLIONODE;
	folio->fn.n_SubsysType = KERNELNODE;
	folio->fn.n_Priority = pri;
	folio->fn.n_Flags = NODE_ITEMVALID|NODE_NAMEVALID;
	folio->fn.n_Name = name;
	folio->fn.n_Size = size;
	folio->fn.n_ItemFlags |= ITEMNODE_UNIQUE_NAME;

	ADDTAIL(&KB_FIELD(kb_FolioList),(Node *)folio);
}

int CheckDebug(Folio *f, int i)
{
    TOUCH(f);
    TOUCH(i);
    return 0;
}

/*
   Event-report routine.
*/

static int32 (*reportEventHook) (void *event) = NULL;

void RegisterReportEvent(int32 (*newHook) (void *event))
{
  reportEventHook = newHook;
}

int32 internalReportEvent(void *event)
{
  if (reportEventHook) {
    return (*reportEventHook)(event);
  } else {
    return MakeKErr(ER_SEVERE,ER_C_STND,ER_NotSupported);
  }
}

/**
|||	AUTODOC -private -class Kernel -group Miscellaneous -name BeginNoReboot
|||	Requests the system to not reboot on disc eject.
|||
|||	  Synopsis
|||
|||	    void BeginNoReboot(void);
|||
|||	  Description
|||
|||	    Normally the system reboots immediately when the CD drawer
|||	    or door is opened.  This command requests that the system
|||	    not reboot.  This behavior persists until a call is made
|||	    to EndNoReboot().
|||
|||	    Note that BeginNoReboot() and EndNoReboot() are intended to
|||	    be used by ROM applications only; whereas ExpectDataDisc() and
|||	    NoExpectDataDisc() are for title application use.  Therefore,
|||	    only one of these mechanisms should be used (noreboot vs.
|||	    datadisc).
|||
|||	    Each BeginNoReboot() should have a matched EndNoReboot().
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    EndNoReboot(), ExpectDataDisc(), NoExpectDataDisc()
|||
**/

/**
|||	AUTODOC -private -class Kernel -group Miscellaneous -name EndNoReboot
|||	Requests the system to again reboot on disc eject.
|||
|||	  Synopsis
|||
|||	    void EndNoReboot(void);
|||
|||	  Description
|||
|||	    This call reinstates the default behavior of the system
|||	    after a call to BeginNoReboot().
|||
|||	    Each BeginNoReboot() should have a matched EndNoReboot().
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    BeginNoReboot(), ExpectDataDisc(), NoExpectDataDisc()
|||
**/

void IncrNoReboot(int32 incr)
{
    KB_FIELD(kb_NoReboot) += incr;
#ifdef BUILD_PARANOIA
    if (KB_FIELD(kb_NoReboot) < 0)
    {
	printf("NoReboot count %d < 0!\n", KB_FIELD(kb_NoReboot));
	KB_FIELD(kb_NoReboot) = 0;
    }
#endif
    SuperSetSysInfo(SYSINFO_TAG_NOREBOOT, (void *)KB_FIELD(kb_NoReboot), 0);
}

/**
|||	AUTODOC -private -class Kernel -group Miscellaneous -name ExpectDataDisc
|||	Requests the system to not reboot on media insertion.
|||
|||	  Synopsis
|||
|||	    void ExpectDataDisc(void);
|||
|||	  Description
|||
|||	    Normally when bootable media is inserted, the system will
|||	    immediately boot that media.  This call requests that the
|||	    system not boot when bootable media is inserted.
|||
|||	    Note that ExpectDataDisc() implies that you do not want to
|||	    reboot the machine if the CD is ejected.  Therefore,
|||	    ExpectDataDisc() also (automatically) calls BeginNoReboot().
|||	    Likewise, NoExpectDataDisc() automatically calls EndNoReboot().
|||
|||	    Also note that ExpectDataDisc() and NoExpectDataDisc() are
|||	    intended for title application use only; whereas BeginNoReboot()
|||	    and EndNoReboot() are intended to be used by ROM applications.
|||	    Therefore, only one of mechanisms should be used (datadisc vs.
|||	    noreboot).
|||
|||	    Each ExpectDataDisc() should have a matched NoExpectDataDisc().
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    NoExpectDataDisc(), BeginNoReboot(), EndNoReboot()
|||
|||	  WARNING
|||
|||	    Full data disc support may not be available in all
|||	    system components.  This call may not be useful until
|||	    a future release of the system.
|||
**/

/**
|||	AUTODOC -private -class Kernel -group Miscellaneous -name NoExpectDataDisc
|||	Requests the system to again reboot on media insertion.
|||
|||	  Synopsis
|||
|||	    void NoExpectDataDisc(void);
|||
|||	  Description
|||
|||	    This call reinstates the default behavior of the system
|||	    after a call to ExpectDataDisc().
|||
|||	    Each ExpectDataDisc() should have a matched NoExpectDataDisc().
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>
|||
|||	  See Also
|||
|||	    ExpectDataDisc(), BeginNoReboot(), EndNoReboot()
|||
**/

void IncrDataDisc(int32 incr)
{
    /* Indicate that since we are expecting a data disc at some later time,
     * that we also do not want to reboot upon a disc ejection.
     */
    IncrNoReboot(incr);

    KB_FIELD(kb_ExpectDataDisc) += incr;
#ifdef BUILD_PARANOIA
    if (KB_FIELD(kb_ExpectDataDisc) < 0)
    {
	printf("ExpectDataDisc count %d < 0!\n", KB_FIELD(kb_ExpectDataDisc));
	KB_FIELD(kb_ExpectDataDisc) = 0;
    }
#endif

    SuperSetSysInfo(SYSINFO_TAG_DATADISC, (void *)KB_FIELD(kb_ExpectDataDisc), 0);
}

typedef struct FolioInfo
{
	void  *base;
	int32 datasize;
	int32 taskData;
	int32 (*finit)(Folio *);
	void *swv;
	void *sv;
	bool specificItem;
} FolioInfo;

static int32 icf_c(Folio *f, FolioInfo *p, uint32 tag, uint32 arg)
{
    switch(tag)
    {
        case CREATEFOLIO_TAG_BASE        : p->base = (void *)arg;
                                           break;

        case CREATEFOLIO_TAG_DATASIZE    : p->datasize = (int32)arg;
                                           break;

        case CREATEFOLIO_TAG_INIT        : p->finit = Make_Func(int32,arg);
                                           break;

        case CREATEFOLIO_TAG_NODEDATABASE: f->f_NodeDB = (NodeData *)arg;
                                           break;

        case CREATEFOLIO_TAG_MAXNODETYPE : f->f_MaxNodeType = (uint8)arg;
                                           break;

        case CREATEFOLIO_TAG_ITEM        : f->fn.n_Item = (Item)arg;
                                           break;

        case CREATEFOLIO_TAG_DELETEF     : f->f_DeleteFolio = Make_Func(int32,arg);
                                           break;

        case CREATEFOLIO_TAG_NSWIS       : f->f_MaxSwiFunctions = (int32)arg;
                                           break;

        case CREATEFOLIO_TAG_SWIS        : f->f_SwiFunctions = (void *)arg;
                                           break;

        case CREATEFOLIO_TAG_TASKDATA    : p->taskData = (uint32)arg;
                                           break;

        default                          : return BADTAG;
    }

    return 0;
}


/*****************************************************************************/


static Err CreateFolioTaskData(Folio *folio)
{
Node *n;
Err   ret;

    /* call the folio's create task vector for all tasks
     * already in the system.
     */
    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        ret = (*folio->f_FolioCreateTask)(Task_Addr(n),NULL);
        if (ret < 0)
            return ret;
    }

    return 0;
}


/*****************************************************************************/


Item internalCreateFolio(Folio *fdummy, TagArg *tagpt)
{
Folio *folio = 0;
int32 *t;
int32 size;
Item ret;
Folio localfolio;
Folio *f;
void  *allocAddr = NULL;
struct FolioInfo finfo;

    TOUCH(fdummy);

#ifdef BUILD_PARANOIA
	size = 0;
#endif

    DBUG(("internalCreateFolio(%lx,%lx)\n",(uint32)fdummy,(uint32)tagpt));

    if (!IsPriv(CURRENTTASK))
        return BADPRIV;

    /* collect all the info in a local folio header */
    f = &localfolio;

    memset(&finfo, 0, sizeof(finfo));
    finfo.datasize = sizeof(Folio);

    memset(f, 0, sizeof(*f));
    f->fn.n_Type       = FOLIONODE;
    f->fn.n_SubsysType = KERNELNODE;
    f->fn.n_Item       = -1;

    DBUG(("Calling TagProcessor\n"));
    ret = TagProcessor(f,tagpt,icf_c,&finfo);

    if (ret < 0)
        return ret;

#ifdef BUILD_PARANOIA
    if (f->fn.n_Name == NULL)
    {
        ret = BADTAGVAL;
        goto done;
    }

    if (finfo.datasize < sizeof(Folio))
    {
	ret = BADTAGVAL;
	goto done;
    }

    if (f->fn.n_Item < 0)
    {
        ret = BADITEM;
        goto done;
    }
#endif

    size = finfo.datasize;

    if (finfo.base)
    {
        t = finfo.base;
    }
    else
    {
        t = (int32 *)SuperAllocMem(size,MEMTYPE_FILL);
        if (!t)
        {
            ret = NOMEM;
            goto done;
        }
        allocAddr = t;
    }

    if (f->f_NodeDB)
    {
        f->f_ItemRoutines = SuperAllocMem(sizeof(ItemRoutines),MEMTYPE_FILL);
        if (!f->f_ItemRoutines)
        {
            ret = NOMEM;
            goto done;
        }
    }

    /* must be done before calling GetItem */

    folio = (Folio *)t;
    DBUG(("/* transfer the header data */\n"));
    *folio = *f;

    /* now must use the real node */

    DBUG(("item requested=%d\n",folio->fn.n_Item));

    folio->fn.n_Item = AssignItem(folio,folio->fn.n_Item);
    if (folio->fn.n_Item < 0)
    {
        ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemTblOvrFlw);
        goto done;
    }

    if (finfo.taskData)
    {
    uint32 i;

        for (i = 0; i < NUM_DATA_FOLIOS; i++)
        {
            if (KB_FIELD(kb_DataFolios)[i] == NULL)
            {
                folio->f_TaskDataIndex     = i;
                KB_FIELD(kb_DataFolios)[i] = folio;
                break;
            }
        }

        if (i == NUM_DATA_FOLIOS)
        {
            ret = -1;
            goto done;
        }
    }

    InitFolio(folio,folio->fn.n_Name,size,folio->fn.n_Priority);

    DBUG(("finfo.finit = %lx\n",finfo.finit));
    if (finfo.finit)
    {
        DBUG(("calling finit routine\n"));
        ret = (*finfo.finit)(folio);
        DBUG(("after calling finit routine ret=%lx\n",ret));
        if ( ret < 0)
        {
            REMOVENODE((Node *)folio);
            goto done;
        }
    }

#ifdef BUILD_PARANOIA
    if (f->f_ItemRoutines)
    {
        if ((f->f_ItemRoutines->ir_Create && !f->f_ItemRoutines->ir_Delete)
         || (!f->f_ItemRoutines->ir_Create))
        {
            ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_NoFuncPtr);
            REMOVENODE((Node *)folio);
            goto done;
        }
    }
#endif

    if (folio->f_FolioCreateTask)
    {
        ret = CreateFolioTaskData(folio);
        if (ret < 0)
        {
            REMOVENODE((Node *)folio);
            goto done;
        }
    }

    if (NODETOSUBSYS(folio) < NUM_FOLIOS_CACHED)
    {
        folioCache[NODETOSUBSYS(folio)] = folio;
    }
#ifdef BUILD_PARANOIA
    else
    {
        printf("INFO: Folio subsystem number exceeds folio cache capacity. You\n");
        printf("      might want to increase the cache size in folio.c to keep\n");
        printf("      performance higher.\n");
    }
#endif

    if (!allocAddr)
        folio->f_DontFree = TRUE;

    return folio->fn.n_Item;

done:
    DBUG(("done:\n"));

    if (folio && (folio->fn.n_Item >= 0)) FreeItem(folio->fn.n_Item);

    if (KB_FIELD(kb_DataFolios)[f->f_TaskDataIndex] == f)
        KB_FIELD(kb_DataFolios)[f->f_TaskDataIndex] = NULL;

    SuperFreeMem(f->f_ItemRoutines,sizeof(ItemRoutines));
    FreeString(f->fn.n_Name);
    SuperFreeMem(allocAddr,size);

    return ret;
}


/*****************************************************************************/


int32
internalDeleteFolio(Folio *f, Task *t)
{
    int32 ret = 0;
    Node *n;

    TOUCH(t);

    /* Have to force everyone out */
    /* close down folio and release memory */
    DBUG(("DeleteFolio: enter\n"));

    if (KB_FIELD(kb_DataFolios)[f->f_TaskDataIndex] == f)
    {
        KB_FIELD(kb_DataFolios)[f->f_TaskDataIndex] = NULL;

        if (f->f_FolioDeleteTask)
        {
            /* call the folio's delete task vector for all tasks
             * still in the system.
             */
            ScanList(&KB_FIELD(kb_Tasks), n, Node)
            {
                (*f->f_FolioDeleteTask)(Task_Addr(n));
            }
        }
    }

    DeleteSubSysItems(NODETOSUBSYS(f));

    if (f->f_DeleteFolio)
        ret = (*f->f_DeleteFolio)(f);

    if (ret < 0)
    {
        /* what the heck should I do if the following fails? Reset? */
        CreateFolioTaskData(f);
        return ret;
    }

    REMOVENODE((Node *)f);

    SuperFreeMem(f->f_ItemRoutines,sizeof(ItemRoutines));

    if (NODETOSUBSYS(f) < NUM_FOLIOS_CACHED)
        folioCache[NODETOSUBSYS(f)] = NULL;

    if (f->f_DontFree)
    {
        RemoveItem((Task *)LookupItem(f->fn.n_Owner),f->fn.n_Item);
        FreeItem(f->fn.n_Item);
        return 1;
    }

    return 0;
}

/* get the internal kernel routines */

/* call priv task back in super mode */
Err callbacksuper(Err (*code)(uint32, uint32, uint32),uint32 arg1, uint32 arg2, uint32 arg3)
{
    if (!IsPriv(CURRENTTASK))
        return BADPRIV;

    return (*code)(arg1,arg2,arg3);
}

void
illegal(void)
{ }

uint32 NOP(void)
{
    return 0;
}


/* Try to keep this table so that related calls are together in order to
 * help improve cache efficiency. Also, keep creation/initialization calls
 * separate from the primary run-time calls.
 */
static const void *(*KernelSwiVectors[])() =
{
	(void *(*)())internalWait,			/* 0 */
	(void *(*)())externalSignal,			/* 1 */
	(void *(*)())externalLockSemaphore,		/* 2 */
	(void *(*)())externalUnlockSemaphore,		/* 3 */
	(void *(*)())externalSendMsg,			/* 4 */
	(void *(*)())externalReplyMsg,			/* 5 */
	(void *(*)())externalGetMsg,			/* 6 */
	(void *(*)())externalGetThisMsg,		/* 7 */
	(void *(*)())externalWaitPort,			/* 8 */
	(void *(*)())externalAbortIO,			/* 9 */
	(void *(*)())externalSendIO,			/* 10 */
	(void *(*)())externalCompleteIO,		/* 11 */
	(void *(*)())externalDoIO,			/* 12 */
	(void *(*)())externalWaitIO,			/* 13 */
	(void *(*)())internalCreateItem,		/* 14 */
	(void *(*)())internalCreateSizedItem,		/* 15 */
	(void *(*)())externalDeleteItem,		/* 16 */
	(void *(*)())internalFindItem,			/* 17 */
	(void *(*)())externalOpenItem,			/* 18 */
	(void *(*)())externalFindAndOpenItem,		/* 19 */
	(void *(*)())externalCloseItem,			/* 20 */
	(void *(*)())externalSetItemPriority,		/* 21 */
	(void *(*)())externalSetItemOwner,		/* 22 */
	(void *(*)())externalOpenItemAsTask,		/* 23 */
	(void *(*)())externalCloseItemAsTask,		/* 24 */
	(void *(*)())externalControlMem,                /* 25 */
	(void *(*)())externalAllocMemPages,             /* 26 */
	(void *(*)())externalFreeMemPages,		/* 27 */
	(void *(*)())externalAllocSignal,		/* 28 */
	(void *(*)())externalFreeSignal,		/* 29 */
	(void *(*)())callbacksuper,			/* 30 */
	(void *(*)())externalOpenModule,		/* 31 */
	(void *(*)())externalCloseModule,		/* 32 */
	(void *(*)())externalImportByName,		/* 33 */
	(void *(*)())externalUnimportByName,		/* 34 */
	(void *(*)())internalDebugPutChar,		/* 35 */
	(void *(*)())internalDebugPutStr,		/* 36 */
	(void *(*)())printf,				/* 37 */
	(void *(*)())externalMayGetChar,		/* 38 */
	(void *(*)())externalControlUserExceptions,	/* 39 */
	(void *(*)())externalRegisterUserExceptionHandler,	/* 40 */
	(void *(*)())externalCompleteUserException,	/* 41 */
	(void *(*)())internalOpenRomAppMedia,		/* 42 */
	(void *(*)())internalFreeInitModules,		/* 43 */
	(void *(*)())internalRegisterOperator,		/* 44 */
	(void *(*)())IncrNoReboot,			/* 45 */
	(void *(*)())IncrDataDisc,			/* 46 */
	(void *(*)())internalReadHardwareRandomNumber,	/* 47 */
	(void *(*)())NOP,				/* 48 */
	(void *(*)())externalSetExitStatus,		/* 49 */
	(void *(*)())internalYield,			/* 50 */
	(void *(*)())internalInvalidateFPState,		/* 51 */
	(void *(*)())internalGetPersistentMem,		/* 52 */
	(void *(*)())internalPrint3DOHeader,		/* 53 */
        (void *(*)())externalControlCaches,		/* 54 */

#ifdef BUILD_MEMDEBUG
	(void *(*)())externalCreateMemDebug,		/* 55 */
	(void *(*)())externalDeleteMemDebug,		/* 56 */
	(void *(*)())externalControlMemDebug,		/* 57 */
	(void *(*)())internalAddAllocation,		/* 58 */
	(void *(*)())internalRemAllocation,		/* 59 */
	(void *(*)())externalAllocMemPagesDebug,	/* 60 */
	(void *(*)())externalFreeMemPagesDebug,		/* 61 */
	(void *(*)())externalRationMemDebug,		/* 62 */
	(void *(*)())internalDoMemRation,		/* 63 */
#else
	(void *(*)())NOP,				/* 55 */
	(void *(*)())NOP,				/* 56 */
	(void *(*)())NOP,				/* 57 */
	(void *(*)())NULL,				/* 58 */
	(void *(*)())NULL,				/* 59 */
	(void *(*)())externalAllocMemPages,		/* 60 */
	(void *(*)())externalFreeMemPages,		/* 61 */
	(void *(*)())NOP,				/* 62 */
	(void *(*)())NULL,				/* 63 */
#endif

#ifdef BUILD_LUMBERJACK
	(void *(*)())internalCreateLumberjack,		/* 64 */
	(void *(*)())internalDeleteLumberjack,		/* 65 */
	(void *(*)())internalLogEvent,			/* 66 */
	(void *(*)())internalObtainLumberjackBuffer, 	/* 67 */
	(void *(*)())internalReleaseLumberjackBuffer,	/* 68 */
	(void *(*)())internalWaitLumberjackBuffer,	/* 69 */
	(void *(*)())internalControlLumberjack,		/* 70 */
#else
        NULL,						/* 64 */
        NULL,						/* 65 */
        NULL,						/* 66 */
        NULL,						/* 67 */
        NULL,						/* 68 */
        NULL,						/* 69 */
        NULL,						/* 70 */
#endif
	(void *(*)())externalIncreaseResourceTable,	/* 71 */
};

#define SwiFuncCount (sizeof(KernelSwiVectors)/sizeof(KernelSwiVectors[0]))

/* Table of Node or FirqList ptrs */
static Node *InterruptHandlers[INTR_MAX];


static struct NodeData NodeDB[] =
{
	{ 0, 0 },	/* no node here */
	{ sizeof(NamelessNode), 0 },
	{ sizeof(FirqList), NODE_NAMEVALID },
	{ sizeof(MemRegion), NODE_NAMEVALID },
	{ 0, NODE_NAMEVALID },	/* Folio */
	{ sizeof(Task), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(FirqNode), NODE_NAMEVALID|NODE_ITEMVALID },
	{ sizeof(Semaphore), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(SemaphoreWaitNode), NODE_SIZELOCKED },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED }, /* Msg */
	{ sizeof(MsgPort), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(PagePool), 0 },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID }, /* RomTag */
	{ sizeof(Driver), NODE_NAMEVALID|NODE_ITEMVALID|NODE_OPENVALID|NODE_SIZELOCKED },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },	/* IOReq */
	{ sizeof(Device), NODE_NAMEVALID|NODE_ITEMVALID|NODE_OPENVALID },
	{ sizeof(Timer), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(ErrorText), NODE_NAMEVALID|NODE_ITEMVALID },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },	/* MEMLOCKNODE */
	{ sizeof(Module), NODE_NAMEVALID | NODE_ITEMVALID | NODE_OPENVALID | NODE_SIZELOCKED }
};

#define NODECOUNT (sizeof(NodeDB)/sizeof(NodeData))


ItemRoutines kb_ItemRoutines =
{
	internalFindKernelItem,			/* ir_Find	  */
	internalCreateKernelItem,		/* ir_Create	  */
	internalDeleteKernelItem,		/* ir_Delete	  */
	internalOpenKernelItem,			/* ir_Open	  */
	internalCloseKernelItem,		/* ir_Close	  */
	internalSetPriorityKernelItem,		/* ir_SetPriority */
	internalSetOwnerKernelItem,		/* ir_SetOwner	  */
	internalLoadKernelItem                  /* ir_Load        */
};


static const char kernelName[] = "\007Kernel";


void InitKernelBase(uint32 (*QueryROMSysInfo)()) {

	_3DOBinHeader	*Kernel3DOHeader;
	SystemInfo si;
	BootInfo *bi;

	KB_FIELD(kb_QueryROMSysInfo) = QueryROMSysInfo;
	QUERY_SYS_INFO(SYSINFO_TAG_PUTC, KB_FIELD(kb_PutC));

	QUERY_SYS_INFO(SYSINFO_TAG_SYSTEM, si);
	if (si.si_SysFlags & SYSINFO_SYSF_SERIAL_OK)
		KB_FIELD(kb_CPUFlags) |= KB_SERIALPORT;

	if (si.si_SysFlags & SYSINFO_SYSF_DUAL_CPU) KB_FIELD(kb_NumCPUs) = 2;
	else KB_FIELD(kb_NumCPUs) = 1;

	DBUG(("Entering InitKernelBase\n"));
	DBUG(("  QueryROMSysInfo at: %lx\n", KB_FIELD(kb_QueryROMSysInfo)));

	_mtsprg0((uint32)&KB);
	QUERY_SYS_INFO(SYSINFO_TAG_PERFORMSOFTRESET, KB_FIELD(kb_PerformSoftReset));
	QUERY_SYS_INFO(SYSINFO_TAG_MAYGETCHAR, KB_FIELD(kb_MayGetChar));
	QUERY_SYS_INFO(SYSINFO_TAG_SETROMSYSINFO, KB_FIELD(kb_SetROMSysInfo));
	QUERY_SYS_INFO(SYSINFO_TAG_ADDBOOTALLOC, KB_FIELD(kb_AddBootAlloc));
	QUERY_SYS_INFO(SYSINFO_TAG_DELETEBOOTALLOC, KB_FIELD(kb_DeleteBootAlloc));
	QUERY_SYS_INFO(SYSINFO_TAG_VERIFYBOOTALLOC, KB_FIELD(kb_VerifyBootAlloc));
	QUERY_SYS_INFO(SYSINFO_TAG_DIPIRROUTINES, KB_FIELD(kb_DipirRoutines));
	QUERY_SYS_INFO(SYSINFO_TAG_KERNELADDRESS, bi);
	QUERY_SYS_INFO(SYSINFO_TAG_APPVOLUMELABEL, KB_FIELD(kb_AppVolumeLabel));

        KB_FIELD(kb_OSComponents) = &bi->bi_ComponentList;
        KB_FIELD(kb_KernelModule) = bi->bi_KernelModule;

	SuperSetSysInfo(SYSINFO_TAG_KERNELBASE, &KernelBase, sizeof(KernelBase));

	{
	    PowerBusDevice pb;

	    pb.pb_DeviceID = CDE_DEVICE_ID_TYPE;
	    QUERY_SYS_INFO(SYSINFO_TAG_PBUSDEV, pb);
	    KB_FIELD(kb_CDEBase) = pb.pb_Address;
	}

	{
	    SysCacheInfo sci;

	    QUERY_SYS_INFO(SYSINFO_TAG_CACHE, sci);
	    KB_FIELD(kb_DCacheSize) = sci.sci_PDCacheSize;
	    KB_FIELD(kb_DCacheBlockSize) = sci.sci_PDCacheLineSize;
	    KB_FIELD(kb_DCacheNumBlocks) = sci.sci_PDCacheSize / sci.sci_PDCacheLineSize;

            /* use the kernel code as the flush data */
	    KB_FIELD(kb_DCacheFlushData) = ((Module *)KB_FIELD(kb_KernelModule))->li->codeBase;
	}

	if (KB_FIELD(kb_AppVolumeLabel) == 0) {
		memset (KB_FIELD(kb_AppVolumeName), 0, 32);
		strcpy (KB_FIELD(kb_AppVolumeName), "rom");
	} else memcpy (KB_FIELD(kb_AppVolumeName), KB_FIELD(kb_AppVolumeLabel)->dl_VolumeIdentifier, 32);

	DBUG(("App Volume: %s\n\n\n", KB_FIELD(kb_AppVolumeName)));

	CopyToConst(&KB_FIELD(kb_CurrentTaskItem),(void *)-1);

	DBUG(("Init some lists\n"));

	PrepList(&KB_FIELD(kb_Tasks));
	PrepList(&KB_FIELD(kb_FolioList));
	PrepList(&KB_FIELD(kb_Drivers));
	PrepList((List*)&KB_FIELD(kb_DDFs));
	PrepList(&KB_FIELD(kb_MsgPorts));
	PrepList(&KB_FIELD(kb_Semaphores));
	PrepList(&KB_FIELD(kb_Errors));
	PrepList(&KB_FIELD(kb_MemLockHandlers));
	PrepList(&KB_FIELD(kb_DeadTasks));
	PrepList(&KB_FIELD(kb_TaskReadyQ));
	PrepList((List*)&KB_FIELD(kb_DuckFuncs));
	PrepList((List*)&KB_FIELD(kb_RecoverFuncs));
	PrepList(&KB_FIELD(kb_Modules));

	/* In case it needs to be increased in the future by some hack */
	KB_FIELD(kb_MaxInterrupts) = INTR_MAX;
	KB_FIELD(kb_InterruptHandlers) = InterruptHandlers;

	DBUG(("Calling InitFolio()\n"));
	InitFolio((Folio*)KernelBase, &kernelName[1], (int32)sizeof(struct KernelBase),254);

	KB_FIELD(kb.f_MaxSwiFunctions)   = SwiFuncCount;
	KB_FIELD(kb.f_SwiFunctions)      = (FolioFunc*)KernelSwiVectors;

	KB_FIELD(kb.f_MaxNodeType)       = NODECOUNT;
	KB_FIELD(kb.f_NodeDB)            = NodeDB;

	folioCache[KERNELNODE] = (Folio *)KernelBase;

	KB_FIELD(kb_FolioTaskDataCnt)  = NUM_DATA_FOLIOS;
	KB_FIELD(kb_FolioTaskDataSize) = NUM_DATA_FOLIOS * sizeof(uint32);

	Kernel3DOHeader = ((Module *)KB_FIELD(kb_KernelModule))->li->header3DO;
	KB_FIELD(kb.f_ItemRoutines)   = &kb_ItemRoutines;
	KB_FIELD(kb_FPOwner)          = NULL;
	KB_FIELD(kb_PleaseReschedule) = 0;
	KB_FIELD(kb_DataFolios)       = PerTaskDataFolio;
	KB_FIELD(kb.fn.n_Version)     = Kernel3DOHeader->_3DO_Item.n_Version;
	KB_FIELD(kb.fn.n_Revision)    = Kernel3DOHeader->_3DO_Item.n_Revision;
	/*KB_FIELD(kb_ShowTaskMessages) = 1;*/

	DBUG(("InitKernelBase now returning\n"));
}


/*****************************************************************************/


Folio *WhichFolio(int32 subsys)
{
Folio *f;

    /* first look in the cache */
    if (subsys < NUM_FOLIOS_CACHED)
    {
        f = folioCache[subsys];
        if (f)
            return f;
    }

    /* if it's not in the cache, use the long route... */
    ScanList(&KB_FIELD(kb_FolioList),f,Folio)
    {
        if (NODETOSUBSYS(f) == subsys)
            return f;
    }

    return NULL;
}


/*****************************************************************************/


void *GetSysCallFunction(uint32 sc_number)
{
Folio  *f;
uint32  cn;

    f = WhichFolio(sc_number >> 16);
    if (f)
    {
	cn = (sc_number & 0xffff);
	if (cn < f->f_MaxSwiFunctions)
            return f->f_SwiFunctions[cn];
    }

#ifdef BUILD_STRINGS
    printf("SYSCALL: 0x%08x is not a valid system call number\n", sc_number);
    printf("         killing task %s\n", CURRENTTASK->t.n_Name);
#endif

    externalDeleteItem(CURRENTTASKITEM);

    /* we never get here, but this keeps the compiler happy */
    return NULL;
}
