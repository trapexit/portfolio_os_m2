/*
 * @(#) ddd.h 96/01/10 1.17
 * Copyright 1995, The 3DO Company
 *
 * Definitions relating to Dipir Device Drivers (DDDs).
 */

typedef void *DDDFunction();

/*
 * Command table
 */
typedef struct DDDCmdTable 
{
	uint32 dddcmd_Cmd;
	DDDFunction *dddcmd_Func;
} DDDCmdTable;

/*
 * DDD representation
 */
typedef struct DDD 
{
	DDDCmdTable *ddd_CmdTable;
} DDD;


/*
 * DDD file descriptor.
 */
typedef struct DDDFile 
{
	uint32 fd_Version;		/* Version of DDDFile structure */
	void *fd_DriverData;		/* For use by device driver */
	struct DipirTemp *fd_DipirTemp;	/* Copy of dtmp (ptr to DipirTemp) */
	DipirHWResource *fd_HWResource;	/* HWResource being driven */
	DDD *fd_DDD;			/* Ptr to DDD (device driver) */
	uint32 fd_BlockSize;		/* Device block size */
	uint32 fd_Flags;		/* Flags (see below) */
	uint32 fd_AllocFlags;		/* Flags for DipirAlloc'd buffers */
	union				/* Volume label from the device */
	{
		ExtVolumeLabel *vl_VolumeLabel;
		TinyVolumeLabel *vl_TinyVolumeLabel;
	} vl;
	uint32 fd_RomTagBlock;		/* Block number of RomTag table */
	struct RomTag *fd_RomTagTable;	/* Ptr to RomTag table */
	uint32 fd_RTTMapLen;		/* Length of RomTag table, if mapped */
} DDDFile;

#define	fd_VolumeLabel		vl.vl_VolumeLabel
#define	fd_TinyVolumeLabel	vl.vl_TinyVolumeLabel

/* Bits in fd_Flags */
#define	DDD_TINY	0x001
#define	DDD_RTTALLOC	0x002
#define	DDD_SECURE	0x004

typedef struct DeviceInfo 
{
	uint32 di_Version;
	uint32 di_BlockSize;
	uint32 di_FirstBlock;
	uint32 di_NumBlocks;
} DeviceInfo;

/*
 * DDD commands.
 */
#define	DDDCMD_OPEN		1
#define	DDDCMD_CLOSE		2
#define	DDDCMD_READASYNC	3
#define	DDDCMD_WAITREAD		4
#define	DDDCMD_EJECT		5
#define	DDDCMD_GETINFO		7
#define	DDDCMD_MAP		8
#define	DDDCMD_UNMAP		9
#define	DDDCMD_RETRYLABEL	10

/* Return values from the DDDCMD_OPEN function */
#define	DDD_OPEN_IGNORE		1	/* Can't open; ignore this device. */
/* Return values from OpenDipirDevice(). */
#define	IGNORE_DEVICE		((DDDFile *)1)

bool DriverSupports(DDDFile *fd, uint32 cmd);
DDDFile *OpenDipirDevice(DipirHWResource *dev, DDD *ddd);
int32 CloseDipirDevice(DDDFile *fd);
void *ReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
int32 WaitRead(DDDFile *fd, void *id);
int32 ReadSync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer);
int32 ReadBytes(DDDFile *fd, uint32 offset, uint32 size, void *buffer);
int32 EjectDevice(DDDFile *fd);
int32 GetDeviceInfo(DDDFile *fd, DeviceInfo *info);
int32 MapDevice(DDDFile *fd, uint32 offset, uint32 len, void **paddr);
int32 UnmapDevice(DDDFile *fd, uint32 offset, uint32 len);
int32 RetryLabelDevice(DDDFile *fd, uint32 *pState);
