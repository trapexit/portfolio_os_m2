/* @(#) taskdump.c 96/07/31 1.14 */

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <hardware/PPCasm.h>
#include <stdio.h>


/*****************************************************************************/


#ifdef BUILD_STRINGS
static const char *MSRNames[] =
{
    "LE",
    "RI",
    NULL,
    NULL,
    "DR",
    "IR",
    "IP",
    NULL,
    "FE1",
    "BE",
    "SE",
    "FE0",
    "ME",
    "FP",
    "PR",
    "EE",
    "ILE",
    "TGPR",
    "POW",
    NULL,
    NULL,
    NULL,
    "SA"
    "AP"
};

static const char *HIDNames[] =
{
    "WIMG_GUARD",
    "WIMG_MCOHR",
    "WIMG_INHIB",
    "WIMG_WRTHU",
    NULL,
    "SL",
    NULL,
    "XMODE",
    NULL,
    NULL,
    "DCI",
    "ICFI",
    "DLOCK",
    "ILOCK",
    "DCE",
    "ICE",
    "NHR",
    NULL,
    NULL,
    NULL,
    "DPM",
    "SLEEP",
    "NAP",
    "DOZE"
};

static const char *TaskNodeFlagNames[] =
{
    "READY",
    "WAITING",
    "RUNNING",
    "QUANTUM_EXPIRED"
};

static const char *TaskItemFlagNames[] =
{
    NULL,
    NULL,
    NULL,
    NULL,
    "DELETED",
    "UNIQUE_NAME",
    "PRIVILEGED",
    "NOTREADY"
};

static const char *TaskFlagNames[] =
{
    "DATADISCOK",
    "FREE_STACK",
    "EXITING",
    "SINGLE_STACK",
    "SUPERVISOR_ONLY",
    "PCMCIA_PERM",
    "PREEMPTED",
    "WAITING_SUPERLOCK"
};


static void DumpFlags(uint32 flags, const char **names, uint32 numNames)
{
uint32 i;

    printf("( ");
    for (i = 0; i < numNames; i++)
    {
        if ((1 << i) & flags)
        {
            if (names[i])
                printf("%s ", names[i]);
        }
    }
    printf(")");
}

#else

#define DumpFlags(f, n, nn)

#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
void DumpTaskList(void)
{
Task    *task;
MinNode *n;

    ScanList(&KB_FIELD(kb_Tasks),n,MinNode)
    {
        task = Task_Addr(n);

        printf(" $%08x $%08x", task->t_WaitBits,task->t_SigBits);
        printf(" %3d", task->t.n_Priority);
        printf(" %6d", task->t_NumTaskLaunch);
        printf(" %c %c",
               task->t.n_ItemFlags & ITEMNODE_PRIVILEGED ? 'P' : 'U',
               task->t.n_Flags & TASK_RUNNING ? 'X' :
               task->t.n_Flags & TASK_READY ? 'R' :
               task->t.n_Flags & TASK_WAITING ? 'W' : ' ');
        printf(" %s",task->t.n_Name);
        printf("\n");
    }
}
#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
void DumpRegBlock(const char *banner, const RegBlock *rb)
{
uint32 i,j,k;

    if (banner)
        printf("%s\n", banner);

    for (i = 0; i < 8; i++)
    {
        printf("  ");
        for (j = 0; j < 4; j++)
        {
            k = i + (j*8);
            printf("GPR[%2d] %08lx  ",k,rb->rb_GPRs[k]);
        }
        printf("\n");
    }

    printf("  PC %08x, LR %08x, CTR %08x, CR %08x, XER %08x\n",
            rb->rb_PC,
            rb->rb_LR,
            rb->rb_CTR,
            rb->rb_CR,
            rb->rb_XER);

    printf("  MSR %08x ", rb->rb_MSR);
    DumpFlags(rb->rb_MSR, MSRNames, sizeof(MSRNames) / sizeof(MSRNames[0]));
    printf("\n");
}
#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
void DumpTaskRegs(const char *banner, const Task *t)
{
    DumpRegBlock(banner, &t->t_RegisterSave);
}
#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
static void DumpBAT(uint32 batu, uint32 batl)
{
uint32  memSize;
char   *size;

    printf("0x%08x, 0x%08x ( ", batu, batl);

    memSize = batu & 0x00001ffc;
    size    = NULL;
    switch (memSize)
    {
        case UBAT_BL_256M: size = "256M"; break;
        case UBAT_BL_128M: size = "128M"; break;
        case UBAT_BL_64M : size	= "64M";  break;
        case UBAT_BL_32M : size = "32M";  break;
        case UBAT_BL_16M : size = "16M";  break;
        case UBAT_BL_8M	 : size = "8M";   break;
        case UBAT_BL_4M	 : size = "4M";   break;
        case UBAT_BL_2M	 : size = "2M";   break;
        case UBAT_BL_1M	 : size = "1M";   break;
        case UBAT_BL_512K: size = "512K"; break;
        case UBAT_BL_256K: size = "256K"; break;
        case UBAT_BL_128K: size = "128K"; break;
    }
    printf("%s ", size);

    if (batu & UBAT_VS)
        printf("VS ");

    if (batu & UBAT_VP)
        printf("VP ");

    if (batl & LBAT_WIMG_WRTHU)
        printf("WRTHU ");

    if (batl & LBAT_WIMG_INHIB)
        printf("INHIB ");

    if (batl & LBAT_WIMG_MCOHR)
        printf("MCOHR ");

    if (batl & LBAT_WIMG_GUARD)
        printf("GUARD ");

    if (batl & LBAT_PP_READWR)
        printf("READWR ");

    if (batl & LBAT_PP_READ)
        printf("READ ");

    if ((batl & (LBAT_PP_READ | LBAT_PP_READWR)) == 0)
        printf("NOACC ");

    printf(")\n");
}
#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
void DumpSysRegs(const char *banner)
{
uint32 i,hid;
uint32 bats[16];

    if (banner)
        printf("%s\n", banner);

    printf("  SER %08x, SEBR %08x, DMISS %08x\n",
           _mfser(),
           _mfsebr(),
           _mfdmiss());

    printf("  DSISR %08x, DAR %08x, IBR %08x\n",
           _mfdsisr(),
           _mfdar(),
           _mfibr());

    hid = _mfhid0();
    printf("  HID0 %08x ", hid);
    DumpFlags(hid, HIDNames, sizeof(HIDNames) / sizeof(HIDNames[0]));
    printf("\n");

    GetIBATs(bats);
    for (i = 0; i < 4; i++)
    {
        printf("  IBAT%d ", i);
        DumpBAT(bats[i*2], bats[i*2+1]);
    }

    GetDBATs(bats);
    for (i = 0; i < 4; i++)
    {
        printf("  DBAT%d ", i);
        DumpBAT(bats[i*2], bats[i*2+1]);
    }

    GetSegRegs(bats);
    printf("  Sgmnt Regs: ");
    for (i = 0; i < 8; i++)
        printf("%08x ",bats[i]);
    printf("\n              ");
    for (i = 8; i < 16; i++)
        printf("%08x ",bats[i]);

    printf("\n");
}
#endif


