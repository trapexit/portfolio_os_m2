/****************************************************************************
**
**  @(#) extractstoreditem.c 96/02/23 1.3
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */


/* -------------------- Debug */

#define DEBUG_StoredItem 0


/* -------------------- Code */

/**
|||	AUTODOC -private -class libmusic -group IFF -name ExtractStoredItem
|||	Finds and removes a stored Item from the IFF context stack.
|||
|||	  Synopsis
|||
|||	    Item ExtractStoredItem (IFFParser *iff, PackedID type, PackedID id)
|||
|||	  Description
|||
|||	    Extracts stored Item from IFF context stack. Also removes the storage
|||	    ContextInfo from the stack, preventing automatic Item deletion when the
|||	    context is popped off of the context stack.
|||
|||	    The returned Item becomes the responsibility of the caller.
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
|||	    Stored Item if found, 0 if not found, or Err code for some other kind of
|||	    failure.
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
|||	    StoreItemInContext(), FindStoredItem(), FindPropChunk()
**/
Item ExtractStoredItem (IFFParser *iff, PackedID type, PackedID id)
{
    ContextInfo * const ci = FindContextInfo (iff, type, id, MIFF_CI_ITEM);
    Item item;

        /* if not found, return 0 */
    if (!ci) return 0;

  #if DEBUG_StoredItem
    IFFPrintf (iff, "ExtractStoredItem %.4s.%.4s item=0x%x\n", &type, &id, *(Item *)ci->ci_Data);
  #endif

        /* return item and remove from context */
    item = *(Item *)ci->ci_Data;
    RemoveContextInfo (ci);
    FreeContextInfo (ci);

    return item;
}
