/* @(#) folio.c 96/09/06 1.71 */

/* Code to initialize the file folio */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/kernel.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/folio.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <kernel/tags.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <stdlib.h>
#include <string.h>

#undef DEBUG

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

extern void OpenFileSWI(void);
extern void CloseOpenFileSWI(void);
extern void MountFileSystemSWI(void);
extern void OpenFileInDirSWI(void);
extern void ChangeDirectorySWI(void);
extern void ChangeDirectoryInDirSWI(void);
extern void GetDirectorySWI(void);
extern void CreateFileSWI(void);
extern void CreateFileInDirSWI(void);
extern void DeleteFileSWI(void);
extern void DeleteFileInDirSWI(void);
extern void CreateAliasSWI(void);
extern void DismountFileSystemSWI(void);
extern void RenameSWI(void);
extern void CreateDirSWI(void);
extern void CreateDirInDirSWI(void);
extern void DeleteDirSWI(void);
extern void DeleteDirInDirSWI(void);
extern void FindFileAndOpenSWI(void);
extern void FindFileAndIdentifySWI(void);
extern void SetFileAttrs(void);
extern void MountAllFileSystemsSWI(void);
extern void RecheckAllFileSystemsSWI(void);
extern void MinimizeFileSystemSWI(void);
extern void SetMountLevelSWI(void);
extern void FormatFileSystemSWI(void);

static const void *(*FileSWIFunctions[])() =
{
  (void *(*)()) OpenFileSWI,             /* 0 */
  (void *(*)()) CloseOpenFileSWI,        /* 1 */
  (void *(*)()) MountFileSystemSWI,      /* 2 */
  (void *(*)()) OpenFileInDirSWI,        /* 3 */
  (void *(*)()) ChangeDirectorySWI,      /* 4 */
  (void *(*)()) GetDirectorySWI,         /* 5 */
  (void *(*)()) CreateFileSWI,           /* 6 */
  (void *(*)()) DeleteFileSWI,           /* 7 */
  (void *(*)()) CreateAliasSWI,          /* 8 */
  (void *(*)()) DismountFileSystemSWI,   /* 9 */
  (void *(*)()) CreateDirSWI,            /* 10 */
  (void *(*)()) DeleteDirSWI,            /* 11 */
  (void *(*)()) ChangeDirectoryInDirSWI, /* 12 */
  (void *(*)()) CreateFileInDirSWI,      /* 13 */
  (void *(*)()) DeleteFileInDirSWI,      /* 14 */
  (void *(*)()) CreateDirInDirSWI,       /* 15 */
  (void *(*)()) DeleteDirInDirSWI,       /* 16 */
  (void *(*)()) FindFileAndOpenSWI,      /* 17 */
  (void *(*)()) FindFileAndIdentifySWI,  /* 18 */
  (void *(*)()) MountAllFileSystemsSWI,  /* 19 */
  (void *(*)()) RecheckAllFileSystemsSWI,/* 20 */
  (void *(*)()) MinimizeFileSystemSWI,   /* 21 */
  (void *(*)()) RenameSWI,		 /* 22 */
  (void *(*)()) SetMountLevelSWI,	 /* 23 */
  (void *(*)()) FormatFileSystemSWI,	 /* 24 */
};

#define NUM_FILESWIFUNCS ((sizeof FileSWIFunctions) / sizeof (int32))

/*
   Imports from file.init module
*/

extern long InitFileFolio (FileFolio *folio);
extern void InitFileCache(void);

extern int32 TrimZeroUseFiles(FileSystem *fs, int32 limit);

#ifdef GIVEDURINGDELETE
extern void GiveDaemon(void *foo);
#endif

static const NodeData FileFolioNodeData[] = {
  { 0,                          0 },
  { sizeof (FileSystem),        NODE_ITEMVALID + NODE_NAMEVALID },
  { 0 /* File size varies */,   NODE_ITEMVALID + NODE_NAMEVALID },
  { sizeof (Alias),             NODE_ITEMVALID + NODE_NAMEVALID },
};

