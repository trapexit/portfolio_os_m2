/* @(#) monitor_pc.c 96/10/18 1.3 */

#ifdef BUILD_PCDEBUGGER
#include <kernel/kernel.h>
#include <hardware/cde.h>
#include <hardware/pcdebugger.h>
void InitMonitor(void)
{
        CDE_SET(KernelBase->kb_CDEBase,CARDCONF, CDE_BIOBUS_SAFE|CDE_IOCONF);
}

#endif /* BUILD_PCDEBUGGER */

/* keep the compiler happy... */
extern int foo;

