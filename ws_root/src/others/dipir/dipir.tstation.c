/*
 *	@(#) dipir.tstation.c 96/07/22 1.3
 *	Copyright 1996, The 3DO Company
 *
 * Device dipir for a test station card.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

#define	REQUIRE_VISA 0

const DipirRoutines *dipr;


/*****************************************************************************
*/
	static DIPIR_RETURN
Validate(DDDFile *fd)
{
	DIPIR_RETURN ret;

#if REQUIRE_VISA
	if (fd->fd_HWResource->dev.hwr_Channel != CHANNEL_PCMCIA ||
	    (fd->fd_HWResource->dev.hwr_Perms & HWR_XIP_OK) == 0)
	{
		PRINTF(("Card has no VISA\n"));
		return DIPIR_RETURN_TROJAN;
	}
#endif
	ret = ReadAndDisplayIcon(fd);
	if (ret != DIPIR_RETURN_OK)
		return ret;
	/*
	 * Now do the magic.
	 * Set DIPIR_SPECIAL, which stays set permanently.
	 * Set DC_NOKEY, to handle the duration of this dipir event.
	 */
	SetPowerBusBits(M2_DEVICE_ID_CDE, CDE_VISA_DIS, 
			CDE_DIPIR_SPECIAL | CDE_WDATA_OK);
	fd->fd_DipirTemp->dt_BootGlobals->bg_DipirControl |= DC_NOKEY;
	/* Don't allow removal of this card. */
	fd->fd_HWResource->dev_Flags |= HWR_NO_HOTREMOVE;
	return DIPIR_RETURN_OK;
}

/*****************************************************************************
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
