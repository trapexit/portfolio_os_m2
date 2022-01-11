/*
 * @(#) freelist.c 96/09/25 1.4
 *
 * Routines to manipulate the block freelist.
 */

#include <file/acromedia.h>
#include <kernel/bitarray.h>
#include "acrofuncs.h"

typedef struct BlockNode
{
	MinNode		bn_Node;
	BlockNum	bn_BlockNum;
} BlockNode;



/******************************************************************************
  Was a block freed during the current transaction?
  (If so, we don't want to reuse it until the transaction is finished.)
*/
static bool
FreedThisTransaction(AcroFS *fs, BlockNum block)
{
	BlockNode *bn;

	ScanList(&fs->fs_FreedThisTransaction, bn, BlockNode)
	{
		if (bn->bn_BlockNum == block)
			return TRUE;
	}
	return FALSE;
}

/******************************************************************************
  Remember that a block has been freed in the current transaction.
*/
static Err
MakeFreedThisTransaction(AcroFS *fs, BlockNum block)
{
	BlockNode *bn;

	bn = (BlockNode *) AllocMem(sizeof(BlockNode), MEMTYPE_NORMAL);
	if (bn == NULL)
		return FILE_ERR_NOMEM;
	bn->bn_BlockNum = block;
	AddTail(&fs->fs_FreedThisTransaction, (Node *) &bn->bn_Node);
	return 0;
}

/******************************************************************************
  Forget about blocks freed during the current transaction.
*/
void
ClearFreedThisTransaction(AcroFS *fs)
{
	BlockNode *bn;

	while ((bn = (BlockNode *) RemHead(&fs->fs_FreedThisTransaction)) != NULL)
		FreeMem(bn, sizeof(BlockNode));
}

/******************************************************************************
  Scan the free list and determine how many blocks are free.
  This can be a rather expensive operation.
*/
static uint32
CountFreeBlocks(AcroFS *fs)
{
	BlockNum block;
	uint32 numFreeBlocks;
	Buffer *buf;
	FreeListBlock *fl;
	uint32 indx;
	Err err;

	numFreeBlocks = 0;
	for (block = fs->fs_Super->sb_FreeListBlockNum;  
	     block != NULL_BLOCKNUM;  
	     block = fl->fl_NextFreeListBlock)
	{
		err = ReadMetaBlock(fs, block, META_FREELIST_BLOCK, 0, &buf);
		if (err < 0)
			break;
		fl = (FreeListBlock *) buf->buf_Data;
		numFreeBlocks++; /* Count one for the FreeListBlock itself. */
		for (indx = 0;  indx < MAX_FREELISTBLOCK_ENTRIES(fs);  indx++)
		{
			if (fl->fl_FreeBlocks[indx] != NULL_BLOCKNUM)
				numFreeBlocks++;
		}
	}
	return numFreeBlocks;
}

#ifndef CheckFreeBlockCount
/******************************************************************************
  Make sure that fs_NumFreeBlocks is correct.
*/
void
CheckFreeBlockCount(AcroFS *fs, char *msg)
{
	uint32 n;

	if (!(acroDebug & DB_FREE))
		return;
	n = CountFreeBlocks(fs);
	if (n != fs->fs_NumFreeBlocks)
	{
		printf("*** %s: CountFreeBlocks %x, fs->NumFree %x\n",
			msg, n, fs->fs_NumFreeBlocks);
	}
}
#endif

/******************************************************************************
  Check that we have enough free blocks to proceed with an allocation.
*/
static Err
CheckSpace(AcroFS *fs, uint32 flags)
{
	if (flags & ALLOC_CRITICAL)
	{
		/* Critical allocation must succeed. */
		return 0;
	}

	if (fs->fs_NumFreeBlocks > CRITICAL_BLOCK_RESERVE)
	{
		/* There's enough space for a non-critical allocation. */
		return 0;
	}

	DBUG_ANY(("No space (non-critical req, %x free)\n", 
			fs->fs_NumFreeBlocks));
	return FILE_ERR_NOSPACE;
}

