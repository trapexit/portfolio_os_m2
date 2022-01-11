/*
 * @(#) dev.lccd.c 96/06/27 1.60
 * Copyright 1994,1995 by The 3DO Company Inc.
 *
 * Dipir device driver for the Low-Cost CD-ROM drive.
 */

#include "kernel/types.h"
#include "setjmp.h"
#include "dipir.h"
#include "notsysrom.h"
#include "lccddev.h"
#include "ddd.cd.h"
#include "hardware/cde.h"

#if 0 /*def DEBUG*/
#define DEBUG_DETAIL
#define DEBUG_CMDS
#define DEBUG_STATUS
#define DEBUG_MAJOR
#define DEBUG_Q_REPORTS
#define DEBUG_IO
#define DEBUG_IO_DETAILS
#endif

#define	MILLISEC		1
#define	OK			0
#define	TAG_ERROR		0x80

#define	READ_RETRIES		12
#define	TOC_RETRIES		1

#define BCD2BIN(bcd)		((((bcd) >> 4) * 10) + ((bcd) & 0x0F))
#define BIN2BCD(bin)		((((bin) / 10) << 4) | ((bin) % 10))
#define	SwapNibbles(x)		((((x) & 0xF) << 4) | (((x) >> 4) & 0xF))

/* Command packets */
static const uchar ReadIDCmd[] =
	{ CMD_READ_ID, 0, 0, 0, 0, 0, 0 };
static const uchar PlayCmd[] =
	{ CMD_PPSO, CMD_PPSO_PLAY };
static const uchar PauseCmd[] =
	{ CMD_PPSO, CMD_PPSO_PAUSE };
static const uchar EjectCmd[] =
	{ CMD_PPSO, CMD_PPSO_OPEN };
static const uchar AudioFormatCmd[] =
	{ CMD_SECTOR_FORMAT, FMT_AUDIO, 0 };
static const uchar DataFormatCmd[] =
	{ CMD_SECTOR_FORMAT, FMT_MODE1_AUXCH0 | FMT_HEADER_CH0 | FMT_COMP_CH0, 0 };
static const uchar MechTypeCmd[] =
	{ CMD_MECH_TYPE, 0, 0, 0 };

const DipirRoutines *dipr;	/* Shared with lccd_cde.c */

static uint32 DefaultSpeed;
static uint32 MaxSpeed;


/***************************************************************************
 Cache of TOC-type stuff.
 Upper levels of dipir read this stuff four or five times, so we cache it.
*/
static Boolean gotCachedInfo;
static TOCInfo cachedTOC1;
static DiscInfo cachedDiscInfo;
static Boolean cachedMultiSession;

static int32 CorrectedECC;

/***************************************************************************
 Cache of buffers which hold pre-fetched data from the disc.
*/
typedef struct DiscBuffer {
	struct DiscBuffer *	Next;		/* Next buffer */
	uint32			BlockNumber;	/* Block in this buffer */
	uint8			Intransit;	/* Buffer is being read into */
	uint8			unused[3];	
	uint32			pad;		/* Align DiscBlock */
	struct DiscBlock	DiscBlock;	/* Data from the disc */
} DiscBuffer;

static struct DiscBuffer *FirstBuffer;
static struct DiscBuffer *IntransitBuffer = NULL;
static uint32 ReqBlock;
static void *ReqBuffer;

/* There are exactly two disc buffers that will be 32-byte aligned */
static uint8 DiscBuffers[64 + (2 * sizeof(DiscBuffer))];

/* External routines in the system dependent lccd modules */
void LCCDSysDepInit(void);
void LCCDSysDepExit(void);
int32 SendCommand(uchar *cmd, int32 cmdLen, uint32 timeout);
int32 RecvdStatusByte(uchar *stat, uint32 timeout);
void FlushStatusFifo(void);
void ConfigDMA(uint8 channel, uint8 *dst, int32 len);
void ConfigNextDMA(uint8 channel, uint8 *dst, int32 len);
void EnableDMA(uint8 channels);
void DisableDMA(uint8 channels);
Boolean DMADone(uint8 channel);


/***************************************************************************
 One-time setup of the buffers.
*/
static void
InitBuffers(void)
{
	struct DiscBuffer *SecondBuffer;

	FirstBuffer = (struct DiscBuffer *) ((((uint32)&DiscBuffers[0]) + 32) & -32);
	SecondBuffer = (struct DiscBuffer *) ((((uint32)(FirstBuffer + 1)) + 32) & -32);
	FirstBuffer->Next = SecondBuffer;
	SecondBuffer->Next = FirstBuffer;
	gotCachedInfo = FALSE;
}

/***************************************************************************
 Set all disc buffers to the "empty" state.
*/
static void
EmptyBuffers(void)
{
	struct DiscBuffer *bp;

	bp = FirstBuffer;
	do {
		bp->BlockNumber = 0;
		bp->Intransit = FALSE;
	} while ((bp = bp->Next) != FirstBuffer);
}

/***************************************************************************
 Find the buffer which holds a specific disc block.
*/
static struct DiscBuffer *
FindBuffer(uint32 block)
{
	struct DiscBuffer *bp;

	bp = FirstBuffer;  
	do {
		if (bp->BlockNumber == block)
			return bp;
	} while ((bp = bp->Next) != FirstBuffer);
	return NULL;
}

/***************************************************************************
 Find the latest (largest) block number currently in a buffer.
*/
static uint32
LatestBlock(void)
{
	struct DiscBuffer *bp;
	uint32 latest = 0;

	bp = FirstBuffer;  
	do {
		if (bp->BlockNumber > latest)
			latest = bp->BlockNumber;
	} while ((bp = bp->Next) != FirstBuffer);
	return latest;
}

/***************************************************************************
 Return the size of a specified report type.
*/
static int
ReportSize(uchar tagByte)
{
	switch (tagByte &~ TAG_ERROR)
	{
	case CMD_READ_ID	&~ TAG_ERROR:	return READ_ID_REPORT_SIZE;
	case CMD_READ_ERROR	&~ TAG_ERROR:	return READ_ERROR_REPORT_SIZE;
	case CMD_CHECK_WO	&~ TAG_ERROR:	return CHECK_WO_REPORT_SIZE;
	case CMD_READ_FIRMWARE	&~ TAG_ERROR:	return READ_FIRMWARE_RESP_SIZE;
	case CMD_MECH_TYPE	&~ TAG_ERROR:	return MECH_TYPE_RESP_SIZE;
	case CMD_COPY_PROT_THRESH &~ TAG_ERROR:	return COPY_PROT_THRESH_RESP_SIZE;
	case CMD_GET_WDATA	&~ TAG_ERROR:	return GET_WDATA_RESP_SIZE;
	case REPORT_DRIVE_STATE	&~ TAG_ERROR:	return DRIVE_STATE_REPORT_SIZE;
	case REPORT_QCODE	&~ TAG_ERROR:	return QCODE_REPORT_SIZE;
	case REPORT_SWITCH	&~ TAG_ERROR:	return SWITCH_REPORT_SIZE;
	}
	/* Anything else is just a standard one-byte command response. */
	return 1;
}

