/* @(#) PlugNPlay.c 96/07/29 1.11 */

/* #define DEBUG */

#define SUPER

#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <kernel/sysinfo.h>
#include <kernel/ddfnode.h>
#include <kernel/ddfuncs.h>
#include <kernel/dipir.h>
#include <file/directoryfunctions.h>
#include <file/filesystem.h>
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <file/filesystemdefs.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>


/* Simple IFF handling macros. */

typedef struct IFFHeader
{
	PackedID	iff_FormID;
	uint32		iff_FormLength;
	uint32		iff_FormType;
} IFFHeader;

#define	IFF_HEADER_LENGTH	sizeof(IFFHeader)
#define	IFF_CHUNK_TYPE(p)	(((IFFHeader*)(p))->iff_ChunkType)
#define IFF_CHUNK_LENGTH(p)	(((IFFHeader*)(p))->iff_ChunkLength)
#define IFF_CHUNK_DATA(p)	((uint8*)(p) + IFF_HEADER_LENGTH)
#define IFF_NEXT_CHUNK(p)	(IFF_CHUNK_DATA(p) + IFF_CHUNK_LENGTH(p) + (IFF_CHUNK_LENGTH(p) & 1))

char const* GetUnscannedFS(void);  /* in mounty.c */
extern Semaphore *fsListSemaphore;

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#endif

#define DBUG0(x) printf x

static int32 pnpRunning = FALSE, pnpRepeat = FALSE, initialPnpComplete = FALSE;
static List unmountedList = PREPLIST(unmountedList);

#ifdef DEBUG
void
PrintDeviceStack(DeviceStack *ds)
{
	DeviceStackNode *dsn;

	ScanList(&ds->ds_Stack, dsn, DeviceStackNode)
	{
		if (dsn->dsn_IsHW)
			printf("HW(%x) ", dsn->dsn_HWResource);
		else
			printf("%s ", dsn->dsn_DDFNode->ddf_n.n_Name);
	}
}

static void PrintUnmountedList(void)
{
	DeviceStack *ds;

	printf("--Unmounted list:\n");
	ScanList(&unmountedList, ds, DeviceStack)
	{
		printf("  ");
		PrintDeviceStack(ds);
		printf("\n");
	}
	printf("--end\n");
}
#endif /* DEBUG */

static void FreeUnmountedList(List *l)
{
	DeviceStack *ds;

	while (!ISEMPTYLIST(l))
	{
		ds = (DeviceStack *) RemHead(l);
		DeleteDeviceStack(ds);
	}
}

static void AddUnmountedList(DeviceStack *ds)
{
	DeviceStack *ds2;

	ds2 = CopyDeviceStack(ds);
	AddTail(&unmountedList, (Node *) ds2);
}

/**
|||	AUTODOC -public -class File -group Filesystem -name DeleteUnmountedList
|||	Deletes a list of device stacks previously returned from GetUnmountedList().
|||
|||	  Synopsis
|||
|||	    Err DeleteUnmountedList(List *l);
|||
|||	  Description
|||
|||	    Deletes a list of device stacks which was previously
|||	    returned from GetUnmountedList().
|||
|||	  Arguments
|||
|||	    l
|||	        Pointer to the list returned from GetUnmountedList().
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>
|||
|||	  See Also
|||
|||	    GetUnmountedList()
|||
**/

Err DeleteUnmountedList(List *l)
{
	FreeUnmountedList(l);
	FreeMem(l, sizeof(List));
	return 0;
}

/**
|||	AUTODOC -public -class File -group Filesystem -name GetUnmountedList
|||	Gets the list of device stacks which failed to mount.
|||
|||	  Synopsis
|||
|||	    Err GetUnmountedList(List **lp);
|||
|||	  Description
|||
|||	    Returns a list of device stacks which the file system
|||	    was able to open successfully, but was not able to mount.
|||	    This list should be freed by calling DeleteUnmountedList().
|||
|||	  Arguments
|||
|||	    lp
|||	        Pointer to a (List *) which is set to point to the
|||	        list of DeviceStacks.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure.  
|||
|||	  Implementation
|||
|||	    Folio call implemented in file folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>
|||
|||	  See Also
|||
|||	    DeleteUnmountedList()
|||
**/

Err GetUnmountedList(List **lp)
{
	List *l;
	DeviceStack *ds;
	DeviceStack *ds2;

	l = AllocMem(sizeof(List), MEMTYPE_NORMAL);
	PrepList(l);
	ScanList(&unmountedList, ds, DeviceStack)
	{
		ds2 = CopyDeviceStack(ds);
		if (ds2 == NULL)
		{
			DeleteUnmountedList(l);
			return NOMEM;
		}
		AddTail(l, (Node *) ds2);
	}
	*lp = l;
	return 0;
}

/*
   Can be called from either user or supervisor mode
*/

