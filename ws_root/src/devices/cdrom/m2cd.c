/* @(#) m2cd.c 96/09/06 1.70 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/cache.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/super.h>
#include <kernel/operror.h>
#include <kernel/interrupts.h>
#ifdef BUILD_M2CD_MONITOR
#include <kernel/msgport.h>
#endif
#include <kernel/sysinfo.h>
#include <kernel/dipir.h>
#include <kernel/debug.h>
#include <hardware/cde.h>
#include <device/m2cd.h>
#include <loader/loader3do.h>
#include <strings.h>

/* This only used to verify that hardware CRC (or software ECC) works */
#define VERIFY_CRC_CHECK	0
#define DEBUG_ECC			0

/* DBUG/2 statements compiled in? */
#ifdef BUILD_STRINGS
#define DEBUG				1
#define DEBUG2				1
#endif

#ifdef DEBUG
	#define DBUG(a,x)	if (gPrintTheStuff & (a)) Superkprintf x
#else
	#define DBUG(a,x)
#endif

#ifdef DEBUG2
	#define DBUG2(a,x)	if (gPrintTheStuff & (a)) Superkprintf x
#else
	#define DBUG2(a,x)
#endif

#ifdef BUILD_STRINGS
	/* Error statements always enabled */
	#define DBUGERR(x)	Superkprintf x
#else
	#define DBUGERR(x)
#endif

/* macros */
#define	CHK4ERR(x,str)	if ((x)<0) { DBUGERR(str); return (x); }
#define	CHK4ERRwithACTION(x,action,str)	if ((x)<0) { action; DBUGERR(str); return (x); }
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#define SwapNybble(x)	(((x) << 4) | (((x) >> 4) & 0x0F))

#define Check4ErrorOrNeededSleep(response)   		\
			if ((response) == kCmdNotYetComplete)	\
				break;       						\
			if ((response) < kCDNoErr)				\
				return (response)

#ifdef BUILD_M2CD_MONITOR
#define kXStat			50
#define kYStat			70
#define kXCIPRpt		50
#define kYCIPRpt		110
#define kXQRpt			50
#define kYQRpt			130
#define kXSwCurVal		50
#define kYSwCurVal		150
#define kXSwDelta		90
#define kYSwDelta		150

#define kStrCIP			0x00010000
#define kStrSwitch		0x00020000
#define kStrCmd			0x00030000
#define kStrCmdLine		0x00040000
#define kStrNull		0x00050000

#define MyDrawString(str,x,y)	{ \
			Item theMsg = WaitPort(cd->cd_MonitorReplyPort, 0);	\
			SendSmallMsg(cd->cd_MonitorMsgPort,					\
						theMsg, (str), ((x) << 16) | (y)); }
#endif

/* prototype definitions */
/* utilities */
uint8	BCD2BIN(uint8 bcd);
uint8	BIN2BCD(uint8 bin);
uint32	Offset2BCDMSF(int32 offset);
uint32	MSF2Offset(uint32 MSF, uint8 selector);
uint32	bufcmp(uint8 *buf1, uint8 *buf2);
uint32	GenChecksum(uint32 *buf);

/* kernel mantra */
Item	LCCDDriverInit(Driver *drv);
Err		LCCDDriverDelete(Driver *drv);
Item	LCCDDeviceInit(Device *dev);
Err		LCCDDeviceDelete(Device *dev);
Err		LCCDDriverDaemon(void);
Err		LCCDDriverDispatch(IOReq *theReq);
void	LCCDDriverAbortIO(IOReq *theReq);

/* internal buffer support routines */
Err		AllocateStandardBuffers(cdrom *cd);
Err		ResizeLinkedBuffers(cdrom *cd, uint32 size);

/* cmd (front-end) stuff */
Err		SendCmdWithWaitState(cdrom *cd, uint8 *cmd, uint8 cmdLen, uint8 desiredCIP, uint8 *errCode);
Err		SendCmdToDev(cdrom *cd, uint8 *cmdBuf, uint8 cmdLen);
IOReq	*CmdCallBack(IOReq *theReq);


/* snarfer (back-end) stuff */
Err		SetupSnarfer(cdrom *cd, uint32 init);
void	FlushStatusFIFO(void);
int32	CDSFLHandler(void);
int32	ProcessSnarfBuffers(cdrom *cd);
void	HandleSwReport(cdrom *cd, uint8 *statusBytes, int32 x);
void	ParseTOCEntries(cdrom *cd, uint8 *statusBytes, int32 x);

/* dipir stuff */
int32	DipirHandler(void);
Err		PrepareDuckAndCover(cdrom *cd);
uint32	Ducky(cdrom *cd);
uint32	Recovery(cdrom *cd);

/* misc. support routines */
void	InitDataSpace(cdrom *cd);
void	InitSubcodeSpace(cdrom *cd);
void	InitPrefetchEngine(cdrom *cd);
void	PrepareCachedInfo(cdrom *cd);
void	TakeDeviceOffline(cdrom *cd);
void	AbortCurrentIOReqs(cdrom *cd);
void	CompleteWorkingIOReq(IOReq *theReq, Err theErr);

/* daemon-state/cmd-action machines */
Err		OpenDrawer(cdrom *cd);
Err		InitReports(cdrom *cd);
Err		BuildDiscData(cdrom *cd);
Err		DuckIntoDipir(cdrom *cd);
Err		RecoverFromDipir(cdrom *cd);
Err		StopPrefetching(cdrom *cd);
void	ProcessClientIOReqs(cdrom *cd);
int32	CmdStatus(cdrom *cd, IOReq *theReq);
int32	DiscData(cdrom *cd, IOReq *theReq);
Err		Read(cdrom *cd);
Err		GetWobbleInfo(cdrom *cd);
Err		ReadSubQ(cdrom *cd);
Err		SetDefaults(cdrom *cd);
Err		ResizeBufferSpace(cdrom *cd);
Err		Monitor(cdrom *cd);
Err		Passthru(cdrom *cd);

/* read support routines */
Err		VerifyCmdOptions(cdrom *cd, uint32 opts);
uint8	MKE2BOB(cdrom *cd, int32 densityCode);
uint32	ValidBlockLengthFormat(uint8 format, int32 len);
uint32	DataFormatDontJive(cdrom *cd, uint32 theMSF, uint8 format);
int8	DataAvailNow(cdrom *cd, int32 sector, uint8 format, uint32 opts, uint8 *blk);
uint32	DataAvailRSN(cdrom *cd, int32 sector, uint8 format);
uint32	SanityCheckSector(cdrom *cd, int8 blk);
void	CopySectorData(cdrom *cd, uint8 *dst, uint8 *src, int32 blkLen, uint8 format);

/* subcode support */
uint32	SubcodeSyncedUp(cdrom *cd);
void	CopyDescrambledSubcode(cdrom *cd, uint8 *dst, uint8 *src);
void	CopySyncedSubcode(cdrom *cd, uint8 *dst);

/* dma-related */
void	EnableLCCDDMA(uint8 channels);
void	DisableLCCDDMA(uint8 channels);
void	ConfigLCCDDMA(uint8 channel, uint8 curORnext, uint8 *dst, int32 len, uint8 fromFIRQ);
int32	ChannelZeroFIRQ(void);
int32	ChannelOneFIRQ(void);

/* globals */

static const TagArg cdRomDaemonTags[] =
{
    TAG_ITEM_NAME,                      (void *)"M2CD Daemon",
    TAG_ITEM_PRI,                       (void *)220,
    CREATETASK_TAG_PC,                  (void *)LCCDDriverDaemon,
    CREATETASK_TAG_STACKSIZE,           (void *)2048,
    CREATETASK_TAG_SUPERVISOR_MODE,     (void *)TRUE,
    CREATETASK_TAG_THREAD,              (void *)TRUE,
    TAG_END,                            (void *)0
};

static void		*gCDEBase;							/* base addr of CDE */
static cdrom	*gDevice;							/* ptr to device globals */
static Item		gDaemonItem;
static uint32	gNumDataBlksInUse;					/* current number of data blocks marked VALID */
static uint32	gNumSnarfBlksInUse;					/* current number of snarfer buffer blocks in use */
static uint32	gPrintTheStuff;						/* kPrintf DBUG() bits */

static uint32	gSubcodeSyncMissCount;				/* count of missed subcode sync marks...used to determine if the subcode is no longer synced up */

static uint32	gHighWaterMark;
static uint32	gMaxHWM;

static uint32	gECCTimer;
#ifdef DEVELOPMENT
static uint32	gBaddCode[4];
static uint32	gDeadBabe[4];
#if VERIFY_CRC_CHECK
static uint8	gECCBuf[2352];
#endif
#endif


#ifdef BUILD_M2CD_MONITOR
Err Monitor(cdrom *cd)
{
	IOReq	*theReq = cd->cd_workingIOR;
	uint8	x;

	if (theReq->io_Info.ioi_UserData)
	{
		cd->cd_MonitorMsgPort = FindMsgPort("M2CD Monitor");
		CHK4ERR(cd->cd_MonitorMsgPort, ("M2CD:  >ERROR<  Could not find monitor message port\n"));

		cd->cd_MonitorReplyPort = CreateMsgPort(NULL, 0, 0);
		CHK4ERR(cd->cd_MonitorReplyPort, ("M2CD:  >ERROR<  Could not create monitor reply port\n"));

		for (x = 0; x < MAX_NUM_MESSAGES; x++)
		{
			cd->cd_MonitorMsg[x] = CreateSmallMsg(NULL, 0, cd->cd_MonitorReplyPort);
			CHK4ERR(cd->cd_MonitorMsg[x], ("M2CD:  >ERROR<  Could not create monitor message\n"));
			ReplySmallMsg(cd->cd_MonitorMsg[x], 0, 0, 0);
		}
	}
	else
	{
		for (x = 0; x < MAX_NUM_MESSAGES; x++)
			if (cd->cd_MonitorMsg[x])
				DeleteMsg(cd->cd_MonitorMsg[x]);

		if (cd->cd_MonitorReplyPort)
			DeleteMsgPort(cd->cd_MonitorReplyPort);

		cd->cd_MonitorMsgPort = 0;
	}

	return (kCmdComplete);
}

void MonitorSnarfBuffer(cdrom *cd, uint8 *data, uint8 x, uint8 len)
{
	if (!cd->cd_MonitorMsgPort)
		return;

	switch (data[x])
	{
		case CD_QREPORTEN:
			MyDrawString(kStrNull, kXQRpt, kYQRpt);
			MyDrawString(kStrNull, kXQRpt+50, kYQRpt);
		case CDE_DIPIR_BYTE:
		case CD_LED:
		case CD_SETSPEED:
		case CD_SPINDOWNTIME:
		case CD_SECTORFORMAT:
		case CD_CIPREPORTEN:
		case CD_SWREPORTEN:
		case CD_SENDBYTES:
		case CD_PPSO:
		case CD_SEEK:
			MyDrawString((kStrCmd | data[x]), kXStat, kYStat);
			MyDrawString(kStrNull, kXStat+20, kYStat+20);
			break;

		case CD_CHECKWO:
			MyDrawString((kStrCmd | data[x]), kXStat, kYStat);
			MyDrawString(kStrNull, kXStat+20, kYStat+20);
			for (x = 1; x <= 8; x++)
				MyDrawString(data[x], kXStat+(20*x), kYStat+20);
			break;

		case CD_MECHTYPE:
			MyDrawString((kStrCmd | data[x]), kXStat, kYStat);
			MyDrawString(kStrNull, kXStat+20, kYStat+20);
			for (x = 1; x <= 7; x++)
				MyDrawString(data[x], kXStat+(20*x), kYStat+20);
			break;

		case CD_CIPREPORT:
			MyDrawString((kStrCIP | data[x+1]), kXCIPRpt, kYCIPRpt);
			break;

		case CD_QREPORT:
			MyDrawString(SwapNybble(data[x+1]), kXQRpt, kYQRpt);
			for (x = 2; x <= 10; x++)
				MyDrawString(data[x], kXQRpt+(20*(x-1)), kYQRpt);
			break;

		case CD_SWREPORT:
			MyDrawString((kStrSwitch | data[x+1]), kXSwCurVal, kYSwCurVal);
			MyDrawString((kStrSwitch | data[x+2]), kXSwDelta, kYSwDelta);
			break;

		case CD_READID:
		default:
			MyDrawString((kStrCmd | data[x]), kXStat, kYStat);
			MyDrawString(kStrNull, kXStat+20, kYStat+20);
			for (x = 1; x < len; x++)
				MyDrawString(data[x], kXStat+(20*x), kYStat+20);
			break;
	}
}
#else
#define Monitor(foo)	kCDErrBadCommand
#define MonitorSnarfBuffer(cd, data, x, len)
#endif

/*******************************************************************************
* BCD2BIN() and BIN2BCD()                                                      *
*                                                                              *
*      Purpose: Utility routines used to convert a byte from binary to BCD.    *
*                                                                              *
*******************************************************************************/
uint8 BCD2BIN(uint8 bcd)
{
	return (((bcd >> 4) * 10) + (bcd & 0x0F));
}

uint8 BIN2BCD(uint8 bin)
{
	uint8	quo = 0;
	uint8	rem = bin;

	while (rem >= 10)
	{
		quo++;
		rem -= 10;
	}

	return ((quo << 4) | rem);
}

/*******************************************************************************
* Offset2BCDMSF()                                                              *
*                                                                              *
*      Purpose: Utility routine to convert a 32-bit binary offset to a 24-bit  *
*               BCD min, sec, frm.                                             *
*                                                                              *
*   Parameters: offset - 32-bit binary offset                                  *
*                                                                              *
*      Returns: uint32 - 0x00MMSSFF in BCD                                     *
*                                                                              *
*******************************************************************************/
uint32 Offset2BCDMSF(int32 offset)
{
	uint8	theMSF[4];

	theMSF[0] = 0;
	theMSF[1] = (uint8)((offset / (75L * 60L)) & 0xFF);			/* binary min */
	theMSF[2] = (uint8)(((offset % (75L * 60L)) / 75L) & 0xFF);	/* binary sec */
	theMSF[3] = (uint8)(((offset % (75L * 60L)) % 75L) & 0xFF);	/* binary frm */

	theMSF[1] = BIN2BCD(theMSF[1]);								/* BCD min */
	theMSF[2] = BIN2BCD(theMSF[2]);								/* BCD sec */
	theMSF[3] = BIN2BCD(theMSF[3]);								/* BCD frm */

	return (*(uint32 *)theMSF);
}

#define kBinary	0
#define kBCD	1
#define BCDMSF2Offset(x)	MSF2Offset((x), kBCD)
#define BinMSF2Offset(x)	MSF2Offset((x),	kBinary)

/*******************************************************************************
* MSF2Offset()                                                                 *
*                                                                              *
*      Purpose: Utility routine to convert a 24-bit MSFs to a 32-bit binary    *
*               offsets.  The MSF can be in either BCD or Binary.              *
*                                                                              *
*   Parameters: MSF      - 0x00MMSSFF, minutes, seconds, frames of sector      *
*                          address.                                            *
*               selector - denotes either BCD MSF or Binary MSF                *
*                                                                              *
*      Returns: uint32 - 32-bit binary offset                                  *
*                                                                              *
*******************************************************************************/
uint32 MSF2Offset(uint32 MSF, uint8 selector)
{
	uint32	minutes, seconds, frames;

	minutes = (MSF >> 16) & 0xFF;							/* BCD min */
	seconds = (MSF >> 8) & 0xFF;							/* BCD sec */
	frames = MSF & 0xFF;									/* BCD frm */

	if (selector == kBCD)
	{
		minutes = BCD2BIN((uint8)minutes);					/* binary min */
		seconds = BCD2BIN((uint8)seconds);					/* binary sec */
		frames = BCD2BIN((uint8)frames);					/* binary frm */
	}

	return ((((minutes * 60L) + seconds) * 75L) + frames);
}

/*******************************************************************************
* Check4Drive()                                                                *
*                                                                              *
*      Purpose: Determine whether or not an internal drive is connected to CDE.*
*               NOTE:  See SendCmdToDev() and CDSFLHandler() for comments on   *
*                      this code.                                              *
*                                                                              *
*      Returns: Err - if no drive present, 0 otherwise.                        *
*                                                                              *
*******************************************************************************/
Err Check4Drive(void)
{
#define MAXUNITS 1
	HWResource hwdev;
	int32 r;
	int32 numUnits = 0;

	hwdev.hwr_InsertID = 0;
	for (;;)
	{
		r = SuperQuerySysInfo(SYSINFO_TAG_HWRESOURCE, &hwdev, sizeof(hwdev));
		if (r == SYSINFO_END)
			break;
		if (r != SYSINFO_SUCCESS)
		{
			DBUGERR(("sysinfo error %x\n", r));
			break;
		}
		if (!MatchDeviceName("LCCD", hwdev.hwr_Name, DEVNAME_TYPE))
			continue;
		if (numUnits >= MAXUNITS)
		{
			DBUGERR(("M2CD: too many units!\n"));
			break;
		}
		numUnits++;
	}

	if (!numUnits)
		return (kCDErrNoLCCDDevices);

	return (kCDNoErr);
}

/*******************************************************************************
* DipirHandler()                                                               *
*                                                                              *
*      Purpose: Handle drive-initiated Media Access events.                    *
*                                                                              *
*******************************************************************************/
int32 DipirHandler(void)
{
	cdrom	*cd = gDevice;

	DBUG(kPrintDipirStuff, ("M2CD:  Media Access on M2CD detected.\n"));

	/* If the media access was caused by the drive, call
	 * TriggerDeviceRescan() to initiate the Duck/SoftReset/
	 * Dipir/Recover mechanism.
	 */
	if (CDE_READ(gCDEBase, CDE_BBLOCK) & CDE_CDROM_DIPIR)
	{
		DBUG(kPrintDipirStuff, ("M2CD: TriggerDeviceRescan()\n"));
		cd->cd_State |= (CD_DIPIRING | CD_GOTO_DIPIR);
		TriggerDeviceRescan();
	}

	ClearInterrupt(INT_CDE_MC);
	return 0;
}

/*******************************************************************************
* LCCDDeviceInit()                                                             *
*                                                                              *
*      Purpose: Initialization routine for M2CD device.  Checks for the        *
*               of hardware and then initializes the device structure.  Then   *
*               spawns LCCD driver daemon task to handle ioReq processing.     *
*                                                                              *
*   Parameters: cd - pointer to cd-rom structure, memory supplied by OS.       *
*                                                                              *
*      Returns: Item number of device.                                         *
*                                                                              *
*    Called by: OS, upon driver item creation -- during createLCCDDriver()     *
*                                                                              *
*******************************************************************************/
Item LCCDDeviceInit(Device *dev)
{
	/*
	 * Find HWResource here, if need to.
	 * dev->dev_HWResource has ID of HWResource.
	 */

	dev->dev_DriverData = gDevice;
	return (dev->dev.n_Item);
}

/*******************************************************************************
* LCCDDeviceDelete()                                                           *
* FIXME
*******************************************************************************/
Err LCCDDeviceDelete(Device *dev)
{
	TOUCH(dev);
	return 0;
}

/*******************************************************************************
* LCCDDriverInit()                                                             *
*******************************************************************************/
Item LCCDDriverInit(Driver *drv)
{
	Err	err;
	CDEInfo	cdeInfo;
	cdrom *	cd;
	uint8	x;

	cd = (cdrom *) SuperAllocMem(sizeof(cdrom), MEMTYPE_NORMAL|MEMTYPE_FILL);
printf("CD: cdrom %x, cd_State %x\n", cd, &cd->cd_State);
	if (cd == NULL)
		return NOMEM;
	gDevice = cd;

	/* get CDE base addr */
	(void)SuperQuerySysInfo(SYSINFO_TAG_CDE, &cdeInfo, sizeof(cdeInfo));
	gCDEBase = cdeInfo.cde_Base;

	/* allocate standard buffer space (subcode space [2K] + 6 data buffers [6 x 2352] = one 16K segment) */
	err = AllocateStandardBuffers(cd);
	CHK4ERR(err, ("M2CD:  >ERROR<  Could not allocate standard buffer space\n"));

	/* allocate extended buffer space (13 data buffers [13 x 2352] = one 32K DRAM page) */
	err = ResizeLinkedBuffers(cd, CDROM_STANDARD_BUFFERS);
	CHK4ERR(err, ("M2CD:  >ERROR<  Could not allocate extended buffer space\n"));

	gSubcodeSyncMissCount = 0;

	InitList(&cd->cd_pendingIORs,"pending_iors");	/* pending ioRequests */
	cd->cd_workingIOR = NULL;				/* no current ioRequest */
	cd->cd_State = 0;						/* clear all internal state bits */
	cd->cd_DevState = DS_INIT_REPORTS;		/* initial device state */

	cd->cd_MonitorMsgPort = 0;
	cd->cd_MonitorReplyPort = 0;
	for (x = 0; x < MAX_NUM_MESSAGES; x++)
		cd->cd_MonitorMsg[x] = 0;

	cd->cd_PrefetchStartOffset = 0;
	cd->cd_PrefetchEndOffset = 0;
	cd->cd_PrefetchCurMSF = 0;
	cd->cd_PrefetchSectorFormat = INVALID_SECTOR;

	cd->cd_DefaultOptions.asFields.densityCode =	CDROM_DEFAULT_DENSITY;
	cd->cd_DefaultOptions.asFields.addressFormat =	CDROM_Address_Blocks;
	cd->cd_DefaultOptions.asFields.speed =			CDROM_DOUBLE_SPEED;
	cd->cd_DefaultOptions.asFields.pitch =			CDROM_PITCH_NORMAL;
	cd->cd_DefaultOptions.asFields.blockLength =	CDROM_MODE1;
	cd->cd_DefaultOptions.asFields.errorRecovery =	CDROM_DEFAULT_RECOVERY;

	/* MKE spec. gives 8 as the default.
	 * The closest we can get given the current cd-rom driver API is 7.
	 * NOTE:  retryShift gets "(1 << retryShift) - 1" applied to it.
	 */
	cd->cd_DefaultOptions.asFields.retryShift =		3;

	DisableLCCDDMA(DMA_CH0 | DMA_CH1);				/* just for kicks... */

	(void)SetupSnarfer(cd, TRUE);

	cd->cd_CDSFLHandler = SuperCreateFIRQ("M2CD Status Channel",
		CDSFL_FIRQ_PRIORITY, CDSFLHandler, INT_CDE_CDSFL);
	CHK4ERR(cd->cd_CDSFLHandler, ("M2CD:  >ERROR<  Can't register CDSFL handler\n"));
	ClearInterrupt(INT_CDE_CDSFL);  /* clear int left over by dipir */
	EnableInterrupt(INT_CDE_CDSFL);

	cd->cd_DipirHandler = SuperCreateFIRQ("Dipir handler", DIPIR_FIRQ_PRIORITY,
		DipirHandler, INT_CDE_MC);
	CHK4ERR(cd->cd_Ch0FIRQ, ("M2CD:  >ERROR<  Can't register DIPIR handler\n"));
	ClearInterrupt(INT_CDE_MC);
	EnableInterrupt(INT_CDE_MC);

	err = PrepareDuckAndCover(cd);
	CHK4ERR(err, ("M2CD:  >ERROR<  Could not prepare duck-n-cover\n"));

	/* Make sure we didn't miss one (e.g. door close during boot). */
	CDSFLHandler();
	DipirHandler();

	cd->cd_Ch0FIRQ = SuperCreateFIRQ("M2CD Data Channel",
		CH0_DMA_FIRQ_PRIORITY, ChannelZeroFIRQ, INT_CDE_DMAC4);
	CHK4ERR(cd->cd_Ch0FIRQ, ("M2CD:  >ERROR<  Can't register Data handler\n"));

	cd->cd_Ch1FIRQ = SuperCreateFIRQ("M2CD Subcode Channel",
		CH1_DMA_FIRQ_PRIORITY, ChannelOneFIRQ, INT_CDE_DMAC5);
	CHK4ERR(cd->cd_Ch1FIRQ, ("M2CD:  >ERROR<  Can't register Subcode handler\n"));

	/* create LCCD driver daemon */
	DBUG(kPrintGeneralStatusStuff, ("M2CD:  Creating the driver daemon\n"));
	gDaemonItem = SuperCreateItem(MKNODEID(KERNELNODE, TASKNODE), cdRomDaemonTags);
	CHK4ERR(gDaemonItem, ("M2CD:  >ERROR<  Could not create driver daemon\n"));

#if DEBUG_ECC
	gPrintTheStuff = kPrintDeath | kPrintWarnings |
		 kPrintQtyNPosOfSectorReqs;
#else
	gPrintTheStuff = kPrintDeath | kPrintWarnings;
#endif

	/* for debugging */
/*
	DBUG2(kPrintWarnings, ("M2CD:  gECCTimer @ %08lx\n", &gECCTimer));
	DBUG2(kPrintWarnings, ("M2CD:  gHWM @ %08lx\n", &gHighWaterMark));
*/
	DBUG2(kPrintWarnings, ("M2CD:  gPTS %lx, data block headers %lx, snarf buffer %lx\n", &gPrintTheStuff, cd->cd_DataBlkHdr, cd->cd_SnarfBuffer));

	return drv->drv.n_Item;
}