/******************************************************************************
  Allocate a block.
*/
Err
AllocAcroBlock(AcroFS *fs, uint32 flags, Buffer **pBuf)
{
	Buffer *buf;
	FreeListBlock *fl;
	BlockNum block;
	PermBorrow *pb;
	uint32 indx;
	Err err;

	if (fs->fs_Borrowing != NULL)
	{
		/* We're in "borrowing" mode.
		 * Just borrow the block, but remember it in a PermBorrow
		 * structure.  We'll make it permanent when the current
		 * transaction is done (CommitPermBorrows). */
		pb = (PermBorrow *) 
			AllocMem(sizeof(PermBorrow), MEMTYPE_NORMAL);
		if (pb == NULL)
			return FILE_ERR_NOMEM;
		err = BorrowAcroBlock(fs, flags, pb, pBuf);
		if (err < 0)
		{
			FreeMem(pb, sizeof(PermBorrow));
			return err;
		}
		DBUG_MAP(("   Create PermBorrow: blk %x, fl %x, indx %x\n",
			pb->pb_BorrowedBlock, 
			pb->pb_FreeListBlock, pb->pb_FreeListIndx));
		AddTail(&fs->fs_PermBorrows, (Node* ) &pb->pb_Node);
		return 0;
	}

	err = CheckSpace(fs, flags);
	if (err < 0)
		return err;

	/* The first FreeListBlock in the chain is kept in memory.
	 * It contains an array of some free blocks.  
	 * Take the last one in the list.  */
Again:
	fl = (FreeListBlock *) fs->fs_FreeListBuf->buf_Data;
	block = NULL_BLOCKNUM;
	for (indx = 0;  indx < MAX_FREELISTBLOCK_ENTRIES(fs); indx++)
	{
		block = fl->fl_FreeBlocks[indx];
		if (block != NULL_BLOCKNUM && !FreedThisTransaction(fs, block))
			break;
	}

	if (block != NULL_BLOCKNUM)
	{
		/* Take the block and update the free list. */
		fl->fl_FreeBlocks[indx] = NULL_BLOCKNUM;
		DirtyBuffer(fs->fs_FreeListBuf);
		fs->fs_NumFreeBlocks--;
		CheckFreeBlockCount(fs, "Alloc1");
		DBUG_ALLOC(("  acroAlloc: got %x\n", block));
		return OverwriteBlock(fs, block, pBuf);
	}

	/* No more free blocks listed in this FreeListBlock.
	 * Use the FreeListBlock itself as the new one, 
	 * but first read the next FreeListBlock in the chain. */

	block = fl->fl_NextFreeListBlock;
	if (block == NULL_BLOCKNUM)
		/* Don't allow allocating the last block on the device, 
		 * so that sb_FreeListBlockNum is never NULL_BLOCKNUM.
		 */
		return FILE_ERR_NOSPACE;
	err = ReadMetaBlock(fs, block, META_FREELIST_BLOCK, 0, &buf);
	if (err < 0)
		return err;
	DBUG_ALLOC(("  acroAlloc: got %x, new freelist %x\n",
		fs->fs_FreeListBuf->buf_BlockNum, buf->buf_BlockNum));
	*pBuf = fs->fs_FreeListBuf;
	fs->fs_FreeListBuf->buf_Flags &= ~(BUF_RESIDENT | BUF_DIRTY);
	buf->buf_Flags |= BUF_RESIDENT;
	fs->fs_FreeListBuf = buf;
	fs->fs_Super->sb_FreeListBlockNum = buf->buf_BlockNum;
	DirtyBuffer(fs->fs_SuperBuf);
	fs->fs_NumFreeBlocks--;
	CheckFreeBlockCount(fs, "Alloc2");
	if (FreedThisTransaction(fs, (*pBuf)->buf_BlockNum))
	{
		/* FIXME: can this happen?  If it does, we've lost
		 * the block currently in *pBuf. */
		DBUG_ANY(("AllocAcroBlock: lost block %x\n", 
			(*pBuf)->buf_BlockNum));
		goto Again;
	}
	return 0;
}

