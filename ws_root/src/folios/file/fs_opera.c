/* @(#) fs_opera.c 96/09/23 1.46 */

/*
  OperaFileSystem.c - implements Opera-format hierarchical read-only file
  systems.
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

extern char *fsCacheBase;
extern int32 fsCacheBusy;

enum OperaChew {OperaSelect, OperaSetup, OperaStart, OperaDone};
enum OperaBusy {OperaIdle, OperaScheduling, OperaRunning};

static Item MountOpera (Device *theDevice, uint32 blockOffset,
			IOReq *rawRequest, ExtVolumeLabel *discLabel,
			uint32 labelOffset, DeviceStatus *devStatus);
static Err OperaActQue (FileSystem *fs, enum FSActQue);
static Err DismountOpera (FileSystem *fs);
static Err OperaDispatch(FileSystem *fs, IOReq *theRequest);
static void OperaSchedLoop (OperaFS *ofs);
static void OperaTimeslice (FileSystem *fs);
static enum OperaChew SelectOperaIO(OperaFS *theDevice);
static enum OperaChew SetupOperaIO (OperaFS *theDevice);
static enum OperaChew StartOperaIO (OperaFS *theDevice);
static IOReq *OperaEndAction(IOReq *theRequest);
static IOReq *CatapultEndAction(IOReq *theRequest);
static void AbortOperaIO(IOReq *userRequest);
static int32 OperaCacheSearch(FileIOReq *theRequest);
static int32 SearchOperaDirectoryPage(FileIOReq *theRequest,
				      IoCacheEntry *ce,
				      File *theParent);

static uchar HighestPriority(List *);

extern FileFolioTaskData *SetupFileFolioTaskData(void);
extern void InitFileSystemFromLabel(FileSystem *fs, ExtVolumeLabel *discLabel);
extern void CatDotted(char *string, uint32 num);
extern void GiveDaemon(void *foo);
extern void Nuke(void *foo);
extern IOReq *StatusEndAction(IOReq *statusRequest);
extern int32 StartStatusRequest(FileSystem *fs, IOReq *rawRequest,
				IOReq *(*callBack)(IOReq *));
extern OFile *OpenPath (File *startingDirectory, char *path);
extern FileSystem *CreateFileSystem (Device *theDevice,
				     uint32 blockOffset, ExtVolumeLabel *discLabel,
				     uint32 highLevelDiskSize,
				     uint32 rootAvatars,
				     FileSystemType *fst);
extern void LinkAndAnnounceFileSystem(FileSystem *fs);
extern FileSystem *FindFileSystem(const uchar *name);

extern IoCacheEntry *AddCacheEntry(OFile *theOpenFile, int32 blockNum,
				   void *buffer,
				   uint8 priority, uint32 format,
				   uint32 bytes);


static FileSystemType OperaType =
{
  {NULL, NULL, 0, 0, 0, 0,
  sizeof(FileSystemType),
  "opera.fs" },                   /* initialize the node */
  VOLUME_STRUCTURE_OPERA_READONLY, /* fst_VolumeStructureVersion */
  0,                               /* fst_DeviceFamilyCode */
  0,                               /* fst_LoadError */
  0,                               /* fst_ModuleItem */
  NULL,                            /* fst_Loader */
  NULL,                            /* fst_Unloader */
  MountOpera,                      /* fst_Mount */
  OperaActQue,                     /* fst_ActQue */
  DismountOpera,                   /* fst_Dismount */
  OperaDispatch,                   /* fst_QueueRequest */
  NULL,                            /* fst_FirstTimeInit */
  OperaTimeslice,                  /* fst_StartIO */
  AbortOperaIO,                    /* fst_AbortIO */
  NULL,                            /* fst_CreateEntry */
  NULL,                            /* fst_DeleteEntry */
  NULL,                            /* fst_AllocSpace */
  NULL,                            /* fst_CloseFile */
  NULL,                            /* fst_CreateIOReq */
  NULL                             /* fst_DeleteIOReq */
};


Err InitOpera(List *fileSystemTypes)
{
  AddTail(fileSystemTypes, (Node *) &OperaType);
  return 0;
}

