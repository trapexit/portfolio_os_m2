/* @(#) open.c 96/11/26 1.79 */

/* Code to open and close files. */

#define SUPER

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <file/directory.h>
#include <string.h>


int32 internalReadNextBlock(OFile *, int32);
int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen);
Err   internalDeleteEntry(File *stardingDir, char *path, uint32 cmd);
Item  internalCreateEntry(File *startingDir, char *path, enum FindMode mode);
IOReq *FileDriverEndAction(IOReq *theRequest);
Err   internalRename(File *fp, char *fname);
static Err   badname(char *fname);

extern FileFolioTaskData *SetupFileFolioTaskData(void);
extern void GiveDaemon(void *foo);


enum FindMode
{
  FindExistingEntry,
  CreateNewEntry,
  DeleteExistingEntry,
  UpdateCurrentDirectory,
  CreateNewDir
};


static const TagArg openFileArgs[] = {
  { TAG_ITEM_PRI,            (void *)1, },
  { CREATEDEVICE_TAG_DRVR,    NULL },
  { TAG_ITEM_NAME,            NULL },
  { TAG_END,                  0 }
};
#define	OPENFILE_TAG_NUM	(sizeof(openFileArgs) / sizeof(TagArg))

static const TagArg createAliasArgs[] = {
  { TAG_ITEM_NAME,            NULL },
  { TAG_END,                  0, },
};

#define MAX_SEARCH_TAGS 12

#define REVISION                  0x80000000
#define VERREV                    0x40000000
#define REJECT_IF_GT              0x20000000
#define REJECT_IF_EQ              0x10000000
#define REJECT_IF_LT              0x08000000
#define VALUE_MASK                0x0000FFFF

typedef struct VersionSearch {
  File       *bestFound;
  uint8       matchFirst;
  uint8       skipNonversioned;
  uint8       traceSearch;
  uint32      tagCount;
  uint32      compareTags[MAX_SEARCH_TAGS];
} VersionSearch;

#if (FILESEARCH_TAG_VERSION_EQ != 1 || FILESEARCH_TAG_TABLE_SIZE != 19)
# error Oops, filesystem search tags aren't right!
#endif

static const int32 SearchParameters[FILESEARCH_TAG_TABLE_SIZE] = {
  /* unused entry               */  0,
  /* FILESEARCH_TAG_VERSION_EQ  */  REJECT_IF_GT + REJECT_IF_LT,
  /* FILESEARCH_TAG_VERSION_NE  */  REJECT_IF_EQ,
  /* FILESEARCH_TAG_VERSION_LT  */  REJECT_IF_GT + REJECT_IF_EQ,
  /* FILESEARCH_TAG_VERSION_GT  */  REJECT_IF_EQ + REJECT_IF_LT,
  /* FILESEARCH_TAG_VERSION_LE  */  REJECT_IF_GT,
  /* FILESEARCH_TAG_VERSION_GE  */  REJECT_IF_LT,
  /* FILESEARCH_TAG_REVISION_EQ */  REVISION + REJECT_IF_GT + REJECT_IF_LT,
  /* FILESEARCH_TAG_REVISION_NE */  REVISION + REJECT_IF_EQ,
  /* FILESEARCH_TAG_REVISION_LT */  REVISION + REJECT_IF_GT + REJECT_IF_EQ,
  /* FILESEARCH_TAG_REVISION_GT */  REVISION + REJECT_IF_EQ + REJECT_IF_LT,
  /* FILESEARCH_TAG_REVISION_LE */  REVISION + REJECT_IF_GT,
  /* FILESEARCH_TAG_REVISION_GE */  REVISION + REJECT_IF_LT,
  /* FILESEARCH_TAG_VERREV_EQ   */  VERREV + REJECT_IF_GT + REJECT_IF_LT,
  /* FILESEARCH_TAG_VERREV_NE   */  VERREV + REJECT_IF_EQ,
  /* FILESEARCH_TAG_VERREV_LT   */  VERREV + REJECT_IF_GT + REJECT_IF_EQ,
  /* FILESEARCH_TAG_VERREV_GT   */  VERREV + REJECT_IF_EQ + REJECT_IF_LT,
  /* FILESEARCH_TAG_VERREV_LE   */  VERREV + REJECT_IF_GT,
  /* FILESEARCH_TAG_VERREV_GE   */  VERREV + REJECT_IF_LT,
};

#undef RENAMEDEBUG
#undef WATCHDIRECTORY
#undef DUMPFILES
#undef DEBUG
#define DEBUG2
#undef SEMDEBUG

#ifdef BUILD_PARANOIA
# undef WARN_ABSOLUTE_SYSTEM_PATH
#else
# undef WARN_ABSOLUTE_SYSTEM_PATH
#endif

#ifdef	SEMDEBUG
#define SEMDBUG(x) Superkprintf x
#define LOCKDIRSEMA(sem,txt) LockDirSema(sem,txt)
#define UNLOCKDIRSEMA(sem,txt) UnlockDirSema(sem,txt)
#else	/* SEMDEBUG */
#define SEMDBUG(x) /* x */
#define LOCKDIRSEMA(sem,txt) LockDirSema(sem,NULL)
#define UNLOCKDIRSEMA(sem,txt) UnlockDirSema(sem,NULL)
#endif	/* SEMDEBUG */

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef RENAMEDEBUG
#define RBG(x) Superkprintf x
#else
#define RBG(x) /* x */
#endif	/* RENAME_DEBUG */

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern int32 fsOpenForBusiness;

extern char *fsCacheBase;
extern IoCache fsCache;
extern Semaphore *fsListSemaphore;

#ifdef BUILD_STRINGS
void DumpFileList(void)
{
  File *theFile;
  DBUG0(("Files currently in list:\n"));
  theFile = (File *) FIRSTNODE(&fileFolio.ff_Files);
  while (ISNODE(&fileFolio.ff_Files,theFile)) {
    DBUG0(("  Item %x at %x name %s", theFile->fi.n_Item, theFile,
		 theFile->fi_FileName));
    DBUG0((" use-count %d\n", theFile->fi_UseCount));
    theFile = (File *) NEXTNODE(theFile);
  }
}
#endif

int32 TrimZeroUseFiles(FileSystem *fs, int32 limit)
{
  File *theFile, *nextFile;
  int32 count = 0;
  theFile = (File *) FirstNode(&fileFolio.ff_Files);
  while (ISNODE(&fileFolio.ff_Files,theFile)) {
    nextFile = (File *) NextNode(theFile);
    DBUG(("Check File item %s\n", theFile->fi_FileName));
    if ((fs == NULL || fs == theFile->fi_FileSystem) &&
	 theFile->fi_UseCount == 0 && --limit < 0) {
      DBUG(("Deleting File item for %s\n", theFile->fi_FileName));
#ifdef	FS_DIRSEMA
      DBUG(("TrimZeroFiles: Calling DelDirSema for [%s], task: 0x%x\n", theFile->fi_FileName, CURRENTTASKITEM));
      DelDirSema(theFile);
#endif	/* FS_DIRSEMA */
      SuperInternalDeleteItem(theFile->fi.n_Item);
      count++;
    }
    theFile = nextFile;
  }
  fileFolio.ff_OpensSinceCleanup = 0;
  return count;
}

static Alias *FindAlias(uchar *name)
{
  DBUG(("Searching for alias %s\n", name));
  return (Alias *) FindNamedNode(&fileFolio.ff_Aliases, name);
}

void CleanupOnClose(Device *theDev)
{
  Err err;
  OFile *theOpenFile = (OFile *) theDev;

  if (theOpenFile == (OFile *) NULL) {
    return;
  }
  DBUG(("Closing Ofile %s\n", theOpenFile->ofi_File->fi_FileName));
  if (theOpenFile->ofi_InternalIOReq != (IOReq *) NULL) {
    DBUG(("CleanupOnClose: Deleting IOReq for %s\n", theOpenFile->ofi_File->fi_FileName));
    if ((err = SuperDeleteItem(theOpenFile->ofi_InternalIOReq->io.n_Item)) != 0) {
      TOUCH(err);
      DBUG(("CleanupOnClose: Failed to delete IOREQ %x\n", err));
    } else {
      theOpenFile->ofi_InternalIOReq = (IOReq *) NULL;
    }
  } else {
    DBUG(("CleanupOnClose: InternalIOReq for %s is NULL\n", theOpenFile->ofi_File->fi_FileName));
  }
  return;
}


Err CleanupOpenFile(Device *theDev)
{
  File *theFile;
  OFile *theOpenFile = (OFile *) theDev;
  DBUG(("CleanupOpenFile\n"));
  if (theOpenFile == (OFile *) NULL) {
    return 0;
  }
  theFile = theOpenFile->ofi_File;
  DBUG(("Deleting Ofile %s\n", theFile->fi_FileName));
#ifdef DEBUG
  Superkprintf("Closing file %s at %lx\n", theOpenFile->ofi.dev.n_Name,
	       theOpenFile);
  DumpFileList();
#endif
/*
 *  we should have deleted the IOReq by now.
 */
  if (theOpenFile->ofi_InternalIOReq != (IOReq *) NULL) {
    DBUG(("Deleting IOReq at %lx\n", theOpenFile->ofi_InternalIOReq));
    SuperDeleteItem(theOpenFile->ofi_InternalIOReq->io.n_Item);
    theOpenFile->ofi_InternalIOReq = (IOReq *) NULL;
  }
  theFile->fi_UseCount --;
  DBUG(("Use count of file %s decremented to %d\n",
	 theFile->fi_FileName,
	 theFile->fi_UseCount));
  if (theFile->fi_UseCount == 0) {
    if (theFile->fi_Flags & FILE_INFO_NOT_CACHED) {
      SuperInternalDeleteItem(theFile->fi.n_Item);
    }
  }
  return 0;
}

