/* @(#) mount.c 96/09/23 1.43 */

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
#include <kernel/semaphore.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <misc/event.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#undef DEBUG
#undef DEBUG2
#undef TRACEDISMOUNT

#ifndef BUILD_STRINGS
# undef TRACEDISMOUNT
#endif

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

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef BUILD_STRINGS
extern void DumpFileList(void);
#endif

#ifdef BARFFS
extern Item MountBarfFilesystem(Device *theDevice, int32 unit,
				uint32 blockOffset);
extern Err DismountBarfFilesystem(BarfDisk *theDevice);
#endif /* BARFFS */

extern OFile *OpenPath (File *startingDirectory, char *path);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);

extern void internalPlugNPlay(void);

extern IoCacheEntry *AddCacheEntry(OFile *theOpenFile, int32 blockNum,
				   void *buffer,
				   uint8 priority, uint32 format,
				   uint32 bytes);

extern FileFolioTaskData *SetupFileFolioTaskData(void);

extern int32 internalDeleteItem(Item,struct Task *);

Err FinishDismount(FileSystem *fs);
extern int32 TrimZeroUseFiles(FileSystem *fs, int32 limit);

const TagArg filesystemArgs[] = {
  { FILESYSTEM_TAG_PRI,     (void *)1, },
  { FILESYSTEM_TAG_END,     0, },
};

const TagArg fsDevArgs[] = {
  { TAG_ITEM_PRI,            (void *)1, },
  { CREATEDEVICE_TAG_DRVR,   NULL },
  { TAG_ITEM_NAME,           "filesystem" },
  { TAG_END,                 0, },
};

extern int32 fsCacheBusy;
extern Semaphore *fsListSemaphore;

void Nuke(void *foo) {
  if (foo) {
    SuperDeleteItem(((ItemNode *)foo)->n_Item);
    DBUG2(("Released.\n"));
  }
}

void GiveDaemon(void *foo) {
  Err err;
  DBUG(("Give item at 0x%x to daemon\n", foo));
  if (foo && fileFolio.ff_Daemon.ffd_Task) {
    DBUG(("Transfer item 0x%x to task 0x%x\n",
	   ((ItemNode *)foo)->n_Item,
	   fileFolio.ff_Daemon.ffd_Task->t.n_Item));
    err = SuperSetItemOwner(((ItemNode *)foo)->n_Item,
			    fileFolio.ff_Daemon.ffd_Task->t.n_Item);
    DBUG(("SuperSetItemOwner returns 0x%x\n", err));
#ifdef BUILD_PARANOIA
    if (err < 0) {
      DBUG0(("SuperSetItemOwner failed on item 0x%x\n",
	     ((ItemNode *)foo)->n_Item));
      ERR(err);
    }
#else
	TOUCH(err);
#endif
  }
}

Err FilesystemEvent(FileSystem *fs, uint32 eventCode)
{
  struct {
    EventFrameHeader    efh;
    FilesystemEventData fsed;
  } event;
  Err err;
  DBUG(("Reporting event %d\n", eventCode));
  memset(&event, 0, sizeof event);
  event.efh.ef_ByteCount = sizeof event;
  event.efh.ef_EventNumber = (uint8) eventCode;
  event.fsed.fsed_FilesystemItem = fs->fs.n_Item;
  memcpy(event.fsed.fsed_Name, fs->fs_FileSystemName,
	 sizeof event.fsed.fsed_Name);
  err = SuperReportEvent(&event);
#ifdef DEBUG
  if (err < 0) {
    ERR(err);
  }
  DBUG(("Event done, returned 0x%X\n", err));
#endif
  return err;
}

static void CatDotted(char *string, uint32 num)
{
  uint32 digits, residue, len;
  len = strlen(string);
  digits = 0;
  residue = num;
  do {
    digits ++;
    residue /= 10;
  } while (residue != 0);
  string[len+digits+1] = '\0';
  string[len] = '.';
  residue = num;
  do {
    string[len+digits] = '0' + (char) (residue % 10);
    digits --;
    residue /= 10;
  } while (residue != 0);
}

