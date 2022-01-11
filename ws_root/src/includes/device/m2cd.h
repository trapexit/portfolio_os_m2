#ifndef __DEVICE_M2CD_H
#define __DEVICE_M2CD_H


/******************************************************************************
**
**  @(#) m2cd.h 96/09/27 1.18
**
**  Defines, enums, etc. for LCCD CD-ROM driver.
**
**  NOTE:   This is header is >INTERNAL ONLY< and should only be referenced
**          by the LCCD CD-ROM driver.  All applications needing to use the
**          cd-rom driver API should include <device/cdrom.h>
**
******************************************************************************/


#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif

#ifndef __KERNEL_IO_H
#include <kernel/io.h>
#endif

/*
#ifndef __KERNEL_DRIVER_H
#include <kernel/driver.h>
#endif
*/

#ifndef __DEVICE_CDROM_H
#include <device/cdrom.h>
#endif

/* device id stuff */
#define LCCD_MAX_NUM_DEV	    1
#define LCCD_MANU_ID		    0x1000
#define LCCD_MANU_DEV_NUM	    0x0050

/* LCCD Firmware Commands */
/* (overhead) */
#define CD_LED			    0x01
#define CD_MECHTYPE		    0x21
/* (format) */
#define CD_SETSPEED		    0x04
#define CD_SECTORFORMAT		    0x06
#define CD_SENDBYTES		    0x07
/* (transport) */
#define CD_PPSO			    0x08
#define CD_SEEK			    0x09
/* (reporting) */
#define CD_CIPREPORTEN		    0x0B
#define CD_QREPORTEN		    0x0C
#define CD_SWREPORTEN		    0x0D
#define CD_CIPREPORT		    0x1B
#define CD_QREPORT		    0x1C
#define CD_SWREPORT		    0x1D
/* (unused) */
#define CD_SPINDOWNTIME		    0x05
#define CD_SENDBYTES		    0x07
#define CD_CHECKWO		    0x0A
#define CD_TRISTATE		    0x0F
#define CD_READROM		    0x20
#define CD_READID		    0x83
/* (CDE dipir byte bug thing) */
#define CDE_DIPIR_BYTE		    0x40

enum CIPStates {
    CIPNone = 0xFF,	/* Invalid CIPState (Only Defined In Driver) */
    CIPOpen = 0,	/* 0 */
    CIPStop,		/* 1 (this is stable in drawer, transient in clamshell mechanism) */
    CIPPause,		/* 2 */
    CIPPlay,		/* 3 */
    CIPOpening,		/* 4 */
    CIPStuck,		/* 5 */
    CIPClosing,		/* 6 */
    CIPStopAndFocused,	/* 7 (this is stable in clamshell, transient in drawer mechanism) */
    CIPStopping,	/* 8 */
    CIPFocusing,	/* 9 */
    CIPFocusError,	/* a */
    CIPSpinningUp,	/* b */
    CIPUnreadable,	/* c */
    CIPSeeking,		/* d */
    CIPSeekFailure,	/* e */
    CIPLatency		/* f */
};

/* dma, firq stuff */
#define DIPIR_FIRQ_PRIORITY	    215
#define CDSFL_FIRQ_PRIORITY	    210
#define CH0_DMA_FIRQ_PRIORITY	    205
#define CH1_DMA_FIRQ_PRIORITY	    200

enum DMAChannelsEtc {
    DMA_CH0 = 0x01,
    DMA_CH1 = 0x02,
    CUR_DMA = 0x04,
    NEXT_DMA = 0x08
};

/* internal buffer stuff */
#define MAX_NUM_SNARF_BLKS	    50
#define MIN_NUM_DATA_BLKS	    6
#define MAX_NUM_DATA_BLKS	    19
#define MIN_NUM_SUBCODE_BLKS	    12
#define MAX_NUM_SUBCODE_BLKS	    24

/* NOTE: Subcode blk count must be at least 3 blks more than data blk count...
 * AND must be a multiple of 4 (due to mod-8 alignment requirement in CDE)
 */

#define kSubcodeBufSizePlusSome	    2352L
#define kDataBlkSize		    2352L
#define kSubcodeBlkSize		    98L

/* NOTE:  The sum of (kStandardSpaceSize + kExtendedSpaceSize) MUST NOT EXCEED the
 *        size of the dipir shared buffer (currently 48K).
 */
#define kStandardSpaceSize	    ((MAX_NUM_SUBCODE_BLKS * kSubcodeBlkSize) +\
				    (MIN_NUM_DATA_BLKS * kDataBlkSize))
