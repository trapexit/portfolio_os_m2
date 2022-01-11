/*
 * @(#) chan.lccd.c 96/04/19 1.27
 * Copyright 1995 by The 3DO Company Inc.
 *
 * Channel driver for the internal Low-Cost CD-ROM drive.
 */

#include "kernel/types.h"
#include "hardware/cde.h"
#include "dipir.h"
#include "insysrom.h"

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
	  "LCCD_ROM",		/* dl_VolumeIdentifier */
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
	  DDDID_LCCD		/* rt_ComponentID */
	}
};

extern const ChannelDriver LCCDChannelDriver;

/*****************************************************************************
*/
	static void
InitLCCD(void)
{
	return;
}

/*****************************************************************************
*/
	static void
ProbeLCCD(void)
{
	uint32 reg;
	uint32 flags;

	/*
	 * It's too hard to probe for the LCCD here in the channel driver.
	 * (It would require duplicating a lot of the lccd_cde.c code.)
	 * So just assume the LCCD is present, and if it's not, the DDD 
	 * (dev.lccd.c) will return DDD_IGNORE when it is opened.
	 */
	flags = HWR_SECURE_ROM | HWR_ROMAPP_OK;
	reg = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_BBLOCK);
	if (reg & (CDE_CDROM_DIPIR | CDE_CDROM_BLOCK))
		flags |= HWR_MEDIA_ACCESS;
	(void) UpdateHWResource("LCCD", CHANNEL_LCCD, (Slot)4, flags,
			&LCCDChannelDriver, sizeof(RomImage), NULL, 0);
}

/*****************************************************************************
*/
	static int32
ReadLCCD(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
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
MapLCCD(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
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
UnmapLCCD(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return 0;
}

/*****************************************************************************
*/
	static int32
DeviceControlLCCD(DipirHWResource *dev, uint32 cmd)
{
	TOUCH(dev);
	switch (cmd)
	{
	case CHAN_BLOCK:
		return SetPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, 
					CDE_CDROM_BLOCK);
	case CHAN_UNBLOCK:
		return ClearPowerBusBits(M2_DEVICE_ID_CDE, CDE_BBLOCK, 
					CDE_CDROM_DIPIR | CDE_CDROM_BLOCK);
	}
	return -1;
}

/*****************************************************************************
*/
	static int32
ChannelControlLCCD(uint32 cmd)
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
RetryLabelLCCD(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver LCCDChannelDriver = 
{
	InitLCCD,
	ProbeLCCD,
	ReadLCCD,
	MapLCCD,
	UnmapLCCD,
	DeviceControlLCCD,
	ChannelControlLCCD,
	RetryLabelLCCD
};

