/* @(#) monitor_mac.c 96/11/13 1.52 */

#include <kernel/types.h>
#include <hardware/bridgit.h>
#include <hardware/debugger.h>
#include <hardware/PPC.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>
#include <kernel/interrupts.h>
#include <kernel/task.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#ifdef BUILD_MACDEBUGGER


/*****************************************************************************/


#define DBUG(x) /* printf x */

#define NGPR   32
#define NFPR   32

#define MONITOR_PANIC 0
#define MONITOR_RETRY 1

#define SLAVE_CONTEXT ((Task *)-1)


/*****************************************************************************/


/* packet flavors sent from the debugger to the monitor */
typedef enum
{
    IN_NoMsg,
    IN_Unused0,
    IN_GetGPRs,
    IN_GetSPRs,
    IN_SetGPRs,
    IN_SetSPRs,
    IN_Abort,
    IN_Propagate,
    IN_SingleStep,
    IN_AcceptMonMsg,
    IN_BranchStep,
    IN_Unused1,
    IN_GetFPRs,
    IN_SetFPRs,
    IN_IInvalidate,
    IN_DFlush,
    IN_DInvalidate,
    IN_GetTaskInfo,
    IN_GetMonitorInfo,
    IN_GetModuleInfo
} InputPacketFlavors;

/* packet flavors sent from the monitor to the debugger */
typedef enum
{
    OUT_NoMsg,
    OUT_Ready,
    OUT_GPRDone,
    OUT_SPRDone,
    OUT_GPRSet,
    OUT_SPRSet,
    OUT_Error,
    OUT_Unused1,
    OUT_Unused2,
    OUT_TaskLaunch,
    OUT_Unused3,
    OUT_Unused4,
    OUT_FPRDone,
    OUT_FPRSet,
    OUT_TaskSet,
    OUT_InfoSet,
    OUT_TaskDied,
    OUT_ModuleSet
} OutputPacketFlavors;

/* why we're calling the monitor */
typedef enum
{
    OLDCAUSE_Unknown,
    OLDCAUSE_ProgramException,
    OLDCAUSE_TraceException,
    OLDCAUSE_ExternalException,
    OLDCAUSE_TaskLaunch,
    OLDCAUSE_MachineCheck,
    OLDCAUSE_DataAccess,
    OLDCAUSE_InstructionAccess,
    OLDCAUSE_Alignment,
    OLDCAUSE_IOError,
    OLDCAUSE_Bad602,
    OLDCAUSE_SMI,
    OLDCAUSE_Misc,
    OLDCAUSE_TaskDied,
    OLDCAUSE_ModuleLoaded,
    OLDCAUSE_ModuleUnloaded
} OldCauses;

/* info about why we've stopped, and other monitor specific stuff */
typedef struct
{
    OldCauses mi_Cause;           /* why we're here */
    uint32    mi_Version;         /* possible useful info for debugger */
    uint32    mi_Reserved[2];
} MonitorInfo;

/* important stuff about stopped task */
typedef struct
{
    void   *ti_TCB;
    void   *ti_TextStart;
    void   *ti_DataStart;
    void   *ti_Reserved;
    void   *ti_TaskEntry;
    char   *ti_TaskName;
    uint32  ti_ProcessorNumber;
} MTaskInfo;

/* important stuff about loaded module */
typedef struct
{
    void *mi_Module;
    void *mi_TextStart;
    void *mi_DataStart;
    void *mi_Reserved;
    void *mi_TaskEntry;
    char *mi_TaskName;
} ModuleInfo;

/* data for cache control */
typedef struct
{
    void   *cb_Addr;
    uint32  cb_NumBytes;
} CacheBlk;

/* supervisor registers we care about */
typedef struct
{
    uint32 sr_MSR;
    uint32 sr_HID0;
    uint32 sr_PVR;
    uint32 sr_DAR;
    uint64 sr_DBAT[4];
    uint64 sr_IBAT[4];
    uint32 sr_DSISR;
    uint32 sr_SPRG[4];
    uint32 sr_SRR0;
    uint32 sr_SRR1;
    uint32 sr_TBL;
    uint32 sr_TBU;
    uint32 sr_DEC;
    uint32 sr_ESASRR;
    uint32 sr_DMISS;
    uint32 sr_SER;
    uint32 sr_SEBR;
    uint32 sr_SGMT[16];
} SuperRegisters;

/* floating point registers we care about */
typedef struct
{
    uint32 fpr_FPRs[NFPR];
    uint32 fpr_FPSCR;
    uint32 fpr_SP;
    uint32 fpr_LT;
} FPRegisters;

/* user mode registers we care about */
typedef struct
{
    uint32 ur_GPRs[NGPR];
    uint32 ur_CR;
    uint32 ur_XER;
    uint32 ur_LR;
    uint32 ur_CTR;
    uint32 ur_PC;
} UserRegisters;