Item FileSystemMountedOn(const DeviceStack *ds)
{
  FileSystem *fs;
  Item ret = 0;

  (void) LockSemaphore(fsListSemaphore->s.n_Item,
		       SEM_WAIT + SEM_SHAREDREAD);

  ScanList(&fileFolio.ff_Filesystems, fs, FileSystem) {
    if (DeviceStackIsIdentical(ds, fs->fs_RawDevice->dev.n_Item)) {
      ret = fs->fs.n_Item;
      break;
    }
  }

  (void) UnlockSemaphore(fsListSemaphore->s.n_Item);

  return ret;
}

static void MountAllStacks(void)
{
	List *list;
	DeviceStack *ds;
	Item dev;
	Err err;

DBUG(("=================== MountAllStacks ==================\n"));

	DBUG(("Creating device stack, mount level %d\n",
	       fileFolio.ff_MountLevel));

	err = CreateDeviceStackListVA(&list,
/*		"FS", DDF_DEFINED, 0, 0, */
		"FS", DDF_LE, DDF_INT, 1, fileFolio.ff_MountLevel,
		NULL);
	DBUG(("Got stack\n"));

	if (err < 0)
	{
#ifdef BUILD_STRINGS
	    printf("MountAllStacks: error 0x%X", err);
#endif
	    return;
	}

	FreeUnmountedList(&unmountedList);
	ScanList(list, ds, DeviceStack)
	{
	    /*
	     * This loop takes so long that it can block other tasks
	     * for an unacceptably long time.  (Since MountAllFileSystems
	     * is a system call, t_Forbid is set while we're in here,
	     * and task switching is disabled.)  So be a good citizen
	     * and yield the CPU if someone else wants to run.
	     */
	    Yield();

	    if (FileSystemMountedOn(ds) > 0)
		continue;
#ifdef DEBUG
	    printf("Checking "); PrintDeviceStack(ds); printf("\n");
#endif
	    dev = OpenDeviceStack(ds);
	    if (dev < 0)
	    {
		DBUG(("MountAll: OpenDeviceStack err %x\n", dev));
		continue;
	    }
	    DBUG(("MountAll: mounting %s\n", ((ItemNode *)LookupItem(dev))->n_Name));
	    err = MountFileSystem(dev, 0);
	    if (err < 0)
		AddUnmountedList(ds);
	    DBUG(("MountAll: mount ret %x\n", err));
	    TOUCH(err);
            CloseDeviceStack(dev);
	}

	DeleteDeviceStackList(list);
#ifdef DEBUG
	PrintUnmountedList();
#endif
}

static char path[FILESYSTEM_MAX_PATH_LEN];
static char* subpath;

static Directory* openDeviceDir(FileSystem *fs)
{
    Directory* rv;

    /* Build the path to the devices directory. */
    DBUG(("Looking on `%s'", fs->fs_FileSystemName));
    *path= '/';
    strcpy(path + 1, fs->fs_FileSystemName);
    strcat(path, "/System.m2/Drivers/Descriptions");
    subpath= path + strlen(path);
    DBUG(("for `%s'...\n", path));

    /* Open the directory for scanning. */
    rv= OpenDirectoryPath(path);
    if(rv) *subpath++= '/';
    return rv;
}

static void* readDeviceFile(DirectoryEntry* de)
{
    IFFHeader header;
    Err err;
    RawFile* file= 0;
#ifdef DEBUG
    char const* errstr;
#endif
    void* buf= 0;
    int buflen= 0;

    /* Open the file. */
    strcpy(subpath, de->de_FileName);
    err= OpenRawFile(&file, path, FILEOPEN_READ);
    if(err < 0)
    {
#ifdef DEBUG
	errstr= "OpenRawFile";
#endif
	goto abort;
    }

    /* Read the FORM chunk header to verify that */
    /* this is a Device Description File. */
    err= ReadRawFile(file, &header, sizeof(header));
    if(err < 0)
    {
#ifdef DEBUG
	errstr= "ReadRawFile(header)";
#endif
	goto abort;
    }
    if (header.iff_FormID != MAKE_ID('F','O','R','M')
	|| header.iff_FormType != MAKE_ID('D','D','F',' '))
    {
#ifdef DEBUG
	errstr= "@CheckChunk";
#endif
	goto abort;
    }

    /* Allocate a suitable buffer for the rest of the file. */
    buflen= header.iff_FormLength - sizeof(uint32);  /* length includes form type */

    if(buflen + 3 * sizeof(uint32) != de->de_ByteCount)
    {
#ifdef DEBUG
	errstr= "@CheckFormLength";
	err= -1;
#endif
	goto abort;
    }

    buf= AllocMem(buflen, MEMTYPE_NORMAL);
    if(!buf)
    {
#ifdef DEBUG
	errstr= "AllocMem";
#endif
	goto abort;
    }

    /* Read the rest of the file into the buffer. */
    err= ReadRawFile(file, buf, buflen);
    if(err < 0)
    {
#ifdef DEBUG
	errstr= "ReadRawFile(buf)";
#endif
	goto abort;
    }

    CloseRawFile(file);
    return buf;

abort:
    DBUG(("Operator:readDeviceFile:%s:\n\t", errstr));
    ERR(err);
    FreeMem(buf, buflen);
    CloseRawFile(file);
    return 0;
}