/***************************************************************************
 Read a response packet from the disc drive.
 We look for a packet of a specified type, discarding any we may get
 which are not of that type.
*/
static int
GetResponse(uchar *resp, int respLen, uchar tagByte, uint32 timeout)
{
	int expectLen = 0;
	int gotLen;
	int err = OK;
	uint32 tm;
	uchar stat;
	Boolean skip;

	do { /* Keep getting packets till we get the right type (tagByte). */
		skip = FALSE;
		gotLen = 0;
		do { /* Keep getting bytes till we get a full packet. */
			if (timeout == NOTIMEOUT)
			{
				if (!RecvdStatusByte(&stat, NOTIMEOUT))
					return -1;
			} else
			{
				for (tm = timeout; ; tm--)
				{
					/* Wait for a byte. */
					if (tm == 0)
						return -1;
					if (RecvdStatusByte(&stat, 1*MILLISEC))
						break;
				}
			}
			if (gotLen == 0)
			{
				/* "stat" is the first byte, the tag byte.
				 * Figure out how long the packet will be. */
				expectLen = ReportSize(stat);
#ifdef DEBUG_STATUS
				PRINTF(("GetResponse: tag %x: expect %d bytes: ",
					stat, expectLen)); 
#endif
				if ((stat &~ TAG_ERROR) != (tagByte &~ TAG_ERROR))
				{
					/* Not the response we want.
					 * Discard it and keep looking. */
					skip = TRUE;
#ifdef DEBUG_STATUS
						PRINTF(("[UNEXPECTED] "));
#endif
				} else if (expectLen > respLen)
				{
					/* Should not happen: caller's
					 * buffer is not big enough. */
					PRINTF(("***ERROR: expectLen > respLen %x\n", respLen));
					skip = TRUE;
					err = -1;
				}
			}
#ifdef DEBUG_STATUS
			PRINTF(("%x ", stat));
#endif
			if (!skip)
				resp[gotLen] = stat;
			gotLen++;
		} while (gotLen < expectLen);
#ifdef DEBUG_STATUS
		PRINTF(("\n"));
#endif
	} while (skip && err == OK);
	return err;
}

/***************************************************************************
 Send a command to the drive and get a response packet.
*/
static int
SendDiscCmdWithResponse(uchar *cmd, int cmdLen, uchar *resp, int respLen, uint32 timeout)
{
	int err;

#ifdef DEBUG_CMDS
	{
		int i;
		PRINTF(("SendDiscCmd: ")); 
		for (i=0; i<cmdLen; i++) {
			PRINTF(("%x ", cmd[i]));
		}
		PRINTF(("\n"));
	}
#endif
	err = SendCommand(cmd, cmdLen, timeout);
	if (err < 0)
		return err;
	/* Response should have same type (tag byte) as the command we sent. */
	return GetResponse(resp, respLen, cmd[0], timeout);
}

/***************************************************************************
 Send a command to the drive and get a standard (one byte) response.
*/
static int
SendDiscCmd(uchar *cmd, int cmdLen)
{
	uchar resp[1];
	int err;

	err = SendDiscCmdWithResponse(cmd, cmdLen, resp, sizeof(resp), NOTIMEOUT);
	if (err < OK) return err;
	return OK;
}

/***************************************************************************
 Ask for a single report, right now.
*/
static int
QueryReport(uchar *report, int reportLen, uchar reportType)
{
	int err;
	uchar GetReportCmd[2];

#ifdef DEBUG_DETAIL
	PRINTF(("QueryReport: type %x\n", reportType));
#endif
	/* Request one immediate report. */
	GetReportCmd[0] = reportType &~ REPORT_TAG;
	GetReportCmd[1] = REPORT_NOW;
	err = SendDiscCmd(GetReportCmd, sizeof(GetReportCmd));
	if (err < OK) return err;
	/* Get the report. */
	return GetResponse(report, reportLen, reportType, NOTIMEOUT);
}

/***************************************************************************
 Get the current state of the drive (PLAY, PAUSE, STOP, OPEN, etc.)
*/
static int
GetDriveState(void)
{
	int err;
	uchar report[DRIVE_STATE_REPORT_SIZE];

	err = QueryReport(report, sizeof(report), REPORT_DRIVE_STATE);
	if (err < OK) return err;
#ifdef DEBUG_DETAIL
	PRINTF(("GetDriveState: state %x\n", report[1]));
#endif
	return report[1];
}

#if 1
/***************************************************************************
 Get the current state of the door switch.
*/
static int
GetSwitchState(void)
{
	int err;
	uchar report[SWITCH_REPORT_SIZE];

	err = QueryReport(report, sizeof(report), REPORT_SWITCH);
	if (err < OK) return err;
#ifdef DEBUG_DETAIL
	PRINTF(("GetSwitchState: state %x\n", report[1]));
#endif
	return report[1];
}
#endif

/***************************************************************************
 Enable/disable a specific type of report from the drive.
*/
static int
EnableReports(uchar reportType, int what)
{
	uchar EnableReportCmd[2];

#ifdef DEBUG_DETAIL
	PRINTF(("EnableReports(%x,%x)\n", reportType, what));
#endif

	/* Enable/disable this type of report. */
	EnableReportCmd[0] = reportType &~ REPORT_TAG;
	EnableReportCmd[1] = what;
	return SendDiscCmd(EnableReportCmd, sizeof(EnableReportCmd));
}

/***************************************************************************
 Wait for the drive to enter a specified state.
*/
static int
WaitDriveState(int needState)
{
	int state;

	/* Wait for the drive to enter the specified state. */
	for (;;)
	{
		state = GetDriveState();
		if (state < OK) return state;
		if (state == needState)
			break;
		if (DRV_ERROR_STATE(state))
			return -1;
		if (DRV_OPEN_STATE(state) && needState != DRV_OPEN)
			return -1;
		Delay(100*MILLISEC);
	}
	return OK;
}

/***************************************************************************
 Seek to a specified block number on the disc.
*/
static int
SeekDisc(int32 block)
{
	uchar SeekCmd[4];

	/* Issue the seek command to the drive. */
	SeekCmd[0] = CMD_SEEK;
	SeekCmd[1] = (uchar) (block >> 16);
	SeekCmd[2] = (uchar) (block >> 8);
	SeekCmd[3] = (uchar) (block);
	return SendDiscCmd(SeekCmd, sizeof(SeekCmd));
}

/***************************************************************************
*/
static int
SendSpeedCmd(uint32 speed)
{
	uchar SpeedCmd[3];

	SpeedCmd[0] = CMD_SET_SPEED;
	SpeedCmd[1] = speed;
	SpeedCmd[2] = 0;
	return SendDiscCmd(SpeedCmd, sizeof(SpeedCmd));
}

/***************************************************************************
 Eject the disc.
*/
static void
EjectDisc(void)
{
	int err;

#ifdef DEBUG_MAJOR
	PRINTF(("EjectDisc:\n"));
#endif
	/* Send the OPEN-DRAWER command to the drive. */
	for (;;)
	{
		err = SendDiscCmd(EjectCmd, sizeof(EjectCmd));
		if (err >= OK) break;
		PRINTF(("EjectDisc: error in SendDiscCmd\n"));
		Delay(100*MILLISEC);
	}
	/*
	 * Don't wait for drive drawer to open. 
	 * Firmware guarantees drive is unusable after eject until it is reset.
	 * Waiting for door open can hang if we never reach the OPEN state.
	 */
}

