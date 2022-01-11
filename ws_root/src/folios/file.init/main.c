/* @(#) main.c 96/07/17 1.16 */

/*
  FileFolioMain.c - coldstart code for the filesystem.  Creates the
  file folio and the file driver, and mounts filesystems.
*/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/dipir.h>
#include <device/cdrom.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/discdata.h>
#include <file/filefunctions.h>
#include <loader/loader3do.h>
#include <misc/event.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>

extern int32 FileDriverInit(struct Driver *me);
extern int32 FileDriverDispatch(IOReq *theRequest);
extern void FileDriverAbortIo(IOReq *theRequest);
extern Err CleanupOpenFile(Device *theDev);
extern void CleanupOnClose(Device *theDev);
extern int32 DeleteIOReqItem(IOReq *req);

/*
   Imports from file folio, via file.stubs
*/

extern void FileDaemonStartup(void);
extern Item CreateFileItem(ItemNode *theNode, uint8 nodeType, TagArg *args);
extern Err SetFileItemOwner(ItemNode *n, Item newOwner, struct Task *t);
extern int32 DeleteFileItem(Item theItem, Task *theTask);
extern void DeleteFileTask(Task *theTask);




extern int32 FileFolioCreateTaskHook(Task *t, TagArg *tags);
extern const TagArg fileFolioErrorTags[];

extern TagArg fileFolioTags[];

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

extern int32 fsOpenForBusiness;
extern int32 awakenSleepers;

extern Semaphore *fsListSemaphore;

extern void FileDaemonInternals(void);
extern char fsDaemonStack[];
extern int32 fsDaemonStackSize;

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
# define INFO(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) /* if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x */
# define INFO(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern const char whatstring[];

FileSystem * InitFileSystem(Device *theDevice);

#undef  DEBUG

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

extern char *fsCacheBase;
extern int32 fsCacheSize;
extern int32 fsCacheBusy;
extern IoCache fsCache;
extern IoCacheEntry *fsCacheEntryArray;
extern int32 fsCacheEntrySize;

void InitFileCache(void)
{
  int32 entryCount;
  int32 i;
  DBUG(("Initializing filesystem cache\n"));
  /*
   *	XXX - temp hack put in to get things going for acrobat
   *          this needs to be revisited.
   */
  fsCacheSize = GetPageSize(MEMTYPE_ANY) * FILESYSTEM_CACHE_SYSPAGES;
  DBUG(("DRAM page size is %d bytes\n", fsCacheSize));
  entryCount = fsCacheSize / FILESYSTEM_CACHE_PAGE_SIZE;
  fsCacheSize = entryCount * FILESYSTEM_CACHE_PAGE_SIZE;
  DBUG(("Requesting %d bytes for the cache\n", fsCacheSize));
  fsCacheBase = SuperAllocMem(fsCacheSize, MEMTYPE_NORMAL);

#ifdef BUILD_PARANOIA
  if (!fsCacheBase) {
    INFO(("FS cache disabled.  AAGH!\n"));
#ifdef NOTDEF
    Panic(MakeFErr(ER_FATAL,ER_C_STND,ER_NoMem),__FILE__,__LINE__);
#endif
    return;
  }
#endif
  DBUG(("Filesystem cache base is %d bytes at 0x%X\n",
	fsCacheSize, fsCacheBase));
  fsCacheEntrySize = entryCount * sizeof (IoCacheEntry);
  fsCacheEntryArray = (IoCacheEntry *) SuperAllocMem(fsCacheEntrySize,
						MEMTYPE_DMA + MEMTYPE_FILL);
#ifdef BUILD_PARANOIA
  if (!fsCacheEntryArray) {
    INFO(("Could not allocate %d-byte cache table\n", fsCacheEntrySize));
    SuperFreeMem(fsCacheBase, fsCacheSize);
    fsCacheBase = NULL;
#ifdef NOTDEF
    Panic(MakeFErr(ER_FATAL,ER_C_STND,ER_NoMem),__FILE__,__LINE__);
#endif
    return;
  }
#endif
  DBUG(("Cache table is %d bytes at 0x%x\n", fsCacheEntrySize, fsCacheEntryArray));
  PrepList(&fsCache.ioc_CachedBlocks);
  PrepList(&fsCache.ioc_TasksSleeping);
  for (i = 0; i < entryCount; i++) {
    fsCacheEntryArray[i].ioce_CachedBlock = fsCacheBase + i * FILESYSTEM_CACHE_PAGE_SIZE;
    fsCacheEntryArray[i].ioce_PageState = CachePageInvalid;
    AddTail(&fsCache.ioc_CachedBlocks, (Node *) &fsCacheEntryArray[i]);
    DBUG(("  Entry %d buffer is at 0x%x\n", i, fsCacheEntryArray[i].ioce_CachedBlock));
  }
  fsCache.ioc_EntriesAllowed = entryCount;
  fsCache.ioc_EntriesPresent = entryCount;
  fsCache.ioc_EntriesReserved = 0;
  DBUG0(("Unified FS cache/buffer enabled, %d bytes\n", fsCacheSize));
}