/* This must be called in supervisor mode since it modifies the DDF list */
/* which is owned by the kernel. */
Err MountAllFileSystemsSWI(void)
{
  FileSystem *fs;
  Directory *dir;
  DirectoryEntry de;

  DBUG(("In MountAllFileSystems SWI\n"));

  pnpRunning = TRUE;

  do {
    pnpRepeat = FALSE;

#ifdef DEBUG
    {
      HWResource hwr;
      Err err;

      hwr.hwr_InsertID= 0;
      DBUG(("Hardware found:\n"));
      while(err= NextHWResource(&hwr), err >= 0)
	{
	  DBUG(("\t`%s'\n", hwr.hwr_Name));
	}
    }
#endif

    (void) SuperInternalLockSemaphore(fsListSemaphore,
				      SEM_WAIT + SEM_SHAREDREAD);

    ScanList(&fileFolio.ff_Filesystems, fs, FileSystem) {

      if (fs->fs_Flags & FILE_FS_SCANNED)     continue;
      fs->fs_Flags |= FILE_FS_SCANNED;

      if (!(fs->fs_VolumeFlags & VF_BLESSED)) continue;

      DBUG(("checking FS %s\n", fs->fs_FileSystemName));
      dir= openDeviceDir(fs);
      if(!dir)
	{
	  DBUG(("file system `%s' has no device directory\n",
		fs->fs_FileSystemName));
	  continue;		/* Skip on error. */
	}

      while(ReadDirectory(dir, &de) >= 0)
	{
	  int l= strlen(de.de_FileName);
	  if(!strcmp(de.de_FileName + l - 4, ".ddf"))
	    {
	      char* buf;
	      uint8 version;
	      uint8 revision;

	      /* Read in the file. */
	      DBUG(("Found DDF %s\n", de.de_FileName));
	      buf= (char*)readDeviceFile(&de);
	      if(!buf) continue; /* Skip on error. */

	      l= de.de_ByteCount - 12;

	      if (de.de_Flags & FILE_HAS_VALID_VERSION) {
		/* Get the version from the directory entry. */
		version= de.de_Version;
		revision= de.de_Revision;
	      }
	      else
	      {
	        version = 0;
	        revision = 0;
	      }

	      DBUG(("version is $%x\n", version));
	      ProcessDDFBuffer(buf, l, version, revision);

	      /* WARNING:  I'm throwing away the buffer pointer */
	      /* returned by readDeviceFile() because we will */
	      /* never free the memory for the lifetime of the */
	      /* system.  Also, too many different parts refer */
	      /* to the buffer internals. */
	    }
	}

      CloseDirectory(dir);
    }

    (void) SuperInternalUnlockSemaphore(fsListSemaphore);

    /* 5
     * For each low-level DDE:
     * If it is currently disabled and there is a HWResource that it
     * knows how to manage, re-enable it and its ancestors.
     * If it is currently enabled and there is no HWResource that it
     * knows how to manage, disable it and its ancestors that have
     * no enabled children.
     */
    DBUG(("rebuild DDF enables...\n"));
    RebuildDDFEnables();

    /* 6
     * Find all DeviceStacks which support a file system and call
     * MountDeviceStack() on each.  Note that the Filesystem keeps
     * track of which file systems were newly-mounted (i.e., unscanned).
     */
    DBUG(("Finding enabled, unmounted, FS stacks...\n"));

    MountAllStacks();

    /* 7
     * Repeat all of the above if anything new filesystems were mounted.
     */
  } while (pnpRepeat);

  pnpRunning = FALSE;

  if (!initialPnpComplete) {
    initialPnpComplete = TRUE;
    SendSignal(FindTask("operator"), WAKEE_WAKE_SIGNAL);
  }

#ifdef DEBUG
  {
    DDFNode* ddfn;

    DBUG(("DDFs found:\n"));
    StartScanProtectedList(&KB_FIELD(kb_DDFs), ddfn, DDFNode)
    {
	DBUG(("\t`%s' flags $%x\n", ddfn->ddf_n.n_Name, ddfn->ddf_Flags));
    }
    EndScanProtectedList(&KB_FIELD(kb_DDFs));
  }
#endif

    return 0;
}

void internalPlugNPlay (void)
{
  if (pnpRunning) {
    pnpRepeat = TRUE;
  } else {
    SuperinternalSignal(fileFolio.ff_Mounter.ffm_Task,
			fileFolio.ff_Mounter.ffm_Signal);
  }
}