static Item InitFileSystem(Device *theDevice, uint32 blockOffset)
{
  Item filesystemItem;
  Item ioReqItem;
#ifdef SEMAPHORE_LMD
  Item semItem;
  Semaphore *sem = NULL;
#endif
  ExtVolumeLabel *discLabel = NULL;
  DeviceStatus devStatus;
  IOReq *rawIOReq = NULL;
  int32 avatarIndex, avatarBlock;
  int32 gotLabel;
  int32 discLabelSize = 0;
  int32 deviceBlockSize;
  Err err;
  FileSystemType *fst;
#if defined(DEBUG2)&&defined(DUMPLABEL)
  int32 i;
  int32 j;
  uint8 *foo, c;
#endif
  TagArg ioReqTags[2];
/*
  Create the filesystem device which resides on this physical
  device.
*/
  DBUG(("Looking for filesystem on %s offset %d\n",
	theDevice->dev.n_Name, blockOffset));
/*****
  Here we need to create an IOReq for the raw device.
 *****/
  DBUG(("Creating IOReq\n"));
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) theDevice->dev.n_Item;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    DBUG(("Can't allocate an internal IOReq for raw device"));
    err = ioReqItem;
    goto nuke;
  }
  rawIOReq = (IOReq *) LookupItem(ioReqItem);
  DBUG(("IOReq is item 0x%X at 0x%X\n", ioReqItem, rawIOReq));
/*
  Query the raw device to determine its block size and block count,
  and record these in the optimized-device structure.
*/
  DBUG(("Getting device status\n"));
  rawIOReq->io_Info.ioi_Command = CMD_STATUS;
  rawIOReq->io_Info.ioi_Flags = IO_QUICK;
  rawIOReq->io_Info.ioi_Offset = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = NULL;
  rawIOReq->io_Info.ioi_Send.iob_Len = 0;
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &devStatus;
  rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof devStatus;
  devStatus.ds_DeviceBlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  devStatus.ds_DeviceBlockCount = 0;
  err = SuperInternalDoIO(rawIOReq);
  if (err < 0) {
    DBUG(("I/O error 0x%x getting device status\n", err));
#ifdef DEBUG
    ERR(err);
#endif
    goto nuke;
  }
  deviceBlockSize = devStatus.ds_DeviceBlockSize;
  DBUG(("Got device status\n"));
  DBUG(("Device says block size is %d, block count is %d\n",
	  devStatus.ds_DeviceBlockSize,
	  devStatus.ds_DeviceBlockCount));
/*
   If device is incapable of supporting any type of filesystem, bail
   out immediately.
*/
  if (!(devStatus.ds_DeviceUsageFlags & DS_USAGE_FILESYSTEM)) {
    DBUG(("Device cannot support filesystem\n"));
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
    goto nuke;
  }
/*
   Run a quick first pass over all of the filesystem types.  Offer
   them all a chance to mount the filesystem before we try to read
   the label.  Block-oriented filesystems such as Opera, Roadkill,
   and Acrobat will no-comment at this point when they see that
   the label is null.  Comm-oriented filesystems such as Mac, BARF,
   etc. will mount (if appropriate) and return a new filesystem item.

   FIXME - decide whether a shared-read lock here, and down below, is
   actually adequate.  I think so;  make sure.  dplatt
*/
  (void) SuperInternalLockSemaphore(fsListSemaphore,
				    SEM_WAIT + SEM_SHAREDREAD);
  filesystemItem = 0;
  for (fst = (FileSystemType *) FirstNode(&fileFolio.ff_FileSystemTypes);
       filesystemItem <= 0 && ISNODE(&fileFolio.ff_FileSystemTypes, fst);
       fst = (FileSystemType *) NextNode(fst)) {
    filesystemItem = (*fst->fst_Mount)(theDevice, blockOffset,
				       rawIOReq, NULL, 0, &devStatus);
  }
  (void) SuperInternalUnlockSemaphore(fsListSemaphore);
  if (filesystemItem > 0) {
    (void) FilesystemEvent((FileSystem *) LookupItem(filesystemItem),
			   EVENTNUM_FilesystemMounted);
    return filesystemItem;
  }
  if (!deviceBlockSize) {
    DBUG(("device is blockless\n"));
    err = 0;
    goto nuke;
  }
