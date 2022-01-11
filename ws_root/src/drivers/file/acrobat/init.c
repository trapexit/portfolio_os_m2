/*
 * @(#) init.c 96/09/24 1.21
 *
 * Mount-time initialization code.
 */

#include <file/acromedia.h>
#include <kernel/super.h>
#include "acrofuncs.h"

#define ACRO_THREAD_STACK_SIZE	2048
#define ACRO_THREAD_PRI		210

#ifdef DEBUG
uint32 acroDebug = DB_DAMAGE;
#endif

/******************************************************************************
  Perform the requests in the filesystem queue.
*/
static void
DoRequests(AcroFS *fs)
{
	FileIOReq *ioreq;
	uint32 intr;
	Err err;

	if (fs->fs_fs.fs_DeviceBusy)
		return;

	intr = Disable();
	while (!ISEMPTYLIST(&fs->fs_fs.fs_RequestsToDo))
	{
		ioreq = (FileIOReq *) RemHead(&fs->fs_fs.fs_RequestsToDo);
		AddTail(&fs->fs_fs.fs_RequestsRunning, (Node *) ioreq);
	}
	Enable(intr);

	/* Service all requests. */
	for (;;)
	{
		intr = Disable();
		if (fs->fs_fs.fs_DeviceBusy ||
		    ISEMPTYLIST(&fs->fs_fs.fs_RequestsRunning))
		{
			Enable(intr);
			return;
		}
		fs->fs_fs.fs_DeviceBusy = TRUE;
		ioreq = (FileIOReq *) FirstNode(&fs->fs_fs.fs_RequestsRunning);
		Enable(intr);
		err = DoAcroRequest(fs, ioreq);
		intr = Disable();
		RemNode((Node *) ioreq);
		fs->fs_fs.fs_DeviceBusy = FALSE;
		Enable(intr);
		ioreq->fio.io_Error = err;
		SuperCompleteIO((IOReq *) ioreq);
	}
}

/******************************************************************************
  If problems are found during mount, flags may be set requesting us
  to clean up the problems.  Do that clean-up now.
*/
static 
DoRewrites(AcroFS *fs)
{
	Buffer *buf;
	Err err;

	if (fs->fs_Flags & ACRO_REWRITE_MAPBLOCKS)
	{
		assert(!IsEmptyList(&fs->fs_MapBlockList));
		buf = (Buffer *) FirstNode(&fs->fs_MapBlockList);
		err = WriteMapBlock(fs, buf);
		if (err < 0)
			return err;
	}

	if ((fs->fs_Flags & ACRO_REWRITE_SUPERBLOCKS) ||
	    (fs->fs_SuperBuf->buf_Flags & BUF_DIRTY))
	{
		err = WriteSuperBlock(fs);
		if (err < 0)
			return err;
	}

	return 0;
}

/******************************************************************************
  Finish mounting the device.
*/
static Err
CompleteMount(AcroFS *fs)
{
	Err err;

	DBUG_MOUNT(("CompleteMount(%x)\n", fs));
	PrepList(&fs->fs_MapBlockList);
	PrepList(&fs->fs_Buffers);
	PrepList(&fs->fs_PermBorrows);
	PrepList(&fs->fs_FreedThisTransaction);
	fs->fs_FreeListBuf = NULL;
	fs->fs_SuperBuf = NULL;
	fs->fs_Super = NULL;

	err = InitBuffers(fs);
	DBUG_MOUNT(("CompleteMount: InitBuffers err %x\n", err));
	if (err < 0)
		return err;
	err = InitSuperBlock(fs);
	DBUG_MOUNT(("CompleteMount: InitSuperBlock err %x\n", err));
	if (err < 0)
		return err;
	err = InitMapper(fs);
	DBUG_MOUNT(("CompleteMount: InitMapper err %x\n", err));
	if (err < 0)
		return err;
	err = ProcessLog(fs);
	DBUG_MOUNT(("CompleteMount: ProcessLog err %x\n", err));
	if (err < 0)
		return err;
	err = InitFreeList(fs);
	DBUG_MOUNT(("CompleteMount: InitFreeList err %x\n", err));
	if (err < 0)
		return err;
	err = InitRootDir(fs);
	DBUG_MOUNT(("CompleteMount: InitRootDir err %x\n", err));
	if (err < 0)
		return err;
	err = DoRewrites(fs);
	DBUG_MOUNT(("CompleteMount: DoRewrites err %x\n", err));
	if (err < 0)
		return err;

	return 0;
}