/* packet for communicating with debugger */
typedef struct
{
    uint32 mp_Flavor;
    uint32 mp_Reserved;

    union
    {
        MTaskInfo      ti;
        MonitorInfo    mi;
        ModuleInfo     modInfo;
        CacheBlk       cb;
        SuperRegisters sprs;
        UserRegisters  gprs;
        FPRegisters    fprs;
    } mp_Data;

} MonitorPkt;


#define mp_MTaskInfo   mp_Data.ti
#define mp_MonitorInfo mp_Data.mi
#define mp_ModuleInfo  mp_Data.modInfo
#define mp_CacheBlk    mp_Data.cb
#define mp_SuperRegs   mp_Data.sprs
#define mp_UserRegs    mp_Data.gprs
#define mp_FPRegs      mp_Data.fprs


/*****************************************************************************/


/* globals used internally by the monitor */
typedef struct
{
    OldCauses  mg_Cause;
    Task      *mg_TCB;
    Module    *mg_Module;
    void      *mg_TextAddr;
    void      *mg_DataAddr;
    void      *mg_EntryPoint;
    char      *mg_Name;
} MonitorGlobals;


/*****************************************************************************/


extern MonitorPkt     *_gInBuf;
extern MonitorPkt     *_gOutBuf;
static MonitorGlobals  g;


/*****************************************************************************/


/* clear msg field of i/o structs */
static void ClearMessages(void)
{
    _gInBuf->mp_Flavor  = IN_NoMsg;
    _gOutBuf->mp_Flavor = OUT_NoMsg;

    /* flush the data out */
    _dcbf(_gInBuf);
    _dcbf(_gOutBuf);
}


/*****************************************************************************/


static void GoDebugger(OutputPacketFlavors flavor)
{
    _gOutBuf->mp_Flavor = flavor;
    FlushDCache(0, _gOutBuf, sizeof(MonitorPkt));
}


/*****************************************************************************/


/* tell debugger we're ready */
static void NotifyDebugger(void)
{
    ClearMessages();
    GoDebugger(OUT_Ready);

    /* wait for the debugger to be ready */
    while (!DEBUGGER_ACK())
	;

    /* debugger is ready.  stuff mailbox with magic cookie */
    /* 82 = monitor active command */
    /* 8000 = trigger interrupt */
    BRIDGIT_WRITE(BRIDGIT_BASE,BR_MISC_REG,0x00828000);
}


/*****************************************************************************/


/* tell debugger we're done for now */
static void ClearDebugger(void)
{
    ClearMessages();

    /* clear the mailbox */
    BRIDGIT_WRITE(BRIDGIT_BASE,BR_MISC_REG,0x00000000);
}


/*****************************************************************************/


/* debugger wants the user registers */
static void GetGPRs(void)
{
uint32    i;
RegBlock *rb;

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
            rb = &KB_FIELD(kb_SlaveState)->ss_RegSave;
        else
            rb = &g.mg_TCB->t_RegisterSave;

	for (i = 0 ; i < NGPR; i++)
	    _gOutBuf->mp_UserRegs.ur_GPRs[i] = rb->rb_GPRs[i];

	_gOutBuf->mp_UserRegs.ur_PC  = rb->rb_PC;
	_gOutBuf->mp_UserRegs.ur_XER = rb->rb_XER;
	_gOutBuf->mp_UserRegs.ur_CR  = rb->rb_CR;
	_gOutBuf->mp_UserRegs.ur_LR  = rb->rb_LR;
	_gOutBuf->mp_UserRegs.ur_CTR = rb->rb_CTR;
    }
    else
    {
        memset(&_gOutBuf->mp_UserRegs, 0, sizeof(UserRegisters));
	_gOutBuf->mp_UserRegs.ur_PC = _mfsrr0();
    }

    GoDebugger(OUT_GPRDone);
}


/*****************************************************************************/


/* stuff user registers with values from debugger */
static void SetGPRs(void)
{
uint32    i;
RegBlock *rb;

    externalInvalidateDCache(_gInBuf, sizeof(MonitorPkt));

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
            rb = &KB_FIELD(kb_SlaveState)->ss_RegSave;
        else
            rb = &g.mg_TCB->t_RegisterSave;

        for (i = 0; i < NGPR; i++)
            rb->rb_GPRs[i] = _gInBuf->mp_UserRegs.ur_GPRs[i];

        rb->rb_PC  = _gInBuf->mp_UserRegs.ur_PC;
        rb->rb_XER = _gInBuf->mp_UserRegs.ur_XER;
        rb->rb_CR  = _gInBuf->mp_UserRegs.ur_CR;
        rb->rb_LR  = _gInBuf->mp_UserRegs.ur_LR;
        rb->rb_CTR = _gInBuf->mp_UserRegs.ur_CTR;
    }

    GoDebugger(OUT_GPRDone);
}


/*****************************************************************************/


