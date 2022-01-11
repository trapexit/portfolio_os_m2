/*
 *	@(#) lccd_cde.c 96/06/27 1.28
 *	Copyright 1994, The 3DO Company
 *
 * System dependent portion of lccddev (using CDE, BOB, powerbus)
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/cde.h"
#include "dipir.h"
#include "notsysrom.h"
#include "lccddev.h"

#define USE_MULT_DMA_CHANNELS 

#if 0 /*def DEBUG*/
#define DEBUG_DETAIL
#define DEBUG_CMDS
#define DEBUG_MAJOR
#define DEBUG_Q_REPORTS
#define DEBUG_IO
/*#define DEBUG_IO_DETAILS*/
#endif


/*****************************************************************************
  Defines
*/

#define	MILLISEC		1
#define BAB_STAT_FIFO_EMPTY	0x180


/*****************************************************************************
  Macros
*/


/*****************************************************************************
  Types
*/


/*****************************************************************************
  External data
*/

extern const DipirRoutines *dipr;


/*****************************************************************************
  Local data
*/

static uint32 cdeBase;
static uint32 intEnableSave;
static uint32 statPktCnt;
static uint32 statPktIndex;
static uchar  statPktData[12];


/*****************************************************************************
  Code follows...
*****************************************************************************/


/*****************************************************************************
  Send a command word to the lccd drive using CDE.
  According to hw guys, the time needed to spew out a command byte is
  on the order of 1.5 uSecs (so a timeout of 10 mSec is probably okay).
*/
int32 
SendACommandWord(uint32 cmd, uint32 timeout)
{
	TimerState tm;

	/* Send the command to the low cost cd */
	PBUSDEV_WRITE(cdeBase, CDE_CD_CMD_WRT, cmd);

	/* Wait for the command byte to be sent out */
	ResetTimer(&tm);
	while ((PBUSDEV_READ(cdeBase, CDE_INT_STS) &
		CDE_CD_CMD_WRT_DONE) == 0) {
		if (timeout != NOTIMEOUT && ReadMilliTimer(&tm) > timeout) {
			/*** PRINTF(("LCCD Send cmd: timed out\n")); ***/
			return -1;
		}
	}

	/* Cmd byte sent: clear the interrupt. */
	PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_CD_CMD_WRT_DONE);

	return 0;
}

/*****************************************************************************
  Send a command to the lccd drive using CDE.
*/
int32 
SendCommand(uint8 *cmd, int32 cmdLen, uint32 timeout)
{
	int32 err = 0;

	for (; cmdLen > 0; cmdLen--, cmd++) {
		err = SendACommandWord((uint32)*cmd, timeout);
		if (err) break;
	}
	return err;
}

/*****************************************************************************
  If a status byte is present, get it and return true.
  If a status byte is not present, set stat fifo empty and return false.
*/
int32 
CheckStatusByte(uchar *stat, uint32 timeout, Boolean anyBytes)
{
	uint32 intr;
	uint32 stsInfo;
	uint32 donebits;
	TimerState tm;

	donebits = CDE_CD_STS_FL_DONE;
	if (anyBytes)
		donebits |= CDE_CD_STS_RD_DONE;

	if (statPktIndex >= statPktCnt) 
	{
		/* Reset the values */
		statPktCnt = 0;
		statPktIndex = 0;

		/* Wait for a status packet */
		ResetTimer(&tm);
		for (;;)
		{
			intr = PBUSDEV_READ(cdeBase, CDE_INT_STS);
			if (intr & donebits)
				break;
			if (timeout != NOTIMEOUT && 
			    ReadMilliTimer(&tm) > timeout)
			{
				PRINTF(("Status fifo timeout %d, intr %x\n", 
					timeout, intr));
				return FALSE;
			}
		}

		/* Suck in the bytes */
		while (TRUE) 
		{
			vuint32 dufus;

			stsInfo = PBUSDEV_READ(cdeBase, CDE_CD_STS_RD);
			dufus = PBUSDEV_READ(cdeBase, CDE_INT_STS);
			if (!(stsInfo & CDE_CD_STS_READY))
				break;
			if (statPktCnt > sizeof(statPktData))
			{
				PRINTF(("LCCD Recv sts: overflow\n"));
				break;
			}
			statPktData[statPktCnt++] = (uchar)stsInfo;
		}

		/* Status packet received: clear the interrupt. */
		PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_CD_STS_RD_DONE);
		PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_CD_STS_FL_DONE);
		/* Let Babette know that it can send more status */
		SendACommandWord(BAB_STAT_FIFO_EMPTY, NOTIMEOUT);
	}

	if (statPktIndex >= statPktCnt)
		return FALSE;
	*stat = statPktData[statPktIndex++];
	return TRUE;
}