static Item MountOpera (Device *theDevice, uint32 blockOffset,
			IOReq *rawRequest, ExtVolumeLabel *discLabel,
			uint32 labelOffset, DeviceStatus *devStatus)
{
  FileIOReq **optBuffer;
  OperaFS *fileSystem;
  File *fsRoot= NULL;
  IoCacheEntry *catapultPage;
  OFile *catapult;
  int32 avatarIndex;
  Err err = 0;
  MemMappableDeviceInfo mmdi;
  MapRangeRequest mrrq;
  MapRangeResponse mrrs;

  TOUCH(labelOffset);

  if (!discLabel) {
    return 0;
  }

  fileSystem = (OperaFS *) FindFileSystem(discLabel->dl_VolumeIdentifier);

  if (fileSystem) {
    if (fileSystem->ofs.fs_RootDirectory->fi_UniqueIdentifier ==
	discLabel->dl_VolumeUniqueIdentifier) {
      if (fileSystem->ofs.fs_Flags & FILESYSTEM_IS_OFFLINE) {
	DBUG0(("Remounting filesystem %s\n",
	       fileSystem->ofs.fs_FileSystemName));
/*
   Same name, same ID, and we know the old device went offline.
   OK, let's close the old device at this point and start paying
   attention to the new one.
*/
	SuperInternalDeleteItem(fileSystem->ofs_RawDeviceRequest->io.n_Item);
	fileSystem->ofs_RawDeviceRequest = rawRequest;
	SuperInternalOpenItem(theDevice->dev.n_Item, NULL,
			      fileFolio.ff_Daemon.ffd_Task);
	GiveDaemon(rawRequest);
	SuperInternalCloseItem(fileSystem->ofs.fs_RawDevice->dev.n_Item,
			       fileFolio.ff_Daemon.ffd_Task);
	fileSystem->ofs.fs_RawDevice = theDevice;
	fileSystem->ofs.fs_Flags &= ~(FILESYSTEM_IS_OFFLINE |
				      FILESYSTEM_WANTS_QUIESCENT |
				      FILESYSTEM_WANTS_DISMOUNT |
				      FILESYSTEM_WANTS_RECHECK);
	return fileSystem->ofs.fs.n_Item;
      }
      /*
	 Name and ID matches, which _should_ indicate that it's an
	 exactly identical volume.  Just to be safe, change the unique
	 ID to a runtime-guaranteed-unique value to ensure that the
	 filesystem cache does not accidentally confuse the two, in case
	 they really are different.
      */
      discLabel->dl_VolumeUniqueIdentifier = fileFolio.ff_NextUniqueID --;
    }
    /*
       If we get here, we have a second filesystem with the same name
       and [1] a different ID, or [2] the same ID as one which is still
       known to be online.  Might be a second copy of a CD, might be a
       second ROM.  Let it through without disturbing the existing
       filesystem;  it'll be renamed automatically.
    */
  }

  DBUG(("Creating optimization buffer\n"));
  optBuffer = (FileIOReq **) SuperAllocMem(DEVICE_SORT_COUNT * sizeof(FileIOReq *),
				       MEMTYPE_FILL);
  if (!optBuffer) {
    goto nuke;
  }
/*
  Create a filesystem which resides on the device
*/
  DBUG(("Creating filesystem node\n"));
  fileSystem = (OperaFS *)
    CreateFileSystem (theDevice, blockOffset,
		      discLabel, sizeof (OperaFS),
		      discLabel->dl_RootDirectoryLastAvatarIndex + 1,
		      &OperaType);
  if (!fileSystem) {
    DBUG(("Could not create filesystem\n"));
    err = NOMEM;
    goto nuke;
  }
  fileSystem->ofs_BlockSize  = devStatus->ds_DeviceBlockSize;
  fileSystem->ofs_BlockCount = devStatus->ds_DeviceBlockCount - blockOffset;
  fileSystem->ofs_RawDeviceBlockOffset = blockOffset;
  fileSystem->ofs_RawDeviceRequest = rawRequest;
  fileSystem->ofs_DeferredPriority = 0;
  fileSystem->ofs_RequestSort = optBuffer;
  fileSystem->ofs_RequestSortLimit = DEVICE_SORT_COUNT;
  fileSystem->ofs_NextBlockAvailable = 0;
  fileSystem->ofs_CatapultFile = (OFile *) NULL;
  fileSystem->ofs_CatapultPage = (IoCacheEntry *) NULL;
  fileSystem->ofs_CatapultPhase = CATAPULT_NONE;
  if (fileSystem->ofs_BlockSize == FILESYSTEM_DEFAULT_BLOCKSIZE) {
    DBUG(("Filesystem directory caching enabled\n"));
    fileSystem->ofs.fs_Flags = FILESYSTEM_IS_READONLY |
      FILESYSTEM_CACHEWORTHY;
  } else {
    fileSystem->ofs.fs_Flags = FILESYSTEM_IS_READONLY;
  }
  fsRoot = fileSystem->ofs.fs_RootDirectory;
  fsRoot->fi_Flags |= FILE_IS_READONLY | FILE_SUPPORTS_DIRSCAN |
    FILE_SUPPORTS_ENTRY;
  if (fileSystem->ofs.fs_Flags & FILESYSTEM_CACHEWORTHY) {
    fsRoot->fi_Flags |= FILE_BLOCKS_CACHED;
  }
  fileSystem->ofs.fs_DeviceBlocksPerFilesystemBlock =
    (int32) (fileSystem->ofs.fs_VolumeBlockSize / devStatus->ds_DeviceBlockSize);
  DBUG(("Volume is %d blocks of %d bytes\n",
	       fileSystem->ofs.fs_VolumeBlockCount,
	       fileSystem->ofs.fs_VolumeBlockSize));
  DBUG(("Root directory (%d blocks of %d bytes) at offset(s) ",
	       fsRoot->fi_BlockCount, fsRoot->fi_BlockSize));
  for (avatarIndex = (int32) fsRoot->fi_LastAvatarIndex;
       avatarIndex >= 0;
       avatarIndex --) {
    fsRoot->fi_AvatarList[avatarIndex] =
      discLabel->dl_RootDirectoryAvatarList[avatarIndex];
    DBUG((" %d", fsRoot->fi_AvatarList[avatarIndex]));
  }
  DBUG(("\n"));
/*
  Check to see if this puppy is on a memory-mappable device.  If so,
  map in the whole thing.
*/
  if (devStatus->ds_DeviceUsageFlags & DS_USAGE_STATIC_MAPPABLE) {
    memset(&mmdi, 0, sizeof mmdi);
    rawRequest->io_Info.ioi_Command = CMD_GETMAPINFO;
    rawRequest->io_Info.ioi_Recv.iob_Buffer = &mmdi;
    rawRequest->io_Info.ioi_Recv.iob_Len = sizeof mmdi;
    err = SuperInternalDoIO(rawRequest);
    if (err < 0) {
      DBUG(("Couldn't get memmap info for a mappable device!\n"));
      ERR(err);
    } else {
      memset(&mrrq, 0, sizeof mrrq);
      mrrq.mrr_BytesToMap = fileSystem->ofs.fs_VolumeBlockSize *
	fileSystem->ofs.fs_VolumeBlockCount;
      rawRequest->io_Info.ioi_Command = CMD_MAPRANGE;
      rawRequest->io_Info.ioi_Offset = blockOffset;
      rawRequest->io_Info.ioi_Send.iob_Buffer = &mrrq;
      rawRequest->io_Info.ioi_Send.iob_Len = sizeof mrrq;
      rawRequest->io_Info.ioi_Recv.iob_Buffer = &mrrs;
      rawRequest->io_Info.ioi_Recv.iob_Len = sizeof mrrs;
      err = SuperInternalDoIO(rawRequest);
      if (err < 0) {
	DBUG(("Couldn't map in a mappable device!\n"));
	ERR(err);
      } else {
	DBUG(("Mapped in filesystem at 0x%X\n", mrrs.mrr_MappedArea));
	fileSystem->ofs_FilesystemMemBase = mrrs.mrr_MappedArea;
	fileSystem->ofs.fs_XIPFlags = mmdi.mmdi_Permissions;
      }
    }
  } else {
    DBUG(("Not a memory-mappable device\n"));
  }
/*
  All ready to go.
*/
  SuperInternalOpenItem(theDevice->dev.n_Item, NULL,
			fileFolio.ff_Daemon.ffd_Task);
  GiveDaemon(rawRequest);
  GiveDaemon(fsRoot);
  GiveDaemon(fileSystem);
#ifdef SEMAPHORE_LMD
  GiveDaemon(LookupItem(fsRoot->fi_DirSema));
#endif
  LinkAndAnnounceFileSystem((FileSystem *) fileSystem);
  DBUG(("Looking for Catapult file\n"));
  catapult = OpenPath(fsRoot, "Catapult");
  if (catapult) {
    DBUG(("Hey, there's a catapult file!\n"));
    if (catapult->ofi_File->fi_FileSystemBlocksPerFileBlock != 1 ||
	catapult->ofi_File->fi_BlockSize > 2048) {
      DBUG(("Catapult file has weird blocking!  Can't use\n"));
      (void) SuperCloseItem(catapult->ofi.dev.n_Item);
      (void) SuperDeleteItem(catapult->ofi.dev.n_Item);
    } else {
      catapultPage = GetFreeCachePage(TRUE);
      if (catapultPage) {
	catapultPage->ioce_Filesystem = (FileSystem *) fileSystem;
	catapultPage->ioce_FileUniqueIdentifier =
	  catapult->ofi_File->fi_UniqueIdentifier;
	catapultPage->ioce_FileBlockOffset = 0;
	catapultPage->ioce_CacheFormat = CACHE_CATAPULT_INDEX;
	catapultPage->ioce_CacheFirstIndex = 0;
	catapultPage->ioce_CacheEntryCount = 0;
	catapultPage->ioce_CacheMiscValue = 0;
	catapultPage->ioce_CachedBlockSize = catapult->ofi_File->fi_BlockSize;
	catapultPage->ioce_PageState = CachePageLoading;
	fileSystem->ofs_CatapultFile = catapult;
	fileSystem->ofs_CatapultPage = catapultPage;
	fsCacheBusy ++;
	fileSystem->ofs_CatapultPhase = CATAPULT_READING;
	/* majority of IOReq still set up from label read */
	rawRequest->io_Info.ioi_Recv.iob_Buffer =
	  catapultPage->ioce_CachedBlock;
	rawRequest->io_Info.ioi_Recv.iob_Len =
	  catapult->ofi_File->fi_BlockSize;
	rawRequest->io_Info.ioi_Flags = 0;
	rawRequest->io_Info.ioi_Offset = (int32) blockOffset +
	  catapult->ofi_File->fi_AvatarList[0] *
	    fileSystem->ofs.fs_DeviceBlocksPerFilesystemBlock;
	rawRequest->io_Info.ioi_UserData = fileSystem;
	rawRequest->io_CallBack = CatapultEndAction;
	DBUG(("Starting read of catapult page 0\n"));
	fileSystem->ofs.fs_DeviceBusy = TRUE;
	fileSystem->ofs_NextBlockAvailable =
	  catapult->ofi_File->fi_AvatarList[0] + 1;
	err = SuperinternalSendIO(rawRequest);
	if (err < 0) {
	  DBUG(("Catapult SendIO error 0x%X\n", err));
	  ERR(err);
	}
      } else {
	DBUG(("Could not get cache page for catapult\n"));
	(void) SuperCloseItem(catapult->ofi.dev.n_Item);
	(void) SuperDeleteItem(catapult->ofi.dev.n_Item);
      }
    }
  } else {
    DBUG(("No catapult file\n"));
  }
  DBUG(("Mount complete\n"));
  return fileSystem->ofs.fs.n_Item;
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
  if (optBuffer) {
    DBUG(("Releasing opt buffer\n"));
    SuperFreeMem(optBuffer, DEVICE_SORT_COUNT * sizeof(FileIOReq *));
  }
  return err;
}

static void ShutDownCatapult(OperaFS *ofs)
{
  DBUG(("Shutting down catapult\n"));
  RelinquishCachePage(ofs->ofs_CatapultPage);
  ofs->ofs_CatapultPage = NULL;
  (void) SuperCloseItem(ofs->ofs_CatapultFile->ofi.dev.n_Item);
  (void) SuperDeleteItem(ofs->ofs_CatapultFile->ofi.dev.n_Item);
  ofs->ofs_CatapultFile = NULL;
  ofs->ofs_CatapultPhase = CATAPULT_NONE;
  DBUG(("TotCatapultStreamedHits is %d\n", ofs->ofs_TotCatapultStreamedHits));
  DBUG(("TotCatapultNonstreamedHits is %d\n", ofs->ofs_TotCatapultNonstreamedHits));
  DBUG(("TotCatapultSeeksAvoided is %d\n", ofs->ofs_TotCatapultSeeksAvoided));
  DBUG(("TotCatapultTimesEntered is %d\n", ofs->ofs_TotCatapultTimesEntered));
  DBUG(("TotCatapultDeclined is %d\n", ofs->ofs_TotCatapultDeclined));
  DBUG(("TotCatapultMisses is %d\n", ofs->ofs_TotCatapultMisses));
  DBUG(("TotCatapultNonstreamedMisses is %d\n", ofs->ofs_TotCatapultNonstreamedMisses));
}

