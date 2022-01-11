/* @(#) item.c 96/11/19 1.102 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/task.h>
#include <kernel/folio.h>
#include <kernel/semaphore.h>
#include <kernel/interrupts.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/timer.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/panic.h>
#include <kernel/lumberjack.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>
#include <kernel/internalf.h>


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


/*****************************************************************************/


/* #define MASTERDEBUG */

#ifdef MASTERDEBUG
#define DBUGCI(x)	if (CheckDebug(KernelBase,0)) printf x
#define DBUGDI(x)	if (CheckDebug(KernelBase,3)) printf x
#define DBUGOI(x)       if (CheckDebug(KernelBase,5)) printf x
#define DBUGSIP(x)	if (CheckDebug(KernelBase,10)) printf x
#define DBUGSIO(x)	if (CheckDebug(KernelBase,28)) printf x
#define DBUGFI(x)       /* printf x */
#else
#define DBUGCI(x)
#define DBUGDI(x)
#define DBUGOI(x)
#define DBUGSIP(x)
#define DBUGSIO(x)
#define DBUGFI(x)
#endif

#define DBUG(x) /*printf x*/
#define LDBUG(x)	/*printf x*/

#define DBUGWF(x)	/*printf x*/
#define PAUSE	/*if (ntype == FOLIONODE)	Pause();*/
#define DBUGCRT(x)	/*printf x*/
#define DBUGGI(x)	/*printf x*/

#define	MIN_ITEM		32
#define MAX_ITEMS		16384
#define ITEMS_PER_BLOCK		512
#define	MAX_BLOCKS		(MAX_ITEMS/ITEMS_PER_BLOCK)


static void RemoveItemReferences(Item it);


/*****************************************************************************/


static ItemEntry *ItemPtrTable[MAX_BLOCKS];
static int32      firstFreeIndex = 0;
static int32      lastFreeIndex  = 0;


/*****************************************************************************/


static ItemEntry *NewItemTableBlock(void)
{
    ItemEntry *ie;
    ItemEntry *p;
    int32      i;

    if (KB_FIELD(kb_MaxItem) >= MAX_ITEMS)
	return 0;
    DBUG(("NewItemTableBlock calling SuperAllocMem\n"));
    ie = (ItemEntry *)SuperAllocMem(sizeof(ItemEntry) * ITEMS_PER_BLOCK,
			       MEMTYPE_ANY);
    DBUG(("ie=%lx\n",ie));
    if (ie == NULL)
	return NULL;

    ItemPtrTable[KB_FIELD(kb_MaxItem)/ITEMS_PER_BLOCK] = ie;

    /* Initialize the new table. All ie_ItemAddr are set to NULL,
     * and all ie_ItemInfo fields are set to the index number of the
     * entry that comes after them, except for the last entry in the
     * block, which gets the value of firstFreeIndex instead
     */

    i = KB_FIELD(kb_MaxItem);
    p = ie;
    DBUG(("while (TRUE)\n"));
    while (TRUE)
    {
        p->ie_ItemAddr = NULL;

        i++;
        if (i == KB_FIELD(kb_MaxItem) + ITEMS_PER_BLOCK)
        {
            p->ie_ItemInfo = firstFreeIndex;
            break;
        }

        p->ie_ItemInfo = i;
        p++;
    }

    DBUG((" firstFreeIndex = KB_FIELD(kb_MaxItem);\n"));
    firstFreeIndex = KB_FIELD(kb_MaxItem);
    if (firstFreeIndex == 0)
        firstFreeIndex = MIN_ITEM;

    KB_FIELD(kb_MaxItem) += ITEMS_PER_BLOCK;

    if (lastFreeIndex == 0)
        lastFreeIndex = KB_FIELD(kb_MaxItem) - 1;

    DBUGCI(("NewItemTableBlock: New MaxItem=%ld\n",KB_FIELD(kb_MaxItem)));
    return ie;
}


/*****************************************************************************/


ItemEntry **InitItemTable(void)
{
    DBUG(("enter InitItemTable KernelBase=%lx\n",KernelBase));
    KB_FIELD(kb_ItemTable) = ItemPtrTable;
    KB_FIELD(kb_MaxItem) = 0;
    NewItemTableBlock();
    NewItemTableBlock();
    DBUG(("ItemTable at:%lx\n",(uint32)ItemPtrTable));
    return ItemPtrTable;
}


/*****************************************************************************/


static ItemEntry *GetItemEntryPtr(Item i)
{
ItemEntry *p;
uint32     j;

    i &= ITEM_INDX_MASK;

    if (i >= KB_FIELD(kb_MaxItem))
	return 0;

    j = i/ITEMS_PER_BLOCK;	/* which block */
    p = ItemPtrTable[j];
    i -= j*ITEMS_PER_BLOCK;	/* which one in this block? */
    return p + i;
}


/*****************************************************************************/


Item AssignItem(void *p, Item i)
{
	ItemEntry *ie;
	DBUG(("AssignItem(%lx,%lx)\n",(long)p,i));

	ie = GetItemEntryPtr(i);
	if (ie == NULL) return BADITEM;
	ie->ie_ItemAddr = p;
	ie->ie_ItemInfo = i;
	return i;
}


/*****************************************************************************/


Item GetItem(void *p)
{
int32	   iix, i;
ItemEntry *t;

    DBUG(("GetItem(%lx)\n",(uint32)p));

    DBUGGI(("GetItem st=%d t=%d\n",((Node *)p)->n_SubsysType,\
	    ((Node *)p)->n_Type ));

    /* The available entries within the item table are kept in a linked list.
     * The list is made by having each empty slot in the table point to
     * another empty slot.
     */

    iix = firstFreeIndex;
    if (iix == 0)
    {
        if (NewItemTableBlock() == NULL)
        {
#ifdef BUILD_STRINGS
            printf("GetItem: unable to extend system item table\n");
#endif
            return NOMEM;
        }
        iix = firstFreeIndex;
    }

    t = GetItemEntryPtr(iix);

    i = (t->ie_ItemInfo & ITEM_INDX_MASK);
    if (i >= MIN_ITEM)
    {
        /* okky dokky, we've got an empty slot we can use in the future */
        firstFreeIndex = i;
    }
    else if (i == 0)
    {
        /* no more free items, we'll need to extend the table later */
        firstFreeIndex = 0;
        lastFreeIndex  = 0;
    }

    /* set things up, and return */
    t->ie_ItemAddr = p;
    t->ie_ItemInfo = (t->ie_ItemInfo & ITEM_GEN_MASK) | iix;

    return t->ie_ItemInfo;
}


/*****************************************************************************/