/* get sprs and give them to debugger */
static void GetSPRs(void)
{
    memset(&_gOutBuf->mp_SuperRegs,0,sizeof(SuperRegisters));

    if (g.mg_TCB && (g.mg_TCB != SLAVE_CONTEXT))
	_gOutBuf->mp_SuperRegs.sr_MSR = g.mg_TCB->t_RegisterSave.rb_MSR;
    else
	_gOutBuf->mp_SuperRegs.sr_MSR = _mfmsr();

    _gOutBuf->mp_SuperRegs.sr_HID0    = _mfhid0();
    _gOutBuf->mp_SuperRegs.sr_PVR     = _mfpvr();

    _gOutBuf->mp_SuperRegs.sr_IBAT[0] = _mfibat0();
    _gOutBuf->mp_SuperRegs.sr_IBAT[1] = _mfibat1();
    _gOutBuf->mp_SuperRegs.sr_IBAT[2] = _mfibat2();
    _gOutBuf->mp_SuperRegs.sr_IBAT[3] = _mfibat3();

    _gOutBuf->mp_SuperRegs.sr_DBAT[0] = _mfdbat0();
    _gOutBuf->mp_SuperRegs.sr_DBAT[1] = _mfdbat1();
    _gOutBuf->mp_SuperRegs.sr_DBAT[2] = _mfdbat2();
    _gOutBuf->mp_SuperRegs.sr_DBAT[3] = _mfdbat3();

    _gOutBuf->mp_SuperRegs.sr_SPRG[0] = _mfsprg0();
    _gOutBuf->mp_SuperRegs.sr_SPRG[1] = _mfsprg1();
    _gOutBuf->mp_SuperRegs.sr_SPRG[2] = _mfsprg2();
    _gOutBuf->mp_SuperRegs.sr_SPRG[3] = _mfsprg3();

    _gOutBuf->mp_SuperRegs.sr_SRR0    = _mfsrr0();
    _gOutBuf->mp_SuperRegs.sr_SRR1    = _mfsrr1();
    _gOutBuf->mp_SuperRegs.sr_ESASRR  = _mfesasrr();
    _gOutBuf->mp_SuperRegs.sr_DEC     = _mfdec();
    _gOutBuf->mp_SuperRegs.sr_DAR     = _mfdar();
    _gOutBuf->mp_SuperRegs.sr_DSISR   = _mfdsisr();
    _gOutBuf->mp_SuperRegs.sr_TBU     = _mftbu();
    _gOutBuf->mp_SuperRegs.sr_TBL     = _mftbl();
    _gOutBuf->mp_SuperRegs.sr_DMISS   = _mfdmiss();
    _gOutBuf->mp_SuperRegs.sr_SER     = _mfser();
    _gOutBuf->mp_SuperRegs.sr_SEBR    = _mfsebr();

    GetSegRegs(_gOutBuf->mp_SuperRegs.sr_SGMT);

    GoDebugger(OUT_SPRDone);
}


/*****************************************************************************/


/* take sprs from debugger and stuff into registers */
static void SetSPRs(void)
{
    externalInvalidateDCache(_gInBuf, sizeof(MonitorPkt));

    if (g.mg_TCB && (g.mg_TCB != SLAVE_CONTEXT))
	g.mg_TCB->t_RegisterSave.rb_MSR = _gOutBuf->mp_SuperRegs.sr_MSR;

    _mthid0(_gInBuf->mp_SuperRegs.sr_HID0);

    /* PPC requires that the BATs be cleared before being set */
    _mtibat0(0,0);
    _mtibat1(0,0);
    _mtibat2(0,0);
    _mtibat3(0,0);
    _mtibat0(_gInBuf->mp_SuperRegs.sr_IBAT[0] >> 32,_gInBuf->mp_SuperRegs.sr_IBAT[0]);
    _mtibat1(_gInBuf->mp_SuperRegs.sr_IBAT[1] >> 32,_gInBuf->mp_SuperRegs.sr_IBAT[1]);
    _mtibat2(_gInBuf->mp_SuperRegs.sr_IBAT[2] >> 32,_gInBuf->mp_SuperRegs.sr_IBAT[2]);
    _mtibat3(_gInBuf->mp_SuperRegs.sr_IBAT[3] >> 32,_gInBuf->mp_SuperRegs.sr_IBAT[3]);

    _mtdbat0(0,0);
    _mtdbat1(0,0);
    _mtdbat2(0,0);
    _mtdbat3(0,0);
    _mtdbat0(_gInBuf->mp_SuperRegs.sr_DBAT[0] >> 32,_gInBuf->mp_SuperRegs.sr_DBAT[0]);
    _mtdbat1(_gInBuf->mp_SuperRegs.sr_DBAT[1] >> 32,_gInBuf->mp_SuperRegs.sr_DBAT[1]);
    _mtdbat2(_gInBuf->mp_SuperRegs.sr_DBAT[2] >> 32,_gInBuf->mp_SuperRegs.sr_DBAT[2]);
    _mtdbat3(_gInBuf->mp_SuperRegs.sr_DBAT[3] >> 32,_gInBuf->mp_SuperRegs.sr_DBAT[3]);

    _mtsprg0(_gInBuf->mp_SuperRegs.sr_SPRG[0]);
    _mtsprg1(_gInBuf->mp_SuperRegs.sr_SPRG[1]);
    _mtsprg2(_gInBuf->mp_SuperRegs.sr_SPRG[2]);
    _mtsprg3(_gInBuf->mp_SuperRegs.sr_SPRG[3]);

    _mtsrr0(_gInBuf->mp_SuperRegs.sr_SRR0);
    _mtsrr1(_gInBuf->mp_SuperRegs.sr_SRR1);

    _mtesasrr(_gInBuf->mp_SuperRegs.sr_ESASRR);
    _mtdec(_gInBuf->mp_SuperRegs.sr_DEC);
    _mtdar(_gInBuf->mp_SuperRegs.sr_DAR);
    _mtdsisr(_gInBuf->mp_SuperRegs.sr_DSISR);
    _mtser(_gInBuf->mp_SuperRegs.sr_SER);
    _mtsebr(_gInBuf->mp_SuperRegs.sr_SEBR);

    /* we don't restore: TBU, TBL, DMISS, and SGMT */

    GoDebugger(OUT_SPRDone);
}


