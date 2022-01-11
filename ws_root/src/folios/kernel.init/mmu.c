/* @(#) mmu.c 96/08/19 1.16 */

#include <kernel/types.h>
#include <kernel/internalf.h>
#include <kernel/kernel.h>
#include <kernel/panic.h>
#include <kernel/mem.h>
#include <hardware/PPCasm.h>
#include <stdio.h>


#define DBUG(x)	 /* printf x */

extern void segreg_init(void);
extern void XTLBInit(void *);


void SetupMMU(void)
{
	vuint32  msr, hid;

	DBUG(("Entering SetupMMU\n"));

	/*
	 *  Turn MSR_IP off for exceptions to vector using IBR
	 *  Turn off translation
	 */
	msr = _mfmsr();
	msr &= ~(MSR_DR|MSR_IR|MSR_IP);
	_mtmsr(msr);

	/*
	 *  Initialize segment register 0 for X-mode
	 */
	segreg_init();

	DBUG(("Initializing all TLB entries\n"));

	XTLBInit((void *)KB_FIELD(kb_MemRegion)->mr_MemBase);

	DBUG(("Disabling BATs\n"));

#if 0
	{
	    unsigned long long ibat1, dbat2;

	    /* NOTE: Uncomment the following lines only */
	    /*       if we want to reclaim the IBAT1    */
	    /* Don't use IBAT1 for user and supervisor   */
	    ibat1 = _mfibat1();
	    _mtibat1((uint32)((ibat1>>32) &~ (UBAT_VS|UBAT_VP)), (uint32)ibat1);

	    /* NOTE: Uncomment the following lines only */
	    /*       if we want to reclaim the DBAT2    */
	    /* Don't use DBAT2 for user and supervisor */
	    dbat2 = _mfdbat2();
	    _mtdbat2((uint32)((dbat2>>32) &~ (UBAT_VS|UBAT_VP)), (uint32)dbat2);
	}
#endif
	/*
	 *  Turn on Xmode
	 */
	hid = _mfhid0();
	hid &= ~HID_WIMG;
	hid |= (HID_XMODE);
	_mthid0(hid);

	DBUG(("Re-enabling translation\n"));

	/* Re-enable translation */
	msr |= (MSR_DR|MSR_IR);
        _mtmsr(msr);
}