static Err OperaActQue (FileSystem *fs, enum FSActQue function)
{
  OperaFS *odi;
  odi = (OperaFS *) fs;
  switch (function) {
  case QuiesceFS:
    if (odi->ofs_CatapultFile) {
      ShutDownCatapult(odi);
    }
    odi->ofs.fs_Flags = (odi->ofs.fs_Flags & ~FILESYSTEM_WANTS_QUIESCENT) |
      FILESYSTEM_IS_QUIESCENT;
    break;
  case ActivateFS:
    break;
  }
  return 0;
}

static Err DismountOpera (FileSystem *fs)
{
  Err err;
  OperaFS *odi;
  odi = (OperaFS *) fs;
  err = SuperInternalDeleteItem(odi->ofs_RawDeviceRequest->io.n_Item);
  if (err < 0) {
    DBUG(("Delete of internal IOReq failed\n"));
  }
  SuperFreeMem(odi->ofs_RequestSort, DEVICE_SORT_COUNT * sizeof (FileIOReq *));
  return err;
}

static Err OperaDispatch(FileSystem *fs, IOReq *theRequest)
{
  uint32 interrupts;
  File *theFile;
  OperaFS *ofs = (OperaFS *) fs;
  MemMappableDeviceInfo mmdi;
  MapRangeRequest *mrrq;
  MapRangeResponse mrrs;
  FileSystemStat *fsst;
  int32 len;
  uint32 blocksToMap;
  theFile = ((OFile *) theRequest->io_Dev)->ofi_File;

  DBUG(("Opera dispatch cmd 0x%X for %s\n",
	 theRequest->io_Info.ioi_Command, fs->fs.n_Name));
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
  case CMD_BLOCKREAD:
    if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_FSSTAT:
    fsst = (FileSystemStat *)theRequest->io_Info.ioi_Recv.iob_Buffer;
    fsst->fst_Used = theFile->fi_FileSystem->fs_VolumeBlockCount;
    fsst->fst_Free = fsst->fst_MaxFileSize = 0;
    fsst->fst_BitMap |= (FSSTAT_MAXFILESIZE | FSSTAT_FREE | FSSTAT_USED);
    return 1;
  case FILECMD_READDIR:
  case FILECMD_READENTRY:
  case FILECMD_OPENENTRY:
    DBUG2(("Trying fast cache search for command %d\n",
	  theRequest->io_Info.ioi_Command));
    if (OperaCacheSearch((FileIOReq *)theRequest)) {
      DBUG2(("Fast cache search completed!\n"));
      return 1;
    }
    DBUG2(("Fast cache search failed, start full search\n"));
    ((FileIOReq *)theRequest)->fio_NextDirBlockIndex = 0;
    ((FileIOReq *)theRequest)->fio_NextDirEntryIndex = 1;
    AgeCache();
    break;
  case CMD_GETMAPINFO:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    memset(&mmdi, 0, sizeof mmdi);
    if (ofs->ofs_FilesystemMemBase != NULL) {
      mmdi.mmdi_Flags = MM_MAPPABLE | MM_READABLE | MM_EXECUTABLE;
      mmdi.mmdi_MaxMappableBlocks = mmdi.mmdi_CurBlocksMapped =
	theFile->fi_BlockCount;
      mmdi.mmdi_Permissions = ofs->ofs.fs_XIPFlags;
    }
    len = theRequest->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof mmdi) {
      len = sizeof mmdi;
    }
    memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer, &mmdi, len);
    theRequest->io_Actual = len;
    return 1;
  case CMD_MAPRANGE:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL ||
	theRequest->io_Info.ioi_Send.iob_Buffer == NULL ||
	theRequest->io_Info.ioi_Send.iob_Len < sizeof (MapRangeRequest)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    if (ofs->ofs_FilesystemMemBase == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
    }
    mrrq = (MapRangeRequest *) theRequest->io_Info.ioi_Send.iob_Buffer;
    blocksToMap = ((mrrq->mrr_BytesToMap + theFile->fi_BlockSize - 1) /
		   theFile->fi_BlockSize) * theFile->fi_BlockSize;
    if (theRequest->io_Info.ioi_Offset < 0 ||
	theRequest->io_Info.ioi_Offset+blocksToMap > theFile->fi_BlockCount ||
#ifdef BUILD_PARANOIA
	 blocksToMap <= 0 ||
#endif
	(mrrq->mrr_Flags & (MM_WRITABLE|MM_EXCLUSIVE))) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    memset(&mrrs, 0, sizeof mrrs);
    mrrs.mrr_MappedArea = ((char *) ofs->ofs_FilesystemMemBase) +
      ofs->ofs.fs_VolumeBlockSize * theFile->fi_AvatarList[0];
    len = theRequest->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof mrrs) {
      len = sizeof mrrs;
    }
    memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer, &mrrs, len);
    theRequest->io_Actual = len;
    return 1;
  case CMD_UNMAPRANGE:
    return 1;
  case CMD_GETICON:
    break;
  case FILECMD_SETFLAGS:
    theFile->fi_Flags = (theFile->fi_Flags & ~FILEFLAGS_SETTABLE) |
      (theRequest->io_Info.ioi_CmdOptions & FILEFLAGS_SETTABLE);
    return 1;
  case CMD_WRITE:
  case CMD_BLOCKWRITE:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_SETTYPE:
  case FILECMD_ADDDIR:
  case FILECMD_DELETEDIR:
  case FILECMD_SETVERSION:
  case FILECMD_SETBLOCKSIZE:
  case FILECMD_SETDATE:
    return FILE_ERR_READONLY;

  default:
    DBUG(("I/O command %d rejected\n", theRequest->io_Info.ioi_Command));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  interrupts = Disable();
  AddTail(&ofs->ofs.fs_RequestsToDo, (Node *) theRequest);
  if (theRequest->io.n_Priority > ofs->ofs.fs_RequestPriority) {
    ofs->ofs.fs_RequestPriority = theRequest->io.n_Priority;
  }
  if (ofs->ofs.fs_DeviceBusy != OperaIdle) {
    if (ofs->ofs.fs_RequestPriority >
	ofs->ofs.fs_RunningPriority) {
      ofs->ofs.fs.n_Flags |= DEVICE_BOINK; /* force defer/resched */
    }
  }
  theRequest->io_Flags &= ~IO_QUICK;
  switch (ofs->ofs.fs_DeviceBusy) {
  case OperaIdle:
    if (!(theRequest->io_Flags & IO_INTERNAL)) {
      Enable(interrupts);
      OperaTimeslice((FileSystem *) ofs);
      interrupts = Disable();
      break;
    }
    /* fall through for IO_INTERNAL requests */
  case OperaScheduling:
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_QueuedSignal);
    break;
  case OperaRunning:
    break;
  }
  Enable(interrupts);
  return 0;
}

/*
   Enter this routine after having locked the DeviceBusy state to
   OperaScheduling, and (if necessary) reenabled interrupts.  After exiting,
   disable, check to see if still scheduling, and if so set to idle.
*/

static void OperaSchedLoop(OperaFS *ofs)
{
  enum OperaChew chew;
  chew = OperaSelect;
  while (ofs->ofs.fs_DeviceBusy == OperaScheduling && chew != OperaDone) {
    switch (chew) {
    case OperaSelect:
      chew = SelectOperaIO(ofs);
      break;
    case OperaSetup:
      chew = SetupOperaIO(ofs);
      break;
    case OperaStart:
      chew = StartOperaIO(ofs);
      break;
    }
  }
  return;
}

static void OperaTimeslice(FileSystem *fs)
{
  OperaFS *ofs = (OperaFS *) fs;
  int32 interrupts;
  interrupts = Disable();
  if (ofs->ofs.fs_DeviceBusy != OperaIdle) {
    Enable(interrupts);
    return;
  }
  ofs->ofs.fs_DeviceBusy = OperaScheduling;
  Enable(interrupts);
  OperaSchedLoop(ofs);
  interrupts = Disable();
  if (ofs->ofs.fs_DeviceBusy == OperaScheduling) {
    ofs->ofs.fs_DeviceBusy = OperaIdle;
    Enable(interrupts);
    if (ofs->ofs.fs_Flags & FILESYSTEM_WANTS_RECHECK) {
      ofs->ofs.fs_DeviceBusy = OperaRunning;
      (void) StartStatusRequest((FileSystem *) ofs,
				ofs->ofs_RawDeviceRequest,
				StatusEndAction);
    } else if (ofs->ofs.fs_DeviceStatusTemp) {
      DBUG(("Releasing temp buffer for /%s at 0x%X\n",
	    ofs->ofs.fs_FileSystemName, ofs->ofs.fs_DeviceStatusTemp));
      SuperFreeMem(ofs->ofs.fs_DeviceStatusTemp, sizeof (DeviceStatus));
      ofs->ofs.fs_DeviceStatusTemp = NULL;
    }
  } else {
    Enable(interrupts);
  }
  return;
}

