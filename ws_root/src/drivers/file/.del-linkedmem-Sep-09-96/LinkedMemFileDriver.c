/* @(#) LinkedMemFileDriver.c 96/07/17 1.8 */

/*
  LinkedMemFileSystem.c - support code for flat linked-memory-block
  file systems.
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
#include <stdlib.h>
#include <string.h>

#undef DEBUG

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x)  /* x */
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern int32 CloseOpenFile(OFile *theOpenFile);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);

static Err LinkedMemFSDispatch(FileSystem *fs, IOReq *theRequest);
static void LinkedMemFSTimeslice(FileSystem *fs);
static void ScheduleLinkedMemFSIo(LinkedMemFS *theDevice);
static void StartLinkedMemFSIo (LinkedMemFS *theDevice);
static void AbortLinkedMemFSIo (IOReq *theRequest);
static void LinkedMemDoStat(LinkedMemFileEntry *fep, FileSystemStat *fsp,
		     uint32 curblk);
static IOReq *LinkedMemFSEndAction(IOReq *theRequest);
static Err LinkedMemFiniteStateMachine(LinkedMemFS *theDevice, FileIOReq *userReq);
static Err ActQueLinkedMem(FileSystem *fs, enum FSActQue actionCode);
static Err UnbindLinkedMemFileSystem(FileSystemType *fst);

extern IOReq *StatusEndAction(IOReq *statusRequest);
extern int32 StartStatusRequest(FileSystem *fs, IOReq *rawRequest,
				IOReq *(*callBack)(IOReq *));

int32 main(int argc, char **argv)
{
  FileSystemType *fst = (FileSystemType *) argv;

  if (argc < 0)
      return 0;

  DBUG(("Linked-memory file driver is registering itself\n"));
  fst->fst_QueueRequest = LinkedMemFSDispatch;
  fst->fst_Timeslice = LinkedMemFSTimeslice;
  fst->fst_AbortIO = AbortLinkedMemFSIo;
  fst->fst_ActQue = ActQueLinkedMem;
  fst->fst_Unloader = UnbindLinkedMemFileSystem;
  return 0;
}

static Err UnbindLinkedMemFileSystem(FileSystemType *fst)
{
  fst->fst_QueueRequest = NULL;
  fst->fst_Timeslice = NULL;
  fst->fst_AbortIO = NULL;
  fst->fst_ActQue = NULL;
  fst->fst_Unloader = NULL;
  return 0;
}