/*****************************************************************************
*/
int32 
RecvdStatusByte(uchar *stat, uint32 timeout)
{
	return CheckStatusByte(stat, timeout, FALSE);
}

/*****************************************************************************
  Clear out any junk in the cde status FIFO.
*/
void 
FlushStatusFifo(void)
{
	uchar junk;

	PRINTF(("Flush status fifo\n"));
	while (CheckStatusByte(&junk, 100*MILLISEC, TRUE))
	{
		PRINTF(("Discard junk status byte %x\n", junk));
	}
	SendACommandWord(BAB_STAT_FIFO_EMPTY, 100*MILLISEC);
}

/*****************************************************************************
  Reset the DMA engine and sets things to a known state.
  This is called when lccd is entered and after detecting CD DMA errors.
*/
void 
ResetDMA(uint8 channels)
{
	/* Global = 0, PowerBus Channel = 0. */
#ifndef USE_MULT_DMA_CHANNELS
	TOUCH(channels);
#else
	if (channels & DMA_CH1) {
		PBUSDEV_SET(cdeBase, CDE_CD_DMA2_CNTL, CDE_DMA_RESET);
		PBUSDEV_CLR(cdeBase, CDE_CD_DMA2_CNTL, 0xFFFFFFFF);
	}
	if (channels & DMA_CH0)
#endif
	{
		PBUSDEV_SET(cdeBase, CDE_CD_DMA1_CNTL, CDE_DMA_RESET);
		PBUSDEV_CLR(cdeBase, CDE_CD_DMA1_CNTL, 0xFFFFFFFF);
	}
}

/*****************************************************************************
  Set up DMA memory address & length (Current).
*/
void 
ConfigDMA(uint8 channel, uint8 *dst, int32 len)
{
#ifdef DEBUG_IO_DETAILS
	PRINTF(("ConfigDMA ch %x: ptr %x, len %x\n", channel, dst, len));
#endif
	ResetDMA(channel);
#ifdef USE_MULT_DMA_CHANNELS
	if (channel == DMA_CH1)
	{
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA2_CPAD, (uint32)dst);
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA2_CCNT, len);
		PBUSDEV_SET(cdeBase, CDE_CD_DMA2_CNTL, CDE_DMA_GO_FOREVER);
	} else
#endif
	{
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA1_CPAD, (uint32)dst);
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA1_CCNT, len);
		PBUSDEV_SET(cdeBase, CDE_CD_DMA1_CNTL, CDE_DMA_GO_FOREVER);
	}
}

/*****************************************************************************
  Set up DMA memory address & length (Next).
*/
void 
ConfigNextDMA(uint8 channel, uint8 *dst, int32 len)
{
#ifdef DEBUG_IO_DETAILS
	PRINTF(("ConfigNextDMA ch %x, ptr %x, len %x\n", channel, dst, len));
#endif
#ifndef USE_MULT_DMA_CHANNELS
	TOUCH(channel);
#else
	if (channel == DMA_CH1)	{
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA2_NPAD, (uint32)dst);
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA2_NCNT, len);
		PBUSDEV_SET(cdeBase, CDE_CD_DMA2_CNTL, CDE_DMA_NEXT_VALID);
	} else
#endif
	{
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA1_NPAD, (uint32)dst);
		PBUSDEV_WRITE(cdeBase, CDE_CD_DMA1_NCNT, len);
		PBUSDEV_SET(cdeBase, CDE_CD_DMA1_CNTL, CDE_DMA_NEXT_VALID);
	}
}

/*****************************************************************************
  Enable DMA from the disc.
*/
void 
EnableDMA(uint8 channels)
{
#ifndef USE_MULT_DMA_CHANNELS
	TOUCH(channels);
#else
	if (channels & DMA_CH1)	
	{
		/* Clear the interrupt. */
		PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_CD_DMA2_DONE);

		/* Set CV */
		PBUSDEV_SET(cdeBase, CDE_CD_DMA2_CNTL, CDE_DMA_CURR_VALID);
	}
	if (channels & DMA_CH0)