/******************************************************************************
  Free a block.
*/
Err
FreeAcroBlock(AcroFS *fs, BlockNum freeBlock)
{
	Buffer *buf;
	FreeListBlock *fl;
	uint32 indx;
	PermBorrow *pb;
	Err err;

	if (fs->fs_Borrowing != NULL)
	{
		/* We're in "borrowing" mode.
		 * Just remove this block from the PermBorrow list. */
		ScanList(&fs->fs_PermBorrows, pb, PermBorrow)
		{
			if (pb->pb_BorrowedBlock == freeBlock)
			{
				DBUG_MAP(("   FreeAcroBlock: free PermBorrow "
					"blk %x, fl %x, indx %x\n",
					pb->pb_BorrowedBlock, 
					pb->pb_FreeListBlock, 
					pb->pb_FreeListIndx));
				RemNode((Node *) &pb->pb_Node);
				break;
			}
		}
		return 0;
	}

	/* Add the block number to the FreedThisTransaction list,
	 * so we don't reallocate it until this transaction is done.  */
	MakeFreedThisTransaction(fs, freeBlock);

	/* Put the block number in the in-memory FreeListBlock.  */
	fl = (FreeListBlock *) fs->fs_FreeListBuf->buf_Data;
	for (indx = 0;  indx < MAX_FREELISTBLOCK_ENTRIES(fs); indx++)
		if (fl->fl_FreeBlocks[indx] == NULL_BLOCKNUM)
			break;
	if (indx < MAX_FREELISTBLOCK_ENTRIES(fs))
	{
		/* Insert new free block into the FreeList array. */
		DBUG_ALLOC(("FreeAcroBlock(%x): cached fl blk %x, index %x\n",
			freeBlock, fs->fs_FreeListBuf->buf_BlockNum, indx));
		fl->fl_FreeBlocks[indx] = freeBlock;
		DirtyBuffer(fs->fs_FreeListBuf);
		fs->fs_NumFreeBlocks++;
		CheckFreeBlockCount(fs, "Free1");
		return 0;
	}

	/* The in-memory FreeListBlock is full.  
	 * Write it to the device and make the block currently being freed 
	 * a new FreeListBlock. */

	DBUG_ALLOC(("FreeAcroBlock(%x): create new FL blk\n", freeBlock));
	err = OverwriteBlock(fs, freeBlock, &buf);
	if (err < 0)
		return err;
	fl = (FreeListBlock *) buf->buf_Data;
	memset(fl, 0, fs->fs_fs.fs_VolumeBlockSize);
	InitMetaHdr(&fl->fl_Hdr, META_FREELIST_BLOCK);
	fl->fl_NextFreeListBlock = fs->fs_FreeListBuf->buf_BlockNum;
	for (indx = 0;  indx < MAX_FREELISTBLOCK_ENTRIES(fs);  indx++)
		fl->fl_FreeBlocks[indx] = NULL_BLOCKNUM;
	DirtyBuffer(buf);
	fs->fs_FreeListBuf->buf_Flags &= ~BUF_RESIDENT;
	buf->buf_Flags |= BUF_RESIDENT;
	fs->fs_FreeListBuf = buf;
	fs->fs_Super->sb_FreeListBlockNum = freeBlock;
	DirtyBuffer(fs->fs_SuperBuf);
	fs->fs_NumFreeBlocks++;
	CheckFreeBlockCount(fs, "Free2");
	return 0;
}


/******************************************************************************
  Enter "borrowing" mode.
*/
void
BeginBorrowing(AcroFS *fs)
{
	fs->fs_BorrowCtx.bor_FreeListBlock = fs->fs_Super->sb_FreeListBlockNum;
	fs->fs_BorrowCtx.bor_FreeListIndx = 0;
	fs->fs_Borrowing = &fs->fs_BorrowCtx;
}

/******************************************************************************
  End "borrowing" mode.
*/
void
EndBorrowing(AcroFS *fs)
{
	fs->fs_Borrowing = NULL;
}