#define kExtendedSpaceSize	    ((MAX_NUM_DATA_BLKS - MIN_NUM_DATA_BLKS) * \
				    kDataBlkSize)

#define kHighWaterBufferZone	    3
#define kLCCDMaxStatusLength	    12
#define kDataTrackTOCEntry	    0x04

/* speed & pitch settings (we use +/- 1% of normal for fast/slow pitch) */
#define kPPct000		    0x00
#define kPPct050		    0x32
#define kNPct050		    0xCE
#define kPPct010		    0x0A
#define kNPct010		    0xF6
#define kFineEn			    0x10
#define kSingleSpeed		    0x01
#define kDoubleSpeed		    0x02
#define k4xSpeed		    0x04
#define k6xSpeed		    0x06
#define k8xSpeed		    0x08

/* Data Availability Flags */
#define CRC_ERROR_BIT		    0x02
#define kNoData			    -1
#define kBadData		    -2
#define kNoSubcode		    -3
#define kResyncSeek		    -4

/* Status FIFO Empty command for CDE */
#define kSFEhandshake   0x180

/* DBUG flags */
#define kPrintGeneralStatusStuff    0x00000001
#define kPrintQtyNPosOfSectorReqs   0x00000002
#define kPrintSendCmdToDevStuff     0x00000004
#define kPrintDataAvailNowResponse  0x00000008
#define kPrintECCStats              0x00000010
#define kPrintDescrambledSubcode    0x00000020
#define kPrintSendCompleteIO        0x00000100
#define kPrintActionMachineStates   0x00000200
#define kPrintSignalAwakeSleep      0x00000400
#define kPrintDeviceStates          0x00000800
#define kPrintStatusResponse        0x00001000
#define kPrintTOCAddrs              0x00002000
#define kPrintDipirStuff            0x00004000
#define kPrintVariPitch             0x00008000
#define kPrintMegaECCStats          0x00010000
#define kPrintCmdOptions            0x00020000
#define kPrintWarnings              0x00040000
#define kPrintDeath                 0x00080000
#define kPrintStatusWord            0x00100000
#define kPrintClamshell             0x00200000
#define kPrintSendCmdWithWaitState  0x00400000

/* TOC Building Flags */
#define TOC_GOT_A0		    0x01
#define TOC_GOT_A1		    0x02
#define TOC_GOT_A2		    0x04

/* Cd_State Bits */
#define CD_DOOR_OPEN_SWITCH	    0x00000001	    /* current state of open switch		   */
#define CD_DOOR_CLOSED_SWITCH	    0x00000002	    /* current state of close switch		   */
#define CD_USER_SWITCH		    0x00000004	    /* current state of user switch		   */
#define CD_RSRV1_SWITCH		    0x00000008	    /* reserved for future use                      */

#define CD_DIPIRING		    0x00000010	    /* are we dipiring a device?                    */
#define CD_GOTO_DIPIR		    0x00000020	    /* goto CL_DIPIR state ASAP                     */

#define CD_NO_DISC_PRESENT	    0x00000100	    /* no disc is present                           */
#define CD_UNREADABLE		    0x00000200	    /* an unreadable disc is present		   */
#define CD_CACHED_INFO_AVAIL	    0x00000400	    /* initial STATUS and DISCDATA available	   */
#define CD_DEVICE_ONLINE	    0x00000800	    /* good disc is inserted, can process CMD_READ */

#define CD_DEVICE_ERROR		    0x00004000	    /* device returned Stuck, FocusFail, Unreadable, SeekFailure */

#define CD_READING_TOC_INFO	    0x00008000	    /* currently reading a (session's) TOC	   */
#define CD_GOT_ALL_TRACKS	    0x00010000	    /* we read all the tracks for the current TOC  */
#define CD_READ_NEXT_SESSION	    0x00020000	    /* we saw an 0x05/0xB0 (multisession) entry	   */
#define CD_READING_INITIAL_TOC	    0x00040000	    /* are we reading the 1st session's TOC?	   */

#define CD_SNARF_OVERRUN	    0x00080000	    /* the snarf buffer ran out of snarf blocks	   */
#define CD_PREFETCH_OVERRUN	    0x00100000	    /* the prefetch space has been filled	   */

#define CD_PREFETCH_SUBCODE_ENABLED 0x00200000	    /* is subcode requested for this read?	   */
#define CD_CURRENTLY_PREFETCHING    0x00400000	    /* are we currently reading data (PLAYing)?	   */
#define CD_SUBCODE_SYNCED_UP	    0x00800000	    /* is the subcode currently "locked-in"?	   */
#define CD_GONNA_HAVE_TO_STOP	    0x01000000	    /* have we not fully stopped prefetching?	   */