#define	DRIVE_OPEN()	DriveOpen()
/***************************************************************************
 Check to see if the drive is open (or opening).
*/
static Boolean
DriveOpen(void)
{
	int state;

	state = GetDriveState();
	if (DRV_ERROR_STATE(state))
	{
		/* If the drive is in some kind of error state,
		 * just say that it is OPEN. */
		PRINTF(("DriveOpen: error state %x\n", state));
		return TRUE;
	}
	return DRV_OPEN_STATE(state);
}

/***************************************************************************
 Seek to a specified block number and start playing.
 Any data currently in the DiscBuffers is discarded.
*/
static int
StartPlaying(uint32 block)
{
	int err;
	int state;
	struct DiscBuffer *bp;
	uchar report[DRIVE_STATE_REPORT_SIZE];

	DisableDMA(DMA_CH0|DMA_CH1);

	/* Enable CIP reports and discard the first one. */
	EnableReports(REPORT_DRIVE_STATE, REPORT_ENABLE_DETAIL);
	err = GetResponse(report, sizeof(report), REPORT_DRIVE_STATE, NOTIMEOUT);
	if (err < OK)
		goto Error;
	state = report[1];
	if (DRV_ERROR_STATE(state) || DRV_OPEN_STATE(state))
		goto Error;
	/* Seek to the block. */
	err = SeekDisc((int32)block);
	if (err < OK) return err;
	/* Wait for drive to enter the PAUSE state.
	 * It is not safe to enable DMA before the drive is in PAUSE,
	 * because garbage gets spit out on the DMA channels when 
	 * the drive enters PAUSE. */
	do {
		err = GetResponse(report, sizeof(report), 
				REPORT_DRIVE_STATE, NOTIMEOUT);
		if (err < OK)
			goto Error;
		state = report[1];
#ifdef DEBUG_IO
		if (state != DRV_PAUSE) { 
			PRINTF(("CIP=%x\n", state));
		}
#endif
		if (DRV_ERROR_STATE(state))
			goto Error;
		if (DRV_OPEN_STATE(state))
			goto Error;
	} while (state != DRV_PAUSE);
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);

	/* Set up buffers and DMA. */
	EmptyBuffers();
	IntransitBuffer = FirstBuffer;
	IntransitBuffer->BlockNumber = block;
	IntransitBuffer->Intransit = TRUE;
	ConfigDMA(DMA_CH0, 
		(uint8*)&IntransitBuffer->DiscBlock, sizeof(struct DiscBlock));
	/* Set up "next" DMA pointers so data for the next block 
	   (after the current one) has someplace to go. */
	/* Note a weirdness here: we point the next DMA pointers to a
	   buffer which potentially already has good data in it.
	   But we don't update that buffer's BlockNumber or Intransit status
	   because we HOPE we're going to get the current data out of that
	   buffer before data starts DMAing into it.
	   We don't use the safer method of updating the BlockNumber here,
	   because then we would need at least three buffers.   With this
	   algorithm, we only need two buffers.  The cost is that after
	   after getting data out of a buffer, we have to check again to
	   make sure the buffer hasn't changed (see WaitReadBlock). */
	bp = IntransitBuffer->Next;
	ConfigNextDMA(DMA_CH0, 
		(uint8*)&bp->DiscBlock, sizeof(struct DiscBlock));
	EnableDMA(DMA_CH0);

	/* Start playing */
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) return err;
	return OK;

Error:
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	return -1;
}

/***************************************************************************
 Check DMA hardware and update software state.
*/
static void
UpdateDMA(void)
{
	struct DiscBuffer *bp;

	if (IntransitBuffer == NULL || !DMADone(DMA_CH0))
		return;

	/* Update status of current buffer (just finished)
	   and next buffer (now receiving data). */
	bp = IntransitBuffer;
	IntransitBuffer = IntransitBuffer->Next;
	bp->Intransit = FALSE;
	IntransitBuffer->Intransit = TRUE;
	IntransitBuffer->BlockNumber = bp->BlockNumber + 1;

	/* Set up "next" DMA pointers so data for the next block 
	   (after the now current one) has someplace to go. */
	bp = IntransitBuffer->Next;
	ConfigNextDMA(DMA_CH0, (uint8*)&bp->DiscBlock, sizeof(struct DiscBlock));
}

/***************************************************************************
 Convert block number to Minutes/Seconds/Frames.
*/
static uint32
BlockToMSF(uint32 block)
{
	uint32 m, s, f;
	m = block / (FRAMEperSEC*SECperMIN);
	s = (block % (FRAMEperSEC*SECperMIN)) / FRAMEperSEC;
	f = block % FRAMEperSEC;
	return (BIN2BCD(m) << 16) | (BIN2BCD(s) << 8) | BIN2BCD(f);
}

/***************************************************************************
 Copy a block of data.
 Source & destination must be uint32-aligned, len is a multiple of uint32.
*/
static void
memcpy32(uint32 *d, uint32 *s, int len)
{
	while (len > 0) 
	{
		*d++ = *s++;
		len -= sizeof(uint32);
	}
}


/***************************************************************************
*/
static int32
LCCD_GetLastECC(void)
{
	return CorrectedECC;
}

/***************************************************************************
 Extract information from various types of Q reports.
*/
static void
SetDiscInfoA0(DiscInfo *di, uchar *report)
{
	di->di_FirstTrackNumber = BCD2BIN(report[QR_PMIN]);
	di->di_DiscId = report[QR_PSEC]; 
#ifdef DEBUG_Q_REPORTS
	PRINTF(("POINT=A0: FirstTrack %x\n",di->di_FirstTrackNumber));
	PRINTF(("DiscId %x\n", di->di_DiscId));
#endif
}

static void
SetDiscInfoA1(DiscInfo *di, uchar *report)
{
	di->di_LastTrackNumber = BCD2BIN(report[QR_PMIN]); 
#ifdef DEBUG_Q_REPORTS
	PRINTF(("POINT=A1: lastTrack %x\n", di->di_LastTrackNumber));
#endif
}

static void
SetDiscInfoA2(DiscInfo *di, uchar *report)
{
	di->di_MSFEndAddr_Min = BCD2BIN(report[QR_PMIN]);
	di->di_MSFEndAddr_Sec = BCD2BIN(report[QR_PSEC]);
	di->di_MSFEndAddr_Frm = BCD2BIN(report[QR_PFRAME]);
#ifdef DEBUG_Q_REPORTS
	PRINTF(("POINT=A2: Min/Sec/Frm %x %x %x\n", di->di_MSFEndAddr_Min, 
		di->di_MSFEndAddr_Sec, di->di_MSFEndAddr_Frm));
#endif
}

static void
SetTOCInfo(TOCInfo *toc, uchar *report)
{
	/* toc_AddrCntrl has ADR in high nibble and CTL in low nibble, 
	 * which is backwards from what we read off the disc. */
	toc->toc_AddrCntrl = SwapNibbles(report[QR_ADRCTL]);
	toc->toc_CDROMAddr_Min = BCD2BIN(report[QR_PMIN]);
	toc->toc_CDROMAddr_Sec = BCD2BIN(report[QR_PSEC]);
	toc->toc_CDROMAddr_Frm = BCD2BIN(report[QR_PFRAME]);
#ifdef DEBUG_Q_REPORTS
	PRINTF(("TOC entry: AdrCtl %x, Min/Sec/Frm %x %x %x\n",
		toc->toc_AddrCntrl, toc->toc_CDROMAddr_Min, 
		toc->toc_CDROMAddr_Sec, toc->toc_CDROMAddr_Frm));
#endif
}

