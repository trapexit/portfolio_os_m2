#ifndef __FILE_FILESYSTEM_H
#define __FILE_FILESYSTEM_H


/******************************************************************************
**
**  @(#) filesystem.h 96/09/24 1.64
**
**  Constants and data structures for filesystem access.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif

#ifndef __KERNEL_DRIVER_H
#include <kernel/driver.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef __KERNEL_TAGS_H
#include <kernel/tags.h>
#endif

#ifndef EXTERNAL_RELEASE
#ifndef __FILE_DISCDATA_H
#include <file/discdata.h>
#endif

#ifndef __KERNEL_FOLIO_H
#include <kernel/folio.h>
#endif

#ifndef __KERNEL_SEMAPHORE_H
#include <kernel/semaphore.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif
#endif

/*****************************************************************************/


/* item types of file folio items */
#define FILESYSTEMNODE  1
#define FILENODE        2
#define FILEALIASNODE   3

#ifndef EXTERNAL_RELEASE
#define FILEFOLIO                      NST_FILESYS
#define FILESYSTEM_DEFAULT_BLOCKSIZE   2048

#define FILESYSTEM_TAG_END             TAG_END
#define FILESYSTEM_TAG_PRI             TAG_ITEM_LAST+1

#define FILETASK_TAG_CURRENTDIRECTORY  TAG_ITEM_LAST+1

#define DEVICE_SORT_COUNT  8

#ifndef	HOWMANY
#define	HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#endif	/* HOWMANY */

#define ALIAS_NAME_MAX_LEN     31
#define ALIAS_VALUE_MAX_LEN    255

#endif /* EXTERNAL_RELEASE

/*****************************************************************************/


#define FILESYSTEM_MAX_PATH_LEN	256   /* max # of chars in a path string    */
#define FILESYSTEM_MAX_NAME_LEN	32    /* max # of chars in a path component */

#define FILE_HIGHEST_AVATAR     255   /* don't ask.... */


/*****************************************************************************/


/* file folio error codes */
#define ER_Fs_NoFile            1
#define ER_Fs_NotADirectory     2
#define ER_Fs_NoFileSystem      3
#define ER_Fs_BadName           4
#define ER_Fs_MediaError        5
#define ER_Fs_DeviceError       6
#define ER_Fs_ParamError        7
#define ER_Fs_NoSpace           8
#define ER_Fs_Damaged           9
#define ER_Fs_Busy             10
#define ER_FS_DuplicateFile    11
#define ER_FS_ReadOnly         12
#define ER_Fs_DuplicateLink    13
#define ER_Fs_CircularLink     14
#define ER_Fs_NotAFile         15
#define ER_Fs_DirNotEmpty      16
#define ER_Fs_InvalidMeta      17
#define ER_Fs_NoLog	       18
#define ER_Fs_InvalidState     19
#define ER_Fs_LogOverflow      20
#define ER_Fs_BadMode          21
#define ER_Fs_BadCount         22
#define ER_Fs_BadSize          23
#define ER_Fs_BadFile          24
#define ER_Fs_FileNotEmpty     25