/*
  The SelectOperaIO routine is not reentrant for any one device (although
  it is reentrant across multiple devices).  If interrupted, the
  interrupt routine must not call ScheduleIo either directly or
  indirectly.  If it looks as if the "no SendIO calls at interrupt
  time" rule is ever to be relaxed, all calls to this routine should
  be guarded with disable-interrupt/enable-interrupt pairs.
*/

static enum OperaChew SelectOperaIO(OperaFS *ofs)
{
  FileIOReq *theRequest, *nextRequest;
  IOReq *rawRequest;
  int32 numRequests, topPriority;
  uint32 requestLimit;
  int32 theCommand;
  CatapultPage *catapultPage;
#ifdef NOTDEF
  File *catapultFile;
#endif
  List *scheduleList;
  if (!IsEmptyList(&ofs->ofs.fs_RequestsRunning)) {
    DBUG(("Scheduler found a request still running, nothing to do\n"));
    return OperaSetup;
  }
  rawRequest = ofs->ofs_RawDeviceRequest;
  switch (ofs->ofs_CatapultPhase) {
  case CATAPULT_NONE:
  case CATAPULT_MUST_SHUT_DOWN:
    DBUG(("No catapult, or must shut down\n"));
    break;
  case CATAPULT_MUST_VERIFY:
    DBUG(("Got end-action on catapult file index read\n"));
    fsCacheBusy --; /* this is never changed at interrupt time */
    catapultPage = (CatapultPage *) rawRequest->io_Info.ioi_Recv.iob_Buffer;
    if (rawRequest->io_Error < 0 ||
	rawRequest->io_Actual < rawRequest->io_Info.ioi_Recv.iob_Len ||
	catapultPage->cp_MBZ != 0 ||
	catapultPage->cp_Fingerprint != FILE_TYPE_CATAPULT) {
      DBUG(("Catapult page not good\n"));
      DBUG(("  raw medium offset %d\n", rawRequest->io_Info.ioi_Offset));
      DBUG(("  io_Error = 0x%X ", rawRequest->io_Error));
      ERR(rawRequest->io_Error);
      DBUG(("  io_Actual %d, wanted %d\n",
	     rawRequest->io_Actual, rawRequest->io_Info.ioi_Recv.iob_Len));
      DBUG(("  mbz field %d\n", catapultPage->cp_MBZ));
      DBUG(("  fingerprint 0x%X, wanted 0x%X\n",
	     catapultPage->cp_Fingerprint, FILE_TYPE_CATAPULT));
      ofs->ofs_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
      ofs->ofs_CatapultPage->ioce_PageState = CachePageInvalid;
    } else {
      DBUG(("Catapult page validated\n"));
      ofs->ofs_CatapultPhase = CATAPULT_AVAILABLE;
      ofs->ofs_CatapultNextIndex = 0;
      ofs->ofs_CatapultPage->ioce_PageState = CachePageValid;
    }
    break;
  case CATAPULT_READING:
    DBUG(("Catapult read still in progress\n"));
    break;
  default:
    DBUG(("Catapult phase %d\n", ofs->ofs_CatapultPhase));
#ifdef NOTDEF
/*
   In the new cache scheme, the Catapult cache page has a non-zero use
   count for as long as the filesystem is using it.  Therefore, it cannot
   be preempted, and this condition need no longer be tested for.
*/
    catapultFile = ofs->ofs_CatapultFile->ofi_File;
    if (!fsCacheBase /* cache killed */ ||
	ofs->ofs_CatapultPage->ioce_CacheFormat != CACHE_CATAPULT_INDEX ||
	ofs->ofs_CatapultPage->ioce_Filesystem !=
	 catapultFile->fi_FileSystem ||
	ofs->ofs_CatapultPage->ioce_FileUniqueIdentifier !=
	 catapultFile->fi_UniqueIdentifier) {
      DBUG(("Catapult page preempted\n"));
      ofs->ofs_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
    }
#endif
    break;
  }
  if (ofs->ofs_CatapultPhase == CATAPULT_MUST_SHUT_DOWN) {
    ShutDownCatapult(ofs);
  }
  theCommand = -1;
  ofs->ofs.fs_RequestPriority =
    HighestPriority(&ofs->ofs.fs_RequestsToDo);
  ofs->ofs_DeferredPriority =
    HighestPriority(&ofs->ofs.fs_RequestsDeferred);
  DBUG(("Request priority %d, deferred priority %d\n",
	       ofs->ofs.fs_RequestPriority,
	       ofs->ofs_DeferredPriority));
  if (ISEMPTYLIST(&ofs->ofs.fs_RequestsDeferred) ||
      ofs->ofs.fs_RequestPriority > ofs->ofs_DeferredPriority) {
    scheduleList = &ofs->ofs.fs_RequestsToDo;
    topPriority = ofs->ofs.fs_RequestPriority;
    DBUG(("Taking to-do list\n"));
  } else {
    scheduleList = &ofs->ofs.fs_RequestsDeferred;
    topPriority = ofs->ofs_DeferredPriority;
    DBUG(("Taking deferred-priority list\n"));
  }
  if (ISEMPTYLIST(scheduleList)) {
    DBUG(("Nothing to schedule\n"));
    return OperaDone;
  }
  theRequest = (FileIOReq *) FIRSTNODE(scheduleList);
  numRequests = 0;
  requestLimit = ofs->ofs_RequestSortLimit;
  while (numRequests < requestLimit &&
	 ISNODE(scheduleList, theRequest)) {
    nextRequest = (FileIOReq *) NEXTNODE(theRequest);
    DBUG(("Examining request %x\n", theRequest));
    if (theRequest->fio.io.n_Priority == topPriority &&
	(theCommand < 0 ||
	 theCommand == theRequest->fio.io_Info.ioi_Command)) {
      RemNode((Node *) theRequest);
      AddTail(&ofs->ofs.fs_RequestsRunning, (Node *) theRequest);
      theCommand = theRequest->fio.io_Info.ioi_Command;
      numRequests ++;
      DBUG(("Took request %x\n", theRequest));
      if ((theCommand != CMD_READ && theCommand != CMD_BLOCKREAD) ||
	  ofs->ofs_CatapultPhase == CATAPULT_AVAILABLE) {
	break; /* take only one at a time while catapulting */
      }
    }
    theRequest = nextRequest;
  }
  DBUG(("%d requests picked off\n", numRequests));
  return OperaSetup;
}