void FreeItem(Item i)
{
ItemEntry *ie;
ItemNode  *in;

    ie = GetItemEntryPtr(i);
    if (ie)
    {
        in = (ItemNode *)ie->ie_ItemAddr;
        if (in && (in->n_Item == i))
        {
            ie->ie_ItemAddr = NULL;    /* entry isn't used anymore... */

            i &= ITEM_INDX_MASK;

            if (i >= MIN_ITEM)
            {
                /* if it's not one of the special reserved folio numbers... */

                ie->ie_ItemInfo += 0x10000; /* bump the generation count */

                if (ie->ie_ItemInfo & 0x80000000)
                {
                    /* The generation count of this item has just wrapped.
                     * Go clean up any references to it to make sure we
                     * don't hit any nasty suprises.
                     */
                    RemoveItemReferences(i);
                }

                ie->ie_ItemInfo = (ie->ie_ItemInfo & ITEM_GEN_MASK);

                if (lastFreeIndex)
                {
                    /* add at the end of the free list */
                    ie               = GetItemEntryPtr(lastFreeIndex);
                    ie->ie_ItemInfo |= i;
                }
                else
                {
                    /* free list is empty, so this item becomes the free list */
                    firstFreeIndex = i;
                }
                lastFreeIndex = i;
            }
        }
    }
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name LookupItem
|||	Gets a pointer to an item.
|||
|||	  Synopsis
|||
|||	    void *LookupItem( Item it )
|||
|||	  Description
|||
|||	    This function finds an item by its item number and returns a
|||	    pointer to the item.
|||
|||	    Note: Because items are owned by the system, user tasks cannot
|||	    change the values of their fields. They can only read the values
|||	    contained in the public fields.
|||
|||	  Arguments
|||
|||	    it
|||	        The number of the item to look up.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the item or NULL if the item does not exist.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CheckItem(), CreateItem(), FindItem(), FindNamedItem()
|||
**/

void *LookupItem(Item i)
{
ItemEntry *ie;
ItemNode  *in;

    LDBUG(("LookupItem(%lx)\n",i));

    ie = GetItemEntryPtr(i);
    if (ie)
    {
        in = (ItemNode *)ie->ie_ItemAddr;
        if (in && (in->n_Item == i))
        {
            return in;
        }
    }

    return NULL;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name CheckItem
|||	Checks to see if an item exists.
|||
|||	  Synopsis
|||
|||	    void *CheckItem( Item i, uint8 ftype, uint8 ntype )
|||
|||	  Description
|||
|||	    This function checks to see if a given item exists. To specify the
|||	    item, you use an item number, an item-type number, and the item
|||	    number of the folio in which the item type is defined. If all three
|||	    of these values match those of the item, CheckItem() returns a
|||	    pointer to the item.
|||
|||	  Arguments
|||
|||	    i
|||	        The item number of the item to be checked.
|||
|||	    ftype
|||	        The item number of the folio that defines the item type.
|||	        (This is the same value that is passed as first parameter to
|||	        the MkNodeID() macro.)
|||
|||	    ntype
|||	        The item-type number for the item. (This is the same value that
|||	        is passed as second parameter to the MkNodeID() macro.)
|||
|||	  Return Value
|||
|||	    If the item exists (and the values of all three arguments match
|||	    those of the item), this function returns a pointer to the item. If
|||	    the item does not exist, it returns NULL.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), FindNamedItem(),
|||	    LookupItem(), MkNodeID()
|||
**/

void *CheckItem(Item i,uint8 SubsysType,uint8 Type)
{
Node *n;

    n = (Node *)LookupItem(i);
    DBUG(("CheckItem(%lx), n=%lx\n",i,n));
    if (n)
    {
        if (n->n_SubsysType != SubsysType) return NULL;
        if (n->n_Type != Type) return NULL;
    }
    return (void *)n;
}

void setitemvr(Item ri) {
    ItemNode *in;
    Task *t = CURRENTTASK;

    if(ri >= 0) {
	     in = (ItemNode *)LookupItem(ri);
#ifdef BUILD_PARANOIA
	     if (in == NULL) {
		printf("setitemvr: LookupItem failed! item %x\n", ri);
		return;
	     }
#endif
	     if(!in->n_Version && !in->n_Revision) {
		in->n_Version = t->t.n_Version;
		in->n_Revision = t->t.n_Revision;
	     }
    }
}

Item
internalCreateKernelItem(void *n,uchar ntype,void *args)
{
	Item ret;

	DBUG(("internalCreateKernelItem ntype=%lx\n",(uint32)ntype));

	switch (ntype) {
	    case TASKNODE:
		ret = internalCreateTask((Task *)n,(TagArg *)args);
		break;
	    case SEMA4NODE:
		ret = internalCreateSemaphore((Semaphore *)n,(TagArg *)args);
		break;
	    case FOLIONODE:
		ret = internalCreateFolio((Folio *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case MSGPORTNODE:
		ret = internalCreateMsgPort((MsgPort *)n,(TagArg *)args);
		break;
	    case MESSAGENODE:
		ret = externalCreateMsg((Msg *)n,(TagArg *)args);
		break;
	    case FIRQNODE:
		ret = internalCreateFirq((FirqNode *)n,(TagArg *)args);
		break;
	    case IOREQNODE:
		ret = internalCreateIOReq((IOReq *)n,(TagArg *)args);
		break;
	    case DRIVERNODE:
		ret = internalCreateDriver((Driver *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case DEVICENODE:
		ret = internalCreateDevice((Device *)n,(TagArg *)args);
		setitemvr(ret);
		break;
	    case TIMERNODE:
		ret = internalCreateTimer((Timer *)n,(TagArg *)args);
		break;
	    case MODULENODE:
		ret = internalCreateModule((Module *) n, (TagArg *) args);
		break;
	    case ERRORTEXTNODE:
		ret = internalCreateErrorText((ErrorText *)n,(TagArg *)args);
		break;

	    default:
		ret = BADSUBTYPE;
	}
    return ret;
}

static Err internalIncreaseResourceTable(Task *t, uint32 numSlots)
{
	Item *newt;
	int32 oldsize = t->t_ResourceCnt;
	int32 newsize = oldsize + numSlots;

	DBUGCRT(("CRT(%lx,%d,%d)\n",(uint32)t,oldsize,newsize));

	newt = (Item *)SuperAllocMem(newsize * sizeof(Item),MEMTYPE_NORMAL);
	if (newt)
	{
		int32 c = oldsize;
		Item *p = newt;
		Item *oldt = t->t_ResourceTable;
		int32 nextSlot;
		int32 oldCount;

		/* transfer contents of old resource table */
		while (c--)
		    {
		    /* DBUGCRT(("CRT moves %x\n", *oldt)); */
		    *p++ = *oldt++;
		    }

		/* Free old resource table */
		SuperFreeMem(t->t_ResourceTable,oldsize*sizeof(Item));

                oldCount           = t->t_ResourceCnt;
		t->t_ResourceCnt   = newsize;
		t->t_ResourceTable = newt;

		/* initialize rest of array */
		newsize -= oldsize;	/* leftover to init */

                nextSlot = oldCount + 1;
		while (newsize--)
		{
		    *p++ = (nextSlot | 0x80000000);
		    nextSlot++;
                }
                p--;
                *p = (t->t_FreeResourceTableSlot | 0x80000000);
                t->t_FreeResourceTableSlot = oldCount;
		DBUGCRT(("CRT returns success, new %x, old %x\n", newt, oldt));

		return 0;
	}

	DBUGCRT(("CRT returns failure\n"));
	return NOMEM;
}


/*****************************************************************************/


Err externalIncreaseResourceTable(uint32 numSlots)
{
    return internalIncreaseResourceTable(CURRENTTASK, numSlots);
}


/*****************************************************************************/


static int32 AllocItemSlot(Task *t)
{
Item  *ip;
int32  i;

    ip = t->t_ResourceTable;

    /* t_FreeResourceTable contains either 0xffffffff, or the index of
     * a free entry within the table. If it has the index of a free entry,
     * the entry within the table itself contains the index of another
     * free entry, or 0xffffffff. We basically have a linked-list of free
     * entries.
     */
    i = t->t_FreeResourceTableSlot;
    if (i < 0)
    {
        /* no free slots in sight, try and extend the table */
        if (internalIncreaseResourceTable(t,4) < 0)
            return -1;

        ip = t->t_ResourceTable;
        i  = t->t_FreeResourceTableSlot;
    }

    if (ip[i] == (Item)0xffffffff)
    {
        /* This entry doesn't point to another entry. This is
         * therefore the last free slot in the table. We set the
         * field to 0xffffffff to indicate this fact.
         */
        t->t_FreeResourceTableSlot = 0xffffffff;
    }
    else
    {
        /* Keep track of the next empty slot. We clear the high bit to
         * indicate the slot number is valid.
         */
        t->t_FreeResourceTableSlot = (ip[i] & 0x7fffffff);
    }

    return i;
}


/*****************************************************************************/


static void FreeItemSlot(Task *t, int32 slot)
{
    t->t_ResourceTable[slot]   = t->t_FreeResourceTableSlot | 0x80000000;
    t->t_FreeResourceTableSlot = slot;
}


/*****************************************************************************/


int32 FindItemSlot(Task *t, Item it)
{
Item  *ip;
int32  i;

    if (it < 0)
        return -1;

    ip = t->t_ResourceTable;

    DBUGDI(("FindItemSlot(ip=%lx it=%lx\n",(uint32)ip,(uint32)it));
    DBUGDI(("ResourceCnt=%d\n",t->t_ResourceCnt));

    for (i = 0; i < t->t_ResourceCnt; i++)
    {
#ifdef DEBUG
        DBUG(("*ip=%lx ",(uint32)*ip));
        if ((i & 3) ==3) DBUG(("\n"));
#endif
        if (*ip == it)
            return i;

        ip++;
    }

    return -1;
}


/*****************************************************************************/


/* Same as FindItemSlot(), but only looks at the item number, ignoring
 * generation counts and other flags
 */
static int32 FindItemSlotNoGen(const Task *t, Item it)
{
Item  *ip;
int32  i;

    if (it < 0)
        return -1;

    it = it & ITEM_INDX_MASK;

    ip = t->t_ResourceTable;
    for (i = 0; i < t->t_ResourceCnt; i++)
    {
        if ((*ip & ITEM_INDX_MASK) == it)
            return i;

        ip++;
    }

    return -1;
}


/*****************************************************************************/


void RemoveItem(Task *t, Item item)
{
int32 slot;

    /* Remove item from task's resource table */
    if (t)
    {
	slot = FindItemSlot(t,item);
	if (slot >= 0)
	    FreeItemSlot(t,slot);
    }
}


/*****************************************************************************/


/* Removes any references we can find to a given item number, regardless
 * of its generation count or other flag bits
 */
static void RemoveItemReferences(Item it)
{
Task  *t;
Node  *n;
int32  slot;

    it &= ITEM_INDX_MASK;
    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        t = Task_Addr(n);
        slot = FindItemSlotNoGen(t,it);
	if (slot >= 0)
	    FreeItemSlot(t,slot);
    }
}

/**
|||	AUTODOC -public -class Kernel -group Items -name OpenItem
|||	Opens an item.
|||
|||	  Synopsis
|||
|||	    Item OpenItem( Item FoundItem, void *args )
|||
|||	  Description
|||
|||	    This function opens an item for access.
|||
|||	    System items are created automatically by the operating system,
|||	    such as folios and devices, and thus do not need to be created
|||	    by tasks. To use a system item, a task first opens it by calling
|||	    OpenItem(). When a task finishes using a system item, it calls
|||	    CloseItem() to inform the operating system that it is no longer
|||	    using the item.
|||
|||	  Arguments
|||
|||	    item
|||	        The number of the item to open. To find the item number of a
|||	        system item use FindItem() or FindNamedItem().
|||
|||	    args
|||	        A pointer to an array of tag arguments. Currently, this
|||	        must be NULL.
|||
|||	  Return Value
|||
|||	    Returns the number of the item that was opened or a negative error
|||	    code for failure.  The item number for the opened item is
|||	    not always the same as the item number returned by FindItem().
|||	    When accessing the opened item you should use the return from
|||	    OpenItem(), not the return from FindItem(). You can also call the
|||	    FindAndOpenItem() function to do both a search and an open
|||	    operation in an atomic manner.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CloseItem(), FindItem()
|||
**/

Item internalOpenItem(Item founditem,void *args, Task *t)
{
	Item ret;
	OpeningItemNode *n = (OpeningItemNode *)LookupItem(founditem);
	Folio *f;
	ItemRoutines *ir;
	int32 slot;

	DBUGOI(("OpenItem item=%lx\n",(uint32)founditem));
	if (n == 0)	return BADITEM;

	/* check to see if the item supports being opened */
	if ((n->n_Flags & NODE_OPENVALID) == 0)
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantOpen);

	/* check to see if the item has been fully created */
	if (n->n_ItemFlags & (ITEMNODE_NOTREADY | ITEMNODE_DELETED))
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantOpen);

	f = WhichFolio(n->n_SubsysType);
	ir = f->f_ItemRoutines;

	DBUGOI(("OI AllocSlot\n"));

        /* make room in the resource table */
	slot = AllocItemSlot(t);
	if (slot < 0)
            return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvrFlw);

	n->n_OpenCount++;

	DBUGOI(("OpenCount=%d\n", n->n_OpenCount));

	if (ir->ir_Open)
            ret = (*ir->ir_Open)((Node *)n, args, t);
        else
            ret = n->n_Item;

	DBUGOI(("OpenItem ret = %lx\n",ret));
	if (ret >= 0)
	{
		/* Install this item in the Tasks ResourceTable */
		DBUGOI(("task=%lx slot = %d\n",(uint32) t,(uint32)slot));
		t->t_ResourceTable[slot] = ret | ITEM_WAS_OPENED;

		LogItemOpened((ItemNode *)n);
	}
	else
	{
		n->n_OpenCount--;
		FreeItemSlot(t, slot);
	}
	DBUGOI(("OpenItem now returns: %d\n",ret));
	return ret;
}


Item
externalOpenItem(Item founditem,void *args)
{
    if (args)
        return BADTAG;

    return internalOpenItem(founditem, args, CURRENTTASK);
}


Item
externalOpenItemAsTask(Item founditem, void *args, Item task)
{
    if(IsPriv(CURRENTTASK))
	return internalOpenItem(founditem, args, TASK(task));
    else
	return BADPRIV;
}



/*****************************************************************************/


static bool EnoughPriv(ItemNode *n, Task *t)
{
    if (t->t.n_Item == n->n_Owner)
    {
        /* always OK for item owner */
        return TRUE;
    }

    if (IsPriv(t))
    {
        /* always OK for privileged tasks */
        return TRUE;
    }

    if (t == (Task *)n)
    {
        /* always OK for self */
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name SetItemPri
|||	Changes the priority of an item.
|||
|||	  Synopsis
|||
|||	    int32 SetItemPri( Item i, uint8 newpri )
|||
|||	  Description
|||
|||	    This function changes the priority of an item. Some items in the
|||	    Portfolio are maintained in lists; for example, tasks and threads.
|||	    Some lists are ordered by priority, with higher-priority items
|||	    coming before lower-priority items. When the priority of an item in
|||	    a list changes, the kernel automatically rearranges the list of
|||	    items to reflect the new priority. The item is moved immediately
|||	    before the first item whose priority is lower.
|||
|||	    A task must own an item to change its priority. A task can change
|||	    its own priority even if it does not own itself.
|||
|||	  Arguments
|||
|||	    i
|||	        The item number of the item whose priority to change.
|||
|||	    newpri
|||	        The new priority for the item.
|||
|||	  Return Value
|||
|||	    Returns the previous priority of the item or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  Notes
|||
|||	    For certain item types, such as devices, the kernel may change the
|||	    priority of an item to help optimize throughput.
|||
|||	  Caveats
|||
|||	    This function is currently not implemented for many item types.
|||
|||	  See Also
|||
|||	    CreateItem()
|||
**/

static int32 internalSetItemPriority(Item it, uint8 pri, Task *t)
{
	int32 ret;
	ItemNode *n = (ItemNode *)LookupItem(it);
	Folio *f;
	ItemRoutines *ir;

	DBUGSIP(("internalSetItemPriority(%d,%lx) n=%lx\n",it,(uint32)t,(uint32)n));
	if (!n)	return BADITEM;

	if (!EnoughPriv(n,t))
	    return BADPRIV;

	f = WhichFolio(n->n_SubsysType);
	DBUGSIP(("internalSetItemPriority, f=%lx\n",(uint32)f));

	ir = f->f_ItemRoutines;
	if (ir->ir_SetPriority == 0)	return  NOSUPPORT;
	ret = (*ir->ir_SetPriority)(n,pri,t);

	return ret;
}

int32 externalSetItemPriority(Item i,uint8 pri)
{
    return internalSetItemPriority(i,pri,CURRENTTASK);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name SetItemOwner
|||	Changes the owner of an item.
|||
|||	  Synopsis
|||
|||	    Err SetItemOwner( Item i, Item newOwner )
|||
|||	  Description
|||
|||	    This function makes another task the owner of an item. A task must
|||	    be the owner of the item to change its ownership. The item is
|||	    removed from the current task's resource table and placed in the
|||	    new owner's resource table.
|||
|||	  Arguments
|||
|||	    i
|||	        The item number of the item to give to a new owner.
|||
|||	    newOwner
|||	        The item number of the new owner task.
|||
|||	  Return Value
|||
|||	    >= 0 if ownership of the item is changed or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  Caveats
|||
|||	    This is not currently implemented for all items that should be able
|||	    to have their ownership changed.
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), DeleteItem()
|||
**/

static Err internalSetItemOwner(Item it, Item newOwner, Task *t)
{
	int32 ret;
	ItemNode *n = (ItemNode *)LookupItem(it);
	Task *newOwnerp = (Task *)CheckItem(newOwner,KERNELNODE,TASKNODE);
	Folio *f;
	ItemRoutines *ir;
	int32 slot;

	DBUGSIO(("internalSetItemOwner(%d,%lx) n=%lx\n",it,(uint32)t,(uint32)n));
	DBUGSIO(("(task is %s, object is %s, newowner is %s)\n",t->t.n_Name,n->n_Name,newOwnerp->t.n_Name));
	if ((!newOwnerp) || (!n))	return BADITEM;

	/* should we make sure a task is not trying to own itself? */
	/* maybe this should be allowed!? */

	if (!EnoughPriv(n,t))
	    return BADPRIV;

	f = WhichFolio(n->n_SubsysType);
	DBUGSIO(("internalSetItemOwner, f=%lx\n",(uint32)f));

	ir = f->f_ItemRoutines;
	if (ir->ir_SetOwner == 0)	return  NOSUPPORT;

	slot = AllocItemSlot(newOwnerp);
	if (slot < 0)
	    return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvrFlw);

	ret = (*ir->ir_SetOwner)(n,newOwner,t);

	if (ret >= 0)
	{
	    /* operation successful, now fix the resource table */
	    RemoveItem((Task *)LookupItem(n->n_Owner),it);
	    n->n_Owner = newOwner;
	    newOwnerp->t_ResourceTable[slot] = it; /* put in resource table */

	    LogItemChangedOwner(n,newOwnerp);
	}
	else
	{
	    FreeItemSlot(newOwnerp,slot);
	}

	return ret;
}

Err externalSetItemOwner(Item i,Item newOwner)
{
    return internalSetItemOwner(i,newOwner,CURRENTTASK);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name CloseItem
|||	Closes a previously opened item.
|||
|||	  Synopsis
|||
|||	    Err CloseItem( Item it )
|||
|||	  Description
|||
|||	    System items are created automatically by the operating system,
|||	    such as folios and devices, and thus do not need to be created
|||	    by tasks. To use a system item, a task first opens it by calling
|||	    OpenItem(). When a task finishes using a system item, it calls
|||	    CloseItem() to inform the operating system that it is no longer
|||	    using the item.
|||
|||	  Arguments
|||
|||	    it
|||	        Number of the item to close.
|||
|||	  Return Value
|||
|||	    >= 0 if the item was closed successfully or a negative error code
|||	    for failure. Possible error codes currently include:
|||
|||	    BADITEM
|||	        The i argument is not an item.
|||
|||	    ER_Kr_ItemNotOpen
|||	        The i argument is not an opened item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    OpenItem()
|||
**/

int32 internalCloseItemSlot(Item it, Task *t, int32 slot)
{
	int32 ret = 0;
	OpeningItemNode *n = (OpeningItemNode *)LookupItem(it);
	Folio *f;
	ItemRoutines *ir;

	if (!n)	{
	    if(slot >= 0) {		/* if item is open but doesn't exist */
		FreeItemSlot(t,slot);
		return ret;
	    }
	    return BADITEM;
	}
	if (slot < 0) return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);

	f = WhichFolio(n->n_SubsysType);

	ir = f->f_ItemRoutines;

	n->n_OpenCount--;

        LogItemClosed((ItemNode *)n);

        if (ir->ir_Close)
            ret = (*ir->ir_Close)(it,t);
        else
            ret = 0;

	if (ret >= 0)
       	{
	    FreeItemSlot(t,slot);

	}
	else
	{
	    n->n_OpenCount++;
	}

	return ret;
}


int32 internalCloseItem(Item it, Task *t)
{
    return internalCloseItemSlot(it, t, FindItemSlot(t,it|ITEM_WAS_OPENED));
}


int32 externalCloseItem(Item i)
{
    return internalCloseItem(i,CURRENTTASK);
}


Item externalCloseItemAsTask(Item founditem, Item task)
{
    if (IsPriv(CURRENTTASK))
	return internalCloseItem(founditem, TASK(task));

    return BADPRIV;
}


/*****************************************************************************/


static int32 icki_c(Node *n, void *p, uint32 tag, uint32 arg)
{
    TOUCH(n);
    TOUCH(p);
    TOUCH(tag);
    TOUCH(arg);

    return 0;
}

/**
|||	AUTODOC -public -class Kernel -group Items -name MkNodeID
|||	Assembles an item type value.
|||
|||	  Synopsis
|||
|||	    int32 MkNodeID( uint8 ftype , uint8 nType )
|||
|||	  Description
|||
|||	    This macro creates an item type value, a 32-bit value that specifies an
|||	    item type and the folio in which the item type is defined. This value is
|||	    required by other functions that deal with items, such as CreateItem()
|||	    and FindItem().
|||
|||	  Arguments
|||
|||	    ftype
|||	        The item number of the folio in which the item type of the
|||	        item is defined.
|||
|||	    ntype
|||	        The item type number for the item.
|||
|||	  Return Value
|||
|||	    Returns an item type value, useful for such calls as CreateItem().
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/nodes.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/nodes.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateItem(), FindItem(), FindNamedItem()
|||
**/

Item internalCreateSizedItem(int32 cntype, void *args, int32 size)
{
	Item ret;
	Folio *f;
	uint8 ntype = TYPEPART( cntype );
	ItemNode *n;
	ItemRoutines *ir;
	ItemNode in;
	int32 slot;

	DBUGCI(("CreateSizedItem(%lx) size=%d\n",(uint32)cntype,size));
	DBUGCI(("args=%lx\n",args));

	f = WhichFolio(SUBSYSPART(cntype));
	DBUGCI(("WhichFolio returns:%lx\n",f));
	if (f == 0)	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);

	if (ntype > f->f_MaxNodeType)
	    return BADSUBTYPE;

	ir = f->f_ItemRoutines;
	if (!ir)
	{
	    /* this folio don't support no item creation */
            return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_BadType);
        }

	/* memset(&in, 0, sizeof(inode)); */
	in.n_Name = NULL;
	in.n_ItemFlags = 0;

	ret = TagProcessorNoAlloc(&in, (TagArg *)args, icki_c, 0);
	if (ret < 0) return ret;

        if (in.n_ItemFlags & ITEMNODE_UNIQUE_NAME)
        {
	    /* Unique name can't be NULL */
	    if (in.n_Name == NULL)
		return BADNAME;

	    /* Don't support unique-named item for folios without a find routine */
	    if (ir->ir_Find == NULL)
		return NOSUPPORT;

            {
	    TagArg tags[2];

                tags[0].ta_Tag = TAG_ITEM_NAME;
                tags[0].ta_Arg = in.n_Name;
                tags[1].ta_Tag = TAG_END;

                ret = (*ir->ir_Find)(TYPEPART(cntype), tags);
                if (ret >= 0)
                {
		    return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_UniqueItemExists);
		}
	    }
	}

	FreeDeadTasks();

	n = (ItemNode *)AllocateSizedNode(f, ntype, size);
	if (n == NULL)
	    return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoMem);

	if ((int)n > 0)	/* valid item pointer? */
		n->n_ItemFlags |= ITEMNODE_NOTREADY;	/* prevent item from being found */

        slot = AllocItemSlot(CURRENTTASK);
        if (slot < 0)
        {
            if ((int32)n > 0)
                FreeNode(f,n);

            return MAKEKERR(ER_KYAGB,ER_C_NSTND,ER_Kr_RsrcTblOvrFlw);
        }

	DBUGCI(("f_CreateItem=%lx\n",(uint32)ir->ir_Create));
	ret = (*ir->ir_Create)(n,ntype,args);

	DBUGCI(("ret = %d\n",ret));
	if (ret >= 0)
        {
                /* set the n_Owner field */
                DBUGCI(("%lx Setting Owner to %lx\n",n,CURRENTTASKITEM));
                n = (ItemNode *)LookupItem(ret);

                n->n_Owner = CURRENTTASKITEM;

                /* insert in the task's resource table */
                CURRENTTASK->t_ResourceTable[slot] = ret;

                LogItemCreated(n);

		n->n_ItemFlags &= ~ITEMNODE_NOTREADY;	/* allow item to be found and opened */
		/* FIXME: this should be a more generic mechanism. */
		if (cntype == MKNODEID(KERNELNODE,DRIVERNODE))
		{
			Err err;
			/* "Ownerless" item: give it to the operator. */
			err = internalSetItemOwner(n->n_Item, KB_FIELD(kb_OperatorTask), CURRENTTASK);
			if (err < 0)
			{
				(void) internalDeleteItem(n->n_Item);
				ret = err;
			}
		}
	}
	else
	{
	        FreeItemSlot(CURRENTTASK,slot);

		if ((int32)n > 0)
                    FreeNode(f,n);
	}
	DBUGCI(("CreateItem returns: %d\n",ret));

	return ret;
}