/*****************************************************************************/


/* get fpr0-fpr31,fpscr,sp,lt from hardware and give to debugger */
__asm static void GetFPRegs(uint32* addr)
{
%	reg addr;

        mfmsr  r5
	ori    r8,r5,0
	ori    r5,r5,0x2000
	mtmsr  r5
	isync

	/* Save the SP/LT bits		  */
	mfspr	r5,fSP
	mfspr	r6,LT
	isync

	/* Setup SP/LT to claim all are shorts so we can save them */
	li	r7,0
	mtspr	LT,r7
	addi	r7,r0,-1
	mtspr	fSP,r7

	/* snarf those registers */
	stfs   f0,0(addr)
	stfs   f1,4(addr)
	stfs   f2,8(addr)
	stfs   f3,12(addr)
	stfs   f4,16(addr)
	stfs   f5,20(addr)
	stfs   f6,24(addr)
	stfs   f7,28(addr)
	stfs   f8,32(addr)
	stfs   f9,36(addr)
	stfs   f10,40(addr)
	stfs   f11,44(addr)
	stfs   f12,48(addr)
	stfs   f13,52(addr)
	stfs   f14,56(addr)
	stfs   f15,60(addr)
	stfs   f16,64(addr)
	stfs   f17,68(addr)
	stfs   f18,72(addr)
	stfs   f19,76(addr)
	stfs   f20,80(addr)
	stfs   f21,84(addr)
	stfs   f22,88(addr)
	stfs   f23,92(addr)
	stfs   f24,96(addr)
	stfs   f25,100(addr)
	stfs   f26,104(addr)
	stfs   f27,108(addr)
	stfs   f28,112(addr)
	stfs   f29,116(addr)
	stfs   f30,120(addr)
	stfs   f31,124(addr)

	mffs   f0
	li     r7,128
	stfiwx f0,r7,addr
	lfs    f0,0(addr)

	stw    r5,132(addr)
	stw    r6,136(addr)
	mtspr  LT,r6
	mtspr  fSP,r5

	mtmsr r8
	isync
}


/*****************************************************************************/


/* debugger wants the floating point registers */
static void GetFPRs(void)
{
uint32 i;

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
        {
            for (i = 0 ; i < NFPR; i++)
                _gOutBuf->mp_FPRegs.fpr_FPRs[i] = KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_FPRs[i];

            _gOutBuf->mp_FPRegs.fpr_SP    = KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_SP;
            _gOutBuf->mp_FPRegs.fpr_LT    = KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_LT;
            _gOutBuf->mp_FPRegs.fpr_FPSCR = KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_FPSCR;
        }
        else if (KB_FIELD(kb_FPOwner) != g.mg_TCB)
        {
            for (i = 0 ; i < NFPR; i++)
                _gOutBuf->mp_FPRegs.fpr_FPRs[i] = g.mg_TCB->t_FPRegisterSave.fprb_FPRs[i];

            _gOutBuf->mp_FPRegs.fpr_SP    = g.mg_TCB->t_FPRegisterSave.fprb_SP;
            _gOutBuf->mp_FPRegs.fpr_LT    = g.mg_TCB->t_FPRegisterSave.fprb_LT;
            _gOutBuf->mp_FPRegs.fpr_FPSCR = g.mg_TCB->t_FPRegisterSave.fprb_FPSCR;
        }
        else
        {
            GetFPRegs((uint32 *) &_gOutBuf->mp_FPRegs);
        }
    }
    else
    {
        GetFPRegs((uint32 *) &_gOutBuf->mp_FPRegs);
    }
    GoDebugger(OUT_FPRDone);
}