/*
   First pass failed.  Second pass looks only at label-oriented file
   systems.
*/
  if (blockOffset < devStatus.ds_DeviceBlockStart) {
    blockOffset = devStatus.ds_DeviceBlockStart;
  }
  discLabelSize = (int32) ((sizeof (ExtVolumeLabel) + deviceBlockSize - 1) /
			   deviceBlockSize) * deviceBlockSize;
  discLabel = (ExtVolumeLabel *) SuperAllocMem(discLabelSize,
				     MEMTYPE_DMA + MEMTYPE_FILL);
  DBUG(("Allocated disc-label at 0x%x, %d bytes\n", discLabel, discLabelSize));
  if (!discLabel) {
    DBUG(("Could not allocate disc label\n"));
    err = NOMEM;
    goto nuke;
  }
  DBUG(("Buffer allocated at 0x%x\n", discLabel));
/*
  Read in the disc label, from any available avatar, and confirm that
  it's what we expected.  Bail if not.
*/
  gotLabel = 0;
  avatarBlock = 0;
  for (avatarIndex = -1;
       avatarIndex <= DISC_LABEL_HIGHEST_AVATAR && !gotLabel &&
       devStatus.ds_DeviceBlockCount > (avatarBlock + blockOffset);
       avatarIndex++) {
    DBUG(("Probing for label at block %d", avatarBlock));
/*
   Null send buffer already set up from CMD_STATUS
*/
    rawIOReq->io_Info.ioi_Command = CMD_BLOCKREAD;
    rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) discLabel;
    rawIOReq->io_Info.ioi_Recv.iob_Len = discLabelSize;
    rawIOReq->io_Info.ioi_Flags = IO_QUICK;
    rawIOReq->io_Info.ioi_Offset = (int32) avatarBlock + blockOffset;
    err = SuperInternalDoIO(rawIOReq);
    if (err < 0) {
      DBUG((" [couldn't read]\n"));
      goto nuke;
    } else {
      DBUG((" [got %d]", rawIOReq->io_Actual));
      if (discLabel->dl_RecordType != 1 ||
	  discLabel->dl_VolumeSyncBytes[0] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[1] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[2] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[3] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[4] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_RootDirectoryLastAvatarIndex > ROOT_HIGHEST_AVATAR)
	{
	  DBUG((" [not valid]\n"));
#if defined(DEBUG2)&&defined(DUMPLABEL)
	  foo = (uint8 *) discLabel;
	  for (i = 0; i < 20; i++) {
	    for (j = 0; j < 16; j++) {
	      qprintf(("%02x ", *(foo+j)));
	    }
	    qprintf(("  "));
	    for (j = 0; j < 16; j++) {
	      c = *foo++;
	      if (!isprint(c)) c = '.';
	      qprintf(("%c", c));
	    }
	    qprintf(("\n"));
	  }
#endif
	}
      else {
	gotLabel = 1;
	DBUG((" [OK]\n"));
      }
    }
    if (avatarIndex < 0) {
      avatarBlock = DISC_LABEL_OFFSET;
    } else {
      avatarBlock += DISC_LABEL_AVATAR_DELTA;
    }
  }
  if (gotLabel) {
    DBUG(("Label '%s' validated, ID %d\n",
	   discLabel->dl_VolumeIdentifier,
	   discLabel->dl_VolumeUniqueIdentifier));
  } else {
    DBUG(("Could not find a valid label on %s\n",
	  theDevice->dev.n_Name));
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
    goto nuke;
  }