#define FILE_ERR_NOMEM          MakeFErr(ER_SEVERE,ER_C_STND,ER_NoMem)
#define FILE_ERR_BADTAG         MakeFErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define FILE_ERR_BADPTR         MakeFErr(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define FILE_ERR_BADITEM        MakeFErr(ER_SEVERE,ER_C_STND,ER_BadItem)
#define FILE_ERR_NOSUPPORT      MakeFErr(ER_SEVERE,ER_C_STND,ER_NotSupported)
#define FILE_ERR_OFFLINE        MakeFErr(ER_SEVERE,ER_C_STND,ER_DeviceOffline)
#define FILE_ERR_SOFTERR        MakeFErr(ER_SEVERE,ER_C_STND,ER_SoftErr)
#define FILE_ERR_NOFILE         MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NoFile)
#define FILE_ERR_NOTADIRECTORY  MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NotADirectory)
#define FILE_ERR_NOFILESYSTEM   MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NoFileSystem)
#define FILE_ERR_BADNAME        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_BadName)
#define FILE_ERR_MEDIAERROR     MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_MediaError)
#define FILE_ERR_DEVICEERROR    MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_DeviceError)
#define FILE_ERR_PARAMERROR     MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_ParamError)
#define FILE_ERR_NOSPACE        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NoSpace)
#define FILE_ERR_DAMAGED        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_Damaged)
#define FILE_ERR_BUSY           MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_Busy)
#define FILE_ERR_DUPLICATEFILE  MakeFErr(ER_SEVERE,ER_C_NSTND,ER_FS_DuplicateFile)
#define FILE_ERR_READONLY       MakeFErr(ER_SEVERE,ER_C_NSTND,ER_FS_ReadOnly)
#define FILE_ERR_DUPLICATELINK  MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_DuplicateLink)
#define FILE_ERR_CIRCULARLINK   MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_CircularLink)
#define FILE_ERR_NOTAFILE       MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NotAFile)
#define FILE_ERR_DIRNOTEMPTY    MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_DirNotEmpty)
#define FILE_ERR_INVALIDMETA    MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_InvalidMeta)
#define FILE_ERR_NOLOG          MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_NoLog)
#define FILE_ERR_INVALIDSTATE   MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_InvalidState)
#define FILE_ERR_LOGOVERFLOW    MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_LogOverflow)
#define FILE_ERR_BADMODE        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_BadMode)
#define FILE_ERR_BADCOUNT       MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_BadCount)
#define FILE_ERR_BADSIZE        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_BadSize)
#define FILE_ERR_BADFILE        MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_BadFile)
#define FILE_ERR_FILENOTEMPTY   MakeFErr(ER_SEVERE,ER_C_NSTND,ER_Fs_FileNotEmpty)


/*****************************************************************************/


/* information returned when sending CMD_STATUS to a file device */
typedef struct FileStatus
{
    DeviceStatus fs;
    uint32       fs_ByteCount;    /* # bytes in the file    */
    uint32       fs_XIPFlags;     /* reserved               */
    uint32       fs_Flags;        /* see below              */
    uint32       fs_Type;         /* user-defined file type */
    uint8        fs_Version;      /* file's version         */
    uint8        fs_Revision;     /* file's revision        */
    TimeVal      fs_Date;
} FileStatus;

/* bits for the FileStatus.fs_Flags field */
#define FILE_IS_DIRECTORY       0x00000001
#define FILE_IS_READONLY        0x00000002
#define FILE_IS_FOR_FILESYSTEM  0x00000004
#define FILE_SUPPORTS_DIRSCAN   0x00000008
#define FILE_INFO_NOT_CACHED    0x00000010
#define FILE_SUPPORTS_ENTRY     0x00000020
#define FILE_BLOCKS_CACHED      0x00000040
#define FILE_USER_STORAGE_PLACE 0x00000200
#define FILE_STATIC_MAPPABLE    0x00000400
#define FILE_DYNAMIC_MAPPABLE   0x00000800
#define FILE_SUPPORTS_ADDDIR    0x00001000
#define FILE_HAS_VALID_VERSION  0x00002000
#define FILE_CONTAINS_VERSIONED 0x00004000
#define FILE_IS_BLESSED         0x00008000
#ifndef EXTERNAL_RELEASE
#define FILEFLAGS_SETTABLE      0xFF000000
#define FILE_FS_SCANNED		0x80000000
#endif

/* some predefined file types */
#define FILE_TYPE_DIRECTORY     0x2a646972
#define FILE_TYPE_CATAPULT      0x2a7a6170
#define FILE_TYPE_SYSTEM        0x2a737973
#define FILE_TYPE_LABEL         0x2a6c626c


/*****************************************************************************/