/*****************************************************************************/


/* stuff hardware fpr0-fpr31,fpscr,sp,lt with values from debugger */
__asm static void SetFPRegs(uint32* addr)
{
%	reg addr;

        mfmsr  r5
	ori    r6,r5,0
	ori    r5,r5,0x2000
	mtmsr  r5
	isync

	lfs   f0,128(addr)
	lis   r5,0x8000
	mtspr LT,r5
	li    r5,0
	mtspr fSP,r5
	mtfsf 0xff,f0

	lfs   f0,0(addr)
	lfs   f1,4(addr)
	lfs   f2,8(addr)
	lfs   f3,12(addr)
	lfs   f4,16(addr)
	lfs   f5,20(addr)
	lfs   f6,24(addr)
	lfs   f7,28(addr)
	lfs   f8,32(addr)
	lfs   f9,36(addr)
	lfs   f10,40(addr)
	lfs   f11,44(addr)
	lfs   f12,48(addr)
	lfs   f13,52(addr)
	lfs   f14,56(addr)
	lfs   f15,60(addr)
	lfs   f16,64(addr)
	lfs   f17,68(addr)
	lfs   f18,72(addr)
	lfs   f19,76(addr)
	lfs   f20,80(addr)
	lfs   f21,84(addr)
	lfs   f22,88(addr)
	lfs   f23,92(addr)
	lfs   f24,96(addr)
	lfs   f25,100(addr)
	lfs   f26,104(addr)
	lfs   f27,108(addr)
	lfs   f28,112(addr)
	lfs   f29,116(addr)
	lfs   f30,120(addr)
	lfs   f31,124(addr)

	lwz   r5,132(addr)
	mtspr fSP,r5
	lwz   r5,136(addr)
	mtspr LT,r5

	mtmsr r6
	isync
}


/*****************************************************************************/


/* restore fp registers with values from debugger */
static void SetFPRs(void)
{
uint32 i;

    externalInvalidateDCache(_gInBuf, sizeof(MonitorPkt));

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
        {
            for (i = 0; i < NFPR; i++)
                KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_FPRs[i] = _gInBuf->mp_FPRegs.fpr_FPRs[i];

            KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_FPSCR = _gInBuf->mp_FPRegs.fpr_FPSCR;
            KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_LT    = _gInBuf->mp_FPRegs.fpr_LT;
            KB_FIELD(kb_SlaveState)->ss_FPRegSave.fprb_SP    = _gInBuf->mp_FPRegs.fpr_SP;
        }
        else if (KB_FIELD(kb_FPOwner) != g.mg_TCB)
        {
            for (i = 0; i < NFPR; i++)
                g.mg_TCB->t_FPRegisterSave.fprb_FPRs[i] = _gInBuf->mp_FPRegs.fpr_FPRs[i];

            g.mg_TCB->t_FPRegisterSave.fprb_FPSCR = _gInBuf->mp_FPRegs.fpr_FPSCR;
            g.mg_TCB->t_FPRegisterSave.fprb_LT    = _gInBuf->mp_FPRegs.fpr_LT;
            g.mg_TCB->t_FPRegisterSave.fprb_SP    = _gInBuf->mp_FPRegs.fpr_SP;
            g.mg_TCB->t_RegisterSave.rb_MSR      &= (~MSR_FP);   /* cause the FP state to be reloaded */
        }
        else
        {
            SetFPRegs((uint32 *) &_gInBuf->mp_FPRegs);
        }
    }
    else
    {
        SetFPRegs((uint32 *) &_gInBuf->mp_FPRegs);
    }

    GoDebugger(OUT_FPRDone);
}


/*****************************************************************************/


/* give debugger info about the stopped task */
static void GetMTaskInfo(void)
{
    if (g.mg_TCB == SLAVE_CONTEXT)
    {
        _gOutBuf->mp_MTaskInfo.ti_TCB             = SLAVE_CONTEXT;
        _gOutBuf->mp_MTaskInfo.ti_TextStart       = NULL;
        _gOutBuf->mp_MTaskInfo.ti_DataStart       = NULL;
        _gOutBuf->mp_MTaskInfo.ti_Reserved        = NULL;
        _gOutBuf->mp_MTaskInfo.ti_TaskEntry       = NULL;
        _gOutBuf->mp_MTaskInfo.ti_TaskName        = "Slave Context";
     }
    else
    {
        _gOutBuf->mp_MTaskInfo.ti_TCB             = g.mg_TCB;
        _gOutBuf->mp_MTaskInfo.ti_TextStart       = g.mg_TextAddr;
        _gOutBuf->mp_MTaskInfo.ti_DataStart       = g.mg_DataAddr;
        _gOutBuf->mp_MTaskInfo.ti_Reserved        = NULL;
        _gOutBuf->mp_MTaskInfo.ti_TaskEntry       = g.mg_EntryPoint;
        _gOutBuf->mp_MTaskInfo.ti_TaskName        = g.mg_Name;
    }

    GoDebugger(OUT_TaskSet);
}


