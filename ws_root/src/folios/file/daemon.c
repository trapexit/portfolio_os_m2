/* @(#) FileDaemon.c 96/07/17 1.31 */

/*
  FileDaemon.c - code to implement the task which runs as a daemon
  on behalf of the file folio.  This task takes care of actions such as
  I/O scheduling, which need to run within a task environment (i.e. not
  at the interrupt level).


  What is the hearbeat signal used for?
  -------------------------------------

  It runs the dismount engine.

  The process of quiescing an active filesystem, and then actually
  dismounting it, may take some time.  There's a significant amount of
  work which may need to be done (checking repeatedly to see if any
  files are open, zapping File nodes with a use count of 0, etc.) that
  can't necessarily be done in the context of the process which asked
  that the dismount actually take place.

  Whenever the Aahz daemon wakes up (heartbeat signal, or "Please do a
  scheduling run" signal from a filesystem adapter) it does two sets of
  work.  For one, it calls the filesystem-adapter timeslice routine for
  each mounted filesystem.  For another, it "cranks the dismount
  engine", checking for filesystems which have not yet reached their
  desired state of quiescence (or un-mountedness) and trying to push
  these filesystems one or more steps further along towards their
  desired state.

  The heartbeat signal is there to make sure that this process happens,
  even if no physical I/O is going on anywhere to awaken the daemon.
  If...

-  you don't have the heartbeat, and
-  a program attempts to quiesce or dismount a filesystem [or a microcard
   is unplugged and the DDF scanner/checker notices this and marks the
   filesystem "offline, please dismount"], and
-  the operation cannot be completed immediately (e.g. because files are
   open), and
-  nobody's doing any physical I/O which causes the filesystem adapters to
   send a "wake up and give me a timeslice" signal to the daemon,

then

*  The dismount/quiesce process will make no progress, because the engine
   will not be cranked.  The filesystem will sit in its current state,
   and (e.g.) its icon will not disappear from the Storage Manager.

This problem won't show up, probably, if there aren't any open files
on the filesystem when the quiesce/dismount is initiated and if no
programs are CD'ed to, or running programs from that filesystem.
Under these conditions, the initial Dismount() call will crank the
dismount engine all the way to the state of being completely
dismounted.

It used to be the case that many of the filesystem modules (Opera and
linked-mem for certain) would be guaranteed to send a "schedule"
signal to Aahz whenever they'd drained their "ready to go" I/O queues,
so that Aahz could give them a timeslice to run their "prepare next
I/O" schedulers.  They'd also signal for a scheduling run when they
got a SendIO dispatch, the filesystem was idle, and there were no
previous requests in the queue.  This would allow the dismount engine
to be run fairly frequently if there was any filesystem I/O.

However, in recent revisions of Portfolio M2, I eliminated most of the
"reschedule" signals by having the "queue is empty" code check first
to see if there were any unprepared requests in the to-be-done list...
if not, there's no sense sending the signal.  Similarly, I optimized
the Opera sendio dispatcher to call the scheduling routine directly if
it was called from user mode, rather than signalling - this saves a
task-switch.  As a result, it's entirely possible for Aahz to sit idle
for long periods, with nothing other than the heartbeat waking it up
to run the dismount engine.

If Aahz's CPU overhead is a concern, the wakeup time could be reduced
to 2 or 5 or 10 seconds, or could some fancy logic could be added
to kill the metronome request when we're absolutely positively entirely certain
that no filesystems need to be cranked, and fire it up again when an unsuccessful
dismount occurs.
[The latter may not be a trivial thing to do, as the filesystem
adapters are permitted to set the "please quiesce and dismount this
thing" flags on a filesystem from within their I/O endaction callbacks
- e.g. if the device goes offline - and this code can't send a request
to the timer.]

*/

#define SUPER

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/folio.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/time.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/filefunctions.h>
#include <file/discdata.h>
#define LOAD_FOR_PORTFOLIO
#include <loader/loadererr.h>
#include <loader/loader3do.h>
#include <string.h>

void FileDaemonStartup(void);
void FileDaemonInternals(void);

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern char *fsCacheBase;
extern int32 fsCacheSize;
extern int32 fsCacheBusy;
extern IoCache fsCache;
extern IoCacheEntry *fsCacheEntryArray;
extern int32 fsCacheEntrySize;

extern struct KernelBase *KernelBase;

extern int32 awakenSleepers;
extern Semaphore *fsListSemaphore;

extern Err CrankDismountEngine(FileSystem *fs);

static void RunMountRequests(void);
static void FSLoader(int argc, char **argv);