const TagArg fileFolioTags[] = {
  { TAG_ITEM_NAME,		  (void *) "File" },
  { CREATEFOLIO_TAG_BASE,         (void *) &fileFolio },
  { CREATEFOLIO_TAG_DATASIZE,	  (void *) sizeof (FileFolio) },
  { CREATEFOLIO_TAG_NSWIS,	  (void *) NUM_FILESWIFUNCS},
  { CREATEFOLIO_TAG_SWIS,	  (void *) FileSWIFunctions },
  { CREATEFOLIO_TAG_INIT,	  (void *) ((long)InitFileFolio) },
  { CREATEFOLIO_TAG_NODEDATABASE, (void *) FileFolioNodeData },
  { CREATEFOLIO_TAG_MAXNODETYPE,  (void *) FILEALIASNODE },
  { CREATEFOLIO_TAG_ITEM,         (void *) FILEFOLIO },
  { CREATEFOLIO_TAG_TASKDATA,     (void *) sizeof (FileFolioTaskData) },
  { 0,				  (void *) 0 },
};

#ifdef BUILD_STRINGS

static const char *fileFolioErrorTable[] = {
  "no error",
  "No such file",
  "Not a directory",
  "No such filesystem",
  "Illegal filename",
  "Medium I/O error",
  "Hardware error",
  "Bad parameter",
  "Filesystem full",
  "Filesystem damaged",
  "File[system] busy",
  "Duplicate filename",
  "Read-only file[system]",
  "Duplicate link",
  "Circular link",
  "Not a file",
  "Directory not empty",
  "Invalid meta data",
  "Logging unavailable",
  "Invalid state",
  "Log overflow",

    /* FILE_ERR_BADMODE */
    "Illegal file open mode or seek mode for operation",

    /* FILE_ERR_BADCOUNT */
    "Illegal number of bytes specified in read or write operation",

    /* FILE_ERR_BADSIZE */
    "Illegal size value given to GetRawFileInfo()",

    /* FILE_ERR_BADFILE */
    "Invalid or corrupt file pointer supplied to a function",
    "File not empty"
};

#endif

#define ffErrSize (sizeof fileFolioErrorTable / sizeof (char *))

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
# define INFO(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define INFO(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef BUILD_STRINGS

const TagArg fileFolioErrorTags[] = {
  { TAG_ITEM_NAME,                (void *) "FileSystem" },
  { ERRTEXT_TAG_OBJID,            (void *) ((ER_FOLI<<ERR_IDSIZE) + ER_FSYS) },
  { ERRTEXT_TAG_MAXERR,           (void *) ffErrSize },
  { ERRTEXT_TAG_TABLE,            (void *) fileFolioErrorTable },
  { TAG_END,                      (void *) 0 }
};

#else

const uint32 fileFolioErrorTags; /* keep linker/export happy */

#endif

FileFolio fileFolio;
char *fsCacheBase;
int32 fsCacheSize;
int32 fsCacheBusy;
IoCache fsCache;
IoCacheEntry *fsCacheEntryArray;
int32 fsCacheEntrySize;

static FileFolioTaskData *SetupFileFolioTaskDataFor(Task *theTask)
{
  FileFolioTaskData *ffPrivate;
  int32 folioIndex;
  folioIndex = fileFolio.ff.f_TaskDataIndex;
  ffPrivate = (FileFolioTaskData *) theTask->t_FolioData[folioIndex];
  if (ffPrivate == NULL) {
    DBUG(("Create file-folio private data for task 0x%x at 0x%x\n",
	   theTask->t.n_Item, theTask));
    ffPrivate = (FileFolioTaskData *) SuperAllocMem(sizeof (FileFolioTaskData),
					      MEMTYPE_ANY | MEMTYPE_FILL);
    if (!ffPrivate) {
      qprintf(("Could not create file-folio private data for task %d\n",
		   theTask->t.n_Item));
      return (FileFolioTaskData *) NULL;
    }
    ffPrivate->fftd_ErrorCode = 0;
    ffPrivate->fftd_CurrentDirectory = fileFolio.ff_Root;
    fileFolio.ff_Root->fi_UseCount ++;
    DBUG(("Allocating private space at %lx for task %x\n",
		 ffPrivate, theTask));
    theTask->t_FolioData[folioIndex] = ffPrivate;
#ifdef DEBUG
    qprintf(("Assigned.\n"));
#endif
  }
  return ffPrivate;
}

