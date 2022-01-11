/*
 * @(#) buf.c 96/09/25 1.3
 *
 * Routines to manipulate Buffer structures.
 */

#include <file/acromedia.h>
#include <kernel/super.h>
#include "acrofuncs.h"

extern Semaphore *fsListSemaphore;

/******************************************************************************
  Find a block in the buffer list.
*/
static Buffer *
FindBlock(AcroFS *fs, BlockNum block)
{
	Buffer *buf;

	ScanList(&fs->fs_Buffers, buf, Buffer)
	{
		assert(!(buf->buf_Flags & BUF_FREED));
		if (buf->buf_BlockNum == block)
			return buf;
	}
	return NULL;
}

/******************************************************************************
  Read a block from the device.
*/
static Err
ReadBlock(Buffer *buf, uint32 flags)
{
	uint32 devBlock;
	AcroFS *fs = buf->buf_FS;
	
	devBlock = buf->buf_BlockNum;
	if ((flags & BLK_PHYS) == 0)
		devBlock = PhysicalBlock(fs, devBlock);
	devBlock *= fs->fs_fs.fs_DeviceBlocksPerFilesystemBlock;
	return ReadDevice(fs->fs_DeviceIOReq, devBlock,
			buf->buf_Data, fs->fs_fs.fs_VolumeBlockSize);
}

/******************************************************************************
  Get a block.
  Find it in the buffer list; if it's not there, allocate a new buffer.
*/
Err
GetBlock(AcroFS *fs, BlockNum block, uint32 flags, Buffer **pBuf)
{
	Buffer *buf;
	Err err;

	assert(block < fs->fs_fs.fs_VolumeBlockCount);
	buf = FindBlock(fs, block);
	if (buf != NULL)
	{
		/* Found it. */
		DBUG_BUF(("   GetBlock(%x,%x) found %x\n", block, flags, buf));
		*pBuf = buf;
		return 0;
	}

	/* Not found: allocate a new Buffer. */
	buf = AllocMem(SIZEOF_BUFFER(fs->fs_fs.fs_VolumeBlockSize), 
			MEMTYPE_NORMAL);
	if (buf == NULL)
		return FILE_ERR_NOMEM;
	buf->buf_BlockNum = block;
	buf->buf_FS = fs;
	buf->buf_Flags = 0;

	if (flags & BLK_READ)
	{
		/* Read the block from the device. */
		err = ReadBlock(buf, flags);
		DBUG_BUF(("   GetBlock(%x,%x) read err %x\n", 
				block, flags, err));
		if (err < 0)
		{
			FreeMem(buf, SIZEOF_BUFFER(fs->fs_fs.fs_VolumeBlockSize));
			return err;
		}
	}
	DBUG_BUF(("   GetBlock(%x,%x) created %x\n", block, flags, buf));
	AddTail(&fs->fs_Buffers, (Node *) &buf->buf_Node);
	*pBuf = buf;
	return 1;
}

/******************************************************************************
  Write a block to the device.
*/
Err
WriteBlock(Buffer *buf, uint32 flags)
{
	BlockNum block;
	MetaHdr *hdr;
	AcroFS *fs = buf->buf_FS;
	Err err;

	assert(!(buf->buf_Flags & BUF_FREED));
	if (flags & BLK_META)
	{
		/* Store the checksum into the block. */
		hdr = (MetaHdr *) buf->buf_Data;
		hdr->mh_Checksum = 0;
		hdr->mh_Checksum = 
			acroChecksum(hdr, fs->fs_fs.fs_VolumeBlockSize);
	}

	/* Write the block.
	 * If the media fails and caller wants us to remap the block,
	 * remap it and try again. */
	for (;;)
	{
		block = buf->buf_BlockNum;
		if (!(flags & BLK_PHYS))
			block = PhysicalBlock(fs, block);
		block *= fs->fs_fs.fs_DeviceBlocksPerFilesystemBlock;
		err = WriteDevice(fs->fs_DeviceIOReq, block,
			buf->buf_Data, fs->fs_fs.fs_VolumeBlockSize);
		DBUG_IO(("    WriteDevice(%x,%x) phys %x, result %x\n", 
			buf->buf_BlockNum, flags, block, err));
		if (err >= 0)
		{
			/* Write succeeded. */
			buf->buf_Flags &= ~BUF_DIRTY;
			break;
		}
		if (!MEDIA_ERROR(err))
			/* Write failed (not a media error). */
			break;
		if (!(flags & BLK_REMAP))
			/* Don't remap. */
			break;
		/* Remap and try again. */
		err = RemapBlock(fs, buf->buf_BlockNum);
		if (err < 0)
			break;
	}
	return err;
}

