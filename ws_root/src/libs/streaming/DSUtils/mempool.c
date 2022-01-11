/******************************************************************************
**
**  @(#) mempool.c 95/12/13 1.14
**
******************************************************************************/

#include <stddef.h>					/* offsetof() */
#include <kernel/mem.h>				/* AllocMem(), FreeMem() */
#include <kernel/debug.h>			/* FAIL_NIL */

#include <streaming/mempool.h>


#define	ROUND_TO_LONG(size)			(((size) + 3) & ~3)


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name --MemPool-Overview--
|||	Overview of the MemPool module.
|||	
|||	  Background
|||	
|||	    A MemPool holds a fixed number of fixed-sized pool entries (aka
|||	    "nodes" or "members"). Entries can then be repeatedly allocated from the
|||	    pool via AllocPoolMem() and returned to the pool via ReturnPoolMem()
|||	    with just a few instructions and with no risk of memory fragmentation.
|||	
|||	    Typically, the client will initialize the pool entries to save work
|||	    for each reuse of the entries. MemPool will not alter the contents of
|||	    free pool entries.
|||	
|||	    MemPools are handy for data streaming and other real-time processing.
|||	
|||	  Usage Overview
|||	
|||	    Call CreateMemPool() or CreateMemPoolWithOptions() to create (allocate)
|||	    a MemPool. The latter lets you supply AllocMem() options such
|||	    as (MEMTYPE_DMA | MEMTYPE_FILL).
|||	
|||	    If you allocate with MEMTYPE_FILL, the pool memory will be zeroed out.
|||	    To further initialize the pool entries, pass an entry-initialization
|||	    function to ForEachFreePoolMember(). E.g. for a pool of I/O-tracker
|||	    entries, your entry-initialization function could create an IOReq Item,
|||	    call LookupItem() and save the result, and setup IOInfo fields.
|||	
|||	    Call AllocPoolMem() to allocate a node from a MemPool, and call
|||	    ReturnPoolMem() to return the node to the MemPool.
|||	
|||	    Call EnumerateMemPoolEntries() to enumerate ALL nodes in a MemPool.
|||	    This is mainly intended for debugging, e.g. to locate a forgotten pool
|||	    entry.
|||	
|||	    Call DeleteMemPool() to delete (deallocate) a MemPool.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
 ******************************************************************************/

/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name CreateMemPool
|||	  Creates a memory pool (a MemPool).
|||	
|||	  Synopsis
|||	
|||	    MemPoolPtr CreateMemPool(int32 numToPreallocate, int32 sizeOfEntry)
|||	
|||	  Description
|||	
|||	    Creates (allocates) a MemPool by calling
|||	    CreateMemPoolWithOptions(numToPreallocate, sizeOfEntry, MEMTYPE_FILL),
|||	    thus allocating a pool of zeroed-out entries.
|||	
|||	    Call ForEachFreePoolMember() to further initialize the pool entries.
|||	
|||	  Arguments
|||	
|||	    numToPreallocate
|||	        Number of entries in the new MemPool.
|||	
|||	    sizeOfEntry
|||	        Size of each pool entry, in bytes. The size gets rounded up to the
|||	        nearest multiple of 4 bytes so each pool entry will start on a word
|||	        boundary.
|||	
|||	  Return Value
|||	
|||	    A pointer to the new MemPool, or NULL if CreateMemPool failed.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPoolWithOptions(), DeleteMemPool(), AllocPoolMem(),
|||	    ReturnPoolMem(), ForEachFreePoolMember()
|||	
 ******************************************************************************/
