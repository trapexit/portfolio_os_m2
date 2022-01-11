/*
 *	@(#) dipir.romapp.c 96/07/02 1.3
 *	Copyright 1995, The 3DO Company
 *
 * Device-dipir for loading a RomApp OS.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;


/*****************************************************************************
  Validate all components of the RomApp OS.
*/
	static DIPIR_RETURN 
Validate(DDDFile *fd)
{
	return ReadAndDisplayIcon(fd);
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
	PRINTF(("romapp dipir entered\n"));
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd);
	}
	return DIPIR_RETURN_TROJAN;
}
