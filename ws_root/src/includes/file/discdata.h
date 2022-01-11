#ifndef __FILE_DISCDATA_H
#define __FILE_DISCDATA_H


/******************************************************************************
**
**  @(#) discdata.h 96/09/24 1.18
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

/*
  Define the position of the primary label on each Opera disc, the
  block offset between avatars, and the index of the last avatar
  (i.e. the avatar count minus one).  The latter figure *must* match
  the ROOT_HIGHEST_AVATAR figure from "filesystem.h", as the same
  File structure is use to read the label at boot time, and to provide
  access to the root directory.
*/

#define DISC_BLOCK_SIZE           2048
#define DISC_LABEL_OFFSET         18
#define DISC_LABEL_AVATAR_DELTA   32786
#define DISC_LABEL_HIGHEST_AVATAR 7
#define DISC_TOTAL_BLOCKS         330000

#define ROOT_HIGHEST_AVATAR       7
#define FILESYSTEM_MAX_NAME_LEN   32

#ifndef EXTERNAL_RELEASE
#define VOLUME_STRUCTURE_OPERA_READONLY    1
#define VOLUME_STRUCTURE_LINKED_MEM        2
#define VOLUME_STRUCTURE_ACROBAT           4

#define VOLUME_SYNC_BYTE          0x5A
#define VOLUME_SYNC_BYTE_LEN      5
#define VOLUME_COM_LEN      	  32
#define VOLUME_ID_LEN      	  32

#define	TINY_VOLUME_SYNC_BYTE     0xA5
#define	TINY_VOLUME_SYNC_BYTE_LEN 0x1

#define	RECORD_STD_VOLUME         0x01
#define	RECORD_TINY_VOLUME        0xC2

/*
  Data structures written on a 3DO volume.
  NOTE: DiscLabel is the old Opera format.
*/
typedef struct DiscLabel {
  uchar    dl_RecordType;                   /* Shd contain RECORD_STD_VOLUME */
  uint8    dl_VolumeSyncBytes[VOLUME_SYNC_BYTE_LEN]; /* Synchronization byte */
  uchar    dl_VolumeStructureVersion;       /* Should contain 1 */
  uchar    dl_VolumeFlags;                  /* See below */
  uchar    dl_VolumeCommentary[VOLUME_COM_LEN];
					    /* Random comments about volume */
  uchar    dl_VolumeIdentifier[VOLUME_ID_LEN]; /* Should contain disc name */
  uint32   dl_VolumeUniqueIdentifier;       /* Roll a billion-sided die */
  uint32   dl_VolumeBlockSize;              /* Usually contains 2048 */
  uint32   dl_VolumeBlockCount;             /* # of blocks on disc */
  uint32   dl_RootUniqueIdentifier;         /* Roll a billion-sided die */
  uint32   dl_RootDirectoryBlockCount;      /* # of blocks in root */
  uint32   dl_RootDirectoryBlockSize;       /* usually same as vol blk size */
  uint32   dl_RootDirectoryLastAvatarIndex; /* should contain 7 */
  uint32   dl_RootDirectoryAvatarList[ROOT_HIGHEST_AVATAR+1];
} DiscLabel;

/*
 * Extended volume label, for M2 discs.
 */
typedef struct ExtVolumeLabel {
  uchar    dl_RecordType;                   /* Shd contain RECORD_STD_VOLUME */
  uint8    dl_VolumeSyncBytes[VOLUME_SYNC_BYTE_LEN]; /* Synchronization byte */
  uchar    dl_VolumeStructureVersion;       /* Should contain 1 */
  uchar    dl_VolumeFlags;                  /* See below */
  uchar    dl_VolumeCommentary[VOLUME_COM_LEN];
					    /* Random comments about volume */
  uchar    dl_VolumeIdentifier[VOLUME_ID_LEN]; /* Should contain disc name */
  uint32   dl_VolumeUniqueIdentifier;       /* Roll a billion-sided die */
  uint32   dl_VolumeBlockSize;              /* Usually contains 2048 */
  uint32   dl_VolumeBlockCount;             /* # of blocks on disc */
  uint32   dl_RootUniqueIdentifier;         /* Roll a billion-sided die */
  uint32   dl_RootDirectoryBlockCount;      /* # of blocks in root */
  uint32   dl_RootDirectoryBlockSize;       /* usually same as vol blk size */
  uint32   dl_RootDirectoryLastAvatarIndex; /* should contain 7 */
  uint32   dl_RootDirectoryAvatarList[ROOT_HIGHEST_AVATAR+1];
  uint32   dl_NumRomTags;                   /* Number of RomTags in table */
  uint32   dl_ApplicationID;                /* ID of application on media */
  uint32   dl_Reserved[9];                  /* Must be zero */
} ExtVolumeLabel;

/* Bits in dl_VolumeFlags */
#define	VF_M2		0x01		/* This is an M2 volume */
#define	VF_M2ONLY	0x02		/* This volume won't work in Opera */
#define	VF_DATADISC	0x04		/* See below */
#define VF_BLESSED	0x08		/* Search me for system software */

/*
// VF_DATADISC used to be 0x01, and which put it into conflict with the
// new M2 flags.  Fortunately, we never shipped any Opera discs with this
// flag set, so we can move it.  What VF_DATADISC is intended to mean is
// "This disc won't necessarily cause a reboot when inserted."  This flag is
// advisory ONLY. Only by checking with cdromdipir can you be really sure.
// Note: the first volume gets this flag also.
*/

/*
 * Tiny volume label, for very small devices.
 */
typedef struct TinyVolumeLabel {
  uchar    tl_RecordType;		/* Should contain RECORD_TINY_VOLUME */
  uint8    tl_VolumeSyncBytes[TINY_VOLUME_SYNC_BYTE_LEN];
  uint16   tl_VolumeSize;		/* Size of entire volume */
  uint16   tl_DipirID;			/* Which dipir shd validate this vol */
  uint16   tl_DDDID;			/* Which DDD shd read the device */
  uint8    tl_RomTagSubSys;		/* Subsystem type of all TinyRomTags */
  uint8    tl_Reserved[7];		/* Must be zero */
  uint8    tl_TinyRomTags[1];		/* TinyRomTag table */
} TinyVolumeLabel;


typedef struct DirectoryHeader {
  int32      dh_NextBlock;
  int32      dh_PrevBlock;
  uint32     dh_Flags;
  uint32     dh_FirstFreeByte;
  uint32     dh_FirstEntryOffset;
} DirectoryHeader;

#define DIRECTORYRECORD(AVATARCOUNT) \
  uint32     dir_Flags; \
  uint32     dir_UniqueIdentifier; \
  uint32     dir_Type; \
  uint32     dir_BlockSize; \
  uint32     dir_ByteCount; \
  uint32     dir_BlockCount; \
  uint8      dir_Version; \
  uint8      dir_Revision; \
  uint16     dir_rfu; \
  uint32     dir_Gap; /* obsolete field, MBZ forever! */ \
  char       dir_FileName[FILESYSTEM_MAX_NAME_LEN]; \
  uint32     dir_LastAvatarIndex; \
  uint32     dir_AvatarList[AVATARCOUNT];

typedef struct DirectoryRecord {
  DIRECTORYRECORD(1)
} DirectoryRecord;
#endif /* EXTERNAL_RELEASE */

#define DIRECTORY_LAST_IN_DIR        0x80000000
#define DIRECTORY_LAST_IN_BLOCK      0x40000000


/*****************************************************************************/


#endif /* __FILE_DISCDATA_H */
