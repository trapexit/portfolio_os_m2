/*
 * @(#) fileblk.c 96/09/25 1.3
 *
 * Routines to manipulate BlockTables and data blocks.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Seek to a data block in a file.
  Returns a pointer to the BlockTable which points to the data block 
  in *pBtBuf, and the index within the BlockTable in *pIndex.
  If the data block is pointed to directly by the FileBlock instead of
  by a BlockTable, *pBtBuf is set to NULL, and *pIndex is set to the
  index within the FileBlock's table.
*/
static Err 
SeekFileBlock(Buffer *fileBuf, BlockNum fblock, Buffer **pBtBuf, uint32 *pIndex)
{
	BlockNum btBlock;
	Buffer *btBuf;
	BlockTableBlock *bt;
	AcroFS *fs = fileBuf->buf_FS;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;
	Err err;

	DBUG_FILE(("SeekFileBlock(%x)\n", fblock));
	if (fblock > fil->fil_NumBlocks)
		return FILE_ERR_PARAMERROR;
	btBuf = NULL;
	if (fblock >= MAX_FILEBLOCK_ENTRIES(fs))
	{
		/* Skip over the entries in the FileBlock itself. */
		fblock -= MAX_FILEBLOCK_ENTRIES(fs);
		/* Scan the chain of BlockTable blocks, till we get to the
		 * that points to the one we want.  */
		btBlock = fil->fil_BlockTable;
		for (;;)
		{
			err = ReadMetaBlock(fs, btBlock, 
					META_BLOCKTABLE_BLOCK, 0, &btBuf);
			if (err < 0)
				return err;
			if (fblock < MAX_BLOCKTABLEBLOCK_ENTRIES(fs))
				break;
			fblock -= MAX_BLOCKTABLEBLOCK_ENTRIES(fs);
			bt = (BlockTableBlock *) btBuf->buf_Data;
			btBlock = bt->bt_NextBlockTable;
		}
	}
	*pBtBuf = btBuf;
	*pIndex = (uint32) fblock;
	DBUG_FILE(("Seek: block %x, index %x\n", 
		btBuf == NULL ? 0 : btBuf->buf_BlockNum, fblock));
	return 0;
}

/******************************************************************************
  Add a block number to the table in a FileBlock.
*/
static Err
AddToFileTable(Buffer *fileBuf, uint32 indx, BlockNum newBlock)
{
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;

	assert(indx < MAX_FILEBLOCK_ENTRIES(fileBuf->buf_FS));
	fil->fil_Blocks[indx] = newBlock;
	if (indx < MAX_FILEBLOCK_ENTRIES(fileBuf->buf_FS)-1)
		fil->fil_Blocks[indx+1] = NULL_BLOCKNUM;
	DirtyBuffer(fileBuf);
	return 0;
}

/******************************************************************************
  Add a block number to the table in a BlockTable.
*/
static Err
AddToBlockTable(Buffer *btBuf, uint32 indx, BlockNum newBlock)
{
	BlockTableBlock *bt = (BlockTableBlock *) btBuf->buf_Data;

	assert(indx < MAX_BLOCKTABLEBLOCK_ENTRIES(btBuf->buf_FS));
	bt->bt_Blocks[indx] = newBlock;
	if (indx < MAX_BLOCKTABLEBLOCK_ENTRIES(btBuf->buf_FS)-1)
		bt->bt_Blocks[indx+1] = NULL_BLOCKNUM;
	DirtyBuffer(btBuf);
	return 0;
}

/******************************************************************************
  Allocate and initialize a new BlockTable.
*/
static Err
AllocBlockTable(AcroFS *fs, BlockNum newBlock, Buffer **pBuf)
{
	Buffer *buf;
	BlockTableBlock *bt;
	uint32 i;
	Err err;

	err = AllocAcroBlock(fs, 0, &buf);
	if (err < 0)
		return err;
	bt = (BlockTableBlock *) buf->buf_Data;
	memset(bt, 0, fs->fs_fs.fs_VolumeBlockSize);
	InitMetaHdr(&bt->bt_Hdr, META_BLOCKTABLE_BLOCK);
	bt->bt_NextBlockTable = NULL_BLOCKNUM;
	bt->bt_Blocks[0] = newBlock;
	for (i = 1;  i < MAX_BLOCKTABLEBLOCK_ENTRIES(fs);  i++)
		bt->bt_Blocks[i] = NULL_BLOCKNUM;
	DirtyBuffer(buf);
	*pBuf = buf;
	return 0;
}

/******************************************************************************
  Allocate the first BlockTable for a file.
*/
static Err
AllocFirstBlockTable(Buffer *fileBuf, BlockNum newBlock, Buffer **pBtBuf)
{
	Err err;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;

	err = AllocBlockTable(fileBuf->buf_FS, newBlock, pBtBuf);
	if (err < 0)
		return err;
	fil->fil_BlockTable = (*pBtBuf)->buf_BlockNum;
	DirtyBuffer(fileBuf);
	return 0;
}