/******************************************************************************
  Code for the Acrobat thread.
*/
static Err
acroThread(void)
{
	uint32 rcvsig;
	FileSystem *fsp;
	AcroFS *fs;
	Err err;

	DBUG_MOUNT(("acroThread started\n"));
	if (!(CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED))
    		return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);

	fs = NULL;
	for (fsp = (FileSystem *) FIRSTNODE(&fileFolio.ff_Filesystems);
	     ISNODE(&fileFolio.ff_Filesystems, fsp);
	     fsp = (FileSystem *) NEXTNODE(fsp))
	{
		if (fsp->fs_Thread == CURRENTTASK)
		{
			fs = (AcroFS *) fsp;
			break;
		}
	}
	if (fs == NULL)
		return FILE_ERR_NOFILESYSTEM;

	err = CompleteMount(fs);
	if (err < 0)
	{
		fs->fs_MountState = MS_FAILED;
		fs->fs_fs.fs_MountError = err;
		return err;
	}
	fs->fs_StartSig = SuperAllocSignal(0L);
	fs->fs_MountState = MS_ACTIVE;

	/* Request processing loop. */
	for (;;)
	{
		rcvsig = SuperWaitSignal(fs->fs_StartSig | SIGF_IODONE);
		if (rcvsig & SIGF_ABORT)
			break;
		DoRequests(fs);
	}
	DBUG_MOUNT(("acroDaemon killed\n"));
	fs->fs_MountState = MS_INACTIVE;
	return 0;
}

/******************************************************************************
  Start up the Acrobat thread.
*/
static Err
acroInitThread(AcroFS *fs)
{
	Item ti;
	Err err;
	Task *t;

	ti = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
		  TAG_ITEM_NAME,                  fs->fs_fs.fs_FileSystemName,
		  TAG_ITEM_PRI,                   ACRO_THREAD_PRI,
		  CREATETASK_TAG_PC,              acroThread,
		  CREATETASK_TAG_STACKSIZE,       ACRO_THREAD_STACK_SIZE,
		  CREATETASK_TAG_SUPERVISOR_MODE, TRUE,
		  CREATETASK_TAG_THREAD,          TRUE,
		  TAG_END);
	DBUG_MOUNT(("acroInitThread: thread %x\n", ti));
	if (ti < 0)
	{
		PrintfSysErr(ti);
		return ti;
	}
	t = (Task *) LookupItem(ti);

	if ((err = SuperInternalOpenItem(fs->fs_DeviceIOReq->io_Dev->dev.n_Item,
		 NULL, t)) < 0)
	{
		PrintfSysErr(err);
		return err;
	}

	if ((err = SuperSetItemOwner(((ItemNode *)fs->fs_DeviceIOReq)->n_Item,
		 ti)) < 0)
	{
		PrintfSysErr(err);
		return err;
	}
	fs->fs_fs.fs_Thread = t;
	return 0;
}

/******************************************************************************
  Activate/quiesce filesystem.
*/
static Err
acroActQue(FileSystem *fsp, enum FSActQue aq)
{
	AcroFS *fs = (AcroFS *) fsp;

	DBUG_MOUNT(("acroActQue(%x), mountstate %x, flags %x\n", 
		aq, fs->fs_MountState, fsp->fs_Flags));
	switch (aq)
	{
	case ActivateFS:
		switch (fs->fs_MountState)
		{
		case MS_INACTIVE:
			fs->fs_MountState = MS_INTRANS;
			fs->fs_fs.fs_MountError = 0;
			if ((fs->fs_fs.fs_MountError = acroInitThread(fs)) < 0)
				fs->fs_MountState = MS_FAILED;
			return 1;
		case MS_INTRANS:
			return 1;
		case MS_ACTIVE:
    			fsp->fs_Flags &= ~FILESYSTEM_IS_QUIESCENT;
			return 0;
		case MS_FAILED:
    			fsp->fs_Flags &= ~FILESYSTEM_IS_QUIESCENT;
			fs->fs_MountState = MS_INACTIVE;
			return fs->fs_fs.fs_MountError;
		default:
    			fsp->fs_Flags &= ~FILESYSTEM_IS_QUIESCENT;
			fs->fs_MountState = MS_INACTIVE;
			return -1;
		}
		break;
	case QuiesceFS:
		switch (fs->fs_MountState) 
		{
		case MS_INACTIVE:
			fsp->fs_Flags &= ~FILESYSTEM_WANTS_QUIESCENT;
			fsp->fs_Flags |= FILESYSTEM_IS_QUIESCENT;
			return 0;
		case MS_INTRANS:
			return 1;
		case MS_ACTIVE:
			if (fsp->fs_Flags & FILESYSTEM_IS_OFFLINE)
				return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
			fsp->fs_Flags |= FILESYSTEM_IS_OFFLINE;
			SuperinternalSignal(fsp->fs_Thread, SIGF_ABORT);
			fs->fs_MountState = MS_INTRANS;
			fs->fs_fs.fs_MountError = 0;
			return 1;
		default:
			fs->fs_MountState = MS_INACTIVE;
			fs->fs_fs.fs_MountError = FILE_ERR_SOFTERR;
			/* FALLTHRU */
		case MS_FAILED:
			fsp->fs_Flags &= ~FILESYSTEM_WANTS_QUIESCENT;
			fsp->fs_Flags |= FILESYSTEM_IS_QUIESCENT;
			return fs->fs_fs.fs_MountError;
		}
		break;
	}
	return FILE_ERR_NOSUPPORT;
}