Err EnsureMounted (FileSystem *fs)
{
  MountRequest mr;
  int32 interrupts;
  int32 gotSignal;

  if (fs->fs_Flags & FILESYSTEM_IS_INTRANS)
    return 0;

  if (fs->fs_MountError < 0) {
    DBUG(("EnsureMounted found an existing error!\n"));
    return fs->fs_MountError;
  }
  if (fs->fs_Flags & FILESYSTEM_IS_QUIESCENT) {
    DBUG(("%s initiating make-it-ready request on /%s\n",
	   CURRENTTASK->t.n_Name, fs->fs_FileSystemName));
    mr.mr_TaskRequestingMount = CURRENTTASK->t.n_Item;
    mr.mr_FileSystemItem = fs->fs.n_Item;
    mr.mr_Err = 1;
    interrupts = Disable();
    ClearSignals(CURRENTTASK, SIGF_ONESHOT);
    AddTail(&fileFolio.ff_MountRequests, (Node *) &mr);
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_QueuedSignal);
    do {
      gotSignal = SuperWaitSignal(SIGF_ONESHOT);
      DBUG(("EnsureMounted got signal 0x%X\n", gotSignal));
    } while (gotSignal >= 0 &&
	     (gotSignal & SIGF_ABORT) == 0 &&
	     mr.mr_Err > 0);
    if (mr.mr_Err > 0) {
      mr.mr_Err = (gotSignal < 0) ? gotSignal : KILLED;
      DBUG(("EnsureMounted removing request node\n"));
      RemNode((Node *) &mr);
    }
    fs->fs_MountError = mr.mr_Err;
    Enable(interrupts);
    DBUG(("EnsureMounted returns 0x%X\n", mr.mr_Err));
    return mr.mr_Err;
  }
  return 0;
}

OFile *OpenAFile(File *theFile)
{
  uint8 taskPrivs;
  Item openFileItem;
  OFile *theOpenFile;
  Err err;
  TagArg openFileArgBlock[OPENFILE_TAG_NUM];
  char deviceName[FILESYSTEM_MAX_NAME_LEN+2];
  if (!theFile) {
    return (OFile *) NULL;
  }
  theFile->fi_UseCount ++;  /* prevent scavenging */
  DBUG(("OpenAFile bumped use count of file '%s' to %d\n",
	 theFile->fi_FileName, theFile->fi_UseCount));
/*
   Do black magic to ensure that the filesystem interpreter for this
   filesystem is loaded and that the filesystem is fully mounted.

*/
  if (theFile != fileFolio.ff_Root &&
      theFile->fi_ParentDirectory == fileFolio.ff_Root) {
    DBUG(("Doing an EnsureMounted call on %s\n",
	   theFile->fi_FileSystem->fs_FileSystemName));
    err = EnsureMounted(theFile->fi_FileSystem);
    if (err < 0) {
      qprintf(("Could not get /%s fully mounted!\n",
	       theFile->fi_FileSystem->fs_FileSystemName));
      theFile->fi_UseCount --;  /* unlock it */
      ((FileFolioTaskData *) CURRENTTASK->
       t_FolioData[fileFolio.ff.f_TaskDataIndex])->fftd_ErrorCode =
	 err;
      return NULL;
    }
  }
  memcpy(openFileArgBlock, openFileArgs, sizeof openFileArgBlock);
  memcpy(deviceName+1, theFile->fi.n_Name, sizeof deviceName - 2);
  deviceName[0] = '@';
  deviceName[sizeof deviceName - 1] = '\0';
  openFileArgBlock[1].ta_Arg = (void *) fileDriver->drv.n_Item;
  openFileArgBlock[2].ta_Arg = (void *) deviceName;
#ifdef DEBUG
  Superkprintf("Creating an open-file for file '%s' at %lx\n",
	       openFileArgBlock[2].ta_Arg, theFile);
  DumpFileList();
#endif
  taskPrivs = CURRENTTASK->t.n_ItemFlags;
  CURRENTTASK->t.n_ItemFlags |= ITEMNODE_PRIVILEGED /* seize privilege */;
  openFileItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
				      openFileArgBlock,
				      sizeof (OFile));
  CURRENTTASK->t.n_ItemFlags =
    (CURRENTTASK->t.n_ItemFlags & ~ITEMNODE_PRIVILEGED) |
      (taskPrivs & ITEMNODE_PRIVILEGED); /* restore privilege */
  if (openFileItem < 0) {
#ifdef DEBUG
    DBUG(("Can't create open-file item (0x%x)\n", openFileItem));
    ERR(openFileItem);
#endif
    theFile->fi_UseCount --;
    ((FileFolioTaskData *) CURRENTTASK->
      t_FolioData[fileFolio.ff.f_TaskDataIndex])->fftd_ErrorCode =
	openFileItem;
    return NULL;
  }
  theOpenFile = (OFile *) LookupItem(openFileItem);
  if (theOpenFile == (OFile *) NULL) {
    DBUG(("Could not create open-file!\n"));
    theFile->fi_UseCount --;
    return theOpenFile;
  }
  theOpenFile->ofi_File = theFile;
#ifdef DEBUG
  DBUG(("Open-file created at %lx\n", theOpenFile));
  DumpFileList();
#endif
  DBUG(("Opening item 0x%x\n", openFileItem));
  err = SuperOpenItem(openFileItem, 0);
  DBUG(("Item 0x%x opened\n", openFileItem));
  if (err < 0) {
    DBUG(("Couldn't OpenItem() on a newly-created OFile!\n"));
    ERR(err);
  }
  if (!(theFile->fi_Flags & FILE_IS_DIRECTORY)) {
    RemNode((Node *) theFile);
    AddHead(&fileFolio.ff_Files, (Node *) theFile);
  }
  if (++fileFolio.ff_OpensSinceCleanup >= CLEANUP_TIMER_LIMIT) {
    TrimZeroUseFiles(NULL, MAX_ZERO_USE_FILES);
  }
  return theOpenFile;
}

/*
  N.B.  If FindFile returns a non-null File pointer, the use count of
        the returned File will have been incremented by one to ensure that
        it is not subject to scavenging.  The caller should account for
        this, and decrement the use count at an appropriate time.

  Memo to self - the use count of "whereNow" is not bumped up by one
  within the Grand Loop.  When an item-table scavenger is implemented
  in the folio, it will be necessary either to do the bumping in this
  fashion or have some other lockout to ensure that the whereNow File
  item is not flushed during the open-the-whereNow-file process or
  elsewhen.

*/