long
InitFileFolio (FileFolio *folio)
{
#ifdef BUILD_STRINGS
  Item errorTableItem;
#endif
/*
  Save the folio address in static memory for later use
*/
  folio->ff_Root = (File *) NULL;
  PrepList(&folio->ff_Filesystems);
  PrepList(&folio->ff_Files);
  PrepList(&folio->ff_FileSystemTypes);
  PrepList(&folio->ff_MountRequests);
  PrepList(&folio->ff_Aliases);

  folio->ff.f_ItemRoutines->ir_Create =
    (Item (*)(void *, uint8, void *)) CreateFileItem;
  folio->ff.f_ItemRoutines->ir_Delete = (int32 (*)())DeleteFileItem;
  folio->ff.f_ItemRoutines->ir_SetOwner = SetFileItemOwner;
  folio->ff.f_FolioDeleteTask = (void (*)()) DeleteFileTask;
  folio->ff.f_FolioCreateTask = (int32 (*)()) FileFolioCreateTaskHook;
  folio->ff_Daemon.ffd_Task = (Task *) NULL;
  folio->ff_Daemon.ffd_QueuedSignal = 0;
  folio->ff_Daemon.ffd_HeartbeatSignal = 0;
  folio->ff_Daemon.ffd_RescheduleSignal = 0;
  folio->ff_Mounter.ffm_Task = CURRENTTASK;
  folio->ff_Mounter.ffm_Signal = SuperAllocSignal(0L);
  folio->ff_NextUniqueID = -1;
  folio->ff_OpensSinceCleanup = 0;
  folio->ff_MountLevel = MOUNT_EXTERNAL_RW_BOOT;
#ifdef BUILD_STRINGS
  errorTableItem = SuperCreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),
				   (TagArg *) fileFolioErrorTags);
  if (errorTableItem < 0) {
    qprintf(("Can't register file-folio error table, 0x%x!\n", errorTableItem));
    ERR(errorTableItem);
  }
#endif
  InitFileCache();
  oqprintf(("File-folio init ends\n"));
  DBUG(("CURRENTTASK pointer is at 0x%X\n", &CURRENTTASK));
  return folio->ff.fn.n_Item;
}

int32 FileSystemInit(void)
{
  int32 i;
  Item fileDaemonItem;
  Item fsModule;

  DBUG(("FSInit started\n"));

  fsListSemaphore = SEMAPHORE(CreateUniqueSemaphore("@FS", 0));
#ifdef BUILD_PARANOIA
  if (!fsListSemaphore) {
    INFO(("Could not create filesystem semaphore!\n"));
  }
#endif
  DBUG(("Creating folio\n"));
  i = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), fileFolioTags);
#ifdef BUILD_PARANOIA
  if (i != FILEFOLIO) {
    PrintError(0,"create file folio",0,(uint32) i);
    if (i >= 0) {
      qprintf(("expected item #%d, got item #%d\n", FILEFOLIO, i));
    }
    goto bailout;
  }
#else
	TOUCH(i);
#endif
  DBUG(("File folio is item 0x%x\n", i));
  DBUG(("Creating file driver\n"));
  fsModule = FindNamedItem(MKNODEID(KERNELNODE,MODULENODE),"filesystem");
  i = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
	  TAG_ITEM_NAME, 	      (void *) "File",
	  CREATEDRIVER_TAG_ABORTIO,   (void *) (int32) FileDriverAbortIo,
	  CREATEDRIVER_TAG_DISPATCH,  (void *) (int32) FileDriverDispatch,
	  CREATEDRIVER_TAG_CREATEDRV, (void *) (int32) FileDriverInit,
	  CREATEDRIVER_TAG_CLOSEDEV,  (void *) CleanupOnClose,
	  CREATEDRIVER_TAG_DELETEDEV, (void *) CleanupOpenFile,
	  CREATEDRIVER_TAG_DLIO,      (void *) DeleteIOReqItem,
	  CREATEDRIVER_TAG_IOREQSIZE, (void *) sizeof (FileIOReq),
	  CREATEDRIVER_TAG_NODDF,      NULL,
	  CREATEDRIVER_TAG_MODULE,     fsModule,
	  TAG_END);

#ifdef BUILD_PARANOIA
  if (i < 0) {
    qprintf(("Couldn't create file driver (0x%x)\n", i));
    PrintError(0,"create file driver",0,(uint32) i);
    goto bailout;
  }
#endif
  fileDriver = (Driver *) LookupItem(i);
  i = OpenItem(i, 0);
  if (i < 0) {
    PrintError(0,"open file driver",0,(uint32) i);
    goto bailout;
  }
  DBUG(("File driver is item 0x%x at 0x%x\n", i, fileDriver));
  /*
    At this point we should create a daemon task and link it up
    */
  DBUG(("Creating file-daemon task\n"));

  AllocSignal(0x00010000);

  fileDaemonItem = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                                TAG_ITEM_NAME,                  "File Daemon",
                                TAG_ITEM_PRI,                   210,
                                CREATETASK_TAG_PC,              FileDaemonInternals,
                                CREATETASK_TAG_SP,              fsDaemonStack + fsDaemonStackSize,
                                CREATETASK_TAG_STACKSIZE,       fsDaemonStackSize,
                                CREATETASK_TAG_SUPERVISOR_MODE, 0,
                                CREATETASK_TAG_THREAD,          0,
                                TAG_END);
#ifdef BUILD_PARANOIA
  if (fileDaemonItem<0) {
    PrintError(0,"create file daemon task",0,(uint32) fileDaemonItem);
  }
#endif

  /* wait for the child to signal us */
  WaitSignal(0x00010000);
  FreeSignal(0x00010000);

  fsOpenForBusiness = TRUE;
  awakenSleepers = TRUE;
  SendSignal(fileDaemonItem, fileFolio.ff_Daemon.ffd_RescheduleSignal);
  DBUG(("Filesystem startup completed.\n"));
/*
   Kick the operator awake, so it does the initial DDF installation
   and triggers the first Plug&Play sequence.
*/
  SendSignal(FindTask("operator"), RESCAN_SIGNAL);
  return 0;
 bailout:
  return (int) i;
}



int32 main(void)
{
    return 0;
}
