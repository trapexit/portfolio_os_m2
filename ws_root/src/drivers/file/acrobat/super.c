/*
 * @(#) super.c 96/09/24 1.2
 * 
 * Routines to deal with the superblock.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Write the superblock.
*/
Err
WriteSuperBlock(AcroFS *fs)
{
	Buffer *buf;
	BlockNum superBlockNum;
	bool changedSuperBlock = FALSE;
	Err err;

	superBlockNum = fs->fs_SuperBuf->buf_BlockNum;
	for (;;)
	{
		/* The superblock mirror is always the block immediately
		 * after the superblock.  */
		err = WriteMirroredBlock(fs,
			superBlockNum, superBlockNum+1,
			BLK_PHYS, fs->fs_Super);
		if (err >= 0)
		{
			if (changedSuperBlock)
			{
				err = OverwriteBlock(fs, superBlockNum, &buf);
				if (err < 0)
					return err;
				UnlinkBuffer(buf);
				memcpy(buf->buf_Data, fs->fs_Super,
					fs->fs_fs.fs_VolumeBlockSize);
				FreeBuffer(fs->fs_SuperBuf);
				fs->fs_SuperBuf = buf;
				fs->fs_Super = (SuperBlock *) buf->buf_Data;
			}
			UndirtyBuffer(fs->fs_SuperBuf);
			return err;
		}
		if (!MEDIA_ERROR(err))
			return err;
		/* Media error: back up to the previous two blocks. */
		superBlockNum -= 2;
		if (superBlockNum < fs->fs_MinSuperBlock)
		{
			DBUG_DAMAGE(("DAMAGE: no more superblocks, err %x\n",
				err));
			return FILE_ERR_DAMAGED;
		}
		changedSuperBlock = TRUE;
	}
}

/******************************************************************************
  Mount-time initialization.
*/
Err
InitSuperBlock(AcroFS *fs)
{
	Buffer *buf;
	BlockNum block;
	Err err;

	/* Scan forward, looking for a valid superblock pair. */
	for (block = fs->fs_MinSuperBlock;
	     block <= fs->fs_MaxSuperBlock;
	     block += 2)
	{
		err = ReadMirroredBlock(fs, block, block+1, 
				META_SUPER_BLOCK, BLK_PHYS, &buf);
		if (err < 0)
			continue;
		if (err != MIRROR_ALL_VALID)
			fs->fs_Flags |= ACRO_REWRITE_SUPERBLOCKS;
		UnlinkBuffer(buf);
		fs->fs_SuperBuf = buf;
		fs->fs_Super = (SuperBlock *) buf->buf_Data;
		DBUG_MOUNT(("Superblock: flags %x, rootDir %x, "
				"freeList %x, map %x,%x, logInfo %x,%x\n",
			fs->fs_Super->sb_Flags,
			fs->fs_Super->sb_RootDirBlockNum,
			fs->fs_Super->sb_FreeListBlockNum,
			fs->fs_Super->sb_MapBlockNum,
			fs->fs_Super->sb_MapBlockMirror,
			fs->fs_Super->sb_LogInfoBlockNum,
			fs->fs_Super->sb_LogInfoBlockMirror));
		return 0;
	}
	DBUG_DAMAGE(("DAMAGE: InitSuperBlock: "
		"no superblock between %x and %x\n",
		fs->fs_MinSuperBlock,  fs->fs_MaxSuperBlock));
	return FILE_ERR_DAMAGED;
}
