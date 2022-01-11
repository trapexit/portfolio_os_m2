/*
	File:		allocator.cpp

	Contains:	memory allocator.  allocate memory within the applications's heap
				whenever possible, otherwise try in MF Temp mem

	 	This code may be used as a replacement for CPlusPlus's operators new and delete, 
	 	std C's malloc and free.  If you use this code to replace new/delete or malloc/free
	 	in and MPW tool, DO NOT USE the "FreeAllBlocks()" function as the MPWRuntime library
	 	makes calls to new/delete AND malloc/free both before your main will is called and
	 	after your main exits (and after all "atexit()" procs are called).
	 	- This code allocates a 'pool' of memory from the OS and divides it up as 
	 	  malloc/new are called for blocks BELOW a threshold size.  Allocation requests
	 	  ABOVE the threshold it passes directly to the OS.  
	 	- A new pool is allocated whenever none of it's pools has enough contiguous free 
	 	  space for an allocation request.
	 	- Pools are allocated in the application heap as long as it has space, then in the 
	 	  system's temporary heap.
	 	- When delete/free is called it joins the block being deleted with any adjacent free 
	 	  block(s) in it's pool and marks it as free for reallocation.  
	 	- Whenever ALL chunks in a pool have been marked s free, it frees the pool back to the OS.
	 	
	 based on "CustomNew.cp" by François Pottier (pottier@clipper.ens.fr)

	Written by:	eric carlson
				Software Attic

	Copyright:	© 1995 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

	09/22/95		ec		Added hack to allow global C++ objects to safely free allocated 
							 memory in a destructor.
	09/08/95		ec		New today.

	To Do:
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <memory.h>

#include <OSUtils.h>

#include "allocator.h"

// ================================ COMPILE SWITCHES ===================================

#undef	TEST_ALLOC

#define	EXIT_IF_NO_MEM		// call "exit(0)" when about to return a NULL pointer (out of memory)

#undef	VERBOSE				// if defined, this code spits out LOTS of diagnostic info
#define	DEBUG				// extra, sanity checks
#undef	PARANOID_CHECKS		// check for minimum stack and heap space before every allocation
#define	QUAD_BYTE_ALIGN		// always return quad byte aligned ptrs


// two types of error printing macros
#ifdef DEBUG
	#define		PANIC_ERROR(str)	{ printf str; exit(0);}	// prints the message and exits from the tool
	#define		PRINT_ERROR(str)	( printf str )			// simply print the error message
#else
	#define		PANIC_ERROR(str)	{exit(0);}				// don't print the message just exit
	#define		PRINT_ERROR(str)	((void)0)				// do nothing
#endif


#ifdef VERBOSE
	#define		VERBAGE(str)		( printf str )
#else
	#define		VERBAGE(str)		((void)0)
#endif



// ==================================== typedefs/defines ========================================
enum 
{
	kPoolSize 			= 65536L,			// Memory pools are allocated in 64K chunks
	kBiggestPoolBlock	= 4048L				// Any allocations bigger than 4K go through
											//  the OS directly
};

#ifdef DEBUG
enum
{
	kPoolHeader		= 'POOL',
	kBlockHead		= 'BLOK'
};
#endif

#ifndef	uint32
typedef	unsigned long	uint32;
#endif	// uint32 

#ifndef	int32
typedef	long			int32;
#endif	// int32 

// Free blocks are linked together in a list
typedef struct FreeMemList 
{								
	struct FreeMemList	*next;				// Pointer to next free block
	int32				size;				// Size of this block
} FreeMemList;

// Memory is allocated from pools (locked handles)
typedef struct MemPool 
{		
#ifdef DEBUG
	uint32			headerCookie;		// magic value to assure this is a pool
	int32			freeBlocks;				// statistics
#endif
	struct MemPool	*next;					// pointer to next pool
	FreeMemList		*freeHead;				// start of free blocks list
	int32			usedBlocks; 
} MemPool;


// structure we add to a block
typedef struct MemBlock 
{								
#ifdef DEBUG
	uint32			headerCookie;		// magic value to assure this is a pool
#endif
	struct MemBlock	*next;					// not used by pool allocated block, for OS blocks it points to next block
	int32			size;					// size of pool allocate block, or -1 if allocated by OS
#ifdef QUAD_BYTE_ALIGN
	uint32			paddingSize;			// magic value point allow us to find structure on Free()
#endif
} MemBlock;

#ifdef PARANOID_CHECKS
enum 
{
	kMinStackSpace		= (8 * 1024),		// bail if stack space is < 8k
	kMinHeapSpace		= (8 * 1024)		// bail if heap space is < 8k
};
#endif

#define	kReserveForMPW		(1024L * 64)	// we TRY to leave MPW this much room

// handy macros for setting and getting the quad alignedness of a number
#define	QUADBYTE_ALIGN_DOWN(num)		(((uint32)(num)) & ~0x03UL)
#define	QUADBYTE_ALIGN_UP(num)			((((uint32)(num)) + 0x03UL) & ~0x03UL)
#define	IS_QUADBYTE_ALIGNED(num)		((Boolean)(0 == ((uint32)(num) & 0x03UL)))

// ============================== private prototypes ===================================
static	void		InitMemoryFcns(void);
static	void		FinalShutDown(void);

static	Handle 		MakeLockedHandle(int32 size);
static	Handle		RecoverHandleFromOSBlock(MemBlock *osBlock);
static	Boolean		NewPool(void);
static	MemBlock	*AllocFromPool(MemPool* pool, int32 size);
static	void		DeleteFromPool(MemPool* pool, MemBlock *block, int32 size);
static	void		FreePoolBlock(MemBlock *pointer);
static	void		*AllocateOSBlock(size_t size);
static	void		*AllocatePoolBlock(size_t size);

static	void		PrintPool(MemPool *pool);
static	void		PrintAllPools(void);

#ifdef PARANOID_CHECKS
static	void		CheckAvailMem(void);
#endif

// ==================================== globals ========================================
// global for mem pools
MemPool			*gPoolBlockList	= NULL;				// list of pools
MemBlock		*gOSBlockList	= NULL;


#ifdef PARANOID_CHECKS
static	Ptr				gSafetyBlock	= NULL;
#endif

#ifdef DEBUG
	int32			gOSBlockCount	= 0;
	int32			gOSBlockSize	= 0;
	int32			gPoolBlockCount	= 0;
	int32			gPoolBlockSize	= 0;
#endif

static	Boolean		gFirstTime		= true;				// first time routine being called?





//
// do all required initial setup
static void
InitMemoryFcns(void)
{
	if ( true == gFirstTime )
	{
			gFirstTime = false;
		
		// make sure we free all memory before the program (or tool) quits.  Set an "atexit()"
		//  function if this is being compiled with the C compiler, we will get a call after
		//  "main()" exits to free any allocated memory.  if this is being built with the 
		//  C++ compiler this method isn't good enough as global objects get destroyed AFTER
		//  all "atexit()" functions have been called, so if any global objects constructor
		//  allocated memory via one of our functions, it will have been freed by time it's
		//  destructor is called.  SO, if this file is being compiled by the C++ compiler, 
		//  we have our own copy of "__destroy_global_chain" so we can free memory AFTER
		//  every global object has been destroyed
		atexit(FinalShutDown);

#ifdef PARANOID_CHECKS
		// allocate a static block so we have SOME free memory to report an error
		//  if we run out of memory
		gSafetyBlock = NewPtr(kMinHeapSpace / 2);
#endif

	}
}

static void
FinalShutDown(void)
{
#ifdef PARANOID_CHECKS
	if ( NULL != gSafetyBlock )	// free the safety block
	{
		DisposePtr(gSafetyBlock);
		gSafetyBlock = NULL;
	}
#endif
	FreeAllBlocks();
}



//
// Create a locked handle from the application heap or the system's temporary zone
//  if the application's heap is out of room
static Handle 
MakeLockedHandle(int32 size)
{
	Handle			reservedMem;
	Handle			hndl;
	OSErr			err;

	// make sure we leave some memory for MPW to do it's stuff.  if we fail don't even bother 
	//  trying to allocate the handle in the application heap
	if ( NULL == (reservedMem = NewHandle(kReserveForMPW)) )
	{
#if 0 /* <Reddy 9-22-95> */
		PANIC_ERROR(("ERROR: MakeLockedHandle() unable to reserve %ld byte buffer for MPW,"\
		" watchout...\n", (int32)kReserveForMPW));
