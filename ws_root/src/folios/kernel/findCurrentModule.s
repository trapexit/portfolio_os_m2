/* @(#) findCurrentModule.s 96/04/24 1.1 */

#include <hardware/PPCMacroequ.i>

	DECFN	FindCurrentModule
	mflr	r3
	b	FindModuleByAddress
