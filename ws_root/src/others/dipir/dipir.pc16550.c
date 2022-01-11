/*
 *	@(#) dipir.pc16550.c 96/07/02 1.3
 *	Copyright 1996, The 3DO Company
 *
 * Device-dipir for the prototype PC16550 card.
 */

#include "kernel/types.h"
#include "dipir/hw.pc16550.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;


/*****************************************************************************
*/
	static DIPIR_RETURN 
Validate(DDDFile *fd)
{
	DipirHWResource *dev = fd->fd_HWResource;
	HWResource_PC16550 *spec = (HWResource_PC16550 *)
					dev->dev.hwr_DeviceSpecific;

	strcpy(dev->dev.hwr_Name, "PC16550\\0");
	spec->pcs_Clock = 11289600/2;
	spec->pcs_CycleTime = 100;
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
