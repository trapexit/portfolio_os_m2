/* @(#) FileUtils.c 96/07/17 1.5 */

/*
  FileUtils.c - contains routines which are needed by two or more of the
  dependent-layer filesystem drivers.

*/

#define SUPER

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/directory.h>
#include <file/discdata.h>
#include <stdlib.h>
#include <string.h>

#undef DEBUG
#undef DEBUG2

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

IOReq *StatusEndAction(IOReq *rawRequest)
{
  FileSystem *fs;
  int32 interrupts;
  fs = (FileSystem *) rawRequest->io_Info.ioi_UserData;
  fs->fs_DeviceBusy = FALSE;
  if (rawRequest->io_Actual != sizeof (DeviceStatus)) {
    DBUG(("UUGH!\n"));
  }
  interrupts = Disable();
  if (rawRequest->io_Error < 0) {
    qprintf(("/%s: ", fs->fs_FileSystemName));
    ERR(rawRequest->io_Error);
    if (fs->fs_Flags & FILESYSTEM_IS_QUIESCENT) {
      fs->fs_Flags |= (FILESYSTEM_IS_OFFLINE |
		       FILESYSTEM_WANTS_DISMOUNT);
    } else {
      fs->fs_Flags |= (FILESYSTEM_IS_OFFLINE |
		       FILESYSTEM_WANTS_QUIESCENT |
		       FILESYSTEM_WANTS_DISMOUNT);
    }
  } else {
    qprintf(("/%s isn't dead yet.\n", fs->fs_FileSystemName));
  }
  Enable(interrupts);
  SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
		      fileFolio.ff_Daemon.ffd_RescheduleSignal);
  return NULL;
}

/*
  returns 0   if status request could not be started,
  returns 1   if started successfully,
  returns err if an error occurred, and also flags filesystem as being
    offline.
*/

int32 StartStatusRequest(FileSystem *fs, IOReq *rawRequest,
			 IOReq *(*callback)(IOReq *))
{
  Err err;
  int32 interrupts;
  if (!fs->fs_DeviceStatusTemp) {
    fs->fs_DeviceStatusTemp =
      (DeviceStatus *) SuperAllocMem(sizeof (DeviceStatus), MEMTYPE_FILL);
    if (!fs->fs_DeviceStatusTemp) {
      fs->fs_DeviceBusy = FALSE;
      return 0;
    }
  }
  DBUG(("Issuing status request for /%s to 0x%0X\n",
	fs->fs_FileSystemName, fs->fs_DeviceStatusTemp));
  memset(&rawRequest->io_Info, 0, sizeof rawRequest->io_Info);
  rawRequest->io_Info.ioi_Command = CMD_STATUS;
  rawRequest->io_Info.ioi_Recv.iob_Buffer = fs->fs_DeviceStatusTemp;
  rawRequest->io_Info.ioi_Recv.iob_Len = sizeof (DeviceStatus);
  rawRequest->io_Info.ioi_UserData = (void *) fs;
  rawRequest->io_CallBack = callback;
  interrupts = Disable();
  fs->fs_Flags &= ~FILESYSTEM_WANTS_RECHECK;
  err = SuperinternalSendIO(rawRequest);
  if (err < 0) {
    ERR(err);
    fs->fs_Flags |= (FILESYSTEM_IS_OFFLINE |
		     FILESYSTEM_WANTS_QUIESCENT |
		     FILESYSTEM_WANTS_DISMOUNT);
    fs->fs_DeviceBusy = FALSE;
  }
  Enable(interrupts);
  return err;
}