/*******************************************************************************
* LCCDDriverChangeOwner()                                                      *
*******************************************************************************/

Err LCCDDriverChangeOwner(Driver *drv, Item newOwner)
{
	cdrom	*cd;

	TOUCH(drv);
	cd = gDevice;
	SetItemOwner(cd->cd_CDSFLHandler, newOwner);
	SetItemOwner(cd->cd_DipirHandler, newOwner);
	SetItemOwner(cd->cd_Ch0FIRQ, newOwner);
	SetItemOwner(cd->cd_Ch1FIRQ, newOwner);
	SetItemOwner(gDaemonItem, newOwner);
	return 0;
}

/*******************************************************************************
* LCCDDriverDelete()                                                           *
* FIXME
*******************************************************************************/
Err LCCDDriverDelete(Driver *drv)
{
	TOUCH(drv);
	SuperFreeMem(gDevice, sizeof(cdrom));
	return 0;
}

/*******************************************************************************
* LCCDDriverDaemon()                                                           *
*                                                                              *
*      Purpose: Performs device monitoring tasks and handles ioReq processing. *
*                                                                              *
*      Returns: Any error that occured -- should only happen if daemon is      *
*               killed.                                                        *
*                                                                              *
*    Called by: LCCDDaemonStub()                                               *
*                                                                              *
*******************************************************************************/
Err LCCDDriverDaemon(void)
{
	cdrom	*cd;
	uint32	awakenedMask;
	int32	r;
	Err		err;

	/* grab first (currently, only) device */
	cd = gDevice;

	cd->cd_DaemonTask = CURRENTTASK;
	cd->cd_DaemonSig = SuperAllocSignal(0L);
	cd->cd_RecoverSig = SuperAllocSignal(0L);

	/* gotta make one pass thru the daemon BEFORE going to sleep
	 * in order to turn on reports, read the TOC, etc. for device init
	 */
	SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

	while (TRUE)
	{
		DBUG(kPrintSignalAwakeSleep, ("M2CD:  Daemon going to sleep...\n"));
		awakenedMask = SuperWaitSignal(cd->cd_DaemonSig);
		DBUG(kPrintSignalAwakeSleep, ("M2CD:  The Daemon has awaken\n"));

		/* somebody killed us...gotta bail */
		if (awakenedMask & SIGF_ABORT)
		{
			DBUG(kPrintSignalAwakeSleep, ("M2CD:  Daemon dieing\n"));
			/* indicate internally that the daemon has gone away */
			cd->cd_DaemonTask = NULL;

			/* abort any currently active or pending client ioReq's */
			AbortCurrentIOReqs(cd);

			/* turn off any DMA */
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);

			/* unregister the firq handlers */
			SuperDeleteItem(cd->cd_Ch0FIRQ);
			SuperDeleteItem(cd->cd_Ch1FIRQ);
			SuperDeleteItem(cd->cd_CDSFLHandler);
			SuperDeleteItem(cd->cd_DipirHandler);

			SuperFreeSignal(cd->cd_DaemonSig);
			SuperFreeSignal(cd->cd_RecoverSig);

			DBUG2(kPrintDeath, ("M2CD:  >DEATH<  We've been killed!\n"));
			break;
		}

		if (awakenedMask & cd->cd_DaemonSig)
		{
			/* Currently, this assumes one LCCD device. */
			cd = gDevice;

		Again:
			/* If dipiring any device, wait till it's done. */
			if (cd->cd_State & CD_DIPIRING)
				continue;

			/* Switch to the post-dipir recovery state if necessary. */
			if (cd->cd_State & CD_GOTO_DIPIR)
			{
				cd->cd_DevState = DS_CLOSE_DRAWER;
			}

			/* handle all async status coming from LCCD device */
			DisableInterrupt(INT_CDE_CDSFL);
			r = ProcessSnarfBuffers(cd);
			EnableInterrupt(INT_CDE_CDSFL);
			if (r) goto Again;

			if (cd->cd_State & CD_SNARF_OVERRUN)
			{
				DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Snarf Buffer Overrun!  (probable loss of status info)\n"));
				err = SetupSnarfer(cd, FALSE);
				if (err < 0)
				{
					DBUG2(kPrintDeath, ("M2CD:  >DEATH<  Error setting-up snarfer\n"));
				}
			}

			if ((cd->cd_State & CD_PREFETCH_OVERRUN) &&
				((cd->cd_DevState & kMajorDevStateMask) != DS_STOP_PREFETCHING))
			{
				/* did the prefetch buffers get filled up? */
				cd->cd_SavedDevState = cd->cd_DevState;
				cd->cd_DevState = DS_STOP_PREFETCHING;
			}

			/* The lack of a 'break's in the cases in this switch statement is
			 * INTENTIONAL.  We want to fall thru and begin the next state
			 * without falling asleep.
			 */
			switch (cd->cd_DevState & kMajorDevStateMask)
			{
				case DS_OPEN_DRAWER:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_OPEN_DRAWER\n"));
					if (!OpenDrawer(cd))
						break;

					goto processReqs;

				case DS_CLOSE_DRAWER:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_CLOSE_DRAWER\n"));
					cd->cd_State &= ~(CD_CACHED_INFO_AVAIL | CD_NO_DISC_PRESENT | CD_UNREADABLE | CD_GOTO_DIPIR);
					cd->cd_DevState = DS_INIT_REPORTS;

				case DS_INIT_REPORTS:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_INIT_REPORTS\n"));
					if (!InitReports(cd))
						break;

					switch (cd->cd_CIPState)
					{
						case CIPPlay:
						case CIPPause:
							/* disc present, device on-line */
							cd->cd_State |= CD_DEVICE_ONLINE;
							break;
						case CIPFocusing:
						case CIPSpinningUp:
						case CIPSeeking:
						case CIPLatency:
							continue;
						default:
							DBUG(kPrintDipirStuff, ("M2CD:  >WARNING<  Unexpected CIPState (%02x) after dipir\n", cd->cd_CIPState));
							/* no break here is intentional */
						case CIPOpen:
						case CIPOpening:
						case CIPStuck:
						case CIPFocusError:
						case CIPClosing:
						case CIPStopping:
							cd->cd_State |= CD_NO_DISC_PRESENT;
						case CIPUnreadable:
						case CIPSeekFailure:
							cd->cd_State |= CD_UNREADABLE;

							/* clear this bit if set by no disc being present */
							cd->cd_State &= ~CD_DEVICE_ERROR;

							PrepareCachedInfo(cd);
							AbortCurrentIOReqs(cd);
							goto processReqs;
					}
					cd->cd_DevState = DS_BUILDING_DISCDATA;

				case DS_BUILDING_DISCDATA:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_BUILDING_DISCDATA\n"));
					err = BuildDiscData(cd);
					if (!err)
						break;
					if (err < 0)
					{
						cd->cd_State |= CD_UNREADABLE;
						PrepareCachedInfo(cd);
						AbortCurrentIOReqs(cd);
					}
processReqs:
					cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;

				case DS_PROCESS_CLIENT_IOREQS:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_PROCESS_CLIENT_IOREQS\n"));
					ProcessClientIOReqs(cd);
					break;

				case DS_DUCK_INTO_DIPIR:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_DUCK_INTO_DIPIR\n"));
					if (!DuckIntoDipir(cd))
						break;
					cd->cd_DevState = DS_RECOVER_FROM_DIPIR;

				case DS_RECOVER_FROM_DIPIR:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_RECOVER_FROM_DIPIR\n"));
					if (RecoverFromDipir(cd))
					{
						/* go to the start of current dev (major) state (this
						 * also RESTARTS any current working ioReq!)
						 */
#if 0
						cd->cd_DevState = (cd->cd_SavedDevState & kMajorDevStateMask);
#else
						cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
#endif
						DBUG(kPrintDipirStuff, ("M2CD:  new dev state...%02lx\n", cd->cd_DevState));
					}
					break;

				case DS_STOP_PREFETCHING:
					DBUG(kPrintDeviceStates, ("M2CD:  DS_STOP_PREFETCHING\n"));
					if (StopPrefetching(cd))	/* how to handle errors here? */
					{
						cd->cd_DevState = cd->cd_SavedDevState;		/* restore saved state */
					}
					break;
			}
		}
	}

	return (kCDErrDaemonKilled);				/* should never get here! */
}

/*******************************************************************************
* LCCDDriverDispatch()                                                         *
*                                                                              *
*      Purpose: Sticks the ioReq onto the pendingIORs queue and then signals   *
*               the daemon to process the ioReq.  Attempts to process          *
*               CMD_STATUS and CDROMCMD_DISCDATA calls immediately, if         *
*               possible.                                                      *
*                                                                              *
*   Parameters: theReq - pointer to incoming ioRequest                         *
*                                                                              *
*      Returns: Err - Error, if one occured                                    *
*                                                                              *
*    Called by: OS, upon receiving a SendIO() to this driver                   *
*                                                                              *
*******************************************************************************/
Err LCCDDriverDispatch(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev->dev_DriverData;
	uint32	interrupts;
	Err		cmdDone = kCmdNotYetComplete;

	DBUG(kPrintSendCompleteIO, ("M2CD:**SendIO(%02x)\n", theReq->io_Info.ioi_Command));

	switch (theReq->io_Info.ioi_Command)
	{
		case CDROMCMD_RESIZE_BUFFERS:
		case CDROMCMD_WOBBLE_INFO:
		case CDROMCMD_MONITOR:
		case CDROMCMD_PASSTHRU:
			/* only allow RESIZE_BUFFERS for privileged tasks */
			if (CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
				goto queueItUp;

			if (!(theReq->io_Flags & IO_INTERNAL))
				CompleteWorkingIOReq(theReq, kCDErrNotPrivileged);
			return (theReq->io_Error);

		case CMD_STATUS:
		case CDROMCMD_DISCDATA:
			/* CMD_STATUS, _DISCDATA can be called anytime after we have at
			 * least _attempted_ to read the TOC.
			 */
			cmdDone = (theReq->io_Info.ioi_Command == CMD_STATUS) ? CmdStatus(cd, theReq) : DiscData(cd, theReq);
			if (cmdDone)
			{
				if (!(theReq->io_Flags & IO_INTERNAL))
					CompleteWorkingIOReq(theReq, kCDNoErr);
				break;
			}
			/* no break is intentional here */
		case CMD_READ:
		case CMD_BLOCKREAD:
		case CDROMCMD_READ:
		case CDROMCMD_SCAN_READ:
		case CDROMCMD_READ_SUBQ:
		case CDROMCMD_OPEN_DRAWER:
		case CDROMCMD_SETDEFAULTS:
queueItUp:
			theReq->io_Flags &= ~IO_QUICK;

			/* insert ioReq in pending queue (using InsertNodeFromTail() places
			 * it in priority order)
			 */
			interrupts = Disable();
			InsertNodeFromTail(&cd->cd_pendingIORs, (Node *)theReq);
			Enable(interrupts);

			/* signal daemon to process ioRequest */
			if (cd->cd_DaemonTask)
			{
				DBUG(kPrintSignalAwakeSleep, ("M2CD:  Signalling daemon\n"));
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);
			}
			break;
#ifdef DEVELOPMENT
		case CDROMCMD_DIAG_INFO:
			cmdDone = kCmdComplete;
			if (theReq->io_Info.ioi_Recv.iob_Buffer)
			{
				memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, cd, sizeof(cdrom));
				theReq->io_Actual = sizeof(cdrom);
				SuperCompleteIO(theReq);
				break;
			}
			else
			{
				uint8	*diagCmd = (uint8 *)theReq->io_Info.ioi_Send.iob_Buffer;

				switch (diagCmd[0])
				{
					case 'p':
						gPrintTheStuff ^= theReq->io_Info.ioi_CmdOptions;
						DBUGERR(("M2CD:  gPTS now set to %08lx\n", gPrintTheStuff));
						break;
					case 'h':
						gMaxHWM = theReq->io_Info.ioi_CmdOptions;
						DBUGERR(("M2CD:  gMaxHWM now set to %ld\n", gMaxHWM));
						break;
				}
				theReq->io_Actual = 0;
				SuperCompleteIO(theReq);
				break;
			}
#endif
		default:
			if (!(theReq->io_Flags & IO_INTERNAL))
				CompleteWorkingIOReq(theReq, kCDErrBadCommand);
			return (theReq->io_Error);
	}

	return (cmdDone);
}

/*******************************************************************************
* LCCDDriverAbortIO()                                                          *
*                                                                              *
*      Purpose: Aborts an ioReq.  If it's currently processing this ioReq, it  *
*               bails.  If this ioReq is still pending, it marks it as         *
*               'aborted' and completes it.                                    *
*                                                                              *
*   Parameters: theReq - the ioReq to be aborted                               *
*                                                                              *
*    Called by: OS, during an AbortIO()                                        *
*                                                                              *
*******************************************************************************/
void LCCDDriverAbortIO(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev->dev_DriverData;
	IOReq	*ior;

	theReq->io_Error = kCDErrAborted;

	/* if it's not currently active, AND it's one of ours */
	if (theReq == cd->cd_workingIOR)
		return;
	ScanList(&cd->cd_pendingIORs, ior, IOReq)
	{
		if (ior == theReq)
		{
			RemNode((Node *)theReq);
			SuperCompleteIO(theReq);
			break;
		}
	}
}



/*******************************************************************************
* AllocateStandardBuffers()                                                    *
*                                                                              *
*      Purpose: Allocates the 16K which is required for minimum operation.     *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*      Returns: Err - if no memory was available.                              *
*                                                                              *
*    Called by: LCCDDeviceInit()                                               *
*                                                                              *
*******************************************************************************/
Err	AllocateStandardBuffers(cdrom *cd)
{
	uint8	x;
	uint8	*dataPtr;
	List	*list;
	BootAlloc *ba;

	if (SuperQuerySysInfo(SYSINFO_TAG_BOOTALLOCLIST, &list, sizeof(list)) != SYSINFO_SUCCESS)
		return (NOMEM);

	cd->cd_SubcodeBuffer = NULL;
	ScanList(list, ba, BootAlloc)
	{
		if ((ba->ba_Flags & BA_DIPIR_SHARED) &&
		    ba->ba_Size >= kStandardSpaceSize + kExtendedSpaceSize)
		{
			cd->cd_SubcodeBuffer = ba->ba_Start;
			break;
		}
	}
	if (cd->cd_SubcodeBuffer == NULL)
		return (NOMEM);

	/* initialize these to "minimum" configuration */
	cd->cd_NumDataBlks = MIN_NUM_DATA_BLKS;
	cd->cd_NumSubcodeBlks = MIN_NUM_SUBCODE_BLKS;

	/* init assuming that all subcode blocks are available/used */
	for (x = 0, dataPtr = (uint8 *)cd->cd_SubcodeBuffer; x < MAX_NUM_SUBCODE_BLKS; x++, dataPtr += kSubcodeBlkSize)
		cd->cd_SubcodeBlkHdr[x].buffer = dataPtr;

	/* initialize data buffer & indeces */
	for (x = 0, dataPtr = ((uint8 *)cd->cd_SubcodeBuffer + kSubcodeBufSizePlusSome); x < MIN_NUM_DATA_BLKS; x++, dataPtr += kDataBlkSize)
		cd->cd_DataBlkHdr[x].buffer = dataPtr;

	return (kCDNoErr);
}

/*******************************************************************************
* ResizeLinkedBuffers()                                                        *
*                                                                              *
*      Purpose: Resizes the internal prefetch buffer space between Standard    *
*               (MAX) mode & Minimum (MIN) mode.  Standard mode provides uses  *
*               48K of memory for the combined (data + subcode) prefetch       *
*               space; whereas Minimum mode uses only 16K for both (limiting   *
*               the data to 6 sectors of prefetch).  This allows the driver to *
*               free-up a 32K page to the application, if requested via        *
*               CDROMCMD_RESIZE_BUFFERS.  Currently, this is only allowed if   *
*               the caller is a privileged task (ie, the system).              *
*                                                                              *
*   Parameters: cd   - ptr to cdrom device struct                              *
*               size - the size/mode to switch to (0 = default, 1 = min,       *
*                      2 = std)                                                *
*                                                                              *
*      Returns: Err, if one occured                                            *
*                                                                              *
*    Called by: LCCDDeviceInit() and ResizeBufferSpace()                       *
*                                                                              *
*******************************************************************************/
Err ResizeLinkedBuffers(cdrom *cd, uint32 size)
{
	int8	x;
	uint8	*dataPtr;

	switch (size)
	{
		case CDROM_MINIMUM_BUFFERS:
			if (cd->cd_NumDataBlks == MIN_NUM_DATA_BLKS)
			{
				/* we're already in this mode
				 * (do we really want to return an error in this case?)
				 */
/*
				return (kCDErrBadIOArg);
*/
				break;
			}

			cd->cd_NumDataBlks = MIN_NUM_DATA_BLKS;
			cd->cd_NumSubcodeBlks = MIN_NUM_SUBCODE_BLKS;
			cd->cd_State |= CD_USING_MIN_BUF_SPACE;

			break;
		case CDROM_STANDARD_BUFFERS:
		case CDROM_DEFAULT_BUFFERS:			 /* the default is to use the max */
			if (cd->cd_NumDataBlks == MAX_NUM_DATA_BLKS)
			{
				/* we're already in this mode
				 * (do we really want to return an error in this case?)
				 */
/*
				return (kCDErrBadIOArg);
*/
				break;
			}

			cd->cd_NumDataBlks = MAX_NUM_DATA_BLKS;
			cd->cd_NumSubcodeBlks = MAX_NUM_SUBCODE_BLKS;
			cd->cd_State &= ~CD_USING_MIN_BUF_SPACE;

			/* in M2, extended buffer space starts immediately following the
			 * standard buffer space (in the DipirSharedBuffer)..
			 */
			for (x = MIN_NUM_DATA_BLKS, dataPtr = (uint8 *)cd->cd_SubcodeBuffer + kStandardSpaceSize;
				x < MAX_NUM_DATA_BLKS;
				x++, dataPtr += kDataBlkSize)
			{
				cd->cd_DataBlkHdr[x].buffer = dataPtr;
			}

			break;
		default:
			return (kCDErrBadArg);
	}

	/* update max HighWaterMark for new buffer space */
	gMaxHWM = (uint32)cd->cd_NumDataBlks - kHighWaterBufferZone;

	InitDataSpace(cd);
	InitSubcodeSpace(cd);

	return (kCmdComplete);
}

/*******************************************************************************
* SendCmdWithWaitState()                                                       *
*                                                                              *
*      Purpose: Provides the mini state machine needed to walk commands thru   *
*               the send-command, wait-for-command-tag, wait-for-state process.*
*                                                                              *
*   Parameters: cd         - ptr to the cdrom device in question               *
*               cmd        - address of byte string containing command to send *
*               cmdLen     - length of command to send                         *
*               desiredCIP - the state to wait for (CIPNone if you only want   *
*                            to wait for the command tag)                      *
*               errCode    - address to hold any errored CIP state in case of  *
*                            dev error                                         *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: StopPrefetching(), RecoverFromDipir(), OpenDrawer(),           *
*               CloseDrawer(), InitReports(), BuildDiscData(), Read(),         *
*               ReadSubQ()                                                     *
*                                                                              *
*******************************************************************************/
Err SendCmdWithWaitState(cdrom *cd, uint8 *cmd, uint8 cmdLen, uint8 desiredCIP, uint8 *errCode)
{
	switch (cd->cd_DevState & kCmdStateMask)
	{
		case kSendCmd:
			DBUG(kPrintSendCmdWithWaitState, ("M2CD: kSendCmd\n"));
			DBUG(kPrintSendCmdWithWaitState, ("CIPState: %02x\n",cd->cd_CIPState));
			SendCmdToDev(cd, cmd, cmdLen);
			cd->cd_DevState = (cd->cd_DevState & ~kCmdStateMask) | kWait4Tag;
		case kWait4Tag:
			DBUG(kPrintSendCmdWithWaitState, ("M2CD: kWait4Tag\n"));
			DBUG(kPrintSendCmdWithWaitState, ("CIPState: %02x\n",cd->cd_CIPState));
			/* wait for our cmd tag */
			if (cd->cd_CmdByteReceived != cmd[0])
				break;
			cd->cd_DevState = (cd->cd_DevState & ~kCmdStateMask) | kWait4CIPState;
		case kWait4CIPState:
			DBUG(kPrintSendCmdWithWaitState, ("M2CD: kWait4CIPState\n"));
			DBUG(kPrintSendCmdWithWaitState, ("CIPState: %02x\n",cd->cd_CIPState));
			/* look for possible error */
			if (cd->cd_State & CD_DEVICE_ERROR)
			{
				cd->cd_State &= ~CD_DEVICE_ERROR;
				if (errCode)
					*errCode = cd->cd_CIPState;
				return (kCDErrDeviceError);
			}
			else if ((desiredCIP == CIPNone) || (cd->cd_CIPState == desiredCIP))
			{
				DBUG(kPrintSendCmdWithWaitState, ("M2CD: kCmdComplete\n"));
				return (kCmdComplete);
			}
			break;
	}
	DBUG(kPrintSendCmdWithWaitState, ("M2CD: kCmdNotYetComplete\n"));
	DBUG(kPrintSendCmdWithWaitState, ("CIPState: %02x\n",cd->cd_CIPState));
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* Jump2Dipir()                                                                 *
*                                                                              *
*      Purpose: Upon detecting the drive's Media Access bit set, jumps to      *
*               dipir.                                                         *
*                                                                              *
*******************************************************************************/
void Jump2Dipir(cdrom *cd)
{
	/* get dipir to look at the CD drive */
	CDE_WRITE(gCDEBase, CDE_BBLOCK, CDE_CDROM_BLOCK);

	if (!KB_FIELD(kb_NoReboot))
	{
		DBUG(kPrintDipirStuff, ("M2CD:  Forcing a reboot...\n"));

		/* force a romapp reboot in dipir */
		SuperSetSysInfo(SYSINFO_TAG_KERNELADDRESS, 0, 0);
	}

	cd->cd_State |= (CD_DIPIRING | CD_GOTO_DIPIR);
	TriggerDeviceRescan();

	/* Wait for recovery signal, so that Jump2Dipir() completes
	 * synchronously, as TriggerDeviceRescan() merely sends a signal
	 * to the Operator to begin the Duck/SoftReset/Dipir/Recover process.
	 */
	SuperWaitSignal(cd->cd_RecoverSig);
}

/*******************************************************************************
* SendCmdToDev()                                                               *
*                                                                              *
*      Purpose: Sends the requested command to the LCCD device via xbus.       *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               cmdBuf - pointer to command byte string to send across xbus    *
*               cmdLen - length of command byte string                         *
*                                                                              *
*      Returns: Err - Any error received from SendIO()                         *
*                                                                              *
*    Called by: Various internal action machines upon needing to send a cmd to *
*               the LCCD device.                                               *
*                                                                              *
*******************************************************************************/
Err SendCmdToDev(cdrom *cd, uint8 *cmdBuf, uint8 cmdLen)
{
	uint8	x;
	uint32	cdStatus;
#ifdef DEVELOPMENT
	uint8	hackblk;
#endif

	/* clear previous cmd byte in case we wish to send 2 like cmds in a row */
	cd->cd_CmdByteReceived = 0;

	/* Need to disable CDSFL interrupts around the "Write, Wait-for-xfer-
	 * completion, Clear" for each byte in command string.
	 */
	DisableInterrupt(INT_CDE_CDSFL);

	/* send byte loop */
	for (x = 0; x < cmdLen; x++)
	{
		/* write the byte to CDE */
		CDE_WRITE(gCDEBase, CDE_CD_CMD_WRT, cmdBuf[x]);

		/* wait for completion of serial transfer, CDE -> Babette */
		do {
			cdStatus = CDE_READ(gCDEBase, CDE_INT_STS);
		} while (!(cdStatus & CDE_CD_CMD_WRT_DONE));

		/* manually clear the steenkin bit */
		CDE_CLR(gCDEBase, CDE_INT_STS, CDE_CD_CMD_WRT_DONE);
	}

#ifdef DEVELOPMENT
	DBUG(kPrintSendCmdToDevStuff, ("M2CD:  SendCmd..."));
	for (x = 0; x < cmdLen; x++)
	{
		DBUG(kPrintSendCmdToDevStuff, ("%02x ", cmdBuf[x]));
	}
	DBUG(kPrintSendCmdToDevStuff, ("\n"));

	hackblk = cd->cd_SnarfWriteIndex;
	if (cd->cd_SnarfBuffer[hackblk].dbgdata[0] | cd->cd_SnarfBuffer[hackblk].dbgdata[1] |
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] | cd->cd_SnarfBuffer[hackblk].dbgdata[3])
	{
		gBaddCode[0] = cd->cd_SnarfBuffer[hackblk].dbgdata[0];
		gBaddCode[1] = cd->cd_SnarfBuffer[hackblk].dbgdata[1];
		gBaddCode[2] = cd->cd_SnarfBuffer[hackblk].dbgdata[2];
		gBaddCode[3] = cd->cd_SnarfBuffer[hackblk].dbgdata[3];

		gDeadBabe[0] = *cmdBuf;
		gDeadBabe[1] = cd->cd_DevState;
		gDeadBabe[2] = cd->cd_State;
		gDeadBabe[3] = cd->cd_CIPState;

		cd->cd_SnarfBuffer[hackblk].dbgdata[0] = 0xFFFFFFFF;
		cd->cd_SnarfBuffer[hackblk].dbgdata[1] = 0xBADDC0DE;
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] = 0xDEADBABE;
		cd->cd_SnarfBuffer[hackblk].dbgdata[3] = 0xFFFFFFFF;

		DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Sent back-to-back commands to drive without waiting on cmd tag!\n"));
		DBUG2(kPrintWarnings, ("M2CD:  cmd1:  %08lx %08lx %08lx %08lx\n", gBaddCode[0], gBaddCode[1], gBaddCode[2], gBaddCode[3]));
		DBUG2(kPrintWarnings, ("M2CD:  cmd1:  %08lx %08lx %08lx %08lx\n", gDeadBabe[0], gDeadBabe[1], gDeadBabe[2], gDeadBabe[3]));
	}
	else
	{
		cd->cd_SnarfBuffer[hackblk].dbgdata[0] = *cmdBuf;
		cd->cd_SnarfBuffer[hackblk].dbgdata[1] = cd->cd_DevState;
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] = cd->cd_State;
		cd->cd_SnarfBuffer[hackblk].dbgdata[3] = cd->cd_CIPState;
	}