/* information returned when sending FILECMD_FSSTAT */
typedef struct FileSystemStat
{
    uint32  fst_BitMap;             /* field bitmap                          */
    char    fst_RawDeviceName[FILESYSTEM_MAX_NAME_LEN]; /* device name       */
    uint32  fst_RawDeviceItem;      /* device stack item number              */
    uint32  fst_RawOffset;          /* device offset                         */
    char    fst_MountName[FILESYSTEM_MAX_NAME_LEN];     /* mount point       */
    TimeVal fst_CreateTime;         /* filesystem creation time              */
    uint32  fst_BlockSize;          /* block size of the filesystem          */
    uint32  fst_Size;               /* size of the filesystem in blocks      */
    uint32  fst_MaxFileSize;        /* max blocks that can be allocated      */
    uint32  fst_Free;               /* total number of free blocks available */
    uint32  fst_Used;               /* total number of blocks in use         */
} FileSystemStat;

/* bits for fst_BitMap that indicate which of the following structure
 * fields contain valid information.
 */
#define	FSSTAT_RAWDEVICENAME	0x001
#define	FSSTAT_RAWDEVICEITEM	0x002
#define	FSSTAT_RAWOFFSET	0x004
#define	FSSTAT_MOUNTNAME	0x008
#define	FSSTAT_CREATETIME	0x010
#define	FSSTAT_BLOCKSIZE	0x020
#define	FSSTAT_SIZE		0x040
#define	FSSTAT_MAXFILESIZE	0x080
#define	FSSTAT_FREE		0x100
#define	FSSTAT_USED		0x200

#define	FSSTAT_ISSET(bmap, bit)	(bmap & bit)
#define	FSSTAT_SET(bmap, bit)	(bmap |= bit)


/*****************************************************************************/

#ifndef EXTERNAL_RELEASE
/*
   Warning - the _VERSION_, _REVISION_, and _VERREV_  search tags must
   start with a value of 1 and increase monotonically... there's a const
   table in OpenClose.c which is indexed by these tags.
*/
#endif /* EXTERNAL_RELEASE */

/* Tags for FindFileXXX() */
#define FILESEARCH_TAG_VERSION_EQ             TAG_END+1
#define FILESEARCH_TAG_VERSION_NE             TAG_END+2
#define FILESEARCH_TAG_VERSION_LT             TAG_END+3
#define FILESEARCH_TAG_VERSION_GT             TAG_END+4
#define FILESEARCH_TAG_VERSION_LE             TAG_END+5
#define FILESEARCH_TAG_VERSION_GE             TAG_END+6
#define FILESEARCH_TAG_REVISION_EQ            TAG_END+7
#define FILESEARCH_TAG_REVISION_NE            TAG_END+8
#define FILESEARCH_TAG_REVISION_LT            TAG_END+9
#define FILESEARCH_TAG_REVISION_GT            TAG_END+10
#define FILESEARCH_TAG_REVISION_LE            TAG_END+11
#define FILESEARCH_TAG_REVISION_GE            TAG_END+12
#define FILESEARCH_TAG_VERREV_EQ              TAG_END+13
#define FILESEARCH_TAG_VERREV_NE              TAG_END+14
#define FILESEARCH_TAG_VERREV_LT              TAG_END+15
#define FILESEARCH_TAG_VERREV_GT              TAG_END+16
#define FILESEARCH_TAG_VERREV_LE              TAG_END+17
#define FILESEARCH_TAG_VERREV_GE              TAG_END+18
#ifndef EXTERNAL_RELEASE
#define FILESEARCH_TAG_TABLE_SIZE             TAG_END+19
#endif /* EXTERNAL_RELEASE */
#define FILESEARCH_TAG_FIND_FIRST_MATCH       TAG_END+19
#define FILESEARCH_TAG_FIND_HIGHEST_MATCH     TAG_END+20
#define FILESEARCH_TAG_NOVERSION_IS_0_0       TAG_END+21
#define FILESEARCH_TAG_NOVERSION_IGNORED      TAG_END+22
#define FILESEARCH_TAG_SEARCH_PATH            TAG_END+23
#define FILESEARCH_TAG_SEARCH_FILESYSTEMS     TAG_END+24
#define FILESEARCH_TAG_SEARCH_ITEM            TAG_END+25
#ifndef EXTERNAL_RELEASE
#define FILESEARCH_TAG_TRACE_SEARCH           TAG_END+26
#endif /* EXTERNAL_RELEASE */

