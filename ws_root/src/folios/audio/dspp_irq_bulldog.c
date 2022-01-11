/******************************************************************************
**
**  @(#) dspp_irq_bulldog.c 96/06/19 1.54
**  $Id: dspp_irq_bulldog.c,v 1.7 1995/03/14 06:40:54 phil Exp phil $
**
**  Support Timer, DMA, and Software interrupts for Bulldog (M2)
**
**  By: Phil Burk
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  950503 WJB  Added header.
**              Added AF_BDA_PASS variants.
**              Much tidying.
**  950511 WJB  Implemented most of dsphHandleSoftInt().
**  950512 WJB  Completed implementation of dsphHandleSoftInt().
**              Added dsphGetReceivedTriggers().
**  950512 WJB  Added dsphEnable/DisableTriggerInterrupts().
**  950512 WJB  Now clearing af_TriggersReceived in dsphDisableTriggerInterrupts().
**  950515 WJB  Fixed a typo in dsphEnable/DisableTriggerInterrupts().
**  950515 WJB  Added EnableDisableDSPPInterrupt() macros.
**              Added extra clearing of af_TriggersReceived and SoftInts in
**              dsphEnable/DisableTriggerInterrupts().
**  950525 WJB  Revised for Arm/DisarmTrigger().
**  950615 PLB  Debugged on real BDA
**  951208 PLB  Made runtime switchable between BDA 1 and 2
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include "audio_folio_modes.h"

#include <dspptouch/dspp_simulator.h>   /* dspsInstallIntHandler() */
#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>
#include <kernel/bitarray.h>            /* FindMSB() */
#include <kernel/interrupts.h>          /* Enable/DisableInterrupt() */

#include "audio_internal.h"


/* -------------------- Debugging */

#define DEBUG_Init      0   /* startup / shutdown */
#define DEBUG_DMA       0   /* DMA servicing */
#define DEBUG_SoftInt   0   /* print status inside dsphHandleSoftInt() */

#define DBUG(x)    /* PRT(x) */

#if DEBUG_Init
#include <stdio.h>
#define DBUGINIT(x) PRT(x)
#else
#define DBUGINIT(x)
#endif

#if DEBUG_DMA
#include <stdio.h>
#define DBUGDMA(x) PRT(x)
#else
#define DBUGDMA(x)
#endif


/* -------------------- Local functions */

static void dsphHandleDMANEXT( void );

static void dsphHandleTimer( void );
static void dsphHandleSoftInt( uint32 SoftIntMask );

	/* convenience macros to cope with interrupt differences between ASIC and simulator */
#ifdef SIMULATE_DSPP
	#define EnableDSPPInterrupt()
	#define DisableDSPPInterrupt()
#else
	#define EnableDSPPInterrupt()  EnableInterrupt(INT_BDA_DSPP)
	#define DisableDSPPInterrupt() DisableInterrupt(INT_BDA_DSPP)
#endif


/* -------------------- Audio FIRQ handler */

static void dsphHandleUnexpected( uint32 IntMask );

/****************************************************************
** dsphFIRQHandler - dispatch DSPP FIRQ to appropriate handler
****************************************************************/
static int32 dsphFIRQHandler (void)
{
	uint32 validInts;

		/* Only process interrupts which are set and enabled. */
	validInts = ReadHardware (DSPX_INTERRUPT_SET) & ReadHardware (DSPX_INTERRUPT_ENABLE);

	DBUG(("dsphFIRQHandler: validInts=0x%x\n", validInts));

	/* Process in order of precedence */

		/* DMA next */
	if (validInts & DSPX_F_INT_DMANEXT) {
		DBUG(("dsphFIRQHandler: DMANext\n"));
		dsphHandleDMANEXT();
		if (!(validInts &= ~DSPX_F_INT_DMANEXT)) goto done;
	}

		/* timer */
	if (validInts & DSPX_F_INT_TIMER) {
		DBUG(("dsphFIRQHandler: timer\n"));
		dsphHandleTimer();
		if (!(validInts &= ~DSPX_F_INT_TIMER)) goto done;
	}

		/* soft int (trigger) */
	if (validInts & DSPX_FLD_INT_SOFT_MASK) {
		DBUG(("dsphFIRQHandler: soft ints 0x%x\n", validInts & DSPX_FLD_INT_SOFT_MASK));
		dsphHandleSoftInt (validInts & DSPX_FLD_INT_SOFT_MASK);
		if (!(validInts &= ~DSPX_FLD_INT_SOFT_MASK)) goto done;
	}

		/* others */
	dsphHandleUnexpected (validInts);

done:
	return 0;
}