/*
  Here's where all the fun happens.  The daemon allocates a set of signals
  for the following purposes:

  "QueuedSignal" - sent by the file driver to tell the daemon that the
     driver has just queued up an I/O for an idle device.

  "HeartbeatSignal" - sent by the timer device metronome a couple of times
     a second.

  "RescheduleSignal" - sent by the file driver's I/O endaction handler to
     tell the daemon that it was unable to start another I/O... either
     because the to-do list has been emptied, or because a higher-
     priority I/O request has caused the device's BOINK bit to be set.

  The arrival of any of these signals will cause the daemon to offer a
  timeslice to each mounted filesystem, if the filesystem's type block
  provides a timeslice function.

*/

void FileDaemonInternals(void)
{
  uint32 queuedSignal;
  uint32 heartbeatSignal;
  uint32 rescheduleSignal;
  uint32 sleepMask;
  Item ioReqItem, moduleItem;
  FileSystem *fileSystem;
  FileSystem *nextFs;
  FileSystemType *fst;
  IOReq *ior;
  Err err;
  uint32 awakenedMask;
  int32 interrupts;

  queuedSignal = SuperAllocSignal(0L);
  heartbeatSignal = SuperAllocSignal(0L);
  rescheduleSignal = SuperAllocSignal(0L);
/*
  Publish them
*/
  fileFolio.ff_Daemon.ffd_QueuedSignal = queuedSignal;
  fileFolio.ff_Daemon.ffd_HeartbeatSignal = heartbeatSignal;
  fileFolio.ff_Daemon.ffd_RescheduleSignal = rescheduleSignal;
  DBUG(("Daemon signals created\n"));

  DBUG(("Creating IOReq(s)s for timer\n"));
  ioReqItem = CreateTimerIOReq();
  if (ioReqItem < 0) {
    qprintf(("Error creating IOReq node\n"));
    goto shutdown;
  }
  DBUG(("Timer IOReq is item 0x%x ", ioReqItem));
  ior = (IOReq *) LookupItem(ioReqItem);
  DBUG(("at 0x%x\n", ior));
  sleepMask = queuedSignal | heartbeatSignal | rescheduleSignal |
    SIGF_IODONE;
  memset(&ior->io_Info, 0, sizeof ior->io_Info);
  ior->io_Info.ioi_Command = TIMERCMD_METRONOME_VBL;
  ior->io_Info.ioi_CmdOptions = heartbeatSignal;
  ior->io_Info.ioi_Offset = 27; /* roughly half a second, give or take... */
  DBUG(("Starting metronome\n"));
  if ((err = SuperinternalSendIO(ior)) < 0) {
    qprintf(("Error 0x%X starting metronome", err));
#ifndef BUILD_STRINGS
	TOUCH(err);
#endif
  }
  oqprintf(("File-daemon idle-loop starting\n"));
  fileFolio.ff_Daemon.ffd_Task = CURRENTTASK;

  /* tell the creator that we're ready for action */
  SuperInternalSignal((Task *)LookupItem(CURRENTTASK->t.n_Owner),0x00010000);

  DBUG(("Filefolio 0x%X, task at 0x%X signal 0x%X\n",
	 &fileFolio,
	 fileFolio.ff_Daemon.ffd_Task,
	 fileFolio.ff_Daemon.ffd_RescheduleSignal));
  do {
    DBUG(("File daemon zzz...\n"));
    awakenedMask = SuperWaitSignal(sleepMask);
    DBUG(("File daemon awake\n"));
    if (awakenedMask & SIGF_ABORT) {
      DBUG0(("SIGF_ABORT!\n"));
      break;
    }
    interrupts = Disable();
    if (awakenSleepers) {
      awakenSleepers = FALSE;
      if (fsCacheBase)	CacheWake();
    }
    Enable(interrupts);
    /*
     * Note: there is the possibility that the fs we're looking at
     * is torn down after calling one of the fs-specific timeslice
     * routines.  Thus, we take care to retrieve & save the next node
     * pointer before we call these routines.
     */
    DBUG(("Scheduling\n"));
    if (!IsEmptyList(&fileFolio.ff_MountRequests)) {
      RunMountRequests();
    }
    if (awakenedMask & heartbeatSignal) {
      err = SuperInternalLockSemaphore(fsListSemaphore, 0); /* excl, nowait */
      if (err >= 0) {
	ScanList(&fileFolio.ff_FileSystemTypes, fst, FileSystemType) {
	  fst->fst.n_Flags &= ~FSTYPE_IN_USE;
	}
	for (fileSystem = (FileSystem *) FIRSTNODE(&fileFolio.ff_Filesystems);
	     ISNODE(&fileFolio.ff_Filesystems, fileSystem);
	     fileSystem = nextFs) {
	  nextFs = (FileSystem *) NEXTNODE(fileSystem);
	  if (!(fileSystem->fs_Flags & FILESYSTEM_IS_QUIESCENT)) {
	    fileSystem->fs_Type->fst.n_Flags |= FSTYPE_IN_USE;
	  }
	  if (fileSystem->fs_Flags & (FILESYSTEM_WANTS_QUIESCENT |
				      FILESYSTEM_WANTS_DISMOUNT)) {
	    (void) CrankDismountEngine(fileSystem);
	  }
	}
	ScanList(&fileFolio.ff_FileSystemTypes, fst, FileSystemType) {
	  if ((fst->fst.n_Flags & FSTYPE_WANTS_UNLOAD) &&
	      !(fst->fst.n_Flags & FSTYPE_IN_USE)) {
	    qprintf(("Unloading the %s filesystem module\n", fst->fst.n_Name));
	    interrupts = Disable();
	    (void) (*fst->fst_Unloader)(fst);
	    moduleItem = fst->fst_ModuleItem;
	    fst->fst_ModuleItem = 0;
	    fst->fst.n_Flags = (fst->fst.n_Flags ^ FSTYPE_WANTS_UNLOAD) |
	      FSTYPE_NEEDS_LOAD;
	    Enable(interrupts);
	    err = CloseModule(moduleItem); /* which makes it purgable */
	    if (err < 0) {
	      DBUG0(("Error closing module\n"));
	      ERR(err);
	    }
	  }
	}
      }
      (void) SuperInternalUnlockSemaphore(fsListSemaphore);
      CacheWake(); /* Prod sleepers */
    }
    for (fileSystem = (FileSystem *) FIRSTNODE(&fileFolio.ff_Filesystems);
	 ISNODE(&fileFolio.ff_Filesystems, fileSystem);
	 fileSystem = nextFs) {
      nextFs = (FileSystem *) NEXTNODE(fileSystem);
      if (fileSystem->fs_Flags & FILESYSTEM_NEEDS_INIT) {
	fileSystem->fs_Flags &= ~ FILESYSTEM_NEEDS_INIT;
	(*fileSystem->fs_Type->fst_FirstTimeInit)(fileSystem);
      }
      if (fileSystem->fs_Type->fst_Timeslice) {
	(*fileSystem->fs_Type->fst_Timeslice)(fileSystem);
      }
    }
    DBUG(("Scheduling done\n"));
  } while (TRUE);
 shutdown:
  qprintf(("File-daemon shutdown!\n"));
  fileFolio.ff_Daemon.ffd_Task = (Task *) NULL;
  SuperDeleteItem(CURRENTTASKITEM); /* Eat frozen death, alien slime! */
}

