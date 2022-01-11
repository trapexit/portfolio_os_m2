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

GRAPHICSFOLIO	.equ 0x020000


/*****************************************************************************/


	DECFN	ModifyGraphicsItem
	SYSTEMCALL	GRAPHICSFOLIO+0

	DECFN	AddViewToViewList
	SYSTEMCALL	GRAPHICSFOLIO+1

	DECFN	RemoveView
	SYSTEMCALL	GRAPHICSFOLIO+2

	DECFN	OrderViews
	SYSTEMCALL	GRAPHICSFOLIO+3

	DECFN	LockDisplay
	SYSTEMCALL	GRAPHICSFOLIO+4

	DECFN	UnlockDisplay
	SYSTEMCALL	GRAPHICSFOLIO+5

	DECFN	ActivateProjector
	SYSTEMCALL	GRAPHICSFOLIO+6

	DECFN	DeactivateProjector
	SYSTEMCALL	GRAPHICSFOLIO+7

	DECFN	SetDefaultProjector
	SYSTEMCALL	GRAPHICSFOLIO+8