/***************************************************************************
 Read the table of contents and disc info from the disc.
*/
static int
ReadTOCAndDiscInfo(TOCInfo *toc, DiscInfo *di, int track, Boolean *pMultiSession)
{
	int err;
	uint32 got;
	uint32 retries;
	uchar point;
	uchar report[QCODE_REPORT_SIZE];

#define	GOT_A0		0x1
#define	GOT_A1		0x2
#define	GOT_A2		0x4
#define	GOT_TOC		0x8
#define	GOT_TOC1	0x10

#ifdef DEBUG_MAJOR
	PRINTF(("ReadTOCAndDiscInfo:\n"));
#endif
	if (DRIVE_OPEN())
		return -1;

	/* Check to see if the info we need is already cached. */
	if (gotCachedInfo)
	{
		if (toc != NULL)
			*toc = cachedTOC1;
		if (di != NULL)
			*di = cachedDiscInfo;
		if (pMultiSession != NULL)
			*pMultiSession = cachedMultiSession;
#ifdef DEBUG_MAJOR
		PRINTF(("Found all info in cache!\n"));
#endif
		return OK;
	}

	EmptyBuffers();

	/* Pause the drive and change to AUDIO mode, so we can read the TOC. */
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK)
	{
		PRINTF(("ReadTOC: error in pause\n"));
		return err;
	}
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK)
	{
		PRINTF(("ReadTOC: error waiting for PAUSE\n"));
		return err;
	}
	err = SendDiscCmd(AudioFormatCmd, sizeof(AudioFormatCmd));
	if (err < OK)
	{
		PRINTF(("ReadTOC: error in sector format\n"));
		return err;
	}
	retries = 0;
Again:
	/* Disable all reports (except switch reports). */
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	/* Flush all status bytes (is this necessary?) */
	FlushStatusFifo();

	/* Seek to beginning of TOC. */
	err = SeekDisc((int32)0);
	if (err < OK) 
	{
		PRINTF(("ReadTOC: error in seek(0)\n"));
		return err;
	}

	/* Start playing the data. 
	 * Watch the QCODEs and parse them to get the TOC and disc info. */
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) 
	{
		PRINTF(("ReadTOC: error in play\n"));
		return err;
	}
	err = WaitDriveState(DRV_PLAY);
	if (err < OK)
	{
		PRINTF(("ReadTOC: error waiting for PLAY\n"));
		return err;
	}

	EnableReports(REPORT_QCODE, REPORT_ENABLE);

	got = 0;
	for (;;)
	{
		if (DRIVE_OPEN())
			return -1;

		if ((got & (GOT_A0|GOT_A1|GOT_A2|GOT_TOC1)) == 
			   (GOT_A0|GOT_A1|GOT_A2|GOT_TOC1))
			gotCachedInfo = TRUE;

		if (gotCachedInfo && ((got & GOT_TOC) || toc == NULL))
		{
			/* Got everything we need. */
			break;
		}

		err = GetResponse(report, sizeof(report), REPORT_QCODE, 
				100*MILLISEC);
		if (err < OK)
			return -1;
		switch (report[QR_ADRCTL] & 0xF)
		{
		case 0: /* Is 0 the same as 1? */
		case 1:
			if (report[QR_TNO] != 0)
			{
				/* This is a normal data-area entry, 
				 * not a lead-in (TOC) entry.
				 * We've run off the end of the TOC. */
#ifdef DEBUG_MAJOR
				PRINTF(("****** TNO != 0; return *****\n"));
#endif
				goto Retry;
			}

			switch (report[QR_POINT])
			{
			case 0xA0:
				got |= GOT_A0;
				if (!gotCachedInfo)
				{
					SetDiscInfoA0(&cachedDiscInfo, report);
					/* If this is a CD-I disc, return
					 * error.  A CD-I TOC is different
					 * than normal TOCs, and we don't have
					 * the code here to interpret it. */
					if (cachedDiscInfo.di_DiscId == 0x10)
					{
						PRINTF(("CD-I disc!\n"));
						return -1;
					}
				}
				if (di != NULL) 
					SetDiscInfoA0(di, report);
				break;
			case 0xA1:
				got |= GOT_A1;
				if (!gotCachedInfo)
					SetDiscInfoA1(&cachedDiscInfo, report);
				if (di != NULL) 
					SetDiscInfoA1(di, report);
				break;
			case 0xA2:
				got |= GOT_A2;
				if (!gotCachedInfo)
					SetDiscInfoA2(&cachedDiscInfo, report);
				if (di != NULL) 
					SetDiscInfoA2(di, report);
				break;
			default:
				point = BCD2BIN(report[QR_POINT]);
				if (toc != NULL && point == track)
				{
					/* This is the track we want. */
					got |= GOT_TOC;
					SetTOCInfo(toc, report);
				}
				if (point == 1)
				{
					/* This is track 1; cache it */
					got |= GOT_TOC1;
					SetTOCInfo(&cachedTOC1, report);
				}
				break;
			}
			break;
		case 2: /* UPC/EAN */
		case 3: /* ISRC */
			break;
		case 5: /* multisession */
			if (!gotCachedInfo)
				cachedMultiSession = TRUE;
			if (pMultiSession != NULL)
				*pMultiSession = TRUE;
			break;
		default:
			PRINTF(("Adr?=%x\n", report[QR_ADRCTL]));
			break;
		}
	}

	/* Disable Qcode reports. */
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	/* Pause the drive and put it back in DATA mode. */
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK)
	{
		PRINTF(("ReadTOC: error in PAUSE\n"));
		return err;
	}
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK)
	{
		PRINTF(("ReadTOC: error waiting for PAUSE\n"));
		return err;
	}
	err = SendDiscCmd(DataFormatCmd, sizeof(DataFormatCmd));
	if (err < OK)
	{
		PRINTF(("ReadTOC: error in FORMAT\n"));
		return err;
	}
	return OK;

Retry:
	if (++retries <= TOC_RETRIES)
		goto Again;
	PRINTF(("FAIL\n"));
	return -1;
}

/***************************************************************************
 Return the WData from the disc.
 Read totalSize bytes of WData and calls a callback function each time it
 gets bufSize bytes of WData.  
 If speed is nonzero, switch to that speed first.
 If block is nonzero, seek to that block first.
*/
static int32
LCCD_GetWData(DDDFile *fd, uint32 block, uint32 speed, 
	uint32 bufSize, uint32 totalSize,
	int32 (*Callback)(DDDFile *fd, void *callbackArg, void *buf, uint32 bufSize),
	void *callbackArg)
{
	void *currBuf;
	void *nextBuf;
	void *buf;
	int32 err;
	int32 err2;
	uchar GetWDataCmd[4];
	uchar resp[GET_WDATA_RESP_SIZE];
	TimerState tm;

#if 1 /* FIXME: remove when old (pre-MEI) drives are unsupported */
if (strcmp(fd->fd_HWResource->dev.hwr_Name, "LCCD\\3000") == 0)
{
  /* Old drives don't understand DeviceGetWData(). */
  EPRINTF(("CD drive ok\n"));
  return -1;
}
#endif
	if (DRIVE_OPEN())
		return -1;
	if (bufSize > sizeof(struct DiscBlock))
		/* Since we use DiscBlocks for temp buffers, 
		 * bufSize must fit in a DiscBlock. */
		return -1;

	DisableDMA(DMA_CH0|DMA_CH1);
	if (speed != 0 && speed != DefaultSpeed)
	{
		/* Switch disc speed. */
		err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
		if (err < OK) return err;
		err = WaitDriveState(DRV_PAUSE);
		if (err < OK) return err;
		err = SendSpeedCmd(speed);
		if (err < OK) return err;
	}
	if (block != 0)
	{
		/* Seek to the desired block. */
		err = SeekDisc(block);
		if (err < OK) goto Exit;
	}

	/* Start the drive playing. */
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) goto Exit;
	err = WaitDriveState(DRV_PLAY);
	if (err < OK) goto Exit;