/****************************************************************
** dsphHandleUnexpected - Unexpected interrupt from DSPP
****************************************************************/
static void dsphHandleUnexpected( uint32 IntMask )
{
	TOUCH(IntMask);
}

/****************************************************************
** InitDSPPInterrupt - create a single FIRQ for entire DSPP
****************************************************************/
Err InitDSPPInterrupt (void)
{
	Item audioFIRQ;

#ifdef SIMULATE_DSPP
	dspsInstallIntHandler( dsphFIRQHandler );
	audioFIRQ = 0;
#else
		/* Make an interrupt request item. */
	audioFIRQ = CreateFIRQ ("dspp", (uint8)100, dsphFIRQHandler, INT_BDA_DSPP);
#endif

	if (audioFIRQ < 0) {
		ERRDBUG(("InitDSPPInterrupt: failed to create interrupt: 0x%x\n", audioFIRQ));
		return audioFIRQ;
	}
	AB_FIELD(af_AudioFIRQ) = audioFIRQ;
	DBUGINIT(("InitDSPPInterrupt: FIRQ = 0x%x\n", audioFIRQ));

		/* Enable DMANEXT interrupt */
	WriteHardware( DSPX_INTERRUPT_ENABLE, DSPX_F_INT_DMANEXT );

		/* Clear most interrupts. Other interrupts cleared at DSPP Init */
	WriteHardware( DSPX_INTERRUPT_CLR, ~0 );

		/* Enable main DSPP interrupt */
	EnableDSPPInterrupt();

	return 0;
}

/***************************************************************/
void TermDSPPInterrupt (void)
{
	Err errcode;

	DBUGINIT(("TermDSPPInterrupt: firq=0x%x @ 0x%x\n", AB_FIELD(af_AudioFIRQ), LookupItem(AB_FIELD(af_AudioFIRQ))));

		/* delete FIRQ. It's not necessary or wise to disable the interrupt
		** here. Deleting the FIRQ takes care of that as necessary. */
#ifdef SIMULATE_DSPP
	dspsInstallIntHandler( NULL );
#else
	errcode = DeleteItem (AB_FIELD(af_AudioFIRQ));
	DBUGINIT(("TermDSPPInterrupt: FIRQ deletion returned 0x%x\n", errcode));
	TOUCH(errcode);
#endif
}


/* -------------------- DMA Interrupt */

/* =============================================================== */
/* =============== DMA INTERRUPT ================================= */
/* =============================================================== */

/****************************************************************
** dsphHandleDMANEXT - look at second level interrupt register.
** Process each channel.
****************************************************************/
static void dsphHandleDMANEXT (void)
{
	int32   DMAChannel;
	uint32  ValidInts;
	uint32  CompletedMask = 0;         /* set of completed channels */

/* Only process interrupts which are set and enabled. */
	ValidInts = ReadHardware (DSPX_INT_DMANEXT_SET) & ReadHardware (DSPX_INT_DMANEXT_ENABLE);

	DBUGDMA(("\ndsphHandleDMANEXT: ValidInts = 0x%x\n", ValidInts));

/* Check each interrupt channel. */
	while ((DMAChannel = FindMSB(ValidInts)) >= 0)
	{
		AudioDMAControl * const admac = &DSPPData.dspp_DMAControls[DMAChannel];
		vuint32 * const DMAControlRegPtr = DSPX_DMA_CONTROL_CURRENT(DMAChannel);
		const uint32 Mask = 1UL << DMAChannel;

		DBUGDMA(("dsphHandleDMANEXT: service channel %d\n", DMAChannel));

			/* Mark this one done. */
		ValidInts &= ~Mask;

			    /* Set Next registers if sample is queued. */
		DBUGDMA(("dsphHandleDMANEXT: CountDown = %d\n", admac->admac_NextCountDown));
		if ((admac->admac_NextCountDown > 0) && (--(admac->admac_NextCountDown) == 0))
		{
			DBUGDMA(("dsphHandleDMANEXT: Next = 0x%x, 0x%x\n", admac->admac_NextAddress, admac->admac_NextCount ));
			dsphSetNextDMA (DMAChannel, admac->admac_NextAddress, admac->admac_NextCount, DSPH_F_DMA_LOOP);
		}

			    /* Clear after setting Next registers so we pick up correct interrupt. */
		WriteHardware (DSPX_INT_DMANEXT_CLR, Mask);

			    /* Ready to signal? If so, add channel flag to completed set to be posted to daemon. */
		if ((admac->admac_SignalCountDown > 0) && (--(admac->admac_SignalCountDown) == 0))
		{
			CompletedMask |= Mask;
		}

			    /* Are we all done? */
		if ((admac->admac_NextCountDown == 0) && (admac->admac_SignalCountDown == 0))
		{
				    /* Disable DMANext interrupt. */
			WriteHardware (DMAControlRegPtr, DSPX_F_SHADOW_SET_DMANEXT);
		}
	}

		/* Signal audio daemon if something finished. */
	DBUGDMA(("dsphHandleDMANEXT: CompletedMask = 0x%x\n", CompletedMask));
	if (CompletedMask) {
		AB_FIELD(af_ChannelsComplete) |= CompletedMask;
		SuperinternalSignal (AB_FIELD(af_AudioDaemon), AB_FIELD(af_DMASignal));
	}
}