/* The following "Don't search" option flags may be set in the argument
 * to the FILESEARCH_TAG_SEARCH_FILESYSTEMS tag.  They will inhibit the
 * searching of any filesystem whose characteristics match _any_ of the
 * options set.
 *
 * "Quiescent" means "only half loaded, the root directory hasn't been
 * opened yet, memory footprint has been minimized."  "No code" means "the
 * driver module for this filesystem type has not been loaded into memory
 * yet."  "User storage" means "the root directory has the `This is a place
 * to store user files' flag set."  "Unblessed" means "The filesystem is
 * not labelled as having 3DO operating-system software present on it."
 */
#define DONT_SEARCH_QUIESCENT                 0x00000001
#define DONT_SEARCH_NO_CODE                   0x00000002
#define DONT_SEARCH_USER_STORAGE              0x00000004
#define DONT_SEARCH_UNBLESSED                 0x00000008


/*****************************************************************************/


/* Flag definitions for the MinimizeFileSystem() function. Identifies specific
 * ways in which the File Folio should be encouraged to reduce its memory
 * footprint.
 *
 * FSMINIMIZE_FLUSH_FILES - locate in-memory data structures for files and
 *                          directories which had been in use, but are no
 *                          longer in use, and delete these structures.
 *
 * FSMINIMIZE_COMPACT_CACHE - reduce the size of the filesystem cache.
 *
 * FSMINIMIZE_QUIESCE - ask filesystems to become quiescent (flushing unused
 *                      file structures, closing metadata files, and reducing
 *                      per-filesystem RAM usage to the bare essentials.  A
 *                      filesystem cannot become quiescent if you have any
 *                      files or directories open on it.
 *                      A quiescent filesystem will automatically be
 *                      reactivated if you try to open any file on it.
 *
 * FSMINIMIZE_UNLOAD - quiesce the filesystems, and then where possible unload
 *                     the filesystem interpreter code from memory.  The code
 *                     will be automatically reloaded when the filesystem is
 *                     opened again.
 *
 * FSMINIMIZE_DISMOUNT - quiesce the filesystems, attempt to unload the code,
 *                       and attempt to completely dismount the filesystems.
 *                       Be very careful with this option - you may unmount
 *                       a filesystem that you can't live without!
 */
#define FSMINIMIZE_FLUSH_FILES   0x00000001
#define FSMINIMIZE_COMPACT_CACHE 0x00000002
#define FSMINIMIZE_QUIESCE       0x00000004
#define FSMINIMIZE_UNLOAD        0x00000008
#define FSMINIMIZE_DISMOUNT      0x00000010

#define FSMINIMIZE_RECOMMENDED   (FSMINIMIZE_FLUSH_FILES |\
                                  FSMINIMIZE_QUIESCE |\
                                  FSMINIMIZE_UNLOAD)


/*****************************************************************************/


/* tags for the FormatFileSystem() function */
typedef enum FormatFSTags
{
    FORMATFS_TAG_FSNAME = TAG_ITEM_LAST+1,
    FORMATFS_TAG_OFFSET,
} FormatFSTags;


#ifndef EXTERNAL_RELEASE
/*****************************************************************************/


typedef struct HighLevelDisk HighLevelDisk;
typedef struct File File;
typedef struct FileSystem FileSystem;
typedef struct FileSystemType FileSystemType;

/*
  Define the prototypes, structures, and flags used to communicate between
  the filesystem-independent layer in the File Folio, and the various
  filesystem "type" interpreters.
*/

#define FSTYPE_LOADABLE     0x01
#define FSTYPE_NEEDS_LOAD   0x02
#define FSTYPE_UNLOADABLE   0x04
#define FSTYPE_WANTS_UNLOAD 0x08
#define FSTYPE_IS_LOADING   0x10
#define FSTYPE_IN_USE       0x20
#define FSTYPE_AUTO_QUIESCE 0x40

enum FSActQue {ActivateFS, QuiesceFS};

typedef struct FileIOReq FileIOReq;
typedef Err (*FileDriverLoader) (FileSystemType *fst);
typedef Item (*FileDriverMount)
 (Device *theDevice, uint32 blockOffset, IOReq *rawRequest,
  ExtVolumeLabel *discLabel, uint32 labelOffset, DeviceStatus *devStatus);
