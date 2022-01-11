/*
 *	@(#) m2sysdep.c 96/05/10 1.68
 *	Copyright 1994,1995, The 3DO Company
 *
 * Miscellaneous system-dependent code.
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/bda.h"
#include "hardware/cde.h"
#include "dipir.h"
#include "insysrom.h"


/*****************************************************************************
  Static variables
*/

/*****************************************************************************
  External functions
*/

extern void ReadTimeBase(TimerState *tm);
extern void SubTimer(TimerState *tm1, TimerState *tm2, TimerState *result);
extern void InitMicroslotForBoot(void);

/*****************************************************************************
  InitTimer
*/
void 
InitTimer(void)
{
}

/*****************************************************************************
  RestoreTimer
*/
void 
RestoreTimer(void)
{
}


/*****************************************************************************
  Read the PPC time base and store away as a reference.
*/
void 
ResetTimer(TimerState *tm)
{
	ReadTimeBase(tm);
}

/*****************************************************************************
*/
static uint32 
ReadTimer(TimerState *tm)
{
	TimerState now;
	TimerState delta;

	ReadTimeBase(&now);
	SubTimer(tm, &now, &delta);
	if (delta.ts_TimerHigh != 0)
	{
		EPRINTF(("Timer delta too big! %x:%x - %x:%x\n",
			now.ts_TimerHigh, now.ts_TimerLow,
			tm->ts_TimerHigh, tm->ts_TimerLow));
		delta.ts_TimerLow = 0xFFFFFFFF;
	}
	if (delta.ts_TimerLow == 0)
	{
		EPRINTF(("Timer delta zero! %x:%x - %x:%x\n",
			now.ts_TimerHigh, now.ts_TimerLow,
			tm->ts_TimerHigh, tm->ts_TimerLow));
	}
	return delta.ts_TimerLow;
}

/*****************************************************************************
  Read the PPC time base again and return the delta between the 
  current time and the time base stored away by ResetTimer.
*/
uint32 
ReadMicroTimer(TimerState *tm)
{
	return ReadTimer(tm) / TICKS_PER_MICROSEC(dtmp->dt_BusClock);
}

/*****************************************************************************
  Read the PPC time base again and return the delta between the 
  current time and the time base stored away by ResetTimer.
*/
uint32 
ReadMilliTimer(TimerState *tm)
{
	return ReadTimer(tm) / TICKS_PER_MILLISEC(dtmp->dt_BusClock);
}

/*****************************************************************************
  Set the enable interrupt bit and recoverable from exception bits in the MSR.
*/
void 
EnableInterrupts(void)
{
	uint32 msr;

	msr = GetMSR();
	msr |= MSR_EE|MSR_RI;
	SetMSR(msr);
}

/*****************************************************************************
  DisableInterrupts

  Clear the enable interrupt bit in the MSR.
*/
void 
DisableInterrupts(void)
{
	uint32 msr;

	msr = GetMSR();
	msr &= ~MSR_EE;
	SetMSR(msr);
}


#ifdef PREFILLSTACK
/*****************************************************************************
  Debugging aid to see where the low (high?) water mark is on the stack
*/
void 
DisplayStackUsage(void)
{
	uint32 *pStack;
	uint32 *pStackTop;
	uint32 *pStackBase;


	pStackTop = (uint32*) (DIPIRBUFSTART + DIPIRBUFSIZE);
	pStackBase = (uint32*) (DIPIRBUFSTART + DIPIRBUFSIZE - DIPIRSTACKSIZE);

	/* Find the low water mark */
	/* Skip the first 4 words, they are reserved for kludge stuff */
	for (pStack = pStackBase;  pStack < pStackTop;  pStack++)
		if (*pStack != 0xDEADFACE)
			break;
	PRINTF(("Greatest stack depth = %d (0x%x) bytes\n",
	       (uint8*)pStackTop - (uint8*)pStack,
	       (uint8*)pStackTop - (uint8*)pStack));
}
#endif /* PREFILLSTACK */

/*****************************************************************************
  Something's very wrong; stop dead.
*/
void 
HardBoot(void)
{
	/* Can't always PRINTF here; we might not be fully initialized. */
#ifdef DIPIR_INTERRUPTS
	DisableInterrupts();
#endif
	for (;;) ;
}

/*****************************************************************************
*/
int32
SoftReset(void)
{
	return (*theBootGlobals->bg_PerformSoftReset)();
}

/*****************************************************************************
*/
	void
InitForBoot(void)
{
	/* Mask all interrupts. */
	ClearPowerBusBits(M2_DEVICE_ID_BDA, BDAPCTL_PBINTENSET, ~0);
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_INT_ENABLE, ~0);
	/* Stop all CDE DMA. */
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_DMA1_CNTL, CDE_DMA_CURR_VALID);
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_DMA2_CNTL, CDE_DMA_CURR_VALID);
	/* Stop all LCCD DMA. */
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_CD_DMA1_CNTL, CDE_DMA_CURR_VALID);
	ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_CD_DMA2_CNTL, CDE_DMA_CURR_VALID);
	InitMicroslotForBoot();
}

/*****************************************************************************
*/
#ifdef DIPIR_INTERRUPTS

uint32 numInterrupts = 0;

void
KickAnimation(void)
{
	VideoPos pos;  
	pos.x = 20 + (numInterrupts % 280);
	pos.y = 66 + (3 * (numInterrupts / 280)); 
	SetPixel(pos, 0x46AE);
	FlushDCacheAll();
}

void
InterruptHandler(void)
{
	BDA_CLR(BDAVDU_VINT, BDAVINT0_MASK);
	numInterrupts++;
	KickAnimation();

}

#endif /* DIPIR_INTERRUPTS */