static enum OperaChew SetupOperaIO(OperaFS *ofs)
{
  FileIOReq **requests;
  FileIOReq *theRequest, *nextRequest;
  IOReq *rawRequest;
  File *theFile;
  IoCacheEntry *ce;
  void *block;
  enum CacheState state;
  int32 numRequests, topPriority;
  uint32 requestLimit;
  int32 delta, leastDelta = 0, avatar, whereNow;
  int32 streaming;
  int32 nextAbove, nextBelow, phase, passes;
  int32 reqIndex, reqLimit;
  uint32 lowBlock, highBlock;
  uint32 absoluteBlock = 0, absoluteOffset;
  int32 avatarIndex, closestAvatar = 0;
  int32 accessesFile;
  int32 flawLevel, leastFlawLevel, i, j;
  int32 devBlocksPerFilesystemBlock;
  int32 fileSystemBlocksPerFileBlock;
  SchedulerSweepDirection sweep;
  requests = ofs->ofs_RequestSort;
  requestLimit = ofs->ofs_RequestSortLimit;
  rawRequest = ofs->ofs_RawDeviceRequest;
  lowBlock = 0x7FFFFFFF /* BIGGEST_POSITIVE_INTEGER */;
  highBlock = 0;
  whereNow = ofs->ofs_NextBlockAvailable;
  DBUG(("SetupOpera\n"));
  DBUG(("Scheduler: next block under arm is %d\n", whereNow));
  numRequests = 0;
  topPriority = 0;
  accessesFile = TRUE;
  while (numRequests < requestLimit &&
	 NULL != (theRequest =
	  (FileIOReq *) RemHead(&ofs->ofs.fs_RequestsRunning))) {
    DBUG(("Checking request %d\n", i));
    requests[numRequests] = theRequest;
    topPriority = theRequest->fio.io.n_Priority;
    numRequests ++;
    theFile = ((OFile *) theRequest->fio.io_Dev)->ofi_File;
    switch (theRequest->fio.io_Info.ioi_Command) {
    default:
      accessesFile = FALSE;
      absoluteOffset = 0;
      closestAvatar = 0;
      break;
    case CMD_READ:
    case CMD_BLOCKREAD:
      theRequest->fio_BlockCount = theRequest->fio.io_Info.ioi_Recv.iob_Len /
	theFile->fi_BlockSize;
      absoluteOffset = theRequest->fio.io_Info.ioi_Offset;
      break;
    case FILECMD_READDIR:
    case FILECMD_READENTRY:
    case FILECMD_OPENENTRY:
      while (1) {
	DBUG(("Cache search block %d\n", theRequest->fio_NextDirBlockIndex));
	state = FindBlockInCache(theFile, theRequest->fio_NextDirBlockIndex,
				 FALSE, &ce, &block);
	if (state == CachePageValid) {
	  DBUG(("Found page in cache, searching it\n"));
	  if (SearchOperaDirectoryPage(theRequest, ce, theFile)) {
	    DBUG(("Search successful\n"));
	    RelinquishCachePage(ce);
	    SuperCompleteIO((IOReq *) theRequest);
	    return OperaSelect;
	  }
	  DBUG(("Search not successful\n"));
	  theRequest->fio_NextDirBlockIndex += ce->ioce_CachedBlockSize /
	    theFile->fi_BlockSize;
	  theRequest->fio_NextDirEntryIndex = ce->ioce_CacheFirstIndex +
	    ce->ioce_CacheEntryCount;
	  RelinquishCachePage(ce);
	} else {
	  DBUG(("Did not find page in cache\n"));
	  if (theRequest->fio_NextDirBlockIndex >= theFile->fi_BlockCount) {
	    DBUG(("No wonder - past end of file!\n"));
	    theRequest->fio.io_Error =
	      MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
	    SuperCompleteIO((IOReq *) theRequest);
	    return OperaSelect;
	  }
	  DBUG(("Asking for a free cache page\n"));
	  ce = GetFreeCachePage(FALSE);
	  if (!ce) {
	    DBUG(("Failed, try again later\n"));
	    AddHead(&ofs->ofs.fs_RequestsRunning, (Node *) theRequest);
	    return OperaDone; /* come back later and try again */
	  }
	  DBUG(("Succeeded in getting a free page\n"));
	  theRequest->fio_CachePage = ce;
	  absoluteOffset = theRequest->fio_NextDirBlockIndex;
	  ce->ioce_Filesystem = theFile->fi_FileSystem;
	  ce->ioce_FileUniqueIdentifier = theFile->fi_UniqueIdentifier;
	  ce->ioce_FileBlockOffset = absoluteOffset;
	  ce->ioce_CacheFormat = CACHE_OPERA_DIRECTORY;
	  ce->ioce_CacheFirstIndex = theRequest->fio_NextDirEntryIndex;
	  ce->ioce_CacheEntryCount = 0;
	  ce->ioce_CacheMiscValue = 0;
	  ce->ioce_CachedBlockSize = 0;
	  ce->ioce_PageState = CachePageLoading;
	  i = 2048 / theFile->fi_BlockSize;
	  if (i > theFile->fi_BlockCount - absoluteOffset) {
	    i = theFile->fi_BlockCount - absoluteOffset;
	  }
	  theRequest->fio_BlockCount = i;
	  DBUG(("Will read %d block(s), index %d\n", i, absoluteOffset));
	  SetCachePagePriority(ce, CACHE_PRIO_MISS);
	  break;
	}
      }
    }
    if (accessesFile) {
      leastFlawLevel = DRIVER_FLAW_MASK + 1;
      avatarIndex = (int32) theFile->fi_LastAvatarIndex;
      theRequest->fio_BlockBurst = theRequest->fio_BlockCount;
      devBlocksPerFilesystemBlock =
	theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
      fileSystemBlocksPerFileBlock =
	theFile->fi_FileSystemBlocksPerFileBlock;
      do {
	avatar = theFile->fi_AvatarList[avatarIndex];
	flawLevel = (int32) ((avatar >> DRIVER_FLAW_SHIFT) & DRIVER_FLAW_MASK);
	avatar = (absoluteOffset * fileSystemBlocksPerFileBlock +
		  (avatar & DRIVER_BLOCK_MASK)) * devBlocksPerFilesystemBlock;
	if (flawLevel < leastFlawLevel) {
	  leastFlawLevel = flawLevel;
	  closestAvatar = avatarIndex;
	  absoluteBlock = avatar;
	  leastDelta = avatar - whereNow;
	  if (leastDelta < 0) {
	    leastDelta = - leastDelta;
	  }
	} else if (flawLevel == leastFlawLevel) {
	  delta = avatar - whereNow;
	  if (delta < 0) {
	    delta = - delta;
	  }
	  if (delta < leastDelta) {
	    leastDelta = delta;
	    closestAvatar = avatarIndex;
	    absoluteBlock = avatar;
	  }
	}
      } while (--avatarIndex >= 0);
      theRequest->fio_DevBlocksPerFileBlock = devBlocksPerFilesystemBlock;
      if (leastDelta == 0) {
	streaming = TRUE;
      } else {
	streaming = FALSE;
      }
      if (ofs->ofs_CatapultPhase == CATAPULT_AVAILABLE) {
	int32 cIndex;
	int32 cCount;
	int32 cIncr, cSign;
	CatapultPage *page;
	File *catapult;
	int32 indexIntoRun;
	int32 fsBlocksNeeded;
	int32 fileFsBlockOffset;
	int32 catapultBase, catapultBlocks;
	int32 err;
	int32 hitRun, hitExactly, usedHit, usedIndex;
	DBUG(("Attempting catapult scan, least delta is %d\n", leastDelta));
	page = (CatapultPage *) ofs->ofs_CatapultPage->ioce_CachedBlock;
	cIndex = ofs->ofs_CatapultNextIndex;
	cIncr = 1;
	cSign = 1;
	indexIntoRun = usedIndex = 0;
	cCount = page->cp_Entries;
	hitRun = hitExactly = usedHit = FALSE;
	fsBlocksNeeded = theRequest->fio_BlockBurst *
	  fileSystemBlocksPerFileBlock;
	fileFsBlockOffset = absoluteOffset * fileSystemBlocksPerFileBlock;
	catapultBase = ofs->ofs_CatapultFile->ofi_File->fi_AvatarList[0];
	catapultBlocks = ofs->ofs_CatapultFile->ofi_File->fi_BlockCount;
	DBUG(("Need %d filesystem blocks, from %d fs blocks into file\n",
	      fsBlocksNeeded, fileFsBlockOffset));
	DBUG(("Page has %d entries\n", cCount));
	while (cCount > 0 && !hitExactly) {
	  if (cIndex >= 0 && cIndex <= page->cp_Entries) {
	    cCount --;
	    DBUG(("Examining index %d id 0x%X", cIndex,
		  page->cpe[cIndex].cpe_FileIdentifier));
	    DBUG((" file-offset %d run length %d catapult offset %d\n",
		  page->cpe[cIndex].cpe_FileBlockOffset,
		  page->cpe[cIndex].cpe_RunLength,
		  page->cpe[cIndex].cpe_RunOffset));
	    if (page->cpe[cIndex].cpe_FileIdentifier ==
		theFile->fi_UniqueIdentifier) {
	      DBUG(("  It's the right file\n"));
	      indexIntoRun = fileFsBlockOffset -
		page->cpe[cIndex].cpe_FileBlockOffset;
	      if (indexIntoRun >= 0 &&
		  indexIntoRun + fsBlocksNeeded <= page->cpe[cIndex].cpe_RunLength) {
		/*
		   Following calcs assume catapult file blocksize is same as
		   filesystem block size... this is checked in the mount/open code.
		   */
		DBUG(("  It's a suitable run."));
		avatar = catapultBase +
		  page->cpe[cIndex].cpe_RunOffset + indexIntoRun;
		DBUG(("  Its location in the filesystem is block %d\n", avatar));
		if (!hitRun && !streaming) {
		  ofs->ofs_CatapultMisses = 0;
		  ofs->ofs_CatapultHits ++;
		  DBUG(("Successive hit count is %d\n", ofs->ofs_CatapultHits));
		  hitRun = TRUE;
		}
		delta = avatar - whereNow;
		DBUG(("  Offset from arm position is %d blocks\n", delta));
		if (delta < 0) {
		  delta = -delta / ofs->ofs_CatapultHits + 8 /* discourage back-seeks */;
		} else if (delta == 0) {
		  hitExactly = TRUE;
		} else {
		  delta = (delta / ofs->ofs_CatapultHits) + 1;
		}
		DBUG(("  Weighted delta is %d\n", delta));
		if (delta < leastDelta) {
		  DBUG(("  Use it!\n"));
		  leastDelta = delta;
		  closestAvatar = -1; /* flag as being from catapult file */
		  absoluteBlock = avatar;
		  usedHit = TRUE;
		  usedIndex = cIndex;
		}
	      }
	    }
	  }
	  cIndex += cIncr * cSign;
	  cIncr ++;
	  cSign = -cSign;
	}
	if (usedHit) {
	  ofs->ofs_CatapultNextIndex = usedIndex;
#ifdef BUILD_PARANOIA
	  if (hitExactly) {
	    ofs->ofs_TotCatapultStreamedHits ++;
	    if (indexIntoRun == 0) {
	      ofs->ofs_TotCatapultSeeksAvoided ++;
	    }
	  } else {
	    ofs->ofs_TotCatapultNonstreamedHits ++;
	  }
	  if (whereNow < catapultBase ||
	      whereNow >= catapultBase + catapultBlocks) {
	    ofs->ofs_TotCatapultTimesEntered ++;
	  }
#else
	TOUCH(catapultBlocks);
	TOUCH(indexIntoRun);
#endif
	} else if (hitRun) {
#ifdef BUILD_PARANOIA
	  DBUG(("Catapult declined\n"));
	  ofs->ofs_TotCatapultDeclined ++;
#endif
	} else {
	  DBUG(("Catapult missed %s, %d+%d\n", theFile->fi_FileName,
		fileFsBlockOffset, fsBlocksNeeded));
	  ofs->ofs_CatapultHits = 0;
	  ofs->ofs_TotCatapultMisses ++;
	  if (!streaming) {
	    ofs->ofs_CatapultMisses ++;
	    ofs->ofs_TotCatapultNonstreamedMisses ++;
	    DBUG(("Miss count is %d\n", ofs->ofs_CatapultMisses));
	    if (ofs->ofs_CatapultMisses > page->cp_Entries -
		ofs->ofs_CatapultNextIndex) {
	      DBUG(("Page exhausted\n"));
	      if (page->cp_NextPage < 0 ||
		  ofs->ofs_CatapultMisses > (page->cp_Entries >> 1) + 1) {
		DBUG(("Exhausted or stale catapult\n"));
		ofs->ofs_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
	      } else {
		DBUG(("Defer I/O, load next catapult page\n"));
		RemNode((Node *) theRequest);
		AddHead(&ofs->ofs.fs_RequestsDeferred, (Node *) theRequest);
		fsCacheBusy ++;
		catapult = ofs->ofs_CatapultFile->ofi_File;
		ofs->ofs_CatapultPhase = CATAPULT_READING;
		rawRequest->io_Info.ioi_Command = CMD_BLOCKREAD;
		rawRequest->io_Info.ioi_Recv.iob_Buffer = page;
		rawRequest->io_Info.ioi_Recv.iob_Len =
		  catapult->fi_BlockSize;
		rawRequest->io_Info.ioi_Flags = 0;
		rawRequest->io_Info.ioi_Offset =
		  ofs->ofs_RawDeviceBlockOffset +
		    (catapult->fi_AvatarList[0] + page->cp_NextPage) *
		      catapult->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
		rawRequest->io_Info.ioi_UserData = catapult->fi_FileSystem;
		rawRequest->io_CallBack = CatapultEndAction;
		DBUG(("Starting read of catapult page %d\n",
		      page->cp_NextPage));
		ofs->ofs.fs_DeviceBusy = OperaRunning;
		ofs->ofs_NextBlockAvailable =
		  catapult->fi_AvatarList[0] + page->cp_NextPage + 1;
		err = SuperinternalSendIO(rawRequest);
		if (err < 0) {
		  DBUG(("Catapult SendIO error 0x%X\n", err));
		  ERR(err);
		}
		return OperaDone;
	      }
	    }
	  }
	}
      }
    }
    theRequest->fio_AvatarIndex = closestAvatar;
    theRequest->fio_AbsoluteBlockNumber = absoluteBlock;
    if (absoluteBlock < lowBlock) {
      lowBlock = absoluteBlock;
    }
    if (absoluteBlock > highBlock) {
      highBlock = absoluteBlock;
    }
  }
  if ((whereNow - lowBlock) < (highBlock - whereNow)) {
    sweep = BottomIsCloser;
  } else {
    sweep = TopIsCloser;
  }
/*
  N.B. the following loop may appear to take "i" one iteration too far.
  "Trust me, I know what I'm doing."

  Yes, it's a double-loop exchange sort.  Should this code ever turn out
  to be of significant CPU impact, it should be rewritten as a quickersort
  or a heapsort or something else with O(n log n) expected-time behavior.

  First - sort into ascending block order.
*/
  DBUG(("Sorting requests\n"));
  nextBelow = -1;
  nextAbove = requestLimit;
  for (i = 0; i < numRequests; i++) {
    theRequest = requests[i];
    for (j = i+1; j < numRequests; j++) {
      nextRequest = requests[j];
      if (theRequest->fio_AbsoluteBlockNumber >
	  nextRequest->fio_AbsoluteBlockNumber) {
	requests[j] = theRequest;
	requests[i] = theRequest = nextRequest;
      }
    }
    if (theRequest->fio_AbsoluteBlockNumber < whereNow) {
      nextBelow = i;
    } else if (nextAbove == requestLimit) {
      nextAbove = i;
    }
  }
/*
  Next - reverse the order of the blocks lying below the current head
  position, so that they'll be handled in a downwards sweep
*/
  DBUG(("Reversing requests below waterline\n"));
  reqIndex = 0;
  reqLimit = nextBelow;
  while (reqIndex < reqLimit) {
    theRequest = requests[reqIndex];
    requests[reqIndex] = requests[reqLimit];
    requests[reqLimit] = theRequest;
    reqIndex ++;
    reqLimit --;
  }
  DBUG(("Queueing requests\n"));
/*
  Run through the two segments of the table and queue up the requests.
*/
  if (sweep == BottomIsCloser) {
    phase = 0;
  } else {
    phase = 1;
  }
  passes = 2;
  do {
    switch (phase) {
    case 0: /* Do the requests lying below the head position */
      DBUG(("Doing requests below waterline\n"));
      reqIndex = 0;
      reqLimit = nextBelow;
      phase = 1;
      break;
    case 1: /* Do the requests lying above the head position */
      DBUG(("Doing requests above waterline\n"));
      reqIndex = nextAbove;
      reqLimit = (int32) numRequests - 1;
      phase = 0;
      break;
    }
    while (reqIndex <= reqLimit) {
      DBUG(("Queued request at %x\n", requests[reqIndex]));
      AddTail(&ofs->ofs.fs_RequestsRunning,
	      (Node *) requests[reqIndex]);
      reqIndex++;
    }
  } while (--passes);
  ofs->ofs.fs_RunningPriority = (uchar) topPriority;
  ofs->ofs.fs_RequestPriority =
    HighestPriority(&ofs->ofs.fs_RequestsToDo);
  ofs->ofs_DeferredPriority =
    HighestPriority(&ofs->ofs.fs_RequestsDeferred);
  DBUG(("I/O scheduling complete\n"));
  return OperaStart;
}

