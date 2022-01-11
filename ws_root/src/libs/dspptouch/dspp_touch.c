/* @(#) dspp_touch.c 95/12/12 1.10 */
/* $Id: dspp_touch.c,v 1.54 1995/03/09 08:09:47 phil Exp phil $ */
/*******************************************************************
**
** Interface to DSPP
** This file now contains hardware routines that are source level
** compatible between Anvil and Bulldog.
**
** by Phil Burk
**
** Copyright (c) 1992 New Technologies Group, Inc.
**
*******************************************************************/

/*
** 930111 PLB Set WRDCLK to #$0
** 930428 PLB Added DisableDMAChannel() and check for NextAddr = 0
** 930521 PLB Change Hardware to DAC_ControlValue that is passed in.
** 930713 PLB Initialize EI memory for dipir scaling romtail.dsp
** 931222 PLB DSPP_SetFullDMA()
** 940126 PLB Remove some annoying messages.
** 940321 PLB Fixed pointer arithmetic bug that was masked by compiler bug.
**	          DMAPtr = (vuint32 *) (RAMtoDSPP0 + (DMAChan<<2)); BAD
** 940606 PLB Fixed roundoff error in CvtBytesToFrame() thet was causing the
**            number of bytes and frames to disagree.  501 => 500.
** 940810 PLB Clear interrupt using proper mask so that Output DMA
**            works.  This was causing a premature termination of
**            Output DMA if MonitorAttachment() was called.
** 940901 PLB Removed AODOUT setup code.  Now done in ROM.
** 950131 WJB Cleaned up includes.
** 950509 WJB Made dsphWriteCodeMem() a macro unless PARANOID is defined.
*/

#include <dspptouch/dspp_touch.h>
#include <dspptouch/touch_hardware.h>

#include "dspptouch_internal.h"


/* -------------------- Debugging */

#define DBUG(x) /* PRT(x) */
#define DBUGDMA(x) /* PRT(x) */
#define NDBUG(x) /* */


/* Shared global. */
int32  gIfBDA_1_1 = FALSE;

/* -------------------- Code Mem writer */

#ifdef PARANOID /* { */
/*******************************************************************
** Write to instruction memory to be executed by DSPP
*******************************************************************/
void dsphWriteCodeMem( int32 CodeAddr, int32 Value )
{
/* TRACEB(TRACE_INT, TRACE_OFX, ("dsphWriteCodeMem(CodeAddr = %x, Value = %x)\n", CodeAddr, Value)); */
	if ((CodeAddr < 0) || (CodeAddr > DSPI_CODE_MEMORY_SIZE-1))
	{
		ERR(("dsphWriteCodeMem: Invalid N Address = $%x\n", CodeAddr));
	}
	else
	{
DBUG(("PatchDSPPCode: 0x%x = 0x%x\n", CodeAddr, Value));
		WriteHardware (DSPX_CODE_MEMORY + CodeAddr, Value);
	}
}
#endif /* } PARANOID */

/*****************************************************************/
void dsphDownloadCode(const uint16 *Image, int32 Entry, int32 CodeSize)
{
	int32 i;

	for (i=0; i<CodeSize; i++)
	{
		dsphWriteCodeMem( Entry + i, (uint32) Image[i] );
	}
}


/* -------------------- DMA / FIFO stuff */

/*******************************************************************
** Read address of DMA channel.  Used by WhereAttachment()
*******************************************************************/
int8  *dsphReadChannelAddress( int32 Channel )
{
	vuint32	*DMAPtr;

	DMAPtr = DSPX_DMA_STACK + (Channel<<2);
	return (int8 *) ReadHardware( DMAPtr );
}