#endif
		goto TRY_TEMPMEM;
	}
	
	// grab a block of the requested size, if we don't have room in our heap get one
	//  from the system
	hndl = NewHandle(size);
	if ( NULL == hndl )
TRY_TEMPMEM:
		hndl = TempNewHandle(size, &err);
	if ( NULL != hndl )
		HLock(hndl);

	// free the handle we reserved for MPW, all done
	if ( NULL != reservedMem )
		DisposeHandle(reservedMem);
	return hndl;
}


//
// recover the handle allocated for an os block
static Handle
RecoverHandleFromOSBlock(MemBlock *osBlock)
{
	Handle		originalHndl;

#ifdef QUAD_BYTE_ALIGN
	// see if we had to add extra padding to the struct to align it
	if ( 0 != osBlock->paddingSize )
		originalHndl = RecoverHandle((Ptr)((char *)osBlock - osBlock->paddingSize));
	else
		originalHndl = RecoverHandle((Ptr)osBlock);
#else
	originalHndl = RecoverHandle((Ptr)osBlock);
#endif
	return originalHndl;
}


//
// Add a new pool to the pool list
static Boolean 
NewPool(void)
{
	Handle		hndl;
	MemPool		*newPool;
	void		*freeBlock;
	
	// try to get a new handle for the pool
	if ( NULL == (hndl = MakeLockedHandle(kPoolSize)) )
		return false;
	
	// OK, that worked.  use the first part of the allocated block a MemPool.  Hook it 
	//  into to the other pools by make it the new queue head
	newPool = (MemPool *)*hndl;
	newPool->next = gPoolBlockList;
	gPoolBlockList = newPool;
	
	// make sure the first block we use is quad aligned since the OS may give us a 16 byte
	//  aligned block.  no need to worry about alignment after this point since we always
	//  pad an allocation request to make it's size quad aligned
	freeBlock = (void *)((char *)newPool + sizeof(MemPool));
	freeBlock = (void *)QUADBYTE_ALIGN_UP((char *)freeBlock);
	
	// all data is free, make it one block
	newPool->freeHead = (FreeMemList *)freeBlock;
	newPool->freeHead->next = NULL;
	newPool->freeHead->size = (char *)newPool + kPoolSize - (char *)freeBlock;

#ifdef QUAD_BYTE_ALIGN
	// since we parcel out chunks from the END of a mempool based on the size of the 
	//  free chunk, we need to make sure that we start off with a free size which is a
	//  quad multiple or every block allocated from the pool will NOT BE aligned.  round
	//  the size of this free block down to the nearest quad multiple
	newPool->freeHead->size = QUADBYTE_ALIGN_DOWN(newPool->freeHead->size);
#endif

	newPool->usedBlocks = 0;
#ifdef DEBUG
	newPool->freeBlocks = 1;
	newPool->headerCookie = kPoolHeader;					// magic value to assure this is a block
	++gPoolBlockCount;
	gPoolBlockSize += kPoolSize;
#endif

	VERBAGE(("allocated new pool 0x%lp\n", newPool));

	return true;
}