/*****************************************************************************/


/* give debugger info about the just-loaded module */
static void GetModuleInfo(void)
{
    _gOutBuf->mp_ModuleInfo.mi_Module     = g.mg_Module;
    _gOutBuf->mp_ModuleInfo.mi_TextStart  = g.mg_TextAddr;
    _gOutBuf->mp_ModuleInfo.mi_DataStart  = g.mg_DataAddr;
    _gOutBuf->mp_ModuleInfo.mi_TaskEntry  = g.mg_EntryPoint;
    _gOutBuf->mp_ModuleInfo.mi_TaskName   = g.mg_Name;

    GoDebugger(OUT_ModuleSet);
}


/*****************************************************************************/


/* give debugger info about why we've stopped */
static void GetMonitorInfo(void)
{
    _gOutBuf->mp_MonitorInfo.mi_Cause       = g.mg_Cause;
    _gOutBuf->mp_MonitorInfo.mi_Version     = 0;
    _gOutBuf->mp_MonitorInfo.mi_Reserved[0] = 0;
    _gOutBuf->mp_MonitorInfo.mi_Reserved[1] = 0;

    GoDebugger(OUT_InfoSet);
}


/*****************************************************************************/


/* Execute a single step by setting the msr trace bits.
 * When the task is relaunched, msr will have correct value
 */
static void DoSingleStep(void)
{
uint32 old;

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
            KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR |= (MSR_SE | MSR_BE);
        else
            g.mg_TCB->t_RegisterSave.rb_MSR |= (MSR_SE | MSR_BE);
    }
    else
    {
        old = _mfsrr1();
        _mtsrr1(old | MSR_SE | MSR_BE);
    }
}


/*****************************************************************************/


/* Execute a bracnh step by setting the msr trace bits.
 * When the task is relaunched, msr will have correct value
 */
static void DoBranchStep(void)
{
uint32 old;

    if (g.mg_TCB)
    {
        if (g.mg_TCB == SLAVE_CONTEXT)
            KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR |= MSR_BE;
        else
            g.mg_TCB->t_RegisterSave.rb_MSR |= MSR_BE;
    }
    else
    {
        old = _mfsrr1();
        _mtsrr1(old | MSR_BE);
    }
}


/*****************************************************************************/


/* flush data cache because debugger told us to */
static void DoDCacheFlush(void)
{
    if (_gInBuf->mp_CacheBlk.cb_NumBytes == 0)
    {
        /* flush entire cache */
        FlushDCacheAll(0);
    }
    else
    {
        /* flush how ever much they tell me to */
        FlushDCache(0, _gInBuf->mp_CacheBlk.cb_Addr, _gInBuf->mp_CacheBlk.cb_NumBytes);
    }
}


/*****************************************************************************/


/* we've stopped.  sit in a loop doing what the debugger tells us to do. */
static uint32 MonitorEventLoop(OldCauses cause, Task *t)
{
bool   exit;
uint32 result;

    SuperSlaveRequest(SLAVE_SUSPEND, 0, 0, 0);

    g.mg_Cause = cause;
    g.mg_TCB   = t;
    NotifyDebugger(); /* tell debugger we're here */

    result = MONITOR_PANIC;
    exit   = FALSE;

    /* process debugger commands until it says we're done */
    while (!exit)
    {
        externalInvalidateDCache(_gInBuf, sizeof(*_gInBuf));

	if (_gInBuf->mp_Flavor != IN_NoMsg)
	{
            switch (_gInBuf->mp_Flavor)
            {
                case IN_GetGPRs       : /* debugger wants to see the GPRs */
                                        GetGPRs();
                                        break;

                case IN_GetSPRs       : /* debugger wants to see the SPRs */
                                        GetSPRs();
                                        break;

                case IN_SetGPRs       : /* debugger changed GPRs, update registers */
                                        SetGPRs();
                                        break;

                case IN_SetSPRs       : /* debugger changed SPRs, update registers */
                                        SetSPRs();
                                        break;

                case IN_Abort         : /* we've handled the exception */
                                        exit   = TRUE;
                                        result = MONITOR_RETRY;
                                        break;

                case IN_Propagate     : /* we didn't handle the exception */
                                        exit   = TRUE;
                                        result = MONITOR_PANIC;

                                        if (g.mg_TCB == SLAVE_CONTEXT)
                                            SuperSlaveRequest(SLAVE_ABORT, 0, 0, 0);

                                        break;

                case IN_SingleStep    : /* source stepping */
                                        DoSingleStep();
                                        exit   = TRUE;
                                        result = MONITOR_RETRY;
                                        break;

                case IN_AcceptMonMsg  : /* used when monitor called from non-exception condition (task launch, etc) */
                                        exit   = TRUE;
                                        result = MONITOR_RETRY;
                                        break;

                case IN_BranchStep    : /* source single stepping over a branch */
                                        DoBranchStep();
                                        exit   = TRUE;
                                        result = MONITOR_RETRY;
                                        break;

                case IN_GetFPRs       : /* debugger changed FPRs, update registers */
                                        GetFPRs();
                                        break;

                case IN_SetFPRs       : /* debugger changed FPRs, update registers */
                                        SetFPRs();
                                        break;

                case IN_IInvalidate   : /* debugger wants us to inval icache. go ahead and do it */
                                        InvalidateICache();
                                        SuperSlaveRequest(SLAVE_INVALIDATEICACHE, 0, 0, 0);
                                        break;

                case IN_DFlush        : /* debugger wants us to flush dcache. go ahead and do it */
                                        DoDCacheFlush();
                                        break;

                case IN_DInvalidate   : /* not supported */
                                        break;

                case IN_GetTaskInfo   : GetMTaskInfo();
                                        break;

                case IN_GetMonitorInfo: GetMonitorInfo();
                                        break;

                case IN_GetModuleInfo : GetModuleInfo();
                                        break;

                default               : /* we should never get here, but for completeness... */
                                        break;
            }

	    _gInBuf->mp_Flavor = IN_NoMsg;
	    _dcbf(_gInBuf);
	}
    }

    FlushDCacheAll(0);
    SuperSlaveRequest(SLAVE_CONTINUE, 0, 0, 0);

    return result;
}


