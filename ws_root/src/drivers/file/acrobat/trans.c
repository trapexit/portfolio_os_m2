/*
 * @(#) trans.c 96/09/24 1.6
 *
 * Routines to handle Acrobat transactions.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Begin a transaction.
*/
void
BeginTransaction(AcroFS *fs)
{
	DBUG_LOG(("===== BeginTransaction(%x): %x free\n", 
		fs, fs->fs_NumFreeBlocks));
	assert(IsEmptyList(&fs->fs_FreedThisTransaction));
	assert(IsEmptyList(&fs->fs_PermBorrows));
	fs->fs_SaveNumFreeBlocks = fs->fs_NumFreeBlocks;
	CheckFreeBlockCount(fs, "BeginTransaction");
}

/******************************************************************************
  Abort a failed transaction.
*/
void
AbortTransaction(AcroFS *fs)
{
	DBUG_LOG(("===== AbortTransaction(%x) start\n", fs));
	DeletePermBorrows(fs);
	(void) DeleteBuffers(fs,1); /* FIXME: what if fails? */
	ClearFreedThisTransaction(fs);
	DBUG_LOG(("  Restoring NumFreeBlocks from %x to %x\n",
		fs->fs_NumFreeBlocks, fs->fs_SaveNumFreeBlocks));
	fs->fs_NumFreeBlocks = fs->fs_SaveNumFreeBlocks;
	CheckFreeBlockCount(fs, "AbortTransaction");
	DBUG_LOG(("===== AbortTransaction(%x) done\n\n", fs));
}

/******************************************************************************
*/
#if defined(BUILD_STRINGS) && defined(DEBUG)
DumpDirtyBuffers(AcroFS *fs, char *msg)
{
	Buffer *buf;

	TOUCH(msg);
	DBUG_LOG(("%s: dirty blocks: ", msg));
	ScanList(&fs->fs_Buffers, buf, Buffer)
	{
		if (!(buf->buf_Flags & BUF_DIRTY))
			continue;
		DBUG_LOG(("%x ", buf->buf_BlockNum));
	}
	if (fs->fs_SuperBuf->buf_Flags & BUF_DIRTY)
		DBUG_LOG(("%x(super) ", fs->fs_SuperBuf->buf_BlockNum));
	DBUG_LOG(("\n"));
}
#else
#define	DumpDirtyBuffers(fs,msg)
#endif

/******************************************************************************
  Write dirty buffers to the device.
*/
static Err
WriteDirtyBuffers(AcroFS *fs)
{
	Buffer *buf;
	Err err;

	DumpDirtyBuffers(fs, "WriteDirtyBuffers");

	ScanList(&fs->fs_Buffers, buf, Buffer)
	{
		if (buf->buf_Flags & BUF_DIRTY)
		{
			err = WriteBlock(buf, BLK_REMAP | BLK_META);
			if (err < 0)
			{
				DBUG_DAMAGE(("DAMAGE: WriteDirtyBuffers: "
					"write block %x, err %x\n",
					buf->buf_BlockNum, err));
				return FILE_ERR_DAMAGED;
			}
		}
	}
	if (fs->fs_SuperBuf->buf_Flags & BUF_DIRTY)
	{
		err = WriteSuperBlock(fs);
		if (err < 0)
		{
			DBUG_DAMAGE(("DAMAGE: WriteDirtyBuffers: "
					"write superblock, err %x\n", err));
			return FILE_ERR_DAMAGED;
		}
	}
	return 0;
}

/******************************************************************************
  End (commit) a transaction.
*/
Err
EndTransaction(AcroFS *fs)
{
	Err err;

	DBUG_LOG(("===== EndTransaction(%x) start\n", fs));

	BeginBorrowing(fs);

	err = CreateLog(fs);
	if (err < 0)
		goto Exit;
	if (err == 0)
		/* Log is empty; nothing to do. */
		goto Exit;

	err = WriteDirtyBuffers(fs);
	if (err < 0)
		goto Exit;

	err = CommitPermBorrows(fs);
	if (err < 0)
		goto Exit;
	
	err = ClearLog(fs);
	if (err < 0)
		goto Exit;

	while (NumDirtyBuffers(fs) > 0)
	{
		err = WriteDirtyBuffers(fs);
		if (err < 0)
			goto Exit;

		err = CommitPermBorrows(fs);
		if (err < 0)
			goto Exit;
	}

	err = 0;
Exit:
	EndBorrowing(fs);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	DeletePermBorrows(fs);
	(void) DeleteBuffers(fs, err<0); /* FIXME: what if fails? */
	ClearFreedThisTransaction(fs);
	DBUG_LOG(("===== EndTransaction(%x) done, err %x\n\n", fs, err));
	CheckFreeBlockCount(fs, "AbortTransaction");
	return err;
}

/******************************************************************************
  Read the log at mount time, and do the operations it requests.
*/
Err
ProcessLog(AcroFS *fs)
{
	Err err;

	DBUG_MOUNT(("===== ProcessLog(%x) start\n", fs));

	err = ReadLog(fs);
	if (err < 0)
		goto Exit;
	if (err == 0)
		/* Log is empty; nothing to do. */
		goto Exit;

	DumpDirtyBuffers(fs, "ProcessLog");

	err = RebuildFreeList(fs);
	if (err < 0)
		goto Exit;

	BeginBorrowing(fs);

	err = WriteDirtyBuffers(fs);
	if (err < 0)
		goto Exit;

	err = CommitPermBorrows(fs);
	if (err < 0)
		goto Exit;
	
	err = ClearLog(fs);
	if (err < 0)
		goto Exit;

	while (NumDirtyBuffers(fs) > 0)
	{
		err = WriteDirtyBuffers(fs);
		if (err < 0)
			goto Exit;

		err = CommitPermBorrows(fs);
		if (err < 0)
			goto Exit;
	}
	err = 0;
Exit:
	EndBorrowing(fs);
	DeletePermBorrows(fs);
	(void) DeleteBuffers(fs, err<0); /* FIXME: what if fails? */
	ClearFreedThisTransaction(fs); /* Should not need this */
	DBUG_MOUNT(("===== ProcessLog(%x) done, err %x\n\n", fs, err));
	return err;
}

