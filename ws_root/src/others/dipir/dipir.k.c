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
	return ReadAndDisplayIcon(fd);
}


/*****************************************************************************
 DeviceDipir
 Entrypoint.
*/
	DIPIR_RETURN
DeviceDipir(DDDFile *fd, uint32 cmd, void *arg, uint32 dipirID, uint32 dddID)
{
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	PRINTF(("kdipir entered!\n"));
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd);
	}
	return DIPIR_RETURN_TROJAN;
}
