/* @(#) HostFileDriver.c 96/09/10 1.9 */

/*
  HostFileDriver.c - loadable driver code for Macintosh "remote" filesystems
  and other host-like stuff.
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

extern int32 CloseOpenFile(OFile *theOpenFile);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);
extern Err FilesystemEvent(FileSystem *fs, uint32 eventCode);

static void StartHostFSIo (HostFS *theFS);
static void AbortHostFSIo (IOReq *theRequest);
static IOReq *HostFSEndAction(IOReq *theRequest);
static void CloseHostFile (File *theFile);
static int32 DeleteHostFSIOReq (FileIOReq *req);

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

static Err HostDispatch(FileSystem *fs, IOReq *theRequest);
static void HostTimeslice(FileSystem *fs);
static Err ActQueHost(FileSystem *fs, enum FSActQue actionCode);
static Err UnbindHostFileSystem(FileSystemType *fst);

extern void GiveDaemon(void *foo);
extern IOReq *StatusEndAction(IOReq *statusRequest);
extern int32 StartStatusRequest(FileSystem *fs, IOReq *rawRequest,
				IOReq *(*callBack)(IOReq *));
extern const char whatstring[];
extern const char copyright[];


int32 main(int argc, char **argv)
{
  FileSystemType *fst = (FileSystemType *) argv;

  if (argc < 0)
      return 0;

  DBUG(("Host file driver is registering itself\n"));
  DBUG(("Host fs init, filefolio 0x%X, task at 0x%X signal 0x%X\n",
	 &fileFolio,
	 fileFolio.ff_Daemon.ffd_Task,
	 fileFolio.ff_Daemon.ffd_RescheduleSignal));
  fst->fst_ActQue = ActQueHost;
  fst->fst_QueueRequest = HostDispatch;
  fst->fst_Timeslice = HostTimeslice;
  fst->fst_AbortIO = AbortHostFSIo;
  fst->fst_CloseFile = CloseHostFile;
  fst->fst_DeleteIOReq = DeleteHostFSIOReq;
  fst->fst_Unloader = UnbindHostFileSystem;
  return 0;
}


static Err UnbindHostFileSystem(FileSystemType *fst)
{
  fst->fst_ActQue = NULL;
  fst->fst_QueueRequest = NULL;
  fst->fst_Timeslice = NULL;
  fst->fst_AbortIO = NULL;
  fst->fst_CloseFile = NULL;
  fst->fst_DeleteIOReq = NULL;
  fst->fst_Unloader = NULL;
  return 0;
}

static Err ActQueHost(FileSystem *fs, enum FSActQue actionCode) {
  DBUG(("Host filesystem activate/quiesce hook has been called for /%s\n",
	 fs->fs_FileSystemName));
  switch (actionCode) {
  case ActivateFS:
    DBUG(("Activating host file system /%s\n", fs->fs_FileSystemName));
    fs->fs_Flags &= ~FILESYSTEM_IS_QUIESCENT;
    return 0;
  case QuiesceFS:
    if (fs->fs_RootDirectory->fi_UseCount == 1) {
      DBUG(("Quiescing host file system /%s\n", fs->fs_FileSystemName));
      fs->fs_Flags = (fs->fs_Flags & ~FILESYSTEM_WANTS_QUIESCENT) |
	FILESYSTEM_IS_QUIESCENT;
      return 0;
    } else {
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
    }
  default:
    return 0;
  }
}

static void CloseHostFile (File *theFile) {
  HostFS *hfs;
  HostToken *token;
  int32 interrupts;
  interrupts = Disable();
  token = (HostToken *) theFile->fi_FilesystemSpecificData;
  if (token) {
    hfs = (HostFS *) theFile->fi_FileSystem;
    AddTail(&hfs->hfs_TokensToRelease, (Node *) token);
    DBUG(("Purging file '%s', queue token %d for release\n",
	  theFile->fi_FileName, token->ht_TokenValue));
    theFile->fi_FilesystemSpecificData = 0;
    if (!hfs->hfs.fs_DeviceBusy) {
      SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			  fileFolio.ff_Daemon.ffd_QueuedSignal);
    }
  }
  Enable(interrupts);
}

static int32 DeleteHostFSIOReq (FileIOReq *ior)
{
  IOReq *rawRequest;
  rawRequest = (IOReq *) ior->fio.io_Extension[0];
  ior->fio.io_Extension[0] = 0;
  if (rawRequest) {
    return SuperInternalDeleteItem(rawRequest->io.n_Item);
  } else {
    return 0;
  }
}

static Err MapHostRequest(IOReq *rawRequest, FileIOReq *userRequest,
			  File *theFile)
{
  File *childFile;
  HostToken *token;

  if (!userRequest->fio.io_Extension[0]) {
    /* first command being processed.  Set the block size. */
    userRequest->fio.io_Extension[0] = (uint32) rawRequest;
    if (!(theFile->fi_Flags & FILE_IS_DIRECTORY)) {
      DBUG(("Setting host block size of %s to %d\n", theFile->fi_FileName,
	     theFile->fi_BlockSize));
      rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETBLOCKSIZE;
      rawRequest->io_Info.ioi_Offset = theFile->fi_BlockSize;
      return 0;
    }
  }
  switch (userRequest->fio.io_Info.ioi_Command) {
  case FILECMD_OPENENTRY:
    childFile = (File *)
      CheckItem(userRequest->fio.io_Info.ioi_CmdOptions, FILEFOLIO, FILENODE);
    if (!childFile || childFile->fi_FilesystemSpecificData) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    token = (HostToken *) SuperAllocMem(sizeof (HostToken), MEMTYPE_DMA |
					MEMTYPE_FILL);
    if (!token) {
      return MakeFErr(ER_SEVER,ER_C_STND, ER_NoMem);
    }
    userRequest->fio.io_Extension[1] = (uint32) token;
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_OPENENTRY;
    rawRequest->io_Info.ioi_Send.iob_Buffer = childFile->fi_FileName;
    rawRequest->io_Info.ioi_Send.iob_Len = strlen(childFile->fi_FileName)+1;
    break;
  case FILECMD_ADDENTRY:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_CREATEFILE;
    break;
  case FILECMD_ADDDIR:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_CREATEDIR;
    break;
  case FILECMD_DELETEDIR:
  case FILECMD_DELETEENTRY:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_DELETEENTRY;
    break;
  case FILECMD_RENAME:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_RENAMEENTRY;
    break;
  case FILECMD_READENTRY:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_READENTRY;
    break;
  case FILECMD_READDIR:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_READDIR;
    break;
  case FILECMD_ALLOCBLOCKS:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_ALLOCBLOCKS;
    break;
  case FILECMD_SETEOF:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETEOF;
    break;
  case FILECMD_SETTYPE:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETTYPE;
    break;
  case FILECMD_SETVERSION:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETVERSION;
    break;
  case FILECMD_SETDATE:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETDATE;
    break;
  case FILECMD_SETBLOCKSIZE:
    if (((OFile *) userRequest->fio.io_Dev)->ofi_File->fi_BlockCount != 0) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_SETBLOCKSIZE;
    break;
  case FILECMD_FSSTAT:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_FSSTAT;
    rawRequest->io_Info.ioi_Send = rawRequest->io_Info.ioi_Recv;
    break;
  case CMD_GETICON:
    rawRequest->io_Info.ioi_Command = CMD_GETICON;
    break;
  case CMD_BLOCKREAD:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_BLOCKREAD;
    break;
  case CMD_BLOCKWRITE:
    rawRequest->io_Info.ioi_Command = HOSTFS_CMD_BLOCKWRITE;
    break;
  default:
    DBUG(("Unsupported host filesystem command 0x%08X\n",
	  userRequest->fio.io_Info.ioi_Command));
    break;
  }
  return 0;
}