/**
|||	AUTODOC -public -class Kernel -group Items -name CreateItem
|||	Creates an item.
|||
|||	  Synopsis
|||
|||	    Item CreateItem( int32 ct, TagArg *p )
|||	    Item CreateItemVA( int32 ct, uint32 tags, ... );
|||
|||	  Description
|||
|||	    This function creates an item.
|||
|||	    There are convenience routines to create many types of items
|||	    (such as CreateMsg() to create a message and CreateIOReq() to create
|||	    an I/O request). You should use CreateItem() only when no
|||	    convenience routine is available or when you need to supply additional
|||	    arguments for the creation of the item beyond what the convenience
|||	    routine provides.
|||
|||	    When you no longer need an item created with CreateItem(), use
|||	    DeleteItem() to delete it.
|||
|||	  Arguments
|||
|||	    ct
|||	        Specifies the type of item to create. Use MkNodeID() to
|||	        generate this value.
|||
|||	    p
|||	        A pointer to an array of tag arguments. The tag arguments can
|||	        be in any order. The last  element of the array must be the
|||	        value TAG_END. If there are no tag arguments, this argument
|||	        must be NULL.
|||
|||	  Return Value
|||
|||	    Returns the item number of the new item or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CheckItem(), CreateIOReq(), CreateMsg(), CreateMsgPort(),
|||	    CreateSemaphore(), CreateSmallMsg(), CreateThread(), DeleteItem()
|||
**/