//
// allocate a block from a specific pool
static MemBlock * 
AllocFromPool(MemPool* pool, int32 size)
{
	FreeMemList		*freeBlock;				// The block under consideration
	FreeMemList		**link;					// The place which points to block in the free blocks list
	MemBlock		*result;
	
#ifdef DEBUG
	// validate that the magic value at the pool head is still there!
	if ( kPoolHeader != pool->headerCookie )
	{
		PANIC_ERROR(("AllocFromPool() passed invalid pool 0x%p\n", pool));
		return NULL;
	}
#endif

	// Walk the free blocks list looking for one large enough
	for ( link = &pool->freeHead; (freeBlock = *link); link = &freeBlock->next )
	{
		// is this block big enough
		if ( size <= freeBlock->size )
		{
			// found a block big enough, either split it or use the wole thing
			if ( freeBlock->size >= size + sizeof(FreeMemList) )//  we can split it, don't need the whole thing
			{
				freeBlock->size -= size;						// The low part remains a free block
				result = (MemBlock *)((char *)freeBlock + freeBlock->size);	// The high part becomes allocated
			}
			else 
			{													// If it's just big enough, we can't split it,
				*link = freeBlock->next;						//  so link around the, now used, block,
				result = (MemBlock *)freeBlock;					//  remove it from the free blocks list,
#ifdef DEBUG
				pool->freeBlocks--;								//  and mark it all as allocated
#endif
			}
			
			// found a block, 
			pool->usedBlocks++;									// remember we have one more used in this pool
#ifdef DEBUG
			result->headerCookie = kBlockHead;					// magic value to assure this is a block
#endif
			result->next = 0L;									//  pool blocks don't use this field
			result->size = size;								//  store the block's size in the first four bytes,
			return result;										//  let the user have the rest
		}
	}
	
	return NULL;
}