/******************************************************************************
  Allocate a (non-first) BlockTable for a file.
*/
static Err
AllocNthBlockTable(Buffer *btBuf, BlockNum newBlock, Buffer **pBtBuf)
{
	Err err;
	BlockTableBlock *bt = (BlockTableBlock *) btBuf->buf_Data;

	err = AllocBlockTable(btBuf->buf_FS, newBlock, pBtBuf);
	if (err < 0)
		return err;
	bt->bt_NextBlockTable = (*pBtBuf)->buf_BlockNum;
	DirtyBuffer(btBuf);
	return 0;
}

/******************************************************************************
  Add data blocks to the end of a file.
*/
static Err
IncrFileBlocks(Buffer *fileBuf, int32 numBlocks)
{
	Buffer *buf;
	Buffer *btBuf;
	BlockNum block;
	uint32 indx;
	Err err;
	AcroFS *fs = fileBuf->buf_FS;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;

	err = SeekFileBlock(fileBuf, fil->fil_NumBlocks, &btBuf, &indx);
	if (err < 0)
		return err;

	while (numBlocks-- > 0)
	{
		err = AllocAcroBlock(fs, 0, &buf);
		if (err < 0)
			return err;
		block = buf->buf_BlockNum;
		DeleteBuffer(buf);
		if (btBuf == NULL)
		{
			/* No block table block; use table in the FileBlock */
			if (indx < MAX_FILEBLOCK_ENTRIES(fs))
			{
				/* New entry fits in the FileBlock table. */
				err = AddToFileTable(fileBuf, indx, block);
				indx++;
			} else
			{
				/* New entry doesn't fit in the FileBlock;
				 * allocate the first BlockTableBlock. */
				err = AllocFirstBlockTable(fileBuf, block, &btBuf);
				indx = 1;
			}
		} else
		{
			/* We have a block table block (btBuf). */
			if (indx < MAX_BLOCKTABLEBLOCK_ENTRIES(fs))
			{
				/* New entry fits in the BlockTable. */
				err = AddToBlockTable(btBuf, indx, block);
				indx++;
			} else
			{
				/* New entry doesn't fit in the BlockTable,
				 * allocate a new BlockTableBlock. */
				err = AllocNthBlockTable(btBuf, block, &btBuf);
				indx = 1;
			}
		}
		if (err < 0)
			return err;
		fil->fil_NumBlocks++;
		DirtyBuffer(fileBuf);
	}
	return 0;
}

/******************************************************************************
  Delete data blocks from the end of a file.
*/
static Err
DecrFileBlocks(Buffer *fileBuf, int32 numBlocks)
{
	Buffer *btBuf;
	BlockNum block;
	BlockNum fblock;
	uint32 indx;
	Err err;
	BlockTableBlock *bt;
	AcroFS *fs = fileBuf->buf_FS;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;

	if (numBlocks >= fil->fil_NumBlocks)
		fblock = 0;
	else
		fblock = fil->fil_NumBlocks - numBlocks;
	err = SeekFileBlock(fileBuf, fblock, &btBuf, &indx);
	if (err < 0)
		return err;
	for (;;)
	{
		if (btBuf == NULL)
		{
			/* There is no block table; the block is pointed to
			 * directly by the FileBlock, at index i.
			 */
			if (indx >= MAX_FILEBLOCK_ENTRIES(fs))
				block = NULL_BLOCKNUM;
			else
				block = fil->fil_Blocks[indx];
			if (block == NULL_BLOCKNUM)
			{
				/* Reached end of the table in the FileBlock.
				 * Move on to the first BlockTable.
				 */
				block = fil->fil_BlockTable;
				if (block == NULL_BLOCKNUM)
					/* No block tables. We're done. */
					break;
				fil->fil_BlockTable = NULL_BLOCKNUM;
				DirtyBuffer(fileBuf);
				err = ReadMetaBlock(fs, block, 
					META_BLOCKTABLE_BLOCK, 0, &btBuf);
				if (err < 0)
					return err;
				indx = 0;
				continue;
			}
			assert(indx < MAX_FILEBLOCK_ENTRIES(fs));
			fil->fil_Blocks[indx++] = NULL_BLOCKNUM;
			DirtyBuffer(fileBuf);
		} else
		{
			/* The block is pointed to by a block table,
			 * at index indx.
			 */
			bt = (BlockTableBlock *) btBuf->buf_Data;
			if (indx >= MAX_BLOCKTABLEBLOCK_ENTRIES(fs))
				block = NULL_BLOCKNUM;
			else
				block = bt->bt_Blocks[indx];
			if (block == NULL_BLOCKNUM)
			{
				/* Reached end of this block table.
				 * Move on to the next one.
				 */
				block = bt->bt_NextBlockTable;
				if (block == NULL_BLOCKNUM)
					/* No more block tables. We're done. */
					break;
				if (bt->bt_Blocks[0] == NULL_BLOCKNUM)
				{
					/* This BlockTable block is empty.
					 * Free it too.
					 */
					FreeAcroBlock(fs, btBuf->buf_BlockNum);
				} else
				{
					/* BlockTable block is not empty.
					 * It will now become the last
					 * block in the BlockTable chain.
					 */
					bt->bt_NextBlockTable = NULL_BLOCKNUM;
					DirtyBuffer(btBuf);
				}
				err = ReadMetaBlock(fs, block, 
					META_BLOCKTABLE_BLOCK, 0, &btBuf);
				if (err < 0)
					return err;
				indx = 0;
				continue;
			}
			assert(indx < MAX_BLOCKTABLEBLOCK_ENTRIES(fs));
			bt->bt_Blocks[indx++] = NULL_BLOCKNUM;
			DirtyBuffer(btBuf);
		}
		fil->fil_NumBlocks--;
		DirtyBuffer(fileBuf);
		(void) FreeAcroBlock(fs, block);
	}
	return 0;
}

