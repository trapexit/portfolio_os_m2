#ifndef __KERNEL_MONITOR_H
#define __KERNEL_MONITOR_H


/******************************************************************************
**
**  @(#) monitor.h 96/11/13 1.12
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* packet flavors sent by the debugger */
typedef enum
{
    DBGR_NOP,

    /* acknowledge a packet from the monitor          */
    DBGR_ACK,

    /* request data from the monitor */
    DBGR_GetGeneralRegs,
    DBGR_GetFPRegs,
    DBGR_GetSuperRegs,

    /* control the execution environment */
    DBGR_SetGeneralRegs,
    DBGR_SetFPRegs,
    DBGR_SetSuperRegs,
    DBGR_SetSingleStep,
    DBGR_SetBranchStep,
    DBGR_ClearSingleStep,
    DBGR_ClearBranchStep,
    DBGR_InvalidateICache,
    DBGR_FlushDCache,
    DBGR_SuspendTask,
    DBGR_ResumeTask,
    DBGR_AbortTask
} DebuggerFlavors;

/* packet flavors sent by the monitor */
typedef enum
{
    MON_NOP,

    /* spontaneous events generated by the OS */
    MON_SystemStartup,
    MON_TaskCreated,
    MON_TaskDeleted,
    MON_TaskCrashed,
    MON_ModuleCreated,
    MON_ModuleDeleted,

    /* sent in response to debugger requests */
    MON_ACK,
    MON_GeneralRegs,
    MON_FPRegs,
    MON_SuperRegs,

    /* sent when we enter the monitor because the debugger asked us to */
    MON_Hello,

    /* more spontaneous fun */
    MON_ModuleDependent,
    MON_MemoryRange
} MonitorFlavors;


/*****************************************************************************/


/* this appears at the head of every packet */
typedef struct
{
    uint32  ph_PacketSize;
    uint32  ph_Flavor;
    void   *ph_Object;
    uint32  ph_Pad[5];

    /* packet data follows */
} PacketHeader;


/*****************************************************************************/


/* packet data for messages of type MON_SystemStartup */
typedef struct
{
    PacketHeader si;
    uint32       si_OSVersion;
    uint32       si_OSRevision;
    uint32       si_Pad[6];
} StartupInfo;

/* packet data for messages of type DBGR_SetGenerelRegs and MON_GeneralRegs */
typedef struct
{
    PacketHeader gr;
    uint32       gr_GPRs[32];
    uint32       gr_CR;
    uint32       gr_XER;
    uint32       gr_LR;
    uint32       gr_CTR;
    uint32       gr_PC;
    uint32       gr_MSR;
    uint32       gr_Pad[2];
} GeneralRegs;

/* packet data for messages of type DBGR_SetFPRegs and MON_FPRegs */
typedef struct
{
    PacketHeader fpr;
    uint32       fpr_FPRs[32];
    uint32       fpr_FPSCR;
    uint32       fpr_SP;
    uint32       fpr_LT;
    uint32       fpr_Pad[5];
} FPRegs;

/* packet data for messages of type DBGR_SetSuperRegs and MON_SuperRegs */
typedef struct
{
    PacketHeader sr;
    uint32       sr_HID0;
    uint32       sr_PVR;
    uint32       sr_DAR;
    uint64       sr_DBAT[4];
    uint64       sr_IBAT[4];
    uint32       sr_DSISR;
    uint32       sr_SPRG[4];
    uint32       sr_SRR0;
    uint32       sr_SRR1;
    uint32       sr_TBL;
    uint32       sr_TBU;
    uint32       sr_DEC;
    uint32       sr_ESASRR;
    uint32       sr_DMISS;
    uint32       sr_SER;
    uint32       sr_SEBR;
    uint32       sr_SGMT[16];
} SuperRegs;

/* what crash occured */
typedef enum
{
    CRASH_Unknown,
    CRASH_ProgramException,
    CRASH_TraceException,
    CRASH_ExternalException,
    CRASH_MachineCheck,
    CRASH_DataAccess,
    CRASH_InstructionAccess,
    CRASH_Alignment,
    CRASH_IOError,
    CRASH_Bad602,
    CRASH_SMI,
    CRASH_Misc
} CrashCauses;

/* packet data for messages of type MON_TaskCrashed */
typedef struct
{
    PacketHeader ci;
    CrashCauses  ci_Cause;
    uint32       ci_PC;
    uint32       ci_Pad[6];
} CrashInfo;

/* packet data for messages of type MON_TaskCreated */
typedef struct
{
    PacketHeader  tci;
    char          tci_Name[64];
    void         *tci_Creator;
    bool          tci_Thread;     /* TRUE if a thread, FALSE if a task */
    void         *tci_Module;
    void         *tci_UserStackBase;
    uint32        tci_UserStackSize;
    void         *tci_SuperStackBase;
    uint32        tci_SuperStackSize;
    uint32        tci_Pad;
} TaskCreationInfo;

/* packet data for messages of type MON_TaskDeleted */
typedef struct
{
    PacketHeader tdi;
    int32        tdi_ExitStatus;
    uint32       tdi_Pad[7];
} TaskDeletionInfo;

/* packet data for messages of type MON_ModuleCreated */
typedef struct
{
    PacketHeader  mci;
    char          mci_Name[64];
    void         *mci_CodeStart;
    uint32        mci_CodeLength;
    void         *mci_DataStart;
    uint32        mci_DataLength;
    void         *mci_BSSStart;
    uint32        mci_BSSLength;
    void         *mci_EntryPoint;
    uint32        mci_Version;
    uint32        mci_Revision;
    char          mci_Path[256];
    uint32        mci_Pad[7];
} ModuleCreationInfo;

/* packet data for messages of type MON_ModuleDependent */
typedef struct
{
    PacketHeader  mdi;
    char          mdi_Name[64];
    uint32        mdi_Version;
    uint32        mdi_Revision;
    uint32        mdi_Pad[6];
} ModuleDependentInfo;

/* packet data for messages of type DBGR_ACK and MON_ACK */
typedef struct
{
    PacketHeader ack;
    int32        ack_ResultCode;
    uint32       ack_Pad[7];
} ACKInfo;

typedef enum
{
    MRT_ROM,
    MRT_RAM,
    MRT_REGS
} MemoryRangeTypes;

/* packet data for messages of type MON_MemoryRange */
typedef struct
{
    PacketHeader      mri;
    char              mri_Name[64];
    void             *mri_Start;
    void             *mri_End;
    MemoryRangeTypes  mri_Type;
    uint32            mri_Pad[5];
} MemoryRangeInfo;


/*****************************************************************************/


#ifdef KERNEL
extern const char *crashNames[];

void Dbgr_SystemStartup(void);
void Dbgr_MemoryRanges(void);
void Dbgr_TaskCreated(Task *t, LoaderInfo *li);
void Dbgr_TaskDeleted(Task *t);
void Dbgr_TaskCrashed(Task *t, CrashCauses cause, uint32 pc);
void Dbgr_MPIOReqCreated(IOReq *ior);
void Dbgr_NPIOReqDeleted(IOReq *ior);
void Dbgr_MPIOReqCrashed(IOReq *ior, CrashCauses cause, uint32 pc);
void Dbgr_ModuleCreated(Module *m);
void Dbgr_ModuleDeleted(Module *m);
#endif


/*****************************************************************************/


#endif /* __KERNEL_MONITOR_H */