typedef Err (*FileDriverDismount) (FileSystem *fs);
typedef Err (*FileDriverActQue) (FileSystem *fs, enum FSActQue);
typedef Err (*FileDriverQueueit)
     (FileSystem *fs, IOReq *theRequest);
typedef void (*FileDriverAction) (FileSystem *fs);
typedef IOReq * (*FileDriverEA) (IOReq *theRequest);
typedef void (*FileDriverAbort) (IOReq *theRequest);
typedef Item (*FileDriverEntry) (File *parent, char *name);
typedef Err (*FileDriverAlloc) (IOReq *theRequest);
typedef void (*FileDriverClose) (File *theFile);
typedef int32 (*FileDriverIOReq) (FileIOReq *ior);

typedef Err (*FileDriverFormat) (Item ioreq,
                                 const DeviceStatus *stat,
                                 const char *name,
                                 const TagArg *tags);

struct FileSystemType {
  Node               fst;
  uint8              fst_VolumeStructureVersion;
  uint8              fst_DeviceFamilyCode;
  Err                fst_LoadError;
  Item               fst_ModuleItem;
  FileDriverLoader   fst_Loader;
  FileDriverLoader   fst_Unloader;
  FileDriverMount    fst_Mount;
  FileDriverActQue   fst_ActQue;
  FileDriverDismount fst_Dismount;
  FileDriverQueueit  fst_QueueRequest;
  FileDriverAction   fst_FirstTimeInit;
  FileDriverAction   fst_Timeslice;
  FileDriverAbort    fst_AbortIO;
  FileDriverEntry    fst_CreateEntry;
  FileDriverEntry    fst_DeleteEntry;
  FileDriverAlloc    fst_AllocSpace;
  FileDriverClose    fst_CloseFile;
  FileDriverIOReq    fst_CreateIOReq;
  FileDriverIOReq    fst_DeleteIOReq;
  FileDriverFormat   fst_Format;
};

struct FileSystem {
  ItemNode         fs;
  char             fs_FileSystemName[FILESYSTEM_MAX_NAME_LEN];
  char             fs_MountPointName[2*FILESYSTEM_MAX_NAME_LEN];
  uchar            fs_DeviceBusy; /* boolean or extended, 0 == not busy! */
  uchar            fs_RequestPriority;
  uchar            fs_RunningPriority;
  uchar            fs_VolumeFlags;
  List             fs_RequestsToDo;
  List             fs_RequestsRunning;
  List             fs_RequestsDeferred;
  uint32           fs_Flags;
  uint32           fs_VolumeBlockSize;
  uint32           fs_VolumeBlockCount;
  uint32           fs_VolumeUniqueIdentifier;
  uint32           fs_RootDirectoryBlockCount;
  int32            fs_DeviceBlocksPerFilesystemBlock;
  struct File     *fs_RootDirectory;
  FileSystemType  *fs_Type;
  Task            *fs_Thread;
  Device          *fs_RawDevice;
  DeviceStatus    *fs_DeviceStatusTemp;
  uint32           fs_RawOffset;
  uint32           fs_XIPFlags;
  uint32           fs_MountError;
};


/*
  OperaFS defines a filesystem which resides on a real
  mass-storage medium, and whose performance characteristics are such
  that doing seek optimization is important.
*/

enum CatapultPhase {
  CATAPULT_NONE = 0,
  CATAPULT_AVAILABLE,
  CATAPULT_READING,
  CATAPULT_MUST_READ_FIRST,
  CATAPULT_SHOULD_READ_NEXT,
  CATAPULT_DO_READ_NEXT,
  CATAPULT_MUST_VERIFY,
  CATAPULT_MUST_SHUT_DOWN
};

