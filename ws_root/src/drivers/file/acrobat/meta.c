/*
 * @(#) meta.c 96/09/24 1.3
 *
 * Routines to deal with generic metablocks.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Is a metablock valid?
*/
bool
ValidMetaBlock(Buffer *buf, MetaBlockType type)
{
	MetaHdr *hdr;
	uint32 checksum;
	uint32 storedChecksum;

	hdr = (MetaHdr *) buf->buf_Data;

	/* Check the header's magic number. */
	if (hdr->mh_Magic != MH_MAGIC)
	{
		DBUG_IO(("  ValidMeta(%x) blk %x, magic %x\n", 
			buf, buf->buf_BlockNum, hdr->mh_Magic));
		return FALSE;
	}

	/* Check the checksum. */
	storedChecksum = hdr->mh_Checksum;
	hdr->mh_Checksum = 0;
	checksum = acroChecksum(hdr, buf->buf_FS->fs_fs.fs_VolumeBlockSize);
	hdr->mh_Checksum = storedChecksum;
	if (storedChecksum != checksum)
	{
		DBUG_IO(("  ValidMeta(%x) blk %x, checksum %x != stored %x\n",
			buf, buf->buf_BlockNum, checksum, storedChecksum));
		return FALSE;
	}

	/* Check the metablock type. */
	if (type == META_FILE_OR_DIR_BLOCK &&
	    (hdr->mh_Type == META_FILE_BLOCK || hdr->mh_Type == META_DIR_BLOCK))
		type = hdr->mh_Type;
	if (hdr->mh_Type != type)
	{
		DBUG_IO(("  ValidMeta(%x) blk %x, block type %x != %x\n",
			buf, buf->buf_BlockNum, hdr->mh_Type, type));
		return FALSE;
	}

	return TRUE;
}

/******************************************************************************
  Read and check a metablock.
*/
Err 
ReadMetaBlock(AcroFS *fs, BlockNum block, MetaBlockType type, uint32 flags, Buffer **pBuf)
{
	Buffer *buf;
	Err err;

	err = GetBlock(fs, block, flags | BLK_READ | BLK_META, &buf);
	if (err < 0)
	{
		DBUG_ANY(("  ReadMetaBlock(%x,%c,%x) GetBlock ret %x\n", 
			block, type, flags, err));
		return err;
	}
	if (err > 0)
	{
		/* We have just read it from the device; check that it is valid.
		 * (We can't do this if we just found the buffer already
		 * in memory, because someone may have modified it without
		 * updating the checksum yet. */
		if (!ValidMetaBlock(buf, type))
		{
			DBUG_DAMAGE(("DAMAGE: ReadMetaBlock: "
				"block %x not ValidMetaBlock "
				"(type %x, flags %x)\n",
				block, type, flags));
			return FILE_ERR_DAMAGED;
		}
	}
	*pBuf = buf;
	return 0;
}