FileFolioTaskData *SetupFileFolioTaskData(void)
{
  return (SetupFileFolioTaskDataFor(CURRENTTASK));
}

/*
 *	Every tag has preference to Current running task. If no tag
 *	is specified the task that is being created will inherit
 *	from the parent task. In case tag is provided we first set
 *	cur/prog dir to curtask, then if tag is valid, curdir and
 *	progdir are selectively	replaced by the appropriate tag args.
 */
int32 FileFolioCreateTaskHook(Task *t, TagArg *tags)
{
  FileFolioTaskData *ffPrivate, *ownerPrivate;
  File *directoryFile;
  TagArg *thisTag;
  DBUG(("File-folio create-task hook for task 0x%x\n", t));
  if (!fileFolio.ff_Root) {
    DBUG(("Called file-folio create-task hook before root was created\n"));
    return t->t.n_Item;
  }
  ffPrivate = SetupFileFolioTaskDataFor(t);
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (CURRENTTASK != NULL) {
    ownerPrivate = (FileFolioTaskData *) CURRENTTASK->t_FolioData[fileFolio.ff.f_TaskDataIndex];
    if (ownerPrivate) {
      if (ownerPrivate->fftd_CurrentDirectory) {
	if (ffPrivate->fftd_CurrentDirectory) {
	  ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
	}
	ffPrivate->fftd_CurrentDirectory = ownerPrivate->fftd_CurrentDirectory;
	ffPrivate->fftd_CurrentDirectory->fi_UseCount ++;
      }
    }
  }
  if (!tags) {
    DBUG(("No tags.\n"));
    return t->t.n_Item;
  }
  while ((thisTag = NextTagArg(&tags)) != NULL) {
    if (((thisTag->ta_Tag >> 16) & 0x0000FFFF) == FILEFOLIO) {
      switch (thisTag->ta_Tag & 0x000000FF) {
      case FILETASK_TAG_CURRENTDIRECTORY:
	DBUG(("CURRENT_DIRECTORY tag 0x%x\n", thisTag->ta_Arg));
	if (thisTag->ta_Arg == 0) {
	  break;
	}
	directoryFile = (File *) CheckItem((Item) thisTag->ta_Arg,
					   FILEFOLIO,
					   FILENODE);
	if (directoryFile) {
	  if (directoryFile->fi_Flags & FILE_IS_DIRECTORY) {
	    if (ffPrivate->fftd_CurrentDirectory) {
	      ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
	    }
	    ffPrivate->fftd_CurrentDirectory = directoryFile;
	    directoryFile->fi_UseCount ++;
	    DBUG(("Setting current directory to %s\n", directoryFile->fi_FileName));
	  } else {
	    DBUG(("Not a directory item\n"));
	    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
	  }
	} else {
	  DBUG(("Bad item\n"));
	  return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
	}
	break;
      default:
	DBUG(("Unknown file-folio tag\n"));
	break; /* maybe should return an error code here */
      }
    } else {
      DBUG(("Not a filesystem tag, ignored.\n"));
    }
  }
  DBUG(("File folio create-task hook done\n"));
  return t->t.n_Item;
}

void DeleteFileTask(Task *theTask)
{
  FileFolioTaskData *ffPrivate;
  ffPrivate = (FileFolioTaskData *) theTask->t_FolioData[fileFolio.ff.f_TaskDataIndex];
  if (ffPrivate) {
    DBUG(("Tearing down private-data block 0x%x for task 0x%x\n",
	   ffPrivate, theTask->t.n_Item));
    theTask->t_FolioData[fileFolio.ff.f_TaskDataIndex] = NULL;
    if (ffPrivate->fftd_CurrentDirectory) {
      ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
#ifdef DEBUG
      qprintf(("Use-count for current-directory %s is now %d\n",
		   ffPrivate->fftd_CurrentDirectory->fi_FileName,
		   ffPrivate->fftd_CurrentDirectory->fi_UseCount));
#endif
    }
#ifdef DEBUG
    qprintf(("Releasing private-data block %x for task %x\n",
		 ffPrivate, theTask));
#endif
    SuperFreeMem(ffPrivate, sizeof (FileFolioTaskData));
#ifdef DEBUG
    qprintf(("Released.\n"));
#endif
  }
#ifdef ALWAYS_TRIM_AT_TASK_EXIT
  if (theTask->t_ThreadTask == (Task *) NULL) {
    (void) TrimZeroUseFiles(NULL, 0);
  }
#endif
}

