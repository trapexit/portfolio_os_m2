#ifndef __KERNEL_PPCSETJMP_I
#define __KERNEL_PPCSETJMP_I


/******************************************************************************
**
**  @(#) PPCsetjmp.i 96/04/24 1.9
**
******************************************************************************/


#ifndef __HARDWARE_PPCMACROEQU_I
#include <hardware/PPCMacroequ.i>
#endif


	.struct JMP_BUF
jb_CR		.long 1
jb_LR		.long 1
jb_R1		.long 1
jb_R2		.long 1
jb_GPRs		.long 19
jb_FPRs		.long 18
jb_FPSCR	.long 2
jb_SP		.long 1
jb_LT		.long 1
	.ends

#endif /* __KERNEL_PPCSETJMP_I */