/*
  Label found.  Do the second pass of the mount process, invoking only
  the specific filesystem interpreter which says that it handles this
  particular flavor of 3DO filesystem.
*/
  (void) SuperInternalLockSemaphore(fsListSemaphore,
				    SEM_WAIT + SEM_SHAREDREAD);
  filesystemItem = 0;
  for (fst = (FileSystemType *) FirstNode(&fileFolio.ff_FileSystemTypes);
       filesystemItem <= 0 && ISNODE(&fileFolio.ff_FileSystemTypes, fst);
       fst = (FileSystemType *) NextNode(fst)) {
    if (fst->fst_VolumeStructureVersion != 0 &&
	fst->fst_VolumeStructureVersion ==
	discLabel->dl_VolumeStructureVersion) {
      DBUG2(("Try filesystem type at 0x%X\n", fst));
      filesystemItem = (*fst->fst_Mount)(theDevice, blockOffset,
					 rawIOReq, discLabel, avatarBlock,
					 &devStatus);
      DBUG2(("Filesystem module returned 0x%X\n", filesystemItem));
    }
  }
  (void) SuperInternalUnlockSemaphore(fsListSemaphore);
  if (filesystemItem > 0) {
    DBUG2(("Filesystem module succeeded in mounting!\n"));
    SuperFreeMem(discLabel, discLabelSize);
    (void) FilesystemEvent((FileSystem *) LookupItem(filesystemItem),
			   EVENTNUM_FilesystemMounted);
    return filesystemItem;
  } else if (filesystemItem < 0) {
    err = filesystemItem;
  } else {
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
  }
 nuke:
  DBUG2(("Releasing IOReq\n"));
  Nuke(rawIOReq);

  DBUG2(("Releasing label\n"));
  SuperFreeMem(discLabel, discLabelSize);

  DBUG(("Returning error code 0x%x\n", err));
  return err;
}

static void InitFileSystemFromLabel(FileSystem *fs, ExtVolumeLabel *discLabel)
{
  fs->fs_DeviceBusy = 0;
  fs->fs_RequestPriority = 0;
  fs->fs_RunningPriority = 0;
  InitList(&fs->fs_RequestsToDo, "To do");
  InitList(&fs->fs_RequestsRunning, "Running");
  InitList(&fs->fs_RequestsDeferred, "Deferred");
  strcpy(fs->fs_MountPointName, "on.");
  strcat(fs->fs_MountPointName, fs->fs_RawDevice->dev.n_Name);
  CatDotted(fs->fs_MountPointName, fs->fs_RawOffset);
  if (discLabel) {
    fs->fs_VolumeBlockSize = discLabel->dl_VolumeBlockSize;
    fs->fs_VolumeBlockCount = discLabel->dl_VolumeBlockCount;
    fs->fs_VolumeUniqueIdentifier = discLabel->dl_VolumeUniqueIdentifier;
    fs->fs_VolumeFlags = discLabel->dl_VolumeFlags;
    strncpy((char *) fs->fs_FileSystemName,
	    (char *) discLabel->dl_VolumeIdentifier,
	    FILESYSTEM_MAX_NAME_LEN);
  }
  DBUG(("Volume is %d blocks of %d bytes, flags 0x%X\n",
	fs->fs_VolumeBlockCount,
	fs->fs_VolumeBlockSize,
	fs->fs_VolumeFlags));
}

static File *CreateRootDirectory(FileSystem *fileSystem, uint32 fileSize,
			  ExtVolumeLabel *discLabel)
{
  Item rootFileItem;
  File *fsRoot;
  rootFileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
				      (void *) NULL, fileSize);
  fsRoot = (File *) LookupItem(rootFileItem);
  if (!fsRoot) {
    DBUG(("Could not create root file node\n"));
    return NULL;
  }
