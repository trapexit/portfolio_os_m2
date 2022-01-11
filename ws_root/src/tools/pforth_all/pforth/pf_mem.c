/* @(#) pf_mem.c 96/05/16 1.8 */
/***************************************************************
** Memory allocator for systems that don't have real one.
** This might be useful when bringing up a new computer with no OS.
**
** For PForth based on 'C'
**
** Author: Phil Burk
** Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
**
** The pForth software code is dedicated to the public domain,
** and any third party may reproduce, distribute and modify
** the pForth software code or any derivative works thereof
** without any compensation or license.  The pForth software
** code is provided on an "as is" basis without any warranty
** of any kind, including, without limitation, the implied
** warranties of merchantability and fitness for a particular
** purpose and their equivalents under the laws of any jurisdiction.
**
****************************************************************
**
***************************************************************/

#include "pf_all.h"

#ifndef TOUCH
#define TOUCH(x) ((void) x)
#endif

#ifdef PF_NO_MALLOC

/* CUSTOM: Make the memory pool bigger if you want. */
#ifndef PF_MEM_POOL_SIZE
	#define PF_MEM_POOL_SIZE (0x10000)
#endif
#define PF_MEM_BLOCK_SIZE (16)
#ifndef PF_MALLOC_ADDRESS
	static char MemoryPool[PF_MEM_POOL_SIZE];
#else
	char *MemoryPool = (char *)PF_MALLOC_ADDRESS;
#endif

/**********************************************************
** Doubly Linked List Tools
**********************************************************/

typedef struct DoublyLinkedListNode
{
	struct DoublyLinkedListNode *dlln_Next;
	struct DoublyLinkedListNode *dlln_Previous;
} DoublyLinkedListNode;

typedef struct DoublyLinkedList
{
	struct DoublyLinkedListNode *dll_First;
	struct DoublyLinkedListNode *dll_Null;
	struct DoublyLinkedListNode *dll_Last;
} DoublyLinkedList;

#define dllPreviousNode(n) ((n)->dlln_Previous)
#define dllNextNode(n) ((n)->dlln_Next)

void dllSetupList( DoublyLinkedList *dll )
{
	dll->dll_First = (DoublyLinkedListNode *) &(dll->dll_Null);
	dll->dll_Null = (DoublyLinkedListNode *) NULL;
	dll->dll_Last = (DoublyLinkedListNode *) &(dll->dll_First);
}

void dllLinkNodes( DoublyLinkedListNode *Node0, DoublyLinkedListNode *Node1 )
{
	Node0->dlln_Next = Node1;
	Node1->dlln_Previous = Node0;
}

void dllInsertNodeBefore( DoublyLinkedListNode *NewNodePtr,
	DoublyLinkedListNode *NodeInListPtr )
{
	DoublyLinkedListNode *NodePreviousPtr = dllPreviousNode( NodeInListPtr );
	dllLinkNodes( NodePreviousPtr, NewNodePtr );
	dllLinkNodes( NewNodePtr, NodeInListPtr );
}

void dllInsertNodeAfter( DoublyLinkedListNode *NewNodePtr,
	DoublyLinkedListNode *NodeInListPtr )
{
	DoublyLinkedListNode *NodeNextPtr = dllNextNode( NodeInListPtr );
	dllLinkNodes( NodeInListPtr, NewNodePtr );
	dllLinkNodes( NewNodePtr, NodeNextPtr );
}

void dllDumpNode( DoublyLinkedListNode *NodePtr )
{
	TOUCH(NodePtr);
	DBUG(("  0x%x -> (0x%x) -> 0x%x\n",
		dllPreviousNode( NodePtr ), NodePtr,
		dllNextNode( NodePtr ) ));
}

int32 dllCheckNode( DoublyLinkedListNode *NodePtr )
{
	if( (NodePtr->dlln_Next->dlln_Previous != NodePtr) ||
	    (NodePtr->dlln_Previous->dlln_Next != NodePtr))
	{
		ERR("dllCheckNode: Bad Node!\n");
		dllDumpNode( dllPreviousNode( NodePtr ) );
		dllDumpNode( NodePtr );
		dllDumpNode( dllNextNode( NodePtr ) );
		return -1;
	}
	else
	{
		return 0;
	}
}
void dllRemoveNode( DoublyLinkedListNode *NodePtr )
{
	if( dllCheckNode( NodePtr ) == 0 )
	{
		dllLinkNodes( dllPreviousNode( NodePtr ), dllNextNode( NodePtr ) );
	}
}