/* We assume the free blocks list to be always ordered from bottom to top, and it is because we insert new free */
/* blocks in the right place. This allows us to easily join adjacent free blocks into a single one when deleting */
/* a block, thus reducing fragmentation. */
static void 
DeleteFromPool(MemPool* pool, MemBlock *freeBlock, int32 size)
{
	FreeMemList		*block,
					*previous,
					*next;
	FreeMemList		**link;
	char 			*pointer = (char *)freeBlock;

#ifdef DEBUG
	// validate that the magic value at the pool head is still there!
	if ( kPoolHeader != pool->headerCookie )
	{
		PANIC_ERROR(("DeleteFromPool() passed invalid pool 0x%p\n", pool));
		return;
	}
	if ( kBlockHead != freeBlock->headerCookie )
	{
		PANIC_ERROR(("DeleteFromPool() passed invalid block 0x%p\n", freeBlock));
		return;
	}
#endif

	/* walk the free block list, lookin for free blocks adjacent to the one we're */
	/*  freeing now */
	previous = next = NULL;									
	for ( link = &pool->freeHead; NULL != (block = *link); link = &block->next )
	{
		/* a free block just before ours */
		if ( (char *)pointer == (char *)block + block->size )
			previous = block;
		/* a free block just after ours */
		else if ( (char *)pointer + size == (char *) block )
		{
			next = block;
			goto COALESCE_BLOCKS;		/* Stop searching now, there's nothing more up there and we will need the value of link */
		}
		/* no adjacent blocks... */
		else if ( (char *)pointer + size < (char *) block )
			goto COALESCE_BLOCKS;		/* The current value of link tells us where to insert the new free block */
	}
		
COALESCE_BLOCKS:
	pool->usedBlocks--;

	/* if we found a previous and a next free block, we now have 3 adjacent free blocks, join them */
	if ( (NULL != previous) && (NULL != next) )
	{
		previous->size += size + next->size;
		previous->next = next->next;
#ifdef DEBUG
		pool->freeBlocks--;
#endif
		VERBAGE(("  coalescing 3 blocks new size = %ld\n", previous->size));
	}
	/* if we only found a previous free block, expand it to include the one we're freeing */
	else if ( NULL != previous )
	{
		previous->size += size;
		VERBAGE(("  found previous free block coalescing, new size = %ld\n", previous->size));
	}
	/* if we only found a free block just after the one we're freeing, expand the one we're */
	/*  freeing to include it */
	else if ( NULL != next )
	{
		((FreeMemList*) pointer)->next = next->next;
		((FreeMemList*) pointer)->size = next->size + size;
		*link = (FreeMemList*) pointer;
		VERBAGE(("  found next free block coalescing, new size = %ld\n", next->size + size));
	}
	/* the block we're freeing is surrounded by allocated blocks, simply add it to the free list */
	else
	{
		((FreeMemList*) pointer)->next = *link;
		((FreeMemList*) pointer)->size = size;
		*link = (FreeMemList*) pointer;
#ifdef DEBUG
		pool->freeBlocks++;
#endif
		VERBAGE(("  no neighbor blocks found\n"));
	}	
}


//
// ask the OS to allocate a block for us, link it into our list
static void *
AllocateOSBlock(size_t size)
{
	MemBlock	*blockPtr;
	Handle		hndl;

#ifdef QUAD_BYTE_ALIGN
	// make sure the we have enough space to quad align the ptr address
	size += 2;
#endif

	// first make a handle
	if ( NULL == (hndl = MakeLockedHandle(size)) )
	{
		PRINT_ERROR(("ERROR: unable to allocate OSBlock (request size = %ld)\nreturning NULL\n", size));
#ifdef EXIT_IF_NO_MEM
		exit(0);
#endif
		return NULL;
	}

	// cast the deference handle to a MemBlock, take care of alignment if need be
	blockPtr = (MemBlock *)*hndl;

#ifdef QUAD_BYTE_ALIGN
	// now, if the ptr we get from the OS isn't quad aligned, bump forward two bytes
	//  so the ptr we return will be aligned correctly.  remember this so we can find
	//  the start of the block when we go to free it
	if ( false == IS_QUADBYTE_ALIGNED(blockPtr) )
	{
		blockPtr = (MemBlock *)(((char *)blockPtr) + 2);
		blockPtr->paddingSize = 2;
	}
	else
		blockPtr->paddingSize = 0;
#endif	// QUAD_BYTE_ALIGN

	// and put it at the head of the list...
	blockPtr->next			= gOSBlockList;
	gOSBlockList			= blockPtr;
	blockPtr->size			= -1L;
#ifdef DEBUG
	blockPtr->headerCookie	= kBlockHead;					// magic value to assure this is a valid block
	gOSBlockSize			+= size;
	++gOSBlockCount;
#endif
	VERBAGE(("OS allocated a block at 0x%p, size = %ld\n", blockPtr, size));
	return (void *)((char *)blockPtr + sizeof(MemBlock));
}


