/******************************************************************************
**
**  @(#) mempool.c 96/08/01 1.2
**
******************************************************************************/

#include <stddef.h>					/* offsetof() */
#include <kernel/mem.h>				/* AllocMem(), FreeMem() */
#include <kernel/debug.h>			/* FAIL_NIL */

#include <streaming/mempool.h>


#define	ROUND_TO_LONG(size)			(((size) + 3) & ~3)


MemPoolPtr		CreateMemPool(int32 numToPreallocate, int32 sizeOfEntry)
	{
	return CreateMemPoolWithOptions(numToPreallocate, sizeOfEntry, MEMTYPE_FILL);
	}


MemPoolPtr		CreateMemPoolWithOptions(int32 numToPreallocate, int32 sizeOfEntry,
					uint32 memtypeOptions)
	{
	MemPoolPtr		pool;
	MemPoolEntryPtr	entryPtr;
	MemPoolEntryPtr	lastEntryPtr;
	int32			totalPoolSize, entryAllocnSize;

	/* Round up the pool entry size to whole word, to insure proper alignment
	 * of struct fields in every pool entry, and add the header size
	 * without including the dummy size of the data[] array. */
	entryAllocnSize = MEM_POOL_ENTRY_HEADER_SIZE + ROUND_TO_LONG(sizeOfEntry);

	/* Allocate one block of memory for the mem pool header and all its entries
	 * without including the dummy size of the data[] array. */
	totalPoolSize = MEM_POOL_HEADER_SIZE + numToPreallocate * entryAllocnSize;
	pool = (MemPoolPtr)AllocMem(totalPoolSize, memtypeOptions);

	if ( pool != NULL )
		{
		pool->numItemsInPool	= numToPreallocate;
		pool->numFreeInPool		= numToPreallocate;
		pool->totalPoolSize		= totalPoolSize;

		/* Parcel the data area up into a linked list of pool entries. */
		entryPtr = &pool->data[0];
		lastEntryPtr = NULL;
		while ( numToPreallocate-- > 0 )
			{
			entryPtr->next = lastEntryPtr;
			lastEntryPtr = entryPtr;

			entryPtr = (MemPoolEntryPtr)( ((uint8 *)entryPtr) + entryAllocnSize );
			}

		/* Point to the first entry in the free list */
		pool->availList = lastEntryPtr;
		}

	return pool;
	}


void			DeleteMemPool(MemPoolPtr memPool)
	{
	if ( memPool != NULL )		/* so memPool->totalPoolSize is safe */
		FreeMem(memPool, memPool->totalPoolSize);
	}


void *AllocPoolMem(MemPoolPtr memPool)
	{
	MemPoolEntryPtr		entry;
	void				*result = NULL;

	/* Pull the first available entry from the free list, if any. */
	entry = memPool->availList;
	if ( entry != NULL )
		{
		memPool->availList = entry->next;
		entry->next = NULL;		/* mark the entry as "in use" */
		memPool->numFreeInPool--;
		result = &entry->data;
		}

	return result;
	}


void	ReturnPoolMem(MemPoolPtr memPool, void* poolEntry)
	{
	MemPoolEntryPtr		entry;

	FAIL_NIL("ReturnPoolMem", poolEntry);

	/* Weird cast to back up the pointer we are handed to the REAL
	 * beginning of the type of structure we manage. */
	entry = (MemPoolEntryPtr)( ((uint8 *)poolEntry) - offsetof(MemPoolEntry, data) );

	/* Put the returning entry point on the front of the free list. */
	entry->next = memPool->availList;
	memPool->availList = entry;
	memPool->numFreeInPool++;

FAILED:
	return;
	}


bool	ForEachFreePoolMember(MemPoolPtr memPool,
			ForEachFreePoolMemberFuncPtr forEachFunc, void *argValue)
	{
	MemPoolEntryPtr	entry;

	for ( entry = memPool->availList; entry != NULL; entry = entry->next )
		if ( !(*forEachFunc)(argValue, (void *)&entry->data) )
			return FALSE;

	return TRUE;
	}


bool EnumerateMemPoolEntries(MemPoolPtr pool,
			ForEachPoolMemberFuncPtr forEachFunc, void *argValue)
	{
	MemPoolEntryPtr	entry;
	MemPoolEntryPtr	beyondTheLastEntry =
		(MemPoolEntryPtr)((uint8 *)pool + pool->totalPoolSize);
	const int32		entryAllocnSize =
		(pool->totalPoolSize - MEM_POOL_HEADER_SIZE) / pool->numItemsInPool;

	for ( entry = &pool->data[0]; entry < beyondTheLastEntry;
			entry = (MemPoolEntryPtr)( ((uint8 *)entry) + entryAllocnSize ) )
		if ( !(*forEachFunc)(argValue,	/* argValue */
				(void *)&entry->data,	/* poolEntry */
				entry->next == NULL 	/* fInUse */) )
			return FALSE;

	return TRUE;
	}
