#ifndef __KERNEL_INTERRUPTS_H
#define __KERNEL_INTERRUPTS_H


/******************************************************************************
**
**  @(#) interrupts.h 96/06/10 1.20
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif


typedef struct FirqList
{
    Node fl;
    List fl_Firqs;
} FirqList;

typedef struct FirqNode
{
    ItemNode  firq;
    void    (*firq_Code)(struct FirqNode *firqP);   /* ptr to routine to call */
    int32     firq_Data;    /* local data for handler */
    int32     firq_Num;     /* which firq int we handle */
} FirqNode;

enum firq_tags
{
    CREATEFIRQ_TAG_DATA = TAG_ITEM_LAST+1,
    CREATEFIRQ_TAG_CODE,
    CREATEFIRQ_TAG_NUM
};


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern Item CreateFIRQ(const char *name, uint8 pri, int32 (*code)(void), int32 num);

#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#define DeleteFIRQ(x)	DeleteItem(x)

/* sources in interrupt word 0 */

/* M2 interrupts - added by Kevin Hester */

/*
   An important note about M2 firq numbers:

   The format of these FIRQ numbers is significant.
   The lower 5 bits of the code is the subdevice code.
   The upper bits tell the device:
   0 = BDA
   1 = CDE
*/

/* First level BDA interrupts */

#define INT_BDA_CLIP    (31-0)
#define INT_BDA_LEND    (31-1)
#define INT_BDA_IMINST  (31-2)
#define INT_BDA_DFINST  (31-3)
#define INT_BDA_TEGEN   (31-4)
#define INT_BDA_MPG     (31-5)
#define INT_BDA_DSPP	(31-6)
#define INT_BDA_V0	(31-7)
#define INT_BDA_V1      (31-8)
#define INT_V1          INT_BDA_V1
#define INT_BDA_PB      (31-9)
#define INT_BDA_CEL     (31-10)
#define INT_BDA_MPGVBD	(31-11)
#define INT_BDA_TO      (31-22)
#define INT_BDA_WVIOL   (31-23)

#define INT_BDA_PVIOL   (31-24)
#define INT_BDA_EXT1    (31-25)
#define INT_BDA_EXT2    (31-26)
#define INT_BDA_EXT3    (31-27)
#define INT_BDA_EXT4    (31-28)

/* Aliases for bridgit/cde locations */
#define INT_BDA_BRIDGIT (INT_BDA_EXT3)
#define INT_BDA_CDE     (INT_BDA_EXT4)

/* Second level CDE interrupts */

#define INT_CDE_BASE    (1 << 5)
#define INT_CDE_INTSENT (INT_CDE_BASE + 31-0)
#define INT_CDE_RSV1    (INT_CDE_BASE + 31-1)
#define INT_CDE_RSV2    (INT_CDE_BASE + 31-2)
#define INT_CDE_SDBGW   (INT_CDE_BASE + 31-3)
#define INT_CDE_SDBGR   (INT_CDE_BASE + 31-4)
#define INT_CDE_MC      (INT_CDE_BASE + 31-5)
#define INT_CDE_RSV3    (INT_CDE_BASE + 31-6)
#define INT_CDE_RSV4    (INT_CDE_BASE + 31-7)
#define INT_CDE_RSV5    (INT_CDE_BASE + 31-8)
#define INT_CDE_CBLK2   (INT_CDE_BASE + 31-9)
#define INT_CDE_CBLK1   (INT_CDE_BASE + 31-10)
#define INT_CDE_IDRDY   (INT_CDE_BASE + 31-11)
#define INT_CDE_RSV6    (INT_CDE_BASE + 31-12)
#define INT_CDE_DEV6    (INT_CDE_BASE + 31-13)
#define INT_CDE_DEV5    (INT_CDE_BASE + 31-14)
#define INT_CDE_RSV7    (INT_CDE_BASE + 31-15)
#define INT_CDE_DEV7    (INT_CDE_BASE + 31-16)
#define INT_CDE_SFLOW2  (INT_CDE_BASE + 31-17)
#define INT_CDE_SFLOW1  (INT_CDE_BASE + 31-18)
#define INT_CDE_RSV8    (INT_CDE_BASE + 31-19)
#define INT_CDE_DMAC5   (INT_CDE_BASE + 31-20)
#define INT_CDE_DMAC4   (INT_CDE_BASE + 31-21)
#define INT_CDE_RSV9    (INT_CDE_BASE + 31-22)
#define INT_CDE_DMAC2   (INT_CDE_BASE + 31-23)
#define INT_CDE_DMAC1   (INT_CDE_BASE + 31-24)
#define INT_CDE_TEA     (INT_CDE_BASE + 31-25)
#define INT_CDE_CDCW    (INT_CDE_BASE + 31-26)
#define INT_CDE_CDSR    (INT_CDE_BASE + 31-27)
#define INT_CDE_CDSFL   (INT_CDE_BASE + 31-28)
#define INT_CDE_GPIO1INT (INT_CDE_BASE + 31-29)
#define INT_CDE_GPIO2INT (INT_CDE_BASE + 31-30)
#define INT_CDE_BIOERRINT (INT_CDE_BASE + 31-31)

#define INTR_MAX	64


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


void EnableInterrupt(uint32 firqNum);
void DisableInterrupt(uint32 firqNum);
void ClearInterrupt(uint32 firqNum);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */

#define INTSTACKSIZE		1024*2		/* interrupt stack size	*/


#endif /* __KERNEL_INTERRUPTS_H */