File *
FindFile (File *startingDirectory, char *path, enum FindMode mode)
{
  File *whereNow, *nextLevel, *punt;
  FileSystem *fs;
  OFile *openFile;
  Item fileItem, ioReqItem;
  FileFolioTaskData *ffPrivate;
  DirectoryEntry remoteDir;
  int32 errcode;
  int32 triedToCreate;
  int32 expansions;
  int32 interrupts;
  IOReq *rawIOReq = (IOReq *) NULL;
  char c, *fullPath, *buffer,  *rescan, *nameStart, *nameEnd;
  char *expandedPath, *newExpandedPath;
  int32 expandedPathLen, newExpandedPathLen;
  char fileName[FILESYSTEM_MAX_NAME_LEN+1];
  int32 nameLen, bufferSize = 0, insertPoint;
  int32 fileSize;
  int32 insideAlternative;
  TagArg ioReqTags[2];
  Alias *theAlias;
#ifdef WARN_ABSOLUTE_SYSTEM_PATH
  uint32 isAbsolute = FALSE;
#endif
  ffPrivate = (FileFolioTaskData *) CURRENTTASK->
    t_FolioData[fileFolio.ff.f_TaskDataIndex];
  if (!ffPrivate) {
    DBUG(("OOPS!  FindFile entered with null private pointer\n"));
  }
  whereNow = startingDirectory;
  punt = (File *) NULL;
  openFile = (OFile *) NULL;
  buffer = (char *) NULL;
  rescan = expandedPath = newExpandedPath = (char *) NULL;
  expandedPathLen = newExpandedPathLen = 0;
  TOUCH(newExpandedPath);
  TOUCH(newExpandedPathLen);
  triedToCreate = FALSE;
  insideAlternative = FALSE;
  fullPath = path;
  expansions = 0;
  if (mode == UpdateCurrentDirectory) {
    mode = FindExistingEntry;
  }

  if (!IsMemReadable(path, FILESYSTEM_MAX_PATH_LEN)) {
      ffPrivate->fftd_ErrorCode = FILE_ERR_BADPTR;
      return (File *) NULL;
  }

  DBUG(("FindFile, base dir %d, path %s\n", startingDirectory, path));
 theGrandLoop:
  while (*path) {
    DBUG(("Opening from 0x%lx, look for '%s' path %lx\n",
		 whereNow, path, path));
    c = *path;
    if (c == '/') { /* go back to root */
      DBUG(("Back to root\n"));
      whereNow = fileFolio.ff_Root;
      path ++;
#ifdef WARN_ABSOLUTE_SYSTEM_PATH
      isAbsolute = TRUE;
#endif
      if (punt && !insideAlternative) { /* {this|that}/foo/bar//root/... dcp */
	punt->fi_UseCount --;
	punt = NULL;
	rescan = NULL;
      }
      continue;
    }
    if (c == '{') {
      DBUG(("Alternative!\n"));
      if (rescan != NULL | mode != FindExistingEntry) {
	DBUG(("Malformed alternative-search!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      punt = whereNow;
      punt->fi_UseCount ++;
      path++;
      rescan = path;
      insideAlternative = TRUE;
      DBUG(("Rescan point is %s\n", rescan));
      continue;
    }
    nameLen = 0;
    nameStart = path;
    do {
      nameEnd = path;
      c = *path;
      if (c == '\0') {
	fileName[nameLen] = '\0';
	break;
      }
      if (c == '/') {
	fileName[nameLen] = '\0';
	path++;
	break;
      }
      if (c == '|') {
	DBUG(("Hit an or-delimiter\n"));
	if (punt == NULL) {
	  ffPrivate->fftd_ErrorCode =
	    MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	  goto trueFailure;
	}
	fileName[nameLen] = '\0';
	path++;
	do {
	  c = *path++;
	  if (c == '\0') {
	    ffPrivate->fftd_ErrorCode =
	      MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	    goto trueFailure;
	  }
	} while (c != '}');
	if (*path == '/') {
	  path++;
	}
	insideAlternative = FALSE;
	DBUG(("Skipped ahead to %s\n", path));
	break;
      }
      if (c == '}') {
	DBUG(("Hit end of alternative\n"));
	if (rescan == NULL) {
	  ffPrivate->fftd_ErrorCode =
	    MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	  goto trueFailure;
	}
	fileName[nameLen] = '\0';
	path++;
	if (*path == '/') {
	  path++;
	}
	rescan = NULL;
	punt->fi_UseCount --;
	punt = (File *) NULL;
	insideAlternative = FALSE;
	DBUG(("Cleared punt and rescan\n"));
	break;
      }
      if (nameLen >= FILESYSTEM_MAX_NAME_LEN) {
	DBUG(("Filename too long!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      fileName[nameLen++] = c;
      path++;
    } while (1);
    if (fileName[0] == '$') {
      DBUG(("Alias substitution for %s\n", fileName));
      if (++expansions >= 32) {
	DBUG(("Excessive alias expansion!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      theAlias = FindAlias(&fileName[1]);
      if (!theAlias) {
	DBUG(("Alias not found\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      DBUG(("Found alias, value is '%s'\n", theAlias->a_Value));
      newExpandedPathLen = (nameStart - fullPath) /* prefix */ +
	strlen(theAlias->a_Value) /* substitution */ +
	  strlen(nameEnd) /* suffix */ + 1;
      DBUG(("Need %d bytes for expansion\n", newExpandedPathLen));
      newExpandedPath = (char *) SuperAllocMem(newExpandedPathLen,
					  MEMTYPE_ANY + MEMTYPE_FILL);
      if (!newExpandedPath) {
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
	goto trueFailure;
      }
      insertPoint = nameStart - fullPath;
      strncpy(newExpandedPath, fullPath, insertPoint);
      strcpy(newExpandedPath + insertPoint, theAlias->a_Value);
      strcat(newExpandedPath, nameEnd);
      path = newExpandedPath + (nameStart - fullPath);
      if (rescan) {
	rescan = newExpandedPath + (rescan - fullPath);
      }

	SuperFreeMem(expandedPath, expandedPathLen);

      expandedPath = fullPath = newExpandedPath;
      expandedPathLen = newExpandedPathLen;
      DBUG(("Aliased path is now '%s'\n", fullPath));
      DBUG(("Start scanning at '%s'\n", path));
      continue;
    }
    DBUG(("Searching for %s from directory 0x%lx\n",
		 fileName, whereNow));
    DBUG(("Remainder is '%s' based at 0x%lx\n",
		 path, path));
    if (strcmp(fileName, ".") == 0) {
      continue;
    }
    if (strcmp(fileName, "..") == 0) {
      if (whereNow) {
	whereNow = whereNow->fi_ParentDirectory;
      }
      continue;
    }
    if (strcmp(fileName, "^") == 0) {
      if (whereNow->fi_FileSystem) {
	whereNow = whereNow->fi_FileSystem->fs_RootDirectory;
      }
#ifdef WARN_ABSOLUTE_SYSTEM_PATH
      else { /* honor the FindFileAndIdentify guardian hack */
	isAbsolute = FALSE;
      }
#endif
      continue;
    }
#ifdef WARN_ABSOLUTE_SYSTEM_PATH
    if (isAbsolute && strcasecmp(fileName, "System.M2") == 0) {
      qprintf(("WARNING: %s is chasing an absolute system path!\n",
	       CURRENTTASK->t.n_Name));
      qprintf(("         Suspect path is %s\n", fullPath));
    }
#endif
    DBUG(("0x%0X searching for %s, %s\n", CURRENTTASKITEM,
	   fileName, fullPath));
    LOCKDIRSEMA(whereNow, "before cache search");
    if (strlen(fileName) >= FILESYSTEM_MAX_NAME_LEN) {
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_BadName);
	goto trueFailure;
    }
    nextLevel = (File *) FIRSTNODE((&fileFolio.ff_Files));
    while (ISNODE(&fileFolio.ff_Files,nextLevel) &&
	   (nextLevel->fi_ParentDirectory != whereNow ||
	    strcasecmp(fileName, nextLevel->fi_FileName) != 0)) {
      nextLevel = (File *) NEXTNODE((&(nextLevel->fi)));
    }
    if (ISNODE(&fileFolio.ff_Files,nextLevel)) { /* found it */
      DBUG(("Found in cache %s, %s, flags 0x%x\n", fileName, nextLevel->fi_FileName, nextLevel->fi_Flags));
      UNLOCKDIRSEMA(whereNow, "found in cache");
      whereNow = nextLevel;
      DBUG(("Found %s in list at %x\n", fileName, whereNow));
      continue;
    }
    if (whereNow == fileFolio.ff_Root) {
      int32 foundIt;
      DBUG(("Searching filesystem list for '%s'\n", fileName));
    searchFilesystems:
      foundIt = FALSE;
      (void) SuperInternalLockSemaphore(fsListSemaphore,
					SEM_WAIT + SEM_SHAREDREAD);
      for (fs = (FileSystem *) FirstNode(&fileFolio.ff_Filesystems);
	   IsNode(&fileFolio.ff_Filesystems, fs);
	   fs = (FileSystem *) NextNode((Node *) fs)) {
	if (strcasecmp(fileName, fs->fs_FileSystemName) == 0 ||
	    strcasecmp(fileName, fs->fs_MountPointName) == 0) {
	  foundIt = TRUE;
	  break;
	}
      }
      (void) SuperInternalUnlockSemaphore(fsListSemaphore);
      if (!foundIt) {
	DBUG(("Cannot locate filesystem /%s for %s\n",fileName,fullPath));
/*
   If the daemon has not yet finished the initial series of filesystem
   mounts, don't fail this open.  Sleep for a while and then try again.
*/
	interrupts = Disable();
	if (!fsOpenForBusiness) {
	  SleepCache();
	  Enable(interrupts);
	  goto searchFilesystems;
	}
	Enable(interrupts);
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
	goto tryAlternate;
      }
      UNLOCKDIRSEMA(whereNow, "found in FScache");
      whereNow = fs->fs_RootDirectory;
      DBUG(("Found filesystem /%s root at %d\n", fileName, whereNow));
      continue;
    }
    if ((whereNow->fi_Flags & FILE_IS_DIRECTORY) == 0) {
      DBUG(("%s is a file, not a directory\n", fileName));
      ffPrivate->fftd_ErrorCode =
	MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
      goto tryAlternate;
    }
    DBUG(("Path is now %lx\n", path));
    rawIOReq = NULL;
    openFile = OpenAFile(whereNow);
    if (!openFile) goto cleanup;
/*
   Effective with M2, all filesystem modules are REQUIRED to support
   FILECMD_READENTRY!
*/
    ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
    ioReqTags[0].ta_Arg = (void *) openFile->ofi.dev.n_Item;
    ioReqTags[1].ta_Tag = TAG_END;
    ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
    if (ioReqItem < 0) {
      DBUG(("Can't allocate an internal IOReq for %s", fileName));
      ffPrivate->fftd_ErrorCode = ioReqItem;
      goto cleanup;
    }
    openFile->ofi_InternalIOReq = rawIOReq = (IOReq *) LookupItem(ioReqItem);
  probeForIt:
    DBUG(("Trying a directory probe on %s/%s\n", whereNow->fi.n_Name,fileName));
    rawIOReq->io_Info.ioi_Command = FILECMD_READENTRY;
    rawIOReq->io_Info.ioi_Flags = 0;
    rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) fileName;
    rawIOReq->io_Info.ioi_Send.iob_Len = strlen(fileName) + 1;
    rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &remoteDir;
    rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof remoteDir;
    rawIOReq->io_Info.ioi_Offset = 0;
    errcode = SuperInternalDoIO(rawIOReq);
    if (errcode < 0) {
      DBUG(("Probe failed for %s, error 0x%x\n", fullPath, errcode));
      ffPrivate->fftd_ErrorCode = errcode;
      goto tryAlternate;
    }
    DBUG(("Got '%s'\n", remoteDir.de_FileName));
    whereNow->fi_UseCount ++;
    fileSize = sizeof (File);
    if ((int32) remoteDir.de_AvatarCount > 1) {
      fileSize += sizeof (ulong) * ((int32) remoteDir.de_AvatarCount - 1);
    }
    fileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
				    (void *) NULL, fileSize);
    nextLevel = (File *) LookupItem(fileItem);
    if (!nextLevel) {
      DBUG(("Can't allocate a new File object of %d bytes\n", fileSize));
      if (fileItem < 0) {
	ERR(fileItem);
      }
      ffPrivate->fftd_ErrorCode = fileItem;
      whereNow->fi_UseCount --;
      goto cleanup;
    }
    DBUG(("Path is now %lx\n", path));
    DBUG(("Initializing File at %lx for %s\n", nextLevel,
	  fileName));
    strncpy(nextLevel->fi_FileName, fileName,
	    FILESYSTEM_MAX_NAME_LEN);
    nextLevel->fi_FileSystem = whereNow->fi_FileSystem;
    nextLevel->fi_ParentDirectory = whereNow;
    if (remoteDir.de_UniqueIdentifier == 0) {
      nextLevel->fi_UniqueIdentifier = fileFolio.ff_NextUniqueID --;
    } else {
      nextLevel->fi_UniqueIdentifier = remoteDir.de_UniqueIdentifier;
    }
    DBUG(("Flags 0x%X, bytes %d, blocks %d\n", remoteDir.de_Flags, remoteDir.de_ByteCount, remoteDir.de_BlockCount));
    nextLevel->fi_Flags = remoteDir.de_Flags;
    nextLevel->fi_BlockSize = remoteDir.de_BlockSize;
    nextLevel->fi_BlockCount = remoteDir.de_BlockCount;
    nextLevel->fi_ByteCount = remoteDir.de_ByteCount;
    nextLevel->fi_LastAvatarIndex = remoteDir.de_AvatarCount - 1;
    nextLevel->fi_AvatarList[0] = remoteDir.de_Location; /** dubious **/
    nextLevel->fi_Type = remoteDir.de_Type;
    nextLevel->fi_Date = remoteDir.de_Date;
    if (remoteDir.de_Flags & FILE_HAS_VALID_VERSION) {
      nextLevel->fi_Version = remoteDir.de_Version;
      nextLevel->fi_Revision = remoteDir.de_Revision;
    } /* otherwise leave initialized to zero by CreateSizedItem */
    nextLevel->fi_FileSystemBlocksPerFileBlock = 1;
    /*
       Kick the parent with an OPENENTRY command before adding the file
       to the list.  This gives the parent a chance to do (or start up)
       final opening/initialization of the file, and if necessary to set
       the "not ready yet" flag.
    */
    rawIOReq->io_Info.ioi_Command = FILECMD_OPENENTRY;
    rawIOReq->io_Info.ioi_Flags = 0;
    rawIOReq->io_Info.ioi_Send.iob_Buffer =
      rawIOReq->io_Info.ioi_Recv.iob_Buffer = NULL;
    rawIOReq->io_Info.ioi_Send.iob_Len =
      rawIOReq->io_Info.ioi_Recv.iob_Len =
	rawIOReq->io_Info.ioi_Offset = 0;
    rawIOReq->io_Info.ioi_CmdOptions = fileItem;
    errcode = SuperInternalDoIO(rawIOReq);
    if (errcode < 0) {
      DBUG(("DoIO error %x doing FILECMD_OPENENTRY\n", errcode));
      ffPrivate->fftd_ErrorCode = errcode;
      goto cleanup;
    }
#ifdef	FS_DIRSEMA
    InitDirSema(nextLevel, 1);
#endif	/* FS_DIRSEMA */
    AddHead(&fileFolio.ff_Files, (Node *) nextLevel);
    UNLOCKDIRSEMA(whereNow, "added to cache");
    GiveDaemon(nextLevel);  /* Transfer ownership of all File items! */
    whereNow = nextLevel;
    DBUG(("Changing levels, path is now %lx\n", path));
    (void) SuperCloseItem(openFile->ofi.dev.n_Item);
#ifdef DELETE_CLOSED_DEVICES
    (void) SuperDeleteItem(openFile->ofi.dev.n_Item);
#endif
    buffer = (char *) NULL;
    openFile = (OFile *) NULL;
    DBUG(("Path is now %lx\n", path));
  }
/*
  End of Grand Loop
*/
  if (((mode == CreateNewEntry) || (mode == CreateNewDir))
       && !triedToCreate) {
    ffPrivate->fftd_ErrorCode =
      MakeFErr(ER_SEVER,ER_C_NSTND,ER_FS_DuplicateFile);
    return (File *) NULL;
  }
  if (whereNow) {
    ++ whereNow->fi_UseCount;
    DBUG(("FindFile bumping use count of %s to %d\n",
	   whereNow->fi_FileName, whereNow->fi_UseCount));
  }
  if (punt) {
    -- punt->fi_UseCount;
  }

    SuperFreeMem(expandedPath, expandedPathLen);

  return whereNow;
 tryAlternate:
  DBUG(("Scan failure\n"));
  if (rescan) {
    DBUG(("Try rescanning from %s\n", rescan));
    path = rescan;
    do {
      c = *path++;
      if (c == '\0' || c == '{') {
	goto trueFailure;
      }
    } while (c != '|' && c != '}');
    rescan = path;
    insideAlternative = TRUE;
    DBUG(("Rescan point advanced to %s\n", rescan));
    UNLOCKDIRSEMA(whereNow, "tryAlternate");
    whereNow = punt;
    goto theGrandLoop;
  }
 cleanup:
  if (((mode == CreateNewEntry) || (mode == CreateNewDir)) &&
      *path == '\0' && openFile && rawIOReq &&
      !triedToCreate) {
    DBUG(("Try to create a new entry\n"));
    triedToCreate = TRUE;
    rawIOReq->io_Info.ioi_Command = (mode == CreateNewEntry)?
				     FILECMD_ADDENTRY: FILECMD_ADDDIR;
    rawIOReq->io_Info.ioi_Flags = 0;
    rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) fileName;
    rawIOReq->io_Info.ioi_Send.iob_Len = strlen(fileName) + 1;
    rawIOReq->io_Info.ioi_Recv.iob_Buffer = NULL;
    rawIOReq->io_Info.ioi_Recv.iob_Len = 0;
    rawIOReq->io_Info.ioi_Offset = 0;
    rawIOReq->io_CallBack = FileDriverEndAction;
    errcode = SuperInternalDoIO(rawIOReq);
    if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
      DBUG(("WaitIO error %x adding entry\n", errcode));
      ffPrivate->fftd_ErrorCode = errcode;
      goto trueFailure;
    }
    DBUG(("Entry created, go back and find it in the usual way\n"));
    goto probeForIt;
  }
 trueFailure:
  UNLOCKDIRSEMA(whereNow, "trueFailure");
  if (punt) {
    punt->fi_UseCount --;
  }
  if (openFile) {
    (void) SuperCloseItem(openFile->ofi.dev.n_Item);
#ifdef DELETE_CLOSED_DEVICES
    (void) SuperDeleteItem(openFile->ofi.dev.n_Item);
#endif
  } else {
    if (buffer) {
      SuperFreeMem(buffer, bufferSize);
    }
  }

    SuperFreeMem(expandedPath, expandedPathLen);

  return (File *) NULL;
}

OFile *OpenPath (File *startingDirectory, char *path)
{
  File *resFile;
  OFile *openFile;

  DBUG(("OpenPath '%s' from '%s'\n", path,
	 startingDirectory ? startingDirectory->fi_FileName : "(root)"));
  if (startingDirectory == (File *) NULL) {
    startingDirectory = fileFolio.ff_Root;
  }
  resFile = FindFile(startingDirectory, path, FindExistingEntry);
  DBUG(("OpenPath got '%s'\n",
	 resFile ? resFile->fi_FileName : "(miss)"));

  if (!resFile) {
    return (OFile *) NULL;
  }
  openFile = OpenAFile(resFile);
  resFile->fi_UseCount --;
  DBUG(("OpenPath decremented use count of file %s to %d\n",
	 resFile->fi_FileName, resFile->fi_UseCount));
  DBUG(("Returning open-file 0x%X '%s' use-count %d\n", openFile,
	 openFile->ofi_File->fi_FileName, openFile->ofi_File->fi_UseCount));
  return openFile;
}

int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen)
{
  int32 nameLen, accumLen;
  File *whereNow;
  if (!theFile) {
    return FILE_ERR_BADPTR;
  }
  accumLen = 1;
  whereNow = theFile;
  do {
    accumLen += strlen(whereNow->fi_FileName) + 1;
    whereNow = whereNow->fi_ParentDirectory;
  } while (whereNow != fileFolio.ff_Root);
  if (accumLen > bufLen) {
    return FILE_ERR_BADPTR;
  }
  theBuffer[--accumLen] = '\0';
  whereNow = theFile;
  do {
    nameLen = strlen(whereNow->fi_FileName);
    accumLen -= nameLen;
    strncpy(&theBuffer[accumLen], whereNow->fi_FileName, nameLen);
    theBuffer[--accumLen] = '/';
    whereNow = whereNow->fi_ParentDirectory;
  } while (whereNow != fileFolio.ff_Root);
  if (accumLen != 0) {
    DBUG(("Whoops, internalGetPath offset wrong by %d\n", accumLen));
  }
  return 0;
}

int32
PerformSearch (VersionSearch *search, File *startingPlace, char *partialPath)
{
  File *theCandidate;
  int32 isSuitable, i;
  uint32 compareTag, compareVal, compareTo, compareMask;
  uint32 version, revision, priorVersion, priorRevision;
  uint32 verrev;
  theCandidate = FindFile(startingPlace, partialPath, FindExistingEntry);
  if (!theCandidate) {
    if (search->traceSearch) qprintf(("No candidate found\n"));
    return FALSE;
  }
  isSuitable = TRUE;
  version = theCandidate->fi_Version;
  revision = theCandidate->fi_Revision;
  verrev = (version << 8) + revision;
  if (search->skipNonversioned &&
      !(theCandidate->fi_Flags & FILE_HAS_VALID_VERSION)) {
    if (search->traceSearch) qprintf(("No version number, reject it\n"));
    isSuitable = FALSE;
  } else {
    if (search->traceSearch) qprintf(("Found version %d.%d\n", version, revision));
    for (i = 0 ; i < search->tagCount && isSuitable; i++) {
      compareTag = search->compareTags[i];
      compareVal = (compareTag & VERREV) ? verrev :
	((compareTag & REVISION) ? revision : version);
      compareTo = compareTag & VALUE_MASK;
      if (search->traceSearch)
	qprintf(("Compare %d to %d, flag word is 0x%X\n", compareVal,
		 compareTo, compareTag));
      compareMask = (compareVal < compareTo) ? REJECT_IF_LT :
	((compareVal == compareTo) ? REJECT_IF_EQ : REJECT_IF_GT);
      if (compareMask & compareTag) {
	if (search->traceSearch) qprintf(("Failed comparison, reject it\n"));
	isSuitable = FALSE;
      }
    }
  }
  if (!isSuitable) {
    if (search->traceSearch) qprintf(("Unsuitable candidate, reject\n"));
    theCandidate->fi_UseCount --;
    return FALSE;
  }
  if (search->matchFirst) {
    if (search->traceSearch)
      qprintf(("Matches criteria, find-first, accept it!\n"));
    search->bestFound = theCandidate;
    return TRUE;
  }
  if (!search->bestFound) {
    if (search->traceSearch)
      qprintf(("Matches criteria, it's the first one\n"));
    search->bestFound = theCandidate;
    return FALSE;
  }
  priorVersion = search->bestFound->fi_Version;
  priorRevision = search->bestFound->fi_Revision;
  if (verrev > ((priorVersion << 8) + priorRevision)) {
    if (search->traceSearch)
      qprintf(("Matches, it's better than the previous one\n"));
    search->bestFound->fi_UseCount --;
    search->bestFound = theCandidate;
  } else {
    if (search->traceSearch)
      qprintf(("Matches, it's not better than the previous one, reject it\n"));
    theCandidate->fi_UseCount --;
  }
  return FALSE;
}


/**
|||	AUTODOC -public -class items -name File
|||	A handle to a data file.
|||
|||	  Description
|||
|||	    A file item is created by the File folio when a file is opened. It is a
|||	    private item used to maintain context information about the file being
|||	    accessed.
|||
|||	  Folio
|||
|||	    file
|||
|||	  Item Type
|||
|||	    FILENODE
|||
|||	  Create
|||
|||	    OpenFile(), ChangeDirectory()
|||
|||	  Delete
|||
|||	    CloseFile()
|||
**/

Err internalAcceptDirectory(Item incomingItem, File **result)
{
  Node *theBaseNode;
  FileFolioTaskData *ffPrivate, *taskPrivate;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (!incomingItem) {
    *result = ffPrivate->fftd_CurrentDirectory;
    return 0;
  }
  theBaseNode = (Node *) LookupItem(incomingItem);
  if (!theBaseNode) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
  }
  if (theBaseNode->n_SubsysType == FILEFOLIO &&
      theBaseNode->n_Type == FILENODE) {
    *result = (File *) theBaseNode;
  } else if (theBaseNode->n_SubsysType == FILEFOLIO &&
	     theBaseNode->n_Type == FILESYSTEMNODE) {
    *result = ((FileSystem *) theBaseNode)->fs_RootDirectory;
  } else if (theBaseNode->n_SubsysType == KERNELNODE &&
	    theBaseNode->n_Type == DEVICENODE &&
	    ((Device *) theBaseNode)->dev_Driver == fileDriver) {
    *result = ((OFile *) theBaseNode)->ofi_File;
  } else if (theBaseNode->n_SubsysType == KERNELNODE &&
	    theBaseNode->n_Type == TASKNODE) {
    taskPrivate = (FileFolioTaskData *)
      ((Task *) theBaseNode)->t_FolioData[fileFolio.ff.f_TaskDataIndex];
    if (taskPrivate) {
      *result = taskPrivate->fftd_CurrentDirectory;
    } else {
      DBUG(("Item 0x%X is a task with no File Folio data\n", incomingItem));
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
    }
  } else {
    DBUG(("Item 0x%X does not identify a directory\n", incomingItem));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
  }
  if (((**result).fi_Flags & FILE_IS_DIRECTORY) == 0) {
    DBUG(("%s is a file, not a directory\n", (**result).fi_FileName));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
  }
  return 0;
}


/**********************
 SWI handler for OFile call
 **********************/

Item OpenFileSWI(char *thePath)
{
  OFile *theOpenFile;
  FileFolioTaskData *ffPrivate;
  DBUG(("OFile %s\n", thePath));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  theOpenFile = OpenPath(ffPrivate->fftd_CurrentDirectory, thePath);
  if (theOpenFile) {
    return theOpenFile->ofi.dev.n_Item;
  } else {
    DBUG(("File not opened, error code %x\n", ffPrivate->fftd_ErrorCode));
    return (Item) ffPrivate->fftd_ErrorCode;
  }
}

/**********************
 SWI handler for OpenFileInDir call
 **********************/

Item OpenFileInDirSWI(Item baseDirItem, char *thePath)
{
  File *theBaseDir;
  OFile *theOpenFile;
  Err err;
  err = internalAcceptDirectory(baseDirItem, &theBaseDir);
  if (err < 0) {
    return err;
  }
  DBUG(("OpenFileInDirSWI, base path %s, path %s\n", theBaseDir->fi_FileName, thePath));
  theOpenFile = OpenPath(theBaseDir, thePath);
  if (theOpenFile) {
    return theOpenFile->ofi.dev.n_Item;
  } else {
    DBUG(("File not opened, error code ?\n"));
    return (Item) SetupFileFolioTaskData()->fftd_ErrorCode;
  }
}

File *FileSearch(char *partialPath, TagArg *tags)
{
  File *searchPoint;
  FileSystem *fs;
  FileFolioTaskData *ffPrivate;
  VersionSearch search;
  TagArg *thisTag;
  int32 halt;
  Err err;
  uint32 exclusion;
  DBUG(("FileSearch %s\n", partialPath));
  ffPrivate = SetupFileFolioTaskData();
  ffPrivate->fftd_ErrorCode = 0;
  memset (&search, 0, sizeof search);
  halt = FALSE;
  while (!halt && (thisTag = NextTagArg(&tags)) != NULL) {
    switch (thisTag->ta_Tag) {
    case FILESEARCH_TAG_VERSION_EQ:
    case FILESEARCH_TAG_VERSION_NE:
    case FILESEARCH_TAG_VERSION_LT:
    case FILESEARCH_TAG_VERSION_GT:
    case FILESEARCH_TAG_VERSION_LE:
    case FILESEARCH_TAG_VERSION_GE:
    case FILESEARCH_TAG_REVISION_EQ:
    case FILESEARCH_TAG_REVISION_NE:
    case FILESEARCH_TAG_REVISION_LT:
    case FILESEARCH_TAG_REVISION_GT:
    case FILESEARCH_TAG_REVISION_LE:
    case FILESEARCH_TAG_REVISION_GE:
      if (search.tagCount >= MAX_SEARCH_TAGS) {
	ffPrivate->fftd_ErrorCode = FILE_ERR_PARAMERROR;
	return NULL;
      }
      search.compareTags[search.tagCount] = SearchParameters[thisTag->ta_Tag] +
	(VALUE_MASK & (int) thisTag->ta_Arg);
      if (search.traceSearch)
	qprintf(("Set compare tag %d to 0x%X\n", search.tagCount,
		 search.compareTags[search.tagCount]));
      search.tagCount ++;
      break;
    case FILESEARCH_TAG_FIND_FIRST_MATCH:
      if (search.traceSearch) qprintf(("Find first match\n"));
      search.matchFirst = TRUE;
      break;
    case FILESEARCH_TAG_FIND_HIGHEST_MATCH:
      if (search.traceSearch) qprintf(("Find best match\n"));
      search.matchFirst = FALSE;
      break;
    case FILESEARCH_TAG_NOVERSION_IS_0_0:
      if (search.traceSearch) qprintf(("Treat non-versioned files as 0.0\n"));
      search.skipNonversioned = FALSE;
      break;
    case FILESEARCH_TAG_NOVERSION_IGNORED:
      if (search.traceSearch) qprintf(("Skip nonversioned files\n"));
      search.skipNonversioned = TRUE;
      break;
    case FILESEARCH_TAG_SEARCH_PATH:
      if (search.traceSearch)
	qprintf(("Search within path '%s'\n", (char *) thisTag->ta_Arg));
      searchPoint = FindFile(ffPrivate->fftd_CurrentDirectory,
			     (char *) thisTag->ta_Arg, FindExistingEntry);
      if (searchPoint) {
	halt = PerformSearch (&search, searchPoint, partialPath);
	searchPoint->fi_UseCount --;
      }
      break;
    case FILESEARCH_TAG_SEARCH_FILESYSTEMS:
      if (search.traceSearch) qprintf(("Search all filesystems\n"));
      (void) SuperInternalLockSemaphore(fsListSemaphore,
					SEM_WAIT + SEM_SHAREDREAD);
      for (fs = (FileSystem *) FirstNode(&fileFolio.ff_Filesystems);
	   !halt && IsNode(&fileFolio.ff_Filesystems, fs);
	   fs = (FileSystem *) NextNode((Node *) fs)) {
	exclusion = 0;
	if (fs->fs_Flags & FILESYSTEM_IS_QUIESCENT){
	  exclusion |= DONT_SEARCH_QUIESCENT;
	}
	if (fs->fs_Type &&
	    (fs->fs_Type->fst.n_Flags & (FSTYPE_NEEDS_LOAD |
					 FSTYPE_IS_LOADING))) {
	  exclusion |= DONT_SEARCH_NO_CODE;
	}
	if (fs->fs_RootDirectory->fi_Flags & FILE_USER_STORAGE_PLACE) {
	  exclusion |= DONT_SEARCH_USER_STORAGE;
	}
	if (!(fs->fs_VolumeFlags & VF_BLESSED)) {
	  exclusion |= DONT_SEARCH_UNBLESSED;
	}
	exclusion &= (uint32) thisTag->ta_Arg;
	if (exclusion) {
	  if (search.traceSearch)
	    qprintf(("Don't search /%s, exclusion 0x%X\n",
		     fs->fs_FileSystemName, exclusion));
	} else {
	  if (search.traceSearch)
	    qprintf(("Search /%s\n", fs->fs_FileSystemName));
	  fs->fs_RootDirectory->fi_UseCount ++;
	  halt = PerformSearch (&search, fs->fs_RootDirectory, partialPath);
	  fs->fs_RootDirectory->fi_UseCount --;
	}
      }
      (void) SuperInternalUnlockSemaphore(fsListSemaphore);
      break;
    case FILESEARCH_TAG_SEARCH_ITEM:
      if (search.traceSearch)
	qprintf(("Search within item 0x%X\n", (char *) thisTag->ta_Arg));
      err = internalAcceptDirectory((Item) thisTag->ta_Arg, &searchPoint);
      if (err >= 0) {
	searchPoint->fi_UseCount ++;
	halt = PerformSearch (&search, searchPoint, partialPath);
	searchPoint->fi_UseCount --;
      } else {
	ffPrivate->fftd_ErrorCode = err;
      }
      break;
    case FILESEARCH_TAG_TRACE_SEARCH:
      search.traceSearch = (uint8) thisTag->ta_Arg;
      break;
    }
    if (halt) {
      if (search.traceSearch) qprintf(("Halt search!\n"));
    }
  }
  if (search.traceSearch) qprintf(("Search finished\n"));
  return search.bestFound;
}

/**********************
 SWI handler for FindFileAndOpen
 **********************/

Item FindFileAndOpenSWI(char *partialPath, TagArg *tags)
{
  File *found;
  FileFolioTaskData *ffPrivate;
  OFile *theOpenFile;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  found = FileSearch(partialPath, tags);
  if (found) {
    theOpenFile = OpenAFile(found);
    found->fi_UseCount --;
  } else {
    theOpenFile = NULL;
  }
  if (theOpenFile) {
    return theOpenFile->ofi.dev.n_Item;
  } else if (ffPrivate->fftd_ErrorCode) {
    DBUG(("File not found, error code %x\n", ffPrivate->fftd_ErrorCode));
    return (Item) ffPrivate->fftd_ErrorCode;
  } else {
    return FILE_ERR_NOFILE;
  }
}

/**********************
 SWI handler for FindFileAndIdentify
 **********************/

Err internalFindFileAndIdentify(char *returnedPath, uint32 returnedPathLength,
				char *partialPath, TagArg *tags,
				int32 checkBuffer)
{
  File *found;
  FileFolioTaskData *ffPrivate;
  Err err;
  DBUG(("FindFileAndIdentify '%s'\n", partialPath));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (checkBuffer && !IsPriv(CURRENTTASK) && !IsMemWritable(returnedPath, returnedPathLength)) {
    return FILE_ERR_BADPTR;
  }
  memset(returnedPath, 0, returnedPathLength);
  found = FileSearch(partialPath, tags);
  if (found) {
    returnedPath[0] = '/';
    returnedPath[1] = '^';
    err = internalGetPath(found, returnedPath+2, returnedPathLength-2);
    found->fi_UseCount --;
    return err;
  } else if (ffPrivate->fftd_ErrorCode) {
    DBUG(("File not found, error code %x\n", ffPrivate->fftd_ErrorCode));
    return (Item) ffPrivate->fftd_ErrorCode;
  } else {
    return FILE_ERR_NOFILE;
  }
}

Err FindFileAndIdentifySWI(char *returnedPath, uint32 returnedPathLength,
			   char *partialPath, TagArg *tags)
{
  return internalFindFileAndIdentify(returnedPath, returnedPathLength,
				     partialPath, tags, TRUE);
}

Err SuperFindFileAndIdentify(char *returnedPath, uint32 returnedPathLength,
				char *partialPath, TagArg *tags)
{
  return internalFindFileAndIdentify(returnedPath, returnedPathLength,
				     partialPath, tags, FALSE);
}


/**********************
 SWI handler for CloseFile call
 **********************/

int32 CloseOpenFileSWI(Item openFileItem)
{
  OFile *theOpenFile;
#ifdef BUILD_PARANOIA
  Err err;
#endif
  SetupFileFolioTaskData();
  theOpenFile = (OFile *) CheckItem(openFileItem, KERNELNODE, DEVICENODE);
  if (!theOpenFile ||
      theOpenFile->ofi.dev_Driver != fileDriver) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
  }
#ifdef BUILD_PARANOIA
  err = SuperCloseItem(theOpenFile->ofi.dev.n_Item);
  DBUG(("SuperCloseItem returns 0x%x\n", err));
  if (err < 0) {
    DBUG(("SuperCloseItem failed in CloseOpenFile!\n"));
    ERR(err);
  }
#ifdef DELETE_CLOSED_DEVICES
  err = SuperDeleteItem(theOpenFile->ofi.dev.n_Item);
  DBUG(("SuperDeleteItem returns 0x%x\n", err));
  if (err < 0) {
    DBUG(("SuperDeleteItem failed in CloseOpenFile!\n"));
    ERR(err);
  }
#endif
  return err;
#else
#ifdef DELETE_CLOSED_DEVICES
  (void) SuperCloseItem(theOpenFile->ofi.dev.n_Item);
  return SuperDeleteItem(theOpenFile->ofi.dev.n_Item);
#else
  return SuperCloseItem(theOpenFile->ofi.dev.n_Item);
#endif
#endif
}

Item internalChangeDirectory(File *startingDir, char *path)
{
  File *theDirectory;
  FileFolioTaskData *ffPrivate;
#ifdef WATCHDIRECTORY
  char foo[FILESYSTEM_MAX_PATH_LEN];
#endif
  DBUG(("ChangeDirectory to %s\n", path));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  theDirectory = FindFile(startingDir, path, UpdateCurrentDirectory);
  if (theDirectory) {
    if (!(theDirectory->fi_Flags & FILE_IS_DIRECTORY)) {
      theDirectory->fi_UseCount --;
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
    }
    ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
    ffPrivate->fftd_CurrentDirectory = theDirectory;
#ifdef WATCHDIRECTORY
    (void) internalGetPath(ffPrivate->fftd_CurrentDirectory, foo, FILESYSTEM_MAX_PATH_LEN);
    DBUG(("Task 0x%x directory changed to %s\n", CURRENTTASKITEM, foo));
#endif
    return theDirectory->fi.n_Item;
  } else {
    DBUG(("Cannot ChangeDirectory to %s\n", path));
    return (Item) ffPrivate->fftd_ErrorCode;
  }
}

/**********************
 SWI handler for ChangeDirectory SWI
 **********************/

Item ChangeDirectorySWI(char *path)
{
  FileFolioTaskData *ffPrivate;
  DBUG(("ChangeDirectory to %s\n", path));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  return internalChangeDirectory(ffPrivate->fftd_CurrentDirectory, path);
}

/**********************
 SWI handler for ChangeDirectoryInDir SWI
 **********************/

Item ChangeDirectoryInDirSWI(Item startingDirItem, char *path)
{
  File *startingDirectory;
  Err err;
  err = internalAcceptDirectory(startingDirItem, &startingDirectory);
  if (err < 0) {
    return err;
  }
  return internalChangeDirectory(startingDirectory, path);
}

/**********************
 SWI handler for GetDirectory SWI
 **********************/

Item GetDirectorySWI(char *path, int32 pathLen)
{
  FileFolioTaskData *ffPrivate;
  int32 err;
  ffPrivate = SetupFileFolioTaskData();
  DBUG(("In GetDirectory, path buffer at 0x%X, length %d\n", path, pathLen));
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (path) {
    if (!IsMemWritable(path, pathLen))
      return FILE_ERR_BADPTR;

    err = internalGetPath(ffPrivate->fftd_CurrentDirectory, path, pathLen);
    if (err < 0) {
      return err;
    }
  }
  DBUG(("GetDirectorySWI: path [%s]\n",
	path));
  return ffPrivate->fftd_CurrentDirectory->fi.n_Item;
}

/*
 * common code for creating files/directories
 */
Item
internalCreateEntry(File *startingDirectory, char *path, enum FindMode mode)
{
  File *whereNow;
  FileFolioTaskData *ffPrivate;

  DBUG(("In internalCreateEntry\n"));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  whereNow = FindFile(startingDirectory, path, mode);
  if (!whereNow) {
    return ffPrivate->fftd_ErrorCode;
  }
  whereNow->fi_UseCount --;
  DBUG(("internalCreateEntry decremented use count of file %s to %d\n",
	 whereNow->fi_FileName, whereNow->fi_UseCount));
  return whereNow->fi.n_Item;
}

/**********************
 SWI handler for CreateFile SWI
 **********************/
Item
CreateFileSWI(char *path)
{
  FileFolioTaskData *ffPrivate;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  return internalCreateEntry(ffPrivate->fftd_CurrentDirectory, path,
			     CreateNewEntry);
}


/**********************
 SWI handler for Rename SWI
 **********************/
Err
RenameSWI(char *path, char *fnam)
{
  File			*srcfp, *dstfp;
  FileFolioTaskData	*ffPrivate;
  char			buf[FILESYSTEM_MAX_PATH_LEN], *cp;
  FileSystem		*fsp;

  RBG(("RenameSWI: [%s] to [%s]\n", path, fnam));

  if(badname(fnam))
    return FILE_ERR_BADNAME;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return NOMEM;
  }

  RBG(("RenameSWI: First looking for [%s]\n", path));
  srcfp = FindFile(ffPrivate->fftd_CurrentDirectory, path, FindExistingEntry);
  if (!srcfp)
    return ffPrivate->fftd_ErrorCode;

  RBG(("RenameSWI: Found existing entry for %s, use count is %d\n",
	srcfp->fi_FileName, srcfp->fi_UseCount));
  fsp = srcfp->fi_FileSystem;
  if (fsp->fs_Flags & FILESYSTEM_IS_READONLY) {
    RBG(("RenameSWI: Can't Rename, read-only directory\n"));
    ffPrivate->fftd_ErrorCode =	FILE_ERR_READONLY;
    goto err_out;
  }

  strncpy(buf, path, FILESYSTEM_MAX_PATH_LEN);
  if ((cp = strrchr(buf, '/')) == NULL)
    *buf = '\0';
  else
    *(cp + 1) = '\0';
  strcat(buf, fnam);
  RBG(("RenameSWI: Second looking for [%s]\n", buf));
  dstfp = FindFile(ffPrivate->fftd_CurrentDirectory, buf, FindExistingEntry);
  if (dstfp) {
    dstfp->fi_UseCount--;
    ffPrivate->fftd_ErrorCode =	FILE_ERR_DUPLICATEFILE;
    goto err_out;
  }

  RBG(("RenameSWI: Starting the process of renaming [%s] to [%s]\n",
       path, fnam));

  if ((int32) (ffPrivate->fftd_ErrorCode = internalRename(srcfp, fnam)) < 0)
    goto err_out;

  RBG(("RenameSWI: internalRename was successful with 0x%x use %d\n",
       ffPrivate->fftd_ErrorCode, srcfp->fi_UseCount));
  srcfp->fi_UseCount--;
  return 0;

err_out:
  RBG(("RenameSWI: err_out with 0x%x, use %d\n", ffPrivate->fftd_ErrorCode,
        srcfp->fi_UseCount));
  srcfp->fi_UseCount--;
  return ffPrivate->fftd_ErrorCode;
}


static Err
badname(char *fname)
{
  for(; *fname; fname++) {
    if ((*fname == '/') || (*fname == '|'))
	return FILE_ERR_BADNAME;
  }
  return 0;
}


/*
 *	all the good stuff happen here
 */
Err
internalRename(File *fp, char *fname)
{
  Err		err = 0;
  OFile		*ofp;
  TagArg 	ioReqTags[2];
  Item 	 	ioReqItem;
  IOReq 	*rawIOReq;
  FileSystem	*fsp;

  TOUCH(err);
  RBG(("internalRename: renaming [%s] to [%s]\n", fp->fi_FileName, fname));

  ofp = OpenAFile(fp);
  if (!ofp) {
    RBG(("internalRename: failed to openAfile ofp [%s] child [%s]\n",
         fp->fi_ParentDirectory->fi_FileName, fp->fi_FileName));
    return FILE_ERR_NOFILE;
  }

  RBG(("internalRename: Openfile [%s], use %d\n", fp->fi_FileName,
       fp->fi_UseCount));
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) ofp->ofi.dev.n_Item;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    err = ioReqItem;
    goto err_out;
  }
  rawIOReq = (IOReq *) LookupItem(ioReqItem);
  rawIOReq->io_Info.ioi_Command = FILECMD_RENAME;
  rawIOReq->io_Info.ioi_Flags = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) fname;
  rawIOReq->io_Info.ioi_Send.iob_Len = strlen(fname)+1;
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) fp;
  rawIOReq->io_Info.ioi_Recv.iob_Len = 0;
  rawIOReq->io_Info.ioi_Offset = 0;
  rawIOReq->io_CallBack = FileDriverEndAction;
  if ((err = SuperInternalDoIO(rawIOReq)) < 0) {
    RBG(("internalRename: WaitIO error 0x%x\n", err));
    goto cleanup;
  }

  fsp = fp->fi_FileSystem;
  if (fp == fsp->fs_RootDirectory) {
    RBG(("internalRename: Renaming root directory [%s], [%s]\n",
	 fp->fi_FileName, fsp->fs_RootDirectory->fi_FileName));
    strncpy(fsp->fs_RootDirectory->fi_FileName, fname,
		FILESYSTEM_MAX_NAME_LEN);
    strncpy(fsp->fs_FileSystemName, fname, FILESYSTEM_MAX_NAME_LEN);
  }

