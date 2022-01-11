/*
 * @(#) chan.powerbus.c 96/10/08 1.44
 * Copyright 1995, The 3DO Company
 */

#include "kernel/types.h"
#include "setjmp.h"
#include "hardware/PPCasm.h" 
#include "hardware/bridgit.h"
#include "hardware/cde.h"
#include "hardware/bda.h"
#include "bootcode/boothw.h"
#include "dipir.h"
#include "insysrom.h"


/*****************************************************************************
*/
	static void
InitPowerBus(void)
{
	return;
}

/*****************************************************************************
  ProbeForDev
*/
	static bool
ProbeForDev(uint32 slot)
{
#ifdef BUILD_DEBUGGER
	/*
	 * We can't trap to 0xFFF0xxxx in the debugger.
	 * Only probe for devices which we know to be present, 
	 * so we don't get any Machine Checks.
	 */
	return (slot == 3 || slot == 4);  /* Bridgit or CDE */
#else 
	/* Probe for everything. */
	TOUCH(slot);
	return TRUE;
#endif
}

/*****************************************************************************
  GetPBusDevices
  Scan the PowerBus slots determining which type of device is in each slot.
*/
	static void 
GetPBusDevices(uint8 *pbusdevs)
{
	uint32 slot;
	uint8 devType;
	PBDevHeader *pbus;
	int save_jb[jmp_buf_size];

	PRINTF(("Probing Powerbus\n"));
	memcpy(save_jb, dtmp->dt_JmpBuf, sizeof(save_jb));

	/*
	 * Enable machine check exceptions since we might need to handle
	 * them while probing for hardware.
	 */
	_mtmsr(_mfmsr() | MSR_ME);


	for (slot = pbDevsSlot0; slot <= pbDevsSlotN; slot++)
	{
		pbus = (PBDevHeader *)(PBDevsBaseAddr(slot));

		/*
		 * If a powerbus device does not exist, we may get a fault
		 * when we try to access it.
		 */
		if (setjmp(dtmp->dt_JmpBuf))
		{
			/* Got a fault accessing the slot. */
			PRINTF(("Handled Machine Check\n"));
			devType = M2_DEVICE_ID_NOT_THERE;
		} else if (slot == 0)
		{
			/* BDA is always is slot 0. */
			devType = M2_DEVICE_ID_BDA;
		} 
		else if (ProbeForDev(slot))
		{
			/* Read the device ID from the device itself. */
			devType = pbus->devType;
		} else
		{
			/* Slot is known to be empty at compile time. */
			devType = M2_DEVICE_ID_NOT_THERE;
		}
		/* Store the devType found (or none for case above) */
		PRINTF(("PowerSlot %x: device %x\n", slot, devType));
		pbusdevs[slot] = devType;
	}

	/* Disable machine check exceptions to prevent bogus ones from
	 * breaking security.
	 */
	_mtmsr(_mfmsr() & ~MSR_ME);

	memcpy(dtmp->dt_JmpBuf, save_jb, sizeof(save_jb));
	PRINTF(("Done with PBus probe\n"));
}

/*****************************************************************************
 PowerSlot
 Find a specified device type on the PowerBus and return which slot it is in.
*/
	int32
PowerSlot(uint32 devType)
{
	int32 slot;

	for (slot = pbDevsSlot0; slot <= pbDevsSlotN; slot++)
	{
		if (theBootGlobals->bg_PBusDevs[slot] == devType)
			return slot;
	}
	return -1;
}

/*****************************************************************************
*/
	static void
InitPowerBusRegs(void)
{

/* Place holder for any necessary powerbus setup */

}

/*****************************************************************************
*/
	static void
ProbePowerBus(void)
{
	/* Fill in the powerbus device assignments */
	GetPBusDevices(theBootGlobals->bg_PBusDevs);

	/* Initialize PowerBus registers for normal operation. */
	InitPowerBusRegs();

#ifdef BUILD_STRINGS
	/* Find the CDE */
	if (PowerSlot(M2_DEVICE_ID_CDE) < 0)
		EPRINTF(("**** No CDE found!\n"));
#endif
}

/*****************************************************************************
*/
	static int32
ReadPowerBus(DipirHWResource *dev, uint32 offset, uint32 len, void *buf)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(buf);
	return -1;
}

/*****************************************************************************
*/
	static int32
MapPowerBus(DipirHWResource *dev, uint32 offset, uint32 len, void **paddr)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	TOUCH(paddr);
	return -1;
}

/*****************************************************************************
*/
	static int32
UnmapPowerBus(DipirHWResource *dev, uint32 offset, uint32 len)
{
	TOUCH(dev);
	TOUCH(offset);
	TOUCH(len);
	return -1;
}

/*****************************************************************************
*/
	static int32
DeviceControlPowerBus(DipirHWResource *dev, uint32 cmd)
{
	TOUCH(dev);
	switch (cmd)
	{
	case CHAN_BLOCK:
	case CHAN_UNBLOCK:
		break;
	}
	return -1;
}

/*****************************************************************************
  DisallowAllUnblocks
  Set the "disable clear dipir bit" which prevents anyone from
  clearing any dipir bits without doing SoftReset first.
*/
	static void
