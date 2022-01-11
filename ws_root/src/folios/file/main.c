/* @(#) FileFolioMain.c 96/07/17 1.44 */

/*
  FileFolioMain.c - coldstart code for the filesystem.  Creates the
  file folio and the file driver, and mounts filesystems.
*/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/dipir.h>
#include <device/cdrom.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <misc/event.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

extern void FileDaemonStartup(void);
extern int32 FileSystemInit(void);
extern TagArg fileFolioTags[];

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

#ifdef SCANMOUNT
static void ScanMount(void);
#endif

#ifdef AUTOMOUNT
static void Automount(void);
Item msgPortItem;
#endif

#define	DAEMONUSERSTACK	2048

char fsDaemonStack[DAEMONUSERSTACK];
const int32 fsDaemonStackSize = DAEMONUSERSTACK;

int32 fsOpenForBusiness = FALSE;
int32 awakenSleepers = FALSE;

Semaphore *fsListSemaphore;

extern void FileDaemonInternals(void);

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) printf x
# define oqprintf(x) /* x */
#endif

extern const char whatstring[];

FileSystem * InitFileSystem(Device *theDevice);

#define DBUG0(x) /* printf x */

static void Automount(void)
{
  int32 theSignal;

  while (TRUE) {
    theSignal = WaitSignal(fileFolio.ff_Mounter.ffm_Signal);
    if (theSignal < 0) {
      ERR(theSignal);
    } else if (theSignal & SIGF_ABORT) {
      qprintf(("Automounter aborted!\n"));
      return;
    } else  if (theSignal & fileFolio.ff_Mounter.ffm_Signal) {
      DBUG0(("Scanning for DDFs\n"));
      MountAllFileSystems();
      DBUG0(("DDF scan complete\n"));
    } else {
      qprintf(("Automounter unknown signal 0x%X\n", theSignal));
    }
  }
}

int main(void)
{
  int res;

  DBUG0(("FS started\n"));

  res = FileSystemInit();

  DBUG0(("Back from FS init\n"));

  Automount();

  return res;
}