/****************************************************************
** dsphGetCompletedDMAChannels - return Channel completion mask.
** Called by main audiofolio process when DMA signal is received.
****************************************************************/
uint32 dsphGetCompletedDMAChannels (void)
{
	uint32 Channels;
/*
** WARNING! - Don't mess with this routine unless you know EXACTLY what
** you are doing.  This routine access data shared with an interrupt.
** Can you say "race condition"?
*/
/* Disable interrupt while messing with shared data. */
	DisableDSPPInterrupt();

/* Pass completed channel bits to main process. Clear for next time. */
	Channels = AB_FIELD(af_ChannelsComplete);
	AB_FIELD(af_ChannelsComplete) = 0;

	EnableDSPPInterrupt();

	return Channels;
}


/***************************************************************/
/*
	Initializes Audio DMA interrupt handling. Called by
	InitAudioFolio() (which is part of the audio daemon).

	Notes
		. Expects caller to clean up.
		. The signal allocated by this function is never explicitly
		  freed, because task deletion automatically takes care of
		  that.
*/
Err InitAudioDMA (void)
{
	int32 i;

		/* Allocate signal for DMA interrupt to signal audio daemon.
		** @@@ This signal is not explicitly freed, because task deletion
		**     automatically takes care of that. */
	{
		const int32 sig = AllocSignal(0);

		if (sig <= 0) return sig ? sig : AF_ERR_NOSIGNAL;
		AB_FIELD(af_DMASignal) = sig;
	}
	DBUGINIT(("InitAudioDMA: AB_FIELD(af_DMASignal) = 0x%x\n", AB_FIELD(af_DMASignal) ));

		/* Set channel address for fast handling by interrupt. */
	for (i=0; i<DSPI_NUM_DMA_CHANNELS; i++) {
		AudioDMAControl * const admac = &DSPPData.dspp_DMAControls[i];

		admac->admac_ChannelAddr = (((DMARegisters *) DSPX_DMA_STACK) + i);
	}

	return 0;
}


/* -------------------- Timer interrupt */

/* =============================================================== */
/* =============== TIMER INTERRUPT =============================== */
/* =============================================================== */

/***************************************************************/
/* Interrupt routine */
static void dsphHandleTimer(void)
{
DBUG(("dsphHandleTimer() called!\n"));
/* Keep interrupt enabled so that Audio Time will get updated roughly
** every second.  Otherwise we will lose time if the FrameCount wraps
** around and we haven't called swiGetAudioTime() lately.  Clear so
** we don't interrupt immediately.
*/
	WriteHardware( DSPX_INTERRUPT_CLR, DSPX_F_INT_TIMER );

/* Assume that we want to signal folio because interrupt was enabled. */
	SuperinternalSignal (AB_FIELD(af_AudioDaemon), AB_FIELD(af_TimerSignal));
	DBUG(("dsphHandleTimer: SuperinternalSignal(0x%x, 0x%x)\n", AB_FIELD(af_AudioDaemon),AB_FIELD(af_TimerSignal) ));
}


