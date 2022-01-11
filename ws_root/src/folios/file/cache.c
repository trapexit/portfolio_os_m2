/* @(#) FileCache.c 96/07/17 1.11 */

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  FileCache.c - contains routines to manage the shared directory/disk
  cache.
*/

#define SUPER

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
#include <file/filesystem.h>
#include <file/filesystemdefs.h>
#include <file/directory.h>
#include <file/discdata.h>

#undef DEBUG
#undef DEBUG2

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifndef BUILD_STRINGS
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

extern IoCache fsCache;
extern int32 fsCacheBusy;

/***
 IMPORTANT:  Both SleepCache() and CacheWake() MUST be called with
 interrupts disabled.  They do not disable/enable themselves.  Failure
 to adhere to this caution will probably result in system malfunction. dplatt
 ***/

void SleepCache(void)
{
  CacheSleeper me;
  me.cs_Sleeper = CURRENTTASK;
  me.cs_CacheWakeup = FALSE;
  AddTail(&fsCache.ioc_TasksSleeping, (Node *) &me);
  do {
    (void) SuperWaitSignal(SIGF_IODONE);
  } while (!me.cs_CacheWakeup);
}

void CacheWake(void)
{
  CacheSleeper *sleeper;
  while (!IsEmptyList(&fsCache.ioc_TasksSleeping)) {
    sleeper = (CacheSleeper *) RemHead(&fsCache.ioc_TasksSleeping);
    sleeper->cs_CacheWakeup = TRUE;
    SuperInternalSignal(sleeper->cs_Sleeper, SIGF_IODONE);
  }
}

Err ReserveCachePages (int32 numNeeded, int32 canSleep)
{
  int32 interrupts, keepGoing;
  Err err = 0;
  interrupts = Disable();
  do {
    keepGoing = FALSE;
    if (fsCache.ioc_EntriesPresent < numNeeded) {
      err = NOMEM;
    } else if (fsCache.ioc_EntriesReserved + numNeeded <=
	       fsCache.ioc_EntriesPresent) {
      fsCache.ioc_EntriesReserved += numNeeded;
      err = numNeeded;
    } else if (!canSleep) {
      err = NOMEM;
    } else {
      SleepCache();
      keepGoing = TRUE;
    }
  } while (keepGoing);
  Enable(interrupts);
  return err;
}

void RelinquishCachePages (int32 numToGiveUp)
{
  int32 interrupts;
  interrupts = Disable();
  if (fsCache.ioc_EntriesAllowed != 0) {
    fsCache.ioc_EntriesReserved -= numToGiveUp;
    CacheWake();
  }
  Enable(interrupts);
}

IoCacheEntry *GetFreeCachePage(int32 canSleep)
{
  int32 interrupts, keepGoing, take;
  IoCacheEntry *ce;
  interrupts = Disable();
  ce = NULL;
  take = FALSE;
  do {
    keepGoing = FALSE;
    if (fsCache.ioc_EntriesAllowed != 0) {
      ce = (IoCacheEntry *) LastNode(&fsCache.ioc_CachedBlocks);
      while (IsNode(&fsCache.ioc_CachedBlocks, ce) && !take) {
	if (ce->ioce_UseCount == 0) {
	  take = TRUE;
	  ce->ioce_UseCount = 1;
	  ce->ioce_PageState = CachePageInvalid;
	} else {
	  ce = (IoCacheEntry *) PrevNode(ce);
	}
      }
      if (!take && canSleep) {
	SleepCache();
	keepGoing = TRUE;
      }
    }
  } while (keepGoing);
  Enable(interrupts);
  if (ce) {
    DBUG(("Get free cache page at 0x%X\n", ce));
  }
  return ce;
}

void RelinquishCachePage(IoCacheEntry *cachePage)
{
  int32 interrupts;
  interrupts = Disable();
  cachePage->ioce_UseCount --;
  if (cachePage->ioce_UseCount == 0) {
    CacheWake();
  }
  Enable(interrupts);
}