#endif

	EnableInterrupt(INT_CDE_CDSFL);

#ifdef BUILD_M2CD_MONITOR
	if (cd->cd_MonitorMsgPort)
		MyDrawString(kStrCmdLine, 50, 50);
	for (x = 0; x < cmdLen; x++)
	{
		if (cd->cd_MonitorMsgPort)
			MyDrawString(cmdBuf[x], 100+20*x, 50);
	}
#endif

	return (kCDNoErr);
}

/*******************************************************************************
* SetupSnarfer()                                                               *
*                                                                              *
*      Purpose: Sends a "snarfer" to xbus to retreive any status being         *
*               returned from the LCCD device.  This mechanism is required in  *
*               order to support async. communication with the device          *
*               (enabling the use of Reports, etc.).  This is primarily req.   *
*               due to the architecture of the firmware.                       *
*                                                                              *
*               The receive buffer for the snarfReq is the next FREE 'snarf    *
*               buffer block', indicated by the SnarfWriteIndex.               *
*                                                                              *
*         NOTE: ALL status (from cmds, reports, etc.) must be returned via a   *
*               snarfReq due to the fact that sync/async device communication  *
*               cannot be mixed.                                               *
*                                                                              *
*   Parameters: cd - pointer to cdrom device to snarf status from              *
*                                                                              *
*      Returns: Err - if one occurs when sending the snarfer to xbus           *
*                                                                              *
*    Called by: LCCDNewDevice(), during device initialization                  *
*                                                                              *
*******************************************************************************/
Err SetupSnarfer(cdrom *cd, uint32 init)
{
	uint8	blk;

	DBUG(kPrintGeneralStatusStuff, ("M2CD:  Setting up snarfer for async. status responses\n"));

	if (init)
	{
		cd->cd_SnarfReadIndex = 0;
		cd->cd_SnarfWriteIndex = 0;
		for (blk = 0; blk < MAX_NUM_SNARF_BLKS; blk++)
			cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_FREE;
	}

	blk = cd->cd_SnarfWriteIndex;

	if (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_FREE)
	{
		cd->cd_State &= ~CD_SNARF_OVERRUN;

		/* mark block as in use */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_INUSE;

		/* FIXME:  So do we wanna use SetupSnarfer() to re-enable CDSFL
		 *         interrupts?  Or do we even need SetupSnarfer() at all?
		 */
	}
	else
	{
		DBUG2(kPrintDeath, ("M2CD:  >DEATH<  No snarf buffer available...couldn't setup snarfer.\n"));

		DBUGERR(("SRI = %d\n",cd->cd_SnarfReadIndex));
		DBUGERR(("SWI = %d\n",cd->cd_SnarfWriteIndex));
		DBUGERR(("state = %02x\n",cd->cd_SnarfBuffer[blk].blkhdr.state));
		DBUGERR(("data = %08x\n",*(uint32 *)cd->cd_SnarfBuffer[blk].blkdata));

		return (kCDErrSnarfBufferOverrun);
	}

	return (kCDNoErr);
}

/*******************************************************************************
* CDSFLHandler()                                                               *
*                                                                              *
*      Purpose: Handles CDSFL (CD Status Flush) interrupts.  These occur when  *
*               there is a complete status packet available in the CDE Status  *
*               FIFO.  Preserves the response in the snarf status buffer,      *
*               redirects the handler to the next available snarf buffer       *
*               block, and signals the daemon to process the snarf buf.        *
*                                                                              *
*    Called by: Upon receiving a CDSFL interrupt (when Babette sends a status  *
*               flush to CDE, indicating that a complete status pkt is avail). *
*                                                                              *
*******************************************************************************/
int32 CDSFLHandler(void)
{
	/* must cheat to get our device globals
	 * (Note that this won't work for multiple drives)
	 */
	cdrom	*cd = gDevice;
	uint8	blk = cd->cd_SnarfWriteIndex;				/* which buffer are we currently snarfing into? */
	uint32	cdStatus;
	uint8	statSize = 0;

	/* read the complete status chunk in status fifo */
	cdStatus = CDE_READ(gCDEBase, CDE_CD_STS_RD);
	while (cdStatus & CDE_CD_STS_READY)
	{
		cd->cd_SnarfBuffer[blk].blkdata[statSize++] = ((uint8)cdStatus & 0xFF);
		cdStatus = CDE_READ(gCDEBase, CDE_CD_STS_RD);
	}

	/* send StatusFifoEmpty to Babette */
	CDE_WRITE(gCDEBase, CDE_CD_CMD_WRT, kSFEhandshake);

	/* wait for completion of serial transfer, CDE -> Babette */
	cdStatus = CDE_READ(gCDEBase, CDE_INT_STS);
	while (!(cdStatus & CDE_CD_CMD_WRT_DONE))
		cdStatus = CDE_READ(gCDEBase, CDE_INT_STS);

	/* manually clear the steenkin bit */
	CDE_CLR(gCDEBase, CDE_INT_STS, CDE_CD_CMD_WRT_DONE);
	ClearInterrupt(INT_CDE_CDSFL);

	/* update current snarfBuffer block header, mark as complete, etc. */
	cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_VALID;
	cd->cd_SnarfBuffer[blk].blkhdr.size = statSize;

#ifdef DEVELOPMENT
	{
		uint8	x;
		for (x = cd->cd_SnarfBuffer[blk].blkhdr.size; x < 12; x++)
			cd->cd_SnarfBuffer[blk].blkdata[x] = 0x00;

		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr &= 0xFFFF0000;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr |= (0x0000FFFF & cd->cd_DevState);
	}
#endif

	/* point to 'next' snarf buffer block */
	blk = cd->cd_SnarfWriteIndex = (blk == (MAX_NUM_SNARF_BLKS-1)) ? 0 : (blk + 1);

	/* is the next buffer block free? */
	if (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_FREE)
	{
		/* if so, mark it as now being in-use; and re-direct snarfReq's buf */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_INUSE;

#ifdef DEVELOPMENT
		/* write over old data (provides a "sync mark" to indicate where
		 * we're currently writing to)
		 */
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr |= 0x00FFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[0] = 0xFFFFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[4] = 0xFFFFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[8] = 0xFFFFFFFF;
		cd->cd_SnarfBuffer[blk].dbgdata[0] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[1] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[2] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[3] = 0x00000000;
#endif

		/* update number of snarfers in use (and max needed) */
		gNumSnarfBlksInUse++;
	}
	else
	{
		cd->cd_State |= CD_SNARF_OVERRUN;				/* uh oh!...crap out! */
	}

	/* notify daemon of newly received response */
	if (cd->cd_DaemonTask)
		SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

	return (0);
}

/*******************************************************************************
* ProcessSnarfBuffers()                                                        *
*                                                                              *
*      Purpose: Parses the status returned in the snarf buffer blocks, dealing *
*               with each status appropriately, and then makes the snarf       *
*               buffer block available for re-use by a snarfReq.               *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon being signaled by SnarfCallBack().    *
*                                                                              *
*******************************************************************************/
int32 ProcessSnarfBuffers(cdrom *cd)
{
	uint8	blk = cd->cd_SnarfReadIndex;
	uint8	x, len;
	uint8	*blockData;
	int32	ret = 0;

	/* process all valid snarfed blocks */
	while (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_VALID)
	{
		/* how much data was returned in this block?  and were is it located? */
		len = cd->cd_SnarfBuffer[blk].blkhdr.size;
		blockData = cd->cd_SnarfBuffer[blk].blkdata;

		for (x = 0; x < len; )
		{
			MonitorSnarfBuffer(cd, blockData, x, len);

			switch (blockData[x])
			{
				case CDE_DIPIR_BYTE:
				case CD_LED:
				case CD_SETSPEED:
				case CD_SPINDOWNTIME:
				case CD_SECTORFORMAT:
				case CD_CIPREPORTEN:
				case CD_QREPORTEN:
				case CD_SWREPORTEN:
				case CD_SENDBYTES:
				case CD_PPSO:
				case CD_SEEK:
					DBUG(kPrintStatusResponse, ("M2CD:  %02x\n", blockData[x]));

					/* save last received cmd tag byte */
					cd->cd_CmdByteReceived = blockData[x];

					x += 1;					/* only cmd tag byte is returned */
					break;
				case CD_CHECKWO:
					DBUG(kPrintStatusResponse, ("M2CD:  %02x\n", blockData[x]));

					/* save last received cmd tag byte */
					cd->cd_CmdByteReceived = blockData[x];

					cd->cd_WobbleInfo.LowKHzEnergy = (blockData[x+1] << 16) |
						(blockData[x+2] << 8) | (blockData[x+3]);
					cd->cd_WobbleInfo.HighKHzEnergy = (blockData[x+4] << 16) |
						(blockData[x+5] << 8) | (blockData[x+6]);
					cd->cd_WobbleInfo.RatioWhole = blockData[x+7];
					cd->cd_WobbleInfo.RatioFraction = blockData[x+8];

					x += 9;					/* CheckWO is 9 bytes long */
					break;
				case CD_MECHTYPE:
					DBUG(kPrintStatusResponse, ("M2CD:  %02x %02x %02x %02x ",
						 blockData[x], blockData[x+1], blockData[x+2], blockData[x+3]));
					DBUG(kPrintStatusResponse, ("M2CD:  %02x %02x %02x %02x\n",
						 blockData[x+4], blockData[x+5], blockData[x+6], blockData[x+7]));

					/* save last received cmd tag byte */
					cd->cd_CmdByteReceived = blockData[x];

					/* is it a drawer or clamshell mechanism? */
					if (blockData[x+1] == kDrawerMechanism)
						cd->cd_State |= CD_DRAWER_MECHANISM;

					DBUG(kPrintClamshell, ("M2CD:  MechType=%02x\n", blockData[x+1]));

					/* does the drive support SCAN_READ seeking? */
					if (blockData[x+2] & kDFScanSeek)
						cd->cd_DriveFunc |= CD_SUPPORTS_SCANNING;

					/* does the drive support 4x, 6x, or 8x speeds? */
					if (blockData[x+2] & kDF4xMode)
					{
						cd->cd_DefaultOptions.asFields.speed = CDROM_4X_SPEED;
						cd->cd_DriveFunc |= CD_SUPPORTS_4X_MODE;
					}
					if (blockData[x+2] & kDF6xMode)
					{
						cd->cd_DefaultOptions.asFields.speed = CDROM_6X_SPEED;
						cd->cd_DriveFunc |= CD_SUPPORTS_6X_MODE;
					}
					if (blockData[x+2] & kDF8xMode)
					{
						cd->cd_DefaultOptions.asFields.speed = CDROM_8X_SPEED;
						cd->cd_DriveFunc |= CD_SUPPORTS_8X_MODE;
					}

					x += 8;					/* MechType cmd returns 8 bytes */
					break;
				case CD_CIPREPORT:
					DBUG(kPrintStatusResponse, ("M2CD:  %02x %02x\n", blockData[x], blockData[x+1]));

					/* save latest known device CIP state */
					cd->cd_CIPState = blockData[x+1];

					if ((cd->cd_CIPState == CIPStop) || (cd->cd_CIPState == CIPStopAndFocused))
					{
						/* clear this bit in case of auto spin-down */
						cd->cd_Status &= ~CDROM_STATUS_SPIN_UP;
						DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));
					}

					switch (cd->cd_CIPState)
					{
						case CIPStuck:
							if (!(cd->cd_State & CD_DRAWER_MECHANISM))
							{
								DBUG2(kPrintWarnings, ("M2CD:  Someone tried to swap discs in the clamshell drive!\n"));
								cd->cd_Status &= ~(CDROM_STATUS_DISC_IN |
									CDROM_STATUS_SPIN_UP |
									CDROM_STATUS_DOUBLE_SPEED |
									CDROM_STATUS_READY);
								TakeDeviceOffline(cd);
								AbortCurrentIOReqs(cd);
							}
						case CIPFocusError:
						case CIPUnreadable:
						case CIPSeekFailure:
							cd->cd_State |= CD_DEVICE_ERROR;
							DBUG2(kPrintDeath, ("M2CD:  >DEATH<  Got error CIP state (%02x)\n", cd->cd_CIPState));
							break;
					}

					x += 2;				/* CIPReport response is 2 bytes long */
					break;
				case CD_QREPORT:
					/* indicate valid Qcode being returned */
					cd->cd_LastQCode.validByte = 0x80;

					/* cache latest Qcode and swap the nybble so it matches
					 * the MKE drive.
					 */
					cd->cd_LastQCode.addressAndControl = SwapNybble(blockData[x+1]);
					memcpy(&cd->cd_LastQCode.trackNumber, &blockData[x+2], sizeof(SubQInfo)-3);

					/* if we're still reading the TOC... */
					if (cd->cd_State & CD_READING_TOC_INFO)
						ParseTOCEntries(cd, blockData, x);

					x += 11;			/* QReport response is 11 bytes long */
					break;
				case CD_SWREPORT:
					DBUG(kPrintStatusResponse, ("M2CD:  %02x %02x %02x\n", blockData[x], blockData[x+1], blockData[x+2]));

					HandleSwReport(cd, blockData, x);
					ret = 1;

					x += 3;				/* SwReport response is 3 bytes long */
					break;
				case CD_READID:
#if 0
					DBUG(kPrintStatusResponse, ("M2CD:  %02x %02x %02x\n", blockData[x], blockData[x+1], blockData[x+2]));
					x += 11;
#endif
				default:
					DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Unknown status response received!\n"));
					DBUG2(kPrintWarnings, ("M2CD:  "));
					for (; x < len; x++)
					{
						DBUG2(kPrintWarnings, ("%02x ",blockData[x]));
					}
					DBUG2(kPrintWarnings, ("\n"));
					break;
			}
		}

		/* mark block as now available for use */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_FREE;

		/* update number of snarfers in use */
		gNumSnarfBlksInUse--;

		/* point to 'next' snarf buffer block */
		blk = cd->cd_SnarfReadIndex = (blk == (MAX_NUM_SNARF_BLKS-1)) ? 0 : (blk + 1);
	}
	return ret;
}

/*******************************************************************************
* HandleSwReport()                                                             *
*                                                                              *
*      Purpose: Handles SwReports.  Saves the latest (and previous) switch     *
*               states in device globals.  Determines if user switch was       *
*               pressed or if someone actually pressed on the drawer itself,   *
*               and initiates appropriate action to open/close the drawer.     *
*               Also updates status byte.                                      *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where SwReport status begins   *
*                                                                              *
*    Called by: ProcessSnarfBuffers(), upon parsing a SwReport.                *
*                                                                              *
*******************************************************************************/
void HandleSwReport(cdrom *cd, uint8 *statusBytes, int32 x)
{
	uint8	curState = statusBytes[x+1];
	uint8	delta = statusBytes[x+2];

	DBUG(kPrintStatusResponse, ("M2CD:  CurState: %02x\n", curState));
	DBUG(kPrintStatusResponse, ("M2CD:    Change: %02x\n", delta));

	if (curState & CD_DOOR_CLOSED_SWITCH)
	{
		cd->cd_Status |= CDROM_STATUS_DOOR;				/* door is now closed */
		DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));

		if (delta & CD_DOOR_OPEN_SWITCH)
		{
			DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Got spurious OPEN SwRpt\n"));
			return;
		}

		cd->cd_State &= ~kSwitchMask;
		cd->cd_State |= (uint32)(curState & kSwitchMask);	/* only copy low nibble */

	}
	else
	{
		cd->cd_Status &= ~(CDROM_STATUS_DOOR | CDROM_STATUS_DISC_IN |
			CDROM_STATUS_SPIN_UP | CDROM_STATUS_8X_SPEED |
			CDROM_STATUS_6X_SPEED | CDROM_STATUS_4X_SPEED |
			CDROM_STATUS_DOUBLE_SPEED | CDROM_STATUS_READY);
		DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));

		TakeDeviceOffline(cd);

		/* support needed for clamshell drives */
		if ((delta & CD_DOOR_CLOSED_SWITCH) && !(cd->cd_State & CD_DRAWER_MECHANISM))
		{
			DBUG(kPrintClamshell, ("M2CD:  Clamshell opened!\n"));
			AbortCurrentIOReqs(cd);
			cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
		}

		cd->cd_State &= ~kSwitchMask;
		cd->cd_State |= (uint32)(curState & kSwitchMask);	/* only copy low nibble */


		/* if drawer just opened and kbNR non-zero, don't call "reboot" */
		if (delta & CD_DOOR_CLOSED_SWITCH)
			Jump2Dipir(cd);
	}
}

/*******************************************************************************
* ParseTOCEntries()                                                            *
*                                                                              *
*      Purpose: Interprets Qcode in order to build the Disc, TOC, and Session  *
*               Info.                                                          *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where QReport/Qcode status     *
*                             begins                                           *
*                                                                              *
*    Called by: ProcessSnarfBuffers(), upon receiving a QReport while we're    *
*               still building the discdata info.                              *
*                                                                              *
*******************************************************************************/
void ParseTOCEntries(cdrom *cd, uint8 *statusBytes, int32 x)
{
	uint8 adrctl = statusBytes[x+1];
	uint8 tracknum = statusBytes[x+2];
	uint8 point = statusBytes[x+3];
	uint8 min = statusBytes[x+4];
	uint8 sec = statusBytes[x+5];
	uint8 frame = statusBytes[x+6];
	uint8 pmin = statusBytes[x+8];
	uint8 psec = statusBytes[x+9];
	uint8 pframe = statusBytes[x+10];

	uint8	track;
	uint8	GotAllTracks;
	uint32	discInfoMSF;

	DBUG(kPrintGeneralStatusStuff, ("M2CD:  %02x\n", point));

	switch (adrctl & 0x0F)					/* mask out the control nybble */
	{
		case 0x01:							/* adr mode 1  (track time info) */
			/* if we're in the lead-out area (just prior to the next TOC) */
			if (tracknum == 0xAA)
				break;

			/* make sure that we've gotten an entry for A0, A1, and A2 */
			if (cd->cd_BuildingTOC == (TOC_GOT_A0 | TOC_GOT_A1 | TOC_GOT_A2))
			{
				GotAllTracks = TRUE;

				/* NOTE: The .firstTrackNumber should not ever change
				 * ...but the .lastTrackNumber can change for MULTISESSION discs
				 */
				if (!(cd->cd_State & CD_DISC_IS_CDI) ||
					(cd->cd_State & CD_DISC_IS_CDI_WITH_AUDIO))
				{
					for (track = cd->cd_DiscInfo.firstTrackNumber;
						(track <= cd->cd_DiscInfo.lastTrackNumber);
						track++)
					{
						if (!cd->cd_TOC_Entry[track].trackNumber)
						{
							GotAllTracks = FALSE;
							break;		/* break out of existence-test loop */
						}
					}
				}

				/* done reading toc (we've started to repeat entries) */
				if (GotAllTracks)
				{
					cd->cd_State &= ~CD_READING_TOC_INFO;
					cd->cd_State |= CD_GOT_ALL_TRACKS;

					/* the DiscInfo.MSF can change for MULTISESSION discs */
					cd->cd_MediumBlockCount = ((uint32)cd->cd_DiscInfo.minutes * 60L +
											  (uint32)cd->cd_DiscInfo.seconds) * 75L +
											  (uint32)cd->cd_DiscInfo.frames;
				}
			}

			if ((cd->cd_State & CD_READING_TOC_INFO) && !(cd->cd_State & CD_GOT_ALL_TRACKS))
			{
				/* NOTE:  For CD-i discs (discID = 0x10), the TOC is
				 * constructed slightly differently.  The PMIN(A0) entry is
				 * the first AUDIO track# (if the disc contains audio tracks);
				 * and PMIN(A1) is the last audio track#.  The AdrCtrl field
				 * of A1 will have it's 0x04 bit clear if the disc contains
				 * audio tracks; and therefore, audio track TOC entries.  The
				 * CD-i tracks do NOT have TOC entries.
				 */
				switch (point)
				{
					case 0xA0:
						/* PMIN(A0) = first track number (BCD) */
						/* PSEC(A0) = DiscID (0x00, 0x10, 0x20) */
						if (cd->cd_State & CD_READING_INITIAL_TOC)
						{
							cd->cd_DiscInfo.discID = psec;

							if (psec == CDROM_DISC_CDI)
							{
								DBUG(kPrintGeneralStatusStuff, ("M2CD:  Disc is CD-i\n"));
								cd->cd_DiscInfo.firstTrackNumber = 1;
								cd->cd_State |= CD_DISC_IS_CDI;

								/* initialize TOC entries for CD-i tracks as
								 * they have no "standard" entries.
								 */
								for (track = 1; track < BCD2BIN(pmin); track++)
								{
									/* mark it as a data track at 00:02:00 */
									cd->cd_TOC_Entry[track].addressAndControl = 0x14;
									cd->cd_TOC_Entry[track].trackNumber = track;
									cd->cd_TOC_Entry[track].minutes = 0x00;
									cd->cd_TOC_Entry[track].seconds = 0x02;
									cd->cd_TOC_Entry[track].frames = 0x00;
								}
							}
							else
								cd->cd_DiscInfo.firstTrackNumber = BCD2BIN(pmin);
						}
						else
							cd->cd_NextSessionTrack = BCD2BIN(pmin);
						cd->cd_BuildingTOC |= TOC_GOT_A0;
						break;
					case 0xA1:
						/* make sure that we've already received an A0 entry
						 * so we know the proper context (WRT CD-i, etc.)
						 */
						if (!(cd->cd_BuildingTOC & TOC_GOT_A0))
							break;

						if (cd->cd_State & CD_DISC_IS_CDI)
						{
							/* Note that the context of adrctl here
					 	 	* assumes that the nybbles are NOT swapped.
					 	 	*/
							if (!(adrctl & 0x40))
							{
    							cd->cd_State |= CD_DISC_IS_CDI_WITH_AUDIO;
								DBUG2(kPrintWarnings, ("M2CD:  Disc is CD-i with audio\n"));
							}
						}

						if (!(cd->cd_State & CD_DISC_IS_CDI) ||
							(cd->cd_State & CD_DISC_IS_CDI_WITH_AUDIO))
						{
							/* PMIN = last track number (BCD) */
							cd->cd_DiscInfo.lastTrackNumber = BCD2BIN(pmin);
						}
						else
						{
							/* PMIN = number of CD-i tracks plus 1 */
							cd->cd_DiscInfo.lastTrackNumber = BCD2BIN(pmin) - 1;
						}
						cd->cd_BuildingTOC |= TOC_GOT_A1;
						break;
					case 0xA2:
						/* PMIN, PSEC, PFRM = MSF of lead-out area (BCD) */

						/* "minus 1" to match the MKE spec (ie, MKE gives the
						 * MSF of the last _valid_ sector)
						 */
						discInfoMSF = Offset2BCDMSF(BCDMSF2Offset(((uint32)pmin << 16) | ((uint32)psec << 8) | pframe) - 1);
						cd->cd_DiscInfo.minutes = BCD2BIN((uint8)((discInfoMSF >> 16) & 0xFF));
						cd->cd_DiscInfo.seconds = BCD2BIN((uint8)((discInfoMSF >> 8) & 0xFF));
						cd->cd_DiscInfo.frames = BCD2BIN((uint8)(discInfoMSF & 0xFF));

						/* support for CD+ discs.  in order to correctly calculate the
						 * length of the last audio track, they need to know the end of
						 * the first session.
						 */
						if (cd->cd_State & CD_READING_INITIAL_TOC)
						{
							cd->cd_FirstSessionInfo.minutes = cd->cd_DiscInfo.minutes;
							cd->cd_FirstSessionInfo.seconds = cd->cd_DiscInfo.seconds;
							cd->cd_FirstSessionInfo.frames = cd->cd_DiscInfo.frames;
						}

						cd->cd_BuildingTOC |= TOC_GOT_A2;
						break;
					default:
						/* hmmm...it must be a TOC entry for a track */
						point = BCD2BIN(point);

						/* must be swapped to match current MKE spec */
						cd->cd_TOC_Entry[point].addressAndControl = SwapNybble(adrctl);
						cd->cd_TOC_Entry[point].trackNumber = point;
						cd->cd_TOC_Entry[point].minutes = BCD2BIN(pmin);
						cd->cd_TOC_Entry[point].seconds = BCD2BIN(psec);
						cd->cd_TOC_Entry[point].frames = BCD2BIN(pframe);
						break;
				}
			}
			break;
		case 0x05:							/* adr mode 5  (Hybrid/MultiSess) */
			/* only accept one mode 5, point 0xB0 per TOC */
			if ((point == 0xB0) && !(cd->cd_State & CD_READ_NEXT_SESSION))
			{
				/* indicate that we need to read the next session's TOC */
				cd->cd_State |= CD_READ_NEXT_SESSION;

				/* back up 01:00:00 from the MSF in the 05-entry (skip
				 * ahead 1 sec to prevent f/w from landing in latency)
				 */
				cd->cd_TOC = BCDMSF2Offset(((uint32)min << 16) | ((uint32)sec << 8) | (uint32)frame) - BCDMSF2Offset(0x005900);
			}
			break;
		default:
			DBUG(kPrintGeneralStatusStuff, ("M2CD:  Unknown Qcode ADR! (CtlAdr = %02x)\n", adrctl));
			break;
	}
}

