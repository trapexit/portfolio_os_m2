#ifndef __FILE_FILEIO_H
#define __FILE_FILEIO_H


/******************************************************************************
**
**  @(#) fileio.h 96/09/05 1.12
**
**  File IO primitives
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif


/*****************************************************************************/


/* different modes used to open a file */
typedef enum FileOpenModes
{
    FILEOPEN_READ,              /* read-only, fail if doesn't exist      */

    FILEOPEN_WRITE,             /* write-only, create if doesn't exist   */
    FILEOPEN_WRITE_NEW,         /* write-only, erase old contents if any */
    FILEOPEN_WRITE_EXISTING,    /* write-only, fail if doesn't exist     */

    FILEOPEN_READWRITE,         /* read/write, create if doesn't exist   */
    FILEOPEN_READWRITE_NEW,     /* read/write, erase old contents if any */
    FILEOPEN_READWRITE_EXISTING /* read/write, fail if doesn't exist     */
} FileOpenModes;


/*****************************************************************************/


/* different types of seek operations */
typedef enum FileSeekModes
{
    FILESEEK_START,         /* relative to start of file    */
    FILESEEK_CURRENT,       /* relative to current position */
    FILESEEK_END            /* relative to end of file      */
} FileSeekModes;


/*****************************************************************************/


/* attributes that can be set for an opened file */
typedef enum FileAttrsTags
{
    FILEATTRS_TAG_FILETYPE = TAG_ITEM_LAST+1,  /* 32-bit type field          */
    FILEATTRS_TAG_VERSION,                     /* 8-bit version              */
    FILEATTRS_TAG_REVISION,                    /* 8-bit revision             */
    FILEATTRS_TAG_BLOCKSIZE,                   /* virtual block size of file */
    FILEATTRS_TAG_DATE                         /* modification date of file  */
} FileAttrsTags;


/*****************************************************************************/


/* information that can be obtained on an open file */
typedef struct FileInfo
{
    Item    fi_File;            /* item for this file              */
    uint32  fi_FileType;        /* 4-byte file type                */
    uint32  fi_ByteCount;       /* number of bytes in the file     */
    uint32  fi_BlockCount;      /* number of blocks in the file    */
    uint32  fi_BlockSize;       /* size of blocks for this file    */
    uint32  fi_BytePosition;    /* cursor position within the file */
    uint8   fi_Version;         /* file's version or 0             */
    uint8   fi_Revision;        /* file's revision or 0            */
    Err	    fi_Error;           /* current error status for file   */
    TimeVal fi_Date;            /* modification date of file       */
} FileInfo;


/*****************************************************************************/


/* a handle to a raw file */
typedef struct RawFile RawFile;


/****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* raw file I/O functions with no buffering */
Err OpenRawFile(RawFile **file, const char *path, FileOpenModes mode);
Err OpenRawFileInDir(RawFile **file, Item dir, const char *path, FileOpenModes mode);
Err CloseRawFile(RawFile *file);
int32 ReadRawFile(RawFile *file, void *buffer, int32 numBytes);
int32 WriteRawFile(RawFile *file, const void *buffer, int32 numBytes);
int32 SeekRawFile(RawFile *file, int32 position, FileSeekModes mode);
Err SetRawFileSize(RawFile *file, uint32 newSize);
Err GetRawFileInfo(const RawFile *file, FileInfo *info, uint32 infoSize);
Err SetRawFileAttrs(RawFile *file, const TagArg *tags);
Err SetRawFileAttrsVA(RawFile *file, uint32 tag, ... );
Err ClearRawFileError(RawFile *file);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* FILE_FILEIO_H */
