/*
 * @(#) ddd.host.h 95/09/01 1.1
 * Copyright 1995, The 3DO Company
 *
 * Definitions relating to DDD commands specific to the "host" device.
 */

#define	DDDCMD_HOST_SELECTFILE	101

extern int32 DeviceSelectHostFile(DDDFile *fd, char *filename, uint32 *pSize);
