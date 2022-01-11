#ifndef __PROTLIST_H
#define __PROTLIST_H


/******************************************************************************
**
**  @(#) protlist.h 96/02/20 1.3
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

/* FIXME:  since ProtectedLists don't exist beyond the Operator: */
#define NEED_CAST

#ifdef NEED_CAST
#define CAST (ProtectedList*)
#else
#define CAST
#endif


/*****************************************************************************/


/* The first l_Flags entry! */
#define LISTF_PROTECTED 0x00000001

typedef struct ProtectedList
{
	uint32     l_Flags;	/* Dale's unused flags */
	ListAnchor ListAnchor;	/* Anchor point for list of nodes */
	Item       l_Semaphore;
} ProtectedList, *ProtectedListP;

/*****************************************************************************/


/* Scan protected list l, for nodes n, of type t.  Use these macros thusly:
 *
 * StartScanProtectedList(&myList, mnp, MyNodeType)
 * {
 *	DoSomthingToMyNode(mnp);
 *	DoSomthingElseToMyNode(mnp);
 * }
 * EndScanProtectedList(&myList);
 *
 * WARNING: You cannot remove the current node from the list when using this
 *          macro.
 */
#define StartScanProtectedList(pl,n,t) do { \
	if(LockSemaphore((CAST pl)->l_Semaphore, SEM_WAIT | SEM_SHAREDREAD) >= 0) \
	{ for (n=(t *)FirstNode(pl);IsNode(pl,n);n=(t *)NextNode(n))
#define EndScanProtectedList(pl) UnlockSemaphore((CAST pl)->l_Semaphore); } \
	} while(0)

/* Scan a protected list backward, from tail to head */
#define StartScanProtectedListB(pl,n,t) do { \
	if(LockSemaphore((CAST pl)->l_Semaphore, SEM_WAIT | SEM_SHAREDREAD) >= 0) \
	{ for (n=(t *)LastNode(pl);IsNodeB(pl,n);n=(t *)PrevNode(n))
#define EndScanProtectedListB(pl) EndScanProtectedList(pl)

/* A macro to let you define statically-initialized protected lists.  Use
 * this macro in the same fashion as for normal lists.
 *
 * WARNING:  does not allocate the semaphore!
 */
#define PREPPROTECTEDLIST(l) \
	{LISTF_PROTECTED,{(Link *)&l.l_Filler,NULL,(Link *)&l.l_Head},-1}
#define PrepProtectedList(l) PrepList((List*)(l))

/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define AccessProtectedListWith(operation,pl) do { \
	if(LockSemaphore((CAST pl)->l_Semaphore, SEM_WAIT | SEM_SHAREDREAD) >= 0) { \
		operation; \
		UnlockSemaphore((CAST pl)->l_Semaphore); } } while(0)

#define ModifyProtectedListWith(operation,pl) do { \
	if(LockSemaphore((CAST pl)->l_Semaphore, SEM_WAIT) >= 0) { \
		operation; \
		UnlockSemaphore((CAST pl)->l_Semaphore); } } while(0)

#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* #ifndef __PROTLIST_H */