/*
  Set up the root file descriptor and fill in the actual locations of the
  root directory.
*/
  DBUG(("Initializing root directory\n"));
  fsRoot->fi_FileSystem = fileSystem;
  fsRoot->fi_ParentDirectory = fileFolio.ff_Root;
  fsRoot->fi_Type = FILE_TYPE_DIRECTORY;
  fileFolio.ff_Root->fi_UseCount ++;
  fsRoot->fi_UseCount = 1; /* lock it in */
  if (discLabel) {
    fsRoot->fi_BlockSize = discLabel->dl_RootDirectoryBlockSize;
    fsRoot->fi_FileSystemBlocksPerFileBlock =
      discLabel->dl_RootDirectoryBlockSize / discLabel->dl_VolumeBlockSize;
    fsRoot->fi_BlockCount = discLabel->dl_RootDirectoryBlockCount;
    fsRoot->fi_ByteCount = fsRoot->fi_BlockSize * fsRoot->fi_BlockCount;
    fsRoot->fi_UniqueIdentifier = discLabel->dl_VolumeUniqueIdentifier;
    fsRoot->fi_LastAvatarIndex = discLabel->dl_RootDirectoryLastAvatarIndex;
  }
  strncpy((char *) fsRoot->fi_FileName,
	  (char *) fileSystem->fs_FileSystemName,
	  FILESYSTEM_MAX_NAME_LEN);
  AddHead(&fileFolio.ff_Files, (Node *) fsRoot);
  return fsRoot;
}

FileSystem *CreateFileSystem (Device *theDevice,
			      uint32 blockOffset, ExtVolumeLabel *discLabel,
			      uint32 highLevelDiskSize, uint32 rootAvatars,
			      FileSystemType *fst)
{
  Item filesystemItem;
  int32 fileSize;
  FileSystem *fileSystem;
  File *fsRoot;
  DBUG(("Creating filesystem node\n"));
  filesystemItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILESYSTEMNODE),
					(TagArg *) filesystemArgs,
					highLevelDiskSize);
  fileSystem = (FileSystem *) LookupItem(filesystemItem);
  if (!fileSystem) {
    DBUG(("Could not create filesystem\n"));
    return NULL;
  }
  fileSystem->fs_Type = fst;
  fileSystem->fs_RawDevice = theDevice;
  fileSystem->fs_RawOffset = blockOffset;
  InitFileSystemFromLabel(fileSystem, discLabel);
/*
  Create a file descriptor which will frame the root directory.
*/
  DBUG(("Creating root-descriptor file node\n"));
  fileSize = (int32) (sizeof(File) + sizeof (ulong) * rootAvatars);
  fsRoot = CreateRootDirectory(fileSystem, fileSize, discLabel);
  if (!fsRoot) {
    DBUG(("Could not create root\n"));
    goto nuke;
  }
  fsRoot->fi_Flags = FILE_IS_DIRECTORY | FILE_IS_FOR_FILESYSTEM;
  if (fileSystem->fs_VolumeFlags & VF_BLESSED) {
    fsRoot->fi_Flags |= FILE_IS_BLESSED;
  }
#ifdef	FS_DIRSEMA
  InitDirSema(fsRoot, 1);
#endif	/* FS_DIRSEMA */
  strcpy(fileSystem->fs_MountPointName, "on.");
  strcat(fileSystem->fs_MountPointName, theDevice->dev.n_Name);
  CatDotted(fileSystem->fs_MountPointName, (uint32) blockOffset);
  fileSystem->fs_RootDirectory = fsRoot;
  internalPlugNPlay();
  return fileSystem;
 nuke:
  DBUG2(("Releasing root\n"));
  Nuke(fsRoot);
  DBUG2(("Releasing filesystem\n"));
  Nuke(fileSystem);
  return NULL;
}

