/*
 *	@(#) alloc.c 96/07/16 1.16
 *	Copyright 1994, The 3DO Company
 *
 * Memory allocation.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

#define	ATRACE(x)	/* printf x */

#define	GRAN		4	/* All allocs are multiple of GRAN */
#define	MINFREE		16	/* Min size of a free block */
#define	MINALLOC	16	/* Min size of an allocated block */

typedef struct FreeHeader {
#ifdef DEBUG
	uint32 f_Magic;
#endif
	struct FreeHeader *f_Next;
	struct FreeHeader *f_Prev;
	uint32 f_Size;
} FreeHeader;

typedef struct AllocHeader {
#ifdef DEBUG
	uint32 a_Magic;
#endif
	uint32 a_Size;
} AllocHeader;

#ifdef DEBUG
#define	A_MAGIC	(0x12E4567B)
#define	F_MAGIC	(0x12F45D7B)
#endif

static FreeHeader Freelist;
#define	fl_Head	f_Next
#define	fl_Tail f_Prev

/*****************************************************************************
 Initialize the memory allocator.
*/
void
InitDipirAlloc(void)
{
	FreeHeader *p;

	/*
	 * DipirTemp sits at the beginning of DipirPrivateBuf.
	 * Take all unused bytes in DipirPrivateBuf as the memory pool.
	 */
	p = (FreeHeader *) (((uint8*)dtmp) + sizeof(DipirTemp));
	p->f_Size = dtmp->dt_PrivateBufSize - ((uint8*)p - (uint8*)dtmp);
	PRINTF(("MemPool size %x\n", p->f_Size));
	p->f_Next = p->f_Prev = &Freelist;
#ifdef DEBUG
	p->f_Magic = F_MAGIC;
#endif
	Freelist.fl_Head = Freelist.fl_Tail = p;
}

/*****************************************************************************
 Allocate memory.
*/
void *
DipirAlloc(uint32 size, uint32 allocFlags)
{
	FreeHeader *p;
	AllocHeader *alloc;
	uint32 *rp;
	uint32 align;

	align = 1;
	if (allocFlags & ALLOC_DMA)
	{
		align = dtmp->dt_CacheLineSize;
		size += dtmp->dt_CacheLineSize - 1;
	}
	size += sizeof(AllocHeader);
	size += align-1;
	size = (size + GRAN-1) & ~(GRAN-1);
	if (size < MINALLOC)
		size = MINALLOC;
	for (p = Freelist.fl_Head;  p != &Freelist;  p = p->f_Next)
	{
#ifdef DEBUG
		if (p->f_Magic != F_MAGIC)
		{
			printf("**** DipirAlloc(%x): bad free list @%x!\n", 
				size, p);
			return NULL;
		}
#endif
		if (p->f_Size >= size + MINFREE)
		{
			/* Take end of this block. */
			p->f_Size -= size;
			alloc = (AllocHeader *)(((uint8*)p) + p->f_Size);
		} else if (p->f_Size >= size)
		{
			/* Take the whole block */
			size = p->f_Size;
			p->f_Prev->f_Next = p->f_Next;
			p->f_Next->f_Prev = p->f_Prev;
			alloc = (AllocHeader *) p;
		} else
		{
			continue;
		}

		alloc->a_Size = size;
#ifdef DEBUG
		alloc->a_Magic = A_MAGIC;
#endif
		/* Increment to reach correct alignment. */
		for (rp = (uint32*)(alloc + 1);  
		     (uint32)rp & (align-1);
		     rp++)
			*rp = 0;
		ATRACE(("@@ alloc %x %x\n", rp, size));
		return rp;
	}
	EPRINTF(("======= DipirAlloc(%x) FAILED\n", size));
	return NULL;
}

/*****************************************************************************
 Free memory previously allocated by DipirAlloc.
*/
void
DipirFree(void *ptr)
{
	AllocHeader *alloc;
	uint32 size;
	uint32 *rp;
	FreeHeader *p;
	FreeHeader *newp;
	FreeHeader *prev;
	FreeHeader *next;
	
	/* Find the AllocHeader for this node */
	for (rp = (uint32 *)(((uint8*)ptr) - sizeof(uint32));  *rp == 0;  rp--)
		continue;
	alloc = (AllocHeader *) (((uint8*)rp) - sizeof(AllocHeader) + sizeof(uint32));
#ifdef DEBUG
	if (alloc->a_Magic != A_MAGIC)
	{
		printf("**** DipirFree(%x): bad block @%x!\n", ptr, alloc);
		return;
	}
#endif
	size = alloc->a_Size;
	ATRACE(("@@ free  %x %x\n", ptr, size));

	/* Find the right place in the free list. */
	for (p = Freelist.fl_Head;  p != &Freelist;  p = p->f_Next)
		if ((void*)p > (void*)alloc)
			break;

	prev = p->f_Prev;
	next = p;
	if (((uint8*)prev) + prev->f_Size == (uint8*)alloc)
	{
		/* Merge with previous node in free list. */
		prev->f_Size += size;
		if (((uint8*)prev) + prev->f_Size == (uint8*)next)
		{
			/* Merge with next node in free list. */
			prev->f_Size += next->f_Size;
			prev->f_Next = next->f_Next;
			next->f_Next->f_Prev = prev;
		}
	} else if (((uint8*)alloc) + size == (uint8*)next)
	{
		/* Merge with next node in free list. */
		newp = (FreeHeader *) alloc;
		newp->f_Next = next->f_Next;
		newp->f_Prev = next->f_Prev;
#ifdef DEBUG
		newp->f_Magic = F_MAGIC;
#endif
		next->f_Next->f_Prev = newp;
		next->f_Prev->f_Next = newp;
		newp->f_Size = next->f_Size + size;
	} else
	{
		/* Make a new free node and link into free list. */
		newp = (FreeHeader *) alloc;
		newp->f_Prev = prev;
		newp->f_Next = next;
		newp->f_Size = size;
#ifdef DEBUG
		newp->f_Magic = F_MAGIC;
#endif
		prev->f_Next = newp;
		next->f_Prev = newp;
	}
}

/*****************************************************************************
*/
#ifdef DEBUG
DumpDipirAlloc(void)
{
	FreeHeader *p;

	printf("=== FreeList ===\n");
	for (p = Freelist.fl_Head;  p != &Freelist;  p = p->f_Next)
		printf("%d: prev %d  next %d  size %d\n",
			p, p->f_Prev, p->f_Next, p->f_Size);
	printf("=== end ===\n");
}
#endif /* DEBUG */
