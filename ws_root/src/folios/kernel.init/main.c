/* @(#) main.c 96/09/30 1.22 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/interrupts.h>
#include <kernel/panic.h>
#include <kernel/memlock.h>
#include <kernel/time.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <hardware/PPCasm.h>
#include <stdio.h>


/*****************************************************************************/


#define	DBUG(x) /*printf x*/

extern void PrintSysConfig(void);
extern void GetUniqueID(void);
extern void ErrorInit(void);
extern void start_timers(void);
extern void start_firq(void);
extern Err initDevSem(void);
extern Err initDDFSem(void);
extern Err initDuckAndRecoverSems(void);
extern void InitMonitor(void);
extern void InitMP(void);
extern void SetupExceptionHandlers(void);
extern void InitESARegion(void);


/*****************************************************************************/


static void StartOperator(void)
{
ItemNode *n;
Task     *task;

    n = (ItemNode *)FindNamedNode(&KB_FIELD(kb_Modules), "Operator");

    DBUG(("Operator module 0x%x\n", n));

    if (task = SuperAllocNode((Folio *) KernelBase, TASKNODE))
    {
        DBUG(("\n ... ABOUT TO LAUNCH OPERATOR!\n"));

        SuperInternalCreateTaskVA(task, CREATETASK_TAG_PRIVILEGED,   TRUE,
                                        CREATETASK_TAG_SINGLE_STACK, TRUE,
                                        CREATETASK_TAG_MODULE,       n->n_Item,
                                        TAG_END);
    }

    PANIC(ER_Kr_NoOperator);
}


/*****************************************************************************/


static void InitDipirBuffer(void)
{
    BootAlloc *ba;
    SysInfoRange range;
    List *list;

    /* Free any existing DipirPrivateBuffer */
    QUERY_SYS_INFO(SYSINFO_TAG_BOOTALLOCLIST, list);
    ScanList(list, ba, BootAlloc)
    {
	if (ba->ba_Flags & BA_DIPIR_PRIVATE)
	    SuperFreeRawMem(ba->ba_Start, ba->ba_Size);
    }

    /* Allocate a new DipirPrivateBuffer and tell bootcode where it is. */
    range.sir_Len = 32*1024;
    range.sir_Addr = SuperAllocMem(range.sir_Len, MEMTYPE_NORMAL);
    SuperSetSysInfo(SYSINFO_TAG_DIPIRPRIVATEBUF, &range , sizeof(range));
}


/*****************************************************************************/


void _InitKernelModule(void)
{
    DBUG(("Calling InitESARegion\n"));
    InitESARegion();

    DBUG(("Calling InitMP\n"));
    InitMP();

    DBUG(("Patching exception vectors, IBR=%x\n", _mfibr()));
    SetupExceptionHandlers();

    DBUG(("Calling ErrorInit\n"));
    ErrorInit();

    DBUG(("Calling InitMemLock\n"));
    InitMemLock();

    DBUG(("Calling initDevSem\n"));
    initDevSem();

    DBUG(("Calling initDDFSem\n"));
    initDDFSem();

    DBUG(("Calling initDuckAndRecoverSems\n"));
    initDuckAndRecoverSems();

    DBUG(("Allocating DIPIR buffer\n"));
    InitDipirBuffer();

    DBUG(("Calling start_timers\n"));
    start_timers ();

    DBUG(("Calling start_firq\n"));
    start_firq ();

#ifdef BUILD_DEBUGGER
    DBUG(("Calling InitMonitor\n"));
    InitMonitor();
#endif

    DBUG(("Calling GetUniqueID\n"));
    GetUniqueID();

    DBUG(("Setting up master MMU schtuff\n"));
    SetupMMU();

    DBUG(("Setting up slave schtuff\n"));
    FlushDCacheAll(0);
    SuperSlaveRequest(SLAVE_INITMMU, 0, 0, 0);
    SuperSlaveRequest(SLAVE_INITVERSION, 0, 0, 0);
    SuperSlaveRequest(SLAVE_CONTROLICACHE, 1, 0, 0);
    SuperSlaveRequest(SLAVE_CONTROLDCACHE, 1, 0, 0);

#ifdef BUILD_STRINGS
    PrintSysConfig();
#endif

    StartOperator();
}


/*****************************************************************************/


void main(void)
{
}
