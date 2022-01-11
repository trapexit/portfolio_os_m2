/*
 *	@(#) chan.hostcd.c 96/04/19 1.10
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for the CD-ROM emulator on the debugger host computer.
 */

#ifdef BUILD_DEBUGGER

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

/*
 * Dummy hostcd ROM image to force host CD device driver (dev.hostcd) to be
 * fetched from the system ROM.
 */
#define	NUM_ROMTAGS	1
static const struct RomStructs
{
	ExtVolumeLabel	rom_VolumeLabel;
	RomTag		rom_RomTagTable[NUM_ROMTAGS];
} RomImage =
{
	{ RECORD_STD_VOLUME,	/* dl_RecordType */
	  { VOLUME_SYNC_BYTE_DIPIR, VOLUME_SYNC_BYTE_DIPIR,
	    VOLUME_SYNC_BYTE_DIPIR, VOLUME_SYNC_BYTE_DIPIR,
	    VOLUME_SYNC_BYTE_DIPIR },
	  1,			/* dl_VolumeStructureVersion */
	  VF_M2|VF_M2ONLY,	/* dl_VolumeFlags */
	  "",			/* dl_VolumeComentary */
	  "HOSTCD_ROM",		/* dl_VolumeIdentifier */
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
	  RSANODE, RSA_M2_DRIVER, 0,0,
	  0,			/* rt_Flags */
	  0,			/* rt_TypeSpecific */
	  0,0,
	  0,0,			/* rt_Offset, rt_Size */
	  DDDID_HOSTCD		/* rt_ComponentID */
	}
};

extern const ChannelDriver HostCDChannelDriver;

/*****************************************************************************
*/
	static void
InitHostCD(void)
{
}

/*****************************************************************************
*/
	static void
ProbeHostCD(void)
{
	uint32 flags;

	flags = HWR_SECURE_ROM;
	if (theBootGlobals->bg_KernelAddr == 0)
		/* If the kernel is not yet running, set MediaAccess. */
		flags |= HWR_MEDIA_ACCESS;

	UpdateHWResource("HOSTCD", CHANNEL_HOST, (Slot)1, flags,
			&HostCDChannelDriver, sizeof(RomImage), NULL, 0);
}

/*****************************************************************************
*/
	static int32
ReadHostCD(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
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
MapHostCD(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
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
UnmapHostCD(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return 0;
}

/*****************************************************************************
*/
	static int32
DeviceControlHostCD(DipirHWResource *dev, uint32 cmd)
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
ChannelControlHostCD(uint32 cmd)
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
RetryLabelHostCD(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver HostCDChannelDriver =
{
	InitHostCD,
	ProbeHostCD,
	ReadHostCD,
	MapHostCD,
	UnmapHostCD,
	DeviceControlHostCD,
	ChannelControlHostCD,
	RetryLabelHostCD
};

#else /* BUILD_DEBUGGER */
extern int foo;
#endif
