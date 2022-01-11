/******************************************************************************
**
**  @(#) beep_interrupts.c 96/08/05 1.7
**
**  DMANext interrupts for Beep Folio
**
**  By: Phil Burk
**
**  Copyright (c) 1996, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/


#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>
#include <kernel/interrupts.h>          /* Enable/DisableInterrupt() */
#include <kernel/super.h>               /* SuperInternalSignal() */
#include "beep_internal.h"


/* -------------------- Debugging */

#define DBUG(x)  /* PRT(x) */


/* -------------------- Local functions */

static void beepHandleDMANEXT( void );
static void beepHandleUnexpected( uint32 IntMask );

#define EnableDSPPInterrupt()  EnableInterrupt(INT_BDA_DSPP)
#define DisableDSPPInterrupt() DisableInterrupt(INT_BDA_DSPP)

/****************************************************************
** dsphFIRQHandler - dispatch DSPP FIRQ to appropriate handler
****************************************************************/
static int32 beepFIRQHandler( void )
{
	uint32  ValidInts;

DBUG(("beepFIRQHandler() !! \n"));
/* Only process interrupts which are set and enabled. */
	ValidInts = ReadHardware( DSPX_INTERRUPT_SET ) & ReadHardware( DSPX_INTERRUPT_ENABLE );

/* Process in order of likely occurence. */
	if( ValidInts & DSPX_F_INT_DMANEXT )
	{
DBUG(("beepFIRQHandler: DMANext \n"));
		beepHandleDMANEXT();
		if( (ValidInts &= ~DSPX_F_INT_DMANEXT) == 0 ) goto done;
	}
	
	beepHandleUnexpected( ValidInts );

done:
	return 0;
}

/****************************************************************
** beepHandleUnexpected - Unexpected interrupt from DSPP
****************************************************************/
static void beepHandleUnexpected( uint32 IntMask )
{
	ERR(("beepHandleUnexpected: IntMask = 0x%x\n", IntMask ));
	ERR(("     Set     = 0x%x\n", ReadHardware( DSPX_INTERRUPT_SET ) ));
	ERR(("     Enabled = 0x%x\n", ReadHardware( DSPX_INTERRUPT_ENABLE ) ));
#ifndef BUILD_STRINGS
	TOUCH(IntMask);
#endif
}

/****************************************************************
** beepInitInterrupt - create a single FIRQ for entire DSPP
****************************************************************/
int32 beepInitInterrupt( void )
{
	Item BeepFIRQ;

	DBUG(("beepInitInterrupt()\n"));
/* Make an interrupt request item. */
	BeepFIRQ = CreateFIRQ("beep", (uint8) 100, beepFIRQHandler, INT_BDA_DSPP);
	DBUG(("beepInitInterrupt: FIRQ = 0x%x\n", BeepFIRQ ));
	if (BeepFIRQ < 0)
	{
		ERR(("beepInitInterrupt failed to create interrupt: 0x%x\n",
			BeepFIRQ));
		return BeepFIRQ;
	}
	BB_FIELD(bf_FIRQ) = BeepFIRQ;

/* Clear most interrupts. Other interrupts cleared at DSPP Init */
	WriteHardware( DSPX_INTERRUPT_CLR, -1 );
	WriteHardware( DSPX_INTERRUPT_DISABLE, -1 );
	WriteHardware( DSPX_INTERRUPT_ENABLE, DSPX_F_INT_DMANEXT );

	EnableDSPPInterrupt();

	return BeepFIRQ;
}

/***************************************************************/
int32 beepTermInterrupt( void )
{
	int32 Result = 0;

	if( BB_FIELD(bf_FIRQ) )
	{
		if( (Result = DeleteItem( BB_FIELD(bf_FIRQ) )) < 0)
		{
			ERR(("beepTermInterrupt: delete item failed  0x%x\n", Result));
			return Result;
		}
		BB_FIELD(bf_FIRQ) = 0;
	}
	return Result;
}


/* =============================================================== */
/* =============== DMA INTERRUPT ================================= */
/* =============================================================== */

/****************************************************************
** beepHandleDMANEXT - look at second level interrupt register.
** Process each channel.
****************************************************************/
static void beepHandleDMANEXT( void )
{
	int32   DMAChannel;
	uint32  ValidInts;
	vuint32 *DMAControlRegPtr;

/* Only process interrupts which are set and enabled. */
	ValidInts = ReadHardware( DSPX_INT_DMANEXT_SET ) & ReadHardware( DSPX_INT_DMANEXT_ENABLE );
	DBUG(("beepHandleDMANEXT: ValidInts = 0x%x\n", ValidInts ));

/* Check each interrupt channel. */
	DMAChannel = 0;
	while( ValidInts != 0 )
	{
		uint32  Mask;
		Mask = 1L << DMAChannel;
		if( ValidInts & Mask )   /* Check channel bit. */
		{
			Item    taskItem;
			ValidInts &= ~Mask;  /* Mark this one done. */
			DMAControlRegPtr = DSPX_DMA_CONTROL_CURRENT(DMAChannel);

/* Clear after setting Next registers so we pick up correct interrupt. */
			WriteHardware( DSPX_INT_DMANEXT_CLR, Mask );

/* Disable DMANext interrupt. */
			WriteHardware( DMAControlRegPtr, DSPX_F_SHADOW_SET_DMANEXT );

/* Signal user process if requested. */
			taskItem = BB_FIELD(bf_TasksToSignal)[DMAChannel];
			if( taskItem )
			{
				Task   *task;
				task = LookupItem( taskItem );  /* Still valid? */
				if( task != NULL )
				{
					DBUG(("beepHandleDMANEXT: Task = 0x%08x, Signal = 0x%08x\n", task,
						BB_FIELD(bf_Signals)[DMAChannel] ));
					SuperInternalSignal(task, BB_FIELD(bf_Signals)[DMAChannel]);
					
				}
/* Clear so that we only send one signal per request. */
				BB_FIELD(bf_Signals)[DMAChannel] = 0;
				BB_FIELD(bf_TasksToSignal)[DMAChannel] = 0;
				
			}
		}
		DMAChannel++;
	}
}
