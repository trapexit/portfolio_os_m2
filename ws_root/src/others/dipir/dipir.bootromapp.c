/*
 *	@(#) dipir.bootromapp.c 96/07/31 1.12
 *	Copyright 1995, The 3DO Company
 *
 * Device-dipir for loading a RomApp OS.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "loader/loader3do.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

#define	MyAppPrio	90		/* Boot priority for cartridge device */

const DipirRoutines *dipr;


/*****************************************************************************
*/
static DIPIR_RETURN
LoadRomAppOS(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	RomTag rt;
	BootInfo *bootInfo;
	void *buffer;

	/* Notify that someone is going to boot. */
	DipirWillBoot(fd->fd_DipirTemp);

	/* Read OS, relocate it, and notify ROM that we want to boot it. */
	if (FindRomTag(fd, RSANODE, RSA_M2_OS, 0, &rt) <= 0)
		return DIPIR_RETURN_TROJAN;
	buffer = AllocScratch(fd->fd_DipirTemp, rt.rt_Size, OS_SCRATCH_BUFFER);
	if (buffer == NULL)
	{
		PRINTF(("OS too big %x\n", rt.rt_Size));
		return DIPIR_RETURN_TROJAN;
	}
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size,
			buffer, KEY_128) < 0)
	{
		PRINTF(("Cannot read OS\n"));
		return DIPIR_RETURN_TROJAN;
	}
	bootInfo = LinkCompFile(fd, buffer, rt.rt_Size);
	if (bootInfo == NULL)
	{
		PRINTF(("Cannot relocate kernel\n"));
		return DIPIR_RETURN_TROJAN;
	}
	SetBootOS(fd->fd_VolumeLabel,
		MakeInt16(rt.rt_Version, rt.rt_Revision),
		bootInfo,
		bootInfo->bi_KernelModule->li->entryPoint,
		rt.rt_OSFlags, rt.rt_OSReservedMem, DevicePerms(fd),
		fd->fd_HWResource, dddID, dipirID);
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
 Validate a CD.
*/
DIPIR_RETURN
Validate(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	int32		osVersion;
	DIPIR_RETURN	ret;
	RomTag		rt;

	ret = ReadAndDisplayIcon(fd);
	if (ret < 0)
		return ret;

	/* Find RomTag for the OS. */
	if (FindRomTag(fd, RSANODE, RSA_M2_OS, 0, &rt) <= 0)
		return DIPIR_RETURN_TROJAN;
	osVersion = MakeInt16(rt.rt_Version, rt.rt_Revision);

	/*
	 * Don't register this RomApp OS if a higher versioned
	 * RomApp OS has already been found.
	 */
	if (osVersion <= BootOSVersion(OS_ROMAPP))
	{
		/* Just validate the signature on the OS. */
		return VerifySigned(fd, RSANODE, RSA_M2_OS);
	}

	/*
	 * Don't load this RomApp OS into memory if
	 * any normal (not RomApp) OS has been found.
	 */
	if (BootOSAddr(OS_NORMAL) != NULL)
	{
		/* Just register this OS as available for booting. */
		SetBootOS(fd->fd_VolumeLabel, osVersion,
			NULL, NULL, rt.rt_OSFlags, rt.rt_OSReservedMem,
			DevicePerms(fd), fd->fd_HWResource, dddID, dipirID);
		return DIPIR_RETURN_OK;
	}

	/* Go ahead and load the OS. */
	return LoadRomAppOS(fd, dipirID, dddID);
}


/*****************************************************************************
 DeviceDipir
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	TOUCH(arg);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	PRINTF(("romapp dipir entered\n"));
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd, dipirID, dddID);
	case DIPIR_LOADROMAPP:
		return LoadRomAppOS(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