/******************************************************************************
  Mark a buffer as "dirty".
*/
void
DirtyBuffer(Buffer *buf)
{
	assert(!(buf->buf_Flags & BUF_FREED));
	buf->buf_Flags |= BUF_DIRTY;
}

/******************************************************************************
  Mark a buffer as "not dirty".
*/
void
UndirtyBuffer(Buffer *buf)
{
	assert(!(buf->buf_Flags & BUF_FREED));
	buf->buf_Flags &= ~BUF_DIRTY;
}

/******************************************************************************
  Return the number of dirty buffers.
*/
uint32
NumDirtyBuffers(AcroFS *fs)
{
	Buffer *buf;
	uint32 num;

	num = 0;
	ScanList(&fs->fs_Buffers, buf, Buffer)
	{
		if (buf->buf_Flags & BUF_DIRTY)
			num++;
	}
	if (fs->fs_SuperBuf->buf_Flags & BUF_DIRTY)
		num++;
	return num;
}

/******************************************************************************
  Change a buffer to point to a different block.
*/
void
RetargetBuffer(Buffer *buf, BlockNum block)
{
	assert(!(buf->buf_Flags & BUF_FREED));
	buf->buf_BlockNum = block;
}

/******************************************************************************
  Free a buffer.
*/
void
FreeBuffer(Buffer *buf)
{
	File *file;
	Err err;

	/* If any Files are pointing to the buffer we're about to delete,
	 * reset their pointers.  */
	DBUG_BUF(("   FreeBuffer(%x) block %x\n", buf, buf->buf_BlockNum));
	assert(!(buf->buf_Flags & BUF_FREED));
	assert(!(buf->buf_Flags & BUF_RESIDENT));
	err = SuperInternalLockSemaphore(fsListSemaphore, 0);
	if (err < 0)
		/* Memory leak is better than scanning an unlocked list. */
		return; 
	ScanList(&fileFolio.ff_Files, file, File)
	{
		if ((AcroFS *) file->fi_FileSystem == buf->buf_FS &&
		    (Buffer *) file->fi_FilesystemSpecificData == buf)
			file->fi_FilesystemSpecificData = 0;
	}
	(void) SuperInternalUnlockSemaphore(fsListSemaphore);

	buf->buf_Flags |= BUF_FREED;
	FreeMem(buf, SIZEOF_BUFFER(buf->buf_FS->fs_fs.fs_VolumeBlockSize));
}

/******************************************************************************
  Unlink a buffer from whatever list it is in.
*/
void
UnlinkBuffer(Buffer *buf)
{
	DBUG_BUF(("   UnlinkBuf(%x) block %x\n", buf, buf->buf_BlockNum));
	RemNode((Node *) &buf->buf_Node);
}

/******************************************************************************
  Delete a buffer.
*/
void
DeleteBuffer(Buffer *buf)
{
	assert(!(buf->buf_Flags & BUF_FREED));
	assert(!(buf->buf_Flags & BUF_RESIDENT));
	assert(!(buf->buf_Flags & BUF_DIRTY));
	UnlinkBuffer(buf);
	FreeBuffer(buf);
}

/******************************************************************************
  Delete all buffers (except the RESIDENT ones).
*/
Err
DeleteBuffers(AcroFS *fs, bool forceUndirty)
{
	Buffer *buf;
	Buffer *nextBuf;
	Err err;

	for (buf = (Buffer *) FirstNode(&fs->fs_Buffers);
	     IsNode(&fs->fs_Buffers, &buf->buf_Node);
	     buf = nextBuf)
	{
		nextBuf = (Buffer *) NextNode((Node *) &buf->buf_Node);
		if (!(buf->buf_Flags & BUF_RESIDENT))
		{
			/* Not resident; just delete it. */
			if (forceUndirty)
				UndirtyBuffer(buf);
			DeleteBuffer(buf);
		} else
		{
			/* Resident; re-read it from the device.
			 * We don't trust the data that's in the buffer now. */
			err = ReadBlock(buf, 0);
			if (err < 0)
			{
				DBUG_DAMAGE(("DAMAGE: DeleteBuffers: "
					"block %x, err %x\n",
					buf->buf_BlockNum, err));
				return FILE_ERR_DAMAGED;
			}
		}
	}
	/* Superblock buffer is always RESIDENT. */
	err = ReadBlock(fs->fs_SuperBuf, 0);
	if (err < 0)
	{
		DBUG_DAMAGE(("DAMAGE: DeleteBuffers: "
			"superblock %x, err %x\n",
			fs->fs_SuperBuf->buf_BlockNum, err));
		return FILE_ERR_DAMAGED;
	}
	return 0;
}

/******************************************************************************
  Mount-time initialization.
*/
Err
InitBuffers(AcroFS *fs)
{
	TOUCH(fs);
	return 0;
}