static void SendHostRequest(FileIOReq *theRequest,
			   IOReq *rawRequest,
			   HostFS *theFS)
{
  Err err;
  int32 interrupts;
  File *theFile;
  HostToken *token;
  theFile = ((OFile *) theRequest->fio.io_Dev)->ofi_File;
  token = (HostToken *) theFile->fi_FilesystemSpecificData;
  if (!token) {
    DBUG(("Got a host IOReq, but there's no token!\n"));
    theRequest->fio.io_Error = MakeFErr(ER_SEVERE,ER_C_STND,ER_SoftErr);
    SuperCompleteIO((IOReq *) theRequest);
    return;
  }
  rawRequest->io_CallBack = HostFSEndAction;
  rawRequest->io_Error = 0;
  rawRequest->io_Flags = 0;
  rawRequest->io_Info = theRequest->fio.io_Info;
  rawRequest->io_Info.ioi_Flags = 0;
  rawRequest->io_Info.ioi_CmdOptions = token->ht_TokenValue;
  rawRequest->io_Info.ioi_UserData = theRequest;
  DBUG(("Mapping host I/O request 0x%x to 0x%x\n", theRequest, rawRequest));
  err = MapHostRequest(rawRequest, theRequest, theFile);
  if (err < 0) {
    DBUG(("Host request-mapper refused request\n"));
    theRequest->fio.io_Error = err;
    SuperCompleteIO((IOReq *) theRequest);
    return;
  } else {
    DBUG(("Raw I/O request for offset %d, buf 0x%X, bytes %d,",
	  rawRequest->io_Info.ioi_Offset,
	  rawRequest->io_Info.ioi_Recv.iob_Buffer,
	  rawRequest->io_Info.ioi_Recv.iob_Len));
    DBUG((" cmd 0x%X, endaction 0x%X\n",
	  rawRequest->io_Info.ioi_Command,
	  rawRequest->io_CallBack));
    DBUG((" for token %d\n",
	  rawRequest->io_Info.ioi_CmdOptions));
    interrupts = Disable();
    AddTail(&theFS->hfs.fs_RequestsRunning, (Node *) theRequest);
    theFS->hfs_UserRequestsRunning ++;
    err = SuperinternalSendIO(rawRequest);
    Enable(interrupts);
    if (err < 0) {
      qprintf(("Error %0x%X from SuperinternalSendIO on host fs!\n", err));
      ERR(err);
    }
  }
  return;
}

