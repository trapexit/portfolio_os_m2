#ifndef __KERNEL_KERNEL_H
#define __KERNEL_KERNEL_H


/******************************************************************************
**
**  @(#) kernel.h 96/11/13 1.64
**
**  Kernel folio structure definition
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_PROTLIST_H
#include <kernel/protlist.h>
#endif

#ifndef __KERNEL_FOLIO_H
#include <kernel/folio.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif

#ifndef __SETJMP_H
#include <setjmp.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef __FILE_DISCDATA_H
#include <file/discdata.h>
#endif


#define rb_SP rb_GPRs[1]


/*****************************************************************************/


/* written by the slave for its global state */
typedef struct
{
    RegBlock   ss_RegSave;
    FPRegBlock ss_FPRegSave;
    uint8      ss_SuperStack[2048];
    uint8     *ss_SuperSP;
    uint32    *ss_WritablePages;
    uint32     ss_NumPages;
    uint32     ss_SlaveSER;
    uint32     ss_SlaveSEBR;
    uint32     ss_SlaveMSR;
    uint32     ss_SlaveHID0;
    uint32     ss_SlaveVersion;
} SlaveState;

/* stuff the master asks the slave to do */
typedef enum
{
    SLAVE_NOP,              /* args are unused                                */
    SLAVE_DISPATCH,         /* arg1 is code ptr, arg2 is userData, arg3 is sp */
    SLAVE_ABORT,            /* args are unused                                */

    SLAVE_CONTROLICACHE,    /* arg1 is boolean saying to turn on/off          */
    SLAVE_INVALIDATEICACHE, /* args are unused                                */
    SLAVE_CONTROLDCACHE,    /* arg1 is boolean saying to turn on/off          */

    SLAVE_INITMMU,          /* args are unused                                */
    SLAVE_UPDATEMMU,        /* arg1 is ptr to fence bits                      */
    SLAVE_INITVERSION,      /* args are unused                                */

    SLAVE_SRESETPREP,       /* arg1 is selects DEC refresh or sreset prep     */
    SLAVE_SRESETRECOVER,    /* args are unused                                */

    SLAVE_SUSPEND,
    SLAVE_CONTINUE
} SlaveActions;

/* stuff the slave asks the master to do */
typedef enum
{
    MASTER_NOP,
    MASTER_SLAVECOMPLETE,
    MASTER_PUTSTR,           /* arg1 is string ptr              */
    MASTER_MONITOR           /* arg1 is crash cause, arg2 is pc */
} MasterActions;

/* written by one cpu to ask the other to do work */
typedef struct
{
    volatile uint32 car_Action;
    volatile uint32 car_Arg1;
    volatile uint32 car_Arg2;
    volatile uint32 car_Arg3;
} CPUActionReq;


/*****************************************************************************/


struct KernelBase
{
    Folio        kb;                  /* system linkage                 */
    uint32       kb_Flags;            /* global flags                   */
    List         kb_Tasks;            /* list of all tasks              */

    Task * const kb_CurrentTask;      /* currently executing task       */
    Item   const kb_CurrentTaskItem;  /* Item of current task           */
    uint32       kb_NumTaskSwitches;  /* total # of switch since bootup */
    List         kb_TaskReadyQ;

    /* NOTE: For best performance, next 3 fields should be in one cache line */
    uint32      *kb_WriteFencePtr;
    uint32       kb_RAMBaseAddress;     /* Logical address for RAM Base */
    uint32       kb_RAMEndAddress;      /* Logical address for end of RAM */
    Task        *kb_CurrentFenceTask;

    List         kb_FolioList;
    List         kb_Drivers;
    Item	 kb_DevSemaphore;	/* Device list lock	*/
    ProtectedList kb_DDFs;
    List         kb_MsgPorts;
    List         kb_Semaphores;
    List         kb_Errors;
    Item	 kb_ErrorSemaphore;	/* Error list lock	*/
    List         kb_MemLockHandlers;
    Item         kb_MemLockSemaphore;	/* MemLock list lock	*/
    List         kb_DeadTasks;
    ProtectedList kb_DuckFuncs;
    ProtectedList kb_RecoverFuncs;

