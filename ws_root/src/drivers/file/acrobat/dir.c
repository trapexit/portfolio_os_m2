/*
 * @(#) dir.c 96/10/14 1.7
 *
 * Routines to manipulate directories.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

/******************************************************************************
  Convert a char to lowercase.
*/
static uint8
LowerCase(uint8 ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 'a';
	return ch;
}

/******************************************************************************
  Hash a filename, to determine where to put it in its parent directory
*/
static uint32
HashName(AcroFS *fs, char *name)
{
	uint32 hash;
	uint32 i;

	hash = 0;
	for (i = 0;  name[i] != '\0';  i++)
		hash ^= LowerCase(name[i]) * i;
	return hash % MAX_DIRBLOCK_ENTRIES(fs);
}

/******************************************************************************
  Link a directory entry into a hash chain in its parent directory.
*/
static Err
LinkDirEntry(Buffer *parBuf, Buffer *newDeBuf)
{
	BlockNum block;
	BlockNum nextBlock;
	uint32 indx;
	Buffer *deBuf;
	DirEntryHdr *deh;
	DirEntryHdr *newDeh = (DirEntryHdr *) newDeBuf->buf_Data;
	DirBlock *par = (DirBlock *) parBuf->buf_Data;
	AcroFS *fs = parBuf->buf_FS;
	Err err;

	DBUG_DIR(("LinkDirEntry %s: parent %x(%x), new %x(%x)\n",
		newDeh->deh_Name, parBuf, parBuf->buf_BlockNum, 
		newDeBuf, newDeBuf->buf_BlockNum));

	/* Scan the hash chain, to find the end of the chain,
	 * and to make sure this name isn't already used. */
	indx = HashName(fs, newDeh->deh_Name);
	deBuf = NULL;
	for (block = par->dir_HashTable[indx];
	     block != NULL_BLOCKNUM;
	     block = nextBlock)
	{
		err = ReadMetaBlock(fs, block,
				META_FILE_OR_DIR_BLOCK, 0, &deBuf);
		if (err < 0)
			return err;
		deh = (DirEntryHdr *) deBuf->buf_Data;
#ifdef BUILD_PARANOIA
		if (HashName(fs, deh->deh_Name) != indx)
		{
			DBUG_DAMAGE(("DAMAGE: "
				"dir %x has block %x in chain %x, hash %x\n", 
				parBuf->buf_BlockNum, block, 
				indx, HashName(fs, deh->deh_Name)));
			return FILE_ERR_DAMAGED;
		}
#endif
		if (strcasecmp(deh->deh_Name, newDeh->deh_Name) == 0)
			return FILE_ERR_DUPLICATEFILE;
		nextBlock = deh->deh_NextDirEntry;
	}

	/* Link the new entry onto the end of the hash chain.  */
	if (deBuf == NULL)
	{
		/* Hash chain is empty:
		 * make the new one the first in the parent's chain. */
		par->dir_HashTable[indx] = newDeBuf->buf_BlockNum;
		DirtyBuffer(parBuf);
	} else
	{
		/* deBuf has the FileBlock or DirBlock of the last entry:
		 * link the new one on after it. */
		deh = (DirEntryHdr *) deBuf->buf_Data;
		deh->deh_NextDirEntry = newDeBuf->buf_BlockNum;
		DirtyBuffer(deBuf);
	}
	return 0;
}

