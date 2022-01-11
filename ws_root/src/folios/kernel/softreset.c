/* @(#) softreset.c 96/08/19 1.16 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <string.h>
#include <stdio.h>
#include <kernel/internalf.h>

#define DBUG(x)	/*printf x */


/*****************************************************************************/

int32 SoftReset(void) {

	int32	(*SoftResetFunc)(void),
		result;

	uint32	oldints;

	SoftResetFunc = (int32 (*)())(KB_FIELD(kb_PerformSoftReset));
	oldints = Disable();

	/* Refresh slave decrementer, then allow any pending interrupt to occur */
	SuperSlaveRequest(SLAVE_SRESETPREP, 1, 0, 0);

	/* Store slave state, disable caches, and put slave in doze mode */
	SuperSlaveRequest(SLAVE_SRESETPREP, 0, 0, 0);

	/* Initiate soft reset */
	do {
		result = (*SoftResetFunc)();
	} while (result == 0);

	/* Restore slave state */
	SuperSlaveRequest(SLAVE_SRESETRECOVER, 0, 0, 0);

	Enable(oldints);
	return result;
}