Item internalCreateItem(int32 cntype, void *args)
{
    return internalCreateSizedItem(cntype,args,0);
}

Err
internalSetOwnerKernelItem(ItemNode *n, Item newowner, Task *oldowner)
{
Task *newownerP;

	/*Item *ip;*/
	/* We know that oldowner does in fact own */
	/* this Item */
	uint8 ntype = n->n_Type;
	switch (ntype)
	{
	    case DEVICENODE   : return SetDeviceOwner((Device *)n,newowner);
            case DRIVERNODE   : return SetDriverOwner((Driver *)n,newowner);
	    case TASKNODE     : return internalSetTaskOwner((Task *)n,newowner);
	    case ERRORTEXTNODE: return 0;
            case FIRQNODE     : return 0;
            case TIMERNODE    : return 0;
            case FOLIONODE    : newownerP = (Task *)LookupItem(newowner);
                                if (IsSameTaskFamily(newownerP,oldowner))
                                {
                                    /* only allow the ownership to change amongst threads of a same task */
				    return 0;
                                }
                                return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);

            case IOREQNODE    : return SetIOReqOwner((IOReq *)n,newowner);
            case SEMA4NODE    : return internalSetSemaphoreOwner((Semaphore *)n,newowner);
            case MESSAGENODE  : return internalSetMsgOwner((Message *)n,newowner);
            case MSGPORTNODE  : return internalSetMsgPortOwner((MsgPort *)n,newowner);
	    case MODULENODE   : return 0;	/* Modules can be passed aroudn */
            default           : return NOSUPPORT;
        }
}