/******************************************************************************
  Borrow a block.
*/
Err
BorrowAcroBlock(AcroFS *fs, uint32 flags, PermBorrow *pb, Buffer **pBuf)
{
	Buffer *buf;
	FreeListBlock *fl;
	BlockNum block;
	AcroBorrow *borrow;
	uint32 indx;
	Err err;

	borrow = fs->fs_Borrowing;
	assert(borrow != NULL);

	err = CheckSpace(fs, flags);
	if (err < 0)
		return err;

	for (;;)
	{ 
		err = ReadMetaBlock(fs, borrow->bor_FreeListBlock,
				META_FREELIST_BLOCK, 0, &buf);
		if (err < 0)
			return err;
		fl = (FreeListBlock *) buf->buf_Data;
		block = NULL_BLOCKNUM;
		for (indx = borrow->bor_FreeListIndx;  
		     indx < MAX_FREELISTBLOCK_ENTRIES(fs); 
		     indx++)
		{
			block = fl->fl_FreeBlocks[indx];
			if (block != NULL_BLOCKNUM && 
			    !FreedThisTransaction(fs, block))
				break;
		}

		if (block != NULL_BLOCKNUM)
		{
			/* Just take a block from the array. */
			assert(block != buf->buf_BlockNum);
			borrow->bor_FreeListIndx = indx+1;
			if (pb != NULL)
			{
				pb->pb_BorrowedBlock = block;
				pb->pb_FreeListBlock = buf->buf_BlockNum;
				pb->pb_FreeListIndx = indx;
			}
			DBUG_ALLOC(("  acroBorrow: got %x\n", block));
			return OverwriteBlock(fs, block, pBuf);
		}

		/* No more blocks in this FreeList block;
		 * move on to the next one. */
		if (fl->fl_NextFreeListBlock == NULL_BLOCKNUM)
		{
			DBUG_ANY(("  acroBorrow: no more free blocks\n"));
			return FILE_ERR_NOSPACE;
		}
		borrow->bor_FreeListBlock = fl->fl_NextFreeListBlock;
		borrow->bor_FreeListIndx = 0;
	}
}

/******************************************************************************
 Commit permanent borrows.
 While in "borrowing" mode, we may have called AllocAcroBlock instead
 of BorrowAcroBlock.  If so, we need to make those allocations permanent.
 Go thru the PermBorrow list and remove those blocks from the freelist.
*/
Err
CommitPermBorrows(AcroFS *fs)
{
	Buffer *buf;
	PermBorrow *pb;
	FreeListBlock *fl;
	Err err;

	while ((pb = (PermBorrow *) RemHead(&fs->fs_PermBorrows)) != NULL)
	{
		DBUG_MAP(("CommitPermBorrows: blk %x, fl %x, indx %x\n",
			pb->pb_BorrowedBlock, 
			pb->pb_FreeListBlock, pb->pb_FreeListIndx));
		err = ReadMetaBlock(fs, pb->pb_FreeListBlock,
				META_FREELIST_BLOCK, 0, &buf);
		if (err < 0)
		{
			DBUG_DAMAGE(("DAMAGE: CommitPermBorrow: "
				"read freelist block %x, err %x\n", 
				pb->pb_FreeListBlock));
			return FILE_ERR_DAMAGED;
		}
		fl = (FreeListBlock *) buf->buf_Data;
		assert(fl->fl_FreeBlocks[pb->pb_FreeListIndx] ==
			pb->pb_BorrowedBlock);
		fl->fl_FreeBlocks[pb->pb_FreeListIndx] = NULL_BLOCKNUM;
		/* Remapping the write may cause us to borrow another block.
		 * That's ok; it just goes on the end of the PermBorrow list,
		 * and we'll process it in a later trip thru this loop. */
		err = WriteBlock(buf, BLK_META | BLK_REMAP);
		if (err < 0)
		{
			DBUG_DAMAGE(("DAMAGE: CommitPermBorrow: "
				"write freelist block %x, err %x\n",
				buf->buf_BlockNum));
			return FILE_ERR_DAMAGED;
		}
		FreeMem(pb, sizeof(PermBorrow));
		fs->fs_NumFreeBlocks--;
		CheckFreeBlockCount(fs, "CommitPermBorrows");
	}
	return 0;
}

/******************************************************************************
  Delete all PermBorrow structures.
*/
void
DeletePermBorrows(AcroFS *fs)
{
	PermBorrow *pb;

	while ((pb = (PermBorrow *) RemHead(&fs->fs_PermBorrows)) != NULL)
	{
		FreeMem(pb, sizeof(PermBorrow));
	}
}

/******************************************************************************
  Mount-time initialization.
*/
Err
InitFreeList(AcroFS *fs)
{
	Buffer *buf;
	Err err;

	/* Read the head of the FreeListBlock chain into memory.  */
	err = ReadMetaBlock(fs, fs->fs_Super->sb_FreeListBlockNum, 
			META_FREELIST_BLOCK, 0, &buf);
	if (err < 0)
		return err;
	buf->buf_Flags |= BUF_RESIDENT;
	fs->fs_FreeListBuf = buf;
	fs->fs_Borrowing = NULL;
	fs->fs_NumFreeBlocks = CountFreeBlocks(fs);
	return 0;
}