enum CacheState FindBlockInCache(File *theFile, int32 fileBlockNumber,
				 int32 canSleep,
				 IoCacheEntry **cachePage, void **blockBase)
{
  int32 interrupts, keepGoing, take, sleep;
  enum CacheState theState;
  IoCacheEntry *ce;
  int32 blocksPerPage, baseBlock, blockSize;
  *cachePage = NULL;
  *blockBase = NULL;
  blockSize = theFile->fi_BlockSize;
  blocksPerPage = 2048 / blockSize;
  if (blocksPerPage == 0) {
    return CachePageInvalid;
  }
  baseBlock = fileBlockNumber - (fileBlockNumber % blocksPerPage);
  interrupts = Disable();
  do {
    theState = CachePageInvalid;
    keepGoing = FALSE;
    take = FALSE;
    sleep = FALSE;
    if (fsCache.ioc_EntriesAllowed != 0) {
      ce = (IoCacheEntry *) FirstNode(&fsCache.ioc_CachedBlocks);
      while (IsNode(&fsCache.ioc_CachedBlocks, ce) && !take && !sleep) {
	if (ce->ioce_FileUniqueIdentifier == theFile->fi_UniqueIdentifier &&
	    ce->ioce_Filesystem == theFile->fi_FileSystem &&
	    ce->ioce_FileBlockOffset == baseBlock) {
	  switch (ce->ioce_PageState) {
	  case CachePageLoading:
	    sleep = TRUE;
	    theState = CachePageLoading;
	    break;
	  case CachePageInvalid:
	    ce = (IoCacheEntry *) NextNode(ce);
	    break;
	  case CachePageValid:
	    *cachePage = ce;
	    if (blockSize * (fileBlockNumber - baseBlock + 1) <=
		ce->ioce_CachedBlockSize) {
	      theState = CachePageValid;
	      *blockBase = blockSize * (fileBlockNumber - baseBlock) +
		(char *) ce->ioce_CachedBlock;
	      ce->ioce_UseCount ++;
	    } else {
	      theState = CachePagePartial;
	    }
	    take = TRUE;
	    break;
	  }
	} else {
	  ce = (IoCacheEntry *) NextNode(ce);
	}
      }
      if (!take && sleep && canSleep) {
	SleepCache();
	keepGoing = TRUE;
      }
    }
  } while (keepGoing);
  Enable(interrupts);
  return theState;
}

Err LoadBlockIntoCache(File *theFile, int32 fileBlockNumber,
		       IoCacheEntry **cachePage, void **blockBase,
		       CacheLoader loaderFunction)
{
  int32 interrupts;
  IoCacheEntry *ce;
  void *block;
  Err err;
  enum CacheState theState;
  int32 blocksPerPage, baseBlock, blockSize, blocksInPage, bytesToRead;
  int32 firstBlockToRead, blocksToRead, blocksLeft;
  blockSize = theFile->fi_BlockSize;
  blocksPerPage = 2048 / blockSize;
  if (blocksPerPage == 0) {
    return BADIOARG;
  }
  baseBlock = fileBlockNumber - (fileBlockNumber % blocksPerPage);
  interrupts = Disable();
  theState = FindBlockInCache(theFile, fileBlockNumber, TRUE, &ce, &block);
  switch (theState) {
  case CachePageValid:
    *cachePage = ce;
    *blockBase = block;
    Enable(interrupts);
    break;
  case CachePageInvalid:
    ce = GetFreeCachePage(TRUE);
    ce->ioce_PageState = CachePageInvalid;
    ce->ioce_FileUniqueIdentifier = theFile->fi_UniqueIdentifier;
    ce->ioce_Filesystem = theFile->fi_FileSystem;
    ce->ioce_FileBlockOffset = baseBlock;
    ce->ioce_CachedBlockSize = 0;
    ce->ioce_UseCount = 0;
    /* drop through into partial-block case */
  case CachePagePartial:
    blocksInPage = ce->ioce_CachedBlockSize / blockSize;
    firstBlockToRead = baseBlock + blocksInPage;
    blocksToRead = blocksPerPage - blocksInPage;
    blocksLeft = theFile->fi_BlockCount - firstBlockToRead;
    if (blocksToRead > blocksLeft) {
      blocksToRead = blocksLeft;
    }
    if (blocksToRead <= 0) {
      *cachePage = NULL;
      *blockBase = NULL;
      Enable(interrupts);
      return BADIOARG;
    }
    ce->ioce_UseCount ++;
    ce->ioce_PageState = CachePageLoading;
    Enable(interrupts);
    bytesToRead = blocksToRead * blockSize;
    err =
      (*loaderFunction)(theFile, firstBlockToRead, (blocksInPage * blockSize) +
			(char *) ce->ioce_CachedBlock, bytesToRead);
    interrupts = Disable();
    if (err < 0) {
      *cachePage = NULL;
      *blockBase = NULL;
      ce->ioce_PageState = CachePageInvalid;
      ce->ioce_UseCount --;
      Enable(interrupts);
      return BADIOARG;
    }
    ce->ioce_CachedBlockSize += bytesToRead;
    ce->ioce_PageState = CachePageValid;
    Enable(interrupts);
    *cachePage = ce;
    *blockBase = blockSize * (fileBlockNumber - baseBlock) +
      (char *) ce->ioce_CachedBlock;
  }
  return 0;
}