cleanup:
  SuperDeleteItem(ioReqItem);
err_out:
  RBG(("internalRename: ending with [%s], use %d\n", fp->fi_FileName,
       fp->fi_UseCount));
  SuperCloseItem(ofp->ofi.dev.n_Item);
#ifdef DELETE_CLOSED_DEVICES
  SuperDeleteItem(ofp->ofi.dev.n_Item);	/* use count decremented here */
#endif
  return err;
}



/**********************
 SWI handler for CreateFileInDir SWI
 **********************/
Item
CreateFileInDirSWI(Item startingDirItem, char *path)
{
  File *startingDir;
  Err err;
  err = internalAcceptDirectory(startingDirItem, &startingDir);
  if (err < 0) {
    return err;
  }
  return internalCreateEntry(startingDir, path, CreateNewEntry);
}


/**********************
 SWI handler for CreateDirectory SWI
 **********************/
Item
CreateDirSWI(char *path)
{
  FileFolioTaskData *ffPrivate;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  return internalCreateEntry(ffPrivate->fftd_CurrentDirectory,
			     path, CreateNewDir);
}

/**********************
 SWI handler for CreateDirectoryInDir SWI
 **********************/
Item
CreateDirInDirSWI(Item startingDirItem, char *path)
{
  File *startingDir;
  Err err;
  err = internalAcceptDirectory(startingDirItem, &startingDir);
  if (err < 0) {
    return err;
  }
  return internalCreateEntry(startingDir, path, CreateNewDir);
}