static enum OperaChew StartOperaIO (OperaFS *ofs)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  File *theFile;
  int32 err;
  IoCacheEntry *ce;
  ofs->ofs.fs_DeviceBusy = OperaRunning;
  rawRequest = ofs->ofs_RawDeviceRequest;
 doit:
  if (ISEMPTYLIST(&ofs->ofs.fs_RequestsRunning)) {
    DBUG(("No I/O to start, bailing out\n"));
    ofs->ofs.fs_DeviceBusy = OperaScheduling;
    return OperaSelect;
  }
  theRequest = (FileIOReq *) FIRSTNODE(&ofs->ofs.fs_RequestsRunning);
  DBUG(("Starting up I/O request %x\n", theRequest));
  theFile = ((OFile *) theRequest->fio.io_Dev)->ofi_File;
  ce = theRequest->fio_CachePage;
  if (theFile->fi_FileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE) {
    DBUG(("Filesystem offline, killing request\n"));
    RemNode((Node *) theRequest);
    theRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
    if (ce) {
      ce->ioce_PageState = CachePageInvalid;
      RelinquishCachePage(ce);
      theRequest->fio_CachePage = NULL;
    }
    SuperCompleteIO((IOReq *) theRequest);
    goto doit;
  }
  rawRequest->io_CallBack = OperaEndAction;
  rawRequest->io_Info = theRequest->fio.io_Info;
  /***
    Pick up the absolute block number in the request, and use it for
    this transfer.
   ***/
  rawRequest->io_Info.ioi_Offset =
    (int32) theRequest->fio_AbsoluteBlockNumber +
      ofs->ofs_RawDeviceBlockOffset;
  rawRequest->io_Info.ioi_UserData = theRequest;
  rawRequest->io_Info.ioi_Flags = 0;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
  switch (theRequest->fio.io_Info.ioi_Command) {
  case CMD_GETICON:
    rawRequest->io_Info.ioi_Command = CMD_GETICON;
    break;
  default:
    rawRequest->io_Info.ioi_Command = CMD_BLOCKREAD;
    break;
  }
  if (ce != NULL) {
    DBUG(("Issuing read to the I/O cache, abs block %d!\n", rawRequest->io_Info.ioi_Offset));
    rawRequest->io_Info.ioi_Recv.iob_Buffer = ce->ioce_CachedBlock;
    rawRequest->io_Info.ioi_Recv.iob_Len = theRequest->fio_BlockCount *
      theFile->fi_BlockSize;
  }
  DBUG(("Raw I/O request for offset %d, buf 0x%lX, bytes %d, endaction 0x%lX\n",
	rawRequest->io_Info.ioi_Offset,
	rawRequest->io_Info.ioi_Recv.iob_Buffer,
	rawRequest->io_Info.ioi_Recv.iob_Len,
	rawRequest->io_CallBack));
  DBUG(("Request block burst %d blocks-per %d\n",
	 theRequest->fio_BlockBurst,
	 theFile->fi_FileSystemBlocksPerFileBlock));
  ofs->ofs_NextBlockAvailable = theRequest->fio_AbsoluteBlockNumber +
    theRequest->fio_BlockBurst * theFile->fi_FileSystemBlocksPerFileBlock;
  DBUG(("Read FS block %d, bytes %d, next block avail %d\n",
	 theRequest->fio_AbsoluteBlockNumber,
	 rawRequest->io_Info.ioi_Recv.iob_Len,
	 ofs->ofs_NextBlockAvailable));
  err = SuperinternalSendIO(rawRequest);
  if (err < 0) {
    qprintf(("Error %d from SuperinternalSendIO to device!\n", err));
#ifdef BUILD_STRINGS
    ERR(rawRequest->io_Error);
#endif
  }
  return OperaDone; /* add error-checking to previous call! */
}

