/* @(#) main.c 96/11/01 1.125 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/internalf.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>
#include <stdio.h>


/*****************************************************************************/


#define	DBUG(x) /* printf x */

extern void _InitKernelModule(void);
extern void InitKernelBase(uint32 (*QueryROMSysInfo)());
extern void InitMem(void);
extern void InitItemTable(void);
extern char copyright[];


/*****************************************************************************/


void main(uint32 (*QueryROMSysInfo)())
{
    InitKernelBase(QueryROMSysInfo);
    DBUG(("Kernel main entered\n"));

#ifdef BUILD_DEBUGGER
    DBUG(("Calling Dbgr_SystemStartup\n"));
    Dbgr_SystemStartup();
#endif

    printf("\n%s\n\n", copyright);

    DBUG(("Calling InitMem\n"));
    InitMem();

#ifdef MEMDEBUG
    DBUG(("Activating MemDebug\n"));
    externalCreateMemDebug(NULL);
    externalControlMemDebug(MEMDEBUGF_ALLOC_PATTERNS
                          | MEMDEBUGF_FREE_PATTERNS
                          | MEMDEBUGF_PAD_COOKIES);
#endif

    DBUG(("Calling InitItemTable\n"));
    InitItemTable();

    DBUG(("Calling AssignItem\n"));
    KB_FIELD(kb.fn.n_Item) = AssignItem(KernelBase, 1);

    DBUG(("Calling CreateBootModules\n"));
    CreateBootModules();

#ifdef BUILD_DEBUGGER
    DBUG(("Calling Dbgr_MemoryRanges\n"));
    Dbgr_MemoryRanges();
#endif

    DBUG(("Calling _InitKernelModule\n"));
    _InitKernelModule();
}
