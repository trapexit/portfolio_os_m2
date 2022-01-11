/****************************************************************************
**
**  @(#) findstoreditem.c 96/02/23 1.3
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */


/* -------------------- Debug */

#define DEBUG_StoredItem 0


/* -------------------- Code */

/**
|||	AUTODOC -private -class libmusic -group IFF -name FindStoredItem
|||	Finds a stored Item in the IFF context stack without removing it from the
|||	context stack.
|||
|||	  Synopsis
|||
|||	    Item FindStoredItem (IFFParser *iff, PackedID type, PackedID id)
|||
|||	  Description
|||
|||	    Finds stored Item in IFF context stack without removing it from context
|||	    stack.
|||
|||	    The returned Item remains the responsibility of the IFF context.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to scan.
|||
|||	    type, id
|||	        Type and ID to of stored Item to search for.
|||
|||	  Return Value
|||
|||	    >0
|||	        Item number of the stored Item.
|||
|||	    0
|||	        Requested Item wasn't found in the context stack, or a 0 was stored to
|||	        simulate the condition of Item not found.
|||
|||	    <0
|||	        Error code on failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/music_iff.h>, libmusic.a, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    StoreItemInContext(), ExtractStoredItem(), FindPropChunk()
**/
Item FindStoredItem (IFFParser *iff, PackedID type, PackedID id)
{
    const ContextInfo * const ci = FindContextInfo (iff, type, id, MIFF_CI_ITEM);

        /* if not found, return 0 */
    if (!ci) return 0;

  #if DEBUG_StoredItem
    IFFPrintf (iff, "FindStoredItem %.4s.%.4s item=0x%x\n", &type, &id, *(Item *)ci->ci_Data);
  #endif

        /* return item */
    return *(Item *)ci->ci_Data;
}
