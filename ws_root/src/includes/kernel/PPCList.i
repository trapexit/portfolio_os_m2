#ifndef __KERNEL_PPCLIST_I
#define __KERNEL_PPCLIST_I


/******************************************************************************
**
**  @(#) PPCList.i 96/04/24 1.9
**
******************************************************************************/


#ifndef __KERNEL_PPCNODES_I
#include <kernel/PPCnodes.i>
#endif

    .struct	Link
flink	.long	1	// struct Link *flink	- forward (next) link
blink	.long	1	// struct Link *blink	- backward (prev) link
    .ends

    .struct	List
l_Flags		.long	1
l_head_flink	.long	1
l_filler	.long	1
l_tail_blink	.long	1
    .ends

#define l_head_blink	List.l_filler
#define l_tail_flink	List.l_filler
#define l_Head		List.l_head_flink
#define l_Tail		List.l_tail_blink
#define l_Middle	List.l_filler


#endif /* __KERNEL_PPCLIST_I */