#define CD_DRAWER_MECHANISM	    0x02000000	    /* presently shows if a SW_USER was pressed	   */
#define CD_USING_MIN_BUF_SPACE	    0x04000000	    /* are we using "minimum" memory usage mode?   */
#define CD_READ_IOREQ_BUSY	    0x08000000	    /* currently processing CMD_READ (re: gHWM)	   */
#define CD_DISC_IS_CDI		    0x10000000	    /* is it a CD-I disc?                          */
#define CD_DISC_IS_CDI_WITH_AUDIO   0x20000000	    /* is it a CD-I disc with audio tracks?        */
#define CD_ALREADY_RETRIED_TOC_READ 0x40000000      /* have we already made a 2nd attempt?         */
#define CD_GOT_PLENTY_O_SUBCODE	    0x80000000	    /* got enough subcode to determine "synced?"   */

/* functionality bits */
#define CD_SUPPORTS_SCANNING	    0x00000001      /* does the drive support scan-mode seeking?   */
#define CD_SUPPORTS_4X_MODE	    0x00000002      /* does the drive support 4x speed             */
#define CD_SUPPORTS_6X_MODE	    0x00000004      /* does the drive support 6x speed             */
#define CD_SUPPORTS_8X_MODE	    0x00000008      /* does the drive support 8x speed             */

/* subcode stuff */
#define SYNC_MARK		    0x80
#define NUM_SYNCS_NEEDED_2_PASS	    3
#define NUM_NOSYNCS_NEEDED_2_FAIL   7

/* mechanism-type stuff */
#define kClamWithClampMechanism	    0x00
#define kClamWithBallMechanism	    0x01
#define kDrawerMechanism	    0x02

/* Drive Functionality stuff */
#define kDFScanSeek		    0x01
#define kDF4xMode		    0x02
#define kDF6xMode		    0x04
#define kDF8xMode		    0x08

/* error stuff */
#define MakeCDErr(svr,class,err)    MakeErr(ER_DEVC,((Make6Bit('C')<<6)|Make6Bit('D')),svr,ER_E_SSTM,class,err)
/* generic system-type errors */
#define kCDErrAborted		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_Aborted)
#define kCDErrBadArg		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadIOArg)
#define kCDErrBadCommand	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadCommand)
#define kCDErrBadUnit		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_BadUnit)
#define kCDErrNotPrivileged	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NotPrivileged)
#define kCDErrSoftErr		    MakeCDErr(ER_SEVERE, ER_C_STND, ER_SoftErr)
#define kCDErrNoMemAvail	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NoMem)
#define kCDErrNoLCCDDevices	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_NoHardware)
#define kCDErrIOInProgress	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_IONotDone)
#define kCDErrIOIncomplete	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_IOIncomplete)
#define kCDErrDeviceOffline	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_DeviceOffline)
#define kCDErrDeviceError	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_DeviceError)
#define kCDErrMediaError	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_MediaError)
#define kCDErrEndOfMedium	    MakeCDErr(ER_SEVERE, ER_C_STND, ER_EndOfMedium)
/* LCCD-specific error messages */
#define kCDErrDaemonKilled	    MakeKErr(ER_SEVERE, ER_C_NSTND,ER_Kr_TaskKilled)
#define kCDErrSnarfBufferOverrun    kCDErrSoftErr
#define kCDErrModeError		    kCDErrBadArg
#define kCDNoErr		    0

/* used in state machines' return values */
#define kCmdNotYetComplete	    0
#define kCmdComplete		    1

#define kMajorDevStateMask	    0x00F0
#define kCmdStateMask		    0x0F00
#define kSwitchMask		    0x07

enum CommandStates {
    kSendCmd =		0x0000,
    kWait4Tag =		0x0100,
    kWait4CIPState =	0x0200
};

