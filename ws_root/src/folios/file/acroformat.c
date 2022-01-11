/* @(#) acroformat.c 96/11/18 1.5 */

#include <file/acromedia.h>
#include <file/discdata.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <string.h>

/* #define DEBUG */


typedef struct FormatInfo
{
	IOReq *		fmt_IOReq;
	uint32		fmt_MinSuperBlock;
	uint32		fmt_MaxSuperBlock;
	uint32		fmt_BlockSize;
	uint32		fmt_NumBlocks;
	uint32		fmt_DeviceBlocksPerBlock;
	BlockNum *	fmt_AllocBlock;
} FormatInfo;

extern uint32 acroChecksum(void *data, uint32 len);
extern Err WriteDevice(IOReq *ioreq, BlockNum block, void *data, uint32 dataSize);
extern void InitMetaHdr(MetaHdr *hdr, MetaBlockType type);

#define   ROUNDUP(v,sz)           (((v)+(sz)-1) & ~((sz)-1))

#ifdef DEBUG
#define	DBUG(x)		printf x
#else
#define	DBUG(x)
#endif


#ifdef DEBUG
/******************************************************************************
*/
void
DumpBytes(char *str, void *buffer, uint32 size)
{
	int i;
	uint8 *buf = buffer;

	printf("=== %s ===\n", str);
	for (i = 0;  i < size;  i++)
	{
		printf("%02x ", buf[i]);
		if ((i % 16) == 15)  printf("\n");
	}
	if ((size % 16) != 0)
		printf("\n");
}
#endif

/******************************************************************************
*/
static uint32
GenerateUniqueID(void)
{
	TimerTicks tt;
	TimeVal tv;
	int32 repeat = 1;
	static uint32 next = 0;

	if (next == 0)
	{
		SampleSystemTimeTT(&tt);
		ConvertTimerTicksToTimeVal(&tt, &tv);
		next = tv.tv_usec;
		repeat = 10;
	}
	do {
		next = next * 1103515245 + 12345;
	} while (--repeat > 0);
	return next;
}

/******************************************************************************
*/
static void
StoreAcroChecksum(void *data, uint32 fsBlockSize)
{
	MetaHdr *hdr = data;

	hdr->mh_Checksum = 0;
	hdr->mh_Checksum = acroChecksum(data, fsBlockSize);
}

/******************************************************************************
  Find a free block and write the specified data there.
  A free block is found using a simple sequential algorithm.
  *pBlock gets the block number of the block we've written.
*/
static Err 
AllocAndFormatDeviceBlock(FormatInfo *fmt, void *data, BlockNum *pBlock)
{
	BlockNum block;
	Err err;

	/* This function should only deal with metablocks, so it
	 * is safe to assume the block begins with a MetaHdr.  */
	StoreAcroChecksum(data, fmt->fmt_BlockSize);

	for (;;)
	{
		/* Get the next available block. */
		block = (*(fmt->fmt_AllocBlock))++;
		if (block >= fmt->fmt_NumBlocks)
		{
			err = FILE_ERR_NOSPACE;
			break;
		}
		err = WriteDevice(fmt->fmt_IOReq, 
				block * fmt->fmt_DeviceBlocksPerBlock, 
				data, fmt->fmt_BlockSize);
		DBUG(("Alloc&Format: Write(%x,%x,%x,%x) ret %x\n",
			fmt->fmt_IOReq, block, data, 
			fmt->fmt_BlockSize, err));
		if (err >= 0)
		{
			/* Write was successful. */
			*pBlock = block;
			break;
		}
		/* If the write failed due to a media error, just skip
		 * this block and try the next block on the device.
		 * If it was some other failure, abort. */
		if (!MEDIA_ERROR(err))
			break;
	}
	return err;
}

/******************************************************************************
*/
static Err
AllocAndFormatMirroredBlock(FormatInfo *fmt, void *data, 
		BlockNum *pBlock1, BlockNum *pBlock2)
{
	Err err;

	err = AllocAndFormatDeviceBlock(fmt, data, pBlock1);
	if (err < 0)
		return err;
	err = AllocAndFormatDeviceBlock(fmt, data, pBlock2);
	if (err < 0)
		return err;
	return 0;
}

