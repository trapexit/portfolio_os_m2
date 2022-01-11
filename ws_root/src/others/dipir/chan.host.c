/*
 *	@(#) chan.host.c 96/04/19 1.10
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for the debugger host computer.
 */

#ifdef BUILD_DEBUGGER

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

/*
 * Dummy host ROM image to force host device driver (dev.host) to be
 * fetched from the system ROM.
 */
#define	ICON_SIZE	4344
#define	NUM_ROMTAGS	2
static const struct RomStructs
{
	ExtVolumeLabel	rom_VolumeLabel;
	RomTag		rom_RomTagTable[NUM_ROMTAGS];
	uint32		rom_Icon[ICON_SIZE/sizeof(uint32)];
} RomImage =
{
	{ RECORD_STD_VOLUME,	/* dl_RecordType */
	  { VOLUME_SYNC_BYTE_DIPIR, VOLUME_SYNC_BYTE_DIPIR, 
	    VOLUME_SYNC_BYTE_DIPIR, VOLUME_SYNC_BYTE_DIPIR, 
	    VOLUME_SYNC_BYTE_DIPIR },
	  1,			/* dl_VolumeStructureVersion */
	  VF_M2|VF_M2ONLY,	/* dl_VolumeFlags */
	  "",			/* dl_VolumeComentary */
	  "HOST_ROM",		/* dl_VolumeIdentifier */
	  0,			/* dl_VolumeUniqueIdentifier */
	  1,			/* dl_VolumeBlockSize */
	  sizeof(struct RomStructs), /* dl_VolumeBlockCount */
	  0,			/* dl_RootUniqueIdentifier */
	  0,			/* dl_RootDirectoryBlockCount */
	  0,			/* dl_RootDirectoryBlockSize */
	  0,			/* dl_RootDirectoryLastAvatarIndex */
	  { 0 },		/* dl_RootDirectoryAvatarList */
	  NUM_ROMTAGS		/* dl_NumRomTags */
	},
	{
	{
	  RSANODE, RSA_M2_DRIVER, 0,0,
	  0,			/* rt_Flags */
	  0,			/* rt_TypeSpecific */
	  0,0,
	  0,0,			/* rt_Offset, rt_Size */
	  DDDID_HOST		/* rt_ComponentID */
	},
	{
	  RSANODE, RSA_M2_ICON, 0,0,
	  0,			/* rt_Flags */
	  0,			/* rt_TypeSpecific */
	  0,0,
	  sizeof(ExtVolumeLabel) + (NUM_ROMTAGS * sizeof(RomTag)),
	  ICON_SIZE,		/* rt_Size */
	  0			/* rt_ComponentID */
	}
	},
	{
#include "hosticon.h"
	}
};

extern const ChannelDriver HostChannelDriver;

/*****************************************************************************
*/
	static void
InitHost(void)
{
}

/*****************************************************************************
 Probe for the Host device.
*/
	static void
ProbeHost(void)
{
	uint32 flags;
	DipirHWResource *dev;

	flags = HWR_SECURE_ROM;
	if (theBootGlobals->bg_KernelAddr == 0)
		/* If the kernel is not yet running, set MediaAccess. */
		flags |= HWR_MEDIA_ACCESS;

	dev = UpdateHWResource("HOST", CHANNEL_HOST, (Slot)0, flags,
			&HostChannelDriver, sizeof(RomImage), NULL, 0);
	dev->dev_IconOffset = 
		sizeof(ExtVolumeLabel) + (NUM_ROMTAGS * sizeof(RomTag));
	dev->dev_IconSize = ICON_SIZE;
}

/*****************************************************************************
*/
	static int32
ReadHost(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	uint32 dlen;

	TOUCH(dev);
	if (offset > sizeof(RomImage))
		return 0;
	dlen = sizeof(RomImage) - offset;
	if (dlen < len)
		len = dlen;
	memcpy(buf, ((uint8*)&RomImage) + offset, len);
	return len;
}

/*****************************************************************************
*/
	static int32
MapHost(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	uint32 dlen;

	TOUCH(dev);
	dlen = sizeof(RomImage) - offset;
	if (dlen < len)
		len = dlen;
	*paddr = (((uint8*)&RomImage) + offset);
	return len;
}

/*****************************************************************************
*/
	static int32
UnmapHost(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return 0;
}

/*****************************************************************************
*/
	static int32
DeviceControlHost(DipirHWResource *dev, uint32 cmd)
{
	TOUCH(dev);
	switch (cmd)
	{
	case CHAN_BLOCK:
	case CHAN_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
ChannelControlHost(uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_DISALLOW_UNBLOCK:
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
RetryLabelHost(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver HostChannelDriver =
{
	InitHost,
	ProbeHost,
	ReadHost,
	MapHost,
	UnmapHost,
	DeviceControlHost,
	ChannelControlHost,
	RetryLabelHost
};

#else /* BUILD_DEBUGGER */
extern int foo;
#endif
