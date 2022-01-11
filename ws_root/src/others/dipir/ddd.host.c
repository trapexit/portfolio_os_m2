/*
 *	@(#) ddd.host.c 95/09/21 1.2
 *	Copyright 1995, The 3DO Company
 *
 * Interface glue to DDD commands specific to the "host" device.
 */

#include "kernel/types.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.host.h"

extern const DipirRoutines *dipr;

typedef int32 DeviceSelectHostFileFunction(DDDFile *fd, char *filename, uint32 *pSize);


/*****************************************************************************
*/
	int32
DeviceSelectHostFile(DDDFile *fd, char *filename, uint32 *pSize)
{
	DeviceSelectHostFileFunction *func;

	func = (DeviceSelectHostFileFunction *)
		FindDDDFunction(fd->fd_DDD, DDDCMD_HOST_SELECTFILE);
	if (func == NULL)
		return -1;
	return (*func)(fd, filename, pSize);
}