/*******************************************************************************
* PrepareDuckAndCover()                                                        *
*                                                                              *
*      Purpose: Sends ioReq to notify xbus that we wish to be notified when    *
*               some (any) device was just dipired.  This notification is      *
*               necessary because we have been playing (prefetching) when the  *
*               dipir of another device took place; and we need this hook to   *
*               be able to call RecoverFromDipir().                            *
*                                                                              *
*   Parameters: cd - pointer to cdrom device struct                            *
*                                                                              *
*      Returns: Err - if one occurs when sending ioReq to xbus                 *
*                                                                              *
*    Called by: LCCDNewDevice(), during device initialization                  *
*                                                                              *
*******************************************************************************/
Err PrepareDuckAndCover(cdrom *cd)
{
	Err	err;

	err = RegisterDuck((void *)Ducky, (uint32)cd);
	if (err >= 0)
		err = RegisterRecover((void *)Recovery, (uint32)cd);

	return (err);
}

/*******************************************************************************
* Ducky()                                                                      *
*                                                                              *
*      Purpose: Signal the daemon to enter the duck state machine (in order to *
*               pause the drive).  This routine waits on a signal to be sent   *
*               back from the daemon indicating that the drive is paused.      *
*                                                                              *
*   Parameters: cd - pointer to the drivers globals                            *
*                                                                              *
*******************************************************************************/
uint32 Ducky(cdrom *cd)
{
	DBUG(kPrintDipirStuff, ("M2CD:  Ducky()\n"));


	/* Only do this if the media access was caused by a device other than
	 * the drive.  We also test the door closed switch because we don't need
	 * to pause the drive if it's open...this also prevents a lock-up
	 * condition with Jump2Dipir().
	 */
	if ((cd->cd_DevState != CL_DIPIR) &&
		(cd->cd_State & CD_DEVICE_ONLINE))
	{
		cd->cd_SavedDevState = cd->cd_DevState;
		cd->cd_DevState = DS_DUCK_INTO_DIPIR;
		cd->cd_DuckTask = CURRENTTASK;
		cd->cd_DuckSig = SuperAllocSignal(0L);

		DBUG(kPrintDipirStuff, ("M2CD:  Signalling daemon\n"));

		/* Wake up the daemon in it's new state, and pause the drive */
		SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

		DBUG(kPrintDipirStuff, ("M2CD:  Waiting for drive to pause...\n"));

		/* Ta-dahhh, drive is paused */
		SuperWaitSignal(cd->cd_DuckSig);
		SuperFreeSignal(cd->cd_DuckSig);

		DBUG(kPrintDipirStuff, ("M2CD:  Drive paused.\n"));
	}

	cd->cd_State |= CD_DIPIRING;
	return (0);
}

/*******************************************************************************
* Recovery()                                                                   *
*                                                                              *
*      Purpose: Notify daemon that we've returned from dipir; so it's ok to    *
*               proceed with psuedo-restart code:  InitReports(),              *
*               BuildDiscData(), etc. (in the case that it was our device that *
*               was dipired); or to call RecoverFromDipir() (in the case that  *
*               it was another device).                                        *
*                                                                              *
*   Parameters: theReq - pointer to the incoming dipirReq                      *
*                                                                              *
*      Returns: IOReq - This automagically resends the recoverReq to xbus.     *
*                                                                              *
*    Called by: OS, just after dipir.                                          *
*                                                                              *
*******************************************************************************/
uint32 Recovery(cdrom *cd)
{
	DBUG(kPrintDipirStuff, ("M2CD:  Recovery()\n"));

	/* Dipiring is now done. */
	cd->cd_State &= ~CD_DIPIRING;

	/* notify daemon of newly received response */
	if (cd->cd_DaemonTask)
	{
		SuperInternalSignal(cd->cd_DaemonTask, cd->cd_RecoverSig);
		SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);
	}

	return (0);
}

/*******************************************************************************
* InitDataSpace()                                                              *
*                                                                              *
*      Purpose: Initializes data prefetch space (actually just the headers     *
*               associated with the data space).                               *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*    Called by: ResizeLinkedBuffers(), RecoverFromDipir(), and Read().         *
*                                                                              *
*******************************************************************************/
void InitDataSpace(cdrom *cd)
{
	int32 x;

	/* initialize data buffer & indeces */
	cd->cd_DataReadIndex = 0;
	cd->cd_CurDataWriteIndex = 0;
	cd->cd_NextDataWriteIndex = 1;
	for (x = 0; x < cd->cd_NumDataBlks; x++)
	{
		cd->cd_DataBlkHdr[x].state = BUFFER_BLK_FREE;
		cd->cd_DataBlkHdr[x].format = INVALID_SECTOR;
		cd->cd_DataBlkHdr[x].MSF = 0;
	}
}

/*******************************************************************************
* InitSubcodeSpace()                                                           *
*                                                                              *
*      Purpose: Initializes subcode prefetch space (actually just the headers  *
*               associated with the subcode space).                            *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*    Called by: ResizeLinkedBuffers(), RecoverFromDipir(), and Read().         *
*                                                                              *
*******************************************************************************/
void InitSubcodeSpace(cdrom *cd)
{
	int32 x;

	/* initialize subcode buffer & indeces */
	cd->cd_SubcodeReadIndex = 0;
	cd->cd_CurSubcodeWriteIndex = 0;
	cd->cd_NextSubcodeWriteIndex = 1;
	for (x = 0; x < cd->cd_NumSubcodeBlks; x++)
		cd->cd_SubcodeBlkHdr[x].state = BUFFER_BLK_FREE;
}

/*******************************************************************************
* InitPrefetchEngine()                                                         *
*                                                                              *
*      Purpose: Initializes the Prefetch and Subcode Engine(s) by marking the  *
*               first two buffer blocks (cur & next) as "in use", then         *
*               configuring the DMA registers to point to those buffer blocks, *
*               and then enabling the interrupts for channels 0 and 1.         *
*               Channel 1 will only be enabled if we have been requested to    *
*               return the subcode info with the sector data.                  *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: Read() and RecoverFromDipir().                                 *
*                                                                              *
*******************************************************************************/
void InitPrefetchEngine(cdrom *cd)
{
	uint8	curBlk = cd->cd_CurDataWriteIndex;
	uint8	nextBlk = cd->cd_NextDataWriteIndex;

	/* at this point, nextBlk = (curBlk + 1) */
	cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_INUSE;
	cd->cd_DataBlkHdr[nextBlk].state = BUFFER_BLK_INUSE;

	/* reset the dma channels */
	CDE_SET(gCDEBase, CDE_CD_DMA1_CNTL, CDE_DMA_RESET);
	CDE_CLR(gCDEBase, CDE_CD_DMA1_CNTL, 0x7FF);
	CDE_SET(gCDEBase, CDE_CD_DMA2_CNTL, CDE_DMA_RESET);
	CDE_CLR(gCDEBase, CDE_CD_DMA2_CNTL, 0x7FF);

	/* if we're reading an audio sector (and subcode was requested) */
	if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
	{
		uint8	curSubBlk = cd->cd_CurSubcodeWriteIndex;
		uint8	nextSubBlk = cd->cd_NextSubcodeWriteIndex;

		/* at this point, nextBlk = (curBlk + 1) */
		cd->cd_SubcodeBlkHdr[curSubBlk*4].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(curSubBlk*4)+1].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(curSubBlk*4)+2].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(curSubBlk*4)+3].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[nextSubBlk*4].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(nextSubBlk*4)+1].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(nextSubBlk*4)+2].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(nextSubBlk*4)+3].state = BUFFER_BLK_INUSE;

		ConfigLCCDDMA(DMA_CH0, CUR_DMA, cd->cd_DataBlkHdr[curBlk].buffer, cd->cd_BlockLength, FALSE);
		ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength, FALSE);

		/* note that we use lengths of 392 because the DMA h/w requires
		 * lengths/ptrs to be multiples of 8...so we cram 4 logical buffer
		 * blocks (98 bytes each) into 1 physical block.
		 */
		ConfigLCCDDMA(DMA_CH1, CUR_DMA, cd->cd_SubcodeBuffer[curSubBlk], 392, FALSE);
		ConfigLCCDDMA(DMA_CH1, NEXT_DMA, cd->cd_SubcodeBuffer[nextSubBlk], 392, FALSE);

		/* Enable cur/next DMA for channels 0 and 1 */
		EnableLCCDDMA(DMA_CH0 | DMA_CH1);
	}
	else
	{
		ConfigLCCDDMA(DMA_CH0, CUR_DMA, cd->cd_DataBlkHdr[curBlk].buffer, cd->cd_BlockLength, FALSE);
		ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength, FALSE);

		/* Enable cur/next DMA for (data) channel 0 only */
		EnableLCCDDMA(DMA_CH0);
	}
}

/*******************************************************************************
* PrepareCachedInfo()                                                          *
*                                                                              *
*      Purpose: Prepares the initial value of the StatusByte.  Also takes the  *
*               device offline (clearing the TOC info, too), if appropriate.   *
*               Sets the flag CD_CACHED_INFO_AVAIL to indicate that we at      *
*               least tried to read the TOC and allows CmdStatus() and         *
*               DiscData() to complete.   However, DiscData() will return all  *
*               zeros if the device is offline...as desired.                   *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon() and BuildDiscData().                        *
*                                                                              *
*******************************************************************************/
void PrepareCachedInfo(cdrom *cd)
{
	if (cd->cd_State & CD_DOOR_CLOSED_SWITCH)	/* is the drive door closed? */
	{
		/* door closed */
		cd->cd_Status |= CDROM_STATUS_DOOR;
	}
	else
	{
		/* door open */
		cd->cd_Status &= ~(CDROM_STATUS_DOOR | CDROM_STATUS_DISC_IN |
			CDROM_STATUS_SPIN_UP | CDROM_STATUS_8X_SPEED |
			CDROM_STATUS_6X_SPEED | CDROM_STATUS_4X_SPEED |
			CDROM_STATUS_DOUBLE_SPEED | CDROM_STATUS_READY);

		TakeDeviceOffline(cd);
		goto done;
	}

	if (cd->cd_State & CD_NO_DISC_PRESENT)
	{
		/* drive "ready", but no disc present */
		cd->cd_Status |= CDROM_STATUS_READY;
		cd->cd_Status &= ~(CDROM_STATUS_DISC_IN | CDROM_STATUS_SPIN_UP |
			CDROM_STATUS_8X_SPEED | CDROM_STATUS_6X_SPEED |
			CDROM_STATUS_4X_SPEED | CDROM_STATUS_DOUBLE_SPEED);

		TakeDeviceOffline(cd);
		goto done;
	}

	if (cd->cd_State & CD_UNREADABLE)
	{
		/* drive not ready...because unreadable disc is present, TOC not read */
		cd->cd_Status &= ~(CDROM_STATUS_READY);
		cd->cd_Status |= CDROM_STATUS_DISC_IN;

		TakeDeviceOffline(cd);
		goto done;
	}

	/* drive ready, readable disc present, TOC read */
	cd->cd_Status |= (CDROM_STATUS_READY | CDROM_STATUS_DISC_IN);

	switch (cd->cd_CIPState)		/* what state is the drive currently in? */
	{
		case CIPPause:
		case CIPPlay:
		case CIPSeeking:
		case CIPSpinningUp:
		case CIPLatency:
			cd->cd_Status |= CDROM_STATUS_SPIN_UP;		/* disc is spinning */
			break;
		default:
			cd->cd_Status &= ~CDROM_STATUS_SPIN_UP;		/* disc is stopped */
			break;
	}

done:
	DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));

	/* indicate that STATUS and DISCDATA cmds can now be completed */
	cd->cd_State |= CD_CACHED_INFO_AVAIL;
}

/*******************************************************************************
* TakeDeviceOffline()                                                          *
*                                                                              *
*      Purpose: Takes the device off-line by clearing CD_DEVICE_ONLINE, and    *
*               clears any current Disc, TOC, and Session Info.                *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where QReport/Qcode status     *
*                             begins                                           *
*                                                                              *
*    Called by: PrepareCachedInfo() and HandleSwReport().                      *
*                                                                              *
*******************************************************************************/
void TakeDeviceOffline(cdrom *cd)
{
	/* take device "off-line" */
	cd->cd_State &= ~CD_DEVICE_ONLINE;

	/* clear disc, TOC, and session info */
	memset(&cd->cd_DiscInfo, 0, sizeof(CDDiscInfo));
	memset(cd->cd_TOC_Entry, 0, sizeof(CDTOCInfo));
	memset(&cd->cd_SessionInfo, 0, sizeof(CDSessionInfo));
}

/*******************************************************************************
* AbortCurrentIOReqs()                                                         *
*                                                                              *
*      Purpose: Aborts all currently active/pending ioReqs, except for         *
*               CMD_STATUS.                                                    *
*                                                                              *
*   Parameters: cd - pointer to cdrom device structure in question             *
*                                                                              *
*    Called by: LCCDDriverDaemon() and HandleSwReport().                       *
*                                                                              *
*******************************************************************************/
void AbortCurrentIOReqs(cdrom *cd)
{
	List	tmp;
	uint32	interrupts;
	IOReq	*theNode, *theNextNode;

	/* abort any workingIOR */
	if (cd->cd_workingIOR)
		CompleteWorkingIOReq(cd->cd_workingIOR, kCDErrAborted);

	InitList(&tmp,"temporary list");

	/* abort all ioReqs except CMD_STATUS */
	interrupts = Disable();
	for(theNode = (IOReq *)FirstNode(&cd->cd_pendingIORs);
		IsNode(&cd->cd_pendingIORs, theNode);
		theNode = theNextNode)
	{
		theNextNode = (IOReq *)NextNode(theNode);
		if (theNode->io_Info.ioi_Command != CMD_STATUS)
		{
			RemNode((Node *)theNode);		/* remove ioReq from pending list */
			AddTail(&tmp, (Node *)theNode);	/* ...add it to temporary list */
		}
	}
	Enable(interrupts);

	/* abort items on temporary list */
	while (!IsEmptyList(&tmp))
		CompleteWorkingIOReq((IOReq *)RemHead(&tmp), kCDErrAborted);
}

/*******************************************************************************
* CompleteWorkingIOReq()                                                       *
*                                                                              *
*      Purpose: Completes the currently working ioReq; allowing the daemon to  *
*               process the next pending ioReq.                                *
*                                                                              *
*   Parameters: theReq - pointer to ioRequest to complete                      *
*               theErr - any error to return in io_Error                       *
*                                                                              *
*      Returns: none                                                           *
*                                                                              *
*    Called by: OS, upon receiving a SendIO() to this driver                   *
*                                                                              *
*******************************************************************************/
void CompleteWorkingIOReq(IOReq *theReq, Err theErr)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev->dev_DriverData;

	DBUG(kPrintSendCompleteIO, ("M2CD:  CompleteIO(%02x, err:%0ld)\n", theReq->io_Info.ioi_Command, theErr));

	if (theErr < kCDNoErr)
	{
		DBUG2(kPrintWarnings, ("M2CD:  CompleteIO(%02x, err:%0ld)\n", theReq->io_Info.ioi_Command, theErr));
		theReq->io_Error = theErr;					/* update for any error */
	}

	SuperCompleteIO(theReq);						/* complete the request */

	if (theReq == cd->cd_workingIOR)
		cd->cd_workingIOR = NULL;					/* not working any longer */
}

/*******************************************************************************
* OpenDrawer()                                                                 *
*                                                                              *
*      Purpose: Provides the state machine (ha!) needed to open the drawer.    *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_OPEN_DRAWER; or ProcessClientIOReqs(), upon receiving a     *
*               CDROM_OPEN_DRAWER command.                                     *
*                                                                              *
*******************************************************************************/
Err OpenDrawer(cdrom *cd)
{
	const uint8 ppsO[] = { CD_PPSO, 0x00 };			/* open the drawer */

	return (SendCmdWithWaitState(cd, ppsO, sizeof(ppsO), CIPOpen, NULL));
}