typedef struct OperaFS {
  FileSystem           ofs;
  FileIOReq          **ofs_RequestSort;
  IOReq               *ofs_RawDeviceRequest;
  int32                ofs_RequestSortLimit;
  uint32               ofs_BlockSize;
  uint32               ofs_BlockCount;
  uint32               ofs_NextBlockAvailable;
  uint32               ofs_RawDeviceBlockOffset;
  uchar                ofs_DeferredPriority;
  struct OFile     *ofs_CatapultFile;
  struct IoCacheEntry *ofs_CatapultPage;
  enum CatapultPhase   ofs_CatapultPhase;
  int32                ofs_CatapultNextIndex;
  int32                ofs_CatapultHits;
  int32                ofs_CatapultMisses;
  int32                ofs_TotCatapultStreamedHits;
  int32                ofs_TotCatapultNonstreamedHits;
  int32                ofs_TotCatapultTimesEntered;
  int32                ofs_TotCatapultDeclined;
  int32                ofs_TotCatapultSeeksAvoided;
  int32                ofs_TotCatapultMisses;
  int32                ofs_TotCatapultNonstreamedMisses;
  void                *ofs_FilesystemMemBase;
} OperaFS;

#define DRIVER_FLAW_MASK              0x000000FF
#define DRIVER_FLAW_SHIFT             24
#define DRIVER_BLOCK_MASK             0x00FFFFFF
#define DRIVER_FLAW_SOFTLIMIT         0x000000FE
#define DRIVER_FLAW_HARDERROR         0x000000FF

#define DEVICE_BOINK                  0x80

#define MAX_ZERO_USE_FILES            16
#define CLEANUP_TIMER_LIMIT           8

/*
  HostFS describes a generalized filesystem which resides on a remote
  host.  Coordination between the filesystem and the remote host is
  performed by a device driver which implements the HOSTCMD command suite.
  In general, HOSTCMD_foo performs the functions needed to implement the
  FILECMD_foo command;  parameters are passed somewhat differently.
*/

typedef struct HostToken {
  MinNode            ht;
  int32              ht_TokenValid;
  int32              ht_TokenValue;
} HostToken;

typedef struct HostFS {
  FileSystem         hfs;
  IOReq             *hfs_RawDeviceRequest;
  List               hfs_TokensToRelease;
  uint32             hfs_DaemonHasOpened;
  uint32             hfs_UserRequestsRunning;
} HostFS;


#define FILESYSTEM_IS_READONLY      0x00000002
#define FILESYSTEM_IS_OFFLINE       0x00000008
#define FILESYSTEM_CACHEWORTHY      0x00000040
#define FILESYSTEM_IS_INTRANS       0x00000080
#define FILESYSTEM_NEEDS_INIT       0x00000100
#define FILESYSTEM_IS_QUIESCENT     0x00000200
#define FILESYSTEM_WANTS_QUIESCENT  0x00000400
#define FILESYSTEM_WANTS_DISMOUNT   0x00000800
#define FILESYSTEM_WANTS_RECHECK    0x00001000

#define	FS_DIRSEMA

struct File {
  ItemNode         fi;
  char             fi_FileName[FILESYSTEM_MAX_NAME_LEN];
  FileSystem      *fi_FileSystem;
  struct File     *fi_ParentDirectory;
#ifdef	FS_DIRSEMA
  Item		   fi_DirSema;
#endif	/* FS_DIRSEMA */
  uint32           fi_UniqueIdentifier;
  uint32           fi_Type;
  uint32           fi_Flags;
  uint32           fi_UseCount;
  uint32           fi_BlockSize;
  uint32           fi_ByteCount;
  uint32           fi_BlockCount;
  TimeVal          fi_Date;
  uint8            fi_Version;
  uint8            fi_Revision;
  uint16           rfu1;        /* was part of burst */
  uint32           fi_LastAvatarIndex;
  uint32           fi_FilesystemSpecificData;
  uint32           fi_FileSystemBlocksPerFileBlock;
  uint32           fi_AvatarList[1];
};

typedef struct OFile {
  Device             ofi;
  File              *ofi_File;
  IOReq             *ofi_InternalIOReq;
} OFile;

