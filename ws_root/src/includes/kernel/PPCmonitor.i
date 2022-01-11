#ifndef __KERNEL_PPCMONITOR_I
#define __KERNEL_PPCMONITOR_I


/******************************************************************************
**
**  @(#) PPCmonitor.i 96/10/05 1.3
**
******************************************************************************/


/* why we're calling the monitor */
#define CRASH_Unknown            0
#define CRASH_ProgramException   1
#define CRASH_TraceException     2
#define CRASH_ExternalException  3
#define CRASH_MachineCheck       4
#define CRASH_DataAccess         5
#define CRASH_InstructionAccess  6
#define CRASH_Alignment          7
#define CRASH_IOError            8
#define CRASH_Bad602             9
#define CRASH_SMI                0
#define CRASH_Misc               10


#endif /* __KERNEL_PPCMONITOR_I */
