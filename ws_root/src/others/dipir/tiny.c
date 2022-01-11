/*
 *	@(#) tiny.c 96/03/10 1.9
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * Code to manage "tiny" devices.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"
#include "tiny.h"


/*****************************************************************************
 Find a TinyRomTag in a TinyRomTag table.
*/
	uint32
FindTinyRomTag(DDDFile *fd, uint8 type, uint32 pos, TinyRomTag **pResult)
{
	uint8 *p;
	uint8 *ep;
	TinyRomTag *trt;
	TinyVolumeLabel *vol = fd->fd_TinyVolumeLabel;

	p = vol->tl_TinyRomTags + pos;
	ep = ((uint8*)vol) + vol->tl_VolumeSize;
	while (p < ep)
	{
		trt = (TinyRomTag *) p;
		p += TRT_DATASIZE(trt) + SIZE_TRT_HEADER;
		if (trt->trt_Type == type)
		{
			*pResult = trt;
			return p - vol->tl_TinyRomTags;
		}
	}
	return 0;
}

/*****************************************************************************
 Validate a tiny device.
*/
	DIPIR_RETURN
DipirValidateTinyDevice(DDDFile *fd, DefaultNameFunction *DefaultName)
{
	int32 ok;
	uint32 size;
	int32 iconSize;
	void *buffer;
	TinyRomTag *trt;
	VideoImage *icon;
	DipirHWResource *dev = fd->fd_HWResource;
	TinyVolumeLabel *vol = fd->fd_TinyVolumeLabel;

	/*
	 * Check signature on the entire ROM.
	 */
	size = vol->tl_VolumeSize - KeyLen(KEY_THDO_64);
	DipirInitDigest();
	DipirUpdateDigest(vol, size);
	DipirFinalDigest();
	ok = RSAFinal(((uint8*)vol) + size, KEY_THDO_64);
	if (!ok)
	{
		EPRINTF(("Bad sig on TINY ROM\n"));
		return DIPIR_RETURN_TROJAN;
	}
	/*
	 * Store the name of the device.
	 * Name is either in a TinyRomTag, or caller supplies a default.
	 */
	if (FindTinyRomTag(fd, TTAG_DEVICENAME, 0, &trt) > 0)
	{
		strncpyz(dev->dev.hwr_Name, trt->trt_Data, 
			sizeof(dev->dev.hwr_Name), TRT_DATASIZE(trt));
	} else
	{
		/* Use default name */
		(*DefaultName)(fd, dev->dev.hwr_Name, sizeof(dev->dev.hwr_Name));
	}
	PRINTF(("Device name <%s>\n", dev->dev.hwr_Name));
	/*
	 * Update device-specific fields in the HWResource with 
	 * info from the device itself.
	 */
	if (FindTinyRomTag(fd, TTAG_DEVICEINFO, 0, &trt) > 0)
	{
		memcpy(dev->dev.hwr_DeviceSpecific, trt->trt_Data,
			min(TRT_DATASIZE(trt), 
				sizeof(dev->dev.hwr_DeviceSpecific)));
	}
	/*
	 * Find and display the device icon.
	 */
	if (FindTinyRomTag(fd, TTAG_ICON, 0, &trt) <= 0)
	{
		EPRINTF(("No device icon\n"));
		return DIPIR_RETURN_TROJAN;
	}

	iconSize = 4096;
	buffer = DipirAlloc(iconSize, 0);
	if (buffer == NULL)
		return DIPIR_RETURN_TROJAN;
	iconSize = ReplaceIcon((VideoImage *)trt->trt_Data, dev, 
				(VideoImage *)buffer, iconSize);
	if (iconSize < 0)
	{
		if (iconSize == -1)
			return DIPIR_RETURN_TROJAN;
		iconSize = -iconSize;
		DipirFree(buffer);
		buffer = DipirAlloc(iconSize, 0);
		if (buffer == NULL)
			return DIPIR_RETURN_TROJAN;
		iconSize = ReplaceIcon((VideoImage *)trt->trt_Data, dev, 
					(VideoImage *)buffer, iconSize);
	}
	if (iconSize < 0)
		return DIPIR_RETURN_TROJAN;
	if (iconSize == 0)
		icon = (VideoImage *)trt->trt_Data;
	else
		icon = (VideoImage *)buffer;

	if (DisplayIcon(icon, 0, 0) < 0)
	{
		EPRINTF(("Cannot display icon\n"));
		return DIPIR_RETURN_TROJAN;
	}

	DipirFree(buffer);

#if 0
{ uint32 i; static uint32 otherIcons[] = 
	{ VI_VISA_GEN_ICON, VI_EXT_MODEM_ICON, VI_EXT_MIDI_ICON, 
	VI_PC_AUDIN_ICON, VI_PC_MODEM_ICON, 0 };
  buffer = DipirAlloc(4000,0);
  for (i = 0; otherIcons[i]; i++) {
    iconSize = InternalIcon(otherIcons[i], (VideoImage*)buffer, 4000);
    if (iconSize > 0)
	DisplayIcon((VideoImage*)buffer, 0, 0);
    else
	EPRINTF(("Error in icon %x: %x\n", otherIcons[i], iconSize));
  }
  DipirFree(buffer);
}
#endif

	if (dev->dev_Flags & HWR_ICONID)
	{
		dev->dev_IconOffset = icon->vi_ImageID;
		dev->dev_IconSize = icon->vi_Size + sizeof(VideoImage);
	} else
	{
		dev->dev_IconOffset = 
			(uint8*)trt->trt_Data - (uint8*)fd->fd_TinyVolumeLabel;
		dev->dev_IconSize = TRT_DATASIZE(trt);
	}
	return DIPIR_RETURN_OK;
}

