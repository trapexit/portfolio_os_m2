/*
 *	@(#) chan.sysmem.c 96/02/29 1.55
 *	Copyright 1995, The 3DO Company
 *
 * Channel driver for system memory (RAM, ROM).
 */

#include "kernel/types.h"
#include "kernel/mem.h"
#include "hardware/bda.h"
#include "dipir/hw.ram.h"
#include "dipir.h"
#include "insysrom.h"

#define	K 1024

#if 0
/* "Kludge ROM" for debugging. */
#define	KROM_START	(0x40750000)
#define	KROM_SIZE	(0x5000)
#endif

extern const ChannelDriver SysMemChannelDriver;

/*****************************************************************************
*/
	static void
InitSysMem(void)
{
	return;
}

/*****************************************************************************
*/
	static void
CreateMemDevice(char *name, Slot slot, uint32 romSize, uint32 romUserStart, 
		uint32 hwflags,
		uint32 memAddr, uint32 memSize, uint32 memUserStart, 
		uint32 pageSize,
		uint32 memtypes, uint32 memflags, uint32 perms)
{
	DipirHWResource *dev;
	HWResource_RAM ram;

	PRINTF(("CreateMemDevice(%s)\n",name));

	ram.ram_Addr = memAddr;
	ram.ram_Size = memSize;
	ram.ram_Start = memUserStart;
	ram.ram_PageSize = pageSize;
	ram.ram_Flags = memflags;
	ram.ram_MemTypes = memtypes;
	dev = UpdateHWResource(name, CHANNEL_SYSMEM, slot, hwflags,
			&SysMemChannelDriver, romSize,
			&ram, sizeof(ram));
	dev->dev.hwr_Perms = perms;
	dev->dev.hwr_ROMUserStart = romUserStart;
}

/*****************************************************************************
*/
	static void
ProbeSysMem(void)
{

	PRINTF(("ProbeSysMem\n"));

#ifdef KROM_START
	CreateMemDevice("KROM", (Slot)2,
		KROM_SIZE, 0,
		HWR_MEDIA_ACCESS,
		KROM_START, KROM_SIZE, 0, 1,
		0, RAM_READONLY, 0);
#endif

#ifdef FONTROM_START
	/* Font ROM */
	if (FONTROM_ADDR)
	{
		CreateMemDevice("SYSFONTROM", (Slot)4,
			FONTROM_SIZE, 0,
			HWR_NODIPIR,
			FONTROM_START, FONTROM_SIZE, 0, 1,
			0, RAM_READONLY, 0);
	}
#endif
	/* RAM */
	CreateMemDevice("SYSMEM", (Slot)10,
		0, 0,
		HWR_NODIPIR,
		theBootGlobals->bg_SystemRAM.mr_Start, 
		theBootGlobals->bg_SystemRAM.mr_Size,
		0, 4*K,
		MEMTYPE_ANY, 0, 0);

#ifdef RAM0_START
	/* acrobat test ram */
	CreateMemDevice("RAM", (Slot)20, 
		0, 0,
		HWR_NODIPIR,
		RAM0_START, RAM0_SIZE, 0, RAM0_PAGESIZE,
		0, 0, 0);
#endif

#ifdef RAM1_START
	/* acrobat 2nd test ram */
	CreateMemDevice("RAM", (Slot)21, 
		0, 0,
		HWR_NODIPIR,
		RAM1_START, RAM1_SIZE, 0, RAM1_PAGESIZE,
		0, 0, 0);
#endif
}

/*****************************************************************************
*/
	static int32
ReadSysMem(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	uint32 dlen;
	HWResource_RAM *raminfo;

	raminfo = (HWResource_RAM *) &dev->dev.hwr_DeviceSpecific;
	if (offset > raminfo->ram_Size)
		return 0;
	dlen = raminfo->ram_Size - offset;
	if (dlen < len)
		len = dlen;
	memcpy(buf, (void*)(raminfo->ram_Addr + offset), len);
	return len;
}

/*****************************************************************************
*/
	static int32
MapSysMem(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	uint32 dlen;
	HWResource_RAM *raminfo;

	raminfo = (HWResource_RAM *) &dev->dev.hwr_DeviceSpecific;
	dlen = raminfo->ram_Size - offset;
	if (dlen < len)
		len = dlen;
	*paddr = (void*)(raminfo->ram_Addr + offset);
	return len;
}

/*****************************************************************************
*/
	static int32
UnmapSysMem(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return 0;
}

/*****************************************************************************
*/
	static int32
DeviceControlSysMem(DipirHWResource *dev, uint32 cmd)
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
ChannelControlSysMem(uint32 cmd)
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
RetryLabelSysMem(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

/*****************************************************************************
*/
const ChannelDriver SysMemChannelDriver =
{
	InitSysMem,
	ProbeSysMem,
	ReadSysMem,
	MapSysMem,
	UnmapSysMem,
	DeviceControlSysMem,
	ChannelControlSysMem,
	RetryLabelSysMem
};