static Err ActQueLinkedMem(FileSystem *fs, enum FSActQue actionCode) {
  switch (actionCode) {
  case ActivateFS:
    DBUG(("Activating linked-memory file system /%s\n",
	     fs->fs_FileSystemName));
    fs->fs_Flags &= ~FILESYSTEM_IS_QUIESCENT;
    return 0;
  case QuiesceFS:
    if (fs->fs_RootDirectory->fi_UseCount == 1) {
      DBUG(("Quiescing linked-memory file system /%s\n",
	       fs->fs_FileSystemName));
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

static Err LinkedMemFSDispatch(FileSystem *fs, IOReq *theRequest)
{
  uint32 interrupts;
  LinkedMemFS *lmfs = (LinkedMemFS *) fs;
  File *theFile = ((OFile *) theRequest->io_Dev)->ofi_File;
  DBUG(("LinkedMemFS I/O dispatch, command 0x%X\n",
	 theRequest->io_Info.ioi_Command));
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
  case CMD_BLOCKREAD:
    if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0 ||
	theRequest->io_Info.ioi_Recv.iob_Len > theFile->fi_BlockSize *
	(theFile->fi_BlockCount - theRequest->io_Info.ioi_Offset)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case CMD_WRITE:
  case CMD_BLOCKWRITE:
    if (theRequest->io_Info.ioi_Send.iob_Len % theFile->fi_BlockSize != 0 ||
	theRequest->io_Info.ioi_Send.iob_Len > theFile->fi_BlockSize *
	(theFile->fi_BlockCount - theRequest->io_Info.ioi_Offset)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_OPENENTRY:
    return 1;
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_SETTYPE:
  case FILECMD_FSSTAT:
  case CMD_GETICON:
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
    return 1;
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  interrupts = Disable();
  InsertNodeFromTail(&lmfs->lmfs.fs_RequestsToDo, (Node *) theRequest);
  theRequest->io_Flags &= ~IO_QUICK;
  Enable(interrupts);
  SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
		      fileFolio.ff_Daemon.ffd_QueuedSignal);
  return 0;
}

static void LinkedMemFSTimeslice(FileSystem *fs)
{
  if (fs->fs_DeviceBusy) {
    return;
  }
  ScheduleLinkedMemFSIo((LinkedMemFS *) fs);
#ifdef THIS_CANNOT_EVER_BE_TRUE
  if (fs->fs_DeviceBusy) {
    return;
  }
#endif
  StartLinkedMemFSIo((LinkedMemFS *) fs);
  if (!fs->fs_DeviceBusy) {
    if (fs->fs_Flags & FILESYSTEM_WANTS_RECHECK) {
      fs->fs_DeviceBusy = TRUE;
      (void) StartStatusRequest(fs,
				((LinkedMemFS *) fs)->lmfs_RawDeviceRequest,
				StatusEndAction);
    } else if (fs->fs_DeviceStatusTemp) {
      SuperFreeMem(fs->fs_DeviceStatusTemp, sizeof (DeviceStatus));
      fs->fs_DeviceStatusTemp = NULL;
    }
  }
}

static void ScheduleLinkedMemFSIo(LinkedMemFS *lmfs)
{
  FileIOReq *theRequest;
  while (!ISEMPTYLIST(&lmfs->lmfs.fs_RequestsToDo)) {
    theRequest = (FileIOReq *) FIRSTNODE(&lmfs->lmfs.fs_RequestsToDo);
    RemNode((Node *) theRequest);
    AddTail(&lmfs->lmfs.fs_RequestsRunning, (Node *) theRequest);
  }
}

static Err PrepareLinkedMemFSIO(LinkedMemFS *lmfs, FileIOReq *theRequest)
{
  IOReq *rawRequest;
  File *theFile;
  int32 avatar, absoluteOffset, absoluteBlock;
  Err err;
  int32 len;
  theFile = ((OFile *) theRequest->fio.io_Dev)->ofi_File;
  DBUG(("LM prepare command 0x%X\n", theRequest->fio.io_Info.ioi_Command));
  rawRequest = lmfs->lmfs_RawDeviceRequest;
  rawRequest->io_CallBack = LinkedMemFSEndAction;
  rawRequest->io_Info = theRequest->fio.io_Info;
  rawRequest->io_Info.ioi_UserData = theRequest;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
  rawRequest->io_Info.ioi_Flags = 0;
  switch (theRequest->fio.io_Info.ioi_Command) {
  case CMD_READ:
  case CMD_BLOCKREAD:
  case CMD_WRITE:
  case CMD_BLOCKWRITE:
    avatar = theFile->fi_AvatarList[0];
    absoluteOffset = theRequest->fio.io_Info.ioi_Offset;
    absoluteBlock = ((avatar & DRIVER_BLOCK_MASK) + absoluteOffset) *
      theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock +
	lmfs->lmfs_FileHeaderBlockSize;
    theRequest->fio_AvatarIndex = 0;
    theRequest->fio_AbsoluteBlockNumber = absoluteBlock;
    theRequest->fio_BlockCount = theRequest->fio.io_Info.ioi_Recv.iob_Len /
      theFile->fi_BlockSize;
    theRequest->fio_DevBlocksPerFileBlock =
      theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
    rawRequest->io_Info.ioi_Command = theRequest->fio.io_Info.ioi_Command;
    rawRequest->io_Info.ioi_Offset =
      (int32) theRequest->fio_AbsoluteBlockNumber +
	lmfs->lmfs_RawDeviceBlockOffset;
    lmfs->lmfs_CurrentFileActingOn = theFile;
    if (theRequest->fio.io_Info.ioi_Command == CMD_READ ||
	theRequest->fio.io_Info.ioi_Command == CMD_BLOCKREAD) {
      lmfs->lmfs_FSM = LMD_Idle;
    } else {
      lmfs->lmfs_FSM = LMD_ExtendEOF;
    }
    DBUG(("User requests offset %d, fs block %d, device block %d\n",
	  absoluteOffset, absoluteBlock, rawRequest->io_Info.ioi_Offset));
    err = 0;
    break;
  case CMD_GETICON:
    rawRequest->io_Info.ioi_Command = theRequest->fio.io_Info.ioi_Command;
    lmfs->lmfs_CurrentFileActingOn = theFile;
    lmfs->lmfs_FSM = LMD_Idle;
    DBUG(("User requests device icon\n"));
    err = 0;
    break;
  case FILECMD_ADDENTRY:
    lmfs->lmfs_CurrentFileActingOn = theFile;
    lmfs->lmfs_DesiredSize =
      (sizeof (LinkedMemFileEntry) +
       theFile->fi_FileSystem->fs_VolumeBlockSize - 1) /
	   theFile->fi_FileSystem->fs_VolumeBlockSize;
    lmfs->lmfs_FSM = LMD_Initialization;
    err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    break;
  case FILECMD_FSSTAT:
    lmfs->lmfs_CurrentFileActingOn = theFile;
    lmfs->lmfs_FSM = LMD_Initialization;
    err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    break;
  case FILECMD_ALLOCBLOCKS:
    lmfs->lmfs_CurrentFileActingOn = theFile;
    lmfs->lmfs_DesiredSize = theFile->fi_BlockCount +
      theRequest->fio.io_Info.ioi_Offset +
	lmfs->lmfs_FileHeaderBlockSize;
    DBUG(("AllocBlocks:  desire a chunk of %d blocks\n", lmfs->lmfs_DesiredSize));
    lmfs->lmfs_FSM = LMD_Initialization;
    lmfs->lmfs_ThisBlockCursor = theFile->fi_AvatarList[0];
    err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    break;
  case FILECMD_READENTRY:
  case FILECMD_DELETEENTRY:
    memset(lmfs->lmfs_CopyBuffer, 0, sizeof lmfs->lmfs_CopyBuffer);
    len = theRequest->fio.io_Info.ioi_Send.iob_Len;
    if (len > FILESYSTEM_MAX_NAME_LEN) {
      len = FILESYSTEM_MAX_NAME_LEN;
    }
    strncpy(lmfs->lmfs_CopyBuffer,
	    (char *) theRequest->fio.io_Info.ioi_Send.iob_Buffer, len);
    /* drop through */
  case FILECMD_READDIR:
    lmfs->lmfs_CurrentFileActingOn = theFile;
    lmfs->lmfs_FSM = LMD_InitScan;
    err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    break;
  case FILECMD_SETEOF:
    DBUG(("Want to set EOF to %d\n", theRequest->fio.io_Info.ioi_Offset));
    if (theRequest->fio.io_Info.ioi_Offset >
	theFile->fi_BlockCount * theFile->fi_BlockSize) {
      DBUG(("Too big!\n"));
      err = MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    } else {
      DBUG(("OK, run FSM\n"));
      lmfs->lmfs_FSM = LMD_ReadToSetEOF;
      lmfs->lmfs_ThisBlockCursor = theFile->fi_AvatarList[0];
      err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    }
    break;
  case FILECMD_SETTYPE:
    DBUG(("Want to set type to %d\n", theRequest->fio.io_Info.ioi_Offset));
    lmfs->lmfs_FSM = LMD_ReadToSetType;
    lmfs->lmfs_ThisBlockCursor = theFile->fi_AvatarList[0];
    err = LinkedMemFiniteStateMachine(lmfs, theRequest);
    break;
  default:
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  return err;
}

static void StartLinkedMemFSIo (LinkedMemFS *lmfs)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  int32 err;
  uint32 interrupts;
  do {
    interrupts = Disable();
    if (lmfs->lmfs.fs_DeviceBusy ||
	ISEMPTYLIST(&lmfs->lmfs.fs_RequestsRunning)) {
      Enable(interrupts);
      return;
    }
    lmfs->lmfs.fs_DeviceBusy = TRUE;
    theRequest = (FileIOReq *) FIRSTNODE(&lmfs->lmfs.fs_RequestsRunning);
    Enable(interrupts);
    err = PrepareLinkedMemFSIO (lmfs, theRequest);
    if (err < 0) {
      interrupts = Disable();
      lmfs->lmfs.fs_DeviceBusy = FALSE;
      RemNode((Node *) theRequest);
      Enable(interrupts);
      theRequest->fio.io_Error = err;
      DBUG(("Error 0x%X from prep routine\n", err));
      SuperCompleteIO((IOReq *) theRequest);
    } else {
      rawRequest = lmfs->lmfs_RawDeviceRequest;
      DBUG(("Raw I/O offset %d, send [0x%lx,%d]",
		   rawRequest->io_Info.ioi_Offset,
		   rawRequest->io_Info.ioi_Send.iob_Buffer,
		   rawRequest->io_Info.ioi_Send.iob_Len));
      DBUG((" recv [0x%lx,%d]\n",
		   rawRequest->io_Info.ioi_Recv.iob_Buffer,
		   rawRequest->io_Info.ioi_Recv.iob_Len));
      err = SuperinternalSendIO(rawRequest);
      if (err < 0) {
	qprintf(("Error %d from SuperinternalSendIO!\n", err));
      } else {
	DBUG(("Sent\n"));
      }
    }
  } while (!lmfs->lmfs.fs_DeviceBusy);
  return;
}

static void AbortLinkedMemFSIo(IOReq *theRequest)
{
  LinkedMemFS *lmfs;
  OFile *theOpenFile;
  theOpenFile = (OFile *) theRequest->io_Dev;
  lmfs = (LinkedMemFS *)theOpenFile->ofi_File->fi_FileSystem;
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O request - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (lmfs->lmfs.fs_DeviceBusy &&
   lmfs->lmfs_RawDeviceRequest->io_Info.ioi_UserData == (void *) theRequest) {
    SuperinternalAbortIO(lmfs->lmfs_RawDeviceRequest);
  } else {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  return;
}

static IOReq *LinkedMemFSEndAction(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  OFile *theOpenFile;
  LinkedMemFS *lmfs;
  File *theFile;
  int32 bytesRead;
  Err err;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_UserData;
  theOpenFile = (OFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  lmfs = (LinkedMemFS *) theFile->fi_FileSystem;
  DBUG(("LMD endaction routine, state %d, error 0x%x, actual %d\n",
	lmfs->lmfs_FSM,
	rawRequest->io_Error,
	rawRequest->io_Actual));
  switch (lmfs->lmfs_FSM) {
  case LMD_Idle:
    bytesRead = rawRequest->io_Actual;
    if ((rawRequest->io_Error & 0x000001FF) == ((ER_C_STND << ERR_CLASHIFT) +
						(ER_Aborted << ERR_ERRSHIFT))) {
      userRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    } else {
      userRequest->fio.io_Error = rawRequest->io_Error;
    }
    userRequest->fio.io_Actual += bytesRead;
    break;
  case LMD_Done:
    lmfs->lmfs_FSM = LMD_Idle;
    break;
  default: /* FSM is still running, prod it */
    err = rawRequest->io_Error;
    if (err < 0) {
      DBUG(("Aborting FSM due to I/O error!\n"));
      userRequest->fio.io_Error = err;
      lmfs->lmfs_FSM = LMD_Idle;
      break;
    }
    err = LinkedMemFiniteStateMachine(lmfs, userRequest);
    if (err > 0) {
      DBUG(("Followon raw I/O offset %d, send [0x%lx,%d]",
	     rawRequest->io_Info.ioi_Offset,
	     rawRequest->io_Info.ioi_Send.iob_Buffer,
	     rawRequest->io_Info.ioi_Send.iob_Len));

      DBUG((" recv [0x%lx,%d]\n",
	     rawRequest->io_Info.ioi_Recv.iob_Buffer,
	     rawRequest->io_Info.ioi_Recv.iob_Len));
      return rawRequest; /* keep running */
    }
    DBUG(("Terminate FSM with error 0x%x\n", err));
    userRequest->fio.io_Error = err;
  }
  lmfs->lmfs.fs_DeviceBusy = FALSE;
/*
  That request is done (successfully or with errors from which we cannot
  recover).  Signal completion and check for a follow-on I/O from an
  upper-level driver.
*/
 wrapup:
  RemNode((Node *) userRequest);
  DBUG(("LM request completed\n"));
  SuperCompleteIO((IOReq *) userRequest);
  if (ISEMPTYLIST(&lmfs->lmfs.fs_RequestsRunning)) {
/*
  Either we've completed the last scheduled I/O, or we've set aside
  some pending requests because of a higher-priority boink.  In either
  case, reschedule to find something to do.
*/
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_RescheduleSignal);
    return (IOReq *) NULL;
  }
  userRequest = (FileIOReq *) FIRSTNODE(&lmfs->lmfs.fs_RequestsRunning);
  err = PrepareLinkedMemFSIO (lmfs, userRequest);
  if (err < 0) {
    userRequest->fio.io_Error = err;
    goto wrapup;
  }
  lmfs->lmfs.fs_DeviceBusy = TRUE;
  rawRequest->io_Actual = 0; /* debug */
  return rawRequest;
}

static Err LinkedMemFiniteStateMachine(LinkedMemFS *lmfs, FileIOReq *userReq)
{
  IOReq *raw;
  File *theFile;
  enum LinkedMemFSFSM nextState;
  enum {
    noAction,
    readThis,
    readOther,
    writeThis,
    writeOther,
    readData,
    writeData} action;
  Err returnVal;
  LinkedMemFileEntry tempBlock;
  FileSystemStat *fstp;
  DirectoryEntry de;
  int32 deSize;
  uint32 newEOF;

  DBUG(("LMD FSM, state %d\n", lmfs->lmfs_FSM));
  theFile = lmfs->lmfs_CurrentFileActingOn;
  nextState = LMD_Fault;
  action = noAction;
  returnVal = 1;
  raw = lmfs->lmfs_RawDeviceRequest;
  switch (lmfs->lmfs_FSM) {
  case LMD_Idle:
  case LMD_Done:
    nextState = LMD_Idle;
    returnVal = 0;
    break;
  case LMD_Fault:
    returnVal = 0;
    break;
  case LMD_FsStat:
    if (lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset <
    	lmfs->lmfs_ThisBlockCursor) {
    	action = noAction;
	/* this is the last block */
    	fstp = (FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer;
	LinkedMemDoStat(&lmfs->lmfs_ThisBlock, fstp,
    			lmfs->lmfs_ThisBlockCursor);
	fstp->fst_BitMap |= (FSSTAT_MAXFILESIZE | FSSTAT_FREE | FSSTAT_USED);
    	nextState = LMD_Done;
    } else {	/* more traversing is needed */
	LinkedMemDoStat(&lmfs->lmfs_ThisBlock,
    		(FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer,
    			lmfs->lmfs_ThisBlockCursor);
    	lmfs->lmfs_ThisBlockCursor =
    	lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
    	action = readThis;
    	nextState = LMD_FsStat;
    }
    break;

  case LMD_Initialization:
    lmfs->lmfs_MergeBlockCursor = 0;
    if (userReq->fio.io_Info.ioi_Command == FILECMD_ADDENTRY) {
      goto TryNewBlock;
    }

    if (userReq->fio.io_Info.ioi_Command == FILECMD_FSSTAT) {
	fstp = (FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer;
        fstp->fst_Used = HOWMANY(sizeof(DiscLabel), fstp->fst_BlockSize);
    	lmfs->lmfs_ThisBlockCursor =
      	theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
    	action = readThis;
    	nextState = LMD_FsStat;
    	break;
    }

    action = readThis;
    nextState = LMD_CheckIsThisLast;
    break;
  case LMD_CheckIsThisLast:
    if (lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset < lmfs->lmfs_ThisBlockCursor) {
      goto TryNewBlock;
    }
    lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_CheckSuccessor;
    break;
  case LMD_CheckSuccessor:
    if (lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint != FINGERPRINT_FREEBLOCK ||
	lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount < lmfs->lmfs_DesiredSize -
	lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount) {
      goto TryNewBlock;
    }
    lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount +=
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount;
    lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset =
      lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset;
    action = writeThis;
    nextState = LMD_CutTheSlack;
    break;
  case LMD_CutTheSlack:
    if (userReq->fio.io_Info.ioi_Command == FILECMD_ADDENTRY) {
      memcpy(&lmfs->lmfs_ThisBlock, &lmfs->lmfs_OtherBlock,
	     sizeof lmfs->lmfs_ThisBlock);
      lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_OtherBlockCursor;
    }
    if (lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount - lmfs->lmfs_DesiredSize <
	sizeof (LinkedMemFileEntry) + LINKED_MEM_SLACK) {
      lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
      action = readOther;
      nextState = LMD_FixOldBackLink;
    } else {
      DBUG(("Trim entry at %d to %d blocks\n", lmfs->lmfs_ThisBlockCursor,
	    lmfs->lmfs_DesiredSize));
      lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_ThisBlockCursor +
	lmfs->lmfs_DesiredSize;
      lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
      lmfs->lmfs_OtherBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
      lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset =
	lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlinkOffset =
	lmfs->lmfs_ThisBlockCursor;
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount =
	lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount - lmfs->lmfs_DesiredSize;
      DBUG(("New successor at %d will have %d blocks\n", lmfs->lmfs_OtherBlockCursor,
	    lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount));
      action = writeOther;
      nextState = LMD_CutOffExcess;
    }
    break;
  case LMD_CutOffExcess:
    lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount = lmfs->lmfs_DesiredSize;
    lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset = lmfs->lmfs_OtherBlockCursor;
    action = writeThis;
    nextState = LMD_GetOldBackLink;
    break;
  case LMD_GetOldBackLink:
    lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_FixOldBackLink;
    break;
  case LMD_FixOldBackLink:
    if (lmfs->lmfs_OtherBlockCursor == lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset) {
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlinkOffset = lmfs->lmfs_ThisBlockCursor;
    } else {
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlinkOffset = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
    }
    action = writeOther;
    nextState = LMD_SuccessfulChomp;
    break;
  case LMD_SuccessfulChomp:
    theFile->fi_BlockCount = lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount -
      lmfs->lmfs_FileHeaderBlockSize;
    if (lmfs->lmfs_MergeBlockCursor != 0) {
      lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_MergeBlockCursor;
      DBUG(("Need to do a merger!\n"));
      goto FetchHeader;
    }
    nextState = LMD_Done;
    returnVal = 0; /* done, success */
    break;
  case LMD_TryNewBlock:
  TryNewBlock: ;
    lmfs->lmfs_OtherBlockCursor =
      theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
    action = readOther;
    nextState = LMD_ExamineNewBlock;
    break;
  case LMD_ExamineNewBlock:
    DBUG(("Examine block at %d\n", lmfs->lmfs_OtherBlockCursor));
    if (lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint != FINGERPRINT_FREEBLOCK ||
	lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount < lmfs->lmfs_DesiredSize) {
      DBUG(("Wanted %d, was %d type 0x%x\n", lmfs->lmfs_DesiredSize,
	    lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount,
	    lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint));
      lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset;
      DBUG(("Advance to %d\n", lmfs->lmfs_OtherBlockCursor));
      if (lmfs->lmfs_OtherBlockCursor ==
	  theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0]) {
	returnVal = MakeFErr(ER_SEVER,ER_C_STND,ER_Fs_NoSpace);
	DBUG(("Wraparound, EOF\n"));
      } else {
	action = readOther;
	nextState = LMD_ExamineNewBlock;
	DBUG(("Do another read\n"));
      }
    } else {
      DBUG(("Acquiring\n"));
      lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FILEBLOCK;
      lmfs->lmfs_OtherBlock.lmfe.lmb_HeaderBlockCount =
	lmfs->lmfs_FileHeaderBlockSize;
      switch (userReq->fio.io_Info.ioi_Command) {
      case FILECMD_ADDENTRY:
	lmfs->lmfs_OtherBlock.lmfe_ByteCount = 0;
	lmfs->lmfs_OtherBlock.lmfe_Type = 0x20202020;
	lmfs->lmfs_OtherBlock.lmfe_UniqueIdentifier = 0;
	strncpy(lmfs->lmfs_OtherBlock.lmfe_FileName,
		(char *) userReq->fio.io_Info.ioi_Send.iob_Buffer,
		sizeof lmfs->lmfs_OtherBlock.lmfe_FileName);
	DBUG(("Add entry, filename is %s\n", lmfs->lmfs_OtherBlock.lmfe_FileName));
	action = writeOther;
	nextState = LMD_CutTheSlack;
	break;
      case FILECMD_ALLOCBLOCKS:
	lmfs->lmfs_OtherBlock.lmfe_ByteCount =
	  lmfs->lmfs_ThisBlock.lmfe_ByteCount;
	lmfs->lmfs_OtherBlock.lmfe_Type =
	  lmfs->lmfs_ThisBlock.lmfe_Type;
	lmfs->lmfs_OtherBlock.lmfe_UniqueIdentifier =
	  lmfs->lmfs_ThisBlock.lmfe_UniqueIdentifier;
	strncpy(lmfs->lmfs_OtherBlock.lmfe_FileName,
		lmfs->lmfs_ThisBlock.lmfe_FileName,
		sizeof lmfs->lmfs_OtherBlock.lmfe_FileName);
	lmfs->lmfs_ContentOffset =
	  lmfs->lmfs_OtherBlock.lmfe.lmb_HeaderBlockCount;
	lmfs->lmfs_BlocksToCopy =
	  (lmfs->lmfs_OtherBlock.lmfe_ByteCount +
	   lmfs->lmfs_BlockSize - 1) / lmfs->lmfs_BlockSize;
	if (lmfs->lmfs_BlocksToCopy > 0) {
	  nextState = LMD_ReadToCopy;
	} else {
	  nextState = LMD_CopyDone;
	}
	action = writeOther;
	lmfs->lmfs_BlocksToRead = 0;
      }
    }
    break;
  case LMD_ReadToCopy:
    lmfs->lmfs_ContentOffset += lmfs->lmfs_BlocksToRead;
    lmfs->lmfs_BlocksToRead = lmfs->lmfs_BlocksToCopy;
    if (lmfs->lmfs_BlocksToRead > lmfs->lmfs_CopyBlockSize) {
      lmfs->lmfs_BlocksToRead = lmfs->lmfs_CopyBlockSize;
    }
    action = readData;
    nextState = LMD_WriteCopiedData;
    break;
  case LMD_WriteCopiedData:
    action = writeData;
    lmfs->lmfs_BlocksToCopy -= lmfs->lmfs_BlocksToRead;
    if (lmfs->lmfs_BlocksToCopy <= 0) {
      nextState = LMD_CopyDone;
    } else {
      nextState = LMD_ReadToCopy;
    }
    break;
  case LMD_CopyDone:
    memcpy(&tempBlock, &lmfs->lmfs_ThisBlock, sizeof tempBlock);
    memcpy(&lmfs->lmfs_ThisBlock, &lmfs->lmfs_OtherBlock, sizeof lmfs->lmfs_ThisBlock);
    memcpy(&lmfs->lmfs_OtherBlock, &tempBlock, sizeof lmfs->lmfs_OtherBlock);
    lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
    lmfs->lmfs_OtherBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
    lmfs->lmfs_MergeBlockCursor = lmfs->lmfs_ThisBlockCursor;
    lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_OtherBlockCursor;
    lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_MergeBlockCursor;
    theFile->fi_AvatarList[0] = lmfs->lmfs_ThisBlockCursor;
    theFile->fi_BlockCount = lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount -
      lmfs->lmfs_FileHeaderBlockSize;
    DBUG(("File avatar[0] set to %d\n", theFile->fi_AvatarList[0]));
    action = writeOther;
    nextState = LMD_CutTheSlack;
    break;
  case LMD_FetchHeader:
  FetchHeader: ;
    action = readThis;
    nextState = LMD_MarkItFree;
    break;
  case LMD_MarkItFree:
  MarkItFree:
    lmfs->lmfs_CurrentEntryIndex = lmfs->lmfs_CurrentEntryOffset = 0;
    lmfs->lmfs_ThisBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
    lmfs->lmfs_ThisBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
    action = writeThis;
    nextState = LMD_BackUpOne;
    break;
  case LMD_BackUpOne:
    if (lmfs->lmfs_ThisBlock.lmfe.lmb_Fingerprint == FINGERPRINT_FREEBLOCK) {
      lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_BlinkOffset;
      nextState = LMD_BackUpOne;
    } else {
      lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
      nextState = LMD_ScanAhead;
    }
    action = readThis;
    break;
  case LMD_ScanAhead:
    lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_AttemptMerge;
    break;
  case LMD_AttemptMerge:
    if (lmfs->lmfs_OtherBlock.lmfe.lmb_Fingerprint == FINGERPRINT_FREEBLOCK) {
      lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount += lmfs->lmfs_OtherBlock.lmfe.lmb_BlockCount;
      lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset = lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset;
      lmfs->lmfs_OtherBlockCursor = lmfs->lmfs_OtherBlock.lmfe.lmb_FlinkOffset;
      action = readOther;
      nextState = LMD_AttemptMerge;
    } else {
      lmfs->lmfs_OtherBlock.lmfe.lmb_BlinkOffset = lmfs->lmfs_ThisBlockCursor;
      action = writeOther;
      nextState = LMD_FixFlink;
    }
    break;
  case LMD_FixFlink:
    action = writeThis;
    nextState = LMD_DoneDeleting;
    break;
  case LMD_DoneDeleting:
    returnVal = 0;
    break;
  case LMD_InitScan:
    lmfs->lmfs_ThisBlockIndex = lmfs->lmfs_CurrentEntryIndex;
    lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_CurrentEntryOffset;
    if (lmfs->lmfs_ThisBlockCursor <= 0 ||
	lmfs->lmfs_ThisBlockIndex <= 0) {
      lmfs->lmfs_ThisBlockCursor = theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
      lmfs->lmfs_ThisBlockIndex = 0;
    }
    lmfs->lmfs_HaltCursor = lmfs->lmfs_ThisBlockCursor;
    action = readThis;
    nextState = LMD_ExamineEntry;
    break;
  case LMD_ExamineEntry:
    switch (lmfs->lmfs_ThisBlock.lmfe.lmb_Fingerprint) {
    case FINGERPRINT_ANCHORBLOCK:
      DBUG(("Found anchor block\n"));
      lmfs->lmfs_ThisBlockIndex = 0;
      break;
    case FINGERPRINT_FILEBLOCK:
      if (lmfs->lmfs_CurrentEntryOffset != lmfs->lmfs_ThisBlockCursor) {
	lmfs->lmfs_ThisBlockIndex ++;
	lmfs->lmfs_CurrentEntryIndex = lmfs->lmfs_ThisBlockIndex;
	lmfs->lmfs_CurrentEntryOffset = lmfs->lmfs_ThisBlockCursor;
      }
      DBUG(("Now at entry index %d file %s\n",
	    lmfs->lmfs_CurrentEntryIndex,
	    lmfs->lmfs_ThisBlock.lmfe_FileName));
      DBUG(("  want entry index %d file %s\n",
	    userReq->fio.io_Info.ioi_Offset,
	    lmfs->lmfs_CopyBuffer));
      if (userReq->fio.io_Info.ioi_Command == FILECMD_DELETEENTRY &&
	  strcasecmp(lmfs->lmfs_ThisBlock.lmfe_FileName,
		      lmfs->lmfs_CopyBuffer) == 0) {
	DBUG(("Found proper index/entry!\n"));
	goto MarkItFree;
      } else if ((userReq->fio.io_Info.ioi_Command == FILECMD_READDIR &&
	   lmfs->lmfs_ThisBlockIndex == userReq->fio.io_Info.ioi_Offset) ||
	  (userReq->fio.io_Info.ioi_Command == FILECMD_READENTRY &&
	   strcasecmp(lmfs->lmfs_ThisBlock.lmfe_FileName,
		       lmfs->lmfs_CopyBuffer) == 0)) {
	returnVal = 0;
	DBUG(("Found proper index/entry!\n"));
	strncpy(de.de_FileName, lmfs->lmfs_ThisBlock.lmfe_FileName,
		FILESYSTEM_MAX_NAME_LEN);
	de.de_BlockSize = theFile->fi_BlockSize;
	DBUG(("Entry block size is %d\n", theFile->fi_BlockSize));
	de.de_Version = 0;
	de.de_Revision = 0;
	de.de_rfu = 0;
	de.de_Gap = -1;
	de.de_AvatarCount = 1;
	de.de_Type = lmfs->lmfs_ThisBlock.lmfe_Type;
	de.de_UniqueIdentifier = lmfs->lmfs_ThisBlock.lmfe_UniqueIdentifier;
	de.de_ByteCount = lmfs->lmfs_ThisBlock.lmfe_ByteCount;
	de.de_BlockCount = lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount -
	  lmfs->lmfs_ThisBlock.lmfe.lmb_HeaderBlockCount;
	de.de_Location = lmfs->lmfs_ThisBlockCursor;
	DBUG(("Total blocks %d, header blocks %d, content blocks %d\n",
	      lmfs->lmfs_ThisBlock.lmfe.lmb_BlockCount,
	      lmfs->lmfs_ThisBlock.lmfe.lmb_HeaderBlockCount,
	      de.de_BlockCount));
	de.de_Flags = 0;
	deSize = userReq->fio.io_Info.ioi_Recv.iob_Len;
	if (deSize > sizeof (DirectoryEntry)) {
	  deSize = sizeof (DirectoryEntry);
	}
	memcpy(userReq->fio.io_Info.ioi_Recv.iob_Buffer, &de, deSize);
	userReq->fio.io_Actual = deSize;
      } else {
	DBUG(("At index %d, want index %d, keep going\n", lmfs->lmfs_ThisBlockIndex,  userReq->fio.io_Info.ioi_Offset));
      }
      break;
    default:
      break;
    }
    if (returnVal != 0) {
      lmfs->lmfs_ThisBlockCursor = lmfs->lmfs_ThisBlock.lmfe.lmb_FlinkOffset;
      DBUG(("Advance cursor to %d\n", lmfs->lmfs_ThisBlockCursor));
      if (lmfs->lmfs_ThisBlockCursor == lmfs->lmfs_HaltCursor) {
	returnVal = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
	nextState = LMD_Done;
      } else {
	action = readThis;
	nextState = LMD_ExamineEntry;
      }
    }
    break;
  case LMD_ReadToSetEOF:
    theFile->fi_ByteCount = userReq->fio.io_Info.ioi_Offset;
    action = readThis;
    nextState = LMD_WriteWithNewEOF;
    break;
  case LMD_WriteWithNewEOF:
    lmfs->lmfs_ThisBlock.lmfe_ByteCount = theFile->fi_ByteCount;
    DBUG(("Setting EOF to %d\n", lmfs->lmfs_ThisBlock.lmfe_ByteCount));
    action = writeThis;
    nextState = LMD_Done;
    break;
  case LMD_ReadToSetType:
    action = readThis;
    nextState = LMD_WriteWithNewType;
    break;
  case LMD_WriteWithNewType:
    lmfs->lmfs_ThisBlock.lmfe_Type = userReq->fio.io_Info.ioi_Offset;
    action = writeThis;
    nextState = LMD_Done;
    break;
  case LMD_ExtendEOF:
    newEOF = userReq->fio.io_Info.ioi_Offset * theFile->fi_BlockSize +
      userReq->fio.io_Info.ioi_Send.iob_Len;
    if (newEOF > theFile->fi_ByteCount) {
      DBUG(("Need to extend EOF after write\n"));
      theFile->fi_ByteCount = newEOF;
      lmfs->lmfs_ThisBlockCursor = theFile->fi_AvatarList[0];
      action = readThis;
      nextState = LMD_WriteWithNewEOF;
    } else {
      nextState = LMD_Done;
    }
  }
  DBUG(("LMD FSM, action %d, nextstate %d, return 0x%x\n", action, nextState, returnVal));
  switch (action) {
  case noAction:
    break;
  case readThis:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = &lmfs->lmfs_ThisBlock;
    raw->io_Info.ioi_Recv.iob_Len = sizeof lmfs->lmfs_ThisBlock;
    raw->io_Info.ioi_Command = CMD_BLOCKREAD;
    raw->io_Info.ioi_Offset = lmfs->lmfs_ThisBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case readOther:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = &lmfs->lmfs_OtherBlock;
    raw->io_Info.ioi_Recv.iob_Len = sizeof lmfs->lmfs_OtherBlock;
    raw->io_Info.ioi_Command = CMD_BLOCKREAD;
    raw->io_Info.ioi_Offset = lmfs->lmfs_OtherBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeThis:
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Send.iob_Buffer = &lmfs->lmfs_ThisBlock;
    raw->io_Info.ioi_Send.iob_Len = sizeof lmfs->lmfs_ThisBlock;
    raw->io_Info.ioi_Command = CMD_BLOCKWRITE;
    raw->io_Info.ioi_Offset = lmfs->lmfs_ThisBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeOther:
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Send.iob_Buffer = &lmfs->lmfs_OtherBlock;
    raw->io_Info.ioi_Send.iob_Len = sizeof lmfs->lmfs_OtherBlock;
    raw->io_Info.ioi_Command = CMD_BLOCKWRITE;
    raw->io_Info.ioi_Offset = lmfs->lmfs_OtherBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case readData:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = lmfs->lmfs_CopyBuffer;
    raw->io_Info.ioi_Recv.iob_Len = lmfs->lmfs_BlocksToRead *
      lmfs->lmfs_BlockSize;
    raw->io_Info.ioi_Command = CMD_BLOCKREAD;
    raw->io_Info.ioi_Offset = lmfs->lmfs_ThisBlockCursor + lmfs->lmfs_ContentOffset;
;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeData:
    raw->io_Info.ioi_Send.iob_Buffer = lmfs->lmfs_CopyBuffer;
    raw->io_Info.ioi_Send.iob_Len = lmfs->lmfs_BlocksToRead *
      lmfs->lmfs_BlockSize;
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Command = CMD_BLOCKWRITE;
    raw->io_Info.ioi_Offset = lmfs->lmfs_OtherBlockCursor + lmfs->lmfs_ContentOffset;
;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  }
  lmfs->lmfs_FSM = nextState;
  return returnVal;
}


/*
 *	provide filesystem statistics and info
 */
static void
LinkedMemDoStat(LinkedMemFileEntry *fep, FileSystemStat *fsp, uint32 curblk)
{
	uint32 realsz, fhdr;

	TOUCH(curblk); /* hmmm... dcp */

	switch(fep->lmfe.lmb_Fingerprint) {
	case FINGERPRINT_FILEBLOCK:
	case FINGERPRINT_ANCHORBLOCK:
		fsp->fst_Used += fep->lmfe.lmb_BlockCount;
		break;

	case FINGERPRINT_FREEBLOCK:
		fhdr = HOWMANY(sizeof(LinkedMemFileEntry), fsp->fst_BlockSize);
		realsz = (fep->lmfe.lmb_BlockCount <= fhdr)? 0:
			  fep->lmfe.lmb_BlockCount - fhdr;
		fsp->fst_Free += realsz;
		if (realsz > fsp->fst_MaxFileSize)
			fsp->fst_MaxFileSize = realsz;
		break;
	default:	/* FS corruption, we should panic */
		break;
	}
}