/**
|||	AUTODOC -private -class Items -name Alias
|||	A character string for referencing the pathname to a file or a
|||	directory of files.
|||
|||	  Description
|||
|||	    A file alias is a character string for referencing the pathname to a file
|||	    or directory of files on disk. Whenever the File folio parses pathnames,
|||	    it looks for path components starting with $, and treats the rest of the
|||	    component at the name of an alias. The file folio then finds the alias of
|||	    that name, and extracts a replacement string from the alias to use as the
|||	    path component.
|||
|||	  Folio
|||
|||	    file
|||
|||	  Item Type
|||
|||	    FILEALIASNODE
|||
|||	  Create
|||
|||	    CreateAlias()
|||
|||	  Delete
|||
|||	    DeleteItem()
|||
**/

/**********************
 SWI handler for CreateAlias SWI
 **********************/

Item CreateAliasSWI(char *aliasPath, char *originalPath)
{
  FileFolioTaskData *ffPrivate;
  TagArg aliasArgs[3];
  Item aliasItem;
  Alias *alias, *oldAlias;
  int32 interrupts;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (!aliasPath || !originalPath ||
      strlen(aliasPath) > ALIAS_NAME_MAX_LEN ||
      strlen(originalPath) > ALIAS_VALUE_MAX_LEN) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadName);
  }
  memcpy(aliasArgs, createAliasArgs, sizeof aliasArgs);
  aliasArgs[0].ta_Arg = aliasPath;
  aliasItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILEALIASNODE),
				   aliasArgs,
				   sizeof (Alias) + strlen(originalPath) + 1);
  alias = (Alias *) LookupItem(aliasItem);
  if (!alias) {
    return aliasItem;
  }
  alias->a_Value = sizeof alias->a_Value + (char *) &alias->a_Value;
  strcpy(alias->a_Value, originalPath);
  oldAlias = (Alias *) FindNamedNode(&fileFolio.ff_Aliases, aliasPath);
  if (oldAlias) {
    SuperInternalDeleteItem(oldAlias->a.n_Item); /* this does a RemNode */
  }
  interrupts = Disable();
  AddTail(&fileFolio.ff_Aliases, (Node *) alias);
  Enable(interrupts);
  GiveDaemon(alias);
  DBUG(("Added alias %s\n", alias->a.n_Name));
  return aliasItem;
}


