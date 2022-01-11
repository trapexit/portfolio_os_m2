/****************************************************************************
**
**  @(#) storeitemincontext.c 96/02/23 1.3
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */


/* -------------------- Debug */

#define DEBUG_StoredItem 0


/* -------------------- Code */

static Err PurgeStoredItem (IFFParser *, ContextInfo *);

/**
|||	AUTODOC -private -class libmusic -group IFF -name StoreItemInContext
|||	Stores an Item into the IFF context stack.
|||
|||	  Synopsis
|||
|||	    Err StoreItemInContext (IFFParser *iff, PackedID type, PackedID id,
|||	                            ContextInfoLocation pos, Item item)
|||
|||	  Description
|||
|||	    Stores an Item into the IFF context stack. Once stored, if the Item is not
|||	    extracted by the time the context is popped off of the context stack, the
|||	    Item is automatically deleted. If another Item with the same type and id has
|||	    already been stored at the specified context location, that Item is deleted
|||	    before the new one is stored (similar in behavior to property chunk storage).
|||
|||	    This is handy for storing an Item into the context stack from an entry or
|||	    exit handler which is to be picked up at some later point as if it were a
|||	    property chunk.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser into whose context stack the Item is to be stored.
|||
|||	    type, id
|||	        Type and ID to give stored Item.
|||
|||	    pos
|||	        Position in context stack to store Item.
|||
|||	    item
|||	        Item to store. Assumed to be a valid Item or 0.
|||
|||	        Because 0 is the value returned by FindStoredItem() if no Item macthing
|||	        the requested type and id is found, you may store 0 to simulate this
|||	        condition. This may be used to avoid having FindStoredItem() return an
|||	        Item from lower on the context stack.
|||
|||	  Return Value
|||
|||	    0 on success, Err code on failure.
|||
|||	    On success, the Item becomes the property of the IFF context until it is
|||	    extracted. If this function fails, the Item is left intact.
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
|||	    ExtractStoredItem(), FindStoredItem(), StoreContextInfo()
**/
Err StoreItemInContext (IFFParser *iff, PackedID type, PackedID id, ContextInfoLocation pos, Item item)
{
    ContextInfo *ci;
    Err errcode;

  #if DEBUG_StoredItem
    IFFPrintf (iff, "StoreItemInContext %.4s.%.4s pos=%d item=0x%x\n", &type, &id, pos, item);
    if (pos == IFF_CIL_PROP) {
        const ContextNode * const cn = FindPropContext (iff);
        if (cn) IFFPrintf (iff, "prop context: %.4s %.4s %Ld\n", &cn->cn_Type, &cn->cn_ID, cn->cn_Size);
    }
  #endif

        /* Allocate ContextInfo to store Item */
        /* ci_Data points to location to write Item (i.e., is of type Item *) */
    if (!(ci = AllocContextInfo (type, id, MIFF_CI_ITEM, sizeof (Item), (IFFCallBack)PurgeStoredItem))) {
        errcode = IFF_ERR_NOMEM;
        goto clean;
    }
    *(Item *)ci->ci_Data = item;    /* @@@ not validating or checking for non-negative */

        /* store it */
    if ((errcode = StoreContextInfo (iff, ci, pos)) < 0) goto clean;

    return 0;

clean:
        /* not calling PurgeStoredItem because this function doesn't free the item if it fails */
    FreeContextInfo (ci);
    return errcode;
}

/*
    MIFF_CI_ITEM ContextInfo purge method.
    Deletes item because client never claimed it.

    Arguments
        iff
        ci
            Never NULL because this is only called by iff folio.
*/
static Err PurgeStoredItem (IFFParser *iff, ContextInfo *ci)
{
    TOUCH(iff);

  #if DEBUG_StoredItem
        /* @@@ ci_Type and ci_ID are private fields to the IFF folio */
    IFFPrintf (iff, "PurgeStoredItem %.4s.%.4s item=0x%x\n", &ci->ci_Type, &ci->ci_ID, *(Item *)ci->ci_Data);
  #endif

    DeleteItem (*(Item *)ci->ci_Data);
    FreeContextInfo (ci);
    return 0;
}