/******************************************************************************
  Add or delete file blocks from a file.
*/
Err
AddFileBlocks(Buffer *fileBuf, int32 numBlocks)
{
	if (numBlocks > 0)
		return IncrFileBlocks(fileBuf, numBlocks);
	else if (numBlocks < 0)
		return DecrFileBlocks(fileBuf, -numBlocks);
	return 0;
}

/******************************************************************************
  Read or write data to the data blocks in a file.
*/
Err
ReadWriteBlocks(Buffer *fileBuf, BlockNum fblock, void *data, uint32 numBytes, bool writing)
{
	Buffer *btBuf;
	Buffer *buf;
	BlockNum block;
	BlockTableBlock *bt;
	uint32 indx;
	int32 n;
	uint8 *bytedata = data;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;
	AcroFS *fs = fileBuf->buf_FS;
	Err err;

	if (fblock >= fil->fil_NumBlocks)
		/* Starting block is past EOF. */
		return FILE_ERR_BADPTR;

	err = SeekFileBlock(fileBuf, fblock, &btBuf, &indx);
	if (err < 0)
		return err;
	while (numBytes > 0)
	{
		if (btBuf == NULL)
		{
			if (indx >= MAX_FILEBLOCK_ENTRIES(fs))
				block = NULL_BLOCKNUM;
			else
				block = fil->fil_Blocks[indx];
			if (block == NULL_BLOCKNUM)
			{
				block = fil->fil_BlockTable;
				if (block == NULL_BLOCKNUM)
					break;
				err = ReadMetaBlock(fs, block, 
					META_BLOCKTABLE_BLOCK, 0, &btBuf);
				if (err < 0)
					return err;
				indx = 0;
				continue;
			}
			assert(indx < MAX_FILEBLOCK_ENTRIES(fs));
			indx++;
		} else
		{
			bt = (BlockTableBlock *) btBuf->buf_Data;
			if (indx >= MAX_BLOCKTABLEBLOCK_ENTRIES(fs))
				block = NULL_BLOCKNUM;
			else
				block = bt->bt_Blocks[indx];
			if (block == NULL_BLOCKNUM)
			{
				block = bt->bt_NextBlockTable;
				if (block == NULL_BLOCKNUM)
					break;
				err = ReadMetaBlock(fs, block, 
					META_BLOCKTABLE_BLOCK, 0, &btBuf);
				if (err < 0)
					return err;
				indx = 0;
				continue;
			}
			assert(indx < MAX_BLOCKTABLEBLOCK_ENTRIES(fs));
			indx++;
		}
		n = fs->fs_fs.fs_VolumeBlockSize;
		if (n > numBytes)
			n = numBytes;
		if (!writing &&
		    n > fil->fil_NumBytes - (fblock * fs->fs_fs.fs_VolumeBlockSize))
			n = fil->fil_NumBytes - (fblock * fs->fs_fs.fs_VolumeBlockSize);
		if (n <= 0)
			break;
		DBUG_FILE(("acroRW: GetBlock(%x)\n", block));
		if (writing && n == fs->fs_fs.fs_VolumeBlockSize)
			err = GetBlock(fs, block, 0, &buf);
		else
			err = GetBlock(fs, block, BLK_READ, &buf);
		if (err < 0)
			return err;
		if (writing)
		{
			memcpy(buf->buf_Data, bytedata, n);
			err = WriteBlock(buf, BLK_REMAP);
			if (err < 0)
				return err;
		} else
		{
			memcpy(bytedata, buf->buf_Data, n);
		}
		DeleteBuffer(buf);
		if (writing &&
		    (fblock * fs->fs_fs.fs_VolumeBlockSize) + n > fil->fil_NumBytes)
		{
			fil->fil_NumBytes = (fblock * fs->fs_fs.fs_VolumeBlockSize) + n;
			DirtyBuffer(fileBuf);
		}
		numBytes -= n;
		bytedata += n;
		fblock++;
	}
	if (numBytes > 0)
	{
		/* Didn't write all the bytes. */
		return 0; /* ??? FIXME */
	}
	return 0;
}