/*
 * common code for deleting files/directories
 */
Err
internalDeleteEntry(File *startingDir, char *path, uint32 cmd)
{
  File *whereNow;
  OFile *parent;
  IOReq *rawIOReq;
  Item ioReqItem;
  FileFolioTaskData *ffPrivate;
  TagArg ioReqTags[2];
  Err errcode;

  DBUG(("In internalDeleteEntry\n"));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (startingDir->fi_FileSystem) {
    while (TrimZeroUseFiles(startingDir->fi_FileSystem, 0) > 0)
      continue;
  }
  whereNow = FindFile(startingDir, path, DeleteExistingEntry);
  if (!whereNow) {
    return ffPrivate->fftd_ErrorCode;
  }
  DBUG(("Found existing entry for %s, use count is %d\n", whereNow->fi_FileName, whereNow->fi_UseCount));
  if (whereNow->fi_UseCount > 1) {
    DBUG(("Can't delete, use count is too high\n"));
    ffPrivate->fftd_ErrorCode =	MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
    goto bail;
  }
  if (whereNow->fi_ParentDirectory->fi_Flags & FILE_IS_READONLY) {
    DBUG(("Can't delete, read-only directory\n"));
    ffPrivate->fftd_ErrorCode =	MakeFErr(ER_SEVER,ER_C_NSTND,ER_FS_ReadOnly);
    goto bail;
  }
  parent = OpenAFile(whereNow->fi_ParentDirectory);
  if (!parent) {
    goto bail;
  }
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) parent->ofi.dev.n_Item;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    (void) SuperCloseItem(parent->ofi.dev.n_Item);
#ifdef DELETE_CLOSED_DEVICES
    (void) SuperDeleteItem(parent->ofi.dev.n_Item);