void dllAddNodeToHead( DoublyLinkedList *ListPtr, DoublyLinkedListNode *NewNodePtr )
{
	dllInsertNodeBefore( NewNodePtr, ListPtr->dll_First );
}

void dllAddNodeToTail( DoublyLinkedList *ListPtr, DoublyLinkedListNode *NewNodePtr )
{
	dllInsertNodeAfter( NewNodePtr, ListPtr->dll_Last );
}

#define dllIsNodeInList( n ) (!((n)->dlln_Next == NULL) )
#define dllIsLastNode( n ) ((n)->dlln_Next->dll_nNext == NULL )
#define dllIsListEmpty( l ) ((l)->dll_First == ((DoublyLinkedListNode *) &((l)->dll_Null)) )
#define dllFirstNode( l ) ((l)->dll_First)

static DoublyLinkedList gMemList;
static int32 gIfMemListInit = 0;

typedef struct MemListNode
{
	DoublyLinkedListNode  mln_Node;
	int32                 mln_Size;
} MemListNode;

#ifdef PF_DEBUG
/***************************************************************
** Dump memory list.
*/
void maDumpList( void )
{
	MemListNode *mln;
	
	MSG("PForth MemList\n");
	
	for( mln = (MemListNode *) dllFirstNode( &gMemList );
	     dllIsNodeInList( (DoublyLinkedListNode *) mln);
		 mln = (MemListNode *) dllNextNode( (DoublyLinkedListNode *) mln ) )
	{
		MSG("  Node at = 0x"); ffDotHex(mln);
		MSG_NUM_H(", size = 0x", mln->mln_Size);
	}
}
#endif


/***************************************************************
** Free mem of any size.
*/
void pfFreeRawMem( char *Mem, int32 NumBytes )
{
	MemListNode *mln, *FreeNode;
	MemListNode *AdjacentLower = NULL;
	MemListNode *AdjacentHigher = NULL;
	MemListNode *NextBiggest = NULL;
	
/* Allocate in whole blocks of 16 bytes */
	DBUG(("\npfFreeRawMem( 0x%x, 0x%x )\n", Mem, NumBytes ));
	NumBytes = (NumBytes + PF_MEM_BLOCK_SIZE - 1) & ~(PF_MEM_BLOCK_SIZE - 1);
	DBUG(("\npfFreeRawMem: Align NumBytes to 0x%x\n", NumBytes ));
	
/* Check memory alignment. */
	if( ( ((int32)Mem) & (PF_MEM_BLOCK_SIZE - 1)) != 0)
	{
		MSG_NUM_H("pfFreeRawMem: misaligned Mem = 0x", (int32) Mem );
		return;
	}
	
/* Scan list from low to high looking for various nodes. */
	for( mln = (MemListNode *) dllFirstNode( &gMemList );
	     dllIsNodeInList( (DoublyLinkedListNode *) mln);
		 mln = (MemListNode *) dllNextNode( (DoublyLinkedListNode *) mln ) )
	{
		if( (((char *) mln) + mln->mln_Size) == Mem )
		{
			AdjacentLower = mln;
		}
		else if( ((char *) mln) == ( Mem + NumBytes ))
		{
			AdjacentHigher = mln;
		}
/* is this the next biggest node. */
		else if( (NextBiggest == NULL) && (mln->mln_Size >= NumBytes) )
		{
			NextBiggest = mln;
		}
	}
	
/* Check to see if we can merge nodes. */
	if( AdjacentHigher )
	{
DBUG((" Merge (0x%x) -> 0x%x\n", Mem, AdjacentHigher ));
		NumBytes += AdjacentHigher->mln_Size;
		dllRemoveNode( (DoublyLinkedListNode *) AdjacentHigher );
	}
	if( AdjacentLower )
	{
DBUG((" Merge 0x%x -> (0x%x)\n", AdjacentLower, Mem ));
		AdjacentLower->mln_Size += NumBytes;
	}
	else
	{
DBUG((" Link before 0x%x\n", NextBiggest ));
		FreeNode = (MemListNode *) Mem;
		FreeNode->mln_Size = NumBytes;
		if( NextBiggest == NULL )
		{
/* Nothing bigger so add to end of list. */
			dllAddNodeToTail( &gMemList, (DoublyLinkedListNode *) FreeNode );
		}
		else
		{
/* Add this node before the next biggest one we found. */
			dllInsertNodeBefore( (DoublyLinkedListNode *) FreeNode,
				(DoublyLinkedListNode *) NextBiggest );
		}
	}
	
/*	maDumpList(); */
}