/*****************************************************************************/


void MonitorFirq(void)
{
uint32  pc;
bool    found;
Module *module;

    DBUG(("MONITOR: firq called\n"));

    BRIDGIT_WRITE(BRIDGIT_BASE,(BR_INT_STS | BR_CLEAR_OFFSET),(BR_NUBUS_INT|BR_INT_SENT));
    BRIDGIT_WRITE(BRIDGIT_BASE,(BR_INT_STS | BR_CLEAR_OFFSET),(0));
    BDA_CLR(BDAPCTL_ERRSTAT,BDAINT_BRDG_MASK);
    BDA_CLR(BDAPCTL_ERRSTAT,0);

    printf("\n### Stopping\n");
    pc    = _mfsrr0();
    found = FALSE;
    ScanList(&KB_FIELD(kb_Modules), module, Module)
    {
        if ((pc >= (uint32)module->li->codeBase)
         && (pc < (uint32)module->li->codeBase + module->li->codeLength))
        {
            found = TRUE;
            break;
        }
    }

    if (found)
        printf("### PC is in module '%s', offset 0x%x\n", module->n.n_Name, pc - (uint32)module->li->codeBase);
    else
        printf("### PC is not in a known code module!\n");

    if (CURRENTTASK)
        DumpTask("\nTask:", CURRENTTASK);
    else
        printf("\nNo active task!\n");

    if (MonitorEventLoop(OLDCAUSE_ExternalException, CURRENTTASK) == MONITOR_RETRY)
    {
        ClearDebugger();
        return;
    }

    if (CURRENTTASK)
    {
        ClearDebugger();
        printf("### Killing task '%s'\n", CURRENTTASK->t.n_Name);
        Murder(CURRENTTASK, CURRENTTASK);
    }
}


/*****************************************************************************/


/* a program exception occurred.  do the right thing. */
static uint32 HandleProgramException(Task *t)
{
    if (!t)
        return MONITOR_PANIC;

    return MonitorEventLoop(OLDCAUSE_ProgramException, t);
}


/*****************************************************************************/


/* handle the trace exception that just occurred */
static uint32 HandleTrace(Task *t)
{
uint32 old;

    if (t)
    {
        if (t == SLAVE_CONTEXT)
            KB_FIELD(kb_SlaveState)->ss_RegSave.rb_MSR &= ~(MSR_SE|MSR_BE);
        else
            t->t_RegisterSave.rb_MSR &= ~(MSR_SE|MSR_BE);
    }
    else
    {
        old = _mfsrr1();
        old &=  ~(MSR_SE|MSR_BE);
        _mtsrr1(old);
    }

    return MonitorEventLoop(OLDCAUSE_TraceException, t);
}


/*****************************************************************************/