#endif
    ffPrivate->fftd_ErrorCode = ioReqItem;
    goto bail;
  }
  rawIOReq = (IOReq *) LookupItem(ioReqItem);
  rawIOReq->io_Info.ioi_Command = cmd;
  rawIOReq->io_Info.ioi_Flags = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) whereNow->fi_FileName;
  rawIOReq->io_Info.ioi_Send.iob_Len = strlen(whereNow->fi_FileName)+1;
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) whereNow;
  rawIOReq->io_Info.ioi_Recv.iob_Len = 0;
  rawIOReq->io_Info.ioi_Offset = 0;
  rawIOReq->io_CallBack = FileDriverEndAction;
  errcode = SuperInternalDoIO(rawIOReq);
  if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
    DBUG(("WaitIO error %x deleting entry\n", errcode));
    ffPrivate->fftd_ErrorCode = errcode;
    whereNow->fi_UseCount --;
    DBUG(("internalDeleteItem decremented use count of file %s to %d\n",
	   whereNow->fi_FileName, whereNow->fi_UseCount));
    goto wrapup;
  }
  whereNow->fi_UseCount --;
  DBUG(("internalDeleteItem decremented use count of file %s to %d\n",
	 whereNow->fi_FileName, whereNow->fi_UseCount));
  SuperCloseItem(whereNow->fi.n_Item);
  SuperInternalDeleteItem(whereNow->fi.n_Item);
  ffPrivate->fftd_ErrorCode = 0;
 wrapup:
  SuperDeleteItem(ioReqItem);
  SuperCloseItem(parent->ofi.dev.n_Item);
  SuperDeleteItem(parent->ofi.dev.n_Item);
  return ffPrivate->fftd_ErrorCode;
 bail:
  whereNow->fi_UseCount --;
  DBUG(("internalDeleteItem decremented use count of file %s to %d\n",
	 whereNow->fi_FileName, whereNow->fi_UseCount));
  return ffPrivate->fftd_ErrorCode;
}