//
// allocate a block from one of our pools
static void *
AllocatePoolBlock(size_t size)
{
	MemBlock	*blockPtr;
	MemPool		*pool;

	/* Try to find a pool with enough room.  our strategy is to look in each pool in succession until we */
	/*  find one with enough space.  if we make it through to the end of the list we'll make a new pool */
	/*  and get it from there */
	pool = gPoolBlockList;
	while ( NULL != pool ) 
	{
		if ( blockPtr = AllocFromPool(pool, size) )
		{
			VERBAGE(("allocated a pool block at 0x%p, size = %ld\n", blockPtr, size));
			goto SUCCESS;
		}
		pool = pool->next;
	}
	
	/* we only get here if NONE of the pools has enough room.  try to make another pool and */
	/*  try again */
	if ( true == NewPool() )
	{
		if ( NULL != (blockPtr = AllocFromPool(gPoolBlockList, size)) )
		{
			VERBAGE(("allocated a pool block at 0x%p, size = %ld\n", blockPtr, size));
			goto SUCCESS;
		}
	}

	PRINT_ERROR(("ERROR: unable to find space in existing pools, or to allocate new pool (request size = %ld)\nreturning NULL\n", size));
#ifdef EXIT_IF_NO_MEM
	exit(0);
#endif
	
	return NULL;
	
SUCCESS:
	return (void *)((char *)blockPtr + sizeof(MemBlock));
}



//
// free a block we had the OS allocate for us
static void
FreeOSBlock(MemBlock *osBlock)
{

	MemBlock	**link;
	MemBlock	*tempOSBlock;
	Handle		originalHndl = RecoverHandleFromOSBlock(osBlock);
#if (defined DEBUG || defined VERBOSE)
	int32		size;
#endif

	size = GetHandleSize(originalHndl);

	// find the block in the list of all blocks allocated for us by the OS
	link = &gOSBlockList;
	while ( (NULL != (tempOSBlock = *link)) && (osBlock != tempOSBlock) )
		link = &tempOSBlock->next;
	if ( NULL == tempOSBlock )
	{
		PANIC_ERROR(("ERROR: FreeOSBlock() passed block which it doesn't own (0x%lp)\n", osBlock));
		goto DONE;
	}

#ifdef DEBUG
	// validate that the magic value at the osBlock head is still there
	if ( kBlockHead != osBlock->headerCookie )
	{
		PANIC_ERROR(("FreeOSBlock() passed invalid block 0x%p\n", osBlock));
		goto DONE;
	}
	if ( (size < kBiggestPoolBlock) || (size > 0x7FFFFFF0) )
		PANIC_ERROR(("Something wrong, found an \"OS allocated\" block %ld bytes long\n", size));
#endif

	VERBAGE(("freeing an OS allocated block 0x%p, size = %ld\n", osBlock, size));

	// 'link' is a pointer the previous block's 'next' ptr, or the global head. 
	//  link around this block to it from the queue,
	*link = osBlock->next;
	DisposeHandle(originalHndl);

#ifdef DEBUG
	--gOSBlockCount;
	gOSBlockSize -= size;
#endif

DONE:
	return;
}


//
// free a block from one of our pools
static void
FreePoolBlock(MemBlock *poolBlock)
{
	MemPool		*pool;
	MemPool		**link;

	if ( (poolBlock->size > kBiggestPoolBlock) || (poolBlock->size <= 0) )
	{
		PANIC_ERROR(("ERROR: FreePoolBlock() passed a pool block with impossible size (%ld bytes)\n", poolBlock->size));
		goto DONE;
	}

	/* find which pool the block belongs to */
	link = &gPoolBlockList;
	while ( (pool = *link) 
			&& ((char *)poolBlock < (char *)pool || (char *)poolBlock > ((char *)pool + kPoolSize)) )
	{
		link = &pool->next;
	}

	if ( NULL == pool )
	{
		PANIC_ERROR(("ERROR: FreePoolBlock() can't find valid pool for block 0x%lp\n", poolBlock));
		goto DONE;
	}
	VERBAGE(("freeing a pool allocated block 0x%p, size = %ld\n", poolBlock, poolBlock->size));

	/* delete the block from this pool, if this was the last block in the pool, free the pool now */
	DeleteFromPool(pool, poolBlock, poolBlock->size);
	if ( pool->usedBlocks == 0 )
	{
		VERBAGE(("freeing empty pool 0x%lp\n", pool));
		/* 'link' is a pointer the previous block's 'next' ptr, or the global head link */
		*link = pool->next;
		DisposeHandle(RecoverHandle((Ptr)pool));
#ifdef DEBUG
		--gPoolBlockCount;
		gPoolBlockSize -= poolBlock->size;
#endif
	}

DONE:
	return;
}


