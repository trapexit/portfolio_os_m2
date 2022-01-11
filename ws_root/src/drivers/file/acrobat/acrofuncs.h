/*
 * @(#) acrofuncs.h 96/09/24 1.3
 */

#ifndef __FILE_ACRO_H
#include <file/acro.h>
#endif
#ifndef __STRING_H
#include <string.h>
#endif
#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif
#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif
#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif

#define ROUNDUP(v,sz)	(((v)+(sz)-1) & ~((sz)-1))

#ifdef BUILD_STRINGS
#define DEBUG 
#endif

#ifdef DEBUG
extern uint32 acroDebug;
#define	DB_ALLOC	0x00000001
#define	DB_BUF		0x00000002
#define	DB_DIR		0x00000004
#define	DB_FILE		0x00000008
#define	DB_IO		0x00000010
#define	DB_LOG		0x00000020
#define	DB_MAP		0x00000040
#define	DB_MOUNT	0x00000080
#define	DB_DAMAGE	0x00000100
#define	DB_FREELIST	0x00000200

#define	DBUG_ANY(x)	if (acroDebug)			{ printf x ; } else ;
#define	DBUG_ALLOC(x)	if (acroDebug & DB_ALLOC)	{ printf x ; } else ;
#define	DBUG_BUF(x)	if (acroDebug & DB_BUF)		{ printf x ; } else ;
#define	DBUG_DIR(x)	if (acroDebug & DB_DIR)		{ printf x ; } else ;
#define	DBUG_FILE(x)	if (acroDebug & DB_FILE)	{ printf x ; } else ;
#define	DBUG_IO(x)	if (acroDebug & DB_IO)		{ printf x ; } else ;
#define	DBUG_LOG(x)	if (acroDebug & DB_LOG)		{ printf x ; } else ;
#define	DBUG_MAP(x)	if (acroDebug & DB_MAP)		{ printf x ; } else ;
#define	DBUG_MOUNT(x)	if (acroDebug & DB_MOUNT)	{ printf x ; } else ;
#define	DBUG_DAMAGE(x)	if (acroDebug & DB_DAMAGE)	{ printf x ; } else ;

#else /* not DEBUG */

#define	DBUG_ANY(x)
#define	DBUG_ALLOC(x)
#define	DBUG_BUF(x)
#define	DBUG_DIR(x)
#define	DBUG_FILE(x)
#define	DBUG_IO(x)
#define	DBUG_LOG(x)
#define	DBUG_MAP(x)
#define	DBUG_MOUNT(x)
#define	DBUG_DAMAGE(x)
#endif
#define	CheckFreeBlockCount(fs,msg)

#ifndef CheckFreeBlockCount
extern void CheckFreeBlockCount(AcroFS *fs, char *msg);
#endif

extern Err ReadDevice(IOReq *ioreq, BlockNum block, 
		void *data, uint32 dataSize);

extern Err WriteDevice(IOReq *ioreq, BlockNum block, 
		void *data, uint32 dataSize);

extern uint32 NumDirtyBuffers(AcroFS *fs);

extern Err GetBlock(AcroFS *fs, BlockNum block, uint32 flags, Buffer **pBuf);

extern Err ReadMetaBlock(AcroFS *fs, BlockNum block, MetaBlockType type, 
		uint32 flags, Buffer **pBuf);

extern Err WriteBlock(Buffer *buf, uint32 flags);

extern void DirtyBuffer(Buffer *buf);

extern void UndirtyBuffer(Buffer *buf);

extern void RetargetBuffer(Buffer *buf, BlockNum block);

extern void FreeBuffer(Buffer *buf);

extern void UnlinkBuffer(Buffer *buf);

extern void DeleteBuffer(Buffer *buf);

extern Err DeleteBuffers(AcroFS *fs, bool forceUndirty);

extern Err FlushBuffers(AcroFS *fs, MetaBlockType type, uint32 flags);

extern Err InitBuffers(AcroFS *fs);

extern void ClearFreedThisTransaction(AcroFS *fs);

extern Err AllocAcroBlock(AcroFS *fs, uint32 flags, Buffer **pBuf);

extern Err FreeAcroBlock(AcroFS *fs, BlockNum freeBlock);

extern Err BorrowAcroBlock(AcroFS *fs, uint32 flags, PermBorrow *pb, Buffer **pBuf);

extern void BeginBorrowing(AcroFS *fs);

extern void EndBorrowing(AcroFS *fs);

extern Err CommitPermBorrows(AcroFS *fs);

extern void DeletePermBorrows(AcroFS *fs);

extern Err WriteFreeList(AcroFS *fs);

extern Err InitFreeList(AcroFS *fs);

extern Err RebuildFreeList(AcroFS *fs);

extern PhysBlockNum PhysicalBlock(AcroFS *fs, BlockNum block);

extern Err WriteMapBlock(AcroFS *fs, Buffer *mapBuf);

extern Err RemapBlock(AcroFS *fs, BlockNum block);

extern Err InitMapper(AcroFS *fs);

extern Err WriteMirroredBlock(AcroFS *fs, BlockNum block1, BlockNum block2,
		uint32 flags, void *data);

extern Err ReadMirroredBlock(AcroFS *fs, BlockNum block1, BlockNum block2,
		MetaBlockType type, uint32 flags, Buffer **pBuf);

extern Err InitFS(AcroFS *fs);

extern Err AddFileOrDir(char *name, Buffer *parBuf, uint32 fileType);

extern Err DeleteFile(Buffer *fileBuf);

extern Err AddDirectory(char *name, Buffer *parBuf);

extern Err DeleteDirectory(Buffer *fileBuf);

extern Err RenameEntry(Buffer *parBuf, Buffer *fileBuf, char *filename);

extern Err RenameRoot(Buffer *fileBuf, char *filename);

extern Err DirectoryLookNum(Buffer *dirBuf, uint32 num, DirectoryEntry *de);

extern Err DirectoryLookName(Buffer *dirBuf, char *name, DirectoryEntry *de);

extern Err InitRootDir(AcroFS *fs);

extern Err AddFileBlocks(Buffer *fileBuf, int32 numBlocks);

extern Err ReadWriteBlocks(Buffer *fileBuf, BlockNum fblock, 
		void *data, uint32 numBytes, bool writing);

extern Err WriteSuperBlock(AcroFS *fs);

extern Err SyncSuperBlock(AcroFS *fs);

extern Err InitSuperBlock(AcroFS *fs);

extern void BeginTransaction(AcroFS *fs);

extern void AbortTransaction(AcroFS *fs);

extern Err EndTransaction(AcroFS *fs);

extern Err ProcessLog(AcroFS *fs);

extern uint32 acroChecksum(void *data, uint32 size);

extern void InitMetaHdr(MetaHdr *hdr, MetaBlockType type);

extern bool ValidMetaBlock(Buffer *buf, MetaBlockType type);

extern Err DoAcroRequest(AcroFS *fs, FileIOReq *ioreq);

extern Err acroFormat(Item ioreqItem, const DeviceStatus *stat, 
		const char *name, const TagArg *tags);

extern Err ClearLog(AcroFS *fs);

extern Err CreateLog(AcroFS *fs);

extern Err ReadLog(AcroFS *fs);
