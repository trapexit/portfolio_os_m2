/* @(#) dspp_touch_dma.c 96/07/01 1.5 */
/*******************************************************************
**
** Interface to DSPP DMA for M2
**
** by Phil Burk
**
** Copyright (c) 1994 3DO
**
*******************************************************************/

#include <dspptouch/dspp_instructions.h>
#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

#include "dspptouch_internal.h"

#include <dipir/hw.audio.h>         /* HWResource_Audio */
#include <kernel/sysinfo.h>         /* SYSINFO_TAG_AUDIN */
#include <kernel/super.h>           /* SuperSetSysInfo() */
#include <string.h>                 /* strncmp() */


/* -------------------- Debugging */

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) DBUG(x)
#define NDBUG(x) /* */

/*******************************************************************/
void dsphEnableMaskDMA( uint32 Mask )
{
	WriteHardware(DSPX_CHANNEL_ENABLE, Mask );
}

/*******************************************************************/
void dsphEnableDMA( int32 DMAChan )
{
	dsphEnableMaskDMA( 1L << DMAChan );
}
/*******************************************************************/
uint32 dsphDisableMaskDMA( uint32 Mask )
{
	uint32 dmaState;
	dmaState = ReadHardware(DSPX_CHANNEL_ENABLE) & Mask;
	WriteHardware(DSPX_CHANNEL_DISABLE, Mask );
	return dmaState;
}

/*******************************************************************/
void dsphDisableDMA( int32 DMAChan )
{
	dsphDisableMaskDMA( 1L << DMAChan );
}


/*******************************************************************/
void dsphSetNextDMA (int32 DMAChan, AudioGrain *NextAddr, int32 NextCnt, uint32 Flags)
{
	register vuint32 *DMAPtr;
	uint32 intState;
	uint32 dmaState;
	uint32 channelMask;
	uint32 shadowMask = 0;

DBUGDMA(("dsphSetNextDMA(0x%x, 0x%x, 0x%x)\n", DMAChan, NextAddr,  NextCnt));
#ifdef PARANOID
	if(NextAddr == 0)
	{
		ERR(("dsphSetDMANext: NextAddr = 0\n"));
		return;
	}
	if(NextCnt < 0)
	{

		ERR(("dsphSetDMANext: NextCnt = 0x%x\n", NextCnt));
		return;
	}
#endif

	DMAPtr = DSPX_DMA_STACK + DSPX_DMA_NEXT_ADDRESS_OFFSET + (DMAChan<<2);
	channelMask = 1L<<DMAChan;

/* Are we setting next address and count? */
	if( NextAddr != NULL )
	{
		shadowMask |= DSPX_F_DMA_NEXTVALID | DSPX_F_SHADOW_SET_ADDRESS_COUNT;  /* Set enable bits */
	}

/* DO we loop forever? */
	if (Flags & DSPH_F_DMA_LOOP) shadowMask |= DSPX_F_DMA_GO_FOREVER;

	shadowMask |= DSPX_F_SHADOW_SET_NEXTVALID | DSPX_F_SHADOW_SET_FOREVER;

/* Do we enable interrupt? */
	if (Flags & DSPH_F_DMA_INT_DISABLE) shadowMask |=  DSPX_F_SHADOW_SET_DMANEXT;
	else if (Flags & DSPH_F_DMA_INT_ENABLE) shadowMask |= DSPX_F_INT_DMANEXT_EN | DSPX_F_SHADOW_SET_DMANEXT;

/* -------------------------------------------------------------- INTs OFF  */
/* Disable interrupts and DMA to prevent race conditions. */
	intState = Disable();
	dmaState = dsphDisableMaskDMA( channelMask );

#if 1
/* Clear interrupt bit from previous DMA This seems scary! !!! */
	WriteHardware( DSPX_INT_DMANEXT_CLR, channelMask );
#endif

	WriteHardware(DMAPtr++, (int32) NextAddr);
	WriteHardware(DMAPtr, NextCnt);
	WriteHardware( DSPX_DMA_CONTROL_NEXT(DMAChan), shadowMask );

/* restore DMA if previously enabled */
	dsphEnableMaskDMA( dmaState );
/* Restore Interrupts. */
	Enable(intState);
/* ---------------------------------------------------------------- INTs ON */
}

/*******************************************************************/
/* Set DMA for single transfer, NO NextAddr. */
void dsphSetInitialDMA (int32 DMAChan, AudioGrain *Addr, int32 Cnt )
{
	vuint32 *DMAPtr;
	uint32 Mask;

	DBUGDMA(("dsphSetInitialDMA( 0x%x, 0x%x, 0x%x )\n",
		DMAChan, Addr, Cnt ));

	dsphDisableDMA( DMAChan );
	DMAPtr = DSPX_DMA_STACK + (DMAChan<<2);
	WriteHardware(DMAPtr++, (int32) Addr);
	WriteHardware(DMAPtr, Cnt);

/* Set shadow control register. */
	Mask = DSPX_F_SHADOW_SET_NEXTVALID | DSPX_F_SHADOW_SET_FOREVER | DSPX_F_SHADOW_SET_ADDRESS_COUNT;
	WriteHardware( DSPX_DMA_CONTROL_CURRENT(DMAChan), Mask );
}