static Err HostDispatch(FileSystem *fs, IOReq *theRequest)
{
  uint32 interrupts;
  HostFS *hfs = (HostFS *) fs;
  IOReq *rawRequest;
  File *theFile = ((OFile *) theRequest->io_Dev)->ofi_File;
  DBUG(("Host request 0x%X, fs 0x%X, task 0x%X\n", theRequest, fs,
	CURRENTTASKITEM));
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_BLOCKREAD:
    if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0) {
      DBUG(("Bad block size, reject\n"));
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case CMD_BLOCKWRITE:
    if (theRequest->io_Info.ioi_Send.iob_Len % theFile->fi_BlockSize != 0) {
      DBUG(("Bad block size, reject\n"));
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case CMD_GETICON:
  case FILECMD_GETPATH:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_RENAME:
  case FILECMD_SETTYPE:
  case FILECMD_OPENENTRY:
  case FILECMD_ADDDIR:
  case FILECMD_DELETEDIR:
  case FILECMD_SETVERSION:
  case FILECMD_SETDATE:
  case FILECMD_FSSTAT:
  case FILECMD_SETBLOCKSIZE:
    break;
  case FILECMD_READDIR:
  case FILECMD_READENTRY:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_SETFLAGS:
    theFile->fi_Flags = (theFile->fi_Flags & ~FILEFLAGS_SETTABLE) |
      (theRequest->io_Info.ioi_CmdOptions & FILEFLAGS_SETTABLE);
    DBUG(("Flags set, command done.\n"));
    return 1;
  default:
    DBUG(("Bad command\n"));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  theRequest->io_Flags &= ~IO_QUICK;
  rawRequest = (IOReq *) theRequest->io_Extension[0];
  if (rawRequest) {
    DBUG(("Sending host request\n"));
    SendHostRequest((FileIOReq *) theRequest, rawRequest, hfs);
  } else {
    DBUG(("Queueing for daemon\n"));
    interrupts = Disable();
    InsertNodeFromTail(&hfs->hfs.fs_RequestsToDo, (Node *) theRequest);
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_QueuedSignal);
    Enable(interrupts);
  }
  DBUG(("Request queued or sent\n"));
  return 0;
}

static void HostTimeslice(FileSystem *fs)
{
  DBUG(("Host timeslice!\n"));
  StartHostFSIo((HostFS *) fs);
}

static void StartHostFSIo (HostFS *theFS)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  int32 err;
  Item ioReqItem;
  HostToken *token;
  int32 interrupts;
  DBUG(("Host I/O initiator\n"));