/******************************************************************************
  Initialize a new directory entry, and write it out to the device.
*/
static Err
WriteDirEntry(char *name, Buffer *parBuf, Buffer *deBuf, uint32 fileType)
{
	uint32 indx;
	DirEntryHdr *deh = (DirEntryHdr *) deBuf->buf_Data;
	AcroFS *fs = parBuf->buf_FS;

	DBUG_DIR(("WriteDirEnt: name %s, parent %x(%x), new %x(%x)\n", name, 
		parBuf, parBuf->buf_BlockNum, deBuf, deBuf->buf_BlockNum));

	memset(deh, 0, fs->fs_fs.fs_VolumeBlockSize);
	InitMetaHdr(&deh->deh_Hdr, (fileType == FILE_TYPE_DIRECTORY) ? 
					META_DIR_BLOCK : META_FILE_BLOCK);
	strncpy(deh->deh_Name, name, FILESYSTEM_MAX_NAME_LEN-1);
	deh->deh_Name[FILESYSTEM_MAX_NAME_LEN-1] = '\0';
	deh->deh_Parent = parBuf->buf_BlockNum;
	SampleSystemTimeTV(&deh->deh_ModDate);
	deh->deh_FileType = fileType;
	deh->deh_Version = 0;
	deh->deh_Revision = 0;
	deh->deh_NextDirEntry = NULL_BLOCKNUM;

	if (fileType == FILE_TYPE_DIRECTORY)
	{
		DirBlock *dir = (DirBlock *) deh;
		for (indx = 0;  indx < MAX_DIRBLOCK_ENTRIES(fs);  indx++)
			dir->dir_HashTable[indx] = NULL_BLOCKNUM;
	} else
	{
		FileBlock *fil = (FileBlock *) deh;
		fil->fil_NumBlocks = 0;
		fil->fil_NumBytes = 0;
		fil->fil_BlockTable = NULL_BLOCKNUM;
		for (indx = 0;  indx < MAX_FILEBLOCK_ENTRIES(fs);  indx++)
			fil->fil_Blocks[indx] = NULL_BLOCKNUM;
	}
	return WriteBlock(deBuf, BLK_REMAP | BLK_META);
}

/******************************************************************************
  Create a new directory entry (file or directory).
*/
Err
AddFileOrDir(char *name, Buffer *parBuf, uint32 fileType)
{
	Buffer *deBuf;
	AcroFS *fs = parBuf->buf_FS;
	Err err;

	DBUG_DIR(("AddFileOrDir(%s,%x)\n", name, fileType));

	/* Allocate the FileBlock. */
	err = AllocAcroBlock(fs, 0, &deBuf);
	if (err < 0)
		return err;

	/* Initialize it and write it out. */
	err = WriteDirEntry(name, parBuf, deBuf, fileType);
	if (err < 0)
		goto Exit;

	/* Link it into its parent directory. */
	err = LinkDirEntry(parBuf, deBuf);
Exit:
	if (err < 0)
	{
		UndirtyBuffer(deBuf);
		FreeAcroBlock(fs, deBuf->buf_BlockNum);
	}
	return err;
}

/******************************************************************************
  Unlink a directory entry from its hash chain.
*/
static Err
UnlinkDirEntry(Buffer *parBuf, Buffer *fileBuf)
{
	DirBlock *par;
	DirEntryHdr *deh;
	DirEntryHdr *prevdeh;
	uint32 indx;
	BlockNum block;
	BlockNum nextBlock;
	Buffer *buf;
	AcroFS *fs = fileBuf->buf_FS;
	Err err;

	deh = (DirEntryHdr *) fileBuf->buf_Data;
	par = (DirBlock *) parBuf->buf_Data;

	indx = HashName(fs, deh->deh_Name);
	if (par->dir_HashTable[indx] == fileBuf->buf_BlockNum)
	{
		/* Entry is pointed to directly by the hash table
		 * in the DirBlock.  Unlink it.  */
		DBUG_DIR(("UnlinkDirEnt(%x,%x) found in parent indx %x\n",
			parBuf->buf_BlockNum, fileBuf->buf_BlockNum, indx));
		par->dir_HashTable[indx] = deh->deh_NextDirEntry;
		DirtyBuffer(parBuf);
	} else
	{
		/* Run down the hash chain and find the entry.  
		 * Unlink it.  */
		for (block = par->dir_HashTable[indx];
		     block != NULL_BLOCKNUM;
		     block = nextBlock)
		{
			err = ReadMetaBlock(fs, block, 
					META_FILE_OR_DIR_BLOCK, 0, &buf);
			DBUG_DIR(("UnlinkDirEnt(%x,%x) looking in %x, err %x\n",
				parBuf->buf_BlockNum, fileBuf->buf_BlockNum,
				block, err));
			if (err < 0)
				return err;
			prevdeh = (DirEntryHdr *) buf->buf_Data;
			nextBlock = prevdeh->deh_NextDirEntry;
			if (nextBlock == fileBuf->buf_BlockNum)
			{
				DBUG_DIR(("Found: %x points to %x\n", 
					buf->buf_BlockNum, nextBlock));
				prevdeh->deh_NextDirEntry = deh->deh_NextDirEntry;
				DirtyBuffer(buf);
				break;
			}
		}
	}
	return 0;
}