void SetCachePagePriority(IoCacheEntry *cachePage, uint8 pagePrio)
{
  int32 interrupts;
  DBUG(("Set prio of page 0x%X to %d\n", cachePage, pagePrio));
  interrupts = Disable();
  cachePage->ioce.n_Priority = pagePrio;
  RemNode((Node *) cachePage);
  InsertNodeFromHead(&fsCache.ioc_CachedBlocks, (Node *) cachePage);
  Enable(interrupts);
  return;
}

void BumpCachePagePriority(IoCacheEntry *cachePage, uint8 pagePrio)
{
  if (cachePage->ioce_UseCount <= 1 ||
      pagePrio > cachePage->ioce.n_Priority) {
    SetCachePagePriority(cachePage, pagePrio);
  }
}

void AgeCache(void)
{
  IoCacheEntry *entry;
  int32 interrupts;
  interrupts = Disable();
  entry = (IoCacheEntry *) FirstNode(&fsCache.ioc_CachedBlocks);
  while (IsNode(&fsCache.ioc_CachedBlocks, entry)) {
    if (entry->ioce.n_Priority > 0) {
      entry->ioce.n_Priority --;
    }
    entry = (IoCacheEntry *) NextNode(entry);
  }
  Enable(interrupts);
}

/*
   InvalidateCachePage() must be passed a cache page which is exclusively
   reserved by the caller.  It marks the page as being invalid and
   relinquishes the page.
*/

void InvalidateCachePage(IoCacheEntry *cachePage)
{
  int32 interrupts;
  interrupts = Disable();
  if (cachePage->ioce_UseCount != 1) {
    Enable(interrupts);
    DBUG0(("Illegal invalidate of cache page 0x%X\n", cachePage));
    return;
  }
  cachePage->ioce_PageState = CachePageInvalid;
  cachePage->ioce_UseCount = 0;
  CacheWake();
  Enable(interrupts);
}

/*
   InvalidateFileCachePages finds all cache pages holding data from a
   specified file, and marks them invalid.
*/

void InvalidateFileCachePages(File *theFile)
{
  int32 interrupts;
  IoCacheEntry *ce;
  interrupts = Disable();
  if (fsCache.ioc_EntriesAllowed != 0) {
    ce = (IoCacheEntry *) FirstNode(&fsCache.ioc_CachedBlocks);
    while (IsNode(&fsCache.ioc_CachedBlocks, ce)) {
      if (ce->ioce_FileUniqueIdentifier == theFile->fi_UniqueIdentifier &&
	  ce->ioce_Filesystem == theFile->fi_FileSystem) {
	if (ce->ioce_UseCount > 0) {
	  DBUG0(("Invalidate, use count > 0, cache page 0x%X, page %d of %s\n",
		 ce, ce->ioce_FileBlockOffset, theFile->fi_FileName));
	}
	ce->ioce_PageState = CachePageInvalid;
      }
      ce = (IoCacheEntry *) NextNode(ce);
    }
  }
  Enable(interrupts);
}

/*
   InvalidateFilesystemCachePages finds all cache pages holding data from
   any file in a specified filesystem, and marks them invalid.
*/


void InvalidateFilesystemCachePages(FileSystem *fs)
{
  int32 interrupts;
  IoCacheEntry *ce;
  interrupts = Disable();
  if (fsCache.ioc_EntriesAllowed != 0) {
    ce = (IoCacheEntry *) FirstNode(&fsCache.ioc_CachedBlocks);
    while (IsNode(&fsCache.ioc_CachedBlocks, ce)) {
      if (ce->ioce_Filesystem == fs) {
	if (ce->ioce_UseCount > 0) {
	  DBUG0(("Invalidate, use count > 0, cache page 0x%X, page %d of id 0x%X\n",
		 ce, ce->ioce_FileBlockOffset,
		 ce->ioce_FileUniqueIdentifier));
	}
	ce->ioce_PageState = CachePageInvalid;
      }
      ce = (IoCacheEntry *) NextNode(ce);
    }
  }
  Enable(interrupts);
}