/******************************************************************************
  Dispatch an I/O request.
*/
static Err
acroDispatch(FileSystem *fsp, IOReq *ioreq)
{
	uint32 intr;
	AcroFS *fs = (AcroFS *) fsp;

	switch (ioreq->io_Info.ioi_Command)
	{
	case CMD_BLOCKREAD:
	case CMD_BLOCKWRITE:
	case FILECMD_ADDENTRY:
	case FILECMD_DELETEENTRY:
	case FILECMD_ADDDIR:
	case FILECMD_DELETEDIR:
	case FILECMD_ALLOCBLOCKS:
	case FILECMD_SETEOF:
	case FILECMD_SETTYPE:
	case FILECMD_SETVERSION:
	case FILECMD_SETBLOCKSIZE:
	case FILECMD_RENAME:
	case FILECMD_READDIR:
	case FILECMD_READENTRY:
	case FILECMD_FSSTAT:
	case FILECMD_SETDATE:
		break;
	case FILECMD_OPENENTRY:
		SuperCompleteIO(ioreq);
		return 0;
	default:
		return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
	}

	intr = Disable();
	InsertNodeFromTail(&fs->fs_fs.fs_RequestsToDo, (Node *) ioreq);
	ioreq->io_Flags &= ~IO_QUICK;
	Enable(intr);

	if (fsp->fs_Thread == CURRENTTASK)
		DoRequests(fs);
	else
		SuperinternalSignal(fsp->fs_Thread, fs->fs_StartSig);
	return 0;
}

/******************************************************************************
  Abort an I/O request.
*/
static void
acroAbortio(IOReq *ioreq)
{
	uint32 intr;
	File *fp = ((OFile *) ioreq->io_Dev)->ofi_File;
	AcroFS *fs = (AcroFS *) fp->fi_FileSystem;

	intr = Disable();
	/* is it me? */
	if (fs->fs_fs.fs_DeviceBusy &&
	    fs->fs_DeviceIOReq->io_Info.ioi_UserData == (void *) ioreq)
	{
		SuperinternalAbortIO(fs->fs_DeviceIOReq);
	} else
	{
		ioreq->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
		RemNode((Node *) ioreq);
		SuperCompleteIO(ioreq);
	}
	Enable(intr);
}

/******************************************************************************
*/
static Err 
acroUnload(FileSystemType *fst)
{
	fst->fst_ActQue = NULL;
	fst->fst_QueueRequest = NULL;
	fst->fst_AbortIO = NULL;
	fst->fst_Unloader = NULL;
	return 0;
}

/******************************************************************************
*/
#if defined(BUILD_PARANOIA) && !defined(NDEBUG)
void
__assert(const char *expr, const char *file, int line)
{
	printf("***** Acrobat assertion failed: %s in %s line %d\n",
		expr, file, line);
	for (;;) ;
}
#endif

/******************************************************************************
*/
int32 
main(int argc, char **argv)
{
	FileSystemType	*fst = (FileSystemType *) argv;

	if (argc < 0)
		return 0;
#ifdef DEBUG
	printf("acroDebug at %x\n", &acroDebug);
#endif
	fst->fst_ActQue = acroActQue;
	fst->fst_QueueRequest = acroDispatch;
	fst->fst_AbortIO = acroAbortio;
	fst->fst_Unloader = acroUnload;
	return 0;
}

