/* @(#) spinlock.s 96/09/03 1.3 */

#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


	.struct SpinLock
sl_Lock		.long	1
sl_Pad0		.long	7
sl_CPU0		.long	1
sl_Pad1		.long	7
sl_CPU1		.long	1
sl_Cookie	.long	1
	.ends


        .macro
        write   &reg1,&reg2
        stw	&reg1,0(&reg2)
        dcbf	0,&reg2
        sync
        .endm

	.macro
	read	&reg1,&reg2
	dcbf	0,&reg2
	lwz	&reg1,0(&reg2)
	dcbf	0,&reg2
	.endm


/*****************************************************************************/


/* bool ObtainSpinLock(SpinLock *lock);
 *                          r3
 */

	DECFN	ObtainSpinLock

	la	r10,SpinLock.sl_Lock(r3)
	la	r11,SpinLock.sl_CPU0(r3)
	la	r12,SpinLock.sl_CPU1(r3)

 	// read CPU number
	esa					// so we can do fancy things
	mfsr	r4,SR14				// get CPU #
	dsa					// return to normal

	// see which CPU this is
	cmpwi	r4,0				// is it CPU 0?
	li	r3,1				// prepare for acquisition
	bne	proc1				// if CPU 1, skip ahead

	// this is processor 0
	write	r3,r11				// indicate CPU 0 is here
	read	r4,r12				// see if CPU 1 is also here
	cmpwi	r4,0				// check state
	bne-	0$				// if it's already here, fail

	// see if the spin lock is currently held
	read	r4,r10				// load lock value
	cmpwi	r4,0				// is it 0?
	bne-	0$				// fail if it's held

	write	r3,r10				// grab the lock!
	write	r4,r11				// indicate we're leaving
	blr					// return to caller

0$:	// the other processor got the thing
	li	r3,0				// indicate failure
	write	r3,r11				// indicate we're leaving
	blr					// return to caller


proc1:	// this is processor 1
	write	r3,r12				// indicate CPU 1 is here

	// do this just to make CPU 1 take longer than CPU 0, in order to
	// avoid dead locks between the two CPUs.
	read	r4,r11

	read	r4,r11				// see if CPU 0 is also here
	cmpwi	r4,0				// check state
	bne-	0$				// if it's already here, fail

	// see if the spin lock is currently held
	read	r4,r10				// load lock value
	cmpwi	r4,0				// is it 0?
	bne-	0$				// fail if it's held

	write	r3,r10				// grab the lock!
	write	r4,r12				// indicate we're leaving
	blr					// return to caller

0$:	// the other processor got the thing
	li	r3,0				// indicate failure
	write	r3,r12				// indicate we're leaving
	blr


/*****************************************************************************/


/* void ReleaseSpinLock(SpinLock *lock)
 *                            r3
 *
 * Relinquish a spin lock.
 */

	DECFN	ReleaseSpinLock

	// clear the lock
	li	r4,0				// 0 to relinquish the lock
	la	r10,SpinLock.sl_Lock(r3)	// address of lock word
	write	r4,r10				// zap! the lock is free
	blr