/******************************************************************************
  Delete a directory entry (file or directory).
*/
static Err
DeleteDirEntry(Buffer *fileBuf)
{
	DirEntryHdr *deh;
	Buffer *parBuf;
	AcroFS *fs = fileBuf->buf_FS;
	Err err;

	deh = (DirEntryHdr *) fileBuf->buf_Data;
	if (deh->deh_Parent == NULL_BLOCKNUM)
		/* Attempt to delete the root directory. */
		return FILE_ERR_PARAMERROR;
	err = ReadMetaBlock(fs, deh->deh_Parent, META_DIR_BLOCK, 0, &parBuf);
	DBUG_DIR(("DeleteDirEnt(%x) parent %x, err %x\n",
		fileBuf->buf_BlockNum, deh->deh_Parent, err));
	if (err < 0)
		return err;
	err = UnlinkDirEntry(parBuf, fileBuf);
	if (err < 0)
		return err;
	return FreeAcroBlock(fs, fileBuf->buf_BlockNum);
}

/******************************************************************************
  Delete a file.
*/
Err
DeleteFile(Buffer *fileBuf)
{
	FileBlock *fil;
	Err err;

	/* Truncate the file. */
	fil = (FileBlock *) fileBuf->buf_Data;
	err = AddFileBlocks(fileBuf, -(fil->fil_NumBlocks));
	DBUG_DIR(("DeleteFile(%x) AddFileBlocks err %x\n", 
		fileBuf->buf_BlockNum, err));
	if (err < 0)
	{
		/* FIXME: Warning only? */
		DBUG_ANY(("DeleteFile(%x): could not delete file blocks\n",
			fileBuf->buf_BlockNum));
	}
	return DeleteDirEntry(fileBuf);
}

/******************************************************************************
  Delete a directory.
*/
Err
DeleteDirectory(Buffer *fileBuf)
{
	DirBlock *dir;
	uint32 i;
	AcroFS *fs = fileBuf->buf_FS;

	dir = (DirBlock *) fileBuf->buf_Data;

	for (i = 0;  i < MAX_DIRBLOCK_ENTRIES(fs);  i++)
		if (dir->dir_HashTable[i] != NULL_BLOCKNUM)
			return FILE_ERR_DIRNOTEMPTY;

	return DeleteDirEntry(fileBuf);
}

/******************************************************************************
  Rename a directory entry.
*/
Err
RenameEntry(Buffer *parBuf, Buffer *fileBuf, char *filename)
{
	DirEntryHdr *deh = (DirEntryHdr *) fileBuf->buf_Data;
	Err err;

	/* Unlink it from its hash chain. */
	err = UnlinkDirEntry(parBuf, fileBuf);
	if (err < 0)
		return err;

	/* Change the name. */
	strncpy(deh->deh_Name, filename, FILESYSTEM_MAX_NAME_LEN-1);
	deh->deh_Name[FILESYSTEM_MAX_NAME_LEN-1] = '\0';
	DirtyBuffer(fileBuf);

	/* Link it into its new hash chain. */
	err = LinkDirEntry(parBuf, fileBuf);
	if (err < 0)
		return err;
	DirtyBuffer(parBuf);
	return 0;
}

/******************************************************************************
  Rename the root directory.
*/
Err
RenameRoot(Buffer *rootBuf, char *name)
{
	Buffer *buf;
	ExtVolumeLabel *label;
	AcroFS *fs = rootBuf->buf_FS;
	Err err;

	/* Change the name in the volume label. */
	err = GetBlock(fs, (BlockNum)0, BLK_READ | BLK_PHYS, &buf);
	if (err < 0)
		return err;
	/* Note: we assume dl_VolumeIdentifier falls entirely within the
	 * first block, so we don't have to deal with multiple blocks. */
	assert(fs->fs_fs.fs_VolumeBlockSize >= 
		OffsetOf(dl_VolumeIdentifier, ExtVolumeLabel) + 
				FILESYSTEM_MAX_NAME_LEN);
	label = (ExtVolumeLabel *) buf->buf_Data;
	strncpy(label->dl_VolumeIdentifier, name, FILESYSTEM_MAX_NAME_LEN);
	label->dl_VolumeIdentifier[FILESYSTEM_MAX_NAME_LEN-1] = '\0';
	err = WriteBlock(buf, 0);
	if (err < 0)
		return err;
	return 0;
}