/* -------------------- Trigger Interrupt Control */

/****************************************************************
**
**  Enable/Disable/Clear trigger interrupts.
**
**  Inputs
**
**      TriggerMask - Subset of triggers to enable or disable.
**                    Any bits out of range are ignored.
**
****************************************************************/

void dsphEnableTriggerInterrupts (uint32 TriggerMask)
{
	const uint32 SoftIntMask = TriggerMask << DSPX_FLD_INT_SOFT_SHIFT & DSPX_FLD_INT_SOFT_MASK;

#if DEBUG_SoftInt
	printf ("dsphEnableTriggerInterrupts: triggers=$%08lx softints=$%08lx\n", TriggerMask, SoftIntMask);
#endif

	WriteHardware (DSPX_INTERRUPT_ENABLE, SoftIntMask);
}

void dsphDisableTriggerInterrupts (uint32 TriggerMask)
{
	const uint32 SoftIntMask = TriggerMask << DSPX_FLD_INT_SOFT_SHIFT & DSPX_FLD_INT_SOFT_MASK;

#if DEBUG_SoftInt
	printf ("dsphDisableTriggerInterrupts: triggers=$%08lx softints=$%08lx\n", TriggerMask, SoftIntMask);
#endif

	WriteHardware (DSPX_INTERRUPT_DISABLE, SoftIntMask);
}

void dsphClearTriggerInterrupts (uint32 TriggerMask)
{
	const uint32 SoftIntMask = TriggerMask << DSPX_FLD_INT_SOFT_SHIFT & DSPX_FLD_INT_SOFT_MASK;

#if DEBUG_SoftInt
	printf ("dsphResetTriggerInterrupts: triggers=$%08lx softints=$%08lx\n", TriggerMask, SoftIntMask);
#endif

	WriteHardware (DSPX_INTERRUPT_CLR, SoftIntMask);
}

/****************************************************************
**
**  Read and clear received triggers. Does so by reading
**  and clearing associated soft interrupt flags in the hardware.
**
**  Inputs
**
**      ReadTriggerMask - set of Triggers (not soft ints!) to read.
**                        No triggers outside this set will be returned
**                        or cleared.
**
**  Results
**
**      Subset of ReadTriggerMask that have been received since last
**      call to dsphGetReceivedTriggers().
**
****************************************************************/

uint32 dsphGetReceivedTriggers (uint32 ReadTriggerMask)
{
	const uint32 ReadSoftIntMask = ReadTriggerMask << DSPX_FLD_INT_SOFT_SHIFT & DSPX_FLD_INT_SOFT_MASK;
	uint32 SoftIntMask;

	    /* read and clear requested soft ints */
	SoftIntMask = ReadHardware (DSPX_INTERRUPT_SET) & ReadSoftIntMask;
    WriteHardware (DSPX_INTERRUPT_CLR, SoftIntMask);

	return SoftIntMask >> DSPX_FLD_INT_SOFT_SHIFT;  /* (no need to mask with DSPX_FLD_INT_SOFT_MASK; ReadSoftIntMask already has been) */
}


/* -------------------- SoftInt (Trigger) Interrupt Handler */

/****************************************************************
**
**  dsphHandleSoftInt - Handle set of soft interrupts from DSPP.
**
**  Simply disables received interrupts and signals audio daemon.
**  Audio daemon reads and clears the received interrupts and
**  translates them into triggers (by calling dsphGetReceivedTriggers()).
**  Eventually audio daemon or client can re-enable these interrupts
**  (re-arm the triggers).
**
**  Arguments
**      softIntMask
**          Subset of DSPX_FLD_INT_SOFT_MASK to process. Assumes
**          that no bits outside of DSPX_FLD_INT_SOFT_MASK are set.
**
****************************************************************/

static void dsphHandleSoftInt (uint32 softIntMask)
{
#if DEBUG_SoftInt
	PRT(("dsphHandleSoftInt (0x%08lx)\n", softIntMask));
#endif

	WriteHardware (DSPX_INTERRUPT_DISABLE, softIntMask);
	SuperinternalSignal (AB_FIELD(af_AudioDaemon), AB_FIELD(af_TriggerSignal));
}