/*******************************************************************************
* InitReports()                                                                *
*                                                                              *
*      Purpose: Provides the state machine needed to enable all reports as     *
*               needed for proper driver/user interaction.                     *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_INIT_REPORTS.                                               *
*                                                                              *
*******************************************************************************/
Err InitReports(cdrom *cd)
{
    const uint8 LED[] = { CD_LED, 0x02 };							/* LED is on in focusing, spinning, seeking, latency, and play states */
	const uint8 SwRptEn[] = { CD_SWREPORTEN, 0x02};				/* turn on switch reports (we take control back from the firmware) */
	const uint8 CIPRptEn[] = { CD_CIPREPORTEN, 0x03};				/* turn on ALL CIP reports (f/w automatically sends one NOW) */
	const uint8 QRptDis[] = { CD_QREPORTEN, 0x00 };				/* make sure QReports are disabled */
	const uint8 MechType[] = { CD_MECHTYPE, 0x00, 0x00, 0x00 };	/* check for drawer/clamshell type */
	Err	err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case IR_START:
			DBUG(kPrintActionMachineStates, ("M2CD: IR_START\n"));
			err = SendCmdWithWaitState(cd, LED, sizeof(LED), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_SWEN;
		case IR_SWEN:
			DBUG(kPrintActionMachineStates, ("M2CD: IR_SWEN\n"));
			err = SendCmdWithWaitState(cd, SwRptEn, sizeof(SwRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_QDIS;
		case IR_QDIS:
			DBUG(kPrintActionMachineStates, ("M2CD: IR_QDIS\n"));
			err = SendCmdWithWaitState(cd, QRptDis, sizeof(QRptDis), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_MECH;
		case IR_MECH:
			DBUG(kPrintActionMachineStates, ("M2CD: IR_MECH\n"));
			err = SendCmdWithWaitState(cd, MechType, sizeof(MechType), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_CIPEN;
		case IR_CIPEN:
			DBUG(kPrintActionMachineStates, ("M2CD: IR_CIPEN\n"));
			err = SendCmdWithWaitState(cd, CIPRptEn, sizeof(CIPRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* BuildDiscData()                                                              *
*                                                                              *
*      Purpose: Provides the state machine needed to read the TOC, build the   *
*               disc info, and read the session info.  If an 0x05/0xB0 entry   *
*               is observed in the TOC by ParseTOCEntries(),                   *
*               CD_READ_NEXT_SESSION is set to indicate that this is           *
*               potentially a multisession disc.  We then jump out to the next *
*               TOC should be located, and attempt to read the next session.   *
*               If there is no next session located there, then the firmware   *
*               returns CIPSeekFailure.                                        *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_BUILDING_DISCDATA.                                          *
*                                                                              *
*******************************************************************************/
Err BuildDiscData(cdrom *cd)
{
	const uint8	pPso[] = { CD_PPSO, 0x02 };									/* stop (drive will automatically seek to start of TOC) */
	const uint8	driveSpeed[] = { CD_SETSPEED, CDROM_DOUBLE_SPEED, 0x00 };	/* single/double speed + pitch */
	const uint8	bobFormat[] = { CD_SECTORFORMAT, DA_SECTOR, 0x00 };			/* sector format + subcode enable */
	uint8	seek2toc[] = { CD_SEEK, 0x00, 0x00, 0x00 };					/* seek to 00:00:00 (f/w will automatically start PLAYing) */
	const uint8	Ppso[] = { CD_PPSO, 0x03 };									/* play */
	const uint8	QRptEn[] = { CD_QREPORTEN, 0x02 };							/* turn on QReports */
	const uint8	QRptDis[] = { CD_QREPORTEN, 0x00 };							/* turn off QReports */
	const uint8	seek2pause[] = { CD_SEEK, 0x00, 0x00, 0x96 };				/* seek somewhere in order to pause (seek is supplied with a binary offset) */
	Err		err;
	uint8	errCode;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case BDD_START:
			cd->cd_State &= ~CD_ALREADY_RETRIED_TOC_READ;
retryTOC:
			memset(&cd->cd_DiscInfo, 0, sizeof(CDDiscInfo));
			memset(&cd->cd_TOC_Entry, 0, sizeof(cd->cd_TOC_Entry));
			memset(&cd->cd_SessionInfo, 0, sizeof(CDSessionInfo));

			/* initial TOC starts at 00:00:00 */
			cd->cd_TOC = 0;

			cd->cd_BuildingTOC = FALSE;
			cd->cd_MediumBlockCount = 0;
			cd->cd_NextSessionTrack = 0;			/* no multi-session, yet */
			cd->cd_State &= ~(CD_READING_TOC_INFO | CD_READ_NEXT_SESSION | CD_GOT_ALL_TRACKS);
			cd->cd_State |= CD_READING_INITIAL_TOC;

			/* if dipir left us playing...we need to pause */
			if (cd->cd_CIPState == CIPPlay)
				cd->cd_DevState = BDD_PAUSE;
			else
				goto speedjump;
		case BDD_PAUSE:
			/* pause the drive */
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
speedjump:
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */
			cd->cd_DevState = BDD_SPEED;
		case BDD_SPEED:
			/* read the TOC in double speed */
			err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_Status |= CDROM_STATUS_DOUBLE_SPEED;
			cd->cd_Status &= ~(CDROM_STATUS_4X_SPEED | CDROM_STATUS_6X_SPEED |
				CDROM_STATUS_8X_SPEED);
			DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));

			/* save current settings */
			cd->cd_CurrentSettings.speed = driveSpeed[1];
			cd->cd_CurrentSettings.pitch = CDROM_PITCH_NORMAL;

			cd->cd_DevState = BDD_FORMAT;
		case BDD_FORMAT:
			/* read the TOC in audio mode */
			err = SendCmdWithWaitState(cd, bobFormat, sizeof(bobFormat), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);

			/* save current settings */
			cd->cd_CurrentSettings.format = bobFormat[1];
			cd->cd_CurrentSettings.subcode = bobFormat[2];

			cd->cd_DevState = BDD_SEEK;
multiseek:
			DBUG(kPrintTOCAddrs, ("TOC @ %06lX\n", Offset2BCDMSF(cd->cd_TOC)));
		case BDD_SEEK:
			/* this flag interpreted upon receiving a QRpt */
			cd->cd_State |= CD_READING_TOC_INFO;

			/* seek to beginning of TOC.  NOTE: addr is a binary offset */
			*(uint32 *)seek2toc |= cd->cd_TOC;
			err = SendCmdWithWaitState(cd, seek2toc, sizeof(seek2toc), CIPPause, &errCode);
			if (errCode == CIPSeekFailure)
				goto multidone;
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_PLAY;
		case BDD_PLAY:
			err = SendCmdWithWaitState(cd, Ppso, sizeof(Ppso), CIPPlay, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_QEN;
		case BDD_QEN:
			/* start listening to QReports to build the TOC */
			err = SendCmdWithWaitState(cd, QRptEn, sizeof(QRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_QDIS;
		case BDD_QDIS:
			/* still reading the TOC? */
			if (cd->cd_State & CD_READING_TOC_INFO)
			{
				/* if Trk# != 0, we've read beyond the TOC area */
				if (((cd->cd_LastQCode.addressAndControl & 0xF0) == 0x10) &&
					cd->cd_LastQCode.trackNumber &&
					(cd->cd_LastQCode.trackNumber != 0xAA))
				{
					if (cd->cd_State & CD_ALREADY_RETRIED_TOC_READ)
					{
						DBUG2(kPrintDeath, ("M2CD:  >DEATH<  Unable to read complete TOC...FAILED\n"));

						cd->cd_State |= CD_UNREADABLE;

						/* clear these so tests in BDD_CHK4MULTI will fail */
						cd->cd_State &= ~(CD_READING_TOC_INFO | CD_READ_NEXT_SESSION);
						cd->cd_NextSessionTrack = 0;
					}
					else
					{
						DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Unable to read complete TOC...ATTEMPTING RETRY\n"));
						cd->cd_State |= CD_ALREADY_RETRIED_TOC_READ;
						goto retryTOC;
					}
				}
				else
					break;
			}
			/* stop listening to QReports */
			err = SendCmdWithWaitState(cd, QRptDis, sizeof(QRptDis), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
multidone:
			cd->cd_DevState = BDD_CHK4MULTI;
		case BDD_CHK4MULTI:
			if (cd->cd_State & CD_READ_NEXT_SESSION)
			{
				cd->cd_BuildingTOC = FALSE;

				cd->cd_State &= ~(CD_READ_NEXT_SESSION | CD_READING_INITIAL_TOC | CD_GOT_ALL_TRACKS);
				cd->cd_DevState = BDD_SEEK;
				goto multiseek;
			}

			if (cd->cd_NextSessionTrack)
			{
				cd->cd_SessionInfo.valid = 0x80;	/* session info valid bit */
				cd->cd_SessionInfo.minutes = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].minutes;
				cd->cd_SessionInfo.seconds = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].seconds;
				cd->cd_SessionInfo.frames = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].frames;
			}

			PrepareCachedInfo(cd);
			cd->cd_DevState = BDD_SEEK2PAUSE;
		case BDD_SEEK2PAUSE:
			/* seek out of the TOC to pause */
			err = SendCmdWithWaitState(cd, seek2pause, sizeof(seek2pause), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* DuckIntoDipir()                                                              *
*                                                                              *
*      Purpose: Put the drive in a steady (non-playing) state, just prior to   *
*               dipir.  This is needed in order to prevent DMA flub-ups.       *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - Cmd completion.                                          *
*******************************************************************************/
Err DuckIntoDipir(cdrom *cd)
{
	const uint8 pPso[] = { CD_PPSO, 0x02 };
	Err	err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case DID_START:
			DBUG(kPrintDipirStuff, ("M2CD:  DID_START\n"));
			if (!cd->cd_CmdByteReceived)
			{
				DBUG(kPrintDipirStuff, ("M2CD:  Waiting for command response\n"));
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);
				break;
			}
			cd->cd_DevState = DID_WAIT;
		case DID_WAIT:
			DBUG(kPrintDipirStuff, ("M2CD:  DID_WAIT\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = DID_DONE;
		case DID_DONE:
			DBUG(kPrintDipirStuff, ("M2CD:  DID_DONE\n"));
			SuperInternalSignal(cd->cd_DuckTask, cd->cd_DuckSig);
			SuperWaitSignal(cd->cd_RecoverSig);
			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* RecoverFromDipir()                                                           *
*                                                                              *
*      Purpose: Provides the state machine needed to recover from a            *
*               "recoverable" dipir.  A "recoverable" dipir is one in which    *
*               the system doesn't get rebooted, and the OS re-loaded (such as *
*               with "data discs").                                            *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon().                                            *
*                                                                              *
*******************************************************************************/
Err RecoverFromDipir(cdrom *cd)
{
	const uint8 CIPRptEn[] = { CD_CIPREPORTEN, 0x03 };
	const uint8 SwRptEn[] = { CD_SWREPORTEN, 0x02 };
	IOReq	*currentIOReq = cd->cd_workingIOR;
	Err		err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case RFD_START:
			DBUG(kPrintDipirStuff, ("M2CD:  RFD_START\n"));
			err = SendCmdWithWaitState(cd, SwRptEn, sizeof(SwRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RFD_CIPEN;

		case RFD_CIPEN:
			DBUG(kPrintDipirStuff, ("M2CD:  RFD_CIPEN\n"));
			err = SendCmdWithWaitState(cd, CIPRptEn, sizeof(CIPRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RFD_RECOVER;

		case RFD_RECOVER:
			DBUG(kPrintDipirStuff, ("M2CD:  RFD_RECOVER\n"));
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */

			cd->cd_State &= ~CD_PREFETCH_OVERRUN;		/* reset overrun flag */
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;	/* reset pref. flag */
			cd->cd_State &= ~CD_GONNA_HAVE_TO_STOP;		/* reset the bit */

			if (cd->cd_workingIOR)
			{
				InitDataSpace(cd);
				InitSubcodeSpace(cd);
				InitPrefetchEngine(cd);
			}

			/* if currently working on an IOReq
			 * ...we need to reset io_Actual (we'll retry this ioReq later)
			 */
			if (currentIOReq)
				currentIOReq->io_Actual = 0;

			DBUG(kPrintDipirStuff, ("M2CD:  Dipir recovery completed\n"));

			/* make sure the daemon doesn't go to sleep w/o sending a command
			 * (in the SavedDevState machine)...otherwise we'd never wake up
			 */
			if (cd->cd_DaemonTask)
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

			return (kCmdComplete);
	}

	return (kCmdNotYetComplete);
}

/*******************************************************************************
* StopPrefetching()                                                            *
*                                                                              *
*      Purpose: Provides the state machine needed to disable the Prefetch      *
*               Engine.  We first seek to the 'next sector' (the last sector   *
*               in the prefetch space plus 1) anticipating sequential reads,   *
*               wait for the drive to enter the paused state, then disable     *
*               DMA, and clear CD_PREFETCH_OVERRUN.                            *
*                                                                              *
*               If we are already in the process of opening the drawer or      *
*               restarting a read to go get data from somewhere else on the    *
*               disc, then we do not seek (and pause), we simply disable DMA,  *
*               etc.                                                           *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon seeing that CD_PREFETCH_OVERRUN was   *
*               set by the FIRQ handler.                                       *
*                                                                              *
*******************************************************************************/
Err StopPrefetching(cdrom *cd)
{
	/* seek to next (sequential) sector */
	uint8	seek2next[] = { CD_SEEK, 0x00, 0x00, 0x00 };
	Err		err;

	/* NOTE:  These conditionals are here to prevent the possibility of
	 *        sending this SEEK before we actually got the cmd tag back for a
	 *        just-sent command; or in the case of Read(), we're already in the
	 *        process of trying to PAUSE, so just disable the DMA and return.
	 */
	if (cd->cd_workingIOR)
	{
		if (((cd->cd_SavedDevState & ~kCmdStateMask) == RD_RESTART) ||
			((cd->cd_workingIOR->io_Info.ioi_Command == CDROMCMD_READ_SUBQ) &&
			((cd->cd_SavedDevState & ~kCmdStateMask) == SQ_QNOW)))
		{
			goto killDMA;
		}
	}

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case PE_START:
			DBUG(kPrintActionMachineStates, ("M2CD:  PE_START\n"));
			/* "pre-seek" to what would be the next sequential sector if we
			 * got a sequential read AFTER the prefetch space filled up
			 */
			*(uint32 *)seek2next |= BCDMSF2Offset(cd->cd_PrefetchCurMSF);
			err = SendCmdWithWaitState(cd, seek2next, sizeof(seek2next), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = PE_PAUSE;
		case PE_PAUSE:
			DBUG(kPrintActionMachineStates, ("M2CD:  PE_PAUSE\n"));
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */
killDMA:
			cd->cd_State &= ~CD_PREFETCH_OVERRUN;		/* reset overrun flag */
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;	/* reset pref. flag */
			cd->cd_State &= ~CD_GONNA_HAVE_TO_STOP;		/* reset the bit */

			DBUG(kPrintGeneralStatusStuff, ("M2CD:  Prefetch Engine disabled\n"));

			/* make sure the daemon doesn't go to sleep w/o sending a command
			 * (in the SavedDevState machine)...otherwise we'd never wake up
			 */
			if (cd->cd_DaemonTask)
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

			return (kCmdComplete);
	}

	return (kCmdNotYetComplete);
}

/*******************************************************************************
* Passthru()                                                                   *
*                                                                              *
*      Purpose: Send command byte stream directly to the drive.  This is used  *
*               for firmware diagnostics (see "monitor" and "m2cd" test        *
*               tools).                                                        *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: LCCDDriverDispatch().                                          *
*                                                                              *
*******************************************************************************/
Err Passthru(cdrom *cd)
{
	IOReq   *theReq = cd->cd_workingIOR;
	Err     err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case PT_START:
			DBUG(kPrintActionMachineStates, ("M2CD:  PT_START\n"));
			err = SendCmdWithWaitState(cd, theReq->io_Info.ioi_Send.iob_Buffer,
			theReq->io_Info.ioi_Send.iob_Len, CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = PT_DONE;

		case PT_DONE:
			DBUG(kPrintActionMachineStates, ("M2CD:  PT_DONE\n"));
			return (kCmdComplete);
	}

	return (kCmdNotYetComplete);
}

/*******************************************************************************
* ProcessClientIOReqs()                                                        *
*                                                                              *
*      Purpose: If we're not currently working on an ioReq, pulls the next     *
*               pendingIOR off and dispatches it to the appropriate command    *
*               action machine.  When the command action machine completes, we *
*               call CompleteWorkingIOReq() and signal the daemon so we can    *
*               process any ioReq(s) left pending.                             *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_PROCESS_CLIENT_IOREQS.                                      *
*                                                                              *
*******************************************************************************/
void ProcessClientIOReqs(cdrom *cd)
{
	uint32	interrupts;
	Err		err = FALSE;

	/* if not currently working, get the next ioReq (if any) */
	if (!cd->cd_workingIOR)
	{
		interrupts = Disable();
		cd->cd_workingIOR = (IOReq *)RemHead(&cd->cd_pendingIORs);
		Enable(interrupts);

		/* reset the action machine(s) */
		cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
	}

	/* if currently working, or got new ioReq, deal with it... */
	if (cd->cd_workingIOR)
	{
		/* jump into the appropriate action machines */
		switch (cd->cd_workingIOR->io_Info.ioi_Command)
		{
			case CDROMCMD_SCAN_READ:
				if (!(cd->cd_DriveFunc & CD_SUPPORTS_SCANNING))
				{
					err = kCDErrBadCommand;
					break;
				}
			case CMD_READ:
			case CMD_BLOCKREAD:
			case CDROMCMD_READ:
			case CDROMCMD_READ_SUBQ:
				if (cd->cd_State & CD_DEVICE_ONLINE)
					err = (cd->cd_workingIOR->io_Info.ioi_Command == CDROMCMD_READ_SUBQ) ? ReadSubQ(cd) : Read(cd);
				else
				{
					/* reset the action machine(s) */
					cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
					err = kCDErrDeviceOffline;
				}
				break;
			case CMD_STATUS:				err = CmdStatus(cd, cd->cd_workingIOR);		break;
			case CDROMCMD_DISCDATA:			err = DiscData(cd, cd->cd_workingIOR);		break;
			case CDROMCMD_OPEN_DRAWER:		err = OpenDrawer(cd);						break;
			case CDROMCMD_SETDEFAULTS:		err = SetDefaults(cd);						break;
			case CDROMCMD_RESIZE_BUFFERS:	err = ResizeBufferSpace(cd);				break;
			case CDROMCMD_WOBBLE_INFO:		err = GetWobbleInfo(cd);					break;
			case CDROMCMD_MONITOR:			err = Monitor(cd);							break;
			case CDROMCMD_PASSTHRU:			err = Passthru(cd);							break;
		}

		/* if the cmd we are working on just finished... */
		if (err)
		{
			CompleteWorkingIOReq(cd->cd_workingIOR, err);

			/* signal daemon to make sure we process any pending ioReq */
			if (cd->cd_DaemonTask)
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);
		}

	}
}

/*******************************************************************************
* CmdStatus()                                                                  *
*                                                                              *
*      Purpose: Support for CMD_STATUS driver API call.                        *
*                                                                              *
*   Parameters: cd     - ptr todrom struct for this device                     *
*               theReq - the ioReq associated with the CMD_STATUS              *
*                                                                              *
*    Called by: LCCDDriverDispatch() and ProcessClientIOReqs().                *
*                                                                              *
*******************************************************************************/
int32 CmdStatus(cdrom *cd, IOReq *theReq)
{
	DeviceStatus	status;
	int32			actual;

	/* this provides an "initial holdoff" so that we can at least attempt to
	 * read the TOC, etc.  This is required so that the filesystem waits for
	 * for valid data (in the DeviceStatus struct) to be returned
	 */
	if (!(cd->cd_State & CD_CACHED_INFO_AVAIL))
		return (kCmdNotYetComplete);

	status.ds_DriverIdentity = DI_M2CD_CDROM;
	status.ds_DriverStatusVersion = 0;
	status.ds_MaximumStatusSize = sizeof(status);
	status.ds_DeviceFlagWord = cd->cd_Status;
	status.ds_FamilyCode = DS_DEVTYPE_CDROM;
	status.ds_DeviceUsageFlags = DS_USAGE_READONLY | DS_USAGE_TRUEROM | DS_USAGE_FILESYSTEM;
	status.ds_DeviceBlockCount = cd->cd_MediumBlockCount;
	status.ds_DeviceLastErrorCode = 0;
	status.ds_DeviceMediaChangeCntr = 0;
	status.ds_DeviceBlockStart =
		BinMSF2Offset(((uint32)cd->cd_TOC_Entry[1].minutes << 16) |
		((uint32)cd->cd_TOC_Entry[1].seconds << 8) |
		((uint32)cd->cd_TOC_Entry[1].frames));

	/* this assumes that all data is of one type */
	if (cd->cd_TOC_Entry[1].addressAndControl & kDataTrackTOCEntry)
	{
		status.ds_DeviceBlockSize = CDROM_MODE1;

		/* 3DO filesystems only supported on Mode1 discs currently */
		if (!cd->cd_DiscInfo.discID)
			status.ds_DeviceUsageFlags |= DS_USAGE_FILESYSTEM;
	}
	else
		status.ds_DeviceBlockSize = CDROM_AUDIO;

	actual = (int32) ((theReq->io_Info.ioi_Recv.iob_Len < sizeof(status)) ?
			  theReq->io_Info.ioi_Recv.iob_Len : sizeof(status));
	memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, &status, actual);
	theReq->io_Actual = actual;

	DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));

	return (kCmdComplete);
}

/*******************************************************************************
* DiscData()                                                                   *
*                                                                              *
*      Purpose: Support for CDROMCMD_DISCDATA driver API call.                 *
*                                                                              *
*   Parameters: cd     - ptr todrom struct for this device                     *
*               theReq - the ioReq associated with the CDROMCMD_DISCDATA       *
*                                                                              *
*    Called by: LCCDDriverDispatch() and ProcessClientIOReqs().                *
*                                                                              *
*******************************************************************************/
int32 DiscData(cdrom *cd, IOReq *theReq)
{
	CDROM_Disc_Data	*data;
	int32			actual;

	if (!(cd->cd_State & CD_CACHED_INFO_AVAIL))
		return (kCmdNotYetComplete);

	/* NOTE:  assumes order of internal structs:  cd_DiscInfo, cd_TOC_Entry,
	 *        cd_SessionInfo in cdrom struct does not change
	 */
	data = (CDROM_Disc_Data *)&cd->cd_DiscInfo;
	actual = theReq->io_Info.ioi_Recv.iob_Len;
	if (actual > sizeof(CDROM_Disc_Data))
		actual = sizeof(CDROM_Disc_Data);
	memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, data, actual);
	theReq->io_Actual = actual;

	return (kCmdComplete);
}

/*******************************************************************************
* Read()                                                                       *
*                                                                              *
*      Purpose: Provides the state machine needed for CMD_READ requests.       *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device                               *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err Read(cdrom *cd)
{
	CDROMCommandOptions	options;
	IOReq				*theReq = cd->cd_workingIOR;
	uint8				sectorFormat, bufBlk;
	uint32				blockLen, bufLen;
	Err					err;

	const uint8 pPso[] = { CD_PPSO, 0x02 };					/* pause (close the data valve, stop any dma) */
	const uint8 Ppso[] = { CD_PPSO, 0x03 };					/* play (open data valve) */

	uint8 driveSpeed[] = { CD_SETSPEED, 0x01, 0x00 };		/* single/double speed + pitch */
	uint8 bobFormat[] = { CD_SECTORFORMAT, 0x00, 0x00 };	/* sector format + subcode enable */
	uint8 seek2sector[] = { CD_SEEK, 0x00, 0x00, 0x00 };	/* seek */


	/* calc stuff that used in multiple states
	 * (NOTE:  blockLen, sectorFormat are not valid on the first pass...
	 * because options gets updated in RD_START)
	 */
	options.asLongword = theReq->io_Info.ioi_CmdOptions;
	blockLen = options.asFields.blockLength;
	sectorFormat = MKE2BOB(cd, options.asFields.densityCode);

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case RD_START:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_START\n"));

			err = VerifyCmdOptions(cd, options.asLongword);
			if (err)
				return (err);

			if (!options.asFields.densityCode)		options.asFields.densityCode = 		cd->cd_DefaultOptions.asFields.densityCode;
			if (!options.asFields.addressFormat)	options.asFields.addressFormat =	cd->cd_DefaultOptions.asFields.addressFormat;
			if (!options.asFields.speed)			options.asFields.speed =			cd->cd_DefaultOptions.asFields.speed;
			if (!options.asFields.pitch)			options.asFields.pitch =			cd->cd_DefaultOptions.asFields.pitch;
			if (!options.asFields.blockLength)		options.asFields.blockLength =		cd->cd_DefaultOptions.asFields.blockLength;

			/* Note that this was originally coded to allow someone to be able
			 * to chose the default errorRecovery (by setting it to zero);
			 * while also allowing them to use a different retryShift (by
			 * setting it to non-zero...if they wanted "no retries", then they
			 * were required to specify the errorRecovery value).  This was a
			 * superset of the MKE functionality.
			 *
			 * As it is, we had to revert back to only allowing a different
			 * retryShift when the errorRecovery was specified (for
			 * compatibility reasons).  Joy.
			 */
			if (!options.asFields.errorRecovery)
			{
				options.asFields.errorRecovery =	cd->cd_DefaultOptions.asFields.errorRecovery;
				options.asFields.retryShift =		cd->cd_DefaultOptions.asFields.retryShift;
			}

			/* convert binary MSF to block offset */
			if (options.asFields.addressFormat == CDROM_Address_Abs_MSF)
			{
				theReq->io_Info.ioi_Offset = BinMSF2Offset(theReq->io_Info.ioi_Offset);
				options.asFields.addressFormat = CDROM_Address_Blocks;
			}

			/* save any defaults back into CmdOptions */
			theReq->io_Info.ioi_CmdOptions = options.asLongword;
			DBUG(kPrintCmdOptions, ("M2CD:  ioi_CmdOptions=%08lx\n", theReq->io_Info.ioi_CmdOptions));

			/* init these during first pass thru Read() */
			blockLen = options.asFields.blockLength;
			sectorFormat = MKE2BOB(cd, options.asFields.densityCode);

			cd->cd_CurRetryCount = (1 << options.asFields.retryShift) - 1;
			cd->cd_SavedRecvPtr = (uint8 *)theReq->io_Info.ioi_Recv.iob_Buffer;

			/* original length of user's buffer */
			bufLen = theReq->io_Info.ioi_Recv.iob_Len;

			/* offset of the next sector to transfer, and number of sectors */
			cd->cd_SectorOffset = theReq->io_Info.ioi_Offset;
			cd->cd_SectorsRemaining = bufLen / blockLen;

			/* range check the incoming block address based on TOC */
			if (cd->cd_SectorOffset > cd->cd_MediumBlockCount)
			{
				DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Block address (%08lx) out-of-range (discEnd=%08lx)...aborting request\n", Offset2BCDMSF(cd->cd_SectorOffset),
						Offset2BCDMSF(BinMSF2Offset(((uint32)cd->cd_DiscInfo.minutes << 16) |
													((uint32)cd->cd_DiscInfo.seconds << 8) |
													((uint32)cd->cd_DiscInfo.frames)))));
				return (kCDErrEndOfMedium);
			}

			/* FIXME:  Test zero-byte-length read failure */
			/* make sure that bufLen is a multiple of blockLen
			 * also, disallow zero-byte-length reads
			 * also, make sure blkLen specified is cool for this format
			 * also, make sure sectorFormat jives with the track they're reading
			 */
			if ((bufLen != (cd->cd_SectorsRemaining * blockLen)) || (!bufLen) ||
				!ValidBlockLengthFormat(sectorFormat, blockLen) ||
				DataFormatDontJive(cd, Offset2BCDMSF(cd->cd_SectorOffset), sectorFormat))
			{
				DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Bad ioRequest buffer size (%08lx), blockLen (%08lx), or sectorFormat (%02x)...aborting request\n", bufLen, blockLen, sectorFormat));
				return (kCDErrBadArg);
			}

			DBUG(kPrintQtyNPosOfSectorReqs, ("M2CD:  %ld @ %06lx\n", cd->cd_SectorsRemaining, Offset2BCDMSF(cd->cd_SectorOffset)));
			theReq->io_Actual = 0;		/* initialize current transfer count */

			/* is the speed different?  if so, gotta pause */
			if (cd->cd_CurrentSettings.speed != options.asFields.speed)
			{
				cd->cd_DevState = RD_RESTART;
				goto ReadRestart;
			}
			cd->cd_DevState = RD_VARIABLE_PITCH;

		case RD_VARIABLE_PITCH:
			/* is the pitch different? */
			if (cd->cd_CurrentSettings.pitch != options.asFields.pitch)
			{
				/* bail, if we're currently not in single speed */
				if (cd->cd_CurrentSettings.speed != CDROM_SINGLE_SPEED)
					return (kCDErrBadArg);

				/* NOTE:  fine pitch always enabled for single speed */
				driveSpeed[1] = kFineEn | kSingleSpeed;
				switch (options.asFields.pitch)
				{
					case CDROM_PITCH_SLOW:		driveSpeed[2] = kNPct010;	break;		/* - 1% of normal */
					case CDROM_PITCH_NORMAL:	driveSpeed[2] = kPPct000;	break;		/* + 0% of normal */
					case CDROM_PITCH_FAST:		driveSpeed[2] = kPPct010;	break;		/* + 1% of normal */
				}
				err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);
				DBUG(kPrintVariPitch,("RD_VARI_PITCH:  (%d) %02X %02X\n", options.asFields.pitch, driveSpeed[1], driveSpeed[2]));
				cd->cd_CurrentSettings.pitch = options.asFields.pitch;
			}
			cd->cd_DevState = RD_LOOP;

		case RD_LOOP:
loopDloop:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_LOOP (%ld sectors left)\n", cd->cd_SectorsRemaining));

			/* is this sector available in the prefetch buffer? */
			err = DataAvailNow(cd, cd->cd_SectorOffset, sectorFormat, options.asLongword, &bufBlk);
			switch (err)
			{
				case kNoData:
					DBUG(kPrintDataAvailNowResponse, ("M2CD:  kNoData (%08lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
					if (DataAvailRSN(cd, cd->cd_SectorOffset, sectorFormat))
					{
						/* look for possible (seek) error */
						if (cd->cd_State & CD_DEVICE_ERROR)
						{
							cd->cd_State &= ~CD_DEVICE_ERROR;
							return (kCDErrDeviceError);
						}
						gHighWaterMark = MIN(gMaxHWM, cd->cd_SectorsRemaining);
						cd->cd_State |= CD_READ_IOREQ_BUSY;
						break;
					}
					DBUG(kPrintDataAvailNowResponse, ("M2CD:  Not avail RSN...S/E = %08lx - %08lx\n", Offset2BCDMSF(cd->cd_PrefetchStartOffset), Offset2BCDMSF(cd->cd_PrefetchEndOffset)));
					cd->cd_State &= ~CD_READ_IOREQ_BUSY;
					cd->cd_DevState = RD_RESTART;
					break;
				case kBadData:
					DBUG(kPrintDataAvailNowResponse, ("M2CD:  kBadData (%08lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
					/* has the retry count expired? */
					if (!cd->cd_CurRetryCount)
					{
						/* if requested, return the data even if unrecoverable
						 * via ECC or via RETRIES, respectively
						 */
						if ((options.asFields.errorRecovery == CDROM_BEST_ATTEMPT_RECOVERY) ||
							(options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY))
						{
							CopySectorData(cd, (uint8 *)cd->cd_SavedRecvPtr,
								cd->cd_DataBlkHdr[bufBlk].buffer, blockLen, sectorFormat);

							/* mark block as avail to prefetch engine */
							cd->cd_DataBlkHdr[bufBlk].state = BUFFER_BLK_FREE;

							/* now that we have another free block available to
							 * prefetch into, we need to update the EndOffset
							 * so DataAvailRSN() works properly.  also, make
							 * sure we're still prefetching (so we don't screw
							 * up the DataAvailRSN test).
							 */
							if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
								cd->cd_PrefetchEndOffset++;

							/* move read index to next VALID block */
							cd->cd_DataReadIndex = (bufBlk == (cd->cd_NumDataBlks-1)) ? 0 : (bufBlk + 1);

							/* update starting offset of prefetch space to
							 * exclude this freed block
							 */
							cd->cd_PrefetchStartOffset++;

							/* increment user length count */
							theReq->io_Actual += blockLen;
						}
						else
						{
							DBUG2(kPrintWarnings, ("M2CD:  Retry count expired!  (sector %06lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
						}

						/* complete this ioRequest */
						return (kCDErrMediaError);
					}
					else
						cd->cd_CurRetryCount--;
				case kResyncSeek:	/* for SCAN_READ support */
				case kNoSubcode:
					cd->cd_DevState = RD_RESTART;
					break;
				default:
					CopySectorData(cd, (uint8 *)cd->cd_SavedRecvPtr, cd->cd_DataBlkHdr[bufBlk].buffer, blockLen, sectorFormat);

#if DEBUG_ECC
					DBUG(kPrintQtyNPosOfSectorReqs, ("M2CD:  buf %08x, chksum %08x\n", cd->cd_SavedRecvPtr, GenChecksum((uint32 *)cd->cd_SavedRecvPtr)));
#endif

					if ((cd->cd_State &
					   (CD_PREFETCH_SUBCODE_ENABLED | CD_GOT_PLENTY_O_SUBCODE)) ==
					   (CD_PREFETCH_SUBCODE_ENABLED | CD_GOT_PLENTY_O_SUBCODE))
					{
						uint8 subBlk = cd->cd_SubcodeReadIndex;

						/* mark blk as avail to subcode engine, update index */
						cd->cd_SubcodeBlkHdr[subBlk].state = BUFFER_BLK_FREE;
						cd->cd_SubcodeReadIndex = (subBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (subBlk + 1);
					}

					/* mark block as avail to prefetch engine */
					cd->cd_DataBlkHdr[bufBlk].state = BUFFER_BLK_FREE;

					/* now that we have another free block available to
					 * prefetch into, we need to update the EndOffset so
					 * DataAvailRSN() works properly.  also, make sure we're
					 * still prefetching (so we don't screw up the
					 * DataAvailRSN test).
					 */
					if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
						cd->cd_PrefetchEndOffset++;

					/* move read index to next VALID block, update index */
					cd->cd_DataReadIndex = (bufBlk == (cd->cd_NumDataBlks-1)) ? 0 : (bufBlk + 1);
					cd->cd_PrefetchStartOffset++;

					cd->cd_SavedRecvPtr += blockLen;	/* update local copy of recv buf ptr */
					theReq->io_Actual += blockLen;		/* increment user length count */
					cd->cd_SectorOffset++;				/* increment sector offset (to read next sector) */
					cd->cd_SectorsRemaining--;			/* update remaining sector count */

					/* reset retry count for next sector */
					cd->cd_CurRetryCount = (1 << options.asFields.retryShift) - 1;

					/* any sectors remaining? */
					if (!cd->cd_SectorsRemaining)
					{
						cd->cd_State &= ~CD_READ_IOREQ_BUSY;
						return (kCmdComplete);		/* return "cmd completed" */
					}

					goto loopDloop;						/* nasty; but fast */
			}

			/* if the data is gonna be avail RSN,
			 * ...break, and wait for it to show up
			 */
			if ((err == kNoData) && (cd->cd_DevState == RD_LOOP))
				break;

		case RD_RESTART:
ReadRestart:
			/* pause (close the data valve, stop any dma) */
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_RESTART\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_Status |= CDROM_STATUS_SPIN_UP;
			DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));
			cd->cd_DevState = RD_SPEED;
		case RD_SPEED:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_SPEED\n"));
			/* is the speed already set? */
			if (cd->cd_CurrentSettings.speed != options.asFields.speed)
			{
				switch (options.asFields.speed)
				{
					case CDROM_SINGLE_SPEED:
						/* single speed, fine pitch disabled... */
						driveSpeed[1] = kSingleSpeed;

						/* fine pitch always enabled for single-speed, audio */
						if (sectorFormat == DA_SECTOR)
							driveSpeed[1] |= kFineEn;
						cd->cd_Status &= ~(CDROM_STATUS_DOUBLE_SPEED |
							CDROM_STATUS_4X_SPEED | CDROM_STATUS_6X_SPEED |
							CDROM_STATUS_8X_SPEED);
						break;
					case CDROM_DOUBLE_SPEED:
						/* double speed, fine pitch disabled... */
						driveSpeed[1] = kDoubleSpeed;
						cd->cd_Status |= CDROM_STATUS_DOUBLE_SPEED;
						cd->cd_Status &= ~(CDROM_STATUS_4X_SPEED |
							CDROM_STATUS_6X_SPEED | CDROM_STATUS_8X_SPEED);
						break;
					case CDROM_4X_SPEED:
						/* 4x speed, fine pitch disabled... */
						driveSpeed[1] = k4xSpeed;
						cd->cd_Status |= CDROM_STATUS_4X_SPEED;
						cd->cd_Status &= ~(CDROM_STATUS_DOUBLE_SPEED |
							CDROM_STATUS_6X_SPEED | CDROM_STATUS_8X_SPEED);
						break;
					case CDROM_6X_SPEED:
						/* 6x speed, fine pitch disabled... */
						driveSpeed[1] = k6xSpeed;
						cd->cd_Status |= CDROM_STATUS_6X_SPEED;
						cd->cd_Status &= ~(CDROM_STATUS_DOUBLE_SPEED |
							CDROM_STATUS_4X_SPEED | CDROM_STATUS_8X_SPEED);
						break;
					case CDROM_8X_SPEED:
						/* 8x speed, fine pitch disabled... */
						driveSpeed[1] = k8xSpeed;
						cd->cd_Status |= CDROM_STATUS_8X_SPEED;
						cd->cd_Status &= ~(CDROM_STATUS_DOUBLE_SPEED |
							CDROM_STATUS_4X_SPEED | CDROM_STATUS_6X_SPEED);
						break;
				}
				DBUG(kPrintStatusWord, ("M2CD:  Device status word currently...0x%02x\n", cd->cd_Status));
				if ((driveSpeed[1] != kSingleSpeed) || (sectorFormat != DA_SECTOR))
				{
					if (options.asFields.pitch != CDROM_PITCH_NORMAL)
						return (kCDErrBadArg);
				}
				else
				{
					switch (options.asFields.pitch)
					{
						case CDROM_PITCH_SLOW:		driveSpeed[2] = kNPct010;	break;		/* - 1% of normal */
						case CDROM_PITCH_NORMAL:	driveSpeed[2] = kPPct000;	break;		/* + 0% of normal */
						case CDROM_PITCH_FAST:		driveSpeed[2] = kPPct010;	break;		/* + 1% of normal */
					}
				}

				err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);
				DBUG(kPrintVariPitch,("RD_SPEED:  (%d) %02X %02X\n", options.asFields.pitch, driveSpeed[1], driveSpeed[2]));
				cd->cd_CurrentSettings.speed = options.asFields.speed;
				cd->cd_CurrentSettings.pitch = options.asFields.pitch;
			}
			cd->cd_DevState = RD_FORMAT;

		case RD_FORMAT:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_FORMAT\n"));
			bobFormat[1] = cd->cd_PrefetchSectorFormat = sectorFormat;

			/* are they asking for subcode? */
			if (options.asFields.blockLength >= 2436)
			{
				cd->cd_State |= CD_PREFETCH_SUBCODE_ENABLED;
				bobFormat[2] = 0x01;
			}
			else
				cd->cd_State &= ~CD_PREFETCH_SUBCODE_ENABLED;

 			/* are the format, subcode already set? */
			if ((cd->cd_CurrentSettings.format != sectorFormat) ||
				(cd->cd_CurrentSettings.subcode != bobFormat[2]))
			{
				err = SendCmdWithWaitState(cd, bobFormat, sizeof(bobFormat), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);

				/* save current settings */
				cd->cd_CurrentSettings.format = sectorFormat;
				cd->cd_CurrentSettings.subcode = bobFormat[2];
			}
			cd->cd_DevState = RD_SEEK;

		case RD_SEEK:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_SEEK\n"));
			/* firmware wants binary frame # (not BCD...NOTE: seek2sector[0]
			 * is safe because MSB of cd_SectorOffset is 0x00)
			 */
			*(uint32 *)seek2sector |= cd->cd_SectorOffset;

			/* if we've been requested to do a SCAN_READ, use the
			 * "loose" form of the seek command.
			 */
			if (cd->cd_workingIOR->io_Info.ioi_Command == CDROMCMD_SCAN_READ)
				seek2sector[1] |= 0x80;

			/* indicate which mode we're reading in, so that if we switch
			 * from "scan" to "normal" we can re-sync the seek (ie, to ignore
			 * any data that may be prefetched...and that might "look like"
			 * the right sector data based on the tagged MSF.
			 */
			cd->cd_PrefetchSeekMode = cd->cd_workingIOR->io_Info.ioi_Command;

			err = SendCmdWithWaitState(cd, seek2sector, sizeof(seek2sector), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RD_PREPARE;

		case RD_PREPARE:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_PREPARE\n"));
			/* reset the prefetch space */
			InitDataSpace(cd);

			/* reset the subcode space, if reading subcode too */
			if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
				InitSubcodeSpace(cd);

			/* Prefetch Engine now enabled */
			cd->cd_State |= (CD_CURRENTLY_PREFETCHING | CD_GONNA_HAVE_TO_STOP);
			cd->cd_PrefetchStartOffset = cd->cd_SectorOffset;

			/* NOTE:  There will be N-1 VALID blocks */
			cd->cd_PrefetchEndOffset = cd->cd_SectorOffset + cd->cd_NumDataBlks - 1;
			cd->cd_PrefetchCurMSF = Offset2BCDMSF(cd->cd_SectorOffset);

			/* audio or cd-rom? */
			cd->cd_BlockLength = (sectorFormat == DA_SECTOR) ? 2352 : 2344;

			/* init blks, dma, etc. */
			InitPrefetchEngine(cd);

			gHighWaterMark = MIN(gMaxHWM, cd->cd_SectorsRemaining);
			cd->cd_State |= CD_READ_IOREQ_BUSY;

			cd->cd_DevState = RD_PLAY;

		case RD_PLAY:
			DBUG(kPrintActionMachineStates, ("M2CD:  RD_PLAY\n"));
			/* CIPNone, because we want to allow the CIPState response(s) to
			 * wake the daemon up
			 */
			err = SendCmdWithWaitState(cd, Ppso, sizeof(Ppso), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RD_LOOP;
			break;
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* GetWobbleInfo()                                                              *
*                                                                              *
*      Purpose: Provides the CheckWO() wobble (copy protection) info from the  *
*               disc.                                                          *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device                               *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err GetWobbleInfo(cdrom *cd)
{
	CDROMCommandOptions options;
	Err					err;
	uint32				actual;
	IOReq				*theReq = cd->cd_workingIOR;
	const uint8			pPso[] = { CD_PPSO, 0x02 };		/* pause */
	const uint8			Ppso[] = { CD_PPSO, 0x03 };		/* play */
	const uint8			checkWO[] = { CD_CHECKWO };
	uint8				driveSpeed[] = { CD_SETSPEED, 0x00, CDROM_PITCH_NORMAL };
	uint8				seek2sector[] = { CD_SEEK, 0x00, 0x00, 0x00 };

	options.asLongword = theReq->io_Info.ioi_CmdOptions;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case WO_START:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_START\n"));
			err = VerifyCmdOptions(cd, options.asLongword);
			if (err)
				return (err);

			if (!options.asFields.addressFormat)	options.asFields.addressFormat =	cd->cd_DefaultOptions.asFields.addressFormat;
			if (!options.asFields.speed)			options.asFields.speed =			cd->cd_DefaultOptions.asFields.speed;

			/* convert binary MSF to block offset */
			if (options.asFields.addressFormat == CDROM_Address_Abs_MSF)
			{
				theReq->io_Info.ioi_Offset = BinMSF2Offset(theReq->io_Info.ioi_Offset);
				options.asFields.addressFormat = CDROM_Address_Blocks;
			}

			/* save any defaults back into CmdOptions */
			theReq->io_Info.ioi_CmdOptions = options.asLongword;

			/* offset of the next sector to transfer, and number of sectors */
			cd->cd_SectorOffset = theReq->io_Info.ioi_Offset;

			/* make sure a buffer was supplied.  also make sure the sector
			 * location is in readable space.
			 */
			if (!theReq->io_Info.ioi_Recv.iob_Buffer ||
				(theReq->io_Info.ioi_Offset < 150))
				return (kCDErrBadArg);

			/* range check the incoming block address based on TOC */
			if (cd->cd_SectorOffset > cd->cd_MediumBlockCount)
				return (kCDErrEndOfMedium);
			cd->cd_DevState = WO_PAUSE;

		case WO_PAUSE:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_PAUSE\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_SPEED;

		case WO_SPEED:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_SPEED\n"));
			driveSpeed[1] = options.asFields.speed;

			err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_SEEK;

		case WO_SEEK:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_SEEK\n"));
			/* firmware wants binary frame # (not BCD...NOTE: seek2sector[0]
			 * is safe because MSB of cd_SectorOffset is 0x00)
			 */
			*(uint32 *)seek2sector |= cd->cd_SectorOffset;

			err = SendCmdWithWaitState(cd, seek2sector, sizeof(seek2sector), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_PLAY;

		case WO_PLAY:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_PLAY\n"));
			err = SendCmdWithWaitState(cd, Ppso, sizeof(Ppso), CIPPlay, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_CHECKWO;

		case WO_CHECKWO:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_CHECKWO\n"));
			err = SendCmdWithWaitState(cd, checkWO, sizeof(checkWO), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_PAUSE2;

		case WO_PAUSE2:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_PAUSE2\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = WO_SPEED2;

		case WO_SPEED2:
			DBUG(kPrintActionMachineStates, ("M2CD:  WO_SPEED2\n"));
			/* return speed to what it was previously */
			driveSpeed[1] = cd->cd_CurrentSettings.speed;
			driveSpeed[2] = cd->cd_CurrentSettings.pitch;

			err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);

			actual = theReq->io_Info.ioi_Recv.iob_Len;
			if (actual > sizeof(CDWobbleInfo))
				actual = sizeof(CDWobbleInfo);
			memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, &cd->cd_WobbleInfo, actual);
			theReq->io_Actual = actual;

			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* ReadSubQ()                                                                   *
*                                                                              *
*      Purpose: Provides the state machine needed for CDROMCMD_READ_SUBQ reqs. *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device                               *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err ReadSubQ(cdrom *cd)
{
	const	uint8	QRptNow[] = { CD_QREPORTEN, 0x01 };		/* get a QRpt immediately */
	IOReq	*theReq = cd->cd_workingIOR;
	int32	actual;
	Err		err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case SQ_START:
			/* invalidate currently stored Qcode, so we can tell when the
			 * new one arrives...
			 */
			cd->cd_LastQCode.validByte = 0x00;
			cd->cd_DevState = SQ_QNOW;
		case SQ_QNOW:
			/* get one Qcode */
			err = SendCmdWithWaitState(cd, QRptNow, sizeof(QRptNow), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = SQ_UNIQUE_QRPT;
		case SQ_UNIQUE_QRPT:
			/* wait until we get a NEW Qcode */
			if (cd->cd_LastQCode.validByte)
			{
				actual = (int32) ((theReq->io_Info.ioi_Recv.iob_Len < sizeof(SubQInfo)) ?
						  theReq->io_Info.ioi_Recv.iob_Len : sizeof(SubQInfo));
				memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, &cd->cd_LastQCode, actual);
				theReq->io_Actual = actual;

				return(kCmdComplete);			/* indicate cmd completed */
			}
			break;
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* SetDefaults()                                                                *
*                                                                              *
*      Purpose: Updates the defaults for the LCCD device based on input from   *
*               the client's ioRequest.                                        *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err SetDefaults(cdrom *cd)
{
	CDROMCommandOptions	options;
	IOReq				*theReq = cd->cd_workingIOR;
	Err					err;

	options.asLongword = theReq->io_Info.ioi_CmdOptions;

	err = VerifyCmdOptions(cd, options.asLongword);
	if (err)
		return (err);

	/* set new defaults based on ioi_CmdOptions */
	if ((uint32)options.asFields.densityCode)		cd->cd_DefaultOptions.asFields.densityCode =	options.asFields.densityCode;
	if ((uint32)options.asFields.addressFormat)		cd->cd_DefaultOptions.asFields.addressFormat =	options.asFields.addressFormat;
	if ((uint32)options.asFields.speed)				cd->cd_DefaultOptions.asFields.speed =			options.asFields.speed;
	if ((uint32)options.asFields.pitch)				cd->cd_DefaultOptions.asFields.pitch =			options.asFields.pitch;
	if ((uint32)options.asFields.blockLength)		cd->cd_DefaultOptions.asFields.blockLength =	options.asFields.blockLength;

	if ((uint32)options.asFields.errorRecovery)		/* new default setting */
	{
		cd->cd_DefaultOptions.asFields.errorRecovery =	options.asFields.errorRecovery;

		/* store whatever's there (ie, allow ZERO retries) */
		cd->cd_DefaultOptions.asFields.retryShift =		options.asFields.retryShift;
	}
	else if ((uint32)options.asFields.retryShift)
	{
		/* if they just wanna update the retry count only */
		cd->cd_DefaultOptions.asFields.retryShift =		options.asFields.retryShift;
	}

	if (theReq->io_Info.ioi_Recv.iob_Buffer)
	{
		*(uint32 *)theReq->io_Info.ioi_Recv.iob_Buffer = cd->cd_DefaultOptions.asLongword;
		theReq->io_Actual = sizeof(cd->cd_DefaultOptions);;
	}

	return (kCmdComplete);
}

/*******************************************************************************
* ResizeBufferSpace()                                                          *
*                                                                              *
*      Purpose: Checks to make sure that we're not prefetching, etc. before    *
*               calling ResizeLinkedBuffers() to reconfigure the prefetch      *
*               space to the desired mode.                                     *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err ResizeBufferSpace(cdrom *cd)
{
	if (cd->cd_State & CD_GONNA_HAVE_TO_STOP)
		return (kCmdNotYetComplete);

	return (ResizeLinkedBuffers(cd, cd->cd_workingIOR->io_Info.ioi_CmdOptions));
}



/*******************************************************************************
* VerifyCmdOptions()                                                           *
*                                                                              *
*      Purpose: Utility to verify that the incoming ioi_CmdOptions are valid.  *
*                                                                              *
*   Parameters: opts - the ioReq's ioi_CmdOptions                              *
*                                                                              *
*      Returns: Err - if any of the options are invalid.                       *
*                                                                              *
*    Called by: Read() and SetDefaults().                                      *
*                                                                              *
*******************************************************************************/
Err	VerifyCmdOptions(cdrom *cd, uint32 opts)
{
	CDROMCommandOptions options;

	options.asLongword = opts;

	if (((uint32)options.asFields.densityCode > CDROM_DIGITAL_AUDIO) ||
		((uint32)options.asFields.addressFormat > CDROM_Address_Abs_MSF) ||
		((uint32)options.asFields.errorRecovery > CDROM_BEST_ATTEMPT_RECOVERY)||
		((uint32)options.asFields.pitch > CDROM_PITCH_FAST) ||
		((uint32)options.asFields.speed > CDROM_8X_SPEED) ||
		(((uint32)options.asFields.speed == CDROM_4X_SPEED) &&
			!(cd->cd_DriveFunc & CD_SUPPORTS_4X_MODE)) ||
		(((uint32)options.asFields.speed == CDROM_6X_SPEED) &&
			!(cd->cd_DriveFunc & CD_SUPPORTS_6X_MODE)) ||
		(((uint32)options.asFields.speed == CDROM_8X_SPEED) &&
			!(cd->cd_DriveFunc & CD_SUPPORTS_8X_MODE)))
	{
		return (kCDErrBadArg);
	}
	return (kCDNoErr);
}

/*******************************************************************************
* MKE2BOB()                                                                    *
*                                                                              *
*      Purpose: Converts an MKE 'density code' to its equiv. Bob 'format code'.*
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               densityCode - the MKE density code to convert                  *
*                                                                              *
*      Returns: uint8 - Bob format code                                        *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
uint8 MKE2BOB(cdrom *cd, int32 densityCode)
{
	switch (densityCode)
	{
		case CDROM_DEFAULT_DENSITY:		return ((cd->cd_DiscInfo.discID) ? XA_SECTOR : M1_SECTOR);
		case CDROM_DATA:				return (M1_SECTOR);
		case CDROM_MODE2_XA:			return (XA_SECTOR);
		case CDROM_DIGITAL_AUDIO:		return (DA_SECTOR);
		default:						return (INVALID_SECTOR);
	}
}

/*******************************************************************************
* ValidBlockLengthFormat()                                                     *
*                                                                              *
*      Purpose: Performs a sanity check to insure that the blockLength         *
*               specified in the ioReq's ioi_CmdOptions is valid for the       *
*               requested format.                                              *
*                                                                              *
*   Parameters: format - the Bob sector format specified in the ioReq          *
*               len    - the blockLength specified in the ioReq                *
*                                                                              *
*      Returns: uint32 - non-zero if the format is INVALID, zero otherwise.    *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
uint32 ValidBlockLengthFormat(uint8 format, int32 len)
{
	int8	valid = FALSE;

	switch (format)
	{
		case DA_SECTOR:
			switch (len)
			{
				case 2352:			/* data                                   */
				case 2353:			/* data + errorbyte (?)                   */
				case 2448:			/* data + subcode                         */
				case 2449:			/* data + subcode + errorbyte (?)         */
					valid = TRUE;
					break;
			}
			break;
		case M1_SECTOR:
			switch (len)
			{
				case 2048:			/* data                                   */
				case 2052:			/* hdr + data                             */
				case 2336:			/* data + aux/ecc                         */
				case 2340:			/* hdr + data + aux/ecc                   */
				case 2352:			/* sync + header + data + aux/ecc         */
				case 2436:			/* hdr + data + aux/ecc + subcode         */
				case 2440:			/* hdr + data + aux/ecc + cmpwrd + subcode*/
				case 2448:			/* sync + hdr + data + aux/ecc + subcode  */
					valid = TRUE;
					break;
			}
			break;
		case XA_SECTOR:
			switch (len)
			{
				case 2048:			/* Form1:   data                          */
				case 2060:			/* Form1:   header + data                 */
				case 2324:			/* Form2:   data                          */
				case 2328:			/* Form1/2: data + aux/ecc                */
				case 2336:			/* Form2:   hdr + data                    */
				case 2340:			/* Form1/2: hdr + data + aux/ecc          */
				case 2352:			/* Form1/2: sync + hdr + data + aux/ecc   */
				case 2436:			/* hdr + data + aux/ecc + subcode         */
				case 2440:			/* hdr + data + aux/ecc + cmpwrd + subcode*/
				case 2448:			/* sync + hdr + data + aux/ecc + subcode  */
					valid = TRUE;
					break;
			}
			break;
	}
	return (valid);
}

/*******************************************************************************
* DataFormatDontJive()                                                         *
*                                                                              *
*      Purpose: Performs a sanity check to insure that the data on the disc is *
*               being requested in the correct format.  IE, don't allow        *
*               reading audio in data, or vice-versa.                          *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               theMSF - the BCD MSF of the desired sector                     *
*               format - the Bob sector format specified in the ioReq (ie, the *
*                        attempted mode)                                       *
*                                                                              *
*      Returns: uint32 - non-zero if the format is INVALID, zero otherwise.    *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
uint32 DataFormatDontJive(cdrom *cd, uint32	theMSF, uint8 format)
{
	uint8	track = cd->cd_DiscInfo.lastTrackNumber;
	uint8	found = FALSE;
	uint32	curTrackStart;

	/* determine current track */
	do {

		/* calculate start of track 'x' */
		curTrackStart = ((uint32)BIN2BCD(cd->cd_TOC_Entry[track].minutes)<<16) +
						((uint32)BIN2BCD(cd->cd_TOC_Entry[track].seconds)<<8) +
						(uint32)BIN2BCD(cd->cd_TOC_Entry[track].frames);

		if (curTrackStart > theMSF)
			track--;
		else
			found = TRUE;
	} while (!found && (track >= cd->cd_DiscInfo.firstTrackNumber));

	/* this should always be true (as long as we _do_ perform range-checking
	 * on incoming ioi_Offset values)
     */
	if (found)
	{
		if (format == DA_SECTOR)
		{
			if (cd->cd_TOC_Entry[track].addressAndControl & kDataTrackTOCEntry)
				return (TRUE);				/* data requested in wrong mode */
		}
		else
		{
			if (!(cd->cd_TOC_Entry[track].addressAndControl & kDataTrackTOCEntry))
				return (TRUE);				/* data requested in wrong mode */
		}
	}
	else
	{
		DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Track match not found for sector %06lx\n", theMSF));
	}

	return (FALSE);							/* data requested in valid mode */
}

#if 0
	void PrintHdrCompWords(cdrom *cd, uint8 curBlk)
	{
		uint8	blk;
		uint8	curRead = cd->cd_DataReadIndex;
		uint8	curWrite = cd->cd_CurDataWriteIndex;
		uint8	nextWrite = cd->cd_NextDataWriteIndex;
		uint8	Bchr, Rchr, Cchr, Nchr;

		DBUG2(kPrintDeath, ("M2CD:  HDR      HDR+1    CW-1     CW\n"));
		for(blk = 0; blk < cd->cd_NumDataBlks; blk++)
		{
			DBUG2(kPrintDeath, ("M2CD:  %08lX %08lX ",
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[0],
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[4]));
			DBUG2(kPrintDeath, ("%08lX %08lX  ",
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2336],
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2340]));
			Bchr = (blk == curBlk) ? 'B' : ' ';
			Rchr = (blk == curRead) ? 'R' : ' ';
			Cchr = (blk == curWrite) ? 'C' : ' ';
			Nchr = (blk == nextWrite) ? 'N' : ' ';
			DBUG2(kPrintDeath, ("%c %c ", Bchr, Rchr));
			DBUG2(kPrintDeath, ("%c %c\n", Cchr, Nchr));
		}
	}
#else
	#define PrintHdrCompWords(cd,curBlk)
#endif

/*******************************************************************************
* DataAvailNow()                                                               *
*                                                                              *
*      Purpose: Determines if the requested sector is currently available in   *
*               the prefetch space (in a block marked as VALID).  This is      *
*               accomplished by starting at the current beginning of the       *
*               prefetch space (DataReadIndex) and scanning forward until the  *
*               sector is found...or the prefetch buffer is completely         *
*               digested.  As the prefetch buffer is scanned, the unneeded     *
*               blocks are marked as FREE so they can be re-used by the        *
*               Prefetch Engine.                                               *
*                                                                              *
*        NOTE:  This means that if sectors 1,2,3,4,5 are available, and we get *
*               the following requests 1,2,5,3; sectors 1,2,5 will be returned *
*               immediately; but sector 3 will have to be re-fetched.  If the  *
*               requested sector is found, its block number is returned;       *
*               otherwise, we return -1.  This also has the effect of          *
*               freeing-up the entire prefetch buffer.  We also insure that if *
*               the data has been prefetched, that it was read using the same  *
*               format as is requested for that sector.  In the case where     *
*               subcode is also requested with audio sector data, we must have *
*               at least 4 (NUM_SYNCS_NEEDED_2_PASS+1) _valid_ subcode blocks  *
*               in order to return the block number (ie, the data is avail     *
*               now); otherwise, we return -1.                                 *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               sector - requested sector (in an absolute block number...not   *
*                        MSF)                                                  *
*               format - requested format of mode to read sector in            *
*                                                                              *
*      Returns: int8 - index of the prefetch buf blk which contains the sector *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
int8 DataAvailNow(cdrom *cd, int32 sector, uint8 format, uint32 opts, uint8 *blk)
{
	CDROMCommandOptions options;
	uint32	theMSFAddr;
	uint8	x;
	uint8	subBlk, nextBlk;
	int32	numErrs;
	uint32	savedHeader;
	uint32	curCmd = cd->cd_workingIOR->io_Info.ioi_Command;	/* SCAN_READ */

	options.asLongword = opts;
	theMSFAddr = Offset2BCDMSF(sector);				/* MSF of sector we want */

	/* start with the current data, subcode blks */
	*blk = cd->cd_DataReadIndex;
	subBlk = cd->cd_SubcodeReadIndex;

	/* continue until we run out of valid blocks */
	while(cd->cd_DataBlkHdr[*blk].state == BUFFER_BLK_VALID)
	{
		/* does the sector read match the one we want? */
		if (cd->cd_DataBlkHdr[*blk].MSF == theMSFAddr)
		{
			/* ...and are things being read in the desired mode? */
			if (cd->cd_DataBlkHdr[*blk].seekmode == curCmd)
			{
				/* ...and does the format match what we want? */
				if (cd->cd_DataBlkHdr[*blk].format == format)
				{
					/* insure that no rogue reads cause a cache coherency
					 * problem for the DMA buffer data.  we invalidate the
					 * cache before EITHER the ECC code or CopySectorData()
					 * has had a chance to touch it.
					 */
					SuperInvalidateDCache(cd->cd_DataBlkHdr[*blk].buffer, kDataBlkSize);

					/* ...and is subcode being requested for this sector? */
					if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
					{
						cd->cd_State |= CD_GOT_PLENTY_O_SUBCODE;
						nextBlk = subBlk;
						/* "plus 1" because the sync has a 98% chance of
						 * falling in the middle of the buffer block which
						 * means we'll need one additional VALID block
					 	 */
						for (x = 0; x < (NUM_SYNCS_NEEDED_2_PASS+1); x++)
						{
							if (cd->cd_SubcodeBlkHdr[nextBlk].state != BUFFER_BLK_VALID)
							{
								if (cd->cd_SubcodeBlkHdr[nextBlk].state == BUFFER_BLK_BUCKET)
									return (kNoSubcode);	/* we will never have enough subcode because the buffer filled up (and we stopped prefetching) */
								else
									cd->cd_State &= ~CD_GOT_PLENTY_O_SUBCODE;
							}

							nextBlk = (nextBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (nextBlk + 1);
						}
					}

					/* if we've gotten this far then we have enough subcode to
				 	 * return this sector
				 	 * note: upon returning, cd->cd_DataReadIndex = *blk
 					 */
					if (format == DA_SECTOR)
						return (*blk);

					/* Verify hdr[31:8] == compword[31:8] for M1,XA sectors */
					if ((*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[0] ^
						*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[2340]) & 0xFFFFFF00)
					{
						DBUG2(kPrintWarnings, ("M2CD:  >WARNING<  Compword does NOT match header! (exp = %06lx, hdr = %06lx, cw = %06lx)\n\n",
							theMSFAddr,
							(*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[0]) >> 8,
							(*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[2340]) >> 8));
						PrintHdrCompWords(cd, *blk);
						return (kBadData);
					}

					if (cd->cd_DataBlkHdr[*blk].buffer[2343] & CRC_ERROR_BIT)
					{
						DBUG(kPrintECCStats, ("M2CD:  Must perform ECC on sector %06lx...", theMSFAddr));

						/* deal with mode 1 sectors */
						if (format == M1_SECTOR)
						{
							/* if requested, return the data even if unrecoverable
						 	* via retries (do not perform ECC in this case)
						 	*/
							if (options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY)
							{
								DBUG(kPrintECCStats,("CIRC_RETRIES_ONLY\n"));
								return (kBadData);
							}

#if DEBUG_ECC
							gECCTimer = 0xbad42ecc;
							gECCTimer = cd->cd_DataBlkHdr[*blk].MSF;
#endif
							gECCTimer = SectorECC(cd->cd_DataBlkHdr[*blk].buffer);
							numErrs = gECCTimer;
							DBUG(kPrintECCStats, ("%08lx fixed\n", numErrs));

							if (numErrs < 0)
								return (kBadData);
						}
						else					/* deal with mode 2 sectors */
						{
							/* is it Form2? */
							if (cd->cd_DataBlkHdr[*blk].buffer[6] & 0x20)
							{
								/* is the optional EDC implemented? */
								if (*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[2336])
								{
									/* return an error (no ECC to perform here) */
									DBUG2(kPrintWarnings, ("M2CD:  >ERROR<  Got CRC error for Form2 data on sector %06lx %08lx\n",
										Offset2BCDMSF(cd->cd_SectorOffset),
										*(uint32 *)&cd->cd_DataBlkHdr[*blk].buffer[2336]));

									return (kBadData);
								}
								else
									return (*blk);
							}
							else
							{
								/* if requested, return the data even if
							 	* unrecoverable via retries (do not perform ECC
							 	* in this case)
							 	*/
								if (options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY)
									return (kBadData);

								savedHeader = *(uint32 *)cd->cd_DataBlkHdr[*blk].buffer;
								*(uint32 *)cd->cd_DataBlkHdr[*blk].buffer = 0x00000000;

#if DEBUG_ECC
								gECCTimer = 0xbad42ecc;
								gECCTimer = cd->cd_DataBlkHdr[*blk].MSF;
#endif
								gECCTimer = SectorECC(cd->cd_DataBlkHdr[*blk].buffer);
								numErrs = gECCTimer;

								*(uint32 *)cd->cd_DataBlkHdr[*blk].buffer = savedHeader;
								DBUG(kPrintECCStats, ("%08lx fixed\n", numErrs));
								if (numErrs < 0)
									return (kBadData);
							}
						}
					}
#if VERIFY_CRC_CHECK
					else
					{
						if (format == XA_SECTOR)
						{
							savedHeader = *(uint32 *)cd->cd_DataBlkHdr[*blk].buffer;
							*(uint32 *)cd->cd_DataBlkHdr[*blk].buffer = 0x00000000;
						}
						gECCTimer = 0xbad42ecc;
						gECCTimer = cd->cd_DataBlkHdr[*blk].MSF;
						memcpy(gECCBuf, cd->cd_DataBlkHdr[*blk].buffer, 2344);
						/* gECCTimer = SectorECC(cd->cd_DataBlkHdr[*blk].buffer); */
						gECCTimer = SectorECC(gECCBuf);
						numErrs = gECCTimer;
flagLoop:
						DBUG(kPrintECCStats, ("%08x fixed\n", numErrs));
						if (format == XA_SECTOR)
							*(uint32 *)cd->cd_DataBlkHdr[*blk].buffer = savedHeader;
						if (numErrs & 0x0000FFFF)		/* only look at the error count */
						{
							uint32 x;
							uint8 *buf = cd->cd_DataBlkHdr[*blk].buffer;
							vuint32 flag;
							DBUGERR(("M2CD:  >ERROR<  CRC Failure @ %06x!  (ECC stat: %08x)\n", cd->cd_DataBlkHdr[*blk].MSF, numErrs));
#if 1
							bufcmp(gECCBuf, cd->cd_DataBlkHdr[*blk].buffer);
							for (x = 0; x < 2336; x += 16)
							{
								Superkprintf("%08x %08x ",
									*(uint32 *)&buf[x], *(uint32 *)&buf[x+4]);
								Superkprintf("%08x %08x\n",
									*(uint32 *)&buf[x+8], *(uint32 *)&buf[x+12]);
							}
							Superkprintf("%08x %08x\n", *(uint32 *)&buf[x], *(uint32 *)&buf[x+4]);
							Superkprintf("sector @ %08x\n", buf);
							Superkprintf("gECCbuf @ %08x\n", gECCBuf);
							Superkprintf("flag @ %08x\n", &flag);
							flag = 0xDEADC0DE;
							while (flag && (flag != 1))
								x++;
							memcpy(gECCBuf, buf, 2344);
							numErrs = SectorECC(gECCBuf);
							if (flag != 1)
								goto flagLoop;
#endif
						}
					}
#endif
					/* verify that the header/sector/etc is what we think it is
				 	* ...but, don't do it if we're currently doing "loose"
				 	* seeking.
				 	*/
					if (curCmd != CDROMCMD_SCAN_READ)
						if (SanityCheckSector(cd, *blk))
						{
							PrintHdrCompWords(cd, *blk);
							return (kBadData);
						}

					/* note:  upon returning cd->cd_DataReadIndex = *blk */
					return (*blk);
				}
				else		/* data found in WRONG format, must re-read it */
					return (kBadData);
			}
			else
				return (kResyncSeek);
		}
		else
		{
			/* this is not the sector we're looking for, so free up this block
			 * (ie, we won't allow people to read data that has been previously
			 * prefetched but read after) meaning...if we prefetch 1,2,3,4,5
			 * and get a request to return 4, we cannot go back and return 2
			 * without re-reading it.  tough nuegies...
			 */

			/* is subcode being requested for this sector? */
			if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
			{
				/* update subcode index to start at next block */
				cd->cd_SubcodeReadIndex = (subBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (subBlk + 1);

				/* mark block as available to Prefetch Engine */
				cd->cd_SubcodeBlkHdr[subBlk].state = BUFFER_BLK_FREE;

				subBlk = cd->cd_SubcodeReadIndex;
			}

			/* update startOffset of pref space to exclude this freed block.
			 * ...mark update data index to start at next block.
			 * ...mark sector as prefetched but skipped over.
			 * ...mark block as available to Prefetch Engine.
			 */
			cd->cd_PrefetchStartOffset++;
			cd->cd_DataReadIndex = (*blk == (cd->cd_NumDataBlks-1)) ? 0 : (*blk + 1);
			cd->cd_DataBlkHdr[*blk].MSF |= 0xFF000000;
			cd->cd_DataBlkHdr[*blk].state = BUFFER_BLK_FREE;

			/* update the endOffset (last sector that will be prefetch-able) */
			if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
				cd->cd_PrefetchEndOffset++;

			*blk = cd->cd_DataReadIndex;
		}
	}

	/* requested sector not found in prefetch buffer */
	return (kNoData);
}

/*******************************************************************************
* DataAvailRSN()                                                               *
*                                                                              *
*      Purpose: Determines if the requested sector is going to be available if *
*               we simply allow the drive to prefetch it (ie, the data WILL be *
*               available before prefetch space fills up.  We also make sure   *
*               that the sector was prefetched in the correct mode (this is    *
*               necessary for discs that mix data/audio tracks.                *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               sector - requested sector (in an abs block number...not MSF)   *
*               format - requested format of mode to read sector in            *
*                                                                              *
*      Returns: int8 - TRUE if the data will be available RSN; FALSE otherwise *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
uint32 DataAvailRSN(cdrom *cd, int32 sector, uint8 format)
{
	/* does the sector we want fall in the range of what we're prefetching?
	 * if so, is it the same format that it will be prefetched in?
	 * and are we currently prefetching? (this was needed to support on-the-fly
	 * ResizeBuffers while we're still prefetching)
     */
	return ((cd->cd_PrefetchStartOffset <= sector) &&
			(cd->cd_PrefetchEndOffset >= sector) &&
			(cd->cd_PrefetchSectorFormat == format) &&
			(cd->cd_State & CD_CURRENTLY_PREFETCHING));
}

/*******************************************************************************
* SanityCheckSector()                                                          *
*                                                                              *
*      Purpose: Verify that the sector header (and Bob's completion word)      *
*               match the MSF that we expect to see.  In theory, the only time *
*               that they should differ is when bit errors occur in the header *
*               data.                                                          *
*                                                                              *
*   Parameters: cd  - pointer to the cdrom device in question                  *
*               blk - index into the data buffer for the sector in question    *
*                                                                              *
*      Returns: uint8 - TRUE if there is an unrecoverable error in the header  *
*                       (ie, the ECC was unable to correct it).                *
*                                                                              *
*    Called by: CopyDAData()                                                   *
*                                                                              *
*******************************************************************************/
uint32 SanityCheckSector(cdrom *cd, int8 blk)
{
	uint32	expected = cd->cd_DataBlkHdr[blk].MSF;
	uint32	header = *(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[0] >> 8;

	/* verify that the header MSF matches what we thought we read */
	if (expected != header)
	{
		DBUG2(kPrintDeath, ("M2CD:  >DEATH<  Header (%06lx) does not match expected (%06lx)! (c=%08lx)\n",
			header, expected, *(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2340]));

#if 0 /* FIXME: MJN 6/10/96 -- is this right? */
		/* Do not return an error if it's a Mode2 sector.  This is due to the
		 * fact that data errors can occur in the header.  These are neither
		 * detectable, nor correctable.  Therefore, we must trust the firmware
		 * to return the correct sector (ie, land at the right place after a
		 * seek).
		 */
		if (cd->cd_DataBlkHdr[blk].format != XA_SECTOR)
#endif
			return (TRUE);
	}

	return (FALSE);
}

/*******************************************************************************
* CopySectorData()                                                             *
*                                                                              *
*      Purpose: Copies the requested data to the users buffer; taking into     *
*               account the specified blockLength and the lengths of the sync, *
*               header, data, aux/ecc, compword, and subcode.  NOTE:  Returns  *
*               zero for the MKE error byte, if requested.                     *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               dst    - address of user's buffer                              *
*               src    - address in prefetch buffer to copy from               *
*               blkLen - block length as specified in ioi_CmdOptions.          *
*               format - the Bob sector format                                 *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
void CopySectorData(cdrom *cd, uint8 *dst, uint8 *src, int32 blkLen, uint8 format)
{
	uint8 syncMark[12] = {	0x00, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0x00 };	/* generic sync mark */
	uint32 compword = 0L;

	switch (blkLen)
	{
		case 2048:
			if (format == M1_SECTOR)
				memcpy(dst, src+4L, 2048);		/* M1:		D            */
			else
				memcpy(dst, src+12L, 2048);		/* M2F1:	D            */
			break;
		case 2052:
			memcpy(dst, src, 2052);				/* M1:		H+D          */
			break;
		case 2060:
			memcpy(dst, src, 2060);				/* M2F1:	H+D          */
			break;
		case 2324:
			memcpy(dst, src+12L, 2324);			/* M2F2:	D            */
			break;
		case 2328:
			memcpy(dst, src+12L, 2328);			/* M2F1(or2):	D+A      */
			break;
		case 2336:
			if (format == M1_SECTOR)
				memcpy(dst, src+4L, 2336);		/* M1:		D+A          */
			else
				memcpy(dst, src, 2336);			/* M2F2:	H(s)+D       */
			break;
		case 2340:
			memcpy(dst, src, 2340);				/* M1,M2F1,M2F2:	H+D+A */
			break;
		case 2449:
			*(uint8 *)(dst + 2448L) = 0;		/* old MKE error byte */
			/* no break intentional */
		case 2448:
			/* subcode for M1, M2F1, M2F2, and Digital Audio */
			CopySyncedSubcode(cd, dst + 2352L);
			/* no break intentional */
		case 2352:
		case 2353:
			if (format == DA_SECTOR)
			{
				memcpy(dst, src, 2352);				/* Digital Audio */
				if (blkLen == 2353)
					*(uint8 *)(dst + 2448L) = 0;	/* old MKE error byte */
			}
			else
			{
				/* M1,M2F1,M2F2:	sync + header + data + aux/ecc */
				memcpy(dst, syncMark, sizeof(syncMark));
				memcpy(dst+12L, src, 2340);
			}
			break;
		case 2440:
			compword = 4L;
			/* no break intentional */
		case 2436:
			/* M1,M1F1,M1F2:	hdr + data + aux/ecc (+ compword?) + subcode */
			memcpy(dst, src, 2340L + compword);
			CopySyncedSubcode(cd, dst + 2340L + compword);
			break;
	}
}



/*******************************************************************************
* SubcodeSyncedUp()                                                            *
*                                                                              *
*      Purpose: Determines if the subcode sync marks are currently "locked-in";*
*               and where the true start of the logical subcode blocks is.     *
*               The algorithm requires a minimum number of consecutive syncs   *
*               to be present (NUM_SYNCS_NEEDED_2_PASS) in order to decide     *
*               that the subcode is locked in.  In addition, there is a        *
*               maximum number of allowable consecutive missed syncs           *
*               (NUM_NOSYNCS_NEEDED_2_FAIL) before we decide that we've lost   *
*               sync.                                                          *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: uint32 - non-zero if we're sync'd-up; zero otherwise           *
*                                                                              *
*    Called by: CopyDAData()                                                   *
*                                                                              *
*******************************************************************************/
uint32 SubcodeSyncedUp(cdrom *cd)
{
	uint8	blk;
	uint8	*syncPtr, *nextPtr;
	uint32	x;

	/* NO_SUBCODE_LATENCY */
	/* must have enough VALID blocks before we can return subcode.
	 * if not enough available (yet), we return FALSE here, and that
	 * causes us to simply return zeros.  This allows FFWD/FREV to
	 * not have to wait for "enough VALID blocks"...ie, it eliminates
	 * some (albeit minor) latency effects.
	 * "plus 1" because the sync has a 98% chance of
	 * falling in the middle of the buffer block which
	 * means we'll need one additional VALID block
	 */
	if (!(cd->cd_State & CD_GOT_PLENTY_O_SUBCODE))
	{
		cd->cd_State &= ~CD_SUBCODE_SYNCED_UP;
		return (FALSE);
	}

	/* if we're not sync'd-up, try to get sync'd */
	if (!(cd->cd_State & CD_SUBCODE_SYNCED_UP))
	{
		/* start with the current "pseudo-block",
		 * start looking from beginning of 98-byte "pseudo-block"
		 * location just beyond this subcode buffer block
		 */
		blk = cd->cd_SubcodeReadIndex;
		syncPtr = cd->cd_SubcodeBlkHdr[blk].buffer;
		nextPtr = syncPtr + kSubcodeBlkSize;

		/* look for sync mark in this block */
		while(!(*syncPtr & SYNC_MARK) && (syncPtr < nextPtr))
			syncPtr++;

		/* have read into the next block without finding a sync mark? */
		if (syncPtr == nextPtr)
		{
			/* free up this block (so it can be used by the subcode engine)
			 * ...and wrap around to top of buffer if needed
			 */
			cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
			cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);

			/* sync not found in first block (need to return here so we
			 * don't digest too much subcode and get out of sync)
			 */
			return (FALSE);
		}
		else
		{
			/* look for the first 1-to-0 transition of the P-bit */
			while (*syncPtr & SYNC_MARK)
				syncPtr++;

			/* backup one so we actually point to SyncByte2 (S1 of "S0,S1") */
			syncPtr--;

			/* at this point syncPtr points to byte 2 of a REAL 98-byte
			 * subcode packet (_potentially_)
			 */

			/* if this is true, then the "true start" of the subcode is in
			 * the next buffer block
			 */
			if (syncPtr >= nextPtr)
			{
				/* free up this blk (so it's avail to the subcode engine) */
				cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
				cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);

				/* sync not found in first block (need to return here so we
				 * don't digest too much subcode and get out of sync)
				 */
				return (FALSE);
			}

			/* start of true subcode block */
			cd->cd_SubcodeTrueStart = syncPtr;

			/* start looking one-block-out
 			 * ...only look in blks we'd actually descramble the subcode from
			 * ...and bump nextPtr to next block each time
			 */
			for (x = 1, nextPtr = syncPtr + kSubcodeBlkSize;
				x < NUM_SYNCS_NEEDED_2_PASS;
				x++, nextPtr += kSubcodeBlkSize)
			{
				/* check to make sure to didn't loop in the buffer pool */
				if (nextPtr >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
					nextPtr -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;

				/* free up this block, and update the index */
				if (!(*nextPtr & SYNC_MARK))
				{
					cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
					cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);

					/* subcode sync not locked-in yet */
					return (FALSE);
				}
			}

			/* clear the "missed sync mark" count */
			gSubcodeSyncMissCount = 0;

			/* apparently we detected enough syncs to pass */
			cd->cd_State |= CD_SUBCODE_SYNCED_UP;
		}
	}
	else							/* try to determine if we've lost sync */
	{
		/* start looking one-block-out
		 * ...only look in blks that we'd actually descramble the subcode from
		 * ...and bump nextPtr to next block each time
		 */
		for (x = 1, nextPtr = cd->cd_SubcodeTrueStart + kSubcodeBlkSize;
			x < NUM_SYNCS_NEEDED_2_PASS;
			x++, nextPtr += kSubcodeBlkSize)
		{
			/* check to make sure to didn't loop in the buffer pool */
			if (nextPtr >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
				nextPtr -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;

			/* did we miss a sync mark? */
			gSubcodeSyncMissCount = (*nextPtr & SYNC_MARK) ? 0 : (gSubcodeSyncMissCount + 1);

			/* did we meet/exceed the # of consecutive missed syncs? */
			if (gSubcodeSyncMissCount >= NUM_NOSYNCS_NEEDED_2_FAIL)
			{
				/* subcode is now assumed to be "out-of-sync" */
				cd->cd_State &= ~CD_SUBCODE_SYNCED_UP;
				return (FALSE);
			}
		}
		/* subcode is still synced up... */
	}

	return (cd->cd_State & CD_SUBCODE_SYNCED_UP);
}

/*******************************************************************************
* Subcode support...                                                           *
*                                                                              *
*   The #define's below cause the compiler to build of table of offsets used   *
*   for subcode descrambling.  The final table is SubcodeOffsetTable[], a      *
*   table of 96 bytes which is used to lookup descrambled entries within three *
*   sequential (scrambled) subcode blocks.                                     *
*                                                                              *
*   The actually descrambling occurs when the requested subcode is copied out  *
*   to the client's buffer within CopyDescrambledSubcode().                    *
*                                                                              *
*******************************************************************************/

#define SUBCODE_BLOCK_OFFSET(x)			( ((x) < 96) ? (x) : ( ((x)<(96+96)) ? (x)+2 : (x)+4 ))

#define PACKDELAY( Pack, Byte ) 		Byte+(24*(7-Pack))

#define ONEPACKBYTE( IP, Pack, Byte )   SUBCODE_BLOCK_OFFSET( 24 * IP + PACKDELAY( Pack, Byte ))

#define ONEFULLPACK( IP ) \
			ONEPACKBYTE( IP, 7, 0 ), \
			ONEPACKBYTE( IP, 5,18 ), \
			ONEPACKBYTE( IP, 2, 5 ), \
			ONEPACKBYTE( IP, 0,23 ), \
			ONEPACKBYTE( IP, 3, 4 ), \
			ONEPACKBYTE( IP, 5, 2 ), \
			ONEPACKBYTE( IP, 1, 6 ), \
			ONEPACKBYTE( IP, 0, 7 ), \
			ONEPACKBYTE( IP, 7, 8 ), \
			ONEPACKBYTE( IP, 6, 9 ), \
			ONEPACKBYTE( IP, 5,10 ), \
			ONEPACKBYTE( IP, 4,11 ), \
			ONEPACKBYTE( IP, 3,12 ), \
			ONEPACKBYTE( IP, 2,13 ), \
			ONEPACKBYTE( IP, 1,14 ), \
			ONEPACKBYTE( IP, 0,15 ), \
			ONEPACKBYTE( IP, 7,16), \
			ONEPACKBYTE( IP, 6,17), \
			ONEPACKBYTE( IP, 6, 1), \
			ONEPACKBYTE( IP, 4,19), \
			ONEPACKBYTE( IP, 3,20), \
			ONEPACKBYTE( IP, 2,21), \
			ONEPACKBYTE( IP, 1,22), \
			ONEPACKBYTE( IP, 4, 3)

static uint32 SubcodeOffsetTable[96] = { ONEFULLPACK(0), ONEFULLPACK(1), ONEFULLPACK(2), ONEFULLPACK(3) } ;

#define SubcodeBitsQRSTUVW	0x7F

/*******************************************************************************
* CopyDescrambledSubcode()                                                     *
*                                                                              *
*      Purpose: Utility routine to copy the descrambled subcode to a specified *
*               location in the user's buffer.                                 *
*                                                                              *
*   Parameters: cd  - pointer to cdrom device in question                      *
*               dst - address to copy 96 bytes of subcode to.                  *
*               src - start of current subcode area                            *
*                                                                              *
*    Called by: CopySyncedSubcode().                                           *
*                                                                              *
*******************************************************************************/
void CopyDescrambledSubcode(cdrom *cd, uint8 *dst, uint8 *src)
{
	uint8	*offset;
	int8	x;

	for (x = 0 ; x < 96 ; x++)
	{
		/* "plus one" to skip over sync byte */
		offset = src + SubcodeOffsetTable[x] + 1;

		/* check to make sure to didn't loop in the buffer pool */
		if (offset >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
			offset -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;

		dst[x] = SubcodeBitsQRSTUVW & *offset;
	}
}

/*******************************************************************************
* CopySyncedSubcode()                                                          *
*                                                                              *
*      Purpose: Utility routine to copy the synced-up (and descrambled)        *
*               subcode to a specified location in the user's buffer.  If the  *
*               subcode has not been synced-up (ie, locked-in), then we simply *
*               zero-fill the 96 bytes.                                        *
*                                                                              *
*   Parameters: cd  - pointer to cdrom device in question                      *
*               dst - address to copy 96 bytes of subcode to.                  *
*                                                                              *
*    Called by: CopyDAData(), CopySectorData().                                *
*                                                                              *
*******************************************************************************/
void CopySyncedSubcode(cdrom *cd, uint8 *dst)
{
	uint8 subBlk = cd->cd_SubcodeReadIndex;

	/* insure that no rogue reads cause a cache coherency problem for the DMA
	 * buffer data.
	 */
	SuperInvalidateDCache(cd->cd_SubcodeBlkHdr[subBlk].buffer, kSubcodeBlkSize);

	/* if we're locked-in and synchronized on subcode */
	if (SubcodeSyncedUp(cd))
	{
		/* SubcodeTrueStart gets set initially in SubcodeSyncedUp() */
		CopyDescrambledSubcode(cd, dst, cd->cd_SubcodeTrueStart);

		/* update start (2nd sync byte) of next subcode packet */
		cd->cd_SubcodeTrueStart += kSubcodeBlkSize;

		/* check to make sure to didn't loop in the buffer pool */
		if (cd->cd_SubcodeTrueStart >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
			cd->cd_SubcodeTrueStart -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;
	}
	else									/* clear all 96 bytes of subcode */
		memset(dst, 0, 96);
}



/*******************************************************************************
* EnableLCCDDMA()                                                              *
*                                                                              *
*      Purpose: Enables DMA channel(s) 0 and/or 1 for the 'current' and/or     *
*               'next' reg's.                                                  *
*                                                                              *
*   Parameters: channels  - which DMA channel(s) to affect?                    *
*               curORnext - which buf/len register pair(s) to affect?          *
*                                                                              *
*    Called by: InitPrefetchEngine()                                           *
*                                                                              *
*******************************************************************************/
void EnableLCCDDMA(uint8 channels)
{
	/* clear the interrupt in case one snuck thru, and is now pending
	 * ...then enable the lccd DMA FIRQ handler
	 * ...and then the CDE lccd DMA channel
	 */
	if (channels & DMA_CH0)
	{
		/* Also clear INT_SENT? */
		CDE_CLR(gCDEBase, CDE_INT_STS, CDE_CD_DMA1_DONE);
		EnableInterrupt(INT_CDE_DMAC4);
		CDE_SET(gCDEBase, CDE_CD_DMA1_CNTL, CDE_DMA_CURR_VALID |
										   CDE_DMA_NEXT_VALID |
										   CDE_DMA_GO_FOREVER);
	}
	if (channels & DMA_CH1)
	{
		/* Also clear INT_SENT? */
		CDE_CLR(gCDEBase, CDE_INT_STS, CDE_CD_DMA2_DONE);
		EnableInterrupt(INT_CDE_DMAC5);
		CDE_SET(gCDEBase, CDE_CD_DMA2_CNTL, CDE_DMA_CURR_VALID |
								   		   CDE_DMA_NEXT_VALID |
										   CDE_DMA_GO_FOREVER);
	}
}

/*******************************************************************************
* DisableLCCDDMA()                                                             *
*                                                                              *
*      Purpose: Disables DMA channel(s) 0 and/or 1 for the 'current' and/or    *
*               'next' reg's.                                                  *
*                                                                              *
*   Parameters: channels  - which DMA channel(s) to affect?                    *
*               curORnext - which buf/len register pair(s) to affect?          *
*                                                                              *
*    Called by: LCCDDeviceInit(), LCCDDriverDaemon(), StopPrefetching(),       *
*               RecoverFromDipir(), BuildDiscData(), ChannelZeroFIRQ().        *
*                                                                              *
*******************************************************************************/
void DisableLCCDDMA(uint8 channels)
{
	/* disable CDE lccd DMA channel
	 * ...then disable the lccd DMA FIRQ handler
	 */
	if (channels & DMA_CH0)
	{
		CDE_CLR(gCDEBase, CDE_CD_DMA1_CNTL, CDE_DMA_CURR_VALID | CDE_DMA_NEXT_VALID | CDE_DMA_GO_FOREVER);
		DisableInterrupt(INT_CDE_DMAC4);
		CDE_SET(gCDEBase, CDE_CD_DMA1_CNTL, CDE_DMA_RESET);
		CDE_CLR(gCDEBase, CDE_CD_DMA1_CNTL, 0x7FF);
	}
	if (channels & DMA_CH1)
	{
		CDE_CLR(gCDEBase, CDE_CD_DMA2_CNTL, CDE_DMA_CURR_VALID | CDE_DMA_NEXT_VALID | CDE_DMA_GO_FOREVER);
		DisableInterrupt(INT_CDE_DMAC5);
		CDE_SET(gCDEBase, CDE_CD_DMA2_CNTL, CDE_DMA_RESET);
		CDE_CLR(gCDEBase, CDE_CD_DMA2_CNTL, 0x7FF);
	}
}

/*******************************************************************************
* ConfigLCCDDMA()                                                              *
*                                                                              *
*      Purpose: Configures the 'current' or 'next' DMA registers for channel 0 *
*               or 1 with the specified buffer pointer and length.             *
*                                                                              *
*   Parameters: channel   - which DMA channel?                                 *
*               curORnext - which buf/len register pair?                       *
*               dst       - pointer to input buffer                            *
*               len       - length of input DMA transaction/buffer             *
*                                                                              *
*    Called by: InitPrefetchEngine(), ChannelZeroFIRQ(), ChannelOneFIRQ()      *
*                                                                              *
*******************************************************************************/
void ConfigLCCDDMA(uint8 channel, uint8 curORnext, uint8 *dst, int32 len, uint8 fromFIRQ)
{
	TOUCH(fromFIRQ);

	/* insure DMA'd data is not trounced by a cache write */
	SuperInvalidateDCache(dst, len);

	if (channel == DMA_CH0)
		if (curORnext == CUR_DMA)
		{
			CDE_WRITE(gCDEBase, CDE_CD_DMA1_CPAD, (uint32)dst);
			CDE_WRITE(gCDEBase, CDE_CD_DMA1_CCNT, len);
		}
		else
		{
			CDE_WRITE(gCDEBase, CDE_CD_DMA1_NPAD, (uint32)dst);
			CDE_WRITE(gCDEBase, CDE_CD_DMA1_NCNT, len);
		}
	else
		if (curORnext == CUR_DMA)
		{
			CDE_WRITE(gCDEBase, CDE_CD_DMA2_CPAD, (uint32)dst);
			CDE_WRITE(gCDEBase, CDE_CD_DMA2_CCNT, len);
		}
		else
		{
			CDE_WRITE(gCDEBase, CDE_CD_DMA2_NPAD, (uint32)dst);
			CDE_WRITE(gCDEBase, CDE_CD_DMA2_NCNT, len);
		}
}

/*******************************************************************************
* ChannelZeroFIRQ()                                                            *
*                                                                              *
*      Purpose: This routine provides the actual Prefetch Engine for sector    *
*               data.  Data is read off of the disc (when the driver envokes a *
*               Ppso) and this FIRQ routine handles all the prefetch buffer    *
*               block (and DMA register) handling needed to spool the data     *
*               'continuously' until the prefetch buffer pool fills up.        *
*                                                                              *
*    Called by: Called upon receiving a DMA EndOfLength interrupt for LCCD     *
*               channel 0.                                                     *
*                                                                              *
*******************************************************************************/
int32 ChannelZeroFIRQ(void)
{
	/* must cheat to get our device globals
	 * (Note that this won't work for multiple drives)
	 */
	cdrom	*cd = gDevice;
	uint8	curBlk = cd->cd_CurDataWriteIndex;
	uint8	nextBlk = cd->cd_NextDataWriteIndex;
	uint8	crcError = 0;

	/* a "Ppso" happens (in the daemon), then...
	 * an EOL interrupt occurs for the block pointed to by "curBlk"
	 */

	/* if these are the same, then we were out of buffer space the previous
	 * pass thru this FIRQ
	 */
	if (curBlk != nextBlk)
	{
		/* mark the previously filled sector buffer as valid */
		cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_VALID;

		/* update the block header's MSF and format fields */
		cd->cd_DataBlkHdr[curBlk].MSF = cd->cd_PrefetchCurMSF;
		cd->cd_DataBlkHdr[curBlk].format = cd->cd_PrefetchSectorFormat;
		cd->cd_DataBlkHdr[curBlk].seekmode = cd->cd_PrefetchSeekMode;

		/* update the MSF 'counter' */
		cd->cd_PrefetchCurMSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) + 1);

		/* if we're below the high water mark, check to see if this sector is
		 * errored...if so, wake up the daemon early
		 */
		crcError = (cd->cd_DataBlkHdr[curBlk].buffer[2343] & CRC_ERROR_BIT);

		/* set curBlk, so that the next time we're in here, we point to
		 * the right block
		 */
		curBlk = cd->cd_CurDataWriteIndex = nextBlk;

		/* point to 'next' data buffer block */
		nextBlk = cd->cd_NextDataWriteIndex = (nextBlk == (cd->cd_NumDataBlks-1)) ? 0 : (nextBlk + 1);

		/* is the next block available? */
		if (cd->cd_DataBlkHdr[nextBlk].state == BUFFER_BLK_FREE)
		{
			/* mark it as used, and re-direct the dma ptrs to this blk */
			cd->cd_DataBlkHdr[nextBlk].state = BUFFER_BLK_INUSE;
			ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength, TRUE);
			ClearInterrupt(INT_CDE_DMAC4);
		}
		else
		{
			/* since we adjusted nextBlk in order to test for a free block,
			 * we need to...point nextBlk back to the current block to "slip
			 * the bucket under the waterfall"
			 */
			cd->cd_NextDataWriteIndex = curBlk;

			/* update the MSF 'counter'
			 * NOTE:  This gets used in StopPrefetching() to pre-seek to the
			 *        next sector (in the case of sequential reads)
			 */
			cd->cd_PrefetchCurMSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) + 1);

			/* HALT DAMMIT!  NOTE: This will cause a dma overflow, ignore it */
			CDE_CLR(gCDEBase, CDE_CD_DMA1_CNTL, CDE_DMA_NEXT_VALID | CDE_DMA_GO_FOREVER);
			ClearInterrupt(INT_CDE_DMAC4);

			/* set the overrun flag, reset the prefetching flag */
			cd->cd_State |= CD_PREFETCH_OVERRUN;
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;

			/* notify daemon that prefetch buffers have filled */
			if (cd->cd_DaemonTask)
				SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);

			return (0);
		}
	}
	else
	{
		/* Since "bucket mode" is disabled, then we'll only make one more
		 * pass thru this FIRQ after curBlk == nextBlk...since we cleared
		 * out NEXT_DMA ptr/len when that occured.  The Cur/Next
		 * DataWriteIndeces get updated to point to the first 2 blocks that
		 * will be freed-up upon _any_ data digestion.
		 *
		 * Optionally, in addition to this, in the digest code, upon detecting
		 * that there are 2 free blocks, we mark the cur/next blocks to
		 * INUSE, InitPrefetchEngine(), and then restart the engine.
		 */

		/* disable channel zero dma now that we've locked in the last buf blk */
		DisableLCCDDMA(DMA_CH0);
		ClearInterrupt(INT_CDE_DMAC4);

		/* mark the last sector block in the prefetch buffer as valid */
		cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_VALID;

		/* update the block header's MSF and format fields */
		cd->cd_DataBlkHdr[curBlk].MSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) - 1);
		cd->cd_DataBlkHdr[curBlk].format = cd->cd_PrefetchSectorFormat;

		/* update cur/next DataWriteIndeces */
		/* ...first block containing valid data (first to be digested) */
		cd->cd_CurDataWriteIndex = (curBlk == (cd->cd_NumDataBlks-1)) ? 0 : (curBlk + 1);

		/* point to 'next' data buffer block (ie, the new CurWriteIndex + 1) */
		cd->cd_NextDataWriteIndex = (cd->cd_CurDataWriteIndex == (MAX_NUM_DATA_BLKS-1)) ? 0 : (cd->cd_CurDataWriteIndex + 1);
	}

	gNumDataBlksInUse = (cd->cd_DataReadIndex < cd->cd_CurDataWriteIndex) ?
						((uint32)cd->cd_CurDataWriteIndex - (uint32)cd->cd_DataReadIndex) :
						(((uint32)cd->cd_CurDataWriteIndex + (uint32)cd->cd_NumDataBlks) - (uint32)cd->cd_DataReadIndex);

	if (cd->cd_DaemonTask &&
		((gNumDataBlksInUse > gHighWaterMark) ||
		(crcError && (gNumDataBlksInUse <= gHighWaterMark))) &&
		(cd->cd_State & CD_READ_IOREQ_BUSY))
	{
		/* notify daemon that we need to copy back data (or an error occured) */
		SuperInternalSignal(cd->cd_DaemonTask, cd->cd_DaemonSig);
	}

	return (0);
}

/*******************************************************************************
* ChannelOneFIRQ()                                                             *
*                                                                              *
*      Purpose: This routine provides the actual Subcode Engine for sector     *
*               subcode data; and handles all the subcode buffer block (and    *
*               DMA register) handling needed to spool the subcode info        *
*               'continuously' until the subcode buffer fills up.              *
*                                                                              *
*               NOTE:  The Subcode Engine gets stopped by the same mechanism   *
*                      that stops the Prefetch Engine; and the Subcode Engine  *
*                      can "overflow" into the BUCKET block to its heart's     *
*                      content until a stop/pause/etc. is issued to close the  *
*                      valve (w/no problems).                                  *
*                                                                              *
*    Called by: Called upon receiving a DMA EndOfLength interrupt for LCCD     *
*               channel 1.                                                     *
*                                                                              *
*******************************************************************************/
int32 ChannelOneFIRQ(void)
{
	/* must cheat to get our device globals
	 * (Note that this won't work for multiple drives)
	 */
	cdrom	*cd = gDevice;
	uint8	curBlk = cd->cd_CurSubcodeWriteIndex;
	uint8	nextBlk = cd->cd_NextSubcodeWriteIndex;

	/* a "Ppso" happens (in the daemon), then...
	 * an EOL interrupt occurs for the block pointed to by "curBlk"
	 */

	/* if these are the same, then we were out of buffer space the
	 * previous pass thru this FIRQ
	 */
	if (curBlk != nextBlk)
	{
		/* mark the previously filled sector buffer as valid */
		cd->cd_SubcodeBlkHdr[curBlk*4].state = BUFFER_BLK_VALID;
		cd->cd_SubcodeBlkHdr[(curBlk*4)+1].state = BUFFER_BLK_VALID;
		cd->cd_SubcodeBlkHdr[(curBlk*4)+2].state = BUFFER_BLK_VALID;
		cd->cd_SubcodeBlkHdr[(curBlk*4)+3].state = BUFFER_BLK_VALID;

		/* set curBlk, so that the next time we're in here, we point to
		 * the right block
		 */
		curBlk = cd->cd_CurSubcodeWriteIndex = nextBlk;

		/* point to 'next' subcode buffer block */
		nextBlk = cd->cd_NextSubcodeWriteIndex = (nextBlk == ((cd->cd_NumSubcodeBlks/4)-1)) ? 0 : (nextBlk + 1);

		/* is the next block available? */
		if ((cd->cd_SubcodeBlkHdr[nextBlk*4].state == BUFFER_BLK_FREE) &&
			(cd->cd_SubcodeBlkHdr[(nextBlk*4)+1].state == BUFFER_BLK_FREE) &&
			(cd->cd_SubcodeBlkHdr[(nextBlk*4)+2].state == BUFFER_BLK_FREE) &&
			(cd->cd_SubcodeBlkHdr[(nextBlk*4)+3].state == BUFFER_BLK_FREE))
		{
			/* mark the next quad as in-use */
			cd->cd_SubcodeBlkHdr[nextBlk*4].state = BUFFER_BLK_INUSE;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+1].state = BUFFER_BLK_INUSE;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+2].state = BUFFER_BLK_INUSE;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+3].state = BUFFER_BLK_INUSE;

			/* re-direct the dma ptrs to this block
			 * note that we use lengths of 392 because the DMA h/w
			 * requires lengths/ptrs to be multiples of 8 ...so we cram 4
			 * logical buffer blocks (98 bytes each) into 1 physical block
			 */
			ConfigLCCDDMA(DMA_CH1, NEXT_DMA, cd->cd_SubcodeBuffer[nextBlk], 392, TRUE);
			ClearInterrupt(INT_CDE_DMAC5);
		}
		else
		{
			/* since we adjusted nextBlk in order to test for a free block,
			 * we need to...point nextBlk back to the current block to "slip
			 * the bucket under the waterfall"
			 */
			nextBlk = cd->cd_NextSubcodeWriteIndex = curBlk;

			/* indicate that the bucket has been used */
			cd->cd_SubcodeBlkHdr[nextBlk*4].state = BUFFER_BLK_BUCKET;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+1].state = BUFFER_BLK_BUCKET;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+2].state = BUFFER_BLK_BUCKET;
			cd->cd_SubcodeBlkHdr[(nextBlk*4)+3].state = BUFFER_BLK_BUCKET;

			/* next block unavailable
			 * leave nextDMA as is (this will cause any incoming DMA to
			 * loop around to the same buffer).  Anvil leaves the NextPtr/Len
			 * alone when copying them to the CurPtr/Len.
			 */

