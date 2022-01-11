/*
 * @(#) acro.h 96/09/26 1.2
 */

#ifndef __FILE_ACRO_H
#define __FILE_ACRO_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif
#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif
#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif
#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif
#ifndef __FILE_FILESYSTEMDEFS_H
#include <file/filesystemdefs.h>
#endif
#ifndef __FILE_DIRECTORY_H
#include <file/directory.h>
#endif
#ifndef __STDIO_H
#include <stdio.h>
#endif
#ifndef __ASSERT_H
#include <assert.h>
#endif

#define	MAX_WRITE_RETRIES	4	/* Num. times to retry a write */

#define	CRITICAL_BLOCK_RESERVE	20	/* Blocks reserved for "critical" ops */

/* A reference to a block number on the media */
typedef uint32 BlockNum;
typedef uint32 PhysBlockNum;
#define	NULL_BLOCKNUM	((BlockNum)~0)

#define	RMASK(bits)	((1 << (bits)) - 1)

#define	OffsetOf(mem,str)	((uint32)&(((str*)0)->mem))

#define	MEDIA_ERROR(err) \
	((((err) >> ERR_CLASHIFT) & RMASK(ERR_CLASSIZE)) == ER_C_STND && \
	 (((err) >> ERR_ERRSHIFT) & RMASK(ERR_ERRSIZE)) == ER_MediaError)

typedef struct Buffer
{
	MinNode		buf_Node;
	BlockNum	buf_BlockNum;
	struct AcroFS *	buf_FS;
	uint32		buf_Flags;
	uint32		buf_Data[1];
} Buffer;

#define	SIZEOF_BUFFER(bs)	(sizeof(Buffer) - sizeof(uint32) + (bs))

/* Bits in buf_Flags */
#define	BUF_DIRTY	0x00000001
#define	BUF_RESIDENT	0x00000002
#define	BUF_FREED	0x80000000

typedef struct AcroBorrow
{
	BlockNum	bor_FreeListBlock;
	uint32		bor_FreeListIndx;
} AcroBorrow;

typedef struct AcroFS
{
	FileSystem	fs_fs;
#ifdef BUILD_STRINGS
	uint32		fs_Marker;		/* Makes it easier to find end of fs_fs in hex dumps */
#endif
	uint16		fs_Flags;
	uint8		fs_MountState;
	uint8		fs_Reserved1;

	uint32		fs_StartSig;
	IOReq *		fs_DeviceIOReq;
	Buffer *	fs_FreeListBuf;
	uint32		fs_NumFreeBlocks;
	uint32		fs_SaveNumFreeBlocks;
	uint32		fs_MinSuperBlock;
	uint32		fs_MaxSuperBlock;
	List		fs_MapBlockList;
	List		fs_Buffers;
	List		fs_PermBorrows;
	List		fs_FreedThisTransaction;
	Buffer *	fs_SuperBuf;
	struct SuperBlock *fs_Super;
	AcroBorrow *	fs_Borrowing;
	AcroBorrow	fs_BorrowCtx;
} AcroFS;

/* Values in fs_MountState */
#define	MS_INACTIVE	0		/* Not mounted */
#define	MS_INTRANS	1		/* In transition; mounting */
#define	MS_ACTIVE	2		/* Fully mounted */
#define	MS_FAILED	3		/* Failed to mount */

/* Bits in fs_Flags */
#define	ACRO_REWRITE_SUPERBLOCKS 0x00000001
#define	ACRO_REWRITE_MAPBLOCKS	0x00000002

typedef struct PermBorrow
{
	MinNode		pb_Node;
	BlockNum	pb_BorrowedBlock;
	BlockNum	pb_FreeListBlock;
	uint32		pb_FreeListIndx;
} PermBorrow;

/* Bits in flags argument to GetBlock() */
#define	BLK_READ	0x00000001
#define	BLK_PHYS	0x00000002
#define	BLK_REMAP	0x00000004
#define	BLK_META	0x00000008

/* Bits in flags argument to AllocAcroBlock() */
#define	ALLOC_CRITICAL	0x00000001

/* Return value from ReadMirroredBlock() */
#define	MIRROR_1_VALID		1
#define	MIRROR_2_VALID		2
#define	MIRROR_ALL_VALID	3

#define	OverwriteBlock(fs,block,pBuf)	GetBlock(fs,block,0,pBuf)

#endif /* __FILE_ACRO_H */

