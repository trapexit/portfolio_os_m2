/*
 * @(#) mapper.c 96/09/25 1.3
 *
 * Routines dealing with the block remapper.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Find the end of a MapBlock array.
*/
static uint32
MapBlockArrayEnd(AcroFS *fs, MapBlock *mb)
{
	uint32 indx;

	for (indx = 0;  indx < MAX_MAPBLOCK_ENTRIES(fs);  indx++)
		if (mb->mb_Map[indx].me_VirtBlock == NULL_BLOCKNUM)
			break;
	return indx;
}

/******************************************************************************
  Get the physical block number corresponding to a virtual block number.
*/
PhysBlockNum
PhysicalBlock(AcroFS *fs, BlockNum block)
{
	Buffer *buf;
	MapBlock *mb;
	int32 indx;

	ScanList(&fs->fs_MapBlockList, buf, Buffer)
	{
		mb = (MapBlock *) buf->buf_Data;
		/* Must scan backwards to ensure that later entries
		 * override earlier ones. */
		for (indx = MapBlockArrayEnd(fs,mb)-1; indx >= 0; indx--)
		{
			if (mb->mb_Map[indx].me_VirtBlock == block)
			{
				/* DBUG_MAP(("PhysicalBlock(%x) = %x\n",
				    block, mb->mb_Map[indx].me_PhysBlock)); */
				return mb->mb_Map[indx].me_PhysBlock;
			}
		}
	}

	/* Block is not in a map table.
	 * The physical block is just the same as the virtual block. */
	return (PhysBlockNum) block;
}

/******************************************************************************
 Write out the map block head.
 If it fails, allocate new (mirrored) map blocks and retry.
*/
Err
WriteMapBlock(AcroFS *fs, Buffer *mapBuf)
{
	Buffer *buf1;
	Buffer *buf2;
	MapBlock *mb = (MapBlock *) mapBuf->buf_Data;
	Err err;

	buf1 = buf2 = NULL;
    	for (;;)
	{
		err = WriteMirroredBlock(fs, 
				fs->fs_Super->sb_MapBlockNum, 
				fs->fs_Super->sb_MapBlockMirror,
				BLK_PHYS, mb);
		DBUG_MAP(("   Write mapblock %x,%x, err %x\n", 
			fs->fs_Super->sb_MapBlockNum, 
			fs->fs_Super->sb_MapBlockMirror, err));
		if (err >= 0)
		{
			if (buf1 != NULL)
			{
				DBUG_MAP(("   Changing mapblock head "
					"from %x (blk %x) to %x (blk %x)\n",
					mapBuf, mapBuf->buf_BlockNum,
					buf1, buf1->buf_BlockNum));
				DeleteBuffer(mapBuf);
				AddHead(&fs->fs_MapBlockList, 
					(Node *) &buf1->buf_Node);
				FreeBuffer(buf2);
			}
			break;
		}
		if (!MEDIA_ERROR(err))
			return err;
		err = AllocAcroBlock(fs, ALLOC_CRITICAL, &buf1);
		if (err < 0)
			return err;
		err = AllocAcroBlock(fs, ALLOC_CRITICAL, &buf2);
		if (err < 0)
		{
			(void) FreeAcroBlock(fs, buf1->buf_BlockNum);
			return err;
		}
		DBUG_MAP(("   Alloc new mapblock head: %x,%x\n",
			buf1->buf_BlockNum, buf2->buf_BlockNum));
		memcpy(buf1->buf_Data, mb, fs->fs_fs.fs_VolumeBlockSize);
		memcpy(buf2->buf_Data, mb, fs->fs_fs.fs_VolumeBlockSize);
		mb = (MapBlock *) buf1->buf_Data;
		UnlinkBuffer(buf1);
		UnlinkBuffer(buf2);
		fs->fs_Super->sb_MapBlockNum = 
				PhysicalBlock(fs, buf1->buf_BlockNum);
		fs->fs_Super->sb_MapBlockMirror = 
				PhysicalBlock(fs, buf2->buf_BlockNum);
		DirtyBuffer(fs->fs_SuperBuf);
	}
	return 0;
}