#if 0
			/* HALT DAMMIT!  NOTE: This will cause a dma overflow, ignore it */
			CDE_SET(gCDEBase, CDE_CD_DMA2_CNTL, CDE_DMA_RESET);
			CDE_CLR(gCDEBase, CDE_CD_DMA2_CNTL, 0x7FF);
#endif
			ClearInterrupt(INT_CDE_DMAC5);
		}
	}
	else
	{
		/* we don't need to do anything here, do we? */
		/* clear the "Go Forever" bit */
		CDE_CLR(gCDEBase, CDE_CD_DMA2_CNTL, 0x20);
		ClearInterrupt(INT_CDE_DMAC5);
	}

	return (0);
}

#if VERIFY_CRC_CHECK
uint32 bufcmp(uint8 *buf1, uint8 *buf2)
{
	uint32	p = 0;
	uint32	q = 0;
	uint32	qx;
	uint32	row, col;
	uint32	err = 0;

	Superkprintf("P  0.........1.........2.........3.........4.........5.........6.........7.........8.....\n");
	Superkprintf("   01234567890123456789012345678901234567890123456789012345678901234567890123456789012345\n");

	for (row = 0; row < 24; row++)
	{
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col++)
		{
			if (buf1[p] == buf2[p])
				Superkprintf(".");
			else
				Superkprintf("*");
			p++;
		}
		Superkprintf("\n");
	}

	Superkprintf("   +---------+---------+---------+---------+---------+---------+---------+---------+-----\n");

	for (; row < 26; row++)
	{
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col++)
		{
			if (buf1[p] == buf2[p])
				Superkprintf(".");
			else
				Superkprintf("*");
			p++;
		}
		Superkprintf("\n");
	}


	Superkprintf("\n\nQ  0.........1.........2.........3.........4.........5.........6.........7.........8..... ....\n");
	Superkprintf("   01234567890123456789012345678901234567890123456789012345678901234567890123456789012345 6789\n");

	for (row = 0; row < 26; row++)
	{
		q = 86*row;
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col += 2)	/* columns done in pairs */
		{
			if (buf1[q] == buf2[q])			/* The even columns */
				Superkprintf(".");
			else
				Superkprintf("*");
			q++;
			if (buf1[q] == buf2[q])			/* The odd columns */
				Superkprintf(".");
			else
				Superkprintf("*");
			q += 87;						/* bump to next even col index */
			if (q >= 2236)					/* did we wrap? */
				q -= 2236;
		}
		if ((row/10)*10 == row)
			Superkprintf("+");
		else
			Superkprintf("|");
		for (; col < 90; col++)
		{
			qx = (col>>1)*52 + (col & 0x01) + (row << 1);
			if (buf1[qx] == buf2[qx])
				Superkprintf(".");
			else
			{
				Superkprintf("*");
				err++;
			}
		}
		Superkprintf("\n");
	}
	return (err);
}