/******************************************************************************
*/
static Err
FormatLabel(FormatInfo *fmt, char *name)
{
	uint32 labelSize;
	BlockNum labelBlock;
	ExtVolumeLabel *label;
	Err err;

	/* Allocate label. */
	labelSize = ROUNDUP(sizeof(ExtVolumeLabel), fmt->fmt_BlockSize);
	label = (ExtVolumeLabel *) 
		AllocMem(labelSize, MEMTYPE_NORMAL | MEMTYPE_FILL);
	if (label == NULL)
		return NOMEM;

	/* Initialize label. */
	memset(label, 0, labelSize);
	label->dl_RecordType = RECORD_STD_VOLUME;
	memset(label->dl_VolumeSyncBytes, VOLUME_SYNC_BYTE,
			VOLUME_SYNC_BYTE_LEN);
	label->dl_VolumeStructureVersion = VOLUME_STRUCTURE_ACROBAT;
	label->dl_VolumeFlags = VF_M2 | VF_M2ONLY;
	strncpy(label->dl_VolumeIdentifier, name, 
			sizeof(label->dl_VolumeIdentifier) - 1);
	label->dl_VolumeUniqueIdentifier = GenerateUniqueID();
	label->dl_VolumeBlockSize = fmt->fmt_BlockSize;
	label->dl_VolumeBlockCount = fmt->fmt_NumBlocks;
	label->dl_RootUniqueIdentifier = GenerateUniqueID();
	label->dl_RootDirectoryBlockCount = 1;
	label->dl_RootDirectoryBlockSize = fmt->fmt_BlockSize;
	label->dl_RootDirectoryLastAvatarIndex = 0;
	label->dl_NumRomTags = 0;
	label->dl_ApplicationID = 0;

	labelBlock = *(fmt->fmt_AllocBlock);
	label->dl_MinSuperBlock = fmt->fmt_MinSuperBlock = 
		labelBlock + (labelSize / fmt->fmt_BlockSize);
	label->dl_MaxSuperBlock = fmt->fmt_MaxSuperBlock = 
		fmt->fmt_MinSuperBlock + NUM_SUPER_BLOCKS - 1;
	DBUG(("MinSuper %x, MaxSuper %x\n", 
		fmt->fmt_MinSuperBlock, fmt->fmt_MaxSuperBlock));

	/* Write label to device. */
	err = WriteDevice(fmt->fmt_IOReq, 
			labelBlock * fmt->fmt_DeviceBlocksPerBlock,
			label, labelSize);
	DBUG(("FormatLabel: Write(%x,%x,%x,%x) ret %x\n",
		fmt->fmt_IOReq, labelBlock, label, labelSize, err));
	FreeMem(label, labelSize);
	*(fmt->fmt_AllocBlock) = fmt->fmt_MaxSuperBlock + 1;
	return err;
}

/******************************************************************************
*/
static Err
FormatMapBlocks(FormatInfo *fmt, void *data, BlockNum *pBlock1, BlockNum *pBlock2) 
{
	MapBlock *mb;
	uint32 indx;
	
	memset(data, 0, fmt->fmt_BlockSize);
	mb = data;
	InitMetaHdr(&mb->mb_Hdr, META_MAP_BLOCK);
	mb->mb_NextMapBlock = NULL_BLOCKNUM;
	for (indx = 0;  indx < MAX_MAPBLOCK_ENTRIES_BS(fmt->fmt_BlockSize);  indx++)
		mb->mb_Map[indx].me_VirtBlock = NULL_BLOCKNUM;
	return AllocAndFormatMirroredBlock(fmt, data, pBlock1, pBlock2);
}

/******************************************************************************
*/
static Err
FormatLogInfoBlocks(FormatInfo *fmt, void *data, BlockNum *pBlock1, BlockNum *pBlock2)
{
	LogInfoBlock *logInfo;

	memset(data, 0, fmt->fmt_BlockSize);
	logInfo = data;
	InitMetaHdr(&logInfo->li_Hdr, META_LOGINFO_BLOCK);
	logInfo->li_NextLogInfoBlock = NULL_BLOCKNUM;

	/* Special setup: setting the LI_VALID bit with the array empty
	 * just causes the FS to rebuild the free list at mount time.
	 * This avoids needing to duplicate the code to initialize 
	 * the freelist here. */
	logInfo->li_Hdr.mh_Flags = LI_VALID;
	logInfo->li_Log[0].le_DataBlock = NULL_BLOCKNUM;
	return AllocAndFormatMirroredBlock(fmt, data, pBlock1, pBlock2);
}

/******************************************************************************
*/
static Err
FormatRootDir(FormatInfo *fmt, void *data, BlockNum *pBlock)
{
	DirBlock *root;
	uint32 indx;

	memset(data, 0, fmt->fmt_BlockSize);
	root = data;
	InitMetaHdr(&root->dir_de.deh_Hdr, META_DIR_BLOCK);
	strcpy(root->dir_de.deh_Name, "*ROOT*");
	root->dir_de.deh_Parent = NULL_BLOCKNUM;
	root->dir_de.deh_FileType = FILE_TYPE_DIRECTORY;
	root->dir_de.deh_Version = 0;
	root->dir_de.deh_Revision = 0;
	SampleSystemTimeTV(&root->dir_de.deh_ModDate);
	root->dir_de.deh_NextDirEntry = NULL_BLOCKNUM;
	for (indx = 0;  indx < MAX_DIRBLOCK_ENTRIES_BS(fmt->fmt_BlockSize);  indx++)
		root->dir_HashTable[indx] = NULL_BLOCKNUM;
	return AllocAndFormatDeviceBlock(fmt, data, pBlock);
}

