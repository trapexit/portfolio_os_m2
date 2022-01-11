/*
 *	@(#) dipir.visa.c 96/07/02 1.9
 *	Copyright 1994,1995, The 3DO Company
 *
 * Device dipir for a vanilla (ROM-less) VISA device.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "tiny.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;

/*****************************************************************************
*/
	static void
DefaultVisaName(DDDFile *fd, char *namebuf, uint32 namelen)
{
	/* Name has already been constructed by the PCMCIA Channel driver. */
	TOUCH(fd);
	TOUCH(namelen);
	TOUCH(namebuf);
}

/*****************************************************************************
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	DipirHWResource *dev;

	TOUCH(arg);
	TOUCH(dipirID);
	TOUCH(dddID);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		dev = fd->fd_HWResource;
		if (dev->dev.hwr_Channel != CHANNEL_PCMCIA ||
		    (dev->dev.hwr_Perms & HWR_XIP_OK) == 0)
		{
			PRINTF(("No VISA!\n"));
			return DIPIR_RETURN_TROJAN;
		}
		dev->dev_Flags |= HWR_ICONID;
		return DipirValidateTinyDevice(fd, DefaultVisaName);
	}
	return DIPIR_RETURN_TROJAN;
}
