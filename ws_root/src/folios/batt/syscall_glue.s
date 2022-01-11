/* @(#) syscall_glue.s 96/11/13 1.5 */

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

BATTFOLIOSWI	.equ 0x090000


/*****************************************************************************/


	DECFN	WriteBattMem
	SYSTEMCALL	BATTFOLIOSWI+0

	DECFN	ReadBattMem
	SYSTEMCALL	BATTFOLIOSWI+1

	DECFN	WriteBattClock
	SYSTEMCALL	BATTFOLIOSWI+2

	DECFN	ReadBattClock
	SYSTEMCALL	BATTFOLIOSWI+3

	DECFN	LockBattMem
	SYSTEMCALL	BATTFOLIOSWI+4

	DECFN	UnlockBattMem
	SYSTEMCALL	BATTFOLIOSWI+5
