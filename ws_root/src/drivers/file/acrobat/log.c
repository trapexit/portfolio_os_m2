/*
 * @(#) log.c 96/09/25 1.3
 *
 * Routines to manipulate the Log.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"


/******************************************************************************
*/
static uint32
LogInfoArrayEnd(AcroFS *fs, LogInfoBlock *logInfo)
{
	uint32 indx;

	for (indx = 0;  indx < MAX_LOGINFOBLOCK_ENTRIES(fs); indx++)
		if (logInfo->li_Log[indx].le_DataBlock == NULL_BLOCKNUM)
			break;
	return indx;
}

/******************************************************************************
 Make an entry in the Log, which says to write the specified buffer.
*/
static Err
CreateLogEntry(LogInfoBlock *logInfo, Buffer *buf)
{
	Buffer *logBuf;
	uint32 indx;
	MetaHdr *hdr;
	AcroFS *fs = buf->buf_FS;
	Err err;

	DBUG_LOG(("CreateLogEntry(%x) block %x\n", logInfo, buf->buf_BlockNum));
	hdr = (MetaHdr *) buf->buf_Data;
	if (hdr->mh_Type == META_FREELIST_BLOCK)
	{
		/* Don't need to log FreeListBlocks, since the free list
		 * gets rebuilt in ProcessLog. */
		return 0;
	}

	do {
		err = BorrowAcroBlock(fs, 0, NULL, &logBuf);
		if (err < 0)
			return err;
		memcpy(logBuf->buf_Data, buf->buf_Data, fs->fs_fs.fs_VolumeBlockSize);
		err = WriteBlock(logBuf, BLK_META);
		if (err < 0 && !MEDIA_ERROR(err))
			return err;
	} while (err < 0);

	indx = LogInfoArrayEnd(fs, logInfo);
	if (indx < MAX_LOGINFOBLOCK_ENTRIES(fs))
	{
		/* There is room to add it to this logInfo block. */
		logInfo->li_Log[indx].le_DataBlock = logBuf->buf_BlockNum;
		logInfo->li_Log[indx].le_TargetBlock = buf->buf_BlockNum;
		if (indx < MAX_LOGINFOBLOCK_ENTRIES(fs)-1)
			logInfo->li_Log[indx+1].le_DataBlock = NULL_BLOCKNUM;
		DeleteBuffer(logBuf);
		return 0;
	}
	/* FIXME: add a new LogInfoBlock to the chain */
	return -1;
}

/******************************************************************************
  Write the LogInfo block to the device.
*/
static Err
WriteLogInfoBlock(AcroFS *fs, LogInfoBlock *logInfo)
{
	Buffer *buf1;
	Buffer *buf2;
	Err err;

	/* Write out the LogInfo block */
	for (;;)
	{
		err = WriteMirroredBlock(fs, 
			fs->fs_Super->sb_LogInfoBlockNum,
			fs->fs_Super->sb_LogInfoBlockMirror,
			0, logInfo);
		DBUG_LOG(("  WriteLogInfo(%x,%x) err %x\n", 
			fs->fs_Super->sb_LogInfoBlockNum,
			fs->fs_Super->sb_LogInfoBlockMirror, err));
		if (err >= 0)
			break;
		if (!MEDIA_ERROR(err))
			break;
		err = AllocAcroBlock(fs, ALLOC_CRITICAL, &buf1);
		if (err < 0)
			return err;
		err = AllocAcroBlock(fs, ALLOC_CRITICAL, &buf2);
		if (err < 0)
		{
			(void) FreeAcroBlock(fs, buf1->buf_BlockNum);
			return err;
		}
		DBUG_LOG(("   Alloc new logInfo: %x,%x\n",
			buf1->buf_BlockNum, buf2->buf_BlockNum));
		memcpy(buf1->buf_Data, logInfo, fs->fs_fs.fs_VolumeBlockSize);
		memcpy(buf2->buf_Data, logInfo, fs->fs_fs.fs_VolumeBlockSize);
		fs->fs_Super->sb_LogInfoBlockNum = buf1->buf_BlockNum;
		fs->fs_Super->sb_LogInfoBlockMirror = buf2->buf_BlockNum;
		DirtyBuffer(fs->fs_SuperBuf);
	}
	return err;
}

