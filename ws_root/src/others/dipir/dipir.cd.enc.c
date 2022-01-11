#define ENCRYPT
#define STRICT_DIPIR
/*
 *	@(#) dipir.cd.c 96/07/31 1.66
 *	Copyright 1995, The 3DO Company
 *
 * Device-dipir for CD-ROMs.
 * Does the security checks on the CD-ROM media.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "loader/loader3do.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

#define	MyAppPrio	100		/* Boot priority for CD-ROM device */

#ifdef STRICT_DIPIR
#define	MAX_4X_TRIES	2		/* Number of tries at 4x */
#define	MAX_2X_TRIES	8		/* Number of tries at 2x */
#else
#define	MAX_4X_TRIES	1
#define	MAX_2X_TRIES	0
#endif


const DipirRoutines *dipr;
static DiscInfo cdinfo;


#ifndef STRICT_DIPIR
/*****************************************************************************
 Display the "gold disc" icon.
*/
static void
DisplayGoldIcon(void)
{
	static const uint32 GoldIcon[] = {
		0x0149434f, 0x4e2d0005, 0x000000a8, 0x002a0020,
		0x01030000, 0x00000000, 0x000600e7, 0x7fe0ffff,
		0xffff8000, 0x00018000, 0x00018000, 0x00018000,
		0x00018000, 0x00018000, 0x00018000, 0x00018003,
		0xc001800f, 0xf001803f, 0xfc01807f, 0xfe0180ff,
		0xff0180ff, 0xff0181ff, 0xff8181fe, 0x7f8181fc,
		0x3f8181fe, 0x7f8181ff, 0xff8180ff, 0xff0180ff,
		0xff01807f, 0xfe01803f, 0xfc01800f, 0xf0018003,
		0xc0018000, 0x00018000, 0x00018000, 0x00018000,
		0x00018000, 0x00018000, 0x00018000, 0x00018000,
		0x00018000, 0x00018000, 0x00018000, 0x00018000,
		0x00018000, 0x00018000, 0x00018000, 0x00018000,
		0x0001ffff, 0xffff0000
	};

	DisplayIcon((VideoImage *) GoldIcon, 0, 0);
}
#endif /* STRICT_DIPIR */

/*****************************************************************************
 Check various aspects of the disc media.
*/
static DIPIR_RETURN
CheckMedia(DDDFile *fd)
{
	DIPIR_RETURN ret;

	TIMESTAMP("CDdm",0);
	ret = CheckDiscMode(fd, &cdinfo);
	if (ret != DIPIR_RETURN_OK)
	{
		PRINTF(("Bad disc mode\n"));
		return ret;
	}

	TIMESTAMP("CDwb",0);
	ret = Check3DODisc(fd, &cdinfo, MAX_4X_TRIES, MAX_2X_TRIES);
	if (ret != DIPIR_RETURN_OK)
	{
		PRINTF(("Not 3DO fmt\n"));
#ifdef STRICT_DIPIR
		if ((ReadPowerBusRegister(M2_DEVICE_ID_CDE, CDE_VISA_DIS) &
					CDE_WDATA_OK) == 0)
			return ret;
#else
		/*
		 * Allow a Gold disc in the unencrypted version, but
		 * display a special icon to signal that we've detected it.
		 */
		DisplayGoldIcon();
#endif
	}
	TIMESTAMP("CDmo",0);
	return DIPIR_RETURN_OK;
}

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

	/* Get CD information. */
	TIMESTAMP("CDgi",0);
	ret = DeviceGetCDInfo(fd, &cdinfo);
        if (ret < 0)
		return DIPIR_RETURN_TROJAN;

	/* Decide if we should boot the OS from this disc. */
	if (!ShouldReplaceOS(fd, MyAppPrio))
	{
		/*
		 * Just return and let the current OS keep running.
		 * (But verify the OS on disc anyway.)
		 */
		PRINTF(("Continuing with current os\n"));
		ret = VerifySigned(fd, RSANODE, RSA_M2_APPBANNER);
		if (ret != DIPIR_RETURN_OK)
			return DIPIR_RETURN_TROJAN;
		ret = CheckMedia(fd);
		if (ret != DIPIR_RETURN_OK)
			return ret;
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
	TIMESTAMP("CDbn",0);
	ret = ReadAndDisplayBanner(fd, buffer, size);
	if (ret != DIPIR_RETURN_OK)
		return ret;

	/* Check the disc media. */
	TIMESTAMP("CDcm",0);
	ret = CheckMedia(fd);
	if (ret != DIPIR_RETURN_OK)
		return ret;

	/* Read OS, relocate it, and notify ROM that we want to boot it. */
	TIMESTAMP("CDos",0);
	if (ReadSigned(fd, rt.rt_Offset + fd->fd_RomTagBlock, rt.rt_Size,
			buffer, KEY_128) < 0)
	{
		PRINTF(("Bad sig on OS\n"));
		return DIPIR_RETURN_TROJAN;
	}
	TIMESTAMP("CDrl",0);
	bootInfo = LinkCompFile(fd, buffer, rt.rt_Size);
	if (bootInfo == NULL)
		return DIPIR_RETURN_TROJAN;
	SetBootOS(fd->fd_VolumeLabel,
		MakeInt16(rt.rt_Version, rt.rt_Revision),
		bootInfo,
		bootInfo->bi_KernelModule->li->entryPoint,
		rt.rt_OSFlags, rt.rt_OSReservedMem, DevicePerms(fd),
		fd->fd_HWResource, dddID, dipirID);
	TIMESTAMP("CDok",0);
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
	TIMESTAMP("CDen",0);
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
