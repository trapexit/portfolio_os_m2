/****************************************************************************
**
**  @(#) mempool.h 96/03/04 1.8
**
*****************************************************************************/
#ifndef	__STREAMING_MEMPOOL_H
#define	__STREAMING_MEMPOOL_H

#ifndef __STDDEF_H
#include <stddef.h>		/* offsetof */
#endif

#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif


/******************************************************************/
/* Data structures for managing a pool of preallocated structures */
/******************************************************************/
typedef struct MemPoolEntry {
	struct MemPoolEntry*	next;	 /* pointer to next in the list */
	uint32					data[1]; /* start of **variable-size** user data */
	} MemPoolEntry, *MemPoolEntryPtr;

#define MEM_POOL_ENTRY_HEADER_SIZE	offsetof(MemPoolEntry, data)


typedef struct MemPool {
	int32			numItemsInPool;	/* total number allocated */
	int32			numFreeInPool;	/* current number of free entries */
	MemPoolEntryPtr	availList;		/* ptr to first available entry */
	int32			totalPoolSize;	/* amount to (de)allocate */
	MemPoolEntry	data[1];		/* start of **variable-size** data block area */
	} MemPool, *MemPoolPtr;

#define MEM_POOL_HEADER_SIZE		offsetof(MemPool, data)


typedef bool (*ForEachFreePoolMemberFuncPtr)(void *argValue, void *poolEntry);
typedef bool (*ForEachPoolMemberFuncPtr)(void *argValue, void *poolEntry,
				bool fInUse);


/******************************************/
/* Fixed pool memory management functions */
/******************************************/
#ifdef __cplusplus 
extern "C" {
#endif

MemPoolPtr	CreateMemPool(int32 numToPreallocate, int32 sizeOfEntry);
MemPoolPtr	CreateMemPoolWithOptions(int32 numToPreallocate, int32 sizeOfEntry,
				uint32 memtypeOptions);
bool		ForEachFreePoolMember(MemPoolPtr memPool, 
				ForEachFreePoolMemberFuncPtr forEachFunc, void *argValue);
void		DeleteMemPool(MemPoolPtr memPool);

void		*AllocPoolMem(MemPoolPtr memPool);
void		ReturnPoolMem(MemPoolPtr memPool, void *poolEntry);

bool		EnumerateMemPoolEntries(MemPoolPtr memPool,
				ForEachPoolMemberFuncPtr forEachFunc, void *argValue);


#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_MEMPOOL_H */
