/* @(#) fs_acrobat.c 96/09/25 1.15 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
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
#include <file/acro.h>
#include <file/acromedia.h>
#include <string.h>
#include <ctype.h>

extern	FileSystem *CreateFileSystem(Device *theDevice,
				     uint32 blockOffset,
				     ExtVolumeLabel *discLabel,
				     uint32 highLevelDiskSize,
				     uint32 rootAvatars,
				     FileSystemType *fst);
extern	void	GiveDaemon(void *foo);
extern	Err	LoadFSDriver(FileSystemType *fst);
extern  void    LinkAndAnnounceFileSystem(FileSystem *fs);
extern  FileSystem *FindFileSystem(const uchar *name);
extern	int32	StartStatusRequest(FileSystem *fs, IOReq *reqp,
				   IOReq *(*callBack)(IOReq *));
extern	Err	acroFormat(Item ioreqItem, const DeviceStatus *stat, 
				const char *name, const TagArg *tags);

static FileSystemType acro_type;

Err
InitAcrobat(List *fstype)
{
	AddTail(fstype, (Node *) &acro_type);
	return 0;
}

static void
acroFakeRoot(FileSystem *fsp)
{
	File *root = fsp->fs_RootDirectory;

	root->fi_LastAvatarIndex = 0;
	root->fi_UniqueIdentifier = 0;
	root->fi_BlockSize = 0;
	root->fi_ByteCount = 0;
	root->fi_BlockCount = 0;
	root->fi_Type = FILE_TYPE_DIRECTORY;
	root->fi_Flags |= FILE_SUPPORTS_DIRSCAN | FILE_SUPPORTS_ENTRY |
			   FILE_USER_STORAGE_PLACE;
	if (fsp->fs_Flags & FILESYSTEM_IS_READONLY)
		root->fi_Flags |= FILE_IS_READONLY;
}

static Item
acroMount(Device *dev, uint32 boff, IOReq *ioreq,
	    ExtVolumeLabel *label, uint32 loff, DeviceStatus *devstat)
{
	FileSystem *fsp;
	AcroFS *fs;
	bool remount = FALSE;
	Err err;

	TOUCH(loff);
	if (dev == NULL || ioreq == NULL || label == NULL || devstat == NULL)
		return 0;

	/* FIXME - remove me after mount tags are implemented */

	fsp = (FileSystem *) FindFileSystem(label->dl_VolumeIdentifier);
	if (fsp != NULL)
	{
		if (fsp->fs_VolumeUniqueIdentifier ==
			label->dl_VolumeUniqueIdentifier)
		{
			if (fsp->fs_Flags & FILESYSTEM_IS_OFFLINE)
				remount = 1;
			else
			{
				label->dl_VolumeUniqueIdentifier =
					fileFolio.ff_NextUniqueID --;
			}
		}
	}

	if (!remount)
	{
		fsp = CreateFileSystem(dev, boff, label, sizeof(AcroFS),
				label->dl_RootDirectoryLastAvatarIndex + 1,
				&acro_type);
		if (fsp == NULL)
			return NOMEM;
		fsp->fs_DeviceBlocksPerFilesystemBlock = 
			fsp->fs_VolumeBlockSize / devstat->ds_DeviceBlockSize;
		if (devstat->ds_DeviceUsageFlags & DS_USAGE_READONLY)
			fsp->fs_Flags |= FILESYSTEM_IS_READONLY;
	}

	fs = (AcroFS *) fsp;
	fs->fs_MountState = MS_INACTIVE;
	fs->fs_MinSuperBlock = label->dl_MinSuperBlock;
	fs->fs_MaxSuperBlock = label->dl_MaxSuperBlock;
#ifdef BUILD_STRINGS
	/* This is just to make it easier to find the AcroFS in a dump. */
	fs->fs_Marker = 0xabacadab;
