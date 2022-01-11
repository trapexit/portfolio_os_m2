#ifndef __KERNEL_PPCSEMAPHORE_I
#define __KERNEL_PPCSEMAPHORE_I


/******************************************************************************
**
**  @(#) PPCsemaphore.i 96/04/24 1.9
**
******************************************************************************/


#ifndef __KERNEL_PPCNODES_I
#include <kernel/PPCnodes.i>
#endif

#ifndef __KERNEL_PPCITEM_I
#include <kernel/PPCitem.i>
#endif

#ifndef __KERNEL_PPCLIST_I
#include <kernel/PPCList.i>
#endif

    .struct	SemaphoreWaitNode
swn		.byte	MinNode
swn_Task	.long	1
swn_Shared	.byte	1
    .ends

    .struct	Semaphore
s			.byte	ItemNode
sem_Locker		.long	1
sem_NestCnt		.long	1
sem_TaskWaitingList	.byte	List
sem_SharedLockers	.long	1
sem_UserData		.long	1
    .ends


#endif /* __KERNEL_PPCSEMAPHORE_I */