/*
   First, check to see if we have a host token which needs to be released
   (i.e. the file using it has been closed out).  If so, and if we are
   not already in the process of releasing a token, then set
   up the token-release and send it.
*/
  if (!theFS->hfs.fs_DeviceBusy &&
      !IsEmptyList(&theFS->hfs_TokensToRelease)) {
    interrupts = Disable();
    token = (HostToken *) RemHead(&theFS->hfs_TokensToRelease);
    if (token->ht_TokenValid) {
      DBUG(("Releasing token %d\n", token->ht_TokenValue));
      rawRequest = theFS->hfs_RawDeviceRequest;
      memset(&rawRequest->io_Info, 0, sizeof (IOInfo));
      rawRequest->io_CallBack = HostFSEndAction;
      rawRequest->io_Info.ioi_UserData = (void *) theFS;
      rawRequest->io_Info.ioi_Command = HOSTFS_CMD_CLOSEENTRY;
      rawRequest->io_Info.ioi_CmdOptions = token->ht_TokenValue;
      theFS->hfs.fs_DeviceBusy = TRUE;
      err = SuperinternalSendIO(rawRequest);
      if (err < 0) {
	qprintf(("Error %0x%X from SuperinternalSendIO on host fs!\n", err));
	ERR(err);
	theFS->hfs.fs_DeviceBusy = FALSE;
      }
    }
    Enable(interrupts);
    SuperFreeMem(token, sizeof (HostToken));
  }
/*
  If not busy, do the CMD_STATUS recheck stuff.
*/
  if (!theFS->hfs.fs_DeviceBusy) {
    if (theFS->hfs.fs_Flags & FILESYSTEM_WANTS_RECHECK) {
      theFS->hfs.fs_DeviceBusy = TRUE;
      DBUG(("Start status request\n"));
      (void) StartStatusRequest((FileSystem *) theFS,
				theFS->hfs_RawDeviceRequest,
				StatusEndAction);
    } else if (theFS->hfs.fs_DeviceStatusTemp) {
      DBUG(("Releasing temp buffer for /%s at 0x%X\n",
	    theFS->hfs.fs_FileSystemName, theFS->hfs.fs_DeviceStatusTemp));
      SuperFreeMem(theFS->hfs.fs_DeviceStatusTemp, sizeof (DeviceStatus));
      theFS->hfs.fs_DeviceStatusTemp = NULL;
    }
  }
/*
   Now, glom through all of the dispatched-but-not-processed lists and
   run 'em.
*/
  ioReqItem = 0;
  while (1 /* !theFS->hfs.fs_DeviceBusy */) {
    interrupts = Disable();
    theRequest = (FileIOReq *) RemHead(&theFS->hfs.fs_RequestsToDo);
    if (!theRequest) {
      Enable(interrupts);
      break;
    }
    Enable(interrupts);
    if (theRequest->fio.io_Error < 0) {
      DBUG(("CompleteIO 0x%x\n", theRequest));
      SuperCompleteIO((IOReq *) theRequest);
      continue;
    }
    rawRequest = (IOReq *) theRequest->fio.io_Extension[0];
    if (!rawRequest) {
      if (!theFS->hfs_DaemonHasOpened) {
	DBUG(("Daemon opening the device\n"));
	err = SuperOpenItem(theFS->hfs.fs_RawDevice->dev.n_Item, 0);
	if (err < 0) {
	  DBUG(("Filesystem daemon could not open host device!\n"));
	  ERR(err);
	  theRequest->fio.io_Error = ioReqItem;
	  DBUG(("CompleteIO 0x%x\n", theRequest));
	  SuperCompleteIO((IOReq *) theRequest);
	  continue;
	}
	theFS->hfs_DaemonHasOpened = TRUE;
      }
      DBUG(("Creating host-device IOReq for user request\n"));
      ioReqItem = SuperCreateIOReq(NULL, theRequest->fio.io.n_Priority,
				   theFS->hfs.fs_RawDevice->dev.n_Item, 0);
      if (ioReqItem < 0)     {
	DBUG(("Error 0x%x creating ioreq\n", ioReqItem));
	ERR(ioReqItem);
	theRequest->fio.io_Error = ioReqItem;
	DBUG(("CompleteIO 0x%x\n", theRequest));
	SuperCompleteIO((IOReq *) theRequest);
	continue;
      }
      rawRequest = (struct IOReq *) LookupItem(ioReqItem);
    }
    SendHostRequest(theRequest, rawRequest, theFS);
  }
  return;
}

/*
   Very simple abort strategy - just mark it aborted and return.  If
   it's still on the to-do queue, it'll be flushed when the scheduler
   sees it.  If it's on the running queue, let it run to completion.

   A better strategy would be to flush immediately if pending, and abort
   the underlying request if in progress.  This will require a flag to tell
   us what the request's current state is.
*/

static void AbortHostFSIo(IOReq *theRequest)
{
  theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
  return;
}