/******************************************************************************
  Construct a DirectoryEntry structure for a directory entry.
*/
static Err
MakeDirectoryEntry(AcroFS *fs, DirEntryHdr *deh, BlockNum block, 
		DirectoryEntry *de)
{
	FileBlock *fil;
#define	DIRECTORY_FLAGS	(FILE_IS_DIRECTORY | FILE_SUPPORTS_ENTRY | \
			 FILE_SUPPORTS_DIRSCAN | FILE_USER_STORAGE_PLACE)

	de->de_Type = deh->deh_FileType;
	de->de_Version = deh->deh_Version;
	de->de_Revision = deh->deh_Revision;
	de->de_UniqueIdentifier = (uint32) block;
	de->de_AvatarCount = 1;
	strcpy(de->de_FileName, deh->deh_Name);
	de->de_Location = (uint32) block;
	de->de_Flags = fs->fs_fs.fs_RootDirectory->fi_Flags;
	de->de_BlockSize = fs->fs_fs.fs_VolumeBlockSize;
	memcpy(&de->de_Date, &deh->deh_ModDate, sizeof(de->de_Date));

	switch (deh->deh_Hdr.mh_Type)
	{
	case META_FILE_BLOCK:
		fil = (FileBlock *) deh;
		de->de_BlockCount = fil->fil_NumBlocks;
		de->de_ByteCount = fil->fil_NumBytes;
		de->de_Flags &= ~DIRECTORY_FLAGS;
		break;
	case META_DIR_BLOCK:
		de->de_BlockCount = 1;
		de->de_ByteCount = fs->fs_fs.fs_VolumeBlockSize;
		de->de_Flags |= DIRECTORY_FLAGS;
		break;
	default:
		DBUG_DAMAGE(("DAMAGE: DirEntry(%x) is type %x\n",
			block, deh->deh_Hdr.mh_Type));
		return FILE_ERR_DAMAGED;
	}
	DBUG_DIR(("MakeDirEntry(%x): name %s, flags %x, type %x, ver %x.%x, uniq %x\n",
		block, de->de_FileName, de->de_Flags, de->de_Type, 
		de->de_Version, de->de_Revision, de->de_UniqueIdentifier));
	DBUG_DIR(("        blockSize %x, blockCount %x, byteCount %x\n",
		de->de_BlockSize, de->de_BlockCount, de->de_ByteCount));
	return 0;
}

/******************************************************************************
  Lookup a file in a directory, by name.
*/
Err
DirectoryLookName(Buffer *dirBuf, char *name, DirectoryEntry *de)
{
	BlockNum block;
	BlockNum nextBlock;
	uint32 indx;
	DirEntryHdr *deh;
	Buffer *buf;
	AcroFS *fs = dirBuf->buf_FS;
	DirBlock *dir = (DirBlock *) dirBuf->buf_Data;
	Err err;

	DBUG_DIR(("DirLookName(%x,%s)\n", dirBuf, name));
	indx = HashName(fs, name);
	for (block = dir->dir_HashTable[indx];
	     block != NULL_BLOCKNUM;
	     block = nextBlock)
	{
		err = ReadMetaBlock(fs, block, 
				META_FILE_OR_DIR_BLOCK, 0, &buf);
		if (err < 0)
			return err;
		deh = (DirEntryHdr *) buf->buf_Data;
		if (strcasecmp(deh->deh_Name, name) == 0)
			return MakeDirectoryEntry(fs, deh, block, de);
		nextBlock = deh->deh_NextDirEntry;
	}
	return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
}

/******************************************************************************
  Lookup a file in a directory, by index number.
*/
Err
DirectoryLookNum(Buffer *dirBuf, uint32 num, DirectoryEntry *de)
{
	BlockNum block;
	BlockNum nextBlock;
	uint32 hash;
	DirEntryHdr *deh;
	Buffer *buf;
	AcroFS *fs = dirBuf->buf_FS;
	DirBlock *dir = (DirBlock *) dirBuf->buf_Data;
	Err err;

	/* FIXME: Add a cache to speed this up? */
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
			if (--num == 0)
				return MakeDirectoryEntry(fs, deh, block, de);
			nextBlock = deh->deh_NextDirEntry;
		}
	}
	return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
}