int32
internalSetPriorityKernelItem(ItemNode *n,uint8 pri,Task *t)
{
	uint8 ntype = n->n_Type;

	TOUCH(t);

	if (ntype == TASKNODE)	return internalChangeTaskPri((Task *)n,pri);
	else return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadSubType);
}

Item
internalOpenKernelItem(Node *n, void *a, Task *t)
{
	uint8 ntype = n->n_Type;
	Item result;

	DBUGOI(("OKI, ntype=%d n=%lx\n",ntype,n));
	switch(ntype)
	    {
	    case DRIVERNODE:	result = OpenDriver((Driver *) n, a, t);
		                break;

	    case DEVICENODE:	result = internalOpenDevice((Device *) n, a, t);
		                break;

	    case MODULENODE:	result = internalOpenModule((Module *) n, a, t);
		                break;

	    default:		result = MAKEKERR(ER_SEVER,ER_C_STND,ER_BadSubType);
		                break;
	    }

	return result;
}

int32 internalCloseKernelItem(Item it, Task *ct)
{
	/* CloseItem has already done pre verification */
	/* We know it is already opened */

	Node *n = (Node *)LookupItem(it);

	switch(n->n_Type)
	    {
	    case DRIVERNODE:	return CloseDriver((struct Driver *)n,ct);
	    case DEVICENODE:	return internalCloseDevice((struct Device *)n,ct);
	    case MODULENODE:	return internalCloseModule((struct Module *)n, ct);
	    default:	        return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadSubType);
	    }
}

int32
internalDeleteKernelItem(Item it, Task *t)
{
	Node *n = (Node *)LookupItem(it);
	switch (n->n_Type)
	{
	    case TASKNODE:
		return internalKill((struct Task *)n,t);
	    case SEMA4NODE:
		return internalDeleteSemaphore((struct Semaphore *)n,t);
	    case FOLIONODE:
		return internalDeleteFolio((struct Folio *)n,t);
	    case MSGPORTNODE:
		return internalDeleteMsgPort((struct MsgPort *)n,t);
	    case MESSAGENODE:
		return internalDeleteMsg((struct Message *)n,t);
	    case FIRQNODE:
		return internalDeleteFirq((struct FirqNode *)n,t);
	    case IOREQNODE:
		return internalDeleteIOReq((struct IOReq *)n,t);
	    case DRIVERNODE:
		return internalDeleteDriver((struct Driver *)n,t);
	    case DEVICENODE:
		return internalDeleteDevice((struct Device *)n,t);
	    case TIMERNODE:
		return internalDeleteTimer((struct Timer *)n,t);
	    case MODULENODE:
		return internalDeleteModule((struct Module *) n, t);
	    case ERRORTEXTNODE:
		return internalDeleteErrorText((ErrorText *)n,t);
	}
	/* should not get here! */
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_SoftErr);
}

extern Item internalFindTask(char *);

static Item NodeToItem(Node *n)
{
ItemNode *in = (struct ItemNode *)n;

    if ((n == NULL) || ((n->n_Flags & NODE_ITEMVALID) == 0))
        return -1;

    return in->n_Item;
}


/*****************************************************************************/


/* load a kernel item from disk */
Item internalLoadKernelItem(int32 ntype, TagArg *tags)
{
ItemNode inode;
Item     result;

    memset(&inode,0,sizeof(inode));

    result = TagProcessorNoAlloc(&inode,tags,NULL,0);
    if (result < 0)
        return result;

    if (inode.n_Name == NULL)
        return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

    switch (ntype)
    {
        case DRIVERNODE: result = internalLoadDriver(inode.n_Name);
                         break;

        default        : result = BADSUBTYPE;
    }

    return result;
}


/*****************************************************************************/


static Item SearchItemTable(uint8 subSys, uint8 type, const char *name)
{
uint32     block;
uint32     i;
ItemNode  *in;
ItemEntry *table;

    for (block = 0; block < MAX_BLOCKS; block++)
    {
        table = ItemPtrTable[block];
        if (!table)
            break;

        for (i = 0; i < ITEMS_PER_BLOCK; i++)
        {
            in = (ItemNode *)table[i].ie_ItemAddr;
            if (in)
            {
                if ((in->n_SubsysType == subSys) && (in->n_Type == type))
                {
                    if (in->n_Name && (strcasecmp(in->n_Name, name) == 0))
                    {
                        return in->n_Item;
                    }
                }
            }
        }
    }

    return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
}


/*****************************************************************************/


static Item FindItemFromItemTable(Folio *f, int8 ntype, TagArg *tagpt)
{
	ItemNode	inode;
	Item		ret;
	uint32		stype = NODETOSUBSYS(f);

	if (stype & ~NST_SYSITEMMASK)
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

	memset(&inode,0,sizeof(inode));

	/* Borrow the kernel tag processor for now */
	ret = TagProcessorNoAlloc(&inode,tagpt,NULL,0);
	if (ret < 0)	return ret;
	if (inode.n_Name == 0)	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);

	return SearchItemTable(stype,ntype,inode.n_Name);
}


/*****************************************************************************/


