/* @(#) dirscan.h 96/10/30 1.5 */

#ifndef __DIRSCAN_H
#define __DIRSCAN_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif


/*****************************************************************************/


typedef enum
{
    ET_FILE,               /* regular file                        */
    ET_DIRECTORY,          /* directory we don't know the size of */
    ET_FILESYSTEM          /* a file system entry                 */
} EntryTypes;

typedef struct
{
    Node en;
} EntryNode;

#define en_Name      en.n_Name
#define en_ByteSize  en.n_Size
#define en_EntryType en.n_Type
#define en_Flags	 en.n_Flags

#define ENTRY_IS_IN_COPYLIST	0x01	/* en_Flags value */

#define EntryType(entry) 	((Node *)entry)->n_Type

/*****************************************************************************/


/* for ET_FILESYSTEM */
typedef struct
{
    EntryNode      fs;
    FileSystemStat fs_Stat;
    char           fs_SizeString[20];   /* size converted to string form */
} FileSysEntry;

/* for ET_FILE */
typedef struct
{
    EntryNode fe;
    char      fe_SizeString[20];  /* size converted to string form */
} FileEntry;

/* for ET_DIRECTORY */
typedef struct
{
    EntryNode de;
} DirEntry;


/*****************************************************************************/


typedef struct
{
    DirectoryEntry ei_DirEntry;
    bool           ei_FileSystem;
    char           ei_Creator[32];
} EntryInfo;


/*****************************************************************************/


/* definitions for callback functions */
typedef Err (* ScanProgressFunc)(void *userData, const char *path);
typedef Err (* ScanNeedMediaFunc)(void *userData, const char *fsName);
typedef Err (* ScanErrorFunc)(void *userData, const char *path, Err error);


/*****************************************************************************/


typedef struct
{
    List               ds_Entries;        /* entries at the current level        */
    uint32             ds_NumEntries;
    struct StorageReq *ds_Requester;
    ScanProgressFunc   ds_Progress;
    ScanNeedMediaFunc  ds_NeedMedia;
    ScanErrorFunc      ds_Error;
    void              *ds_UserData;
	bool			   ds_IsInCopyList;
} DirScanner;


/*****************************************************************************/


void PrepDirScanner(DirScanner *ds, ScanProgressFunc spf, ScanNeedMediaFunc snmf, ScanErrorFunc sef, struct StorageReq *req, void *userData);
void UnprepDirScanner(DirScanner *ds);
void ResetDirScanner(DirScanner *ds);
Err ScanDirectory(DirScanner *ds, const char *path);

Err AddEntry(DirScanner *ds, const char *fileName);
void RemoveEntry(DirScanner *ds, uint32 entryNum);
Err GetEntryInfo(const char *fileName, EntryInfo *info);


/*****************************************************************************/


#endif /* __DIRSCAN_H */
