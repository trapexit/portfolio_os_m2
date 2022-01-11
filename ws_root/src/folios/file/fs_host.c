/* @(#) fs_host.c 96/09/23 1.51 */

/*
  HostFileSystem.c - support code for Macintosh "remote" filesystems
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
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <file/directory.h>
#include <misc/event.h>
#include <string.h>


#undef DEBUG

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifdef BUILD_DEBUGGER

#undef FOLLOWON

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

static Item MountHost (Device *theDevice, uint32 blockOffset,
		       IOReq *rawRequest, ExtVolumeLabel *discLabel,
		       uint32 labelOffset, DeviceStatus *devStatus);
static Err DismountHost (FileSystem *fs);

extern Err BindHostFileSystem(FileSystemType *myType);
extern Err UnbindHostFileSystem(FileSystemType *myType);

extern Err LoadFSDriver(FileSystemType *fst);

extern void GiveDaemon(void *foo);
extern IOReq *StatusEndAction(IOReq *statusRequest);

extern FileSystem *CreateFileSystem (Device *theDevice,
				     uint32 blockOffset, 
				     ExtVolumeLabel *discLabel,
				     uint32 highLevelDiskSize,
				     uint32 rootAvatars,
				     FileSystemType *fst);

extern void LinkAndAnnounceFileSystem(FileSystem *fs);
extern FileSystem *FindFileSystem(const uchar *name);

static FileSystemType HostType =
{
  { NULL, NULL, 0, 0, 0,
      FSTYPE_NEEDS_LOAD + FSTYPE_UNLOADABLE,
      sizeof (FileSystemType),
      "host.fs"},                  /* initialize the node */
  0,                               /* fst_VolumeStructureVersion */
  0,                               /* fst_DeviceFamilyCode */
  0,                               /* fst_LoadError */
  0,                               /* fst_ModuleItem */
  LoadFSDriver,                    /* fst_Loader */
  NULL,                            /* fst_Unloader */
  MountHost,                       /* fst_Mount */
  NULL,                            /* fst_ActQue */
  DismountHost,                    /* fst_Dismount */
  NULL,                            /* fst_QueueRequest */
  NULL,                            /* fst_FirstTimeInit */
  NULL,                            /* fst_TimeSlice */
  NULL,                            /* fst_AbortIO */
  NULL,                            /* fst_CreateEntry */
  NULL,                            /* fst_DeleteEntry */
  NULL,                            /* fst_AllocSpace */
  NULL,                            /* fst_CloseFile */
  NULL,                            /* fst_CreateIOReq */
  NULL                             /* fst_DeleteIOReq */
};


static void Nuke(void *foo) {
  if (foo) {
    SuperDeleteItem(((ItemNode *)foo)->n_Item);
  }
}

Err InitHost(List *fileSystemTypes)
{
  AddTail(fileSystemTypes, (Node *) &HostType);
  return 0;
}

