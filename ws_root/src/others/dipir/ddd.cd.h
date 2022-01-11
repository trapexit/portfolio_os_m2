/*
 * @(#) ddd.cd.h 96/04/04 1.12
 * Copyright 1995, The 3DO Company
 *
 * Definitions relating to CD-ROM-specific DDD commands.
 */

#define	DDDCMD_CD_GETFLAGS	101
#define	DDDCMD_CD_GETINFO	102
#define	DDDCMD_CD_GETTOC	104
#define	DDDCMD_CD_GETLASTECC	105
#define	DDDCMD_CD_GETWDATA	106
#define	DDDCMD_CD_FIRMCMD	107

/* These should be in discdata.h? */
#define	FRAMEperSEC		75
#define	SECperMIN		60

/*
 * Returned from DDDCMD_CD_GETINFO.
 */
typedef struct DiscInfo
{
	uint8	di_DiscId;
	uint8	di_FirstTrackNumber;
	uint8	di_LastTrackNumber;
	uint8	di_MSFEndAddr_Min;
	uint8	di_MSFEndAddr_Sec;
	uint8	di_MSFEndAddr_Frm;
	uint8	di_MaxSpeed;
	uint8	di_DefaultSpeed;
} DiscInfo;

/*
 * Returned from DDDCMD_CD_GETTOC.
 */
typedef struct TOCInfo 
{
	uint8	toc_AddrCntrl;
	uint8	toc_CDROMAddr_Min;
	uint8	toc_CDROMAddr_Sec;
	uint8	toc_CDROMAddr_Frm;
} TOCInfo;

/* Bits in toc_AddrCntrl */
#define ACB_AUDIO_PREEMPH	0x01
#define ACB_DIGITAL_COPIABLE	0x02
#define ACB_DIGITAL_TRACK	0x04
#define ACB_FOUR_CHANNEL	0x08

/*
 * Returned from DDDCMD_CD_GETFLAGS.
 */
#define	CD_WRITABLE		0x00000001	/* CD is writable */
#define	CD_MULTISESSION		0x00000002	/* CD is multisession */
#define	CD_NOT_3DO		0x00000004	/* CD is not 3DO format */

typedef int32 DeviceGetCDInfoFunction(DDDFile *fd, DiscInfo *di);
typedef int32 DeviceGetCDFlagsFunction(DDDFile *fd, uint32 *pFlags);
typedef int32 DeviceGetCDTOCFunction(DDDFile *fd, uint32 track, TOCInfo *ti);
typedef int32 DeviceGetLastECCFunction(DDDFile *fd, uint32 *pECC);
typedef int32 DeviceGetWDataFunction(DDDFile *fd, uint32 block, uint32 speed, 
	uint32 bufSize, uint32 totalSize,
	int32 (*Callback)(DDDFile *fd, void *callbackArg, void *buf, uint32 bufSize),
	void *callbackArg);
typedef int32 DeviceCDFirmCmdFunction(DDDFile *fd, uint8 *cmd, uint32 cmdLen,
	uint8 *resp, uint32 respLen, uint32 timeout);

extern DeviceGetCDInfoFunction	DeviceGetCDInfo;
extern DeviceGetCDFlagsFunction	DeviceGetCDFlags;
extern DeviceGetCDTOCFunction	DeviceGetCDTOC;
extern DeviceGetLastECCFunction	DeviceGetLastECC;
extern DeviceGetWDataFunction	DeviceGetWData;
extern DeviceCDFirmCmdFunction	DeviceCDFirmCmd;