uint32 GenChecksum(uint32 *buf)
{
	int32	x;
	uint32	checksum = 0;

	for (x = 0; x < 512; x++)
		checksum ^= buf[x];

	return (checksum);
}
#endif

/*******************************************************************************
* main() - previously createLCCDDriver()                                       *
*                                                                              *
*      Purpose: Creates the LCCD driver                                        *
*                                                                              *
*      Returns: Item number of LCCD driver                                     *
*                                                                              *
*    Called by: kernel                                                         *
*                                                                              *
*******************************************************************************/

int32 main(void)
{
	Item	drvrItem;

	drvrItem = Check4Drive();
	CHK4ERR(drvrItem, ("M2CD:  No drive available\n"));

	drvrItem = CreateItemVA(MKNODEID(KERNELNODE, DRIVERNODE),
		TAG_ITEM_NAME,			"cdrom",
		CREATEDRIVER_TAG_DISPATCH,	LCCDDriverDispatch,
		CREATEDRIVER_TAG_CREATEDRV,	LCCDDriverInit,
		CREATEDRIVER_TAG_DELETEDRV,	LCCDDriverDelete,
		CREATEDRIVER_TAG_CREATEDEV,	LCCDDeviceInit,
		CREATEDRIVER_TAG_DELETEDEV,	LCCDDeviceDelete,
		CREATEDRIVER_TAG_ABORTIO,	LCCDDriverAbortIO,
		CREATEDRIVER_TAG_MODULE,	FindCurrentModule(),
		TAG_END);
	CHK4ERR(drvrItem, ("M2CD:  Couldn't create CD-ROM driver\n"));

	return OpenItem(drvrItem, 0);
}