void LinkAndAnnounceFileSystem(FileSystem *fs)
{
  int32 interrupts, nLen, seqLen;
  static int32 reseq=0;
  Node *conflict;
  uchar name[VOLUME_ID_LEN], seqNum[VOLUME_ID_LEN];
  strcpy(name, fs->fs_FileSystemName);
  nLen = strlen(name);
  do {
    conflict = FindNamedNode(&fileFolio.ff_Filesystems, name);
    if (conflict) {
      sprintf(seqNum, "_%d", ++reseq);
      seqLen = strlen(seqNum);
      if (nLen + seqLen < VOLUME_ID_LEN) {
	strcpy(name + nLen, seqNum);
      } else {
	strcpy(name + VOLUME_ID_LEN - seqLen - 1, seqNum);
      }
    }
  } while (conflict);
  strcpy(fs->fs_FileSystemName, name);
  interrupts = Disable();
  AddTail(&fileFolio.ff_Filesystems, (Node *) fs);
  Enable(interrupts);
  qprintf(("Filesystem /%s is mounted on %s\n",
	   fs->fs_FileSystemName,
	   fs->fs_RawDevice->dev.n_Name));
}

FileSystem *FindFileSystem(const uchar *name)
{
  FileSystem *it;
  int32 done = FALSE;
  do {
    it = (FileSystem *) FindNamedNode(&fileFolio.ff_Filesystems, name);
    if (!it || !(it->fs_Flags & FILESYSTEM_WANTS_RECHECK)) {
      done = TRUE;
    } else {
      SleepCache();
    }
  } while (!done);
  return it;
}

/**
  WARNING!  WARNING!  The CrankDismountEngine may delete the filesystem
  upon which it is called.  It may be called ONLY if the fsListSemaphore
  has been locked (for exclusive access) by its caller!
 **/

Err CrankDismountEngine(FileSystem *fs)
{
  Err err = 0;
  FileSystemType *fst = fs->fs_Type;
  if (fs->fs_Flags & FILESYSTEM_WANTS_QUIESCENT) {
    TrimZeroUseFiles(fs, 0); /* try purging them all */
    if (fst->fst_ActQue) {
      err = (*fst->fst_ActQue)(fs, QuiesceFS);
      if (err < 0) {
	return err;
      }
    } else if (fs->fs_RootDirectory->fi_UseCount == 1) {
      fs->fs_Flags = (fs->fs_Flags & ~FILESYSTEM_WANTS_QUIESCENT) |
	FILESYSTEM_IS_QUIESCENT;
    } else {
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
    }
  }
  if ((fs->fs_Flags & FILESYSTEM_WANTS_DISMOUNT) &&
      (fs->fs_Flags & FILESYSTEM_IS_QUIESCENT)) {
    err = (*fst->fst_Dismount)(fs);
    if (err >= 0) {
      err = FinishDismount(fs);  /* there is a small puff of orange smoke... */
    }
  }
  return err;
}

static Err SuperDismountFileSystem(char *name)
{
  FileSystem *fs;
  Err err;
  if (name[0] == '/') {
    name ++;
  }
  fs = (FileSystem *) FindNamedNode(&fileFolio.ff_Filesystems, name);
  if (!fs) {
    DBUG(("No such filesystem\n"));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
  }
#ifdef TRACEDISMOUNT
  DumpFileList();
#endif
  fs->fs_Flags |= FILESYSTEM_WANTS_QUIESCENT + FILESYSTEM_WANTS_DISMOUNT;
  while (TrimZeroUseFiles(fs, 0) > 0) {
    ;;;;;;; /* trim until it won't trim no more */
  }
  err = SuperInternalLockSemaphore(fsListSemaphore, SEM_WAIT);
  if (err < 0) {
    return err;
  }
  err = CrankDismountEngine(fs); /** This may destroy the filesystem! **/
  SuperInternalUnlockSemaphore(fsListSemaphore);
  return err;
}

/*
 * Finish up the process of dismounting.  This is called from within
 * CrankDismountEngine, and thus the caller MUST have locked the
 * fsListSemaphore for exclusive access!
 */