/******************************************************************************
  Set bits in a bit array.
*/
static Err
SetBits(uint32 *array, uint32 arraySize, uint32 bit1, uint32 bit2)
{
	if (bit1 > bit2 || bit1 >= arraySize * 32)
		return FILE_ERR_DAMAGED;
	SetBitRange(array, bit1, bit2);
	return 0;
}

#define	SetBit(array,size,bit)	SetBits(array,size,bit,bit)

/******************************************************************************
  Mark all the blocks associated with a file.
*/
static Err
MarkInUseFile(uint32 *usedBlocks, uint32 arraySize, Buffer *fileBuf)
{
	Buffer *btBuf;
	BlockNum btBlock;
	BlockNum nextBtBlock;
	uint32 indx;
	BlockTableBlock *bt;
	FileBlock *fil = (FileBlock *) fileBuf->buf_Data;
	AcroFS *fs = fileBuf->buf_FS;
	Err err;

	/* Mark the FileBlock itself. */
	SetBit(usedBlocks, arraySize, fileBuf->buf_BlockNum);

	/* Mark the data blocks (pointed to by the FileBlock). */
	for (indx = 0;  indx < MAX_FILEBLOCK_ENTRIES(fs);  indx++)
	{
		if (fil->fil_Blocks[indx] == NULL_BLOCKNUM)
			break;
		SetBit(usedBlocks, arraySize, fil->fil_Blocks[indx]);
	}

	/* Mark the BlockTableBlocks and the data blocks 
	 * (pointed to by the BlockTables). */
	for (btBlock = fil->fil_BlockTable;  
	     btBlock != NULL_BLOCKNUM;
	     btBlock = nextBtBlock)
	{
		SetBit(usedBlocks, arraySize, btBlock);
		err = ReadMetaBlock(fs, btBlock, 
				META_BLOCKTABLE_BLOCK, 0, &btBuf);
		if (err < 0)
			return err;
		bt = (BlockTableBlock *) btBuf->buf_Data;
		for (indx = 0;  indx < MAX_BLOCKTABLEBLOCK_ENTRIES(fs);  indx++)
			SetBit(usedBlocks, arraySize, bt->bt_Blocks[indx]);
		nextBtBlock = bt->bt_NextBlockTable;
		if (!(btBuf->buf_Flags & BUF_DIRTY))
			DeleteBuffer(btBuf);
	}
	return 0;
}

/******************************************************************************
  Mark all blocks associated with a directory, and
  recursively mark all the directory's children (files and directories).
*/
static Err
MarkInUseDir(uint32 *usedBlocks, uint32 arraySize, Buffer *dirBuf)
{
	Buffer *buf;
	BlockNum block;
	BlockNum nextBlock;
	uint32 hash;
	DirEntryHdr *deh;
	DirBlock *dir = (DirBlock *) dirBuf->buf_Data;
	AcroFS *fs = dirBuf->buf_FS;
	Err err;

	/* Mark the DirBlock itself. */
	SetBit(usedBlocks, arraySize, dirBuf->buf_BlockNum);

	/* Scan the directory's children. */
	for (hash = 0;  hash < MAX_DIRBLOCK_ENTRIES(fs);  hash++)
	{
		if (dir->dir_HashTable[hash] == NULL_BLOCKNUM)
			continue;
		for (block = dir->dir_HashTable[hash];
		     block != NULL_BLOCKNUM;
		     block = nextBlock)
		{
			err = ReadMetaBlock(fs, block,
					META_FILE_OR_DIR_BLOCK, 0, &buf);
			if (err < 0)
				return err;
			deh = (DirEntryHdr *) buf->buf_Data;
			switch (deh->deh_Hdr.mh_Type)
			{
			case META_FILE_BLOCK:
				err = MarkInUseFile(usedBlocks, arraySize, buf);
				break;
			case META_DIR_BLOCK:
				err = MarkInUseDir(usedBlocks, arraySize, buf);
				break;
			default:
				err = FILE_ERR_DAMAGED;
				break;
			}
			if (err < 0)
				return err;
			nextBlock = deh->deh_NextDirEntry;
			if (!(buf->buf_Flags & BUF_DIRTY))
				DeleteBuffer(buf);
		}
	}
	return 0;
}

