#ifndef __KERNEL_PPCITEM_I
#define __KERNEL_PPCITEM_I


/******************************************************************************
**
**  @(#) PPCitem.i 96/04/24 1.10
**
******************************************************************************/


#define SEM_WAIT	1
#define SEM_SHAREDREAD	2

//  common TagArg commands for all Items
//  All system routines that create Items must assign their
//  TAGS after ITEM_TAG_LAST

#define TAG_ITEM_NAME		1
#define TAG_ITEM_PRI		1+TAG_ITEM_NAME
#define TAG_ITEM_VERSION	1+TAG_ITEM_PRI
#define TAG_ITEM_REVISION	1+TAG_ITEM_VERSION
#define TAG_ITEM_UNIQUE_NAME	1+TAG_ITEM_REVISION
#define TAG_ITEM_RESERVED6	1+TAG_ITEM_UNIQUE_NAME
#define TAG_ITEM_RESERVED7	1+TAG_ITEM_RESERVED6
#define TAG_ITEM_RESERVED8	1+TAG_ITEM_RESERVED7
#define TAG_ITEM_RESERVED9	1+TAG_ITEM_RESERVED8
#define TAG_ITEM_LAST		TAG_ITEM_RESERVED9


    .struct	ItemEntry
ie_ItemAddr	.long	1	// void *ie_ItemAddr
ie_ItemInfo	.long	1	// uint32 ie_ItemInfo
    .ends

#define ITEM_GEN_MASK		0x7fff0000
#define ITEM_INDX_MASK		0x00003fff
#define ITEM_FLGS_MASK		0x0000c000


#endif /* __KERNEL_PPCITEM_I */
