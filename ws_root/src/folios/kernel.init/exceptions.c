/* @(#) exceptions.c 96/07/18 1.4 */

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/panic.h>
#include <kernel/operror.h>
#include <kernel/cache.h>
#include <kernel/internalf.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#define DBUG(x)     printf x
#else
#define DBUG(x)
#endif


/*****************************************************************************/


/* all of the kernel's exception handlers */
extern void systemCall(void);
extern void machineCheckHandler(void);
extern void FPExceptionHandler(void);
extern void traceHandler(void);
extern void handle602(void);
extern void dataAccessHandler(void);
extern void instrAccessHandler(void);
extern void alignmentHandler(void);
extern void programHandler(void);
extern void ioErrorHandler(void);
extern void iLoadMissHandler(void);
extern void iLoadMissHandlerEnd(void);
extern void dStoreMissHandler(void);
extern void dStoreMissHandlerEnd(void);
extern void dLoadMissHandler(void);
extern void dLoadMissHandlerEnd(void);
extern void instrAddrHandler(void);
extern void smiHandler(void);
extern void runModeHandler(void);
extern void miscExceptionHandler(void);
extern void interruptHandler(void);
extern void decrementerHandler(void);


/*****************************************************************************/


static void SetExceptionVector(void (*v)(void), uint32 vn)
{
uint32 *p;
uint32  ins;
int32   delta;

    p = (uint32 *)_mfibr();
    p += vn*256/sizeof(uint32);

    /* find the branch instruction */
    while (TRUE)
    {
        ins = *p;
        ins &= 0xfc000003;
        if (ins == 0x48000000)  /* relative branch instruction */
        {
            /* compute delta */
            delta = (int32)v - (int32)p;
            delta &= 0x03FFFFFFC;
            *p = ins | delta;
            return;
        }
        p++;
    }
}


/*****************************************************************************/


static void CopyExceptionVector(void (*v)(void), void (*ve)(void), uint32 vn)
{
uint8 *p;

    p = (uint8 *)_mfibr();
    memcpy(&p[vn * 256], v, (uint32)ve - (uint32)v);
}


/*****************************************************************************/


void SetupExceptionHandlers(void)
{
    /* SYSTEM_RESET handled in bootcode.s */

    SetExceptionVector (machineCheckHandler, MACHINE_CHK);
    SetExceptionVector (dataAccessHandler,   DATA_ACCESS);
    SetExceptionVector (instrAccessHandler,  INSTRUCT_ACCESS);
    SetExceptionVector (interruptHandler,    EXTERNAL_INT);
    SetExceptionVector (alignmentHandler,    ALIGNMENT);
    SetExceptionVector (programHandler,      PROGRAM);
    SetExceptionVector (FPExceptionHandler,  FP_UNAVAIL);
    SetExceptionVector (decrementerHandler,  DECREMENTER);
    SetExceptionVector (ioErrorHandler,      IO_ERROR);
    SetExceptionVector (systemCall,          SYSTEM_CALL);
    SetExceptionVector (traceHandler,        TRACE_EXCPTN);
    SetExceptionVector (handle602,           BAD_602_VER1);
    CopyExceptionVector(iLoadMissHandler,  iLoadMissHandlerEnd,  ILOAD_MISS);
    CopyExceptionVector(dLoadMissHandler,  dLoadMissHandlerEnd,  DLOAD_MISS);
    CopyExceptionVector(dStoreMissHandler, dStoreMissHandlerEnd, DSTORE_MISS);
    SetExceptionVector (instrAddrHandler,    INST_ADDR);
    SetExceptionVector (smiHandler,          SMI);
    SetExceptionVector (handle602,           BAD_602_VER2);

    /* make sure all the new handlers get seen by the CPU */
    FlushDCacheAll(0);
    InvalidateICache();
}