/***************************************************************
** Setup memory list. Initialize allocator.
*/
static void maSetupList( void )
{
	char *AlignedMemory;
	int32 AlignedSize;
	
	dllSetupList( &gMemList );
	gIfMemListInit = TRUE;
	
/* Adjust to next highest aligned memory location. */
	AlignedMemory = (char *) ((((int32)MemoryPool) + PF_MEM_BLOCK_SIZE - 1) &
	                  ~(PF_MEM_BLOCK_SIZE - 1));
					  
/* Adjust size to reflect aligned memory. */
	AlignedSize = PF_MEM_POOL_SIZE - (AlignedMemory - MemoryPool);
	
/* Align size of pool. */
	AlignedSize = AlignedSize & ~(PF_MEM_BLOCK_SIZE - 1);
	
/* Free to pool. */
	pfFreeRawMem( AlignedMemory, AlignedSize );
	
}

/***************************************************************
** Allocate mem from list of free nodes.
*/
char *pfAllocRawMem( int32 NumBytes )
{
	char *Mem = NULL;
	MemListNode *mln;
	
	if( NumBytes <= 0 ) return NULL;
	
	if( gIfMemListInit == 0 ) maSetupList();
	
/* Allocate in whole blocks of 16 bytes */
	NumBytes = (NumBytes + PF_MEM_BLOCK_SIZE - 1) & ~(PF_MEM_BLOCK_SIZE - 1);
	
	DBUG(("\npfAllocRawMem( 0x%x )\n", NumBytes ));
	
/* Scan list from low to high until we find a node big enough. */
	for( mln = (MemListNode *) dllFirstNode( &gMemList );
	     dllIsNodeInList( (DoublyLinkedListNode *) mln);
		 mln = (MemListNode *) dllNextNode( (DoublyLinkedListNode *) mln ) )
	{
		if( mln->mln_Size >= NumBytes )
		{
			int32 RemSize;

			Mem = (char *) mln;
			
/* Remove this node from list. */
			dllRemoveNode( (DoublyLinkedListNode *) mln );
			
/* Is there enough left in block to make it worth splitting? */
			RemSize = mln->mln_Size - NumBytes;
			if( RemSize >= PF_MEM_BLOCK_SIZE )
			{
				pfFreeRawMem( (Mem + NumBytes), RemSize );
			}
			break;
		}
				
	}
/*	maDumpList(); */
	DBUG(("Allocate mem at 0x%x.\n", Mem ));
	return Mem;
}

/***************************************************************
** Keep mem size at first cell.
*/
char *pfAllocMem( int32 NumBytes )
{
	int32 *IntMem;
	
	if( NumBytes <= 0 ) return NULL;
	
/* Allocate an extra cell for size. */
	NumBytes += sizeof(int32);
	
	IntMem = (int32 *)pfAllocRawMem( NumBytes );
	
	if( IntMem != NULL ) *IntMem++ = NumBytes;
	
	return (char *) IntMem;
}

/***************************************************************
** Free mem with mem size at first cell.
*/
void pfFreeMem( void *Mem )
{
	int32 *IntMem;
	int32 NumBytes;
	
	if( Mem == NULL ) return;
	
/* Allocate an extra cell for size. */
	IntMem = (int32 *) Mem;
	IntMem--;
	NumBytes = *IntMem;
	
	pfFreeRawMem( (char *) IntMem, NumBytes );
	
}

#endif /* PF_NO_MALLOC */
