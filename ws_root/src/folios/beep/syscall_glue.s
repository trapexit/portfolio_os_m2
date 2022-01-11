/* @(#) syscall_glue.s 96/05/29 1.6 */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* Calls the system call handler, the correct arguments are already in
 * the registers (r3 = folio/selector code, r4-r10 has the arguments)
 * We just do an sc and a return.
 */
	.macro
	SYSTEMCALL	&number
	lis		r0,(((&number)>>16) & 0xFFFF)
	ori		r0,r0,((&number) & 0xFFFF)
	sc
	.endm

/* Must correspond with ID NST_BEEP assigned in <kernel/nodes.h> */
BEEPFOLIO .equ 0x080000


/*****************************************************************************/

	DECFN	ConfigureBeepChannel
	SYSTEMCALL	BEEPFOLIO+0

	DECFN	SetBeepParameter
	SYSTEMCALL	BEEPFOLIO+1

	DECFN	SetBeepVoiceParameter
	SYSTEMCALL	BEEPFOLIO+2

	DECFN	SetBeepChannelData
	SYSTEMCALL	BEEPFOLIO+3

	DECFN	HackBeepChannelDataNext
	SYSTEMCALL	BEEPFOLIO+4

	DECFN	StartBeepChannel
	SYSTEMCALL	BEEPFOLIO+5

	DECFN	StopBeepChannel
	SYSTEMCALL	BEEPFOLIO+6

	DECFN	GetBeepTime
	SYSTEMCALL	BEEPFOLIO+7

