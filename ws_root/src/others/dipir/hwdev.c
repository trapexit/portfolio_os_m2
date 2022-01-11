/*
 *	@(#) hwdev.c 96/06/10 1.39
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * Routines to manipulate the HWResource structures.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "insysrom.h"

#define HWDEV_FREELIST 1

extern int32 MakeExternalImage(VideoImage *image, uint32 bufLen);

DipirHWResource *HWResources = NULL;

static DipirHWResource *OldHWResources = NULL;
static uint32 gInsertID = 0;

#ifdef HWDEV_FREELIST
static DipirHWResource *freeHWResources;
static DipirHWResource HWResourcePool[MAX_HWDEVICES];
#endif

#if 0
/*****************************************************************************
 Link a HWResource onto the head of a list.
*/
	static void
LinkHWResourceHead(DipirHWResource **list, DipirHWResource *dev)
{
	dev->dev_Next = *list;
	*list = dev;
}
#endif

/*****************************************************************************
 Link a HWResource onto the tail of a list.
*/
	static void
LinkHWResourceTail(DipirHWResource **list, DipirHWResource *dev)
{
	DipirHWResource *idev;

	dev->dev_Next = NULL;
	if (*list == NULL)
	{
		*list = dev;
	} else
	{
		for (idev = *list;  idev->dev_Next != NULL;  idev = idev->dev_Next)
			continue;
		idev->dev_Next = dev;
	}
}

/*****************************************************************************
 Unlink a HWResource from a list.
*/
	static void
UnlinkHWResource(DipirHWResource **list, DipirHWResource *dev)
{
	DipirHWResource *idev;

	if (*list == dev)
	{
		*list = dev->dev_Next;
	} else
	{
		for (idev = *list;  idev != NULL;  idev = idev->dev_Next)
			if (idev->dev_Next == dev)
			{
				idev->dev_Next = dev->dev_Next;
				return;
			}
		printf("UnlinkHWResource: %x not found\n", dev);
	}
}

/*****************************************************************************
 Return a HWResource structure to the free list.
 The HWResource must not be linked on any list.
*/
	static void
FreeHWResource(DipirHWResource *dev)
{
#ifdef HWDEV_FREELIST
	dev->dev_Next = freeHWResources;
	freeHWResources = dev;
#else
	DipirFree(dev);
#endif
}

/*****************************************************************************
 Delete a HWResource from the list of all HWResources.
*/
	void
DeleteHWResource(DipirHWResource *dev)
{
	UnlinkHWResource(&HWResources, dev);
	FreeHWResource(dev);
}

/*****************************************************************************
 Delete a HWResource structure from the "old" list (during probing).
*/
	static void
DeleteOldHWResource(DipirHWResource *dev)
{
	if ((dev->dev_Flags & HWR_NO_HOTREMOVE) &&
	    !(theBootGlobals->bg_DipirFlags & DF_HARD_RESET))
	{
		EPRINTF(("Cannot hot remove!  Reboot!\n"));
		HardBoot();
	}
	UnlinkHWResource(&OldHWResources, dev);
	FreeHWResource(dev);
}

/*****************************************************************************
  Begin to probe devices.
*/
	void
PreProbe(void)
{
	/*
	 * Move all devices to a temporary list while we see
	 * which ones are still valid.
	 */
	OldHWResources = HWResources;
	HWResources = NULL;
}

/*****************************************************************************
  Finish probing devices.
*/
	void
PostProbe(void)
{
	DipirHWResource *dev;
	
	/*
	 * Delete any devices that are still on the temporary list.
	 */
	while ((dev = OldHWResources) != NULL)
	{
		EPRINTF(("DIPIR: device %s no longer present\n",
			dev->dev.hwr_Name));
		DeleteOldHWResource(dev);
	}
}

/*****************************************************************************
 Find a HWResource, given its channel and slot number.
*/
	static DipirHWResource *
FindHWResource(DipirHWResource *list, Channel channel, Slot slot)
{
	DipirHWResource *dev;

	for (dev = list;  dev != NULL;  dev = dev->dev_Next)
		if (dev->dev.hwr_Channel == channel &&
		    dev->dev.hwr_Slot == slot)
			return dev;
	return NULL;
}

/*****************************************************************************
 Get a new HWResource structure from the free list.
*/
	static DipirHWResource *