/******************************************************************************
  Remap a block (usually because a write to the block has failed).
*/
Err
RemapBlock(AcroFS *fs, BlockNum block)
{
	Buffer *mapBuf;
	Buffer *newBuf;
	MapBlock *mb;
	uint32 indx;
	Err err;

	DBUG_MAP(("  RemapBlock(%x)\n", block));
	err = AllocAcroBlock(fs, ALLOC_CRITICAL, &newBuf);
	if (err < 0)
		return err;

	assert(!IsEmptyList(&fs->fs_MapBlockList));
	mapBuf = (Buffer *) FirstNode(&fs->fs_MapBlockList);
	mb = (MapBlock *) mapBuf->buf_Data;
	indx = MapBlockArrayEnd(fs, mb);
	if (indx >= MAX_MAPBLOCK_ENTRIES(fs))
	{
		/* New entry won't fit in the MapBlock.
		 * Allocate a new MapBlock.
		 */
		err = AllocAcroBlock(fs, ALLOC_CRITICAL, &mapBuf);
		DBUG_MAP(("   RemapBlock(%x): alloc new mapblock %x\n", 
			block, mapBuf->buf_BlockNum));
		if (err < 0)
			return err;
		mb = (MapBlock *) mapBuf->buf_Data;
		memset(mb, 0, fs->fs_fs.fs_VolumeBlockSize);
		InitMetaHdr(&mb->mb_Hdr, META_MAP_BLOCK);
		mb->mb_NextMapBlock = fs->fs_Super->sb_MapBlockNum;
		for (indx = 0;  indx < MAX_MAPBLOCK_ENTRIES(fs);  indx++)
			mb->mb_Map[indx].me_VirtBlock = NULL_BLOCKNUM;
		/* Move the buffer to the MapBlock list. */
		UnlinkBuffer(mapBuf);
		AddHead(&fs->fs_MapBlockList, (Node *) &mapBuf->buf_Node);
		/* Keep the same mirror block. */
		fs->fs_Super->sb_MapBlockNum = 
			PhysicalBlock(fs, mapBuf->buf_BlockNum);
		DirtyBuffer(fs->fs_SuperBuf);
		indx = 0;
	}
	DBUG_MAP(("   Add block %x to mapblock %x indx %x\n",
		block, mapBuf->buf_BlockNum, indx));
	mb->mb_Map[indx].me_VirtBlock = block;
	mb->mb_Map[indx].me_PhysBlock = PhysicalBlock(fs, newBuf->buf_BlockNum);
	if (indx < MAX_MAPBLOCK_ENTRIES(fs)-1)
		mb->mb_Map[indx+1].me_VirtBlock = NULL_BLOCKNUM;
	/* Lose this (virtual) block forever. */
	DeleteBuffer(newBuf);

	err = WriteMapBlock(fs, mapBuf);
	if (err < 0)
		return err;
	return 0;
}

/******************************************************************************
*/
#if defined(BUILD_STRINGS) && defined(DEBUG)
static void
DumpMapBlock(Buffer *buf)
{
	uint32 indx;
	MapBlock *mb = (MapBlock *) buf->buf_Data;

	DBUG_MAP(("MapBlock %x (buf %x): ", buf->buf_BlockNum, buf));
	for (indx = 0;  indx < MAX_MAPBLOCK_ENTRIES(buf->buf_FS);  indx++)
	{
		if (mb->mb_Map[indx].me_VirtBlock == NULL_BLOCKNUM)
			break;
		DBUG_MAP(("%x->%x ", 
			mb->mb_Map[indx].me_VirtBlock,
			mb->mb_Map[indx].me_PhysBlock));
	}
	DBUG_MAP(("\n"));
}
#else
#define	DumpMapBlock(mb)
#endif

/******************************************************************************
  Mount-time initialization.
*/
Err
InitMapper(AcroFS *fs)
{
	BlockNum block;
	Buffer *buf;
	MapBlock *mb;
	Err err;

	/* Read the map block head. */
	err = ReadMirroredBlock(fs, 
		fs->fs_Super->sb_MapBlockNum, 
		fs->fs_Super->sb_MapBlockMirror, 
		META_MAP_BLOCK, BLK_PHYS, &buf);
	if (err < 0)
		return err;
	if (err != MIRROR_ALL_VALID)
		fs->fs_Flags |= ACRO_REWRITE_MAPBLOCKS;

	/* Read the rest of the map blocks. */
	for (;;)
	{
		DumpMapBlock(buf);
		mb = (MapBlock *) buf->buf_Data;
		block = mb->mb_NextMapBlock;
		UnlinkBuffer(buf);
		AddTail(&fs->fs_MapBlockList, (Node *) &buf->buf_Node);
		if (block == NULL_BLOCKNUM)	
			break;
		err = ReadMetaBlock(fs, block, META_MAP_BLOCK, BLK_PHYS, &buf);
		if (err < 0)
			return err;
	}
	return 0;
}