Item internalFindKernelItem(int32 ntype, TagArg *tagpt)
{
List    *l;
ItemNode inode;
Item     result;

    memset(&inode,0,sizeof(inode));

    result = TagProcessorNoAlloc(&inode,tagpt,NULL,0);
    if (result < 0)
        return result;

    if (inode.n_Name == NULL)
    {
        if (ntype == TASKNODE)
            return CURRENTTASKITEM;

        return MakeKErr(ER_SEVER,ER_C_STND,ER_NotFound);
    }

    switch (ntype)
    {
        case SEMA4NODE    : l = &KB_FIELD(kb_Semaphores); break;
        case FOLIONODE    : l = &KB_FIELD(kb_FolioList); break;
        case MSGPORTNODE  : l = &KB_FIELD(kb_MsgPorts); break;
        case ERRORTEXTNODE: l = &KB_FIELD(kb_Errors); break;
	/* FIXME: should lock DevSemaphore for DRIVER and DEVICE */
        case DRIVERNODE   : l = &KB_FIELD(kb_Drivers); break;
        case DEVICENODE   :
		{
		    Driver *drv;
		    drv = (Driver *)
			FindNamedNode(&KB_FIELD(kb_Drivers),inode.n_Name);
		    if (drv == NULL)
		        return MakeKErr(ER_SEVER,ER_C_STND,ER_NotFound);
		    l = &drv->drv_Devices;
		    break;
		}

        case TASKNODE     : return internalFindTask(inode.n_Name);

        case MESSAGENODE  :
        case IOREQNODE    : return SearchItemTable(KERNELNODE,ntype,inode.n_Name);

        default           : return BADSUBTYPE;
    }

    result = NodeToItem(FindNamedNode(l,inode.n_Name));
    if (result < 0)
        result = MakeKErr(ER_SEVER,ER_C_STND,ER_NotFound);

    return result;
}


/**
|||	AUTODOC -public -class Kernel -group Items -name FindItem
|||	Finds an item by type and tags.
|||
|||	  Synopsis
|||
|||	    Item FindItem( int32 cType, TagArg *tp )
|||	    Item FindItemVA( int32 cType, uint32 tags, ...)
|||
|||	  Description
|||
|||	    This function finds an item of the specified type whose tag
|||	    arguments match those pointed to by the tp argument. If more than
|||	    one item of the specified type has matching tag arguments, this
|||	    function returns the item number for the first matching item.
|||
|||	  Arguments
|||
|||	    cType
|||	        Specifies the type of the item to find. Use MkNodeID() to
|||	        generate this value.
|||
|||	    tp
|||	        A pointer to an array of tag arguments. The tag arguments can
|||	        be in any order. The array can contain some, all, or none of
|||	        the possible tag arguments for an item of the specified type;
|||	        to make the search more specific, include more tag arguments.
|||	        The last element of the array must be the value TAG_END. If
|||	        there are no tag arguments, this argument must be NULL.
|||
|||	  Return Value
|||
|||	    Returns the number of the first item that matches or a negative
|||	    error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  Caveats
|||
|||	    This function currently ignores most tag arguments.
|||
|||	  See Also
|||
|||	    CheckItem(), FindNamedItem(), LookupItem(), MkNodeID()
|||
**/

Item
internalFindItem(int32 cntype, TagArg *tagpt)
{
	Folio *f;
	ItemRoutines *ir;
	Item ret;
	ItemNode *n;

	f = WhichFolio(SUBSYSPART(cntype));
	if (f == 0)	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);
	ir = f->f_ItemRoutines;
	if (ir->ir_Find == 0)
		ret = FindItemFromItemTable(f,TYPEPART(cntype),tagpt);
	else ret = (*ir->ir_Find)(TYPEPART(cntype),tagpt);
	if (ret >= 0)
	{
	    if (ret != CURRENTTASKITEM)
	    {
		n=(ItemNode *)LookupItem(ret);
		if(n && (n->n_ItemFlags & (ITEMNODE_NOTREADY | ITEMNODE_DELETED)))
		    return MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
	    }
	}
	return ret;
}

/**
|||	AUTODOC -public -class Kernel -group Items -name DeleteItem
|||	Deletes an item.
|||
|||	  Synopsis
|||
|||	    Err DeleteItem( Item it )
|||
|||	  Description
|||
|||	    This function deletes the specified item and frees any resources
|||	    (including memory) that were allocated for the item.
|||
|||	    There are convenience procedures for deleting most types of items
|||	    (such as DeleteMsg() to delete a message and DeleteIOReq() to delete
|||	    an I/O request). You should use DeleteItem() only if you used
|||	    CreateItem() to create the item. If you used a convenience routine
|||	    to create an item, you must use the corresponding convenience
|||	    routine to delete the item.
|||
|||	    If a task tries to perform operations on an item that has been
|||	    deleted, the system will return an error code indicating the
|||	    item doesn't exist. Note that it is possible for item numbers to
|||	    be reused. This generally only happens after the system has been
|||	    up and running without a reboot for a few weeks. This can become
|||	    an issue in set-top box environments.
|||
|||	    Tasks can only delete items that they own. If a task transfers
|||	    ownership of an item to another task, it can no longer delete the
|||	    item. The lone exception to this is the task itself; you can always
|||	    commit suicide regardless of who owns the task/thread item.
|||
|||	    When a task dies, the kernel automatically deletes all of the items
|||	    it has created, and closes any items it has opened. In addition,
|||	    any semaphores the task has locked get unlocked, and any messages
|||	    the task hasn't replied to get replied with an error code.
|||
|||	  Arguments
|||
|||	    it
|||	        Number of the item to be deleted.
|||
|||	  Return Value
|||
|||	    >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CheckItem(), CreateItem(), DeleteIOReq(), DeleteMsg(), DeleteMsgPort(),
|||	    DeleteThread(), exit()
|||
**/

static int32 realDeleteItem(Item it, Task *t)
{
int32         ret;
Folio        *f;
ItemRoutines *ir;
ItemNode     *n;

    n = (ItemNode *)LookupItem(it);
    if (!n)
        return BADITEM;

    if (t && (!EnoughPriv(n,t)))
    {
#ifdef BUILD_STRINGS
        printf("WARNING: %s '%s' is trying to delete item 0x%x\n", (CURRENTTASK->t_ThreadTask ? "Thread" : "Task"), CURRENTTASK->t.n_Name, it);
        printf("         (subsys '%s', type %d", WhichFolio(n->n_SubsysType)->fn.n_Name, n->n_Type);
        if ((n->n_Flags & NODE_NAMEVALID) && (n->n_Name))
            printf(", name '%s')\n",n->n_Name);
        else
            printf(")\n");
        printf("         but either doesn't own the item or doesn't have enough privilege\n");
#endif

        return BADPRIV;
    }

    t = (Task *)LookupItem(n->n_Owner);
    if (!t)
    {
#ifdef BUILD_STRINGS
        printf("WARNING: %s '%s' is trying to delete item 0x%x\n", (CURRENTTASK->t_ThreadTask ? "Thread" : "Task"), CURRENTTASK->t.n_Name, it);
        printf("         (subsys '%s', type %d", WhichFolio(n->n_SubsysType)->fn.n_Name, n->n_Type);
        if ((n->n_Flags & NODE_NAMEVALID) && (n->n_Name))
            printf(", name '%s')\n",n->n_Name);
        else
            printf(")\n");
        printf("         but this item doesn't have a valid owner!?\n");
#endif

        return BADITEM;
    }

    if (n->n_ItemFlags & ITEMNODE_DELETED)
    {
        /* it's already in the process of being deleted */
        return BADITEM;
    }

    /* avoid recursion */
    n->n_ItemFlags |= ITEMNODE_DELETED;

    f = WhichFolio(n->n_SubsysType);
    DBUGDI(("realDeleteItem, f=%lx\n",(uint32)f));

    ir = f->f_ItemRoutines;

    ret = (*ir->ir_Delete)(it,t);

    if (ret > 0)
    {
        /* Don't remove/free the item */
        ret = 0;
    }
    else if (ret == 0)
    {
        LogItemDeleted(n);
        RemoveItem(t,it);
        FreeNode(f, n);
    }
    else
    {
        /* something went arry... */
        n->n_ItemFlags &= ~(ITEMNODE_DELETED);

#ifdef BUILD_STRINGS
        printf("WARNING: The '%s' folio refused to delete item 0x%x\n", f->fn.n_Name, it);
        printf("         (subsys '%s', type %d", WhichFolio(n->n_SubsysType)->fn.n_Name, n->n_Type);
        if ((n->n_Flags & NODE_NAMEVALID) && (n->n_Name))
            printf(", name '%s')\n",n->n_Name);
        else
            printf(")\n");
        printf("         DeleteItem() called by %s '%s' (TASK_EXITING is %s)\n", (CURRENTTASK->t_ThreadTask ? "thread" : "task"), CURRENTTASK->t.n_Name, (CURRENTTASK->t_Flags & TASK_EXITING ? "set" : "clear"));
        printf("         ");
        PrintfSysErr(ret);
#endif
    }

    DBUGDI(("realDeleteItem, ret=%d\n",ret));

    return ret;
}