//
// free a block we allocated
void 
FreeBlock(void *pointer)
{
	MemBlock	*memBlock;

	// freeing a NULL pointer does nothing
	if ( NULL == pointer ) 
		return;

	memBlock = (MemBlock *)((char *)pointer - sizeof(MemBlock));

	// see if it is a block in one of our pools, or one the OS allocated
	if ( memBlock->size == -1L )
		FreeOSBlock(memBlock);
	else
		FreePoolBlock(memBlock);
}


//
// delete ALL memory allocated by these routines
void
FreeAllBlocks(void)
{
	MemPool		*pool,
				*nextPool;
	MemBlock	*osBlock,
				*nextOSBlock;

#ifdef VERBOSE
	int32	count = 0;
	VERBAGE(("Freeing all pools, head at 0x%lp:\n", gPoolBlockList));
#endif

	// first the pools
	pool = gPoolBlockList;
	while ( NULL != pool )
	{
		nextPool = pool->next;
#ifdef VERBOSE
		VERBAGE((" Pool %ld\n", ++count));
		PrintPool(pool);
		VERBAGE(("\n"));
#endif
#ifdef DEBUG
		--gPoolBlockCount;
		gPoolBlockSize -= GetHandleSize(RecoverHandle((Ptr)pool));
#endif
		DisposeHandle(RecoverHandle((Ptr)pool));
		pool = nextPool;
	}
	gPoolBlockList = NULL;

	// now the OS blocks
	osBlock = gOSBlockList;
#ifdef VERBOSE
	count = 0;
	VERBAGE(("Freeing all OS allocated blocks, head at 0x%lp:\n", osBlock));
#endif
	while ( NULL != osBlock )
	{
		VERBAGE(("  block %ld", count));
		nextOSBlock = osBlock->next;
		FreeOSBlock(osBlock);
		osBlock = nextOSBlock;
	}
	gOSBlockList = NULL;
}



#ifdef DEBUG
//
// print a detailed description of a pool
static void
PrintPool(MemPool *pool)
{
	FreeMemList		*freeBlock;
	MemBlock		*block;
	MemBlock		*nextFree;

	printf("  Pool at 0x%lp, should have %ld used blocks, %ld free blocks:\n", pool, pool->usedBlocks, pool->freeBlocks);
	for ( freeBlock = pool->freeHead; NULL != freeBlock; freeBlock = freeBlock->next )
	{
		printf("    free block at 0x%lp, size = %ld\n", freeBlock, freeBlock->size);
		
		// the next run of "in use" blocks ends at the next free block, or the end
		//  of the current pool
		if ( NULL != freeBlock->next )
			nextFree =  (MemBlock *)freeBlock->next;
		else
			nextFree = (MemBlock *)((char *)pool + kPoolSize);
		for ( block = (MemBlock *)((char *)freeBlock + freeBlock->size);
				block < nextFree;
				block = (MemBlock *)((char *)block + block->size) )
		{
			printf("    allocated block at 0x%lp, size = %ld\n", block, block->size);
		}
	}
}


//
// print descriptions of all pools
static void
PrintAllPools(void)
{
	MemPool		*pool;
	uint32		count;
	MemBlock	*osBlock = gOSBlockList,
				*nextOSBlock;

	pool = gPoolBlockList;
	printf("\n*********************\nPrinting all pools, head at 0x%lp:\n", pool);
	while ( NULL != pool )
	{
		PrintPool(pool);
		pool = pool->next;
	}

	count = 0;
	printf("Freeing all OS allocated blocks, head at 0x%lp:\n", osBlock);
	while ( NULL != osBlock )
	{
		nextOSBlock = osBlock->next;
		printf("    block %ld at 0x%lp, size = %ld\n", ++count, osBlock, GetHandleSize(RecoverHandle((Ptr)osBlock)));
#ifdef DEBUG
	--gOSBlockCount;
	gOSBlockSize -= GetHandleSize(RecoverHandle((Ptr)osBlock));
#endif
		DisposeHandle(RecoverHandle((Ptr)osBlock));
		osBlock = nextOSBlock;
	}
	printf("*********************\n\n");

}
#endif	// DEBUG