/******************************************************************************
  Create the Log entries for the current transaction.
*/
Err
CreateLog(AcroFS *fs)
{
	Buffer *buf;
	Buffer *logInfoBuf;
	LogInfoBlock *logInfo;
	uint32 indx;
	Err err;

	/* Prepare the buffer for the LogInfo block. */
	err = OverwriteBlock(fs, fs->fs_Super->sb_LogInfoBlockNum, &logInfoBuf);
	if (err < 0)
		return err;
	logInfo = (LogInfoBlock *) logInfoBuf->buf_Data;
	memset(logInfo, 0, fs->fs_fs.fs_VolumeBlockSize);
	InitMetaHdr(&logInfo->li_Hdr, META_LOGINFO_BLOCK);
	logInfo->li_NextLogInfoBlock = NULL_BLOCKNUM;
	for (indx = 0;  indx < MAX_LOGINFOBLOCK_ENTRIES(fs);  indx++)
		logInfo->li_Log[indx].le_DataBlock = NULL_BLOCKNUM;

	/* Allocate & write the Log data blocks. */
	ScanList(&fs->fs_Buffers, buf, Buffer)
	{
		assert(!(buf->buf_Flags & BUF_FREED));
		if (!(buf->buf_Flags & BUF_DIRTY))
			continue;
		/* assert(buf->buf_Flags & BUF_META); */
		err = CreateLogEntry(logInfo, buf);
		if (err < 0)
			return err;
	}
	if (fs->fs_SuperBuf->buf_Flags & BUF_DIRTY)
	{
		err = CreateLogEntry(logInfo, fs->fs_SuperBuf);
		if (err < 0)
			return err;
	}

	/* If log is empty, no need to write the LogInfo block. */
	if (logInfo->li_Log[0].le_DataBlock == NULL_BLOCKNUM)
		return 0;

	logInfo->li_Hdr.mh_Flags |= LI_VALID;
	err = WriteLogInfoBlock(fs, logInfo);
	if (err < 0)
		return err;

	return 1; /* Tell caller there's a non-empty log. */
}

/******************************************************************************
*/
Err
ClearLog(AcroFS *fs)
{
	Buffer *logInfoBuf;
	LogInfoBlock *logInfo;
	Err err;

	err = OverwriteBlock(fs, fs->fs_Super->sb_LogInfoBlockNum, &logInfoBuf);
	if (err < 0)
		return err;
	logInfo = (LogInfoBlock *) logInfoBuf->buf_Data;
	logInfo->li_Hdr.mh_Flags &= ~LI_VALID;
	err = WriteLogInfoBlock(fs, logInfo);
	if (err < 0)
		return err;
	/* FIXME: free non-head LogInfo blocks */
	return 0;
}

/******************************************************************************
*/
Err
ReadLog(AcroFS *fs)
{
	Buffer *buf;
	Buffer *logInfoBuf;
	LogInfoBlock *logInfo;
	LogEntry *le;
	uint32 indx;
	Err err;

	err = ReadMirroredBlock(fs, 
		fs->fs_Super->sb_LogInfoBlockNum,
		fs->fs_Super->sb_LogInfoBlockMirror,
		META_LOGINFO_BLOCK, 0, &logInfoBuf);
	if (err < 0)
	{
		DBUG_DAMAGE(("DAMAGE: read logInfo block %x,%x, err %x\n",
			fs->fs_Super->sb_LogInfoBlockNum,
			fs->fs_Super->sb_LogInfoBlockMirror, err));
		return FILE_ERR_DAMAGED;
	}

	logInfo = (LogInfoBlock *) logInfoBuf->buf_Data;
	if (!(logInfo->li_Hdr.mh_Flags & LI_VALID))
		/* Log is empty (not valid). */
		return 0;

	for (;;)
	{
		logInfo = (LogInfoBlock *) logInfoBuf->buf_Data;
		for (indx = 0;  indx < MAX_LOGINFOBLOCK_ENTRIES(fs);  indx++)
		{
			le = &logInfo->li_Log[indx];
			if (le->le_DataBlock == NULL_BLOCKNUM)
				break;
			err = GetBlock(fs, le->le_DataBlock, 
					BLK_READ, &buf);
			DBUG_MOUNT(("ReadLog: read blk %x, target %x, err %x\n",
				le->le_DataBlock, le->le_TargetBlock, err));
			if (err < 0)
			{
				DBUG_DAMAGE(("DAMAGE: ReadLog: "
					"block %x, err %x\n", 
					le->le_DataBlock, err));
				return FILE_ERR_DAMAGED;
			}
			if (le->le_TargetBlock >= fs->fs_MinSuperBlock &&
			    le->le_TargetBlock <= fs->fs_MaxSuperBlock)
			{
				/* It's the superblock. */
				memcpy(fs->fs_Super, buf->buf_Data,
					fs->fs_fs.fs_VolumeBlockSize);
				DeleteBuffer(buf);
				DirtyBuffer(fs->fs_SuperBuf);
			} else
			{
				RetargetBuffer(buf, le->le_TargetBlock);
				DirtyBuffer(buf);
			}
		}

		if (logInfo->li_NextLogInfoBlock == NULL_BLOCKNUM)
			break;
		err = ReadMetaBlock(fs, logInfo->li_NextLogInfoBlock,
			META_LOGINFO_BLOCK, 0, &logInfoBuf);
		if (err < 0)
		{
			DBUG_DAMAGE(("DAMAGE: ReadLog: "
				"read next logInfo block %x, err %x\n",
				logInfo->li_NextLogInfoBlock, err));
			return FILE_ERR_DAMAGED;
		}
	}
	return 1; /* Tell caller the log was not empty. */
}
