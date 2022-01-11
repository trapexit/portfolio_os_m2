#ifndef __KERNEL_PPCPROTLIST_I
#define __KERNEL_PPCPROTLIST_I


/******************************************************************************
**
**  @(#) PPCprotlist.i 96/04/24 1.3
**
******************************************************************************/


#ifndef __KERNEL_PPCLIST_I
#include <kernel/PPCList.i>
#endif

    .struct	ProtectedList
l_Flags		.long	1
l_head_flink	.long	1
l_filler	.long	1
l_tail_blink	.long	1
l_Semaphore	.long	1
    .ends


#endif /* __KERNEL_PPCPROTLIST_I */