    struct MemRegion *kb_MemRegion;
    struct PagePool  *kb_PagePool;

    Folio      **kb_DataFolios;
    uint8        kb_FolioTaskDataCnt;    /* words */
    uint8        kb_FolioTaskDataSize;   /* words */

    bool         kb_PleaseReschedule;
    uint8        kb_MaxInterrupts;
    Node       **kb_InterruptHandlers;
    uint32      *kb_InterruptStack;     /* ptr to interrupt stack */

    ItemEntry  **kb_ItemTable;
    int32        kb_MaxItem;

    Task        *kb_FPOwner;            /* Task currently using FPU */

    DiscLabel	*kb_AppVolumeLabel;
    char	 kb_AppVolumeName[32];

    Item         kb_OperatorTask;
    uint32       kb_UniqueID[2];	/* 64 bit unique id from dallas chip */
    uint32       kb_BusClk;		/* bus clock speed in Hz */
    bool	 kb_ShowTaskMessages;	/* debug aid */
    bool	 kb_pad0[3];		/* space for more flags */

    uint32	(*kb_QueryROMSysInfo)(unsigned long, void *, unsigned long);
    uint32	(*kb_SetROMSysInfo)(unsigned long, void *, unsigned long);
    uint32	(*kb_PerformSoftReset)(void);
    void        (*kb_PutC)(char);
    int         (*kb_MayGetChar)(void);
    void	*kb_DipirRoutines;

    void        *kb_KernelModule;      /* The primitive OS components */
    uint32	kb_CDEBase;		/* Base address of CDE */
    int32	kb_NoReboot;		/* Don't reboot on CD drawer open */
    int32	kb_ExpectDataDisc;	/* Don't boot newly inserted title */
    uint32	kb_DCacheSize;		/* Data cache total size */
    uint32	kb_DCacheBlockSize;	/* Data cache block (line) size */
    uint32	kb_DCacheNumBlocks;
    void       *kb_DCacheFlushData;	/* data to read to flush the cache */

    void       *kb_PersistentMemStart;
    uint32      kb_PersistentMemSize;

    uint32      kb_ROMBaseAddress;     /* Logical address for ROM Base */
    uint32      kb_ROMEndAddress;      /* Logical address for end of ROM */

    List        kb_Modules;
    List       *kb_OSComponents;	        /* stuff pulled in by dipir that should go through the loader */

    uint32         kb_NumCPUs;
    CPUActionReq  *kb_MasterReq;
    CPUActionReq  *kb_SlaveReq;
    SlaveState    *kb_SlaveState;
    Item           kb_CurrentSlaveTask;
    struct IOReq  *kb_CurrentSlaveIO;

    void	(*kb_AddBootAlloc)(void *, uint32, uint32);
    void	(*kb_DeleteBootAlloc)(void *, uint32, uint32);
    void	(*kb_VerifyBootAlloc)(void *, uint32, uint32);
};


extern struct KernelBase * const KernelBase;

extern struct KernelBase KB;
#ifdef KERNEL
#define KB_FIELD(x)	KB.x
#define CURRENTTASK	KB_FIELD(kb_CurrentTask)
#define CURRENTTASKITEM KB_FIELD(kb_CurrentTaskItem)
#else
#define KB_FIELD(x)	KernelBase->x
#endif

/* kb_Flags */
#define KB_NODBGR	0x00000001  /* debugger is not present      */
#define KB_LOGGING      0x00000002  /* lumberjack is active         */
#define KB_MEMDEBUG     0x00000004  /* memdebug is active           */
#define KB_SERIALPORT   0x00000008  /* debugging serial port active */
#define KB_IODEBUG      0x00000010  /* iodebug is active            */
#define KB_MPACTIVE	0x00000020  /* mp is active                 */
#define KB_UNIQUEID	0x00000040  /* unique ID chip is present    */
#define KB_OPERATOR_UP  0x00000080  /* operator is ready for device rescan */

#define kb_CPUFlags     kb_Flags

bool IsUser(void);
#define IsSuper() !IsUser()


/*****************************************************************************/


#endif /* __KERNEL_KERNEL_H */