int32 internalDeleteItem(Item i)
{
    return realDeleteItem(i, NULL);
}

int32 externalDeleteItem(Item i)
{
    return realDeleteItem(i, CURRENTTASK);
}


/**
|||	AUTODOC -public -class Kernel -group Items -name IsItemOpened
|||	Determines whether a task or thread has opened a given item.
|||
|||	  Synopsis
|||
|||	    Err IsItemOpened( Item task, Item it )
|||
|||	  Description
|||
|||	    This function determines whether a task or thread has currently got
|||	    an item opened.
|||
|||	  Arguments
|||
|||	    task
|||	        The task or thread to inquire about. For the current task,
|||	        use CURRENTTASKITEM.
|||
|||	    it
|||	        The number of the item to verify.
|||
|||	  Return Value
|||
|||	    >= 0 if the item was opened, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    OpenItem(), CloseItem(), CheckItem(), LookupItem()
|||
**/

Err IsItemOpened(Item task, Item i)
{
Task *t;

    t = (Task *)CheckItem(task, KERNELNODE,TASKNODE);
    if (t == NULL)
	return BADITEM;

    if (FindItemSlot(t, i | ITEM_WAS_OPENED) < 0)
	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Items -name FindAndOpenItem
|||	Finds an item by type and tags and opens it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenItem( int32 cType, TagArg *tp )
|||	    Item FindAndOpenItemVA( int32 cType, uint32 tags, ... );
|||
|||	  Description
|||
|||	    This function finds an item of the specified type whose tag
|||	    arguments match those pointed to by the tp argument. If more than
|||	    one item of the specified type has matching tag arguments, this
|||	    function returns the item number for the first matching item. When
|||	    an item is found, it is automatically opened and prepared for use.
|||
|||	  Arguments
|||
|||	    cType
|||	        Specifies the type of the item to find. Use MkNodeID() to
|||	        generate this value.
|||
|||	    tp
|||	        A pointer to an array of tag arguments. The tag arguments can
|||	        be in any order. The array can contain some, all, or none of
|||	        the possible tag arguments for an item of the specified type;
|||	        to make the search more specific, include more tag arguments.
|||	        The last element of the array must be the value TAG_END. If
|||	        there are no tag arguments, this argument must be NULL.
|||
|||	  Return Value
|||
|||	    Returns the number of the first item that matches or a negative
|||	    error code if it can't find the item or if an error occurs. The
|||	    returned item is already opened, so there is no need to call
|||	    OpenItem() on it. You should call CloseItem() on the supplied item
|||	    when you are done with it.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    CheckItem(), FindNamedItem(), LookupItem(),
|||	    MkNodeID(), OpenItem(), FindItem()
|||
**/

Item internalFindAndOpenItem(int32 cntype, TagArg *tags, Task *t)
{
uint8         ntype;
Folio        *f;
ItemRoutines *ir;
Item          result;

    DBUGFI(("FaOI: type 0x%x\n", cntype));

    /* try and find it in memory first */
    result = internalFindItem(cntype,tags);
    if (result >= 0)
    {
        DBUGFI(("FaOI: found 0x%x\n", result));
        /* if it was found, then open it */
        return (internalOpenItem(result,NULL,t));
    }
    else if ((result & 0x1ff) != ER_NotFound)
    {
        /* if ir_Find failed, except for a NotFound error, return */
        return result;
    }

    DBUGFI(("FaOI: looking for folio\n"));

    /* see what folio we need to talk to for this item type */
    f = WhichFolio(SUBSYSPART(cntype));
    if (f == NULL)
    {
        /* unknown item type */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadType);
    }

    ntype = TYPEPART(cntype);
    if ((ntype < 0) || (ntype > f->f_MaxNodeType))
    {
        /* being asked to access an item type that the folio doesn't know about */
        return BADSUBTYPE;
    }

    ir = f->f_ItemRoutines;
    if (ir->ir_Load == NULL)
    {
        /* no load vector for this folio, so fail... */
        return result;
    }

    DBUGFI(("FaOI: loading from folio\n"));

    /* load it! */
    result = (*ir->ir_Load)(ntype,tags);
    if (result >= 0)
    {
        DBUGFI(("FaOI: opening loaded item\n"));

        /* if it was loaded, then find and open it */
        result = internalFindItem(cntype,tags);
        if (result >= 0)
            result = internalOpenItem(result,NULL,t);
    }

    return result;
}

Item externalFindAndOpenItem(int32 cntype, TagArg *tags)
{
    return internalFindAndOpenItem (cntype, tags, CURRENTTASK);
}


/*****************************************************************************/


Err TransferItems(Item recipient)
{
uint32    i;
Task     *ct;
Item      it;
ItemNode *n;
Err       err;

    ct = CURRENTTASK;
    if (ct->t_ResourceTable == NULL)
        return 0;

    err = 0;
    for (i = 0; i < ct->t_ResourceCnt; i++)
    {
        it = ct->t_ResourceTable[i] & (~ITEM_WAS_OPENED);
        n  = (ItemNode *)LookupItem(it);
        if (n)
        {
            if (ct->t_ResourceTable[i] & ITEM_WAS_OPENED)
            {
                err = OpenItemAsTask(it, NULL, recipient);
#ifdef BUILD_STRINGS
                if (err < 0)
                {
		    printf("WARNING: Unable to open item $%06x (%s)\n",it, n->n_Name);
		    printf("         (type %d, subtype %d) for task '%s', err %d\n",n->n_Type,n->n_SubsysType,TASK(recipient)->t.n_Name,err);
                }
#endif
            }
        }
    }

    for (i = 0; i < ct->t_ResourceCnt; i++)
    {
        it = ct->t_ResourceTable[i] & (~ITEM_WAS_OPENED);
        n  = (ItemNode *)LookupItem(it);
        if (n)
        {
            if ((ct->t_ResourceTable[i] & ITEM_WAS_OPENED) == 0)
            {
                err = SetItemOwner(it, recipient);
#ifdef BUILD_STRINGS
                if (err < 0)
                {
		    printf("WARNING: Unable to transfer ownership of item $%06x (%s)\n",it,n->n_Name);
		    printf("         (type %d, subtype %d) to '%s', err %d\n",n->n_Type,n->n_SubsysType,TASK(recipient)->t.n_Name,err);
                }
#endif
            }
        }
    }

    for (i = 0; i < ct->t_ResourceCnt; i++)
    {
        it = ct->t_ResourceTable[i] & (~ITEM_WAS_OPENED);
        n  = (ItemNode *)LookupItem(it);
        if (n)
        {
            if (ct->t_ResourceTable[i] & ITEM_WAS_OPENED)
            {
                err = CloseItem(it);
#ifdef BUILD_STRINGS
                if (err < 0)
                {
		    printf("WARNING: Unable to close item $%06x (%s)\n",it, n->n_Name);
		    printf("         (type %d, subtype %d), err %d\n",n->n_Type,n->n_SubsysType,err);
                }
#endif
            }
        }
    }

    TOUCH(err);

    return 0;
}


/*****************************************************************************/


void DeleteSubSysItems(NodeSubsysTypes subSys)
{
uint32     block;
uint32     i;
ItemNode  *in;
ItemEntry *table;

    for (block = 0; block < MAX_BLOCKS; block++)
    {
        table = ItemPtrTable[block];
        if (!table)
            break;

        for (i = 0; i < ITEMS_PER_BLOCK; i++)
        {
            in = (ItemNode *)table[i].ie_ItemAddr;
            if (in)
            {
                if (in->n_SubsysType == subSys)
                {
                    internalDeleteItem(in->n_Item);
                }
            }
        }
    }
}