NewHWResource(void)
{
	DipirHWResource *dev;

#ifdef HWDEV_FREELIST
	dev = freeHWResources;
	if (dev == NULL)
	{
		printf("Too many HWResources!\n");
		return NULL;
	}
	freeHWResources = dev->dev_Next;
#else
	dev = DipirAlloc(sizeof(DipirHWResource), 0);
	if (dev == NULL)
	{
		PRINTF(("Cannot alloc HWresource\n"));
		return NULL;
	}
#endif
	dev->dev.hwr_Version = 1;
	dev->dev_Next = NULL; /* paranoid */
	return dev;
}

/*****************************************************************************
 Create a new HWResource.
*/
	static DipirHWResource *
CreateHWResource(char *name, Channel channel, Slot slot, uint32 hwflags, 
	ChannelDriver *channelDriver, uint32 romSize,
	void *devspec, uint32 devspecSize)
{
	DipirHWResource *dev;

	PRINTF(("DIPIR: create HWResource (%s in slot %x.%x)\n",
		name, channel, slot));

	if ((hwflags & HWR_NO_HOTINSERT) && 
	    !(theBootGlobals->bg_DipirFlags & DF_HARD_RESET))
	{
		EPRINTF(("Cannot hot insert!  Reboot!\n"));
		HardBoot();
	}

	dev = NewHWResource();
	if (dev == NULL)
		return NULL;

	strncpyz(dev->dev.hwr_Name, name, 
		sizeof(dev->dev.hwr_Name), strlen(name));
	dev->dev_ChannelDriver = channelDriver;
	dev->dev_Flags = hwflags;
	dev->dev.hwr_Channel = channel;
	dev->dev.hwr_Slot = slot;
	dev->dev.hwr_InsertID = ++gInsertID;
	dev->dev.hwr_ROMSize = romSize;
	dev->dev.hwr_ROMUserStart = 0;
	dev->dev.hwr_Perms = 0;
	memset(&dev->dev.hwr_DeviceSpecific, 0, 
		sizeof(dev->dev.hwr_DeviceSpecific));
	memcpy(&dev->dev.hwr_DeviceSpecific, devspec, devspecSize);

	LinkHWResourceTail(&HWResources, dev);
	return dev;
}

/*****************************************************************************
 Create a new HWResource, or update an existing one.
*/
	DipirHWResource *
UpdateHWResource(char *name, Channel channel, Slot slot, uint32 hwflags,
	ChannelDriver *channelDriver, uint32 romSize, 
	void *devspec, uint32 devspecSize)
{
	DipirHWResource *dev;

	/*
	 * See if there was a HWResource for this slot in the old list.
	 */
	dev = FindHWResource(OldHWResources, channel, slot);

	if (dev == NULL)
	{
		/*
		 * A device is physically present,
		 * but there is no HWResource.
		 * Create a new HWResource.
		 */
		EPRINTF(("DIPIR: %s in slot %x.%x\n", name, channel, slot));
		dev = CreateHWResource(name, channel, slot, hwflags,
			channelDriver, romSize, devspec, devspecSize);
	} else if ((hwflags & HWR_MEDIA_ACCESS) || 
		   (dev->dev_Flags & HWR_MEDIA_ACCESS))
	{
		/*
		 * There is a HWResource, and the device is present, but
		 * the device has its MediaAccess bit set.
		 * This means it has probably been removed and reinserted.
		 * Delete the HWResource and create a new one.
		 * If the HWResource itself has MediaAccess but the caller
		 * didn't say MediaAccess, force MediaAccess anyway.
		 */
		EPRINTF(("DIPIR: MediaAccess on %s; recreate\n", name));
		DeleteOldHWResource(dev);
		dev = CreateHWResource(name, channel, slot, 
			hwflags | HWR_MEDIA_ACCESS,
			channelDriver, romSize, devspec, devspecSize);
	} else
	{
		/*
		 * Device is physically present, and we already 
		 * had a HWResource for it.  
		 * Just move the HWResource back to the HWResources list.
		 * Note: the HWResource may have been corrupted by a
		 * supervior-breaker, but it doesn't matter.  Since
		 * MediaAccess is clear, dipir is not going to touch it.
		 */
		PRINTF(("DIPIR: retain device %s\n", name));
		UnlinkHWResource(&OldHWResources, dev);
		LinkHWResourceTail(&HWResources, dev);
	}
	return dev;
}

