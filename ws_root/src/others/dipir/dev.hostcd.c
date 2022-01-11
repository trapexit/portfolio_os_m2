/*
 *	@(#) dev.hostcd.c 96/08/22 1.10
 *	Copyright 1995, The 3DO Company
 *
 * Dipir Device Driver (DDD) for the "hostcd" device.
 */

#include "kernel/types.h"
#include "hardware/PPCasm.h"
#include "hardware/debugger.h"
#include "file/directory.h"
#include "dipir/hw.host.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.cd.h"
#include "hostcd.h"

typedef struct HostCDContext
{
	void	*hc_ReferenceToken;
	uint32	 hc_NumBlocks;
	uint32	 hc_BlockSize;
} HostCDContext;

const DipirRoutines *dipr;


/*****************************************************************************
 Close the host device.
*/
	static int32
HostCDClose(DDDFile *fd)
{
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;

	/* DismountHostCD(fd->fd_HWResource, ctx->hc_ReferenceToken); */
	DipirFree(fd->fd_DriverData);
	fd->fd_DriverData = NULL;
	return 0;
}

/*****************************************************************************
 Open the host device.
*/
	static int32
HostCDOpen(DDDFile *fd)
{
	int32 err;
	HostCDContext *ctx;
	uint32 numBlocks, blockSize;
	void *refToken;
	HWResource_Host *res;


	dipr = fd->fd_DipirTemp->dt_DipirRoutines;

	err = MountHostCD(fd->fd_HWResource, 0, &refToken, &numBlocks, &blockSize);
	if (err < 0)
	{
		if (err == -715722493) /* 0xd556f103 = No such filesystem */
			return DDD_OPEN_IGNORE;
		EPRINTF(("HostCD: cannot mount unit 0 (err %x)\n", err));
		return err;
	}
	ctx = DipirAlloc(sizeof(HostCDContext), 0);
	ctx->hc_ReferenceToken = refToken;
	ctx->hc_NumBlocks      = numBlocks;
	ctx->hc_BlockSize      = blockSize;
	fd->fd_DriverData      = ctx;
	fd->fd_BlockSize       = blockSize;
	fd->fd_AllocFlags |= ALLOC_DMA;

	res = (HWResource_Host *)fd->fd_HWResource->dev.hwr_DeviceSpecific;
	res->host_ReferenceToken = refToken;
	res->host_BlockSize      = blockSize;
	res->host_NumBlocks      = numBlocks;
	PRINTF(("Set devspec %x: %x %x %x\n", res, res->host_ReferenceToken, 
		res->host_BlockSize, res->host_NumBlocks));
	return 0;
}

/*****************************************************************************
 Read data from the host file.
*/
	static void *
HostCDReadAsync(DDDFile *fd, uint32 blk, uint32 nblks, void *buffer)
{
	HostCDContext *ctx;

	ctx = (HostCDContext *)fd->fd_DriverData;
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	return ReadHostCDAsync(fd->fd_HWResource, ctx->hc_ReferenceToken, 
			blk, nblks * DISC_BLOCK_SIZE, buffer);
}

/*****************************************************************************
 Wait for the last HostCDReadAsync to complete.
*/
	static int32
HostCDWaitRead(DDDFile *fd, void *id)
{
	int32 nbytes;

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	nbytes = WaitReadHostCD(fd->fd_HWResource, id);
	if (nbytes < 0)
		return nbytes;
	return (nbytes + DISC_BLOCK_SIZE - 1) / DISC_BLOCK_SIZE;
}

/*****************************************************************************
*/
	static int32
HostCDGetInfo(DDDFile *fd, DeviceInfo *info)
{
	HostCDContext *ctx = (HostCDContext *)(fd->fd_DriverData);

	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	info->di_Version = 1;
	info->di_BlockSize = ctx->hc_BlockSize;
	info->di_FirstBlock = 0;
	info->di_NumBlocks = ctx->hc_NumBlocks;
	return 0;
}

/*****************************************************************************
 Fake some disc flags.
*/
	static int32
HostCDGetCDFlags(DDDFile *fd, uint32 *flags)
{
	TOUCH(fd);
	*flags = 0;
	return 0;
}

/***************************************************************************
 Fake a table of contents.
*/
	static int32
HostCDGetTOC(DDDFile *fd, uint32 track, TOCInfo *ti)
{
	TOUCH(fd);
	TOUCH(track);
	ti->toc_AddrCntrl = ACB_DIGITAL_TRACK;
	ti->toc_CDROMAddr_Min = 0;
	ti->toc_CDROMAddr_Sec = 0;
	ti->toc_CDROMAddr_Frm = 0;
	return 0;
}

/***************************************************************************
 Fake the disc info.
*/
static int32
HostCDGetDiscInfo(DDDFile *fd, DiscInfo *di)
{
	uint32 endBlock;
	HostCDContext *ctx = (HostCDContext *)(fd->fd_DriverData);

	di->di_DiscId = 0;
	di->di_FirstTrackNumber = 1;
	di->di_LastTrackNumber = 1;
	endBlock = ctx->hc_NumBlocks - 1;
	di->di_MSFEndAddr_Min = endBlock / (FRAMEperSEC*SECperMIN);
	di->di_MSFEndAddr_Sec = (endBlock % (FRAMEperSEC*SECperMIN)) / FRAMEperSEC;
	di->di_MSFEndAddr_Frm = endBlock % FRAMEperSEC;
	return 0;
}

/*****************************************************************************
*/
static int32
HostCDGetWData(DDDFile *fd, uint32 block, uint32 speed, 
	uint32 bufSize, uint32 totalSize,
	int32 (*Callback)(DDDFile *fd, void *callbackArg, void *buf, uint32 bufSize),
	void *callbackArg)
{
	TOUCH(fd);
	TOUCH(block);
	TOUCH(speed);
	TOUCH(bufSize);
	TOUCH(totalSize);
	TOUCH(Callback);
	TOUCH(callbackArg);

	return DDD_OPEN_IGNORE;
}

/*****************************************************************************
*/
static const DDDCmdTable HostCDCmdTable[] =
{
	{ DDDCMD_OPEN,		(DDDFunction *) HostCDOpen },
	{ DDDCMD_CLOSE,		(DDDFunction *) HostCDClose },
	{ DDDCMD_READASYNC,	(DDDFunction *) HostCDReadAsync },
	{ DDDCMD_WAITREAD,	(DDDFunction *) HostCDWaitRead },
	{ DDDCMD_GETINFO,	(DDDFunction *) HostCDGetInfo },
	/* CD-specific commands */
	{ DDDCMD_CD_GETFLAGS,	(DDDFunction *) HostCDGetCDFlags },
	{ DDDCMD_CD_GETINFO,	(DDDFunction *) HostCDGetDiscInfo },
	{ DDDCMD_CD_GETTOC,	(DDDFunction *) HostCDGetTOC },
	{ DDDCMD_CD_GETWDATA,	(DDDFunction *) HostCDGetWData },
	{ 0 }
};

/*****************************************************************************
*/

static const DDD HostCDDDD = { HostCDCmdTable };

	DDD *
InitDriver(DipirTemp *dt)
{
	dipr = dt->dt_DipirRoutines;
	InitHost(dt);
	return &HostCDDDD;
}