/******************************************************************************
*/
static Err
FormatSuperBlock(FormatInfo *fmt, void *data)
{
	BlockNum block;
	Err err;

	StoreAcroChecksum(data, fmt->fmt_BlockSize);
	for (block = fmt->fmt_MaxSuperBlock - 1;
	     block >= fmt->fmt_MinSuperBlock;
	     block -= 2)
	{
		err = WriteDevice(fmt->fmt_IOReq, 
				block * fmt->fmt_DeviceBlocksPerBlock,
				data, fmt->fmt_BlockSize);
		DBUG(("Write super block %x, err %x\n", block, err));
		if (err < 0 && MEDIA_ERROR(err))
			continue;
		if (err < 0)
			return err;

		err = WriteDevice(fmt->fmt_IOReq,
				(block+1) * fmt->fmt_DeviceBlocksPerBlock,
				data, fmt->fmt_BlockSize);
		DBUG(("Write super block mirror %x, err %x\n", block+1, err));
		if (err < 0 && MEDIA_ERROR(err))
			continue;
		if (err < 0)
			return err;
		
		/* Successfully wrote both superblocks.
		 * Now keep going, and invalidate all the earlier ones. */
		memset(data, 0xBD, fmt->fmt_BlockSize);
		while (--block >= fmt->fmt_MinSuperBlock)
		{
			err = WriteDevice(fmt->fmt_IOReq, 
					block * fmt->fmt_DeviceBlocksPerBlock,
					data, fmt->fmt_BlockSize);
			DBUG(("Trash super block %x, err %x\n", block, err));
			TOUCH(err);
		}
		return 0;
	}
	return FILE_ERR_NOSPACE;
}

/******************************************************************************
*/
Err
acroFormat(Item ioreqItem, const DeviceStatus *stat, const char *name, const TagArg *tags)
{
	BlockNum allocBlock;
	void *blockBuffer;
	SuperBlock *super;
	FormatInfo fmt;
	Err err;

	TOUCH(tags);
	memset(&fmt, 0, sizeof(fmt));
	fmt.fmt_BlockSize = stat->ds_DeviceBlockSize;
	if (fmt.fmt_BlockSize < MIN_ACRO_BLOCK_SIZE)
		fmt.fmt_BlockSize = MIN_ACRO_BLOCK_SIZE;
	fmt.fmt_NumBlocks = 
		(stat->ds_DeviceBlockCount * stat->ds_DeviceBlockSize) / 
			fmt.fmt_BlockSize;
	fmt.fmt_DeviceBlocksPerBlock = 
		fmt.fmt_BlockSize / stat->ds_DeviceBlockSize;

	fmt.fmt_IOReq = CheckItem(ioreqItem, KERNELNODE,IOREQNODE);
	DBUG(("acroFormat(%x,%s) blkSize %x numBlks %x ioreq %x\n",
		ioreqItem, name, 
		fmt.fmt_BlockSize, fmt.fmt_NumBlocks, fmt.fmt_IOReq));
	if (fmt.fmt_IOReq == NULL)
		return BADITEM;

	/* Write the volume label */
	allocBlock = stat->ds_DeviceBlockStart;
	fmt.fmt_AllocBlock = &allocBlock;
	err = FormatLabel(&fmt, name);
	DBUG(("acroFormat: FormatLabel ret %x\n", err));
	if (err < 0)
		return err;

	/* Allocate a buffer for the superblock */
	super = (SuperBlock *) 
		AllocMem(fmt.fmt_BlockSize, MEMTYPE_NORMAL | MEMTYPE_FILL);
	if (super == NULL)
		return NOMEM;
	memset(super, 0, fmt.fmt_BlockSize);
	InitMetaHdr(&super->sb_Hdr, META_SUPER_BLOCK);
	super->sb_Flags = 0;
	SampleSystemTimeTV(&super->sb_CreateDate);

	/* Get a spare block-sized buffer */
	blockBuffer = AllocMem(fmt.fmt_BlockSize, MEMTYPE_NORMAL);
	if (blockBuffer == NULL)
	{
		err = NOMEM;
		goto Exit;
	}

	/* Write empty MapBlocks */
	err = FormatMapBlocks(&fmt, blockBuffer,
		&super->sb_MapBlockNum, &super->sb_MapBlockMirror);
	DBUG(("acroFormat: FormatMapBlocks ret %x\n", err));
	if (err < 0)
		goto Exit;

	/* Write empty LogInfoBlock */
	err = FormatLogInfoBlocks(&fmt, blockBuffer,
		&super->sb_LogInfoBlockNum, &super->sb_LogInfoBlockMirror);
	DBUG(("acroFormat: FormatLogInfoBlocks ret %x\n", err));
	if (err < 0)
		goto Exit;

	/* Write empty DirBlock for the root directory */
	err = FormatRootDir(&fmt, blockBuffer, 
		&super->sb_RootDirBlockNum);
	DBUG(("acroFormat: FormatRootDir ret %x\n", err));
	if (err < 0)
		goto Exit;

	/* Finally, write the superblock */
	err = FormatSuperBlock(&fmt, super);
	DBUG(("acroFormat: FormatSuperBlock ret %x\n", err));
Exit:
	DBUG(("acroFormat: exit %x\n", err));
	if (super != NULL)
		FreeMem(super, fmt.fmt_BlockSize);
	if (blockBuffer != NULL)
		FreeMem(blockBuffer, fmt.fmt_BlockSize);
	return err;
}