#ifdef PARANOID_CHECKS

// CheckAvailMem
//  verify that there is some minimum amount of heap and stack space available
static void
CheckAvailMem(void)
{
	Str255	errorStr;
	uint32	memAvail;
	Boolean	compactedOnce = false;
	
	// now make sure the program isn't getting too low on stack space...
	if ( (memAvail = StackSpace()) < kMinStackSpace )
	{
		if ( NULL != gSafetyBlock )	// free the safety block
			DisposePtr(gSafetyBlock);
		SysBeep(10);
		SysBeep(10);
		SysBeep(10);
		sprintf((char *)errorStr, "Stack space dangerously low (%ld bytes available)!!\n\n"\
					"Increase MPW's partition and/or modify it's 'HEXA' resource and try again.", memAvail );
		DebugStr(c2pstr((char *)errorStr));
		exit(0);
	}

	// now make sure the program isn't getting too low on heap space either...
TRY_AGAIN:	
	if ( ((memAvail = FreeMem()) < kMinHeapSpace) && ((memAvail = FreeMemSys()) < kMinHeapSpace) )
	{
		// make ONE try to compact the heap, bail if we've aleady done it and still
		//  are too low
		if ( false == compactedOnce )
		{
			compactedOnce = true;
			(void)MaxMem((int32 *)&memAvail);
			(void)MaxMemSys((int32 *)&memAvail);
			MaxApplZone();
			goto TRY_AGAIN;
		}
		
		if ( NULL != gSafetyBlock )	// free the safety block
			DisposePtr(gSafetyBlock);
		SysBeep(10);
		SysBeep(10);
		SysBeep(10);
		sprintf((char *)errorStr, "Heap space dangerously low (%ld bytes available)!!"\
					"Increase MPW's partition and/or modify it's 'HEXA' resource and try again.", memAvail );
		DebugStr(c2pstr((char *)errorStr));
		exit(0);
	}

}
#endif



// Inside Mac says that C-style functions returning pointers will return them in
//  D0.  By default, Metrowerks returns pointers in A0.  Define our external functions
//  return pointers in D0 so that they will work for calls from the MPW runtime and
//  from routines that expect us to behave in the 'standard' way
#if __MWERKS__
#pragma pointers_in_D0		
#endif

// 
// resize a block allocated by "AllocateBlock" above
void * 
ReallocateBlock(void *ptr, size_t size)
{
	// look for a couple of strange historical conditions first:  
	//  - realloc(NULL, size) is the same as malloc(size)
	//  - realloc(ptr, 0) is the same as free(ptr)
	if ( NULL == ptr )
		ptr = AllocateBlock(size);
	else if ( 0 == size )
	{
		free(ptr);
		ptr = NULL;
	}
	else
	{
		void	*tempPtr = AllocateBlock(size);
		if ( NULL == tempPtr )
		{
			PANIC_ERROR(("realloc() failed in malloc()"));
			ptr = NULL;
			goto DONE;
		}
		memcpy(tempPtr, ptr, size);
		FreeBlock(ptr);
		ptr = tempPtr;
	}
DONE:
	return ptr;
}

//
// allocate a block
void *
AllocateBlock(size_t size)
{
	void	*ptr;

	// if this function is being called for the first time, setup the exit callback so
	//  we can be sure to free everything before the application quits
	if ( true == gFirstTime )
		InitMemoryFcns();

#ifdef PARANOID_CHECKS
	CheckAvailMem();
#endif

	// make sure the size is quad aligned
	if ( (int32)size < 0 )
		size = 4;
	size = QUADBYTE_ALIGN_UP(size + sizeof(MemBlock));

	// If the requested size is too large,  use the Memory Manager directly
	if ( size > kBiggestPoolBlock )
	{
		ptr = AllocateOSBlock(size);
#ifdef QUAD_BYTE_ALIGN
		if ( false == IS_QUADBYTE_ALIGNED(ptr) )
			PANIC_ERROR(("AllocateOSBlock returned unaligned ptr : 0x%x = %d\n", ptr, size));
#endif	// QUAD_BYTE_ALIGN
	}		
	else
	{
		ptr = AllocatePoolBlock(size);
#ifdef QUAD_BYTE_ALIGN
		if ( false == IS_QUADBYTE_ALIGNED(ptr) )
			PANIC_ERROR(("AllocatePoolBlock returned unaligned ptr : 0x%x = %d\n", ptr, size));
#endif	// QUAD_BYTE_ALIGN
	}

	return ptr;
}


