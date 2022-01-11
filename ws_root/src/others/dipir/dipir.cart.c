/*
 *	@(#) dipir.cart.c 96/07/31 1.14
 *	Copyright 1995, The 3DO Company
 *
 * Device-dipir for bootable cartridges.
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
 Validate a CD.
*/
DIPIR_RETURN
Validate(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	RomTag		rt;
	BootInfo *	bootInfo;
	void *		buffer;
	uint32		size;
	DIPIR_RETURN	ret;

	/* Decide if we should boot the OS from this cart. */
	if (!ShouldReplaceOS(fd, MyAppPrio))
	{
		/*
		 * Just return and let the current OS keep running.
		 * (But verify the OS on cart anyway.)
		 */
		PRINTF(("Continuing with current os\n"));
		ret = VerifySigned(fd, RSANODE, RSA_M2_APPBANNER);
		if (ret != DIPIR_RETURN_OK)
			return DIPIR_RETURN_TROJAN;
		ret = VerifySigned(fd, RSANODE, RSA_M2_OS);
		if (ret != DIPIR_RETURN_OK)
			return ret;
	}

	/* Notify the system that *someone* is going to boot. */
	DipirWillBoot(fd->fd_DipirTemp);

	if (FindRomTag(fd, RSANODE, RSA_M2_OS, 0, &rt) <= 0)
		return DIPIR_RETURN_TROJAN;

	size = max(rt.rt_Size, BANNER_SIZE);
	buffer = AllocScratch(fd->fd_DipirTemp, size, OS_SCRATCH_BUFFER);
	if (buffer == NULL)
		return DIPIR_RETURN_TROJAN;

	/* Read and display the app banner screen. */
	ret = ReadAndDisplayBanner(fd, buffer, size);
	if (ret != DIPIR_RETURN_OK)
		return ret;

	/* Read OS, relocate it, and notify ROM that we want to boot it. */
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size,
			buffer, KEY_128) < 0)
	{
		PRINTF(("Bad sig on OS\n"));
		return DIPIR_RETURN_TROJAN;
	}
	bootInfo = LinkCompFile(fd, buffer, rt.rt_Size);
	if (bootInfo == NULL)
	{
		PRINTF(("Cannot relocate\n"));
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
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	TOUCH(arg);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