/*****************************************************************************/


#ifdef BUILD_DEBUGGER
void DumpTask(const char *banner, const Task *task)
{
uint32	 s;
uint32	 u;
Task    *p;
Item     m;
Module  *module;

    if (banner)
        printf("%s\n", banner);

    {
    TimeVal tv;

        ConvertTimerTicksToTimeVal((TimerTicks *)&task->t_ElapsedTime, &tv);
        s = tv.tv_Seconds;
        u = tv.tv_Microseconds;
    }

    p = task->t_ThreadTask;

    printf("  Name            : %s\n",task->t.n_Name);
    printf("  Item            : $%05x\n",task->t.n_Item);
    printf("  Address         : $%08x\n",task);
    printf("  Priority        : %d\n",task->t.n_Priority);

    printf("  n_Flags         : $%02x ", task->t.n_Flags);     DumpFlags(task->t.n_Flags, TaskNodeFlagNames, sizeof(TaskNodeFlagNames) / sizeof(TaskNodeFlagNames[0]));      printf("\n");
    printf("  n_ItemFlags     : $%02x ", task->t.n_ItemFlags); DumpFlags(task->t.n_ItemFlags, TaskItemFlagNames, sizeof(TaskItemFlagNames) / sizeof(TaskItemFlagNames[0])); printf("\n");
    printf("  t_Flags         : $%08x ", task->t_Flags);       DumpFlags(task->t_Flags, TaskFlagNames, sizeof(TaskFlagNames) / sizeof(TaskFlagNames[0]));                    printf("\n");

    if (p)
        printf("  Thread Of Task  : %s (item $%05x)\n",p->t.n_Name,p->t.n_Item);

    printf("  Time Quantum    : %dms\n",task->t_MaxUSecs / 1000);
    printf("  Launch Count    : %d\n",task->t_NumTaskLaunch);
    printf("  Accumulated Time: %d:%02d:%02d.%03d\n", s / 3600, (s / 60) % 60, s % 60, u / 1000);

    printf("  Sigs Allocated  : $%08x\n",task->t_AllocatedSigs);
    printf("  Sigs Received   : $%08x\n",task->t_SigBits);
    printf("  Sigs Wait       : $%08x\n",task->t_WaitBits);
    printf("  Item Wait       : $%08x\n",task->t_WaitItem);

    printf("  Stack Base      : $%08x\n",task->t_StackBase);
    printf("  Stack Size      : %d\n",task->t_StackSize);
    printf("  Super Stack Base: $%08x\n",task->t_SuperStackBase);
    printf("  Super Stack Size: %d\n",task->t_SuperStackSize);

    printf("  Writable Pages  : $%08x\n", task->t_PagePool->pp_WritablePages);
    printf("  Owned Pages     : $%08x\n", task->t_PagePool->pp_OwnedPages);

    if (p)
        m = p->t_Module;
    else
        m = task->t_Module;

    module = (Module *)LookupItem(m);
    if (module)
    {
        printf("  Code Area       : $%08x-$%08x\n", module->li->codeBase, (uint32)module->li->codeBase + module->li->codeLength - 1);
        printf("  Data Area       : $%08x-$%08x\n", module->li->dataBase, (uint32)module->li->dataBase + module->li->dataLength + module->li->bssLength - 1);
    }
}
#endif