Err FinishDismount(FileSystem *fs)
{
  Err err;
  fs->fs_RootDirectory->fi_UseCount = 0;
#ifdef	FS_DIRSEMA
  DelDirSema(fs->fs_RootDirectory);
#endif	/* FS_DIRSEMA */
  DBUG(("Deleting filesystem root node\n"));
  qprintf(("Dismounting /%s from %s\n",
	   fs->fs_FileSystemName,
	   fs->fs_RawDevice->dev.n_Name));
  err = SuperInternalDeleteItem(fs->fs_RootDirectory->fi.n_Item);
  if (err < 0) {
    return err;
  }

/*
   At this point we've crossed the Rubicon... the root directory has
   been deleted.  We can no longer permit accesses to this filesystem.
   If one of the subsequent deletions fails, that's too bad... we'll
   have zombie items hanging around.
*/

  DBUG(("Unlinking filesystem\n"));
  RemNode((Node *) fs);
  InvalidateFilesystemCachePages(fs);
  SuperInternalCloseItem(fs->fs_RawDevice->dev.n_Item,
			 fileFolio.ff_Daemon.ffd_Task);
  DBUG(("Reporting event, just before we delete!\n"));
  (void) FilesystemEvent(fs, EVENTNUM_FilesystemDismounted);
  err = SuperInternalDeleteItem(fs->fs.n_Item);
  if (err < 0) {
    DBUG(("Filesystem deletion failed\n"));
    return err;
  }
  return 0;
}


/**********************
 SWI handler for MountFileSystem call
 **********************/

Item MountFileSystemSWI(Item deviceItem, uint32 blockOffset)
{
  Device *theDevice;
  Item fsItem;
  DBUG(("MountFileSystem SWI task Flag: 0x%x\n", CURRENTTASK->t.n_Flags));
#ifndef BARFFS
  /* BARF mounts/dismounts don't require privilege. */
  if (!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
  }
#endif /* BARFFS */
  (void) SetupFileFolioTaskData();
  theDevice = (Device *) CheckItem(deviceItem, KERNELNODE, DEVICENODE);
  if (!theDevice) {
    DBUG(("Item %d is not a device!\n", theDevice));
    return FILE_ERR_BADITEM;
  }
  fsItem = InitFileSystem(theDevice, blockOffset);
  DBUG2(("MountFileSystem SWI exiting, code 0x%x\n", fsItem));
  return fsItem;
}

/**********************
 SWI handler for DismountFileSystem call
 **********************/

Err DismountFileSystemSWI(char *name)
{
  DBUG(("Dismount SWI task Flag: 0x%x\n", CURRENTTASK->t.n_Flags));
#ifndef BARFFS
  /* BARF mounts/dismounts don't require privilege. */
  if (!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
  }
#endif /* BARFFS */
  (void) SetupFileFolioTaskData();
  return SuperDismountFileSystem(name);
}


/**********************
 Handler for PlugNPlay call
 **********************/

void PlugNPlay(void)
{
  if (fileFolio.ff_Mounter.ffm_Task) {
    SendSignal(fileFolio.ff_Mounter.ffm_Task->t.n_Item,
	       fileFolio.ff_Mounter.ffm_Signal);
  }
}

/**********************
 SWI handler for RecheckAllFileSystems call
 **********************/

Err RecheckAllFileSystemsSWI(void)
{
  FileSystem *fs;
  Err err;
  int32 interrupts;
  err = SuperInternalLockSemaphore(fsListSemaphore, SEM_WAIT);
  if (err < 0) {
    return err;
  }
  interrupts = Disable();
  ScanList(&fileFolio.ff_Filesystems, fs, FileSystem) {
    fs->fs_Flags |= FILESYSTEM_WANTS_RECHECK;
  }
  Enable(interrupts);
  SuperInternalUnlockSemaphore(fsListSemaphore);
#ifdef DOITREALLYFAST
  SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
		      fileFolio.ff_Daemon.ffd_RescheduleSignal);
#endif
  return 0;
}

int32 SetMountLevelSWI(int32 newLevel)
{
  int32 oldLevel;
  oldLevel = fileFolio.ff_MountLevel;
  fileFolio.ff_MountLevel = newLevel;
  if (newLevel > oldLevel) {
    internalPlugNPlay();
  }
  return oldLevel;
}

int32 GetMountLevel(void)
{
  return fileFolio.ff_MountLevel;
}
