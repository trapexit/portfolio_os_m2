/*
 *	@(#) dev.hostfs.c 96/08/22 1.18
 *	Copyright 1995, The 3DO Company
 *
 * Dipir Device Driver (DDD) for the HostFS device.
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/debugger.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.host.h"
#include "hostfs.h"

typedef struct HostContext
{
	RefToken	hc_RootDirToken;
	RefToken	hc_CurrDirToken;
	RefToken	hc_CurrFileToken;
} HostContext;

const DipirRoutines *dipr;


/*****************************************************************************
 Select a new host file as the current file.
*/
	static int32
SelectHostFile(DDDFile *fd, char *filename, uint32 *pSize)
{
	HostContext *ctx = (HostContext *)(fd->fd_DriverData);

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	CloseHostFSFile(fd->fd_HWResource, ctx->hc_CurrFileToken);
	return OpenHostFSFile(fd->fd_HWResource, ctx->hc_CurrDirToken, 
		filename, &ctx->hc_CurrFileToken, pSize, &fd->fd_BlockSize);
}

/*****************************************************************************
*/
	static int32
ChangeHostDir(DDDFile *fd, char *dirname)
{
	HostContext *ctx = (HostContext *)(fd->fd_DriverData);
	RefToken dirToken;
	int32 err;

	err = OpenHostFSFile(fd->fd_HWResource, ctx->hc_CurrDirToken, 
			dirname, &dirToken, NULL, NULL);
	if (err < 0)
		return err;
	if (ctx->hc_CurrDirToken != ctx->hc_RootDirToken)
		CloseHostFSFile(fd->fd_HWResource, ctx->hc_CurrDirToken);
	ctx->hc_CurrDirToken = dirToken;
	return 0;
}

/*****************************************************************************
 Close the host device.
*/
	static int32
HostClose(DDDFile *fd)
{
	HostContext *ctx = (HostContext *)(fd->fd_DriverData);

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	CloseHostFSFile(fd->fd_HWResource, ctx->hc_CurrFileToken);
	CloseHostFSFile(fd->fd_HWResource, ctx->hc_CurrDirToken);
	DismountHostFS(fd->fd_HWResource, ctx->hc_RootDirToken);
	DipirFree(fd->fd_DriverData);
	fd->fd_DriverData = NULL;
	return 0;
}

/*****************************************************************************
 Open the host device.
*/
	static int32
HostOpen(DDDFile *fd)
{
	HostContext *ctx;
	RefToken dirToken;
	int32 err;

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;

	err = MountHostFS(fd->fd_HWResource, "remote", &dirToken);
	if (err < 0)
	{
		EPRINTF(("Host: cannot mount /remote\n"));
		return DDD_OPEN_IGNORE;
	}

	ctx = DipirAlloc(sizeof(HostContext), 0);
	ctx->hc_RootDirToken = ctx->hc_CurrDirToken = dirToken;
	ctx->hc_CurrFileToken = NULL_TOKEN;
	fd->fd_DriverData = ctx;
	fd->fd_AllocFlags |= ALLOC_DMA;

	if (ChangeHostDir(fd, "System.m2") < 0)
	{
		EPRINTF(("Cannot chdir System.m2\n"));
		HostClose(fd);
		return -1;
	}
	if (ChangeHostDir(fd, "Boot") < 0)
	{
		EPRINTF(("Cannot chdir Boot\n"));
		HostClose(fd);
		return -1;
	}
	if (SelectHostFile(fd, "remotevol", NULL) < 0)
	{
		EPRINTF(("Cannot open remotevol\n"));
		HostClose(fd);
		return -1;
	}
	return 0;
}

/*****************************************************************************
 Read data from the currently selected host file.
*/
	static void *
HostReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer)
{
	HostContext *ctx = (HostContext *)(fd->fd_DriverData);

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	if (ctx->hc_CurrFileToken == NULL_TOKEN)
	{
		/* No file currently selected. */
		EPRINTF(("No host file selected\n"));
		return 0;
	}
	return ReadHostFSFileAsync(fd->fd_HWResource, ctx->hc_CurrFileToken, 
			blk, nblks * fd->fd_BlockSize, buffer);
}

/*****************************************************************************
 Wait for the last HostReadAsync to complete.
*/
	static int32
HostWaitRead(DDDFile *fd, void *id)
{
	int32 nbytes;

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	nbytes = WaitReadHostFSFile(fd->fd_HWResource, id);
	if (nbytes < 0)
		return nbytes;
	return (nbytes + fd->fd_BlockSize - 1) / fd->fd_BlockSize;
}

/*****************************************************************************
*/
	static int32
HostGetInfo(DDDFile *fd, DeviceInfo *info)
{
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	info->di_Version = 1;
	info->di_BlockSize = fd->fd_BlockSize;
	info->di_FirstBlock = 0;
	info->di_NumBlocks = 0;
	return 0;
}



/*****************************************************************************
*/
static const DDDCmdTable HostCmdTable[] =
{
	{ DDDCMD_OPEN,		(DDDFunction *) HostOpen },
	{ DDDCMD_CLOSE,		(DDDFunction *) HostClose },
	{ DDDCMD_READASYNC,	(DDDFunction *) HostReadAsync },
	{ DDDCMD_WAITREAD,	(DDDFunction *) HostWaitRead },
	{ DDDCMD_GETINFO,	(DDDFunction *) HostGetInfo },
	/* Commands specific to the host device */
	{ DDDCMD_HOST_SELECTFILE,(DDDFunction *) SelectHostFile },
	{ 0 }
};

/*****************************************************************************
*/

static const DDD HostDDD = { HostCmdTable };

	DDD *
InitDriver(DipirTemp *dt)
{
	dipr = dt->dt_DipirRoutines;
	InitHost(dt);
	return &HostDDD;
}