static uint32 CallMonitor(OldCauses cause, Task *t, ...)
{
va_list args;
uint32  result;

    if (_gInBuf == NULL)
    {
        /* Monitor was not successfully initialized. */
        return MONITOR_PANIC;
    }

    va_start(args, t);
    switch (cause)
    {
        case OLDCAUSE_TaskLaunch        : g.mg_TextAddr   = va_arg(args, uint32 *);
                                          g.mg_DataAddr   = va_arg(args, uint32 *);
                                          g.mg_EntryPoint = va_arg(args, uint32 *);
                                          g.mg_Name       = va_arg(args, char *);
                                          result = MonitorEventLoop(OLDCAUSE_TaskLaunch, t);
                                          break;

        case OLDCAUSE_TaskDied          : result = MonitorEventLoop(OLDCAUSE_TaskDied, t);
                                          break;

        case OLDCAUSE_ModuleLoaded      : g.mg_Module     = (Module *)va_arg(args, Module *);
                                          g.mg_TextAddr   = va_arg(args, uint32 *);
                                          g.mg_DataAddr   = va_arg(args, uint32 *);
                                          g.mg_EntryPoint = va_arg(args, uint32 *);
                                          g.mg_Name       = va_arg(args, char *);
                                          result = MonitorEventLoop(OLDCAUSE_ModuleLoaded, t);
                                          break;

        case OLDCAUSE_ModuleUnloaded    : g.mg_Module = (Module *)va_arg(args, Module *);
                                          result = MonitorEventLoop(OLDCAUSE_ModuleUnloaded, t);
                                          break;

        default                         : cause = OLDCAUSE_Misc;
        case OLDCAUSE_Misc              :
        case OLDCAUSE_SMI               :
        case OLDCAUSE_Bad602            :
        case OLDCAUSE_IOError           :
        case OLDCAUSE_MachineCheck      :
        case OLDCAUSE_DataAccess        :
        case OLDCAUSE_InstructionAccess :
        case OLDCAUSE_Alignment         : result = MonitorEventLoop(cause, t);
                                          break;

        case OLDCAUSE_ProgramException  : result = HandleProgramException(t);
                                          break;

        case OLDCAUSE_TraceException    : result = HandleTrace(t);
                                          break;
    }
    va_end(args);

    ClearDebugger();

    return result;
}


/*****************************************************************************/


void Dbgr_SystemStartup(void)
{
}


/*****************************************************************************/


void Dbgr_MemoryRanges(void)
{
}


/*****************************************************************************/


void Dbgr_TaskCreated(Task *t, LoaderInfo *li)
{
    CallMonitor(OLDCAUSE_TaskLaunch, t,
                li->codeBase,
                li->dataBase,
                li->entryPoint,
                t->t.n_Name);
}


/*****************************************************************************/


void Dbgr_TaskDeleted(Task *t)
{
    CallMonitor(OLDCAUSE_TaskDied, t);
}


/*****************************************************************************/


void Dbgr_TaskCrashed(Task *t, CrashCauses cause, uint32 pc)
{
OldCauses oldcause;

    TOUCH(pc);

    switch (cause)
    {
        case CRASH_Unknown          : oldcause = OLDCAUSE_Unknown; break;
        case CRASH_ProgramException : oldcause = OLDCAUSE_ProgramException; break;
        case CRASH_TraceException   : oldcause = OLDCAUSE_TraceException; break;
        case CRASH_ExternalException: oldcause = OLDCAUSE_ExternalException; break;
        case CRASH_MachineCheck     : oldcause = OLDCAUSE_MachineCheck; break;
        case CRASH_DataAccess       : oldcause = OLDCAUSE_DataAccess; break;
        case CRASH_InstructionAccess: oldcause = OLDCAUSE_InstructionAccess; break;
        case CRASH_Alignment        : oldcause = OLDCAUSE_Alignment; break;
        case CRASH_IOError          : oldcause = OLDCAUSE_IOError; break;
        case CRASH_Bad602           : oldcause = OLDCAUSE_Bad602; break;
        case CRASH_SMI              : oldcause = OLDCAUSE_SMI; break;
        case CRASH_Misc             : oldcause = OLDCAUSE_Misc; break;
        default                     : oldcause = OLDCAUSE_Unknown; break;
    }

    if (CallMonitor(oldcause, t) != MONITOR_RETRY)
    {
        if (t != SLAVE_CONTEXT)
        {
            printf("### Killing task '%s'\n", t->t.n_Name);
            Murder(t, t);
        }
    }
}


/*****************************************************************************/


void Dbgr_MPIOReqCreated(IOReq *ior)
{
    TOUCH(ior);
}


/*****************************************************************************/


void Dbgr_MPIOReqDeleted(IOReq *ior)
{
    TOUCH(ior);
}


/*****************************************************************************/


void Dbgr_MPIOReqCrashed(IOReq *ior, CrashCauses cause, uint32 pc)
{
    TOUCH(ior);
    Dbgr_TaskCrashed((Task *)-1, cause, pc);
}


/*****************************************************************************/


void Dbgr_ModuleCreated(Module *m)
{
    TOUCH(m);
}


/*****************************************************************************/


void Dbgr_ModuleDeleted(Module *m)
{
    TOUCH(m);
}

#endif /* BUILD_MACDEBUGGER */
