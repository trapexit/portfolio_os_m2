/*
 *	@(#) dipir.pcdevcard.c 96/08/23 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Device dipir for the PC developer card.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;


/*****************************************************************************
*/
	DIPIR_RETURN
Validate(DDDFile *fd)
{
	DIPIR_RETURN ret;
#if 0
	if (dev->dev.hwr_Channel != CHANNEL_PCMCIA ||
	    (dev->dev.hwr_Perms & HWR_XIP_OK) == 0)
	{
		PRINTF(("No VISA!\n"));
		return DIPIR_RETURN_TROJAN;
	}
#endif
	ret = ReadAndDisplayIcon(fd);
	if (ret != DIPIR_RETURN_OK)
		return ret;
	strcpy(fd->fd_HWResource->dev.hwr_Name, "PCDEVCARD\\1");
	return DIPIR_RETURN_OK;
}


/*****************************************************************************
 DeviceDipir
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	TOUCH(arg);
	TOUCH(dipirID);
	TOUCH(dddID);
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd);
	}
	return DIPIR_RETURN_TROJAN;
}