static void AbortOperaIO(IOReq *theRequest)
{
  uint32 interrupts;
  OperaFS *ofs;
  OFile *theOpenFile;
  theOpenFile = (OFile *) theRequest->io_Dev;
  ofs = (OperaFS *)theOpenFile->ofi_File->fi_FileSystem;
  interrupts = Disable(); /*** DISABLE ***/
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O request - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (ofs->ofs.fs_DeviceBusy == OperaRunning &&
   ofs->ofs_RawDeviceRequest->io_Info.ioi_UserData == (void *) theRequest) {
    SuperinternalAbortIO(ofs->ofs_RawDeviceRequest);
  } else {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  Enable(interrupts);
  return;
}

static IOReq *CatapultEndAction(IOReq *rawRequest)
{
  OperaFS *ofs;
  ofs = (OperaFS *) rawRequest->io_Info.ioi_UserData;
  ofs->ofs_CatapultPhase = CATAPULT_MUST_VERIFY;
  ofs->ofs.fs_DeviceBusy = OperaIdle;
  SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
		      fileFolio.ff_Daemon.ffd_RescheduleSignal);
  return (IOReq *) NULL;
}

static IOReq *OperaEndAction(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  OFile *theOpenFile;
  OperaFS *ofs;
  File *theFile;
  int32 avatarIndex;
  uint32 avatar, flawLevel;
  int32 bytesRead;
  int32 significantErr;
  IoCacheEntry *ce;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_UserData;
  theOpenFile = (OFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  ofs = (OperaFS *) theFile->fi_FileSystem;
  bytesRead = rawRequest->io_Actual;
  ce = userRequest->fio_CachePage;
  userRequest->fio_CachePage = NULL;
/*
  Pass back the completion information and dequeue the request from the
  RequestsRunning list.
*/
  RemNode((Node *) userRequest);
  if ((rawRequest->io_Error & 0x000001FF) == ((ER_C_STND << ERR_CLASHIFT) +
					      (ER_Aborted << ERR_ERRSHIFT))) {
    userRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
  } else if ((rawRequest->io_Error & 0x000001FF) == ((ER_C_STND << ERR_CLASHIFT) +
						     (ER_DeviceOffline << ERR_ERRSHIFT))) {
    userRequest->fio.io_Error = rawRequest->io_Error;
    DBUG0(("Marking /%s offline\n", ofs->ofs.fs_FileSystemName));
    ofs->ofs.fs_Flags |= (FILESYSTEM_IS_OFFLINE |
			  FILESYSTEM_WANTS_QUIESCENT |
			  FILESYSTEM_WANTS_DISMOUNT);
  } else {
    userRequest->fio.io_Error = rawRequest->io_Error;
/*
  Check for media errors.  If a medium error occurs on a file which
  has more than one avatar, bump up the soft-error count on this avatar
  (unless the limit has been reached) and requeue the I/O for another
  try.  For the purposes of this code, parameter errors are considered
  equivalent to medium errors, since the CD-ROM drive can return such an
  error code if a block header is damaged beyond repair.  Errors occuring
  during a catapult-file access cause the entire catapult file to be
  considered NFG - I/O will be requeued using the real avatars.
*/
    significantErr = userRequest->fio.io_Error & 0x000001FF;
    if (significantErr == ((ER_C_STND << ERR_CLASHIFT) +
			   (ER_MediaError << ERR_ERRSHIFT)) ||
	significantErr == ((ER_C_STND << ERR_CLASHIFT) +
			   (ER_ParamError << ERR_ERRSHIFT))) {
      if (theFile->fi_LastAvatarIndex != 0) {
	avatarIndex = (int32) userRequest->fio_AvatarIndex;
	if (avatarIndex < 0) {
	  ofs->ofs_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
	} else {
	  avatar = theFile->fi_AvatarList[avatarIndex];
	  flawLevel = (int32) ((avatar >> DRIVER_FLAW_SHIFT) & DRIVER_FLAW_MASK);
	  if (flawLevel < DRIVER_FLAW_SOFTLIMIT) {
	    theFile->fi_AvatarList[avatarIndex] += (1L << DRIVER_FLAW_SHIFT);
	  }
	}
	AddTail(&ofs->ofs.fs_RequestsToDo,
		(Node *) userRequest);
	if (ce) {
	  ce->ioce_PageState = CachePageInvalid;
	  RelinquishCachePage(ce);
	}
	goto nextIO;
      }
    }
  }
/*
   If there's a cache page involved with this operation, we don't want to
   mark the user's I/O as being done yet, because it isn't (we're doing
   a READDIR/READENTRY/OPENENTRY command).  Update the cache page info
   as appropriate, relinquish it, and signal the daemon to wake up and
   take the next step to complete this command.
*/
  if (ce) {
    DBUG(("Cache read complete, got %d bytes, error 0x%X\n",
	  bytesRead, userRequest->fio.io_Error));
    if (userRequest->fio.io_Error == 0) {
      DBUG(("Page marked valid!\n"));
      ce->ioce_CachedBlockSize = bytesRead;
      ce->ioce_PageState = CachePageValid;
    } else {
      DBUG(("Page marked invalid!\n"));
      ce->ioce_PageState = CachePageInvalid;
      ERR(userRequest->fio.io_Error);
    }
    RelinquishCachePage(ce);
    AddHead(&ofs->ofs.fs_RequestsRunning, (Node *) userRequest);
    ofs->ofs.fs_DeviceBusy = OperaIdle;
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_RescheduleSignal);
    return (IOReq *) NULL;
  }
  userRequest->fio.io_Actual += bytesRead;
/*
  That request is done (successfully or with errors from which we cannot
  recover).  Signal completion and check for a follow-on I/O from an
  upper-level driver.
*/
  SuperCompleteIO((IOReq *) userRequest);
/*
  Check to see if a boink has occurred.  If so, move any already-
  scheduled requests to the defer list.  Do likewise if we realize that
  the filesystem is offline... the scheduler will accept responsibility
  for nuking all pending requests.
*/
nextIO:
  if ((ofs->ofs.fs.n_Flags & DEVICE_BOINK) ||
      (ofs->ofs.fs_Flags & FILESYSTEM_IS_OFFLINE)) {
    while (!ISEMPTYLIST(&ofs->ofs.fs_RequestsRunning)) {
      userRequest = (FileIOReq *) RemTail(&ofs->ofs.fs_RequestsRunning);
      AddHead(&ofs->ofs.fs_RequestsDeferred, (Node *) userRequest);
    }
    ofs->ofs.fs.n_Flags &= ~DEVICE_BOINK;
  }
  if (ISEMPTYLIST(&ofs->ofs.fs_RequestsRunning)) {
/*
  Either we've completed the last scheduled I/O, or we've set aside
  some pending requests because of a higher-priority boink.  In either
  case, we can't do any more at this point.  If either TBD list is non-
  empty, awaken the daemon to reschedule.  Bail.
*/
    ofs->ofs.fs_DeviceBusy = OperaIdle;
    if (!ISEMPTYLIST(&ofs->ofs.fs_RequestsToDo) ||
	!ISEMPTYLIST(&ofs->ofs.fs_RequestsDeferred)) {
      SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			  fileFolio.ff_Daemon.ffd_RescheduleSignal);
    }
    return (IOReq *) NULL;
  }
  userRequest = (FileIOReq *) FIRSTNODE(&ofs->ofs.fs_RequestsRunning);
  rawRequest = ofs->ofs_RawDeviceRequest;
  rawRequest->io_CallBack = OperaEndAction;
  rawRequest->io_Info = userRequest->fio.io_Info;