static void RunMountRequests(void) {
  MountRequest *thisRequest, *nextRequest;
  int32 interrupts;
  int32 doneWith, repeat;
  FileSystem *theFS;
  FileSystemType *type;
  Task *theTask;
  int32 err;
  thisRequest = (MountRequest *) FirstNode(&fileFolio.ff_MountRequests);
  DBUG(("Running mount requests\n"));
  while (IsNode(&fileFolio.ff_MountRequests,thisRequest)) {
    nextRequest = (MountRequest *) NextNode(thisRequest);
    doneWith = FALSE;
    repeat = FALSE;
    theFS = (FileSystem *) LookupItem(thisRequest->mr_FileSystemItem);
    theTask = (Task *) LookupItem(thisRequest->mr_TaskRequestingMount);
    if (!theTask) {
      DBUG(("Task went away without dequeueing request!\n"));
      doneWith = TRUE;
    } else if (!theFS) { /* It was dismounted behind our backs */
      DBUG(("Filesystem was dismounted\n"));
      thisRequest->mr_Err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
      doneWith = TRUE;
    } else if (theFS->fs_MountError < 0) {
      DBUG0(("Mount failed\n"));
      thisRequest->mr_Err = theFS->fs_MountError;
      doneWith = TRUE;
    } else if ((theFS->fs_Flags & FILESYSTEM_IS_QUIESCENT) == 0 ||
	       (type = theFS->fs_Type) == NULL) {
      DBUG(("Mount succeeded without error\n"));
      doneWith = TRUE;
    } else if (type->fst_LoadError < 0) {
      DBUG0(("Unable to load code\n"));
      thisRequest->mr_Err = type->fst_LoadError;
      doneWith = TRUE;
    } else if (type->fst.n_Flags & FSTYPE_IS_LOADING) {
      DBUG(("Code is loading\n"));
      ;;; /* do nothing at this time */
    } else if (type->fst.n_Flags & FSTYPE_NEEDS_LOAD) {
      DBUG(("Starting code load\n"));
      (*type->fst_Loader)(type);
      repeat = TRUE;
    } else if (type->fst_ActQue != NULL) {
      DBUG(("Attempting to activate filesystem /%s\n",
	     theFS->fs_FileSystemName));
      err = (*type->fst_ActQue)(theFS, ActivateFS);
      DBUG(("Got return code 0x%x\n", err));
      if (err <= 0) {
	theFS->fs_MountError = err;
	repeat = TRUE;
      }
    } else {
      qprintf(("Aiee, mount engine got stuck\n"));
      /* errgh.  If we get here, the filesystem is marked as non-ready,
	 but there's no error information on file, and no apparent way to
	 push the mount process along any further.  Let's just call it
	 offline, and bail. */
      thisRequest->mr_Err = MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
      doneWith = TRUE;
    }
    if (doneWith) {
      interrupts = Disable();
      DBUG(("Mount engine removing request node\n"));
      RemNode((Node *) thisRequest);
      if (thisRequest->mr_Err > 0) {
	thisRequest->mr_Err = 0;
      }
      if (theTask) {
	SuperinternalSignal(theTask, SIGF_ONESHOT);
      }
      Enable(interrupts);
    }
    if (!repeat) {
      thisRequest = nextRequest;
    }
  }
  DBUG(("End of mount requests\n"));
}