#endif
	/* XXX - for remount need to reinitialize all changed fields of fs */
	if (remount)
	{
		if (fs->fs_DeviceIOReq != ioreq) 
		{
			(void) SuperInternalDeleteItem(fs->fs_DeviceIOReq->io.n_Item);
		}
	}
	fs->fs_DeviceIOReq = ioreq;
	ioreq->io_Info.ioi_Flags = 0;
	ioreq->io_Info.ioi_CmdOptions = 0;
	err = SuperInternalOpenItem(dev->dev.n_Item, NULL,
				      fileFolio.ff_Daemon.ffd_Task);
	if (err < 0)
		return err;

	acroFakeRoot(fsp);
	GiveDaemon(ioreq);
	if (!remount)
	{
		GiveDaemon(fsp->fs_RootDirectory);
		GiveDaemon(fsp);
	}

	/*
	 *	now lets set up thread stuff, so that in daemon context
	 *	we create acrobat thread for this filesystem.
	 */
	fsp->fs_Flags |= FILESYSTEM_IS_QUIESCENT;
	if (remount)
		fsp->fs_Flags &= ~FILESYSTEM_IS_OFFLINE;
	else
	        LinkAndAnnounceFileSystem(fsp);
	return fsp->fs.n_Item;
}


static Err
acroDismount(FileSystem *fsp)
{
	AcroFS *fs = (AcroFS *) fsp;
	Err err;

	if (fsp->fs_Flags & FILESYSTEM_WANTS_QUIESCENT)
	{
		fsp->fs_Flags |= FILESYSTEM_WANTS_DISMOUNT;
		return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
	}
	if (fs->fs_DeviceIOReq != NULL)
	{
		/* FIXME: why does this fail sometimes? */
		err = SuperInternalDeleteItem(fs->fs_DeviceIOReq->io.n_Item);
#ifdef BUILD_STRINGS
		if (err < 0)
			printf("acroDismount: delete IOReq %x returns %x\n", 
				fs->fs_DeviceIOReq->io.n_Item, err);
#else
		TOUCH(err);
#endif
	}
	return 0;
}

static IOReq *
acroStatDone(IOReq *ioreq)
{
	FileSystem *fsp = (FileSystem *) ioreq->io_Info.ioi_UserData;

	fsp->fs_DeviceBusy = FALSE;
	if (ioreq->io_Error < 0)
	{
		fsp->fs_Flags |= FILESYSTEM_WANTS_DISMOUNT;
		if (!(fsp->fs_Flags & FILESYSTEM_IS_QUIESCENT))
			fsp->fs_Flags |= FILESYSTEM_WANTS_QUIESCENT;
	}
	SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			    fileFolio.ff_Daemon.ffd_RescheduleSignal);
	return NULL;
}

static void
acroTimeslice(FileSystem *fsp)
{
	AcroFS *fs = (AcroFS *) fsp;

	if (fsp->fs_DeviceBusy)
		return;

	if ((fsp->fs_Flags & FILESYSTEM_WANTS_RECHECK) &&
	    fs->fs_DeviceIOReq != NULL)
	{
		fsp->fs_DeviceBusy = TRUE;
		(void) StartStatusRequest(fsp, fs->fs_DeviceIOReq, acroStatDone);
	} else if (fsp->fs_DeviceStatusTemp)
	{
		SuperFreeMem(fsp->fs_DeviceStatusTemp, sizeof(DeviceStatus));
		fsp->fs_DeviceStatusTemp = NULL;
	}
}

static FileSystemType acro_type =
{
	{NULL, NULL, 0, 0, 0,
	 FSTYPE_NEEDS_LOAD + FSTYPE_UNLOADABLE + FSTYPE_AUTO_QUIESCE,
	 sizeof(FileSystemType),
	 "acrobat.fs" },                /* initialize the node */
	VOLUME_STRUCTURE_ACROBAT,	/* fst_VolumeStructureVersion */
	0,				/* fst_DeviceFamilyCode */
	0,                              /* fst_LoadError */
	0,                              /* fst_ModuleItem */
	LoadFSDriver,			/* fst_Loader */
	NULL,                           /* fst_Unloader */
	acroMount,			/* fst_Mount */
        NULL,	                	/* fst_ActQue */
	acroDismount,			/* fst_Dismount */
	NULL,				/* fst_QueueRequest */
	NULL,				/* fst_FirstTimeInit */
	acroTimeslice,			/* fst_Timeslice */
	NULL,				/* fst_AbortIO */
	NULL,				/* fst_CreateEntry */
	NULL,				/* fst_DeleteEntry */
	NULL,				/* fst_AllocSpace */
	NULL,				/* fst_CloseFile */
	NULL,				/* fst_CreateIOReq */
	NULL,				/* fst_DeleteIOReq */
	acroFormat,			/* fst_Format */
};