static int32 PostprocessHostRequest(FileIOReq *userRequest, IOReq *rawRequest,
				    HostFS *theFS)
{
  HostToken *token;
  File *childFile;
  DirectoryEntry *de;
  userRequest->fio.io_Error = rawRequest->io_Error;
  userRequest->fio.io_Actual = rawRequest->io_Actual;
  if (rawRequest->io_Error < 0) {
    DBUG(("Postprocess, error 0x%X\n", rawRequest->io_Error));
  }
  switch (rawRequest->io_Info.ioi_Command) {
  case HOSTFS_CMD_SETBLOCKSIZE:
    DBUG(("Postprocessing SetBlockSize command\n"));
    if (rawRequest->io_Error >= 0) {
      if (userRequest->fio.io_Info.ioi_Command == FILECMD_SETBLOCKSIZE) {
	DBUG(("SETBLOCKSIZE worked, change my block size\n"));
	((OFile *)userRequest->fio.io_Dev)->ofi_File->fi_BlockSize =
	  rawRequest->io_Info.ioi_Offset;
	break;
      }
      DBUG(("SETBLOCKSIZE worked, kick off real I/O\n"));
      InsertNodeFromTail(&theFS->hfs.fs_RequestsToDo, (Node *) userRequest);
      return TRUE;
    }
    break;
  case HOSTFS_CMD_OPENENTRY:
    DBUG(("Postprocessing OpenEntry command\n"));
    childFile = (File *)
      CheckItem(userRequest->fio.io_Info.ioi_CmdOptions, FILEFOLIO, FILENODE);
    DBUG(("Child file is at 0x%X\n", childFile));
    token = (HostToken *) userRequest->fio.io_Extension[1];
    DBUG(("Token structure at 0x%X, new token is %d\n", token,
	   rawRequest->io_Info.ioi_CmdOptions));
    token->ht_TokenValue = rawRequest->io_Info.ioi_CmdOptions;
    if (userRequest->fio.io_Error >= 0) {
      token->ht_TokenValid = TRUE;
    }
    childFile->fi_FilesystemSpecificData = (uint32) token;
    break;
  case HOSTFS_CMD_READDIR:
  case HOSTFS_CMD_READENTRY:
    if (userRequest->fio.io_Info.ioi_Recv.iob_Len >= sizeof (DirectoryEntry)) {
      de = (DirectoryEntry *) userRequest->fio.io_Info.ioi_Recv.iob_Buffer;
      if (de->de_Flags & FILE_IS_DIRECTORY) {
	de->de_Flags &= ~ FILE_INFO_NOT_CACHED;
      }
      DBUG(("Got directory entry %s\n", de->de_FileName));
    }
    break;
  default:
    break;
  }
  return FALSE;
}

static IOReq *HostFSEndAction(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  HostFS *theFS;
  int32 signalDaemon;
  DBUG(("Host device endaction on IOReq 0x%X, error 0x%X\n", rawRequest,
	rawRequest->io_Error));
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_UserData;
  signalDaemon = FALSE;
  if (!userRequest) {
    DBUG(("No user request!\n"));
    return NULL; /* this is really a panic() situation! */
  }
  if (userRequest->fio.io.n_SubsysType == NST_FILESYS) {
    theFS = (HostFS *) userRequest;
    theFS->hfs.fs_DeviceBusy = FALSE;
    DBUG(("Token released\n"));
    if (!IsEmptyList(&theFS->hfs_TokensToRelease)) {
      signalDaemon = TRUE;
    }
  } else {
    DBUG(("Postprocessing user IOReq 0x%X\n", userRequest));
    theFS = (HostFS *)
      ((OFile *) userRequest->fio.io_Dev)->ofi_File->fi_FileSystem;
    RemNode((Node *) userRequest);
    signalDaemon = PostprocessHostRequest(userRequest, rawRequest, theFS);
    theFS->hfs_UserRequestsRunning --;
    if (!signalDaemon) {
      DBUG(("CompleteIO 0x%x\n", userRequest));
      SuperCompleteIO((IOReq *) userRequest);
    }
  }
  if (signalDaemon) {
    DBUG(("Signal daemon, filefolio 0x%X, task at 0x%X signal 0x%X\n",
	   &fileFolio,
	   fileFolio.ff_Daemon.ffd_Task,
	   fileFolio.ff_Daemon.ffd_RescheduleSignal));
    DBUG(("Daemon name is '%s'\n", fileFolio.ff_Daemon.ffd_Task));
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_RescheduleSignal);
  }
  return (IOReq *) NULL;
}
