/*
 *  @(#) syscall_glue.s 96/11/13 1.3
 *  Assembly stubs for SWI calls.
 */


#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* Calls the system call handler, the correct arguments are already in
 * the registers (r3 = folio/selector code, r4-r10 has the arguments)
 * We just do an sc and a return.
 */
	.macro
	SYSTEMCALL	&number

#ifdef BUILD_DEBUGGER
	mflr		r10
	stw		r10,4(r1)
#endif

	lis		r0,(((&number)>>16) & 0xFFFF)
	ori		r0,r0,((&number) & 0xFFFF)
	sc
	.endm

ICONFOLIO	.equ 0x070000


/*****************************************************************************/


	DECFN		GetHWIcon
	SYSTEMCALL	ICONFOLIO+0