#endif
	{
		/* Clear the interrupt. */
		PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_CD_DMA1_DONE);

		/* Set CV */
		PBUSDEV_SET(cdeBase, CDE_CD_DMA1_CNTL, CDE_DMA_CURR_VALID);
	}
}

/*****************************************************************************
  Disable DMA from the disc.
*/
void 
DisableDMA(uint8 channels)
{
#ifndef USE_MULT_DMA_CHANNELS
	TOUCH(channels);
#else
	if (channels & DMA_CH1)	{
		/* Clear CV */
		PBUSDEV_CLR(cdeBase, CDE_CD_DMA2_CNTL, CDE_DMA_CURR_VALID);
	}
	if (channels & DMA_CH0)
#endif
	{
		/* Clear CV */
		PBUSDEV_CLR(cdeBase, CDE_CD_DMA1_CNTL, CDE_DMA_CURR_VALID);
	}
}

/*****************************************************************************
  Poll to see if DMA has completed.
  Note: unlike many of the routines above, this only works for one DMA
  channel or the other, NOT both.
*/
Boolean
DMADone(uint8 channel)
{
	uint32 intrBits;
	uint32 intrTest;

#ifndef USE_MULT_DMA_CHANNELS
	TOUCH(channel);
#else
	if (channel == DMA_CH1)	{
		intrTest = CDE_CD_DMA2_DONE;
	} else
#endif
	{
		intrTest = CDE_CD_DMA1_DONE;
	}

	/* See if we've gotten an interrupt for DMA completion. */
	/* Dipir has interrupts disabled, so we must poll the intr bit. */
	intrBits = PBUSDEV_READ(cdeBase, CDE_INT_STS);
	if (!(intrBits & intrTest))
	{
		/* DMA not done yet */
		return FALSE;
	}

	/* DMA is done: clear the interrupt. */
	PBUSDEV_CLR(cdeBase, CDE_INT_STS, 
		intrTest | CDE_CD_DMA1_OF | CDE_CD_DMA2_OF);
	return TRUE;
}

/*****************************************************************************
  Do any initialization needed for the system dependent routines.
*/
void 
LCCDSysDepInit(void)
{
	int32 cdeSlot;
#ifdef DEBUG
	uint32 regValue;
#endif

	/* Initialize the cde chip base address */
	cdeSlot = PowerSlot(M2_DEVICE_ID_CDE);
	cdeBase = PBDevsBaseAddr(cdeSlot);

	/* Clear all bits in int enable register so I don't have  */
	/* to keep clearing int_sent after each int/state change. */
	intEnableSave = PBUSDEV_READ(cdeBase, CDE_INT_ENABLE);
	PBUSDEV_WRITE(cdeBase, CDE_INT_ENABLE, 0);

	/* Clear dipir in the int stat register */
	PBUSDEV_CLR(cdeBase, CDE_INT_STS, (CDE_INT_SENT | CDE_DIPIR));

#ifdef DEBUG
	regValue = PBUSDEV_READ(cdeBase, CDE_INT_STS);
	if (regValue & CDE_DIPIR) {
		PRINTF(("LCCD could not clear CDE INT_STS DIPIR bit\n"));
	}
	regValue = PBUSDEV_READ(cdeBase, CDE_BBLOCK);
	if (regValue & (CDE_CDROM_DIPIR | CDE_CDROM_BLOCK)) {
		PRINTF(("LCCD called with CDE DIPIR/BLOCK CD_ROM set\n"));
	}
#endif

	/* Init the DMA channels to a known state */
	ResetDMA(DMA_CH0|DMA_CH1);

	/* No status packets received yet */
	statPktCnt = 0;
	statPktIndex = 0;
	FlushStatusFifo();
}

/*****************************************************************************
  Do any system dependent related cleanup before exiting device driver.
*/
void 
LCCDSysDepExit(void)
{
	FlushStatusFifo();
	ResetDMA(DMA_CH0|DMA_CH1);
	PBUSDEV_CLR(cdeBase, CDE_INT_STS, CDE_INT_SENT | CDE_DMA1_DONE | CDE_DMA2_DONE);
	PBUSDEV_WRITE(cdeBase, CDE_INT_ENABLE, intEnableSave);
}