#if 1
	/* FIXME: why do we need this delay? */
	Delay(10);
#endif

	/* Set up DMA to receive the WData. */
	EmptyBuffers();
	currBuf = &FirstBuffer->DiscBlock;
	nextBuf = &FirstBuffer->Next->DiscBlock;
	InvalidateDCache(currBuf, bufSize);
	InvalidateDCache(nextBuf, bufSize);
	ConfigDMA(DMA_CH1, currBuf, bufSize);
	ConfigNextDMA(DMA_CH1, nextBuf, bufSize);
	EnableDMA(DMA_CH1);

	/* Send the GetWData command to the drive. */
	GetWDataCmd[0] = CMD_GET_WDATA;
	GetWDataCmd[1] = (uchar) (totalSize >> 16);
	GetWDataCmd[2] = (uchar) (totalSize >> 8);
	GetWDataCmd[3] = (uchar) (totalSize);
	err = SendCommand(GetWDataCmd, sizeof(GetWDataCmd), NOTIMEOUT);
	if (err < 0) goto Exit;

	for (;;)
	{
		/* DMA is going into currBuf.  Wait for it to finish. */
		ResetTimer(&tm);
		while (!DMADone(DMA_CH1))
		{
			if (ReadMilliTimer(&tm) > 2000*MILLISEC)
			{
				PRINTF(("GetWData timeout!  %x left\n",
					totalSize));
				err = -1;
				goto Exit;
			}
		}
		/* DMA is now going into nextBuf.
		 * Meanwhile, process currBuf. */
		err = (*Callback)(fd, callbackArg, currBuf, bufSize);
		if (err < 0) goto Exit;
		InvalidateDCache(currBuf, bufSize);
		totalSize -= bufSize;
		if (totalSize <= 0)
			/* Processed all data; we're done. */
			break;
		/* Swap buffers. */
		buf = currBuf;  currBuf = nextBuf;  nextBuf = buf;
		/* Now DMA is going into currBuf.  Setup for nextBuf. */
		ConfigNextDMA(DMA_CH1, nextBuf, bufSize);
		/* Make sure DMA didn't finish too early. */
		if (DMADone(DMA_CH1))
		{
			PRINTF(("DMA overflow after callback!\n"));
			err = -1;
			goto Exit;
		}
	}
Exit:
	/* Finally, get the response to the GetWData command. */
	err2 = GetResponse(resp, sizeof(resp), CMD_GET_WDATA, 2000*MILLISEC);
	if (err >= 0)
		err = err2;
	DisableDMA(DMA_CH0|DMA_CH1);
	if (DRIVE_OPEN())
		return -1;
	if (speed != 0 && speed != DefaultSpeed)
	{
		/* Switch back to default speed. */
		(void) SendDiscCmd(PauseCmd, sizeof(PauseCmd));
		(void) WaitDriveState(DRV_PAUSE);
		(void) SendSpeedCmd(DefaultSpeed);
	}
	return err;
}

#ifdef CHECK_FIRMWARE
/***************************************************************************
 Display error message if RSA check of firmware fails.
*/
static void
DisplayFirmwareError(DipirTemp *dt)
{
	VideoPos pos;
	VideoRect rect;
	bootGlobals *bg = &dt->dt_BootGlobals;

	/* This pile of bits is a VideoImage structure which 
	 * displays the "SELF TEST FAILED" message. */
	static uint32 firmwareMessage[] = {
		0x01000000, 0x00000000, 0x00000108, 0x00160060,
		0x01020000, 0x00000000, 0xffffffff, 0xffffffff,
		0xffffffff, 0x80000000, 0x00000000, 0x00000001,
		0x80000000, 0x00000000, 0x00000001, 0x8007cfe8,
		0x0fe00fef, 0xe7cfe001, 0x80082808, 0x08000108,
		0x08210001, 0x80080808, 0x08000108, 0x08010001,
		0x8007cf88, 0x0f80010f, 0x87c10001, 0x80002808,
		0x08000108, 0x00210001, 0x80082808, 0x08000108,
		0x08210001, 0x8007cfef, 0xe800010f, 0xe7c10001,
		0x80000000, 0x00000000, 0x00000001, 0x80000000,
		0x00000000, 0x00000001, 0x80000fe1, 0x03880fef,
		0xc3800001, 0x80000802, 0x81080808, 0x23800001,
		0x80000804, 0x41080808, 0x23800001, 0x80000f88,
		0x21080f88, 0x21000001, 0x8000080f, 0xe1080808,
		0x20000001, 0x80000808, 0x21080808, 0x23800001,
		0x80000808, 0x238fefef, 0xc3800001, 0x80000000,
		0x00000000, 0x00000001, 0x80000000, 0x00000000,
		0x00000001, 0xffffffff, 0xffffffff, 0xffffffff,
	};

	rect.LL.x = rect.LL.y = 0;
	rect.UR.x = bg->bg_LinePixels;
	rect.UR.y = bg->bg_ScrnLines;
	FillRect(rect, 0x0000);

	pos.x = (bg->bg_LinePixels - 0x60) / 2;
	pos.y = (bg->bg_ScrnLines - 0x16) / 2;
	DisplayImage((VideoImage*)firmwareMessage, pos, 1, 0, 0);
}

/***************************************************************************
 Read a block of the firmware.
*/
static int 
ReadFirmwareBlock(uint32 block, uint8 *buffer, 
		uint32 *pNumBlocks, uint32 *pSigOffset)
{
	int err;
	uchar ReadFirmwareCmd[4];
	uchar resp[READ_FIRMWARE_RESP_SIZE];
	TimerState tm;

	DisableDMA(DMA_CH0|DMA_CH1);
	ConfigDMA(DMA_CH0, buffer, FIRMWARE_BLOCK_SIZE);
	EnableDMA(DMA_CH0);

	ReadFirmwareCmd[0] = CMD_READ_FIRMWARE;
	ReadFirmwareCmd[1] = (uint8)block;
	ReadFirmwareCmd[2] = 0;
	ReadFirmwareCmd[3] = 0;
	err = SendDiscCmdWithResponse(ReadFirmwareCmd, sizeof(ReadFirmwareCmd),
			resp, sizeof(resp), 2000*MILLISEC);
	if (err < OK)
	{
		PRINTF(("ReadFirmware: SendCmd error\n"));
		DisableDMA(DMA_CH0|DMA_CH1);
		return err;
	}
	*pNumBlocks = (uint32) resp[1];
	*pSigOffset = (uint32) MakeInt16(resp[2], resp[3]);
	ResetTimer(&tm);
	while (!DMADone(DMA_CH0))
	{
		if (ReadMilliTimer(&tm) > 2000*MILLISEC)
		{
			PRINTF(("ReadFirmware: DMA timeout\n"));
			DisableDMA(DMA_CH0|DMA_CH1);
			return -1;
		}
	}
	DisableDMA(DMA_CH0|DMA_CH1);
	return OK;
}