struct FileIOReq {
  IOReq                fio;
  uint32               fio_BlockCount;
  uint32               fio_BlockBurst;
  uint32               fio_DevBlocksPerFileBlock;
  uint32               fio_AbsoluteBlockNumber;
  uint32               fio_AvatarIndex;
  uint32               fio_NextDirBlockIndex;
  uint32               fio_NextDirEntryIndex;
  struct IoCacheEntry *fio_CachePage;
};


/*
  MountRequest is used so that tasks which are trying to open a filesystem
  that has not yet been fully mounted, can hand off a request-for-action
  to the File Daemon.
*/

typedef struct MountRequest {
  MinNode      mr;
  Item         mr_TaskRequestingMount;
  Item         mr_FileSystemItem;
  Err          mr_Err; /* 1 if pending, 0 if no error, <0 if error */
} MountRequest;

enum CacheState {CachePageInvalid,
		   CachePageValid,
		   CachePageLoading,
		   CachePagePartial};

/*
   The CacheState enum is used to record the state of cache pages, and
   also to return status information from FindBlockInCache to tell the
   caller just how valid the block information is.

   CachePageValid:  in the IoCacheEntry, indicates that the information
   in ioce_CachedBlock is valid (meaningful identifiers, valid up through
   ioce_CachedBlockSize bytes.  Returned from FindBlockInCache, indicates
   that the desired block has been found in a cache page, that both
   the returned IoCacheEntry and blockBase pointers are valid, and the use
   count has been incremented.

   CachePageInvalid: in the IoCacheEntry, indicates that no information in
   ioce_CachedBlock has any validity.  Returned from FindBlockInCache,
   indicates that the desired block was nowhere to be found, and neither
   returned pointer is valid.

   CachePageLoading:  in the IoCacheEntry, indicates that some process is
   currently loading information into (or appending to) the ioce_CachedBlock
   buffer; I/O is not yet done, and ioce_CachedBlockSize has not been
   updated.  Returned from FindBlockInCache, indicates that a cache page
   was located which looks as if it could contain the desired block, but
   some process is doing I/O into it and we don't know yet whether the
   desired data will be loaded there;  the returned IoCacheEntry pointer is
   valid and points to the page in question, blockBase pointer is not valid,
   use count on this page has not been incremented.

   CachePagePartial: not used in the IoCacheEntry.  Returned from
   FindBlockInCache, indicates that a page was found which would be the
   one that would hold the desired block, but the data isn't there (that
   is, the cache page contains one or more blocks but is not filled up
   through the desired block); the returned IoCacheEntry pointer is
   valid and points to the page in question, blockBase pointer is not
   valid, use count on this page has not been incremented.

*/

typedef struct IoCacheEntry {
  NamelessNode   ioce;
  FileSystem    *ioce_Filesystem;
  uint32         ioce_FileUniqueIdentifier;
  uint32         ioce_FileBlockOffset;
  uint32         ioce_CacheFormat;
  uint32         ioce_CacheFirstIndex;
  uint32         ioce_CacheEntryCount;
  uint32         ioce_CacheMiscValue;
  uint32         ioce_CachedBlockSize;
  uint32         ioce_UseCount;
  enum CacheState ioce_PageState;
  void          *ioce_CachedBlock;
} IoCacheEntry;

#define CACHE_PRIO_HIT 16
#define CACHE_PRIO_MISS 8

#define CACHE_OPERA_DIRECTORY         1
#define CACHE_CATAPULT_INDEX          3

#define FILESYSTEM_CACHE_PAGE_SIZE    2048
#define FILESYSTEM_CACHE_SYSPAGES     8

typedef struct IoCache {
  uint32         ioc_EntriesAllowed;
  uint32         ioc_EntriesPresent;
  uint32         ioc_EntriesReserved;
  List           ioc_CachedBlocks;
  List           ioc_TasksSleeping;
} IoCache;

typedef struct CacheSleeper {
  MinNode        cs;
  uint32         cs_CacheWakeup;
  Task          *cs_Sleeper;
} CacheSleeper;

typedef Err (*CacheLoader)(File *theFile, int32 fileBlockNumber,
			   void *bufBase, int32 bufLen);