/******************************************************************************
*/
#if 0
static void
DumpFreeBlocks(AcroFS *fs, uint32 *usedBlocks)
{
	BlockNum block;
	bool inRun = 0;

	printf("Free blocks:\n");
	for (block = 0; block < fs->fs_fs.fs_VolumeBlockCount;  block++)
	{
		if (!inRun)
		{
			if (IsBitRangeClear(usedBlocks, block, block))
			{
				printf("%x", block);
				if (block < fs->fs_fs.fs_VolumeBlockCount-1 &&
				 IsBitRangeClear(usedBlocks, block+1, block+1))
				{
					printf("...");
					inRun = 1;
				} else printf(", ");
			}
		} else
		{
			if (!IsBitRangeClear(usedBlocks, block, block))
				inRun = 0;
			else if (block < fs->fs_fs.fs_VolumeBlockCount-1 &&
				 !IsBitRangeClear(usedBlocks, block+1, block+1))
				printf("%x; ", block);
		}
	}
	printf("\n");
}
#endif

/******************************************************************************
  Get the next unmarked block from a bit array.
*/
static BlockNum
NextFreeBlock(AcroFS *fs, uint32 *usedBlocks, BlockNum block)
{
	do {
		block++;
		if (block >= fs->fs_fs.fs_VolumeBlockCount)
			return NULL_BLOCKNUM;
	} while (IsBitRangeSet(usedBlocks, block, block));
	return block;
}

/******************************************************************************
  Create a freelist, containing all blocks which are unmarked in the bit array.
*/
static Err
WriteNewFreeList(AcroFS *fs, uint32 *usedBlocks)
{
	Buffer *buf;
	BlockNum block;
	BlockNum nextFreeListBlock;
	uint32 indx;
	FreeListBlock *fl;
	Err err;

	nextFreeListBlock = NULL_BLOCKNUM;
	block = (BlockNum) -1;
	buf = NULL;
	indx = 0; fl = NULL; /* just for the compiler */
	while ((block = NextFreeBlock(fs, usedBlocks, block)) != NULL_BLOCKNUM)
	{
		if (buf != NULL && indx >= MAX_FREELISTBLOCK_ENTRIES(fs))
		{
			/* No room in the current FreeListBlock.
			 * Write it out. */
			for (;;)
			{
				err = WriteBlock(buf, BLK_META);
				DBUG_MOUNT(("WriteNewFL: block %x, err %x\n",
					buf->buf_BlockNum, err));
				if (err >= 0)
				{
					/* Write successful.
					 * Now start a new FreeListBlock. */
					nextFreeListBlock = buf->buf_BlockNum;
					buf = NULL;
					break;
				}
				if (!MEDIA_ERROR(err))
					return err;
				DBUG_MOUNT(("WriteNewFL: retarget %x to %x\n", 
					buf->buf_BlockNum, block));
				RetargetBuffer(buf, block);
				block = NextFreeBlock(fs, usedBlocks, block);
				if (block == NULL_BLOCKNUM)
					goto Exit;
			}
		}
		if (buf == NULL)
		{
			/* We don't have a FreeListBlock.
			 * Use this block as a FreeListBlock. */
			err = OverwriteBlock(fs, block, &buf);
			if (err < 0)
				return err;
			fl = (FreeListBlock *) buf->buf_Data;
			memset(fl, 0, fs->fs_fs.fs_VolumeBlockSize);
			InitMetaHdr(&fl->fl_Hdr, META_FREELIST_BLOCK);
			fl->fl_NextFreeListBlock = nextFreeListBlock;
			indx = 0;
			continue;
		}
		fl->fl_FreeBlocks[indx++] = block;
	}

	if (buf != NULL)
	{
		while (indx < MAX_FREELISTBLOCK_ENTRIES(fs))
			fl->fl_FreeBlocks[indx++] = NULL_BLOCKNUM;
		err = WriteBlock(buf, BLK_META);
		DBUG_MOUNT(("WriteNewFL: last block %x, err %x\n",
					buf->buf_BlockNum, err));
		if (err >= 0)
			nextFreeListBlock = buf->buf_BlockNum;
	}
Exit:
	fs->fs_Super->sb_FreeListBlockNum = nextFreeListBlock;
	DirtyBuffer(fs->fs_SuperBuf);
	return 0;
}