/***************************************************************************
 Check validity of the firmware.
 This is a crude attempt to prevent hardware licencees from modifying
 the firmware without approval from 3DO.
*/
static void
CheckFirmware(DDDFile *fd)
{
	uint32 block;
	uint8 *buffer;
	uint32 numBlocks;
	uint32 sigOffset;
	uint8 sig[SIG_LEN];

	buffer = (uint8*) FirstBuffer;
	block = 0;
	DipirInitDigest();
	do {
		if (ReadFirmwareBlock(block, buffer, 
				&numBlocks, &sigOffset) < OK)
		{
			PRINTF(("CheckFirmware: read error\n"));
			goto Bogus;
		}
		if (numBlocks < MIN_FIRMWARE_SIZE / FIRMWARE_BLOCK_SIZE)
		{
			PRINTF(("CheckFirmware: too small %x\n", numBlocks));
			goto Bogus;
		}
		if (block == sigOffset / FIRMWARE_BLOCK_SIZE)
		{
			sigOffset %= FIRMWARE_BLOCK_SIZE;
			memcpy(sig, buffer + sigOffset, SIG_LEN);
			memset(buffer + sigOffset, 0, SIG_LEN);
		}
		DipirUpdateDigest(buffer, FIRMWARE_BLOCK_SIZE);
	} while (++block < numBlocks);
	DipirFinalDigest();
	if (RSAFinal(sig, KEY_128) == 0)
	{
		PRINTF(("CheckFirmware: RSA fail\n"));
		goto Bogus;
	}
	return;

Bogus:
	/* Firmware check failed.  Display error message and hang. */
	PRINTF(("===== CheckFirmware: BOGUS!\n"));
	DisplayFirmwareError(fd->fd_DipirTemp);
	for (;;) ;
}
#endif /* CHECK_FIRMWARE */

/***************************************************************************
 Determine if a CD drive is connected to the system.
*/
Boolean
DrivePresent(DDDFile *fd)
{
	int err;
	char *p;
	uchar resp[READ_ID_REPORT_SIZE];

	/* Try sending a READID command and see if it times out. */
	err = SendDiscCmdWithResponse(ReadIDCmd, sizeof(ReadIDCmd), 
		resp, sizeof(resp), 500*MILLISEC);
	if (err != OK)
		return FALSE;
	/* Update the device name with the hardware version number. */
	p = fd->fd_HWResource->dev.hwr_Name;
	p += strlen(p);
	*p++ = '\\';
	SPutHex((resp[5] << 8) | resp[6], p, 4);
	p[4] = '\0';
	return TRUE;
}

/***************************************************************************
*/
static Boolean
DigitalDisc(void)
{
	int32 err;
	TOCInfo ti;

	err = ReadTOCAndDiscInfo(&ti, (DiscInfo*)NULL, 1, (Boolean*)NULL);
	if (err < 0)
		return FALSE;
	if (ti.toc_AddrCntrl & (ACB_AUDIO_PREEMPH|ACB_FOUR_CHANNEL))
		return FALSE;
	if (!(ti.toc_AddrCntrl & ACB_DIGITAL_TRACK))
		return FALSE;
	return TRUE;
}

/***************************************************************************
 Initialize the disc drive.
*/
int
LCCD_Open(DDDFile *fd)
{
	int state;
	int err;
	uint32 timeout;
	Boolean sentPause = FALSE;
	uchar resp[MECH_TYPE_RESP_SIZE];

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
/*PRINTF(("LCCD: buf1 %x, buf1 %x\n", &FirstBuffer->DiscBlock, &FirstBuffer->Next->DiscBlock));*/
#ifdef DEBUG_MAJOR
	PRINTF(("LCCD_Open called\n"));
#endif
	TIMESTAMP("LCop",0);
	fd->fd_BlockSize = DISC_BLOCK_SIZE;

	LCCDSysDepInit();

	if (!DrivePresent(fd))
	{
		/* 
		 * This would be more appropriate in the channel driver,
		 * but it would require a lot of code to be duplicated there.
		 * Mark the HWResource to be deleted as soon as possible.
		 */
		PRINTF(("LCCD not connected\n"));
		fd->fd_HWResource->dev_Flags |= HWR_DELETE;
		return DDD_OPEN_IGNORE;
	}

	EnableReports(REPORT_SWITCH, REPORT_NEVER);

	/* Determine drive speed. */
	err = SendDiscCmdWithResponse(MechTypeCmd, sizeof(MechTypeCmd),
			resp, sizeof(resp), NOTIMEOUT);
	if (err < OK) return -1;

	if (resp[2] & MECH_TYPE_SPEED_8)
		MaxSpeed = 8;
	else if (resp[2] & MECH_TYPE_SPEED_6)
		MaxSpeed = 6;
	else if (resp[2] & MECH_TYPE_SPEED_4)
		MaxSpeed = 4;
	else
		MaxSpeed = 2;
	DefaultSpeed = MaxSpeed;
#if 1 /* FIXME: remove when Babette is fixed and can work at 4x. */
if (DefaultSpeed == 4 &&
    strcmp(fd->fd_HWResource->dev.hwr_Name, "LCCD\\3000") == 0)
{
  DefaultSpeed = 2;
  fd->fd_HWResource->dev.hwr_Name[8]++; /* Change version from 3000 to 3001. */
}
#endif
	PRINTF(("DefaultSpeed %d\n", DefaultSpeed));

	InitBuffers();
	EmptyBuffers();

	TIMESTAMP("LCwt",0);
	timeout = 0;
	for (;;)
	{
		if (timeout > 20000*MILLISEC)
		{
			EPRINTF(("LCCD open timeout\n"));
			LCCDSysDepExit();
			return -1;
		}
		state = GetDriveState();
#if 1 /* FIXME: this works around a firmware bug */
{
		int sw;
		sw = GetSwitchState();
		if (sw < OK)
		{
			LCCDSysDepExit();
			return -1;
		}
		if ((sw & CLOSE_SWITCH) == 0)
		{
			/* Door is open */
			if (state == DRV_PAUSE || state == DRV_PLAY)
			{
PRINTF(("LCCD: State %x should be STOPPING\n", state));
				state = DRV_STOPPING;
			}
		}
PRINTF(("(%x,%x)", state, sw));
}
#endif
		if (state < OK)
		{
			LCCDSysDepExit();
			return -1;
		}
		if (state == DRV_PAUSE)
			break;

		switch (state) 
		{
		case DRV_STOPPING:
		case DRV_CLOSING:
		case DRV_FOCUSING:
		case DRV_SPINNINGUP:
		case DRV_SEEKING:
		case DRV_LATENCY:
			/* Wait for stable state */
			break;
		case DRV_STOP:
		case DRV_STOP_FOCUSED:
			/* The only way we can be in STOP state is as
			 * a transition to OPEN.  Just wait for OPEN. */
			break;
		case DRV_OPEN:
		case DRV_OPENING:
			PRINTF(("InitDisc: door open\n"));
#ifdef CHECK_FIRMWARE
			CheckFirmware(fd);
#endif
			LCCDSysDepExit();
			return DDD_OPEN_IGNORE;
		case DRV_UNREADABLE:
		case DRV_SEEKFAILURE:
			EjectDisc();
			/* Fall thru */
		case DRV_FOCUSERROR:
		case DRV_STUCK:
			PRINTF(("InitDisc: drive error %x\n", state));
#ifdef CHECK_FIRMWARE
			CheckFirmware(fd);
#endif
			LCCDSysDepExit();
			return DDD_OPEN_IGNORE;
		case DRV_PLAY:
		default:
			/* Keep waiting... */
			if (!sentPause)
			{
				/* Try to go into PAUSE state. */
				state = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
				if (state < OK)
				{
					EPRINTF(("InitDisc: error in pause\n"));
					LCCDSysDepExit();
					return -1;
				}
				sentPause = TRUE;
			}
			break;
		}
		Delay(100*MILLISEC);
		timeout += 100*MILLISEC;
	}

	TIMESTAMP("LCsp",DefaultSpeed);
	if (SendSpeedCmd(DefaultSpeed))
	{
		PRINTF(("error in Speed cmd\n"));
	}

	/* Cache the TOC and disc info. */
	TIMESTAMP("LCtc",0);
	if (ReadTOCAndDiscInfo((TOCInfo*)NULL, (DiscInfo*)NULL,
					-1, (Boolean *)NULL) < OK)
	{
		EPRINTF(("LCCD open error in read TOC\n"));
		return -1;
	}
#ifdef DEBUG_MAJOR
	PRINTF(("ready\n"));
#endif
	TIMESTAMP("LCok",0);
	return 0;
}

