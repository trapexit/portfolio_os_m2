/* @(#) monitor.c 96/11/15 1.5 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>

/*****************************************************************************/


const char *crashNames[]=
{
    "Unknown",
    "Program Exception",
    "Trace Exception",
    "Debugger Exception",
    "Machine Check Exception",
    "Data Access Fault",
    "Instruction Access Fault",
    "Alignment Exception",
    "I/O Error Exception",
    "Bad 602 Instruction",
    "SMI",
    "Misc"
};


uint32 _gInBuf, _gOutBuf;


/*****************************************************************************/


#ifndef BUILD_DEBUGGER

void MonitorFirq(void)
{
}

void Dbgr_MPIOReqCreated(IOReq *ior)
{
}

void Dbgr_MPIOReqDeleted(IOReq *ior)
{
}

void Dbgr_MPIOReqCrashed(IOReq *ior, CrashCauses cause, uint32 pc)
{
}


#endif
