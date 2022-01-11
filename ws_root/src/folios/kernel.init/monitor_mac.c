/* @(#) monitor_mac.c 96/07/03 1.9 */

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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


extern Item AllocFirq (char *name, int pri, int num, void (*code) ());
extern void MonitorFirq(void);

#ifdef BUILD_MACDEBUGGER

#define DBUG(x)/* printf x */

extern void *_gInBuf;	/* Really a MonitorPkt */
extern void *_gOutBuf;

/* initialize monitor state */
void InitMonitor(void)
{
  uint32 old;
  int32 err;
  DebuggerInfo *dbinfo;
  Item firq;

  DBUG(("MONITOR: initializing...\n"));

  _gInBuf = _gOutBuf = NULL;
  err = QUERY_SYS_INFO(SYSINFO_TAG_DEBUGGERREGION, dbinfo);
  if (err != SYSINFO_SUCCESS) {
#ifdef BUILD_STRINGS
     printf("No debugger monitor: err %x\n", err);
#endif
     return;
  }

  firq = AllocFirq("Monitor",200,INT_BDA_BRIDGIT,MonitorFirq);
  if(firq < 0) {
    printf("MONITOR: failed to alloc firq\n");
    return;
  }

  _gInBuf  = dbinfo->dbg_MonInPtr;
  _gOutBuf = dbinfo->dbg_MonOutPtr;

  /* nuke any bogus pending ints */
  BRIDGIT_WRITE(BRIDGIT_BASE,(BR_INT_STS | BR_CLEAR_OFFSET),BR_NUBUS_INT);
  BRIDGIT_WRITE(BRIDGIT_BASE,(BR_INT_STS | BR_CLEAR_OFFSET),BR_INT_SENT);
  BDA_CLR(BDAPCTL_ERRSTAT,BDAINT_BRDG_MASK);
  /* turn on the bridgit ints */
  old = BRIDGIT_READ(BRIDGIT_BASE,BR_INT_ENABLE);
  BRIDGIT_WRITE(BRIDGIT_BASE,BR_INT_ENABLE,(BR_NUBUS_INT | old) );
  /* allow bridgit ints to make it through */
  EnableInterrupt(INT_BDA_BRIDGIT);
}

#endif /* BUILD_MACDEBUGGER */