MemPoolPtr		CreateMemPool(int32 numToPreallocate, int32 sizeOfEntry)
	{
	return CreateMemPoolWithOptions(numToPreallocate, sizeOfEntry, MEMTYPE_FILL);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name CreateMemPoolWithOptions
|||	  Creates a memory pool (a MemPool) using mem-type options.
|||	
|||	  Synopsis
|||	
|||	    MemPoolPtr CreateMemPoolWithOptions(int32 numToPreallocate,
|||	        int32 sizeOfEntry, uint32 memtypeOptions)
|||	
|||	  Description
|||	
|||	    Creates (allocates) a MemPool. You can supply AllocMem() options such
|||	    as (MEMTYPE_DMA | MEMTYPE_FILL).
|||	
|||	    MemPools support data streaming and other real-time processing.
|||	
|||	    If you allocate with MEMTYPE_FILL, the pool memory will be zeroed out.
|||	    To further initialize the pool entries, pass an entry-initialization
|||	    function to ForEachFreePoolMember(). E.g. for a pool of I/O-tracker
|||	    entries, your entry-initialization function could create an IOReq Item,
|||	    call LookupItem() and save the result, and setup IOInfo fields.
|||	
|||	  Arguments
|||	
|||	    numToPreallocate
|||	        Number of entries to create in the new MemPool.
|||	
|||	    sizeOfEntry
|||	        Size to create each pool entry, in bytes. Each pool entry will start
|||	        on a mod-4 byte boundary.
|||	
|||	    memtypeOptions
|||	        AllocMem() options such as (MEMTYPE_DMA | MEMTYPE_FILL).
|||	
|||	  Return Value
|||	
|||	    A pointer to the new MemPool, or NULL if CreateMemPoolWithOptions failed.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPool(), DeleteMemPool(), AllocPoolMem(), ReturnPoolMem(),
|||	    ForEachFreePoolMember(), AllocMem()
|||	
 ******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name DeleteMemPool
|||	  Deletes a MemPool.
|||	
|||	  Synopsis
|||	
|||	    void DeleteMemPool(MemPoolPtr memPool)
|||	
|||	  Description
|||	
|||	    Deletes (deallocates) the specified MemPool by calling FreeMem().
|||	
|||	  Arguments
|||	
|||	    memPool
|||	        The MemPool to delete.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPool(), FreeMem()
|||	
*******************************************************************************/
void			DeleteMemPool(MemPoolPtr memPool)
	{
	if ( memPool != NULL )		/* so memPool->totalPoolSize is safe */
		FreeMem(memPool, memPool->totalPoolSize);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name AllocPoolMem
|||	  Allocates an entry from a MemPool.
|||	
|||	  Synopsis
|||	
|||	    void *AllocPoolMem(MemPoolPtr memPool)
|||	
|||	  Description
|||	
|||	    Allocates an entry (or "node" or "member") from a MemPool.
|||	    This is implemented as a very fast singly-linked list operation.
|||	
|||	  Arguments
|||	
|||	    memPool
|||	        The MemPool to allocate from.
|||	
|||	  Return Value
|||	
|||	    A pointer to the entry data allocated from the MemPool.
|||	    If there are no free entries, this returns NULL.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPool(), ReturnPoolMem()
|||	
*******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name ReturnPoolMem
|||	  Returns an entry to a MemPool.
|||	
|||	  Synopsis
|||	
|||	    void ReturnPoolMem(MemPoolPtr memPool, void* poolEntry)
|||	
|||	  Description
|||	
|||	    Returns an entry to a MemPool, i.e. "deallocates" the entry back to the
|||	    pool. This is implemented as a very fast singly-linked list operation.
|||	    ASSUMES: memPool is the MemPool that poolEntry was allocated from.
|||	
|||	  Arguments
|||	
|||	    memPool
|||	        The MemPool to return poolEntry to.
|||	
|||	    poolEntry
|||	        The MemPool entry to return to memPool.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    AllocPoolMem(), CreateMemPool()
|||	
*******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name ForEachFreePoolMember
|||	  Enumerate all free entries in a MemPool, e.g. for initialization.
|||	
|||	  Synopsis
|||	
|||	    bool ForEachFreePoolMember(MemPoolPtr memPool,
|||	        ForEachFreePoolMemberFuncPtr forEachFunc, void *argValue)
|||	
|||	  Description
|||	
|||	    Applies the function forEachFunc to each free entry in a MemPool. For
|||	    each free entry in the MemPool, this passes the input argValue and the
|||	    entry data pointer to forEachFunc.
|||	    
|||	    This is handy for initializing the entries in a new pool and for cleaning
|||	    up the entries in a pool about to be deleted.
|||	
|||	  Arguments
|||	
|||	    memPool
|||	        The MemPool to scan.
|||	
|||	    forEachFunc
|||	        Function to apply to each free entry in the pool:
|||	        forEachFunc(void *argValue, void *poolEntry). If forEachFunc( )
|||	        returns FALSE, ForEachFreePoolMember() will immediately stop
|||	        scanning the pool and will return FALSE.
|||	
|||	    argValue
|||	        Client data argument to pass through to forEachFunc. This lets you
|||	        supply context info to forEachFunc( ).
|||	
|||	  Return Value
|||	
|||	    If forEachFunc( ) ever returns FALSE, ForEachFreePoolMember() will
|||	    immediately return FALSE. Otherwise ForEachFreePoolMember() will
|||	    return TRUE. (If quitting the scan early leaves the pool entries in an
|||	    incomplete state, it's up to the caller to clean up any side effects.)
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPool(), EnumerateMemPoolEntries()
|||	
*******************************************************************************/
bool	ForEachFreePoolMember(MemPoolPtr memPool,
			ForEachFreePoolMemberFuncPtr forEachFunc, void *argValue)
	{
	MemPoolEntryPtr	entry;

	for ( entry = memPool->availList; entry != NULL; entry = entry->next )
		if ( !(*forEachFunc)(argValue, (void *)&entry->data) )
			return FALSE;

	return TRUE;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group MemPool -name EnumerateMemPoolEntries
|||	Enumerate all entries in a MemPool.
|||	
|||	  Synopsis
|||	
|||	    bool EnumerateMemPoolEntries(MemPoolPtr pool,
|||	        ForEachPoolMemberFuncPtr forEachFunc, void *argValue)
|||	
|||	  Description
|||	
|||	    Applies the function forEachFunc to each entry in a MemPool. For
|||	    each entry in the MemPool, this passes the input argValue, the
|||	    entry data pointer, and an in-use bool to forEachFunc.
|||	    
|||	    This is mainly intended for debugging, e.g. to locate a forgotten pool
|||	    entry.
|||	
|||	  Arguments
|||	
|||	    pool
|||	        MemPool to scan.
|||	
|||	    forEachFunc
|||	        Function to apply to each entry in the pool:
|||	        forEachFunc(void *argValue, void *poolEntry). If forEachFunc( )
|||	        returns FALSE, EnumerateMemPoolEntries() will immediately stop
|||	        scanning the pool and will return FALSE.
|||	
|||	    argValue
|||	        Argument string passed to the function.
|||	
|||	  Return Value
|||	
|||	    If forEachFunc( ) ever returns FALSE, EnumerateMemPoolEntries() will
|||	    immediately return FALSE. Otherwise EnumerateMemPoolEntries() will
|||	    return TRUE.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mempool.h>, libdsutils.a
|||	
|||	  See Also
|||	
|||	    CreateMemPool(), ForEachFreePoolMember()
|||	
*******************************************************************************/
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
