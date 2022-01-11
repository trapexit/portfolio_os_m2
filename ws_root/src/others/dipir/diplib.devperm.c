/*
 *	@(#) diplib.devperm.c 96/07/02 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Get device permission bits.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
 Get the device permission mask from the RomTag table.
*/
uint32
DevicePerms(DDDFile *fd)
{
	RomTag rt;

	if (FindRomTag(fd, RSANODE, RSA_DEV_PERMS, 0, &rt) <= 0)
		return 0;
	return rt.rt_Reserved3[0];
}