DisallowAllUnblocks(void)
{
	uint32 slot;
	uint32 devBase;
	uint8 devType;

	/* Check the device in each powerbus slot. */
	for (slot = pbDevsSlot0;  slot <= pbDevsSlotN;  slot++)
	{
		devType = theBootGlobals->bg_PBusDevs[slot];
		devBase = PBDevsBaseAddr(slot);
		switch (devType)
		{
		case M2_DEVICE_ID_NOT_THERE:
			/* No device at this slot */
			break;
		case M2_DEVICE_ID_BDA:
			break;
		case M2_DEVICE_ID_CDE:
			PBUSDEV_SET(devBase, CDE_BBLOCK_EN, CDE_BLOCK_CLEAR);
			break;
		case M2_DEVICE_ID_BRIDGIT:
			PBUSDEV_SET(devBase, BR_CL_DIPIR, BR_BLOCK_CLEAR);
			break;
		default:
			PRINTF(("Unknown powerbus device %x in slot %x\n",
				devType, slot));
			break;
		}
	}
}

/*****************************************************************************
*/
	uint32
ReadPowerBusRegister(uint32 devType, uint32 regaddr)
{
	int32 slot;
	uint32 devBase;

	slot = PowerSlot(devType);
#if DEBUG
	if (slot < 0)
	{
		PRINTF(("ReadPowerBusReg(%x,%x): dev not found\n", 
			devType, regaddr));
		return 0;
	}
#endif
	devBase = PBDevsBaseAddr(slot);
	return PBUSDEV_READ(devBase, regaddr);
}

/*****************************************************************************
*/
	void
WritePowerBusRegister(uint32 devType, uint32 regaddr, uint32 value)
{
	int32 slot;
	uint32 devBase;

	slot = PowerSlot(devType);
#ifdef DEBUG
	if (slot < 0)
	{
		PRINTF(("WritePowerBusReg(%x,%x): dev not found\n", 
			devType, regaddr));
		return;
	}
#endif
	devBase = PBDevsBaseAddr(slot);
	PBUSDEV_WRITE(devBase, regaddr, value);
}

/*****************************************************************************
*/
	int32
SetPowerBusBits(uint32 devType, uint32 regaddr, uint32 bits)
{
	int32 slot;
	uint32 devBase;
	uint32 reg;

	slot = PowerSlot(devType);
#ifdef DEBUG
	if (slot < 0)
	{
		PRINTF(("SetPowerBusBits(%x,%x,%x): dev not found\n", 
			devType, regaddr, bits));
		return -1;
	}
#endif
	devBase = PBDevsBaseAddr(slot);
	PBUSDEV_SET(devBase, regaddr, bits);
	reg = PBUSDEV_READ(devBase, regaddr);
	if ((reg & bits) != bits)
	{
		PRINTF(("SetPowerBusBits(%x,%x,%x): bits did not set: %x\n", 
			devType, regaddr, bits, reg));
		return -1;
	}
	return 0;
}

/*****************************************************************************
*/
	int32
ClearPowerBusBits(uint32 devType, uint32 regaddr, uint32 bits)
{
	int32 slot;
	uint32 devBase;
	uint32 reg;

	slot = PowerSlot(devType);
#ifdef DEBUG
	if (slot < 0)
	{
		PRINTF(("ClearPowerBusBits(%x,%x,%x): dev not found\n", 
			devType, regaddr, bits));
		return -1;
	}
#endif
	devBase = PBDevsBaseAddr(slot);
	PBUSDEV_CLR(devBase, regaddr, bits);
	reg = PBUSDEV_READ(devBase, regaddr);
	if (reg & bits)
	{
		PRINTF(("ClearPowerBusBits(%x,%x,%x): bits did not clear: %x\n", 
			devType, regaddr, bits, reg));
		return -1;
	}
	return 0;
}

/*****************************************************************************
*/
	Boolean
DeviceBlockedError(void)
{
	uint32 errstat;

	errstat = ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_DEV_ERROR);
	if (errstat & (CDE_DEV1_BLOCK_ERR | CDE_DEV2_BLOCK_ERR | 
			CDE_DEV5_BLOCK_ERR | CDE_DEV6_BLOCK_ERR | 
			CDE_DEV7_BLOCK_ERR))
		return TRUE;
	return FALSE;
}

/*****************************************************************************
*/
	static int32
ChannelControlPowerBus(uint32 cmd)
{
	switch (cmd)
	{
	case CHAN_DISALLOW_UNBLOCK:
#ifndef PCMCIA_DETECT
		DisallowAllUnblocks();
#endif
		return 0;
	}
	return -1;
}

/*****************************************************************************
*/
	uint32
BDARead(uint32 *addr)
{
	uint32 reg;
 uint32 tries = 0;
	int save_jb[jmp_buf_size];

	memcpy(save_jb, dtmp->dt_JmpBuf, sizeof(save_jb));
Again:
	tries++;
	if (setjmp(dtmp->dt_JmpBuf))
		goto Again;
	reg = *addr;
	memcpy(dtmp->dt_JmpBuf, save_jb, sizeof(save_jb));
PRINTF(("BDARead(%x) = %x (%d tries)\n", addr, reg, tries));
	return reg;
}

	static int32
RetryLabelPowerBus(DipirHWResource *dev, uint32 *pState)
{
	TOUCH(dev);
	TOUCH(pState);
	return -1;
}

const ChannelDriver PowerBusChannelDriver =
{
	InitPowerBus,
	ProbePowerBus,
	ReadPowerBus,
	MapPowerBus,
	UnmapPowerBus,
	DeviceControlPowerBus,
	ChannelControlPowerBus,
	RetryLabelPowerBus
};
