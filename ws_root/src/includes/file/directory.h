#ifndef __FILE_DIRECTORY_H
#define __FILE_DIRECTORY_H


/******************************************************************************
**
**  @(#) directory.h 96/09/05 1.14
**
**  Folio data structures for access to entries in filesystem directories.
**
******************************************************************************/


#ifndef _KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif


/*****************************************************************************/


typedef struct Directory Directory;

typedef struct DirectoryEntry
{
    uint32  de_Flags;
    uint32  de_UniqueIdentifier;
    uint32  de_Type;
    uint32  de_BlockSize;
    uint32  de_ByteCount;
    uint32  de_BlockCount;
    uint8   de_Version;
    uint8   de_Revision;
    uint16  de_rfu;
    uint32  de_rfu2;
    uint32  de_AvatarCount;
    char    de_FileName[FILESYSTEM_MAX_NAME_LEN];
    uint32  de_Location;
    TimeVal de_Date;
} DirectoryEntry;

#ifndef EXTERNAL_RELEASE
#define de_Gap de_rfu2
#endif

/*****************************************************************************/


#endif /* __FILE_DIRECTORY_H */