/**********************
 SWI handler for DeleteFile SWI
 **********************/
Err
DeleteFileSWI(char *path)
{
  FileFolioTaskData *ffPrivate;

  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  return internalDeleteEntry(ffPrivate->fftd_CurrentDirectory, path,
			     FILECMD_DELETEENTRY);
}


/**********************
 SWI handler for DeleteFileInDir SWI
 **********************/
Err
DeleteFileInDirSWI(Item startingDirItem, char *path)
{
  File *startingDir;
  Err err;
  err = internalAcceptDirectory(startingDirItem, &startingDir);
  if (err < 0) {
    return err;
  }
  return internalDeleteEntry(startingDir, path, FILECMD_DELETEENTRY);
}


/**********************
 SWI handler for DeleteDirSWI
 **********************/
Err
DeleteDirSWI(char *path)
{
  FileFolioTaskData *ffPrivate;

  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  return internalDeleteEntry(ffPrivate->fftd_CurrentDirectory,
			     path, FILECMD_DELETEDIR);
}

/**********************
 SWI handler for DeleteDirInDir SWI
 **********************/
Err
DeleteDirInDirSWI(Item startingDirItem, char *path)
{
  File *startingDir;
  Err err;
  err = internalAcceptDirectory(startingDirItem, &startingDir);
  if (err < 0) {
    return err;
  }
  return internalDeleteEntry(startingDir, path, FILECMD_DELETEDIR);
}

/**********************
 SWI handler for MinimizeFileSystem SWI
 **********************/
void
MinimizeFileSystemSWI(uint32 minimizationFlags)
{
  FileSystem *fs;
  FileSystemType *fst;
  Err err;
  DBUG(("MinimizeFileSystemSWI (%s): 0x%x\n", CURRENTTASK->t.n_Name,
	 minimizationFlags));
  if (minimizationFlags & FSMINIMIZE_FLUSH_FILES) {
    TrimZeroUseFiles(NULL, 0);
  }
  if (minimizationFlags & (FSMINIMIZE_QUIESCE |
			   FSMINIMIZE_UNLOAD  |
			   FSMINIMIZE_DISMOUNT)) {
    err = SuperInternalLockSemaphore(fsListSemaphore,
				     SEM_WAIT + SEM_SHAREDREAD);
    if (err < 0) return;
    ScanList(&fileFolio.ff_Filesystems, fs, FileSystem) {
      fst = fs->fs_Type;
      if (fst->fst.n_Flags & FSTYPE_AUTO_QUIESCE) {
	fs->fs_Flags |= FILESYSTEM_WANTS_QUIESCENT;
      }
      if (minimizationFlags & (FSMINIMIZE_UNLOAD | FSMINIMIZE_DISMOUNT)) {
	if (fst->fst.n_Flags & FSTYPE_UNLOADABLE) {
	  fst->fst.n_Flags |= FSTYPE_WANTS_UNLOAD;
	}
      }
      if (minimizationFlags & FSMINIMIZE_DISMOUNT) {
	fs->fs_Flags |= FILESYSTEM_WANTS_DISMOUNT;
      }
    }
    (void) SuperInternalUnlockSemaphore(fsListSemaphore);
    SuperinternalSignal(fileFolio.ff_Daemon.ffd_Task,
			fileFolio.ff_Daemon.ffd_HeartbeatSignal);
  }
  DBUG(("MinimizeFileSystemSWI (%s): OUT OF HERE\n", CURRENTTASK->t.n_Name));
/*
   Add other minimization actions here
*/
}

IOReq *
FileDriverEndAction(IOReq *theRequest)
{
  TOUCH(theRequest);
  qprintf(("FileDriverEndAction: Catastrophe, Pigs Are DEAD\n"));
  return NULL;
}


#ifdef	FS_DIRSEMA
TagArg dstags[2] = {
  TAG_ITEM_NAME, NULL,
  TAG_END,	0,
};

void
InitDirSema(File *fp, int setowner)
{
  Err	err;

  if (!fp) {
    SEMDBUG(("InitDirSema: NULL fp\n"));
    return;
  }
  if (!(fp->fi_Flags & FILE_IS_DIRECTORY)) {
    DBUG(("InitDirSema: not a dir [%s]\n", fp->fi_FileName));
    return;
  }
  dstags[0].ta_Arg = fp->fi_FileName;
  fp->fi_DirSema =
    SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), dstags);

  if (fp->fi_DirSema < 0) {
    SEMDBUG(("InitDirSema: Failed to Create sema for [%s] Sema: 0x%x, Task: 0x%x\n", fp->fi_FileName, fp->fi_DirSema, CURRENTTASKITEM));
    return;
  }

  if (setowner) {
    err = SuperSetItemOwner(fp->fi_DirSema,
			    fileFolio.ff_Daemon.ffd_Task->t.n_Item);
    if (err < 0) {
      SEMDBUG(("InitDirSema: Failed to set sema owner for [%s] at 0x%x, owner: 0x%x, ", fp->fi_FileName, fp->fi_DirSema, CURRENTTASKITEM));
      SEMDBUG(("newowner: 0x%x, err: 0x%x\n", fileFolio.ff_Daemon.ffd_Task->t.n_Item, err));
      return;
    }
    DBUG(("InitDirSema (flg: 0x%x): Created sema (setowner 0x%x) for [%s] ", fp->fi_Flags, fileFolio.ff_Daemon.ffd_Task->t.n_Item, fp->fi_FileName));
    DBUG(("at 0x%x, Task: 0x%x\n", fp->fi_DirSema, CURRENTTASKITEM));
  } else {
    DBUG(("InitDirSema: Created sema (NO setowner) for [%s] at 0x%x, Task: 0x%x\n", fp->fi_FileName, fp->fi_DirSema, CURRENTTASKITEM));
  }
  return;
}


void
DelDirSema(File *fp)
{
  Item	ret;
#if	(defined(DEBUG) || defined(SEMDEBUG))
  ItemNode *n = (ItemNode *)LookupItem(fp->fi_DirSema);
#endif /* DEBUG || SEMDEBUG */

  if (!fp) {
    SEMDBUG(("DelDirSema: NULL fp\n"));
    return;
  }
  if (!(fp->fi_Flags & FILE_IS_DIRECTORY)) {
    DBUG(("DelDirSema: not a dir [%s], [0x%x]\n", fp->fi_FileName, fp->fi_Flags));
    return;
  }

  if ((ret = SuperInternalDeleteItem(fp->fi_DirSema)) < 0) {
    TOUCH(ret);
    SEMDBUG(("DelDirSema: Failed to Delete sema for [%s] at 0x%x, myTask: 0x%x, ", fp->fi_FileName, fp->fi_DirSema, CURRENTTASKITEM));
    SEMDBUG(("OwnerTask: 0x%x, ret: 0x%x\n", n->n_Owner, ret));
    return;
  }
  DBUG(("DelDirSema (flg:0x%x): Deleted sema for [%s] at 0x%x, ", fp->fi_Flags, fp->fi_FileName, fp->fi_DirSema));
  DBUG(("myTask: 0x%x, OwnerTask: 0x%x, ret: 0x%x\n", CURRENTTASKITEM, n->n_Owner, ret));
  fp->fi_DirSema = (Item) NULL;
  return;
}


void
LockDirSema(File *fp, char *msg)
{
  Err err;

  if (!fp) {
    SEMDBUG(("LockDirSema (%s): NULL fp\n", msg));
    return;
  }
  if (fp->fi_Flags & FILE_IS_DIRECTORY) {
    if (fp->fi_FileSystem) {
      if (fp->fi_FileSystem->fs_Flags & FILESYSTEM_IS_INTRANS)
	return;
    }
    DBUG(("LockDirSema (%s): TRYING TO LOCK dir [%s]\n", CURRENTTASK->t.n_Name,
	  fp->fi_FileName));
    if ((err = SuperLockSemaphore(fp->fi_DirSema, SEM_WAIT)) != 1) {
      TOUCH(err);
      TOUCH(msg);
      SEMDBUG(("FAILED TO LOCK (%s): [%s][0x%x]", msg, fp->fi_FileName, fp->fi_DirSema));
      SEMDBUG(("[0x%x], err: 0x%x\n", CURRENTTASKITEM, err));
    } else {
      DBUG(("LOCKED (%s) [0x%x]: [%s]\n", msg, CURRENTTASKITEM, fp->fi_FileName));
    }
  }
}


void
UnlockDirSema(File *fp, char *msg)
{
  Err err;

  if (!fp) {
    SEMDBUG(("UnlockDirSema (%s): NULL fp\n", msg));
    return;
  }
  if (fp->fi_Flags & FILE_IS_DIRECTORY) {
    if (fp->fi_FileSystem) {
      if (fp->fi_FileSystem->fs_Flags & FILESYSTEM_IS_INTRANS)
	return;
    }
    if ((err = SuperUnlockSemaphore(fp->fi_DirSema)) != 0) {
      TOUCH(err);
      TOUCH(msg);
      SEMDBUG(("FAILED TO UNLOCK (%s): [%s][0x%x]", msg, fp->fi_FileName, fp->fi_DirSema));
      SEMDBUG(("[0x%x], err: 0x%x\n", CURRENTTASKITEM, err));
    } else {
      DBUG(("UNLOCKED (%s) [0x%x]: [%s]\n", msg, CURRENTTASKITEM, fp->fi_FileName));
    }
  }
}
#endif	/* FS_DIRSEMA */