enum DeviceStates {
    DS_INIT_REPORTS = 0x10,
	IR_START = 0x10,
	IR_SWEN,
	IR_QDIS,
	IR_MECH,
	IR_CIPEN,
    DS_OPEN_DRAWER = 0x20,
    DS_CLOSE_DRAWER = 0x30,
	CL_START = 0x30,
	CL_CLOSE,
	CL_DIPIR,
    DS_BUILDING_DISCDATA = 0x40,
	BDD_START = 0x40,
	BDD_PAUSE,
	BDD_SPEED,
	BDD_FORMAT,
	BDD_SEEK,
	BDD_PLAY,
	BDD_QEN,
	BDD_QDIS,
	BDD_CHK4MULTI,
	BDD_SEEK2PAUSE,
    DS_PROCESS_CLIENT_IOREQS = 0x50,
	RD_START = 0x50,
	RD_VARIABLE_PITCH,
	RD_LOOP,
	RD_RESTART,
	RD_SPEED,
	RD_FORMAT,
	RD_SEEK,
	RD_PREPARE,
	RD_PLAY,
	SQ_START = 0x50,
	SQ_QNOW,
	SQ_UNIQUE_QRPT,
	WO_START = 0x50,
	WO_PAUSE,
	WO_SPEED,
	WO_SEEK,
	WO_PLAY,
	WO_CHECKWO,
	WO_PAUSE2,
	WO_SPEED2,
	PT_START = 0x50,
	PT_DONE,
    DS_STOP_PREFETCHING = 0x60,
	PE_START = 0x60,
	PE_PAUSE,
    DS_DUCK_INTO_DIPIR = 0x70,
	DID_START = 0x70,
	DID_WAIT,
	DID_DONE,
    DS_RECOVER_FROM_DIPIR = 0x80,
	RFD_START = 0x80,
	RFD_CIPEN,
	RFD_PAUSE,
	RFD_RECOVER
};

/* These will need to go in "cdrom.h" if we decide to make RESIZE_BUFFERS public */
#define CDROM_DEFAULT_BUFFERS	    0
#define CDROM_MINIMUM_BUFFERS	    1
#define CDROM_STANDARD_BUFFERS	    2


enum SectorTypes {
    DA_SECTOR = 0x00,	    /* Values also correpsond to Bob's format eegister expectations */
    M1_SECTOR = 0x78,
    XA_SECTOR = 0xF8,	    /* Arbitrarily picked Mode2Form2 (since Form1/Form2 are now the "same" WRT EDC) */
    INVALID_SECTOR = 0xFF   /* invalid mode */
};

enum BufferBlockStates {
    BUFFER_BLK_FREE = 0,
    BUFFER_BLK_INUSE,
    BUFFER_BLK_VALID,
    BUFFER_BLK_BUCKET
};

typedef struct snarfBlock {
    struct snarfBlockHeader {
	uint8	state;
	uint8	size;
	uint8	reserved1;		/* these were added so that the snarfBlockHeader stuck will be */
	uint8	reserved2;		/* compatible with opera */
    } blkhdr;
    uint8 blkdata[kLCCDMaxStatusLength];    /* (the max that bob can return is 12 bytes or so, see spec.) */
#define DEVELOPMENT
#ifdef DEVELOPMENT
    uint32 dbgdata[4];
#endif
} snarfBlock;

typedef struct sectorBlockHeader {
    uint8   *buffer;
    uint8   state;			/* State Of Given Buffer Block (FREE, INUSE, Etc.) */
    uint8   format;			/* Format That This Sector Was Read In As (DA, M1, M2) */
    uint32  seekmode;			/* mode (scan vs. normal) that this sector was read in */
    uint32  MSF;
} sectorBlkHdr;

typedef uint8 subcodeBlock[392];
typedef struct subcodeBlockHeader {
    uint8   *buffer;
    uint8   state;
    uint8   reserved1;			/* these are here so that the subcodeBlkHdr struct will get crammed */
    uint8   reserved2;			/* into one 4-byte word.  without them, the struct is two words long. */
} subcodeBlkHdr;

#define MAX_NUM_MESSAGES	16

