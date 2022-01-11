/* @(#) driver.c 96/09/26 1.46 */

/*
  FileDriver.c - implements the driver which handles OFile "devices"
  and OptimizedDisk "devices".

  The driver receives IORequests issued against OFile "devices".  It
  queues these requests against the OptimizedDisk device associated with
  the actual (raw, physical) device on which the filesystem resides.
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

Driver *fileDriver;

extern char *fsCacheBase;
extern int32 fsCacheBusy;

int32 FileDriverInit(struct Driver *me);
int32 FileDriverDispatch(IOReq *theRequest);

void FileDriverAbortIo(IOReq *theRequest);
int32  FileDriverDoit(IOReq *theRequest);
static int32  FileDriverStatus(IOReq *theRequest);
int32  FileDriverGetPath(IOReq *theRequest);

extern int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen);
extern FileFolioTaskData *SetupFileFolioTaskData(void);
extern Err CleanupOpenFile(Device *theDev);
extern void CleanupOnClose(Device *theDev);
extern int32 DeleteIOReqItem(IOReq *req);

extern void InitOpera(List *fsTypes);
extern void InitLinkedMem(List *fsTypes);
extern void InitAcrobat(List *fsTypes);

#ifdef BUILD_DEBUGGER
extern void InitHost(List *fsTypes);
#endif



void FileDriverAbortIo(IOReq *theRequest)
{
  OFile *theOpenFile;
  FileSystem *fs;

  theOpenFile = (OFile *) theRequest->io_Dev;
  fs = theOpenFile->ofi_File->fi_FileSystem;
  if (fs) {
    if (fs->fs_Type && fs->fs_Type->fst_AbortIO) {
      (*fs->fs_Type->fst_AbortIO)(theRequest);
    }
  } else {
    /*
       Must be a root request, presumably an OpenEntry.  Mark it aborted,
       clean it up the next time the daemon is scheduled.
    */
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
  }
  return;
}