Item CreateFileItem(ItemNode *theNode, uint8 nodeType, TagArg *args)
{
Err result;

  switch (nodeType) {
  case FILESYSTEMNODE:
    theNode->n_Name = (uchar *) ((FileSystem *)theNode)->fs_FileSystemName;
    DBUG(("Created filesystem node %s\n",theNode->n_Name));
    break;
  case FILENODE:
    if (theNode->n_Size < sizeof (File)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadSubType);
    }
    theNode->n_Name = (uchar *) ((File *) theNode)->fi_FileName;
    DBUG(("Created file node %s\n",theNode->n_Name));
    break;
  case FILEALIASNODE:
    result = TagProcessor(theNode, args, NULL, NULL);
    if (result < 0)
        return result;
  }
  return theNode->n_Item;
}

int32 DeleteIOReqItem(IOReq *req)
{
  OFile *ofi;
  FileSystem *fs;
  FileSystemType *fst;
  Err err = 0;
  ofi = (OFile *) req->io_Dev;
  fs = ofi->ofi_File->fi_FileSystem;
  if (fs) {
    fst = fs->fs_Type;
    if (fst->fst_DeleteIOReq) {
      err = (*fst->fst_DeleteIOReq) ((FileIOReq *) req);
    }
  }
  return err;
}

int32 DeleteFileItem(Item theItem, Task *theTask)
{
  Node *theNode;
  File *parent;
  FileSystem *fs;
  int32 useCount;
  theNode = (Node *) LookupItem(theItem);
  if (!theNode) {
    return 0;
  }
  switch (theNode->n_Type) {
  case FILESYSTEMNODE:
#ifdef DEBUG
    qprintf(("Delete filesystem node '%s' at 0x%lx for task %x\n",
	     theNode->n_Name, theNode, theTask->t.n_Item));
#else
	TOUCH(theTask);
#endif
    theNode->n_Name = NULL; /* since it's not a kernel-owned string */
    break;
  case FILENODE:
    DBUG(("Delete File node '%s' at %lx for task %x\n",
	   theNode->n_Name, theNode, theTask->t.n_Item));
    useCount = (int32) ((File *)theNode)->fi_UseCount;
    if (useCount > 0) {
      DBUG(("Nope!  Use count = %d\n", ((File *)theNode)->fi_UseCount));
#ifdef GIVEDURINGDELETE
      GiveDaemon(theNode);
#endif
      return -1;
    } else if (useCount < 0) {
      qprintf(("Gaah! File %s use count = %d\n",
		   ((File *)theNode)->fi_FileName,
		   ((File *)theNode)->fi_UseCount));
#ifdef GIVEDURINGDELETE
      GiveDaemon(theNode);
#endif
      return -1;
    } else {
      fs = ((File *)theNode)->fi_FileSystem;
      if (fs->fs_Type->fst_CloseFile) {
	DBUG(("Calling close-file hook for /%s\n",
	       ((File *)theNode)->fi_FileSystem->fs.n_Name));
	(*fs->fs_Type->fst_CloseFile)((File *) theNode);
      }
      parent = ((File *)theNode)->fi_ParentDirectory;
      RemNode(theNode);
      if (parent) {
	parent->fi_UseCount --;
      }
#ifdef DEBUG
      qprintf(("Unlinked from files list\n"));
#endif
    }
    theNode->n_Name = NULL; /* since it's not a kernel-owned string */
    break;
  case FILEALIASNODE:
    DBUG(("Unlinking and deleting alias node '%s' for task 0x%lx\n",
	  theNode->n_Name, theTask->t.n_Item));
    RemNode(theNode);
    break;
  default:
    qprintf(("Deleting filesystem node type %d, at %lx for task %x\n",
		 theNode->n_Type, theNode, theTask->t.n_Item));
    break;
  }
  return 0;
}

Err SetFileItemOwner(ItemNode *n, Item newOwner, struct Task *t)
{
  TOUCH(n);
  TOUCH(newOwner);
  TOUCH(t);
  return 0;
}
