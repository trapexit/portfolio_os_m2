/*
 *	@(#) dipir.microcard.c 96/07/02 1.24
 *	Copyright 1996, The 3DO Company
 *
 * Device-dipir for Microcards.
 */

#include "kernel/types.h"
#include "hardware/microcard.h"
#include "dipir/hw.storagecard.h"
#include "dipir.h"
#include "notsysrom.h"
#include "tiny.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;

#define	K	*1024

const uint32 MemSizeTable[] = {
	128 K,		256 K,		512 K,		64 K,
	2048 K,		4096 K,		8192 K,		1024 K,
	32768 K,	1 K,		2 K,		16384 K,
	8 K,		16 K,		32 K,		4 K
};
const uint32 SectorSizeTable[] = {
	128,		256,		512,		64,
	2048,		16,		32,		1024
};

/*****************************************************************************
 How many bits do we have to right-shift the mask to get the
 lowest order 1-bit into bit position 0?
*/
	static uint32
GetShift(uint32 mask)
{
	uint32 shift;

	for (shift = 0;  shift < 32;  shift++)
		if (mask & (1 << shift))
			break;
	return shift;
}

/*****************************************************************************
 Extract the field specified by the mask.
*/
	static uint32
ExtractField(uint32 reg, uint32 mask)
{
	return (reg & mask) >> GetShift(mask);
}

/*****************************************************************************
*/
	static DIPIR_RETURN
GetMicrocardInfo(DDDFile *fd)
{
	uint32 n;
	StorageCardROM rom;
	HWResource_StorCard *sc = (HWResource_StorCard *)
				(fd->fd_HWResource->dev.hwr_DeviceSpecific);

	if (strncmp(fd->fd_HWResource->dev.hwr_Name, "Microcard00\\", 12))
		return DIPIR_RETURN_OK;

	n = ReadBytes(fd, 0, sizeof(rom), &rom);
	if (n != sizeof(rom))
	{
		PRINTF(("Cannot read StorageCard ROM, err %x\n", n));
		return DIPIR_RETURN_TROJAN;
	}

	sc->sc_MemSize = 
		MemSizeTable[ExtractField(rom.scr_Mem, SCR_MEM_SIZE)];
	if (rom.scr_Mem & SCR_MEM_2CHIPS)
		sc->sc_MemSize *= 2;
	sc->sc_SectorSize = 
		SectorSizeTable[ExtractField(rom.scr_Sector, SCR_SECTOR_SIZE)];
	sc->sc_MemType = ExtractField(rom.scr_Mem, SCR_MEM_TYPE);
	sc->sc_ChipRev = ExtractField(rom.scr_ChipRev, SCR_CHIP_REV);
	sc->sc_ChipMfg = ExtractField(rom.scr_ChipRev, SCR_CHIP_MFG);
	if (rom.scr_Sector & SCR_WRITEPROT)
		fd->fd_HWResource->dev.hwr_Perms |= HWR_WRITE_PROTECT;
	PRINTF(("Storagecard: MemSize %x, SectorSize %x, MemType %x\n", 
		sc->sc_MemSize, sc->sc_SectorSize, sc->sc_MemType));
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
*/
	static void
DefaultDeviceName(DDDFile *fd, char *namebuf, uint32 namelen)
{
	/* Name has already been constructed by the Microcard Channel driver. */
	TOUCH(fd);
	TOUCH(namebuf);
	TOUCH(namelen);
}

/*****************************************************************************
 DeviceDipir
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	DIPIR_RETURN ret;

	TOUCH(arg);
	TOUCH(dipirID);
	TOUCH(dddID);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		ret = GetMicrocardInfo(fd);
		if (ret != DIPIR_RETURN_OK)
			return ret;
		return DipirValidateTinyDevice(fd, DefaultDeviceName);
	}
	return DIPIR_RETURN_TROJAN;
}