/*****************************************************************************
 Initialize the HWResource free list.
*/
	void
InitHWResources(void)
{
#ifdef HWDEV_FREELIST
	DipirHWResource *dev;

	freeHWResources = NULL;
	for (dev = HWResourcePool;  dev < &HWResourcePool[MAX_HWDEVICES];  dev++)
		FreeHWResource(dev);
#endif
}

/*****************************************************************************
 Find a HWResource, given its insertion ID.
*/
	DipirHWResource *
DeviceFromID(HardwareID hwID)
{
	DipirHWResource *dev;

	for (dev = HWResources;  dev != NULL;  dev = dev->dev_Next)
		if (dev->dev.hwr_InsertID == hwID)
			return dev;
	return NULL;
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name GetHWResource
|||	Get a HWResource structure.
|||
|||	  Synopsis
|||
|||	    int32 GetHWResource(HardwareID hwID, HWResource *buf, uint32 buflen);
|||
|||	  Description
|||
|||	    If hwID is zero, returns the first HWResource structure
|||	    in the list of all valid HWResource structures.  If hwID is
|||	    non-zero, returns the one AFTER the one with that ID.  The
|||	    HWResource structure is copied into the buffer supplied by
|||	    the caller.
|||
|||	  Arguments
|||
|||	    hwID
|||	        Either zero, to retrieve the first HWResource, or the
|||	        ID from a HWResource, to retrieve the HWResource after
|||	        the one with that ID.
|||
|||	    buf
|||	        Pointer to a HWResource structure, into which is copied
|||	        the desired HWResource.
|||
|||	    buflen
|||	        Length of the HWResource buffer, in bytes.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
|||
**/

/*****************************************************************************
 Get the next HWResource from the list of all HWResources.
 External interface used by the OS to scan the list of HWResources.
*/
	int32
GetHWResource(HardwareID hwID, HWResource *buf, uint32 buflen)
{
	DipirHWResource *dev;

	if (hwID == 0)
		dev = HWResources;
	else
	{
		dev = DeviceFromID(hwID);
		if (dev == NULL)
			return -2;
		dev = dev->dev_Next;
	}
	if (dev == NULL)
		return -1;
	memcpy(buf, &dev->dev, buflen);
	return 0;
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name GetHWIcon
|||	Get the icon associated with a HWResource. 
|||
|||	  Synopsis
|||
|||	    int32 GetHWIcon(HardwareID hwID, void *buffer, uint32 bufLen);
|||
|||	  Description
|||
|||	    Return the icon associated with a particular hardware
|||	    resource.
|||
|||	  Arguments
|||
|||	    hwID
|||	        ID of the hardware resource.
|||
|||	    buffer
|||	        Buffer into which to place the icon.
|||
|||	    bufLen
|||	        Size of the buffer, in bytes.
|||
|||	  Return Value
|||
|||	    If the icon was successfully copied into the supplied buffer,
|||	    returns the size of the icon, in bytes.  If the hardware 
|||	    resource does not have an icon, returns 0.  If the supplied 
|||	    buffer is too small to hold the icon, returns the negative 
|||	    of the required buffer size.  If there was any other error,
|||	    returns -1.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
**/

/*****************************************************************************
 Get the icon associated with a hardware resource.
*/
	int32
GetHWIcon(HardwareID hwID, void *buffer, uint32 bufLen)
{
	DipirHWResource *dev;
	int32 iconSize;

	if (hwID == 0)
	{
		/* Special case: get the "machine icon". */
		iconSize = InternalIcon(VI_M2_ICON, (VideoImage *)buffer, bufLen);
	} else
	{
		dev = DeviceFromID(hwID);
		if (dev == NULL)
		{
			/* Invalid insertion ID. */
			return -1;
		}
		if (dev->dev_IconSize == 0)
		{
			/* Device has no icon. */
			return 0;
		}
		if (bufLen < dev->dev_IconSize)
		{
			/* Buffer is too small. */
			return -(dev->dev_IconSize);
		}
		if (dev->dev_Flags & HWR_ICONID)
		{
			iconSize = InternalIcon(dev->dev_IconOffset, 
					(VideoImage *)buffer, bufLen);
		} else
		{
			int32	newSize;
			/* Read the icon from the device using the Channel Driver. */
			iconSize = (*dev->dev_ChannelDriver->cd_Read)(dev, 
				dev->dev_IconOffset, dev->dev_IconSize, buffer);
			if (iconSize != dev->dev_IconSize)
			{
				/* Didn't read it all. */
				return -1;
			}
			/* See if there is a replacement icon. */
			newSize = ReplaceIcon((VideoImage*)buffer, dev, 
					(VideoImage*)buffer, bufLen);
			if (newSize)
				iconSize = newSize;
		}
	}
	if (iconSize <= 0)
		return iconSize;

	return MakeExternalImage(buffer, bufLen);
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name ChannelRead
|||	Read data from a channel driver.
|||
|||	  Synopsis
|||
|||	    int32 ChannelRead(HardwareID hwID, uint32 offset, uint32 len, void *buffer);
|||
|||	  Description
|||
|||	    Reads data from a dipir-level "channel driver".  The data
|||	    is read from the ROM of the specified hardware resource.
|||
|||	  Arguments
|||
|||	    hwID
|||	        ID of the hardware resource.
|||
|||	    offset
|||	        Byte offset into the device at which to start reading.
|||
|||	    len
|||	        Number of bytes to read.
|||
|||	    buffer
|||	        Buffer into which to place the data.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes actually read, or a negative
|||	    number if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
**/

/*****************************************************************************
 Read from a HWResource, using its Channel Driver.
*/
	int32
ChannelRead(HardwareID hwID, uint32 offset, uint32 len, void *buffer)
{
	DipirHWResource *dev;

	dev = DeviceFromID(hwID);
	if (dev == NULL)
		return -1;
	return (*dev->dev_ChannelDriver->cd_Read)(dev, offset, len, buffer);
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name ChannelMap
|||	Memory-map data from a channel driver.
|||
|||	  Synopsis
|||
|||	    int32 ChannelMap(HardwareID hwID, uint32 offset, uint32 len, void **paddr);
|||
|||	  Description
|||
|||	    Attempts to memory-map the ROM of the specified hardware resource.
|||
|||	  Arguments
|||
|||	    hwID
|||	        ID of the hardware resource.
|||
|||	    offset
|||	        Byte offset into the device at which to start mapping.
|||
|||	    len
|||	        Number of bytes to map.
|||
|||	    paddr
|||	        Pointer to a variable into which is placed the address
|||	        of the memory-mapped buffer.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes actually mapped, or a negative
|||	    number if an error occurs or if the device does not support.
|||	    memory-mapping.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
**/

/*****************************************************************************
 Map a HWResource, using its Channel Driver.
*/
	int32
ChannelMap(HardwareID hwID, uint32 offset, uint32 len, void **paddr)
{
	DipirHWResource *dev;

	dev = DeviceFromID(hwID);
	if (dev == NULL)
		return -1;
	return (*dev->dev_ChannelDriver->cd_Map)(dev, offset, len, paddr);
}

/**
|||	AUTODOC -private -class Dipir -group PublicDipir -name ChannelUnmap
|||	Un-memory-map data from a channel driver.
|||
|||	  Synopsis
|||
|||	    int32 ChannelUnmap(HardwareID hwID, uint32 offset, uint32 len);
|||
|||	  Description
|||
|||	    Unmaps the ROM of the specified hardware resource, which was
|||	    previously mapped via ChannelMap().  The offset and len
|||	    arguments must be identical to those passed to the call to
|||	    ChannelMap() which originally created the mapping.
|||
|||	  Arguments
|||
|||	    hwID
|||	        ID of the hardware resource.
|||
|||	    offset
|||	        Byte offset into the device at which the mapping begins.
|||
|||	    len
|||	        Number of bytes mapped.
|||
|||	  Return Value
|||
|||	    Returns 0 on success, or a negative number if an error occurs.
|||
|||	  Implementation
|||
|||	    Public Dipir function
|||
**/

/*****************************************************************************
 Unmap a HWResource, using its Channel Driver.
*/
	int32
ChannelUnmap(HardwareID hwID, uint32 offset, uint32 len)
{
	DipirHWResource *dev;

	dev = DeviceFromID(hwID);
	if (dev == NULL)
		return -1;
	return (*dev->dev_ChannelDriver->cd_Unmap)(dev, offset, len);
}