/******************************************************************************
  Rebuild the free list on the device.
*/
Err
RebuildFreeList(AcroFS *fs)
{
	Buffer *buf;
	uint32 *usedBlocks;
	uint32 arraySize;
	BlockNum block;
	BlockNum nextBlock;
	LogInfoBlock *logInfo;
	MapBlock *mb;
	uint32 labelSize;
	uint32 indx;
	Err err;

	/* Create a bit array.  Mark all used blocks. */
	arraySize = ROUNDUP(fs->fs_fs.fs_VolumeBlockCount, 32) / 8;
	usedBlocks = AllocMem(arraySize, MEMTYPE_NORMAL | MEMTYPE_FILL);
	if (usedBlocks == NULL)
		return FILE_ERR_NOMEM;

	/* Mark the label blocks in-use. */
	labelSize = ROUNDUP(sizeof(ExtVolumeLabel), 
				fs->fs_fs.fs_VolumeBlockSize);
	err = SetBits(usedBlocks, arraySize, 
		0, labelSize / fs->fs_fs.fs_VolumeBlockSize);
	if (err < 0)
		goto Exit;


	/* Mark the SuperBlocks in-use. */
	err = SetBits(usedBlocks, arraySize,
		fs->fs_MinSuperBlock, fs->fs_MaxSuperBlock);
	if (err < 0)
		goto Exit;

	/* Mark the MapBlocks in-use. */
	err = SetBit(usedBlocks, arraySize, fs->fs_Super->sb_MapBlockNum);
	if (err < 0)
		goto Exit;
	err = SetBit(usedBlocks, arraySize, fs->fs_Super->sb_MapBlockMirror);
	if (err < 0)
		goto Exit;
	ScanList(&fs->fs_MapBlockList, buf, Buffer)
	{
		err = SetBit(usedBlocks, arraySize, buf->buf_BlockNum);
		if (err < 0)
			goto Exit;
		mb = (MapBlock *) buf->buf_Data;
		for (indx = 0;  indx < MAX_MAPBLOCK_ENTRIES(fs);  indx++)
		{
			if (mb->mb_Map[indx].me_VirtBlock == NULL_BLOCKNUM)
				break;
			SetBit(usedBlocks, arraySize, 
				mb->mb_Map[indx].me_PhysBlock);
		}
	}

	/* Mark the LogInfoBlocks in-use */
	for (block = fs->fs_Super->sb_LogInfoBlockNum;
	     block != NULL_BLOCKNUM;
	     block = nextBlock)
	{
		err = SetBit(usedBlocks, arraySize, block);
		if (err < 0)
			goto Exit;
		err = ReadMetaBlock(fs, block, META_LOGINFO_BLOCK, 0, &buf);
		if (err < 0)
			break;
		logInfo = (LogInfoBlock *) buf->buf_Data;
		nextBlock = logInfo->li_NextLogInfoBlock;
		if (!(logInfo->li_Hdr.mh_Flags & LI_VALID))
			continue;
		/* Add all the Log data blocks to the FreedThisTransaction
		 * list, so they don't get reallocated during ProcessLog(). */
		for (indx = 0;  indx < MAX_LOGINFOBLOCK_ENTRIES(fs); indx++)
		{
			if (logInfo->li_Log[indx].le_DataBlock == NULL_BLOCKNUM)
				break;
			err = MakeFreedThisTransaction(fs,
					logInfo->li_Log[indx].le_DataBlock);
			if (err < 0)
				goto Exit;
		}
	}
	err = SetBit(usedBlocks, arraySize, fs->fs_Super->sb_LogInfoBlockMirror);
	if (err < 0)
		goto Exit;

	/* Scan the directory tree, marking all FileBlocks, DirBlocks,
	 * BlockTableBlocks and data blocks in-use. */
	err = ReadMetaBlock(fs, fs->fs_Super->sb_RootDirBlockNum,
			META_DIR_BLOCK, 0, &buf);
	if (err < 0)
		return err;
	err = MarkInUseDir(usedBlocks, arraySize, buf);
	if (err < 0)
		goto Exit;
	
	err = WriteNewFreeList(fs, usedBlocks);
Exit:
	FreeMem(usedBlocks, arraySize);
	return err;
}