Err LoadFSDriver(FileSystemType *fst)
{
  Item thread;
  int32 interrupts;
  DBUG(("Creating a loader thread for '%s'\n", fst->fst.n_Name));
  interrupts = Disable();
  thread = CreateThreadVA(FSLoader, "FSLoader", 0, 2048,
			  CREATETASK_TAG_PRIVILEGED, 0,
			  CREATETASK_TAG_SINGLE_STACK, 0, /* can we do this? */
			  CREATETASK_TAG_ARGC, (TagData) 1,
			  CREATETASK_TAG_ARGP, (TagData) fst,
			  TAG_END);
  if (thread < 0) {
    fst->fst_LoadError = thread;
    DBUG(("Failed to create thread!\n"));
    ERR(thread);
    Enable(interrupts);
    return thread;
  }
  fst->fst.n_Flags = (fst->fst.n_Flags & ~FSTYPE_NEEDS_LOAD) |
    FSTYPE_IS_LOADING;
  DBUG(("Created thread 0x%X\n", thread));
  Enable(interrupts);
  return 0;
}

static const TagArg filesystemSearch[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) (DONT_SEARCH_UNBLESSED +
                                                   DONT_SEARCH_NO_CODE),
    TAG_END
};

static const TagArg modTags[] =
{
    MODULE_TAG_MUST_BE_SIGNED, (TagData)TRUE,
    TAG_END
};

static void FSLoader(int argc, char **argv)
{
  Item moduleItem;
  char pathname[256], searchname[64];
  Err err;
  FileSystemType *fst = (FileSystemType *) argv;
  DBUG(("Entered FS loader thread for '%s'\n", fst->fst.n_Name));
  strcpy(searchname, "System.m2/FileSystems/");
  strcat(searchname, fst->fst.n_Name);
  err = FindFileAndIdentify(pathname, sizeof(pathname),
			    searchname, filesystemSearch);
  if (err < 0) {
    DBUG(("Failed to find driver module!\n"));
    goto bail;
  }
  DBUG(("Opening module '%s'\n", pathname));
  err = moduleItem = OpenModule(pathname, OPENMODULE_FOR_TASK, modTags);
  if (err < 0) {
    DBUG(("Failed to open driver module!\n"));
    goto bail;
  }
  DBUG(("Calling module to initialize filesystem driver\n"));
  err = ExecuteModule(moduleItem, argc, argv);
  if (err < 0) {
    DBUG(("Module returned an error from its init routine!\n"));
    (void) CloseModule(moduleItem);
    goto bail;
  }
  DBUG(("Module OK, load completed, clearing needs-load flag\n"));
  fst->fst_ModuleItem = moduleItem;
  goto fini;
 bail:
  ERR(err);
 fini:
  fst->fst_LoadError = err;
/*
   Caution - a potential race condition exists in the following statement.
   Reconsider this and see if there's any way that the daemon could interrupt
   this statement and futz with the flags... if so, the daemon's new
   settings could be clobbered.  We may need to do some semaphore locking
   somehow, or switch to supervisor mode momentarily.
*/
  DBUG(("Transferring item ownership to daemon\n"));
  TransferItems(fileFolio.ff_Daemon.ffd_Task->t.n_Item);
  fst->fst.n_Flags &= ~(FSTYPE_IS_LOADING + FSTYPE_NEEDS_LOAD);
  SendSignal(fileFolio.ff_Daemon.ffd_Task->t.n_Item,
	     fileFolio.ff_Daemon.ffd_RescheduleSignal);
  DBUG(("FSLoader thread exiting\n"));
  return;
}

Err UnloadFSDriver(FileSystemType *fst)
{
  TOUCH(fst);
  return 0;
}