typedef struct cdrom
{
    Item	cd_DipirHandler;		/* Item associated Dipir Handler */
    Item	cd_CDSFLHandler;		/* Item Associated CDSFL Handler */
    Item	cd_Ch0FIRQ;			/* Item Associated W/Ch0 (Data) DMA Handler */
    Item	cd_Ch1FIRQ;			/* Item Associated W/Ch1 (Subcode) DMA Handler */
    IOReq	*cd_workingIOR;			/* pointer to ioReq currently being processed */
    List	cd_pendingIORs;			/* list of ioReq's to be processed */

    sectorBlkHdr    cd_DataBlkHdr[MAX_NUM_DATA_BLKS];		/* block headers for each data block in the data buffer */
    subcodeBlkHdr   cd_SubcodeBlkHdr[MAX_NUM_SUBCODE_BLKS];	/* block headers for each subcode block in the subcode buffer */
    snarfBlock	    cd_SnarfBuffer[MAX_NUM_SNARF_BLKS];		/* buffer pool for async snarf responses */

    subcodeBlock    *cd_SubcodeBuffer;		/* buffer pool for sector prefetch (subcode) engine */
						/* [NOTE: Each Physical Block contains 4 Logical Blocks] */
    uint8	*cd_SubcodeTrueStart;		/* actual starting addr in SubcodeBuffer of the subcode sync mark */
    uint8	*cd_SavedRecvPtr;		/* save off ptr to the client's recv buf so that we can manipulate the ptr */

    uint32	cd_SectorOffset;		/* offset of sector(S) to read during this CMD_READ ioRequest */
    uint32	cd_SectorsRemaining;		/* number of sectors left in this CMD_READ ioRequest */
    uint32	cd_BlockLength;			/* current setting of device block length */

    Item	cd_MonitorReplyPort;		/* port for monitor messaging */
    Item	cd_MonitorMsgPort;		/* port for monitor messaging */
    Item	cd_MonitorMsg[MAX_NUM_MESSAGES];
    Task	*cd_DaemonTask;			/* place to save daemon task for recover code */
    Task	*cd_DuckTask;			/* place to save current task for Ducky() */
    uint32	cd_DaemonSig;			/* place to save allocated signal for daemon task */
    uint32	cd_DuckSig;			/* place to save allocated signal for ducking task */
    uint32	cd_RecoverSig;			/* place to save allocated signal for ducking task */
    uint32	cd_Status;			/* returned in .ds_DeviceFlagWord */
    uint32	cd_State;			/* current state (flags) of device (i/o done, etc.) */
    uint32	cd_DriveFunc;			/* drive functionality bits */
    uint32	cd_DevState;			/* current state of device machine (the major state) */
    uint32	cd_SavedDevState;		/* place to save state while we stop the prefetch engine */
    uint32	cd_PrefetchStartOffset;		/* start of range of prefetched data */
    uint32	cd_PrefetchEndOffset;		/* end of range of prefetched data */
    uint32	cd_PrefetchCurMSF;		/* BCD MSF that is used to tag a DataBlkHdr with what should be its contents */
    uint32	cd_PrefetchSeekMode;		/* the mode (scan vs. normal) that we are currently reading in */
    uint32	cd_MediumBlockCount;		/* returned in .ds_DeviceBlockCount */

    CDROMCommandOptions cd_DefaultOptions;
    struct {
	uint8	speed;
	uint8	pitch;
	uint8	format;
	uint8	subcode;
    } cd_CurrentSettings;

    uint32	cd_NextSessionTrack;
    uint32	cd_TOC;

    SubQInfo	cd_LastQCode;			/* last valid Qcode (Lead-In Area) returned by a QRpt cached (TOC info) */

    CDDiscInfo	    cd_DiscInfo;		/* NOTE:  Do not change the order of these structs, or place anything */
    CDTOCInfo	    cd_TOC_Entry[100];		/*        between them.  These must be consistent with the struct     */
    CDSessionInfo   cd_SessionInfo;		/*        CDROM_Disc_Data.                                            */
    CDSessionInfo   cd_FirstSessionInfo;	/* The MSF of the end of the First Session */

    CDWobbleInfo    cd_WobbleInfo;		/* CheckWO() wobble info from disc */

    uint8	cd_BuildingTOC;
    uint8	cd_NumDataBlks;			/* current number of blocks in prefetch (data) space */
    uint8	cd_NumSubcodeBlks;		/* current number of blocks in subcode space */
    uint8	cd_DataReadIndex;		/* points to next valid data block */
    uint8	cd_CurDataWriteIndex;		/* points to current data block...used by data FIRQ */
    uint8	cd_NextDataWriteIndex;		/* points to next data block...used by data FIRQ */
    uint8	cd_SubcodeReadIndex;		/* points to next valid data block */
    uint8	cd_CurSubcodeWriteIndex;	/* points to current subcode block...used by subcode FIRQ */
    uint8	cd_NextSubcodeWriteIndex;	/* points to next subcode block...used by subcode FIRQ */
    uint8	cd_SnarfReadIndex;		/* Points To Next Valid Snarf Block...Used By Async Response Processor */
    uint8	cd_SnarfWriteIndex;		/* points to next available snarf block...used by snarf callback routine */
    uint8	cd_CurRetryCount;		/* current retry count for this sector */
    uint8	cd_PrefetchSectorFormat;	/* the sector format we're currently using to prefetch data (this gets copied into blkhdr.format) */
    uint8	cd_CmdByteReceived;		/* last received cmd tag byte from device */
    uint8	cd_CIPState;			/* last achieved state of device (received from a CIPReport)	 */
} cdrom;

#endif /* __DEVICE_M2CD_H */