/***************************************************************************
 Clean up device after we're done using it.
*/
static int32
LCCD_Close(DDDFile *fd)
{
	int state;

	TOUCH(fd);
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	EnableReports(REPORT_SWITCH, REPORT_NEVER);
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	state = GetDriveState();
	if (state == DRV_PLAY) {
		(void) SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	}
	LCCDSysDepExit();
	return 0;
}

/***************************************************************************
*/
static int32
ReadBlock(DDDFile *fd, uint32 block, void *buffer)
{
	uint32 latest;
#define	WAITLIMIT	(4) /* blocks */

	TOUCH(fd);
#ifdef DEBUG_IO
	PRINTF(("ReadBlock(%x)\n", block));
#endif
	if (DRIVE_OPEN())
	{
		PRINTF(("Door open!\n"));
		return -1;
	}
	if (!DigitalDisc())
	{
		PRINTF(("Not digital disc!\n"));
		return -1;
	}

	ReqBlock = block;
	ReqBuffer = buffer;
	UpdateDMA();
	if (FindBuffer(block) != NULL)
	{
		/* The block is already in a buffer
		   (it may be Intransit, but that's ok). */
		return 0;
	}
	latest = LatestBlock();
	if (latest != 0 && block > latest && block < latest + WAITLIMIT)
	{
		/* The block is close enough that we won't bother 
		   to seek; we'll just wait until we play to it. */
		return 0;
	}
PRINTF(("Read(%x): got %x %x\n", block, 
FirstBuffer->BlockNumber, FirstBuffer->Next->BlockNumber));
	(void) StartPlaying(block);
	return 0;
}

/***************************************************************************
 Wait for the disc read currently in progress to complete.
*/
static int32
WaitReadBlock(DDDFile *fd)
{
	int err;
	uint32 msf;
	uint32 retries;
	uint32 timeInLoop;
	uint32 reg;
	struct DiscBuffer *bp;
	TimerState tm;

	retries = 0;
Start:
	if (IntransitBuffer == NULL)
	{
		PRINTF(("WaitReadBlock: no intransit\n"));
		goto Restart;
	}

	/* Wait for DMA completion.  When DMA completes, the buffer
	   will be available via FindBuffer. */
	ResetTimer(&tm);
	timeInLoop = 0;
	for (;;) 
	{
		if (ReadMilliTimer(&tm) > 100*MILLISEC)
		{
			timeInLoop += 100;
			if (DRIVE_OPEN())
				return -1;
			/*
			 * We may have timed out because the device got
			 * BLOCKED (door was opened & closed while we
			 * were in dipir).  Check the BBLOCK register
			 * to detect this.
			 */
			reg = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_BBLOCK);
			if (reg & CDE_CDROM_DIPIR)
				longjmp(fd->fd_DipirTemp->dt_JmpBuf, JMP_OFFLINE);
			ResetTimer(&tm);
			if (timeInLoop > 1000*MILLISEC)
			{
				PRINTF(("Read timeout, bufs %x(%x) %x(%x)\n",
					FirstBuffer->BlockNumber, 
					FirstBuffer->Intransit,
					FirstBuffer->Next->BlockNumber,
					FirstBuffer->Next->Intransit));
				goto Restart;
			}
		}
		UpdateDMA();
		/* See if the buffer is available and not in-transit.
		 * If so, break out of this loop. */
		bp = FindBuffer(ReqBlock);
		if (bp != NULL && !bp->Intransit)
			break;
		if (IntransitBuffer->BlockNumber > ReqBlock)
		{
			/* Oops, we are past the block we want. */
			PRINTF(("Missed block %x, now reading %x\n",
				ReqBlock, IntransitBuffer->BlockNumber));
			goto Restart;
		}
	}

	if ((bp->DiscBlock.db_Header >> 8) != 
	    (bp->DiscBlock.db_Completion >> 8))
	{
		PRINTF(("MSF mismatch (1): hdr %x, comp %x\n",
			bp->DiscBlock.db_Header,
			bp->DiscBlock.db_Completion));
		goto Restart;
	}

	CorrectedECC = 0;
	if (bp->DiscBlock.db_Completion & COMPL_ECC)
	{
		/* Try ECC correction. */
		PRINTF(("WaitReadBlock: ECC error\n"));
		DisableDMA(DMA_CH0|DMA_CH1);
		if (IntransitBuffer != NULL)
		{
			IntransitBuffer->Intransit = FALSE;
			IntransitBuffer = NULL;
		}
		CorrectedECC = SectorECC((uint8*)&bp->DiscBlock);
		PRINTF(("ECC=%x\n", CorrectedECC));
		if (CorrectedECC < 0)
			goto Restart;
	}
	memcpy32((uint32*)(ReqBuffer), (uint32*)(&bp->DiscBlock.db_Data), 
		fd->fd_BlockSize);

	UpdateDMA();
	if (bp->BlockNumber != ReqBlock || bp->Intransit)
	{
		/* Yikes! It changed!
		   This is the weird case.  The DMA caught up with us,
		   and we didn't get the data out in time.  It may have
		   been corrupted by the DMA before we copied it.
		   Start over. */
		PRINTF(("Missed block: wanted %x, blk now %x, intransit %x\n",
			ReqBlock, bp->BlockNumber, bp->Intransit));
		goto Restart;
	}

	/* Make sure the header word and completion word match the block
	   number we requested. */
	msf = BlockToMSF(ReqBlock);
	if ((bp->DiscBlock.db_Header >> 8) != msf)
	{
		PRINTF(("MSF mismatch (2): exp %x, hdr %x, comp %x\n", 
			msf, bp->DiscBlock.db_Header,
			bp->DiscBlock.db_Completion));
		goto Restart;
	}
	return OK;