/*****************************************************************************/
/* ItemRoutines private autodocs */

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Find
|||	FindItem() item routine.
|||
|||	  Synopsis
|||
|||	    Item (*ir_Find)(int32 ntype, TagArg *tp)
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific find item
|||	    operation.
|||
|||	  Arguments
|||
|||	    ntype
|||	        Folio-specific node type (without subsystem type) of item to find.
|||
|||	  Return Value
|||
|||	    Returns the number of the first item that matches or a negative error code
|||	    for failure.
|||
|||	    Returns a standard error code containing on ER_NotFound if Item isn't found.
|||	    This special error code causes FindAndOpenItem() to try to load the item
|||	    when not found.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, the kernel uses a
|||	    default item find method, which simply returns the first matching item. Also,
|||	    ITEMNODE_UNIQUE_NAME items aren't allowed without an ir_Find vector.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    FindItem()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Create
|||	CreateItem() item routine.
|||
|||	  Synopsis
|||
|||	    Item (*ir_Create)(void *n, uint8 ntype, void *args)
|||
|||	  Description
|||
|||	    This required item routine implements folio-specific item creation.
|||
|||	    This routine is passed a partially initialized ItemNode to fill out. All
|||	    but the following fields are initialized to 0.
|||
|||	    n_SubsysType
|||	        Set to folio's subsystem type.
|||
|||	    n_Type
|||	        Set to supplied node type.
|||
|||	    n_Flags
|||	        Set to flags from folio's NodeData array.
|||
|||	    n_Size
|||	        Set to size of ItemNode.
|||
|||	    n_ItemFlags
|||	        Set to ITEMNODE_NOTREADY.
|||
|||	    n_Item
|||	        Item number that Item will have when ready.
|||
|||	  Arguments
|||
|||	    n
|||	        Pointer to newly allocated ItemNode structure to be filled out by
|||	        ir_Create(), or -1 if ir_Create() is to allocate the ItemNode.
|||
|||	    ntype
|||	        Folio-specific node type (without subsystem type) of item to create.
|||
|||	    args
|||	        Item-specific TagArg list to be used to create item.
|||
|||	  Return Value
|||
|||	    Item number of the completed Item (non-negative value) if successful, or
|||	    Err code on failure. On failure, the kernel deletes anything which it
|||	    allocated, including a name allocated for TAG_ITEM_NAME. The ir_Create()
|||	    vector is required to clean up anything beyond that.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  Caveats
|||
|||	    !!! I'm a little unclear on the exact behavior of variable-sized ItemNode
|||	    allocations (when n == -1), so that part of this document may not be
|||	    entirely accurate. -peabody
|||
|||	  See Also
|||
|||	    CreateItem(), ir_Delete()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Delete
|||	DeleteItem() item routine.
|||
|||	  Synopsis
|||
|||	    int32 (*ir_Delete)(Item it, struct Task *t)
|||
|||	  Description
|||
|||	    This required item routine implements folio-specific item deletion.
|||
|||	    The kernel sets the ITEMNODE_DELETED flag in n_ItemFlags before dispatching
|||	    the Item to ir_Delete().
|||
|||	  Arguments
|||
|||	    it
|||	        Item number of Item to delete. The kernel makes sure that this Item
|||	        is valid before calling ir_Delete() (how else could it figure out
|||	        which folio's ir_Delete() vector to call?), the caller has
|||	        sufficient privelege to delete the item, and the item doesn't
|||	        already have ITEMNODE_DELETED set in n_ItemFlags.
|||
|||	    t
|||	        Task which owns the Item.
|||
|||	  Return Value
|||
|||	    > 0
|||	        The kernel does not actually delete the Item, but it is left intact with
|||	        the ITEMNODE_DELETED set. (!!! what does this mean anyway?)
|||
|||	    0
|||	        Success. The kernel finishes deleting the Item.
|||
|||	    < 0
|||	        Err code to return. When the kernel receives this, it clears the
|||	        ITEMNODE_DELETED flag and does not delete the Item.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    DeleteItem(), ir_Create()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Open
|||	OpenItem() item routine.
|||
|||	  Synopsis
|||
|||	    Item (*ir_Open)(Node *n, void *args, Task *t)
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific OpenItem()
|||	    operation.
|||
|||	    The kernel increments the usage count of the Item prior to dispatching the
|||	    Item to the folio's ir_Open() vector.
|||
|||	  Arguments
|||
|||	    n
|||	        Pointer to ItemNode for Item to open.
|||
|||	    args
|||	        TagArgs to be used to open the Item. Currently this is always NULL.
|||
|||	    t
|||	        Task on whose behalf Item is to be opened.
|||
|||	  Return Value
|||
|||	    Item number of opened item (non-negative), or Err code on failure. The kernel
|||	    takes care of decrementing the usage count if an error code is returned.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, the kernel
|||	    simply returns the Item number of the Item passed to OpenItem() after
|||	    bumping its usage count.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    OpenItem(), ir_Close()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Close
|||	CloseItem() item routine.
|||
|||	  Synopsis
|||
|||	    int32 (*ir_Close)(Item it,struct Task *t)
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific CloseItem()
|||	    operation.
|||
|||	    The kernel decrements the usage count of the Item prior to dispatching the
|||	    Item to the folio's ir_Close() vector.
|||
|||	  Arguments
|||
|||	    it
|||	        Item number of Item to close. The kernel makes sure that this Item
|||	        is valid before calling ir_Close() and that the caller has sufficient
|||	        privelege to close the item.
|||
|||	    t
|||	        Task on whose behalf Item is to be closed.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, Err code on failure. The kernel takes care of
|||	    restoring the usage count if an error code is returned.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, the kernel
|||	    simply decrements the Item's usage count.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    CloseItem(), ir_Open()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_SetPriority
|||	SetItemPri() item routine.
|||
|||	  Synopsis
|||
|||	    int32 (*ir_SetPriority)(ItemNode *n, uint8 pri, struct Task *t)
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific SetItemPri()
|||	    operation.
|||
|||	    The ir_SetPriority() vector is entirely responsible for validating the new
|||	    priority, setting n_Pri, and doing any list reording that is required.
|||
|||	  Arguments
|||
|||	    n
|||	        ItemNode of Item to set priority.
|||
|||	    pri
|||	        New priority for Item.
|||
|||	    t
|||	        Task on whose behalf Item is to be re-prioritized (not necessarily
|||	        the owner of the Item).
|||
|||	  Return Value
|||
|||	    Old priority (non-negative value) on success, Err code on failure.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, SetItemPri()
|||	    returns NOSUPPORT.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    SetItemPri()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_SetOwner
|||	SetItemOwner() item routine.
|||
|||	  Synopsis
|||
|||	    Err (*ir_SetOwner)(ItemNode *n, Item newOwner, struct Task *t);
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific SetItemOwner()
|||	    operation.
|||
|||	    n_Owner is set to the new owner after ir_SetOwner() returns successfully.
|||
|||	  Arguments
|||
|||	    n
|||	        ItemNode of Item to change ownership.
|||
|||	    newOwner
|||	        Item number of new owner task. The kernel validates this Item.
|||
|||	    t
|||	        Task on whose behalf item ownership is being transfered (not necessarily
|||	        the current owner of the Item).
|||
|||	  Return Value
|||
|||	    Non-negative value on success, Err code on failure.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, SetItemOwner()
|||	    returns NOSUPPORT.
|||
|||	  Implementation
|||
|||	    V20
|||
|||	  See Also
|||
|||	    SetItemOwner()
**/

/**
|||	AUTODOC -private -class Kernel -group ItemRoutines -name ir_Load
|||	FindAndOpenItem() load item routine.
|||
|||	  Synopsis
|||
|||	    Item (*ir_Load)(int32 ntype, TagArg *tp)
|||
|||	  Description
|||
|||	    This optional item routine implements the folio-specific load operation
|||	    requested by FindAndOpenItem() when the Item can't be found in memory. This
|||	    function should do whatever is necessary, including loading a file, to create
|||	    the requested item if possible. When successful, the ir_Load() returns the
|||	    Item to the kernel, which then opens the Item.
|||
|||	  Arguments
|||
|||	    ntype
|||	        Folio-specific node type (without subsystem type) of item to load.
|||
|||	    tp
|||	        TagArgs to be used to load item (e.g., TAG_ITEM_NAME).
|||
|||	  Return Value
|||
|||	    Item number of newly loaded Item, or Err code on failure.
|||
|||	  Default Action
|||
|||	    When this routine isn't provided in the ItemRoutines table, and FindItem()
|||	    was unable to find the item, FindAndOpenItem() returns the error code from
|||	    FindItem().
|||
|||	  Implementation
|||
|||	    V24
|||
|||	  See Also
|||
|||	    FindAndOpenItem()
**/
