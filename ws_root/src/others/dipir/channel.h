/*
 *      @(#) channel.h 96/03/05 1.21
 *      Copyright (c) 1995, The 3DO Company
 *
 * Definitions related to Channel Drivers.
 */

#define	MAX_HWDEVICES		20


typedef struct DipirHWResource
{
	struct DipirHWResource *dev_Next;
	struct ChannelDriver *dev_ChannelDriver;
	uint32		dev_IconOffset;
	uint32		dev_IconSize;
	uint16		dev_Flags;
	uint16		dev_Pad1;
	uint32		dev_Pad2;
	HWResource	dev;
} DipirHWResource;

/* Bits in dev_Flags */
#define	HWR_ROMAPP_OK		0x0001 /* Device supports RomApp media */
#define	HWR_MEDIA_ACCESS	0x0002 /* Device's MediaAccess bit is set */
#define	HWR_SECURE_ROM		0x0004 /* Assume device ROM is secure */
#define	HWR_DELETE		0x0008 /* Get rid of this HWResource */
#define	HWR_DIPIRED		0x0010 /* Dev has been dipired this time */
#define	HWR_NODIPIR		0x0020 /* Dev shd never have MEDIA_ACCESS */
#define	HWR_ICONID		0x0040 /* dev_IconOffset is really ID */
#define	HWR_NO_HOTINSERT	0x0080 /* Device cannot be hot-inserted */
#define	HWR_NO_HOTREMOVE	0x0100 /* Device cannot be hot-removed */
/* These flags are private to each channel driver. */
#define	HWR_CHANSPEC1		0x8000
#define	HWR_CHANSPEC2		0x4000
#define	HWR_CHANSPEC3		0x2000
#define	HWR_CHANSPEC4		0x1000


typedef struct ChannelDriver
{
	void	(*cd_Init)(void);
	void	(*cd_Probe)(void);
	int32	(*cd_Read)(DipirHWResource *dev, uint32 offset, uint32 len, void *buf);
	int32	(*cd_Map)(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr);
	int32	(*cd_Unmap)(DipirHWResource *dev, uint32 offset, uint32 len);
	int32	(*cd_DeviceControl)(DipirHWResource *dev, uint32 cmd);
	int32	(*cd_ChannelControl)(uint32 cmd);
	int32	(*cd_RetryLabel)(DipirHWResource *dev, uint32 *pState);
} ChannelDriver;

/* Commands to cd_DeviceControl */
#define	CHAN_BLOCK		1	/* Block access to device */
#define	CHAN_UNBLOCK		2	/* Allow access to device */

/* Commands to cd_ChannelControl */
#define	CHAN_DISALLOW_UNBLOCK	1	/* Disallow CHAN_UNBLOCK to devices on the channel*/


void InitHWResources(void);
DipirHWResource * UpdateHWResource(char *name, Channel channel, Slot slot, 
			uint32 hwflags, ChannelDriver *channelDriver, 
			uint32 romSize, void *devspec, uint32 devspecSize);
int32 GetHWResource(HardwareID hwID, HWResource *buf, uint32 buflen);
int32 GetHWIcon(HardwareID hwID, void *buf, uint32 buflen);
int32 ChannelRead(HardwareID hwID, uint32 offset, uint32 len, void *buffer);
int32 ChannelMap(HardwareID hwID, uint32 offset, uint32 len, void **paddr);
int32 ChannelUnmap(HardwareID hwID, uint32 offset, uint32 len);
