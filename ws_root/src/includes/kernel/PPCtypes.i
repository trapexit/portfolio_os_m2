#ifndef __KERNEL_PPCTYPES_I
#define __KERNEL_PPCTYPES_I


/******************************************************************************
**
**  @(#) PPCtypes.i 96/04/24 1.4
**
******************************************************************************/


    .struct	TagArg
ta_Tag		.long	1
ta_Arg		.long	1
    .ends

#define TAG_END  	0
#define TAG_JUMP 	0xfffffffe
#define TAG_NOP  	0xffffffff

#endif /* __KERNEL_PPCTYPES_I */