#if __MWERKS__
//	reset the standard Metrowerks parameter return convention
#pragma pointers_in_A0
#endif



//@@@@@
#ifdef TEST_ALLOC

#define kAllocSize2	997
#define	kTestSize2	10
static void
CorrectnessTest(void)
{
	void	*memArray[kTestSize2];
	void	*tempPtr;
	MemPool	*currPool;
	int32	ndx;

	memset(memArray, 0, kTestSize2 * sizeof(void *));
	for ( ndx = 0 ; ndx < kTestSize2 ; ndx++ )
	{
		memArray[ndx] = AllocateBlock(kAllocSize2);
		if ( NULL == memArray[ndx] )
			break;
	}

	// suck up the rest of the pool
	currPool = gPoolBlockList;
/*
	while ( currPool == gPoolBlockList )
		tempPtr = AllocateBlock(kBiggestPoolBlock - sizeof(MemBlock) - 10);
	// free the 2nd pool just allocated
	FreeBlock(tempPtr);
*/
	// and fill the rest of the pool up with little blocks
	while ( currPool == gPoolBlockList )
		tempPtr = AllocateBlock(kAllocSize2);
	// free the 2nd pool just allocated
	FreeBlock(tempPtr);
//@@@@@	PrintAllPools();
	
	// make sure the coalescing code works
	FreeBlock(memArray[2]); memArray[2] = NULL;
	FreeBlock(memArray[4]); memArray[4] = NULL;
	// the call should coalesce the three blocks
	FreeBlock(memArray[3]); memArray[3] = NULL;
//@@@@@	PrintAllPools();
	
	// and make sure the we reuse freed blocks.  first allocate a block the size of 
	//  two single blocks (remember that this will only have one header so add the 
	//  size of one header to the allocation size)
	memArray[2] = AllocateBlock(kAllocSize2 * 2 + sizeof(MemBlock));
	// this alloc should plug the one free slot
	memArray[3] = AllocateBlock(kAllocSize2);
//@@@@@	PrintAllPools();

	// allocate a couple of blocks in the system heap for the 'freeall' test
	(void)AllocateBlock(2048);
	(void)AllocateBlock(3048);
	(void)AllocateBlock(7777);
	tempPtr = AllocateBlock(9875);
	
	// free everything
	FreeAllBlocks();
	
	// and now make sure we don't hurl trying to free things that have already been freed
	FreeBlock(NULL);
	FreeBlock(memArray[4]);
}


#define	RandRange(max) ((rand() % max) + 1)
#define	kAllocsPerCycle	100
#define	kCycleCount		100

int
main(int argc, char **argv)
{
	void	*memArray[kAllocsPerCycle];
	int32	ndx = 1,
			counter = 0;
	void	*memPtr;
	
/*
	// first, check allocations with matching deallocations.  randomly pick a size to allocate so
	//  that approx. half are in pools, and half are OS allocated
	while ( counter++ < kCycleCount )
	{
		printf("  starting alloc loop, counter = %d\n", counter);
		for ( ndx = 0 ; ndx < kAllocsPerCycle ; ndx++ )
		{
			memArray[ndx] = AllocateBlock((kBiggestPoolBlock / 2 ) + RandRange(kBiggestPoolBlock));
			if ( NULL == memArray[ndx] )
				printf("allocation failed!/n");
		}
		for ( ndx = 0 ; ndx < kAllocsPerCycle ; ndx++ )
		{
			if ( NULL == memArray[ndx] )
				printf("allocation failed?/n");
			else
				FreeBlock(memArray[ndx]);
		}
	}
*/	

	
	// now allocate a bunch of blocks, both pool and OS based, and don't free them explicitly.  
	//  rather, let the shutdown code free them
	printf("\n\n\n\n  starting alloc loop\n");
//@@@@@	for ( ndx = 0 ; ndx < kAllocsPerCycle ; ndx++ )
	for ( ndx = 0 ; ndx < 1000000L ; ndx++ )
	{
		memPtr = AllocateBlock(RandRange(kBiggestPoolBlock * 4));
		if ( NULL == memPtr )
		{
			printf(" allocation failed, guess we're out of memory/n");
			break;
		}
	}

	return 0;
}

#endif		// TEST_ALLOC
//@@@@@