static Item MountHostFilesystem(Device *theDevice,
				int32 rootToken, char *name) {
  HostFS *fileSystem;
  HostToken *token;
  File *fsRoot = NULL;
  Item ioReqItem;
  IOReq *rawIOReq;
  Err err = 0;
/*
  Create a filesystem which resides on the device
*/
  DBUG(("Creating filesystem node for %s, root token %d\n", name, rootToken));
  fileSystem = (HostFS *)
    CreateFileSystem (theDevice, 0,
		      NULL, sizeof (HostFS), 1, &HostType);

  if (!fileSystem) {
    DBUG(("Could not create filesystem\n"));
    err = NOMEM;
    goto nuke;
  }
  ioReqItem = SuperCreateIOReq(NULL, 0, theDevice->dev.n_Item, 0);
  if (ioReqItem < 0)     {
    qprintf(("Error 0x%x creating ioreq\n", ioReqItem));
    ERR(ioReqItem);
    goto nuke;
  }
  rawIOReq = (struct IOReq *) LookupItem(ioReqItem);
  token = (HostToken *) SuperAllocMem(sizeof (HostToken), MEMTYPE_DMA |
				      MEMTYPE_FILL);
  if (!token) {
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
    goto nuke;
  }
  token->ht_TokenValue = rootToken;
  token->ht_TokenValid = TRUE;
  DBUG(("Initializing host device\n"));
  fileSystem->hfs_RawDeviceRequest = rawIOReq;
  InitList(&fileSystem->hfs_TokensToRelease, "List of HostTokens to release");
  fsRoot = fileSystem->hfs.fs_RootDirectory;
  fsRoot->fi_Flags |= FILE_SUPPORTS_DIRSCAN | FILE_SUPPORTS_ENTRY |
    FILE_USER_STORAGE_PLACE | FILE_IS_BLESSED;
  fsRoot->fi_FilesystemSpecificData = (uint32) token;
  fileSystem->hfs.fs_DeviceBlocksPerFilesystemBlock = 1;
  fileSystem->hfs.fs_Flags |= FILESYSTEM_IS_QUIESCENT;
  fileSystem->hfs.fs_VolumeFlags |= VF_BLESSED;
  strncpy(fileSystem->hfs.fs_FileSystemName, name, FILESYSTEM_MAX_NAME_LEN);
  strncpy(fsRoot->fi_FileName, name, FILESYSTEM_MAX_NAME_LEN);
/*
  All ready to go.
*/
  SuperInternalOpenItem(theDevice->dev.n_Item, NULL,
			fileFolio.ff_Daemon.ffd_Task);
  GiveDaemon(rawIOReq);
  GiveDaemon(fsRoot);
  GiveDaemon(fileSystem);
#ifdef SEMAPHORE_LMD
  GiveDaemon(LookupItem(fsRoot->fi_DirSema));
#endif
  LinkAndAnnounceFileSystem((FileSystem *) fileSystem);
  DBUG(("Mount complete\n"));
  return fileSystem->hfs.fs.n_Item;
 nuke:
#ifdef SEMAPHORE_LMD
  DBUG(("Releasing semaphore\n"));
  if (fsRoot) {
    Nuke(LookupItem(fsRoot->fi_DirSema));
  }
#endif
  DBUG(("Releasing root\n"));
  Nuke(fsRoot);
  DBUG(("Releasing filesystem\n"));
  Nuke(fileSystem);

  return err;
}

static Item MountHost (Device *theDevice, uint32 blockOffset,
		       IOReq *rawRequest, ExtVolumeLabel *discLabel,
		       uint32 labelOffset, DeviceStatus *devStatus)
{
  Err err;
  Item mountedFS = 0;
  int fsNum = 0;
  char name[32];

  TOUCH(blockOffset);
  TOUCH(labelOffset);

  if (discLabel || !(devStatus->ds_DeviceUsageFlags & DS_USAGE_HOSTFS)) {
    return 0;
  }

  DBUG(("Attempting mount-host on %s\n", theDevice->dev.n_Name));

  while (1) {
    DBUG(("Looking for filesystem %d\n", fsNum));
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_MOUNTFS;
    rawRequest->io_Info.ioi_Offset = fsNum;
    rawRequest->io_Info.ioi_Recv.iob_Buffer = name;
    rawRequest->io_Info.ioi_Recv.iob_Len = sizeof name;
    err = SuperInternalDoIO(rawRequest);
    if (err < 0) {
      DBUG(("HOSTFS_CMD_MOUNTFS returned error "));
#ifdef DEBUG
      ERR(err);
#endif
      break;
    }
/*
   FIXME - don't try to mount if there's already one of that name
*/
    err = MountHostFilesystem(theDevice, rawRequest->io_Info.ioi_CmdOptions,
			      name);
    if (err < 0) {
      break;
    }
    mountedFS = err;
    fsNum ++;
  }
  if (mountedFS) {
    SuperDeleteItem(rawRequest->io.n_Item);
    return mountedFS;
  } else {
    return err;
  }
}

static Err DismountHost (FileSystem *fs)
{
  HostFS *hfs = (HostFS *) fs;
  Err err;

  err = SuperInternalDeleteItem(hfs->hfs_RawDeviceRequest->io.n_Item);
  if (err < 0) {
    DBUG(("Delete of internal IOReq failed\n"));
  }
  return err;
}

#else

/* keep the compiler happy... */
extern int foo;

/* Provide a stub for the linker's exports */

Err InitHost(List *fileSystemTypes)
{
	TOUCH(fileSystemTypes);
  return 0;
}

#endif /* BUILD_DEBUGGER */