Restart:
	if (++retries > READ_RETRIES)
	{
		PRINTF(("Max restarts exceeded\n"));
		return -1;
	}
	PRINTF(("WaitReadBlock(%x): restarting seek/read\n", ReqBlock));
	err = StartPlaying(ReqBlock);
	if (err < OK) return err;
	goto Start;
}

/***************************************************************************
*/
static int32
LCCD_WaitRead(DDDFile *fd, void *id)
{
	int32 ret;

	if ((int32)id < 0)
		return (int32)id;
	ret = WaitReadBlock(fd);
	if (ret < 0)
		return ret;
	return (int32)id;
}

/***************************************************************************
 Start reading some data from the disc.
*/
static void *
LCCD_ReadAsync(DDDFile *fd, uint32 block, uint32 nblocks, void *buffer)
{
	uint32 blk;
	uint8 *buf;

	buf = buffer;
	for (blk = 0;  blk < nblocks;  blk++)
	{
		if (blk > 0)
		{
			if (WaitReadBlock(fd) < 0)
				return (void *)-1;
		}
		if (ReadBlock(fd, block + blk, buf) < 0)
			return (void *)-1;
		buf += fd->fd_BlockSize;
	}
	return (void *)nblocks;
}

/***************************************************************************
*/
	static int32
LCCD_GetInfo(DDDFile *fd, DeviceInfo *info)
{
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	info->di_Version = 1;
	info->di_BlockSize = DISC_BLOCK_SIZE;
	info->di_FirstBlock = 
		cachedTOC1.toc_CDROMAddr_Frm + 
		(cachedTOC1.toc_CDROMAddr_Sec * FRAMEperSEC) +
		(cachedTOC1.toc_CDROMAddr_Min * FRAMEperSEC * SECperMIN);
	info->di_NumBlocks = 
		cachedDiscInfo.di_MSFEndAddr_Frm +
		(cachedDiscInfo.di_MSFEndAddr_Sec * FRAMEperSEC) +
		(cachedDiscInfo.di_MSFEndAddr_Min * FRAMEperSEC * SECperMIN);
	info->di_NumBlocks -= info->di_FirstBlock;
	return 0;
}

/***************************************************************************
*/
	static int32
LCCD_GetCDFlags(DDDFile *fd, uint32 *flags)
{
	int32 err;
	Boolean multiSession;

	TOUCH(fd);
	*flags = 0;
	err = ReadTOCAndDiscInfo((TOCInfo*)NULL, (DiscInfo*)NULL,
					-1, &multiSession);
	if (err < OK)
		return err;
	if (multiSession)
		*flags |= CD_MULTISESSION;
	return 0;
}

/***************************************************************************
 Read a table of contents from the disc.
*/
	static int32
LCCD_GetTOC(DDDFile *fd, uint32 track, TOCInfo *ti)
{
	int32 err;

	TOUCH(fd);
	err = ReadTOCAndDiscInfo(ti, (DiscInfo*)NULL, track, (Boolean*)NULL);
#ifdef DEBUG_MAJOR
	{
		PRINTF(("ReadTOC: track %x, AdrCtl %x, M/S/F %x %x %x\n",
			track, ti->toc_AddrCntrl, ti->toc_CDROMAddr_Min,
			ti->toc_CDROMAddr_Sec, ti->toc_CDROMAddr_Frm));
	}
#endif
	return err;
}

/***************************************************************************
 Read the disc info from the disc.
*/
static int32
LCCD_GetDiscInfo(DDDFile *fd, DiscInfo *di)
{
	int32 err;

	TOUCH(fd);
	err = ReadTOCAndDiscInfo((TOCInfo*)NULL, di, -1, (Boolean*)NULL);
#ifdef DEBUG_MAJOR
	{
		PRINTF(("ReadDiscInfo: discId %x, firstTrack %x, last %x, end %x %x %x\n",
			di->di_DiscId, di->di_FirstTrackNumber,
			di->di_LastTrackNumber, di->di_MSFEndAddr_Min,
			di->di_MSFEndAddr_Sec, di->di_MSFEndAddr_Frm));
	}
#endif
	di->di_MaxSpeed = MaxSpeed;
	di->di_DefaultSpeed = DefaultSpeed;
	return err;
}

/***************************************************************************
*/
static int32
LCCD_Eject(DDDFile *fd)
{
	TOUCH(fd);
	EjectDisc();
	return 0;
}

/***************************************************************************
*/
	int32
LCCD_CDFirmCmd(DDDFile *fd, uint8 *cmd, uint32 cmdLen,
	uint8 *resp, uint32 respLen, uint32 timeout)
{
	TOUCH(fd);
	return SendDiscCmdWithResponse(cmd, cmdLen, resp, respLen, timeout);
}

/***************************************************************************
*/
static int32
LCCD_RetryLabel(DDDFile *fd, uint32 *pState)
{
	int32 block;

	TOUCH(fd);

	block = cachedTOC1.toc_CDROMAddr_Frm + 
		(cachedTOC1.toc_CDROMAddr_Sec * FRAMEperSEC) +
		(cachedTOC1.toc_CDROMAddr_Min * FRAMEperSEC * SECperMIN);
	switch (*pState)
	{
	case 0:
		/* Try first data block. */
		break;
	case 1:
		/* Try alternate block. */
		block += DISC_LABEL_OFFSET;
		break;
	default:
		return -1;
	}

	(*pState)++;
	return block;
}

/***************************************************************************
*/
static DDDCmdTable LCCDCmdTable[] =
{
	{ DDDCMD_OPEN,		(DDDFunction *) LCCD_Open },
	{ DDDCMD_CLOSE,		(DDDFunction *) LCCD_Close },
	{ DDDCMD_READASYNC,	(DDDFunction *) LCCD_ReadAsync },
	{ DDDCMD_WAITREAD,	(DDDFunction *) LCCD_WaitRead },
	{ DDDCMD_GETINFO,	(DDDFunction *) LCCD_GetInfo },
	{ DDDCMD_EJECT,		(DDDFunction *) LCCD_Eject },
	{ DDDCMD_RETRYLABEL,	(DDDFunction *) LCCD_RetryLabel },
	/* CD-specific commands */
	{ DDDCMD_CD_GETFLAGS,	(DDDFunction *) LCCD_GetCDFlags },
	{ DDDCMD_CD_GETINFO,	(DDDFunction *) LCCD_GetDiscInfo },
	{ DDDCMD_CD_GETTOC,	(DDDFunction *) LCCD_GetTOC },
	{ DDDCMD_CD_GETLASTECC,	(DDDFunction *) LCCD_GetLastECC },
	{ DDDCMD_CD_GETWDATA,	(DDDFunction *) LCCD_GetWData },
	{ DDDCMD_CD_FIRMCMD,	(DDDFunction *) LCCD_CDFirmCmd },
	{ 0 }
};

static DDD LCCD_DDD = { LCCDCmdTable };

	DDD *
InitDriver(DipirTemp *dt)
{
	TOUCH(dt);
	return &LCCD_DDD;
}
