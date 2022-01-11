/* @(#) audio_misc.c 96/08/19 1.27 */
/* $Id: audio_misc.c,v 1.18 1995/03/02 07:39:58 phil Exp phil $ */
/****************************************************************
**
** Audio Folio Miscellaneous Support routines
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930617 PLB Force owner before deleting Item.  NO DON'T
** 930904 PLB Call SuperInternalDeleteItem to avoid ownership problem.
** 940608 PLB Disabled calls to mySumAvailMem
** 941128 PLB Added afi_IsRamAddress
** 950106 WJB Made Read16() take a const pointer.
** 950106 WJB Silenced a warning in Read16().
** 950109 WJB Fixed Write16() to take a uint32 - saves a few
**            instructions for caller and callee.
** 950131 WJB Cleaned up includes.
** 950515 WJB Surrounded DiscOsVersion() check in afi_IsRamAdrr() with AF_API_OPERA.
** 951207 WJB Fixed afi_DeleteLinkedItems() and afi_DeleteReferenceItems() to
**            cope with side-effect Item deletions.
**            Removed afi_RemoveReferenceNode().
**            Added RemoveAndMarkNode().
****************************************************************/

#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */


#if 0
/***************************************************************/
int32 afi_ReportMemoryUsage( void )
{
	uint32 BytesFree, BytesOwned;

	mySumAvailMem( KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	PRT(("Memory: UD=%8d", (BytesOwned-BytesFree)));

	mySumAvailMem( KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	PRT((", UV=%8d", (BytesOwned-BytesFree)));

	mySumAvailMem( KernelBase->kb_MemFreeLists,
		MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	PRT((", SD=%8d", (BytesOwned-BytesFree)));

	mySumAvailMem( KernelBase->kb_MemFreeLists,
		MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	PRT((", SV=%8d\n", (BytesOwned-BytesFree)));

	return 0;
}
#endif

/*************************************************************/
/* !!! is this really useful? */
int32 afi_SuperDeleteItemNode( ItemNode *n )
{
	return afi_SuperDeleteItem( n->n_Item );
}

/*************************************************************/
/* !!! is this really useful? */
int32 afi_SuperDeleteItem( Item it )
{
	int32 Result;

/* This routine allows non-owners to delete! 930904 */
	Result = SuperInternalDeleteItem( it );
	if(Result < 0)
	{
		ERR(("afi_SuperDeleteItem: delete of %x failed: 0x%x\n", it,
			 Result));
	}

	return Result;
}

/**************************************************************/
/*
	Delete entire List of ItemNodes. The list typically belongs to an item
	which 'owns' this list of child items, and is being deleted.

	Items which can't be deleted are removed from the list to avoid having them
	contain a bogus list pointer. (@@@ this assumes that ir_Delete method for
	all Items for which this function is called also removes the item from its
	containing list with RemoveAndMarkNode()).

	Arguments
		itemList
			Valid list of ItemNodes to delete. May be empty.
*/
void afi_DeleteLinkedItems( List *itemList )
{
	ItemNode *n;

		/*
			Because deleting this item _can_ cause other items in the list to
			be deleted, we can't rely on _any_ of the n_Next pointers to remain
			valid across the call to DeleteItem(). So we need to delete the
			'first' item in the list, until the list is empty. Because the
			delete method for any of these items removes the ItemNode from this
			list, the list must remain intact at all times, and the nodes
			mustn't be removed before calling DeleteItem().
		*/
	while ( n=(ItemNode *)FirstNode(itemList), IsNode(itemList,(Node *)n) ) {
			/*
				If for some reason we can't delete an Item, safely remove it from the
				list. This avoid having the item contain bogus pointers, and permits list
				processing to continue.
			*/
		if (SuperInternalDeleteItem (n->n_Item) < 0) {
			ERR(("afi_DeleteLinkedItems: Unable to delete item 0x%x (type %d) @ 0x%x\n", n->n_Item, n->n_Type, n));
			RemoveAndMarkNode ((Node *)n);
		}
	}
}

/**************************************************************/
/*
	Delete all Items contained in the List of AudioReferenceNodes. The list
	typically belongs to an item which 'owns' this list of child items, and is
	being deleted.

	Items which can't be deleted are removed from the list to avoid having them
	contain a bogus list pointer. (@@@ this assumes that ir_Delete method for
	all Items for which this function is called also removes the item from its
	containing list with RemoveAndMarkNode()).

	Currently this is only used for Attachments, which contain an embedded
	AudioReferenceNode.

	Arguments
		refList
			Valid list of AudioReferenceNodes to delete. May be empty.
*/
void afi_DeleteReferencedItems (List *refList)
{
	AudioReferenceNode *arnd;

		/*
			The Item deletion code is responsible for removing the
			AudioReferenceNode from the list.

			Because deleting this item _can_ cause other items in the list to
			be deleted, we can't rely on _any_ of the n_Next pointers to remain
			valid across the call to DeleteItem(). So we need to delete the
			'first' item in the list, until the list is empty. Because the
			delete method for any of these items removes the ItemNode from this
			list, the list must remain intact at all times, and the nodes
			mustn't be removed before calling DeleteItem().
		*/
	while ( arnd=(AudioReferenceNode *)FirstNode(refList), IsNode(refList,(Node *)arnd) ) {
		if (SuperInternalDeleteItem (arnd->arnd_RefItem) < 0) {
			/*
				If for some reason we can't delete an Item, safely remove it from the
				list. This avoid having the item contain bogus pointers, and permits list
				processing to continue.
			*/
		#ifdef BUILD_STRINGS
			{
				const ItemNode * const n = LookupItem (arnd->arnd_RefItem);

				ERR(("afi_DeleteReferenceItems: Unable to delete item 0x%x", arnd->arnd_RefItem));
				if (n) ERR((" (type %d) @ 0x%x\n", n->n_Type, n));
			}
		#endif
			RemoveAndMarkNode ((Node *)arnd);
		}
	}
}


/**************************************************************/
/*
	Remove node and mark it as removed (by setting n_Next to NULL).
	This function checks to make sure that the target node hasn't
	already been marked as removed prior to removing it.

	Arguments
		n
			Node to remove. Must be a valid node, but doesn't need to be in a
			list.
*/
void RemoveAndMarkNode (Node *n)
{
	if (n->n_Next) {
		ParanoidRemNode (n);
		n->n_Next = NULL;
	}
}


/**************************************************************
** Dummy routine to satisfy TagProcessor.
** !!! will go away soon
**************************************************************/
int32 afi_DummyProcessor( void *i, void *p, uint32 tag, uint32 arg)
{
	TOUCH(i);
	TOUCH(p);
	TOUCH(tag);
	TOUCH(arg);
	return 0;
}


/**************************************************************/
/*
	Count strings in PackedString Array

	Arguments
		packedStringBuf
			packed string array

		packedStringBufSize
			size of packed string array. Last byte is assumed to be '\0'

	Return Value
		Number of strings found.
*/
int32 CountPackedStrings (const char *packedStringBuf, uint32 packedStringBufSize)
{
	const char * const packedStringBufEnd = packedStringBuf + packedStringBufSize;
	const char *s = packedStringBuf;
	int32 numStrings = 0;

	for ( ; s < packedStringBufEnd; s = NextPackedString(s)) numStrings++;

	return numStrings;
}


/******************************************************************
** Before this routine, the folio called SuperIsRamAddress
** from many places.  (Warning, the return value for SuperIsRamAddress
** is the opposite of SuperValidateMem.) The folio was expecting a
** negative return code from SuperIsRamAddress which is incorrect.
** SuperIsRamAddress returns FALSE if it is not a RAM address.
** This new routine does return a negative return code for out of RAM,
** and returns zero if in legal RAM.
******************************************************************/
Err afi_IsRamAddr( const void *p, int32 Size )
{
	/*
		950608: removed support for pre-V24 behavior, which failed to
				call IsRamAddr() properly and let invalid addresses slide thru.
	*/

	if( !IsMemReadable( p, Size ) )
	{
		ERR(("afi_IsRamAddr: bad addr = 0x%x, Size = %d\n",
			p, Size ));
		return AF_ERR_BADPTR;
	}
	return 0;  /* OK! */
}


/******************************************************************/
/*
	Tests Audio Item data (e.g., sample data, tuning data, ...) for elegibility for
	inclusion in an audio item.

	Arguments
		dataAddr, dataSize
			Address and size of memory to test.

		autoFree
			TRUE if memory is to be freed automatically on Item deletion. If so, tests
			whether memory is actually freeable by the calling task. Otherwise, the
			memory only needs to be readable by the calling task (i.e., not hardware).
*/
Err ValidateAudioItemData (const void *dataAddr, const int32 dataSize, bool autoFree)
{
		/* Is memory readable? */
	if (!IsMemReadable (dataAddr, dataSize)) {
		ERR(("ValidateAudioItemData: Not valid memory (0x%08x, %d bytes)\n", dataAddr, dataSize));
		return AF_ERR_BADPTR;
	}

		/* If autoFree, make sure memory is freeable */
		/* @@@ could also make sure that dataSize <= GetMemTrackSize(), but it's not necessary */
	if ( autoFree &&
		 !( IsMemWritable (dataAddr, dataSize) &&
			IsMemOwned (dataAddr, dataSize) ) ) {
		ERR(("ValidateAudioItemData: Caller doesn't own memory (0x%08x, %d bytes); can't set AF_TAG_AUTO_FREE_DATA.\n", dataAddr, dataSize));
		return AF_ERR_BADPTR;
	}

	return 0;
}