Err ReserveCachePages (int32 numNeeded, int32 canSleep);
void  RelinquishCachePages (int32 numToGiveUp);
IoCacheEntry *GetFreeCachePage(int32 canSleep);
void RelinquishCachePage(IoCacheEntry *cachePage);
void InvalidateCachePage(IoCacheEntry *cachePage);
void InvalidateFileCachePages(File *theFile);
void InvalidateFilesystemCachePages(FileSystem *fs);
enum CacheState FindBlockInCache(File *theFile, int32 fileBlockNumber,
				 int32 canSleep,
				 IoCacheEntry **cachePage, void **blockBase);
Err LoadBlockIntoCache(File *theFile, int32 fileBlockNumber,
		       IoCacheEntry **cachePage, void **blockBase,
		       CacheLoader loaderFunction);
void SetCachePagePriority(IoCacheEntry *cachePage, uint8 pagePrio);
void BumpCachePagePriority(IoCacheEntry *cachePage, uint8 pagePrio);
void AgeCache(void);
void SleepCache(void); /** must be called disabled! **/
void CacheWake(void); /** must be called disabled! **/

typedef enum SchedulerSweepDirection {
  BottomIsCloser = 1,
  TopIsCloser = 2
} SchedulerSweepDirection;

typedef struct Alias {
  ItemNode       a;
  uchar         *a_Value;
} Alias;

typedef struct CatapultPage {
  uint32         cp_MBZ;         /* must be zero */
  uint32         cp_Fingerprint; /* contains '*zap' */
  int32          cp_Entries;     /* contains # of valid entries */
  int32          cp_NextPage;    /* contains block # of next page, or -1 */
  struct {
    uint32       cpe_FileIdentifier;  /* unique ID of file */
    uint32       cpe_FileBlockOffset; /* offset of run in original file */
    uint32       cpe_RunLength;       /* # of blocks in this run */
    uint32       cpe_RunOffset;       /* offset of run in catapult file */
  } cpe[1];                    /* actually there are enough to fill page */
} CatapultPage;

typedef struct FileFolio {
  Folio          ff;
  File          *ff_Root;
  List           ff_Filesystems;
  List           ff_Files;
  List           ff_MountRequests;
  int32          ff_NextUniqueID;
  int32          ff_OpensSinceCleanup;
  int32          ff_MountLevel;
  struct {
      Task          *ffd_Task;
      uint32         ffd_QueuedSignal;    /* "first I/O queued for a device" */
      uint32         ffd_HeartbeatSignal; /* "Heartbeat timer went off"      */
      uint32         ffd_RescheduleSignal;/* "Did last I/O for a device      */
    } ff_Daemon;
  struct {
      Task          *ffm_Task;
      uint32         ffm_Signal;
    } ff_Mounter;
  List           ff_FileSystemTypes;
  List           ff_Aliases;
} FileFolio;

typedef struct FileFolioTaskData {
  File          *fftd_CurrentDirectory;
  uint32         fftd_ErrorCode;
} FileFolioTaskData;

enum MountLevels {
  NO_MOUNTS                = 0,
  MOUNT_INTERNAL_RO        = 1,
  MOUNT_EXTERNAL_RO        = 2,
  MOUNT_INTERNAL_RW_BOOT   = 3,
  MOUNT_EXTERNAL_RW_BOOT   = 4,
  MOUNT_INTERNAL_RW_NOBOOT = 5,
  MOUNT_EXTERNAL_RW_NOBOOT = 6,
  MOUNT_EVERYTHING         = 255
};

#ifndef	HOWMANY
#define	HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#endif	/* HOWMANY */

#ifdef	FS_DIRSEMA
void	InitDirSema(File *fp, int setowner);
void	DelDirSema(File *fp);
void	LockDirSema(File *fp, char *msg);
void	UnlockDirSema(File *fp, char *msg);
#else	/* FS_DIRSEMA */
#define LockDirSema(fp, msg)	/* no sema */
#define UnlockDirSema(fp, msg)	/* no sema */
#endif	/* FS_DIRSEMA */

#endif /* EXTERNAL_RELEASE */
/*****************************************************************************/


#endif /* __FILE_FILESYSTEM_H */
