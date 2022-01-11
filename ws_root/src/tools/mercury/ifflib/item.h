#ifndef __KERNEL_ITEM_H
#define __KERNEL_ITEM_H


/******************************************************************************
**
**  @(#) item.h 95/06/14 1.14
**  $Id: item.h,v 1.18 1994/12/21 21:10:09 vertex Exp $
**
**  Kernel item management definitions
**
**  The programmer interface is done via Items, which are passed to the system
**  instead of direct pointers. The system keeps a database of Items and their
**  associated structures
**
******************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


Item  CreateItem(int32 ctype, TagArg *p);
Item  CreateSizedItem(int32 ctype, TagArg *p, int32 size);
Err   DeleteItem(Item i);
Item  FindItem(int32 ctype, TagArg *tp);
Err   CloseItem(Item i);
Item  OpenItem(Item foundItem, void *args);
int32 SetItemPri(Item i, uint8 newpri);
Err   SetItemOwner(Item i, Item newOwner);
int32 LockItem(Item s, uint32 flags);
Err   UnlockItem(Item s);
Item  FindAndOpenItem(int32 ctype, TagArg *tp);

/* varargs variants of some of the above */
Item CreateItemVA(int32 ctype, uint32 tags, ...);
Item CreateSizedItemVA(int32 ctype, int32 size, uint32 tags, ...);
Item FindItemVA(int32 ctype, uint32 tags, ...);
Item FindAndOpenItemVA(int32 ctype, uint32 tags, ...);

/* learn more about an item */
void *LookupItem(Item i);
void *CheckItem(Item i, uint8 ftype, uint8 ntype);
Err IsItemOpened(Item task, Item i);

/* convenience routines */
Item FindNamedItem(int32 ctype, const char *name);
Item FindAndOpenNamedItem(int32 ctype, const char *name);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

/* flags for LockItem() and LockSemaphore() */
#define SEM_WAIT        1
#define SEM_SHAREDREAD  2

/* Common tags for all Items, useful when creating any type of item. */
enum item_tags
{
	TAG_ITEM_END = TAG_END,	/* 0 */
	TAG_ITEM_NAME,		/* 1 */
	TAG_ITEM_PRI,		/* 2 */
	TAG_ITEM_VERSION,	/* 3 */
	TAG_ITEM_REVISION,	/* 4 */
	TAG_ITEM_CONSTANT_NAME, /* 5 */
	TAG_ITEM_UNIQUE_NAME,   /* 6 */
	TAG_ITEM_RESERVED7,
	TAG_ITEM_RESERVED8,
	TAG_ITEM_RESERVED9,
	TAG_ITEM_LAST = TAG_ITEM_RESERVED9
};

#ifndef EXTERNAL_RELEASE

typedef struct ItemEntry
{
	void	*ie_ItemAddr;
	uint32	ie_ItemInfo;
} ItemEntry;

#define	ITEM_GEN_MASK	0x7fff0000
#define ITEM_INDX_MASK	0x00003fff
#define ITEM_FLGS_MASK	0x0000c000

#endif /* EXTERNAL_RELEASE */

/* argc values passed to main() of demand-loaded items */
#define DEMANDLOAD_MAIN_CREATE -1
#define DEMANDLOAD_MAIN_DELETE -2


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects FindItem, FindItemVA, FindNamedItem, FindVersionedItem
#pragma no_side_effects LookupItem, CheckItem, IsItemOpened
#endif


/*****************************************************************************/


#endif /* __KERNEL_ITEM_H */
