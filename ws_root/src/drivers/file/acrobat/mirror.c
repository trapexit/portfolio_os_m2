/*
 * @(#) mirror.c 96/09/25 1.4
 *
 * Routines to deal with "mirrored" blocks.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Write a mirrored block pair.
*/
Err
WriteMirroredBlock(AcroFS *fs, BlockNum block1, BlockNum block2,
	uint32 flags, void *data)
{
	Buffer *buf1;
	Buffer *buf2;
	Err err;

	err = OverwriteBlock(fs, block1, &buf1);
	if (err < 0)
		return err;
	err = OverwriteBlock(fs, block2, &buf2);
	if (err < 0)
		return err;
	memcpy(buf1->buf_Data, data, fs->fs_fs.fs_VolumeBlockSize);
	memcpy(buf2->buf_Data, data, fs->fs_fs.fs_VolumeBlockSize);
	err = WriteBlock(buf1, flags | BLK_META);
	if (err < 0)
		return err;
	err = WriteBlock(buf2, flags | BLK_META);
	if (err < 0)
		return err;
	return 0;
}

/******************************************************************************
  Read a mirrored block pair.
*/
Err
ReadMirroredBlock(AcroFS *fs, BlockNum block1, BlockNum block2,
	MetaBlockType type, uint32 flags, Buffer **pBuf)
{
	Buffer *buf1;
	Buffer *buf2;
	Err err1;
	Err err2;

	err1 = GetBlock(fs, block1, BLK_READ | flags, &buf1);
	if (err1 < 0)
	{
		DBUG_ANY(("ReadMirror(%x,%x,%x) ret1 %x\n",
			block1, block2, flags, err1));
		return err1;
	}
	err2 = GetBlock(fs, block2, BLK_READ | flags, &buf2);
	if (err2 < 0)
	{
		DBUG_ANY(("ReadMirror(%x,%x,%x) ret2 %x\n", 
			block1, block2, flags, err2));
		return err2;
	}

	if (err1 > 0 && ValidMetaBlock(buf1, type))
	{
		*pBuf = buf1;
		if (memcmp(buf1->buf_Data, buf2->buf_Data,
				fs->fs_fs.fs_VolumeBlockSize) == 0)
			return MIRROR_ALL_VALID;
		return MIRROR_1_VALID;
	} 
	if (err2 > 0 && ValidMetaBlock(buf2, type))
	{
		*pBuf = buf2;
		return MIRROR_2_VALID;
	}
	return FILE_ERR_DAMAGED;
}
