/*
 * @(#) acromedia.h 96/09/26 1.3
 */

#ifndef __FILE_ACROMEDIA_H
#define __FILE_ACROMEDIA_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif
#ifndef __KERNEL_TYPES_H
#include <kernel/time.h>
#endif
#ifndef __FILE_ACRO_H
#include <file/acro.h>
#endif
#ifndef __FILE_DISCDATA_H
#include <file/discdata.h>
#endif

/* Types of meta blocks supported */
typedef uint8 MetaBlockType;
#define	META_SUPER_BLOCK	'S'	/* SuperBlock */
#define	META_MAP_BLOCK		'M'	/* MapBlock */
#define	META_FREELIST_BLOCK	'X'	/* FreeListBlock */
#define	META_DIR_BLOCK		'D'	/* DirBlock */
#define	META_FILE_BLOCK		'F'	/* FileBlock */
#define	META_BLOCKTABLE_BLOCK	'B'	/* BlockTableBlock */
#define	META_LOGINFO_BLOCK	'L'	/* BlockTableBlock */

#define	META_FILE_OR_DIR_BLOCK	100	/* FileBlock or DirBlock; never saved */

#define	MAX_ARRAY_ENTRIES(fs,str,atype) \
	MAX_ARRAY_ENTRIES_BS((fs)->fs_fs.fs_VolumeBlockSize, str, atype)

#define	MAX_ARRAY_ENTRIES_BS(bs,str,atype) \
	(((bs) - sizeof(str) + sizeof(atype)) / sizeof(atype))

#define	MIN_ACRO_BLOCK_SIZE	128	/* Any metablock must fit in 1 block */


#define	NUM_SUPER_BLOCKS	(6 *2)	/* Number of pairs of superblocks */

/******************************************************************************
*/
#define	dl_MinSuperBlock	dl_Reserved[7]
#define	dl_MaxSuperBlock	dl_Reserved[8]

/******************************************************************************
 This appears at the head of every meta block in the file system.
*/
typedef struct MetaHdr
{
	uint16		mh_Magic;
	MetaBlockType	mh_Type;
	uint8		mh_Flags;
	uint32		mh_Checksum;
} MetaHdr;

#define	MH_MAGIC	0xCD47

/******************************************************************************
  The superblock is mirrored in adjacent blocks near the beginning
  of the media.  
*/
typedef struct SuperBlock
{
	MetaHdr		sb_Hdr;
	uint32		sb_Flags;
	PhysBlockNum	sb_MapBlockNum;
	PhysBlockNum	sb_MapBlockMirror;
	BlockNum	sb_LogInfoBlockNum;
	BlockNum	sb_LogInfoBlockMirror;
	BlockNum	sb_FreeListBlockNum;
	BlockNum	sb_RootDirBlockNum;
	TimeVal		sb_CreateDate;
} SuperBlock;

/* Bits in sb_Flags */

/******************************************************************************
 This appears at the head of a FileBlock or a DirBlock.
 */
typedef struct DirEntryHdr
{
	MetaHdr		deh_Hdr;
	BlockNum	deh_NextDirEntry;
	char		deh_Name[FILESYSTEM_MAX_NAME_LEN];
	BlockNum	deh_Parent;
	uint32		deh_FileType;
	TimeVal		deh_ModDate;
	uint8		deh_Version;
	uint8		deh_Revision;
	uint16		deh_Reserved;
} DirEntryHdr;

/******************************************************************************
 This is a directory block describing a hierarchy in the file system. To
 find a particular file, the name is hashed and indexed into the hash table.
*/
typedef struct DirBlock
{
	DirEntryHdr	dir_de;
	BlockNum	dir_HashTable[1]; /* To end of block */
} DirBlock;

#define	MAX_DIRBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, DirBlock, BlockNum)
#define	MAX_DIRBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, DirBlock, BlockNum)

/******************************************************************************
 This is a file block describing a particular file in the file system.
*/
typedef struct FileBlock
{
	DirEntryHdr	fil_de;
	BlockNum	fil_BlockTable;
	uint32		fil_NumBlocks;
	uint32		fil_NumBytes;
	BlockNum	fil_Blocks[1]; /* To end of block */
} FileBlock;

#define	MAX_FILEBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, FileBlock, BlockNum)
#define	MAX_FILEBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, FileBlock, BlockNum)

/******************************************************************************
*/
typedef struct BlockTableBlock
{
	MetaHdr		bt_Hdr;
	BlockNum	bt_NextBlockTable;
	BlockNum	bt_Blocks[1]; /* To end of block */
} BlockTableBlock;

#define	MAX_BLOCKTABLEBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, BlockTableBlock, BlockNum)
#define	MAX_BLOCKTABLEBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, BlockTableBlock, BlockNum)

/******************************************************************************
*/
typedef struct FreeListBlock
{
	MetaHdr		fl_Hdr;
	BlockNum	fl_NextFreeListBlock;
	BlockNum	fl_FreeBlocks[1]; /* To end of block */
} FreeListBlock;

#define	MAX_FREELISTBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, FreeListBlock, BlockNum)
#define	MAX_FREELISTBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, FreeListBlock, BlockNum)

/******************************************************************************
*/
typedef struct MapEntry
{
	BlockNum	me_VirtBlock;
	PhysBlockNum	me_PhysBlock;
} MapEntry;

/******************************************************************************
*/
typedef struct MapBlock
{
	MetaHdr		mb_Hdr;
	PhysBlockNum	mb_NextMapBlock;
	MapEntry	mb_Map[1]; /* To end of block */
} MapBlock;

#define	MAX_MAPBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, MapBlock, MapEntry)
#define	MAX_MAPBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, MapBlock, MapEntry)

/******************************************************************************
*/
typedef struct LogEntry
{
	BlockNum	le_DataBlock;
	BlockNum	le_TargetBlock;
} LogEntry;

typedef struct LogInfoBlock
{
	MetaHdr		li_Hdr;
	BlockNum	li_NextLogInfoBlock; /* ? */
	LogEntry	li_Log[1]; /* To end of block */
} LogInfoBlock;

/* Bits in li_Hdr.mh_Flags */
#define	LI_VALID	0x01		/* LogInfo block has valid data */

#define	MAX_LOGINFOBLOCK_ENTRIES(fs) \
	MAX_ARRAY_ENTRIES(fs, LogInfoBlock, LogEntry)
#define	MAX_LOGINFOBLOCK_ENTRIES_BS(bs) \
	MAX_ARRAY_ENTRIES_BS(bs, LogInfoBlock, LogEntry)


#endif /* __FILE_ACROMEDIA_H */