int32 FileDriverDispatch (IOReq *theRequest)
{
  FileFolioTaskData *ffPrivate;
  FileSystem *theFS;
  FileSystemStat *fstp;

  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  theFS = ((OFile *) theRequest->io_Dev)->ofi_File->fi_FileSystem;
  switch (theRequest->io_Info.ioi_Command) {
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_ADDDIR:
  case FILECMD_DELETEDIR:
  case FILECMD_RENAME:
    if (!theRequest->io_CallBack) {
       theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
       SuperCompleteIO(theRequest);
       return 1;
    } else {
       theRequest->io_CallBack = 0;
    }
    /* follow through */
  case CMD_READ:
  case CMD_WRITE:
  case CMD_BLOCKREAD:
  case CMD_BLOCKWRITE:
  case CMD_GETMAPINFO:
  case CMD_MAPRANGE:
  case CMD_UNMAPRANGE:
  case CMD_GETICON:
  case FILECMD_READDIR:
  case FILECMD_READENTRY:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_SETTYPE:
  case FILECMD_OPENENTRY:
  case FILECMD_SETVERSION:
  case FILECMD_SETBLOCKSIZE:
  case FILECMD_SETFLAGS:
  case FILECMD_SETDATE:
    return FileDriverDoit(theRequest);
  case FILECMD_FSSTAT:
    fstp = (FileSystemStat *) theRequest->io_Info.ioi_Recv.iob_Buffer;
    if (fstp == NULL) {
       theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
       return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    if (theRequest->io_Info.ioi_Recv.iob_Len < sizeof(FileSystemStat)) {
       theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
       return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    /* filesystem independent data */
    memset(fstp, 0, sizeof(FileSystemStat));
    if (theFS) {
      strncpy(fstp->fst_RawDeviceName, theFS->fs_RawDevice->dev.n_Name,
	      FILESYSTEM_MAX_NAME_LEN);
      fstp->fst_RawDeviceItem = theFS->fs_RawDevice->dev.n_Item;
      fstp->fst_RawOffset = theFS->fs_RawOffset;
      strncpy(fstp->fst_MountName, theFS->fs_FileSystemName,
	      FILESYSTEM_MAX_NAME_LEN);
      fstp->fst_BlockSize = theFS->fs_VolumeBlockSize;
      fstp->fst_Size = theFS->fs_VolumeBlockCount;
      fstp->fst_BitMap |= (FSSTAT_RAWDEVICENAME | FSSTAT_RAWDEVICEITEM |
		           FSSTAT_RAWOFFSET | FSSTAT_MOUNTNAME |
		           FSSTAT_BLOCKSIZE | FSSTAT_SIZE);
    }
    return FileDriverDoit(theRequest);
  case CMD_STATUS:
    return FileDriverStatus(theRequest);
  case FILECMD_GETPATH:
    return FileDriverGetPath(theRequest);
  default:
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
    SuperCompleteIO(theRequest);
    return 1;
  }
}
/*
   Returns error code, 0 if completed without error, 1 if queued for later
   processing.
*/

int32 RootFileIO(IOReq *theRequest)
{
  File *rootDirectory;
  FileSystem *theFS;
  DirectoryEntry de;
  int32 deSize;
  int32 num;
  char fileName[32];
  switch (theRequest->io_Info.ioi_Command) {
  case FILECMD_READDIR:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    num = (int32) theRequest->io_Info.ioi_Offset;
    DBUG(("FILECMD_READDIR %d on /\n", num));
    if (num <= 0) {
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
    }
    theFS = (FileSystem *) FIRSTNODE(&fileFolio.ff_Filesystems);
    while (--num > 0) {
      if (!ISNODE(&fileFolio.ff_Filesystems,theFS)) {
	return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
      }
      theFS = (FileSystem *) NEXTNODE(theFS);
    }
    break;
  case FILECMD_READENTRY:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    memset(fileName, 0, sizeof fileName);
    num = theRequest->io_Info.ioi_Send.iob_Len;
    if (num >= sizeof fileName) {
      num = sizeof fileName - 1;
    }
    strncpy(fileName, (char *) theRequest->io_Info.ioi_Send.iob_Buffer, num);
    DBUG(("FILECMD_READENTRY '%s' on /\n", fileName));
    theFS = (FileSystem *) FIRSTNODE(&fileFolio.ff_Filesystems);
    while (ISNODE(&fileFolio.ff_Filesystems,theFS)) {
      if (strcasecmp(fileName, theFS->fs_FileSystemName) == 0 ||
	  strcasecmp(fileName, theFS->fs_MountPointName) == 0) {
	break;
      } else {
	theFS = (FileSystem *) NEXTNODE(theFS);
      }
    }
    break;
  case FILECMD_OPENENTRY:
    return 0;
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  if (!ISNODE(&fileFolio.ff_Filesystems,theFS)) {
    DBUG(("No file\n"));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
  }
  DBUG(("Found it, creating de\n"));
  rootDirectory = theFS->fs_RootDirectory;
  strncpy(de.de_FileName, theFS->fs.n_Name,
	  FILESYSTEM_MAX_NAME_LEN);
  de.de_UniqueIdentifier = theFS->fs_RootDirectory->fi_UniqueIdentifier;
  de.de_Type = rootDirectory->fi_Type;
  de.de_BlockSize = rootDirectory->fi_BlockSize;
  de.de_AvatarCount = rootDirectory->fi_LastAvatarIndex + 1;
  de.de_ByteCount = rootDirectory->fi_ByteCount;
  de.de_BlockCount = rootDirectory->fi_BlockCount;
  de.de_Flags = rootDirectory->fi_Flags;
  de.de_Version = rootDirectory->fi_Version;
  de.de_Revision = rootDirectory->fi_Revision;
  de.de_Location = rootDirectory->fi_AvatarList[0];
  memcpy(&de.de_Date, &rootDirectory->fi_Date, sizeof(de.de_Date));
  deSize = theRequest->io_Info.ioi_Recv.iob_Len;
  if (deSize > sizeof (DirectoryEntry)) {
    deSize = sizeof (DirectoryEntry);
  }
  DBUG(("Copying de, %d bytes\n", deSize));
  memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer, &de, deSize);
  theRequest->io_Actual = deSize;
  DBUG(("Copy done, return\n"));
  return 0;
}

int32 FileDriverDoit(IOReq *theRequest)
{
  OFile *theOpenFile;
  FileSystem *theFileSystem;
  int32 err;
  theOpenFile = (OFile *) theRequest->io_Dev;
  theFileSystem = theOpenFile->ofi_File->fi_FileSystem;
  DBUG(("I/O command 0x%X for file '%s'\n", theRequest->io_Info.ioi_Command,
	 theOpenFile->ofi_File->fi.n_Name));
  if (!theFileSystem) {
    err = RootFileIO(theRequest);
    theRequest->io_Error = err;
    SuperCompleteIO(theRequest);
    return 1;
  }
  if (theFileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE) {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
    SuperCompleteIO(theRequest);
    return MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
  }
  DBUG(("Dispatching request\n"));
  err = (*theFileSystem->fs_Type->fst_QueueRequest)(theFileSystem, theRequest);
  if (err < 0) {
    DBUG(("Request rejected by device, code 0x%x\n", err));
    theRequest->io_Error = err;
    SuperCompleteIO(theRequest);
  } else if (err > 0) {
    if ((theRequest->io_Flags & IO_INTERNAL) == 0) {
      SuperCompleteIO(theRequest);
    }
  }
  return err;
}

static int32 FileDriverStatus(IOReq *theRequest)
{
  FileStatus status;
  OFile *theOpenFile;
  File *theFile;
  FileSystem *fs;
  DBUG(("File-driver status call via request at %d\n", theRequest));
  theOpenFile = (OFile *) theRequest->io_Dev;
  theFile = theOpenFile->ofi_File;
  fs = theFile->fi_FileSystem;
  memset(&status, 0, sizeof status);
  status.fs.ds_DriverIdentity = DI_FILE;
  status.fs.ds_FamilyCode = DS_DEVTYPE_FILE;
  status.fs.ds_MaximumStatusSize = sizeof status;
  status.fs.ds_DeviceBlockSize = theFile->fi_BlockSize;
  status.fs.ds_DeviceBlockCount = theFile->fi_BlockCount;
  status.fs.ds_DeviceFlagWord = theFile->fi_Flags;
  if (theFile->fi_Flags & FILE_IS_READONLY) {
    status.fs.ds_DeviceUsageFlags |= DS_USAGE_READONLY;
  }
  if (theFile->fi_Flags & FILE_STATIC_MAPPABLE) {
    status.fs.ds_DeviceUsageFlags |= DS_USAGE_STATIC_MAPPABLE;
  }
  status.fs_ByteCount = theFile->fi_ByteCount;
  if (fs) {
    status.fs_XIPFlags = fs->fs_XIPFlags;
    if (fs->fs_Flags & FILESYSTEM_IS_OFFLINE) {
      status.fs.ds_DeviceUsageFlags = DS_USAGE_OFFLINE;
    }
  }

  status.fs_Flags    = theFile->fi_Flags;
  status.fs_Type     = theFile->fi_Type;
  status.fs_Version  = theFile->fi_Version;
  status.fs_Revision = theFile->fi_Revision;
  status.fs_Date     = theFile->fi_Date;

  DBUG(("Status on %s: %d bytes, %d blocks\n", theFile->fi_FileName,
	       status.fs_ByteCount, status.fs.ds_DeviceBlockCount));
  memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer,
	 &status,
	 (int32) ((theRequest->io_Info.ioi_Recv.iob_Len < sizeof status) ?
		theRequest->io_Info.ioi_Recv.iob_Len :  sizeof status));
  SuperCompleteIO(theRequest);
  DBUG(("Status call completed\n"));
  return 1;
}

int32 FileDriverGetPath(IOReq *theRequest)
{
  OFile *theOpenFile;
  File *theFile;
  theOpenFile = (OFile *) theRequest->io_Dev;
  theFile = theOpenFile->ofi_File;
  theRequest->io_Error =
    internalGetPath(theFile,
		    (char *) theRequest->io_Info.ioi_Recv.iob_Buffer,
		    (int32) theRequest->io_Info.ioi_Recv.iob_Len);
  SuperCompleteIO(theRequest);
  return 1;
}

