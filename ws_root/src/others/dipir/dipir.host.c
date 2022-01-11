/*
 *	@(#) dipir.host.c 96/07/31 1.21
 *	Copyright 1995, The 3DO Company
 *
 * Special device-dipir for loading the system files from the host.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "loader/loader3do.h"
#include "dipir.h"
#include "notsysrom.h"
#include "ddd.host.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;

typedef struct CTX
{
	DDDFile *	ctx_Fd;
	uint8 *		ctx_FreeP;
	char **		ctx_Filename;
} CTX;


/*****************************************************************************
*/
static void *
Alloc(void *allocCtx, uint32 size)
{
	CTX *ctx = allocCtx;
	void *p;

	size = (size + sizeof(uint32)-1) & ~(sizeof(uint32)-1);
	p = ctx->ctx_FreeP;
	ctx->ctx_FreeP += size;
	PRINTF(("Host.Alloc(%x,%x) ret %x\n", allocCtx, size, p));
	return p;
}

/*****************************************************************************
*/
static int32
NextFile(void *fileCtx, void **pAddr)
{
	CTX *ctx = fileCtx;
	uint32 size;
	uint32 nread;
	uint32 nblks;
	void *buffer;

	if (*ctx->ctx_Filename == NULL)
		return 0;
	PRINTF(("Host.NextFile %s\n", *ctx->ctx_Filename));
	if (DeviceSelectHostFile(ctx->ctx_Fd, *ctx->ctx_Filename, &size) < 0)
	{
		EPRINTF(("HostFile: cannot select %s\n", *ctx->ctx_Filename));
		return -1;
	}
	buffer = Alloc(ctx, size);
	if (buffer == NULL)
	{
		EPRINTF(("HostFile: cannot alloc %x for %s\n",
			size, *ctx->ctx_Filename));
		return -1;
	}
	nblks = (size + ctx->ctx_Fd->fd_BlockSize - 1) / ctx->ctx_Fd->fd_BlockSize;
	nread = ReadSync(ctx->ctx_Fd, 0, nblks, buffer);
	PRINTF(("Host.NextFile nblks %x, nread %x\n", nblks, nread));
	if (nread != nblks)
	{
		PRINTF(("HostFile: bad read for %s: %x != %x\n",
			*ctx->ctx_Filename, nread, nblks));
		return -1;
	}
	PRINTF(("Host.NextFile ret %x,%x\n", buffer, size));
	ctx->ctx_Filename++;
	*pAddr = buffer;
	return size;
}

/*****************************************************************************
*/
static DIPIR_RETURN
Validate(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	BootInfo *	bootInfo;
	_3DOBinHeader *thdo;
	uint32		osVersion;
	void *		buffer;
	CTX		ctx;

	static const char *componentNames[] =
	{
		"kernel", /* Must be first */
		"operator",
		"filesystem",
		"kernel.init",
		"operator.init",
		"filesystem.init",
		NULL
	};

	DipirWillBoot(fd->fd_DipirTemp);
	ctx.ctx_Fd = fd;
	ctx.ctx_FreeP = buffer =
		AllocScratch(fd->fd_DipirTemp, MAX_OS_SIZE, OS_SCRATCH_BUFFER);
	ctx.ctx_Filename = componentNames;
	bootInfo = LoadOS(NextFile, &ctx, Alloc, &ctx);
	if (bootInfo == NULL || ISEMPTYLIST(&bootInfo->bi_ComponentList))
		return DIPIR_RETURN_TROJAN;
	if (RelocateKernel(fd, bootInfo) < 0)
		return DIPIR_RETURN_TROJAN;
	thdo = Get3DOBinHeader(bootInfo->bi_KernelModule);
	osVersion = MakeInt16(thdo->_3DO_Item.n_Version,
				thdo->_3DO_Item.n_Revision);
	SetBootOS(fd->fd_VolumeLabel, osVersion, bootInfo,
		bootInfo->bi_KernelModule->li->entryPoint,
		OS_NORMAL | OS_ROMAPP | OS_ANYTITLE, OSRESERVEDMEM, 0xFFFFFFFF,
		fd->fd_HWResource, dddID, dipirID);
	(*fd->fd_DipirTemp->dt_BootGlobals->bg_AddBootAlloc)
		(buffer, ROUNDUP((uint8*)ctx.ctx_FreeP - (uint8*)buffer, 4096), BA_OSDATA|BA_PREMULTITASK);
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
	dipr = fd->fd_DipirTemp->dt_DipirRoutines;
	switch (cmd)
	{
	case DIPIR_VALIDATE:
		return Validate(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
