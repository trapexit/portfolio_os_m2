/*
 * @(#) ops.c 96/09/26 1.10
 *
 * Entrypoints for Acrobat filesystem operations.
 */

#include <file/acromedia.h>
#include "acrofuncs.h"

typedef struct AcroOp
{
	uint32		aop_Cmd;
	Err		(*aop_Func)(AcroFS *fs, FileIOReq *ioreq);
} AcroOp;

const AcroOp *GetAcroOp(uint32 cmd);


/******************************************************************************
  Perform an I/O request.
*/
Err
DoAcroRequest(AcroFS *fs, FileIOReq *ioreq)
{
	const AcroOp *aop;

	aop = GetAcroOp(ioreq->fio.io_Info.ioi_Command);
	if (aop == NULL)
		return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
	return (*aop->aop_Func)(fs, ioreq);
}

/******************************************************************************
  Check permissions.
*/
static Err
acroPerm(FileIOReq *ioreq, File *file, File *child)
{
	uint32 cmd;
	Task *owner;

	cmd = ioreq->fio.io_Info.ioi_Command;
	owner = (Task *) LookupItem(ioreq->fio.io.n_Owner);
	if (owner == 0)
		return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
	if (owner->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
		return 0;

	switch (cmd)
	{
	case CMD_BLOCKWRITE:
	case CMD_BLOCKREAD:
	case FILECMD_SETEOF:
	case FILECMD_SETTYPE:
	case FILECMD_SETBLOCKSIZE:
	case FILECMD_ALLOCBLOCKS:
		if (file->fi_Flags & FILE_IS_DIRECTORY)
			return FILE_ERR_NOTAFILE;
		break;
	case FILECMD_READDIR:
	case FILECMD_READENTRY:
	case FILECMD_ADDENTRY:
	case FILECMD_ADDDIR:
		if (!(file->fi_Flags & FILE_IS_DIRECTORY))
			return FILE_ERR_NOTADIRECTORY;
		break;
	case FILECMD_DELETEENTRY:
	case FILECMD_DELETEDIR:
		if (!(file->fi_Flags & FILE_IS_DIRECTORY))
			return FILE_ERR_NOTADIRECTORY;
		if (cmd == FILECMD_DELETEENTRY &&
		    (child->fi_Flags & FILE_IS_DIRECTORY))
			return FILE_ERR_NOTAFILE;
		if (cmd == FILECMD_DELETEDIR &&
		    !(child->fi_Flags & FILE_IS_DIRECTORY))
			return FILE_ERR_NOTADIRECTORY;
		break;
	case FILECMD_RENAME:
	case FILECMD_FSSTAT:
	case FILECMD_SETVERSION:
	case FILECMD_SETDATE:
		break;
	default:
		return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
	}
	return 0;
}

/******************************************************************************
  Get a FileBlock into a Buffer.
*/
static Err
GetFileBuffer(File *file, Buffer **pBuf)
{
	Buffer *buf;
	AcroFS *fs = (AcroFS *) file->fi_FileSystem;
	Err err;

	/* The buffer is pointed to by FilesystemSpecificData in the File. */
	buf = (Buffer *) file->fi_FilesystemSpecificData;
	if (buf == NULL)
	{
		err = ReadMetaBlock(fs, (BlockNum) file->fi_AvatarList[0],
				META_FILE_OR_DIR_BLOCK, 0, &buf);
		if (err < 0)
			return err;
		file->fi_FilesystemSpecificData = (uint32) buf;
	}
	*pBuf = buf;
	return 0;
}

/******************************************************************************
*/
static Err
acroNoOp(AcroFS *fs, FileIOReq *ioreq)
{
        TOUCH(fs);
	TOUCH(ioreq);
	DBUG_ANY(("acroNoOp cmd %x\n", ioreq->fio.io_Info.ioi_Command));
        return MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
}

/******************************************************************************
*/
static Err
acroFsStat(AcroFS *fs, FileIOReq *ioreq)
{
	FileSystemStat *fsstat;
	File *file;
	Err err;

	TOUCH(fs);
	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	fsstat = (FileSystemStat *) ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
	if (fsstat == NULL)
		return FILE_ERR_BADPTR;
	if (ioreq->fio.io_Info.ioi_Recv.iob_Len < sizeof(FileSystemStat))
		return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
        fsstat->fst_Free = fs->fs_NumFreeBlocks;
        fsstat->fst_Used = fsstat->fst_Size - fsstat->fst_Free;
        fsstat->fst_MaxFileSize = fsstat->fst_Free;
	memcpy(&fsstat->fst_CreateTime, &fs->fs_Super->sb_CreateDate,
		sizeof(fsstat->fst_CreateTime));
        fsstat->fst_BitMap |= 
		(FSSTAT_MAXFILESIZE | FSSTAT_FREE | FSSTAT_USED | 
		 FSSTAT_CREATETIME);
	return 0;
}

/******************************************************************************
*/
static Err
acroReadWriteFile(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	Buffer *fileBuf;
	FileBlock *fil;
	void *data;
	uint32 numBytes;
	uint32 block;
	bool writing;
	Err err;

	switch (ioreq->fio.io_Info.ioi_Command)
	{
	case CMD_BLOCKREAD:
		data = ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
		numBytes = ioreq->fio.io_Info.ioi_Recv.iob_Len;
		writing = FALSE;
		break;
	case CMD_BLOCKWRITE:
		data = ioreq->fio.io_Info.ioi_Send.iob_Buffer;
		numBytes = ioreq->fio.io_Info.ioi_Send.iob_Len;
		writing = TRUE;
		break;
	default:
		return MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
	}
	block = ioreq->fio.io_Info.ioi_Offset;
	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;

	DBUG_FILE(("acro %s: %x,%x offset %x\n", writing ? "Write" : "Read", 
		data, numBytes, block));
	if (data == NULL)
	{
		DBUG_ANY(("acroRW: sendBuffer NULL\n"));
		return FILE_ERR_BADPTR;
	}
	if (block >= file->fi_BlockCount || 
	    numBytes > file->fi_BlockSize * (file->fi_BlockCount - block))
	{
		DBUG_ANY(("acroRW: bad range: blk %x, blkCount %x, numBytes %x\n",
			block, file->fi_BlockCount, numBytes));
		return FILE_ERR_BADPTR;
	}

	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	err = ReadWriteBlocks(fileBuf, block, data, numBytes, writing);
	if (err < 0)
	{
		DBUG_ANY(("RWBlocks ret %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	fil = (FileBlock *) fileBuf->buf_Data;
	if (ioreq->fio.io_Info.ioi_Command == CMD_BLOCKWRITE)
	{
		SampleSystemTimeTV(&fil->fil_de.deh_ModDate);
		DirtyBuffer(fileBuf);
	}
	file->fi_ByteCount = fil->fil_NumBytes;
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroReadEntry(AcroFS *fs, FileIOReq *ioreq)
{
	File *dirfile;
	Buffer *dirBuf;
	char *filename;
	DirectoryEntry *de;
	Err err;

	dirfile = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	filename = (char *) ioreq->fio.io_Info.ioi_Send.iob_Buffer;
	DBUG_DIR(("acroReadEntry: dirFile %x, name %s\n", dirfile, filename));
	de = (DirectoryEntry *) ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
	if (de == NULL)
		return FILE_ERR_BADPTR;
	err = acroPerm(ioreq, dirfile, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(dirfile, &dirBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	err = DirectoryLookName(dirBuf, filename, de);
	if (err < 0)
	{
		DBUG_DIR(("DirLookName err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroReadDir(AcroFS *fs, FileIOReq *ioreq)
{
	File *dirfile;
	Buffer *dirBuf;
	uint32 num;
	DirectoryEntry *de;
	Err err;

	dirfile = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	num = ioreq->fio.io_Info.ioi_Offset;
	de = (DirectoryEntry *) ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
	DBUG_DIR(("acroReadDir: dirfile %x, num %x\n", dirfile, num));
	if (de == NULL)
		return FILE_ERR_BADPTR;
	err = acroPerm(ioreq, dirfile, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(dirfile, &dirBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	err = DirectoryLookNum(dirBuf, num, de);
	if (err < 0)
	{
		DBUG_DIR(("DirLookNum err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroAllocBlocks(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	Buffer *fileBuf;
	FileBlock *fil;
	int32 newBlocks;
	Err err;

	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	DBUG_FILE(("acroAllocBlocks %x\n", ioreq->fio.io_Info.ioi_Offset));
	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	newBlocks = file->fi_BlockCount + ioreq->fio.io_Info.ioi_Offset;
	if (newBlocks < 0 ||
	    newBlocks * fs->fs_fs.fs_VolumeBlockSize < file->fi_ByteCount)
	{
		AbortTransaction(fs);
      		return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
	}
	err = AddFileBlocks(fileBuf, ioreq->fio.io_Info.ioi_Offset);
	if (err < 0)
	{
		DBUG_FILE(("AddFileBlocks err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	fil = (FileBlock *) fileBuf->buf_Data;
	SampleSystemTimeTV(&fil->fil_de.deh_ModDate);
	DirtyBuffer(fileBuf);
	file->fi_BlockCount = fil->fil_NumBlocks;
	file->fi_ByteCount = fil->fil_NumBytes;
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroSetEOF(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	Buffer *fileBuf;
	FileBlock *fil;
	uint32 newSize;
	Err err;

	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	newSize = ioreq->fio.io_Info.ioi_Offset;
	DBUG_FILE(("acroSetEOF %x\n", newSize));
	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	if (newSize > file->fi_BlockSize * file->fi_BlockCount)
	{
		AbortTransaction(fs);
		return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
	}
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	fil = (FileBlock *) fileBuf->buf_Data;
	fil->fil_NumBytes = newSize;
	SampleSystemTimeTV(&fil->fil_de.deh_ModDate);
	DirtyBuffer(fileBuf);
	file->fi_ByteCount = fil->fil_NumBytes;
	return EndTransaction(fs);
}

/****************************************************************************** */
static Err
acroAddEntry(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	char *filename;
	Buffer *parBuf;
	Err err;

	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	DBUG_FILE(("acroAddEntry\n"));
	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(file, &parBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	filename = (char *) ioreq->fio.io_Info.ioi_Send.iob_Buffer;

	switch (ioreq->fio.io_Info.ioi_Command)
	{
	case FILECMD_ADDENTRY:
		err = AddFileOrDir(filename, parBuf, 0);
		break;
	case FILECMD_ADDDIR:
		err = AddFileOrDir(filename, parBuf, FILE_TYPE_DIRECTORY);
		break;
	default:
		return MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
	}
	if (err < 0)
	{
		DBUG_FILE(("AddFile/Dir err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroDelEntry(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	Buffer *fileBuf;
	Err err;

	file = (File *) ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
	DBUG_FILE(("acroDelEntry\n"));
	if (file == fs->fs_fs.fs_RootDirectory)
		return FILE_ERR_PARAMERROR;
	err = acroPerm(ioreq, file->fi_ParentDirectory, file);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}

	switch (ioreq->fio.io_Info.ioi_Command)
	{
	case FILECMD_DELETEENTRY:
		err = DeleteFile(fileBuf);
		break;
	case FILECMD_DELETEDIR:
		err = DeleteDirectory(fileBuf);
		break;
	default:
		err = MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
	}
	if (err < 0)
	{
		DBUG_FILE(("DelFile/Dir err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	return EndTransaction(fs);
}

/****************************************************************************** */
static Err
acroSetAttr(AcroFS *fs, FileIOReq *ioreq)
{
	File *file;
	Buffer *fileBuf;
	FileBlock *fil;
	Err err;

	file = ((OFile *) ioreq->fio.io_Dev)->ofi_File;
	err = acroPerm(ioreq, file, NULL);
	if (err < 0)
		return err;
	BeginTransaction(fs);
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	fil = (FileBlock *) fileBuf->buf_Data;
	SampleSystemTimeTV(&fil->fil_de.deh_ModDate);

	switch (ioreq->fio.io_Info.ioi_Command)
	{
	case FILECMD_SETTYPE:
	    {
		uint32 newType = ioreq->fio.io_Info.ioi_Offset;
		fil->fil_de.deh_FileType = newType;
		file->fi_Type = fil->fil_de.deh_FileType;
		break;
	    }
	case FILECMD_SETVERSION:
	    {
		uint32 newVersion = ioreq->fio.io_Info.ioi_Offset;
		fil->fil_de.deh_Version = file->fi_Version = 
			(newVersion >> 8) & 0xFF;;
		fil->fil_de.deh_Revision = file->fi_Revision = 
			newVersion & 0xFF;;
		break;
	    }
	case FILECMD_SETDATE:
	    {
		memcpy(&fil->fil_de.deh_ModDate, 
			ioreq->fio.io_Info.ioi_Send.iob_Buffer, 
			sizeof(fil->fil_de.deh_ModDate));
		break;
	    }
	default:
		return FILE_ERR_SOFTERR;
	}
	DirtyBuffer(fileBuf);
	return EndTransaction(fs);
}

/******************************************************************************
*/
static Err
acroRename(AcroFS *fs, FileIOReq *ioreq)
{
	char *filename;
	File *file;
	Buffer *fileBuf;
	FileBlock *fil;
	Buffer *parBuf;
	Err err;

	filename = (char *) ioreq->fio.io_Info.ioi_Send.iob_Buffer;
	file = (File *) ioreq->fio.io_Info.ioi_Recv.iob_Buffer;
	DBUG_FILE(("acroRename %s\n", filename));
	err = acroPerm(ioreq, file->fi_ParentDirectory, file);
	if (err < 0)
		return err;

	BeginTransaction(fs);
	err = GetFileBuffer(file, &fileBuf);
	if (err < 0)
	{
		AbortTransaction(fs);
		return err;
	}
	fil = (FileBlock *) fileBuf->buf_Data;
	if (fil->fil_de.deh_Parent == NULL_BLOCKNUM)
	{
		err = RenameRoot(fileBuf, filename);
	} else
	{
		err = GetFileBuffer(file->fi_ParentDirectory, &parBuf);
		if (err < 0)
		{
			AbortTransaction(fs);
			return err;
		}
		err = RenameEntry(parBuf, fileBuf, filename);
	}
	if (err < 0)
	{
		DBUG_FILE(("Rename err %x\n", err));
		AbortTransaction(fs);
		return err;
	}
	SampleSystemTimeTV(&fil->fil_de.deh_ModDate);
	strcpy(file->fi_FileName, fil->fil_de.deh_Name);
	return EndTransaction(fs);
}

/******************************************************************************
*/
Err
InitRootDir(AcroFS *fs)
{
	Buffer *buf;
	DirBlock *dir;
	File *root = fs->fs_fs.fs_RootDirectory;
	Err err;

	DBUG_MOUNT(("InitRootDir\n"));
	err = ReadMetaBlock(fs, fs->fs_Super->sb_RootDirBlockNum,
			META_DIR_BLOCK, 0, &buf);
	if (err < 0)
		return err;
	dir = (DirBlock *) buf->buf_Data;
	root->fi_UniqueIdentifier = (uint32) buf->buf_BlockNum;
	root->fi_BlockSize = fs->fs_fs.fs_VolumeBlockSize;
	root->fi_BlockCount = 1;
	root->fi_ByteCount = root->fi_BlockSize;
	root->fi_Type = dir->dir_de.deh_FileType;
	memcpy(&root->fi_Date, &dir->dir_de.deh_ModDate, sizeof(root->fi_Date));
	root->fi_Flags |= FILE_SUPPORTS_DIRSCAN | 
			FILE_SUPPORTS_ENTRY |
			FILE_HAS_VALID_VERSION |
			FILE_USER_STORAGE_PLACE;
	if (fs->fs_fs.fs_Flags & FILESYSTEM_IS_READONLY)
		root->fi_Flags |= FILE_IS_READONLY;
	root->fi_AvatarList[0] = buf->buf_BlockNum;
	root->fi_LastAvatarIndex = 0;
	return 0;
}

/******************************************************************************
*/
const AcroOp acroOps[] =
{
	{ CMD_STATUS,		acroNoOp		},
	{ CMD_BLOCKWRITE,	acroReadWriteFile	},
	{ CMD_BLOCKREAD,	acroReadWriteFile	},
	{ FILECMD_READDIR,	acroReadDir		},
	{ FILECMD_GETPATH,	acroNoOp		},
	{ FILECMD_READENTRY,	acroReadEntry		},
	{ FILECMD_ALLOCBLOCKS,	acroAllocBlocks		},
	{ FILECMD_SETEOF,	acroSetEOF		},
	{ FILECMD_ADDENTRY,	acroAddEntry		},
	{ FILECMD_DELETEENTRY,	acroDelEntry		},
	{ FILECMD_SETTYPE,	acroSetAttr		},
	{ FILECMD_OPENENTRY,	acroNoOp		},
	{ FILECMD_FSSTAT,	acroFsStat		},
	{ FILECMD_ADDDIR,	acroAddEntry		},
	{ FILECMD_DELETEDIR,	acroDelEntry		},
	{ FILECMD_SETVERSION,	acroSetAttr		},
	{ FILECMD_SETBLOCKSIZE,	acroNoOp		},
	{ FILECMD_RENAME,	acroRename		},
	{ FILECMD_SETDATE,	acroSetAttr		},
	{ 0, NULL }
};

/******************************************************************************
*/
static const AcroOp *
GetAcroOp(uint32 cmd)
{
	const AcroOp *aop;

	for (aop = acroOps;  aop->aop_Func != NULL;  aop++)
		if (aop->aop_Cmd == cmd)
			return aop;
	return NULL;
}