/***
  Pick up the absolute block number in the request, and use it for
  this transfer.
 ***/
  rawRequest->io_Info.ioi_Offset =
    (int32) userRequest->fio_AbsoluteBlockNumber +
      ofs->ofs_RawDeviceBlockOffset;
  rawRequest->io_Info.ioi_Command = CMD_BLOCKREAD;
  rawRequest->io_Info.ioi_UserData = userRequest;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
  rawRequest->io_Flags = 0;
  theOpenFile = (OFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  ofs->ofs_NextBlockAvailable = userRequest->fio_AbsoluteBlockNumber +
    userRequest->fio_BlockBurst * theFile->fi_FileSystemBlocksPerFileBlock;
  return rawRequest;
}

static uchar HighestPriority(List *theList)
{
  int32 highest = 0;
  Node *theNode;
  int32 interrupts;
  interrupts = Disable();
  for (theNode = FIRSTNODE(theList);
       ISNODE(theList, theNode);
       theNode = NEXTNODE(theNode)) {
    if (theNode->n_Priority > highest) {
      highest = theNode->n_Priority;
    }
  }
  Enable(interrupts);
  return (uchar) highest;
}

static void MakeDirectoryEntryFromDirectoryRecord(FileIOReq *theRequest,
						  DirectoryRecord *dirRec,
						  File *theParent)
{
  DirectoryEntry de;
  int32 deSize;
  de.de_Flags = dirRec->dir_Flags;
  if (dirRec->dir_Type == FILE_TYPE_DIRECTORY) {
    de.de_Flags |= FILE_SUPPORTS_DIRSCAN | FILE_SUPPORTS_ENTRY;
  }
  if (((OperaFS *) theParent->fi_FileSystem)->ofs_FilesystemMemBase != NULL) {
    de.de_Flags |= FILE_STATIC_MAPPABLE;
  }
  de.de_UniqueIdentifier = dirRec->dir_UniqueIdentifier;
  de.de_Type = dirRec->dir_Type;
  de.de_BlockSize = dirRec->dir_BlockSize;
  de.de_ByteCount = dirRec->dir_ByteCount;
  de.de_BlockCount = dirRec->dir_BlockCount;
  de.de_Date.tv_Seconds = 0;
  de.de_Date.tv_Microseconds = 0;
  de.de_Version = dirRec->dir_Version;
  de.de_Revision = dirRec->dir_Revision;
  de.de_rfu = 0;
  de.de_Gap = dirRec->dir_Gap;
  de.de_AvatarCount = dirRec->dir_LastAvatarIndex + 1;
  de.de_Location = dirRec->dir_AvatarList[0];
  strncpy(de.de_FileName, dirRec->dir_FileName, FILESYSTEM_MAX_NAME_LEN);
  deSize = theRequest->fio.io_Info.ioi_Recv.iob_Len;
  if (deSize > sizeof de) {
    deSize = sizeof de;
  }
  memcpy(theRequest->fio.io_Info.ioi_Recv.iob_Buffer, &de, deSize);
  theRequest->fio.io_Actual = deSize;
}

static void FillInAvatars(DirectoryRecord *dirRec, File *theFile)
{
  uint32 fileSize;
  uint32 avatarSpace;
  int32 highestAvatarIndex;
  fileSize = theFile->fi.n_Size;
  avatarSpace = fileSize - sizeof (File);
  highestAvatarIndex = avatarSpace / sizeof (uint32);
  if (highestAvatarIndex > dirRec->dir_LastAvatarIndex) {
    highestAvatarIndex = dirRec->dir_LastAvatarIndex;
  }
  DBUG(("Filling in avatar list of file '%s'\n", theFile->fi_FileName));
  theFile->fi_LastAvatarIndex = highestAvatarIndex;
  while (highestAvatarIndex >= 0) {
    DBUG(("  Avatar %d at %d\n", highestAvatarIndex,
	  dirRec->dir_AvatarList[highestAvatarIndex]));
    theFile->fi_AvatarList[highestAvatarIndex] =
      dirRec->dir_AvatarList[highestAvatarIndex];
    highestAvatarIndex --;
  }
  theFile->fi_FileSystemBlocksPerFileBlock = theFile->fi_BlockSize /
    theFile->fi_FileSystem->fs_VolumeBlockSize;
}

static int32 SearchOperaDirectoryPage(FileIOReq *theRequest,
				      IoCacheEntry *ce,
				      File *theFile)
{
  uint32 offset, index;
  int32 compare;
  char *point, *dirPage;
  DirectoryRecord *dirRec;
  int32 blockNum, blocks;
  int32 i;
  File *theChild = NULL;
  char fileName[FILESYSTEM_MAX_NAME_LEN];
  index = ce->ioce_CacheFirstIndex;
  dirPage = (char *) ce->ioce_CachedBlock;
  blocks = ce->ioce_CachedBlockSize / theFile->fi_BlockSize;
  blockNum = 0;
  switch (theRequest->fio.io_Info.ioi_Command) {
  case FILECMD_READENTRY:
    memset(fileName, 0, sizeof fileName);
    i = theRequest->fio.io_Info.ioi_Send.iob_Len;
    if (i >= sizeof fileName) {
      i = sizeof fileName - 1;
    }
    strncpy(fileName, (char *) theRequest->fio.io_Info.ioi_Send.iob_Buffer, i);
    DBUG(("Search for '%s'\n", fileName));
    break;
  case FILECMD_OPENENTRY:
    theChild = (File *) LookupItem(theRequest->fio.io_Info.ioi_CmdOptions);
    if (!theChild) {
      return FALSE;
    }
    strncpy(fileName, theChild->fi_FileName, FILESYSTEM_MAX_NAME_LEN);
    DBUG(("Search for '%s'\n", fileName));
    break;
  case FILECMD_READDIR:
    DBUG(("Search for entry %d\n", theRequest->fio.io_Info.ioi_Offset));
    break;
  }
  while (blockNum < blocks) {
    DBUG(("Probing sub-block %d\n", blockNum));
    offset = (int32) ((DirectoryHeader *) dirPage)->dh_FirstEntryOffset;
    while (offset != 0) {
      DBUG(("Probe offset %d\n", offset));
      point = dirPage + offset;
      dirRec = (DirectoryRecord *) point;
      switch (theRequest->fio.io_Info.ioi_Command) {
      case FILECMD_READENTRY:
      case FILECMD_OPENENTRY:
	compare = strcasecmp(fileName, dirRec->dir_FileName);
	break;
      case FILECMD_READDIR:
      default:
	compare = (index != theRequest->fio.io_Info.ioi_Offset);
	break;
      }
      index ++;
      if (compare == 0) {
	DBUG(("Hit!\n"));
	switch (theRequest->fio.io_Info.ioi_Command) {
	case FILECMD_OPENENTRY:
	  FillInAvatars(dirRec, theChild);
	  break;
	case FILECMD_READDIR:
	case FILECMD_READENTRY:
	  MakeDirectoryEntryFromDirectoryRecord(theRequest, dirRec, theFile);
	  break;
	}
	BumpCachePagePriority(ce, CACHE_PRIO_HIT);
	return TRUE;
      }
      if (dirRec->dir_Flags & DIRECTORY_LAST_IN_BLOCK) {
	offset = 0;
      } else {
	offset += sizeof (DirectoryRecord) +
	  (sizeof (ulong) * (int32) dirRec->dir_LastAvatarIndex);
      }
    }
    dirPage += theFile->fi_BlockSize;
    blockNum ++;
  }
  ce->ioce_CacheEntryCount = index - ce->ioce_CacheFirstIndex;
  return FALSE;
}


static int32 OperaCacheSearch(FileIOReq *theRequest)
{
  int32 blockNum;
  int32 blocksPerPage;
  IoCacheEntry *ce;
  File *theFile;
  void *block;
  enum CacheState state;
  theFile = ((OFile *) theRequest->fio.io_Dev)->ofi_File;
  blocksPerPage = 2048 / theFile->fi_BlockSize;
  DBUG2(("Cache contains %d blocks per page for this file\n", blocksPerPage));
  blockNum = 0;
  while (blockNum < theFile->fi_BlockCount) {
    DBUG2(("Look for block %d\n", blockNum));
    state = FindBlockInCache(theFile, blockNum, FALSE, &ce, &block);
    if (state == CachePageValid) {
      DBUG2(("Found it!  Searching\n"));
      if (SearchOperaDirectoryPage(theRequest, ce, theFile)) {
	DBUG2(("Search successful!\n"));
	RelinquishCachePage(ce);
	return TRUE;
      }
      DBUG2(("Search failed\n"));
      RelinquishCachePage(ce);
    }
    blockNum += blocksPerPage;
  }
  return FALSE;
}
