/*
 *	@(#) dipir.sysromapp.c 96/07/31 1.19
 *	Copyright 1995, The 3DO Company
 *
 * Special device-dipir for loading the RomApp OS from the system ROM.
 * This is used when we want to run a RomApp and no device has a RomApp OS.
 * This loads the default RomApp OS from the system ROM.
 */

#include "kernel/types.h"
#include "loader/loader3do.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

char Copyright[] = "Copyright (c) 1996 The 3DO Company, All rights reserved";

const DipirRoutines *dipr;

typedef struct CTX
{
	DDDFile *	ctx_Fd;
	uint8 *		ctx_FreeP;
	const uint32 *	ctx_FileType;
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
	return p;
}

/*****************************************************************************
*/
static int32
NextFile(void *fileCtx, void **pAddr)
{
	CTX *ctx = fileCtx;
	uint32 nread;
	uint32 nblks;
	void *buffer;
	RomTag rt;

	if (*ctx->ctx_FileType == 0)
		return 0;
	if (FindRomTag(ctx->ctx_Fd, RT_SUBSYS_ROM, *ctx->ctx_FileType, 0, &rt) <= 0)
	{
		PRINTF(("Cannot find OS component %x\n", *ctx->ctx_FileType));
		return -1;
	}
	buffer = Alloc(ctx, rt.rt_Size);
	if (buffer == NULL)
	{
		EPRINTF(("SysRomApp: cannot alloc %x for %x\n",
			rt.rt_Size, *ctx->ctx_FileType));
		return -1;
	}
	nblks = (rt.rt_Size + ctx->ctx_Fd->fd_BlockSize - 1) /
			ctx->ctx_Fd->fd_BlockSize;
	nread = ReadSync(ctx->ctx_Fd,
		ctx->ctx_Fd->fd_RomTagBlock + rt.rt_Offset, nblks, buffer);
	if (nread != nblks)
	{
		PRINTF(("SysRomApp: bad read for %x: %x != %x\n",
			*ctx->ctx_FileType, nread, nblks));
		return -1;
	}
	*pAddr = buffer;
	ctx->ctx_FileType++;
	return rt.rt_Size;
}

/*****************************************************************************
*/
static DIPIR_RETURN
LoadRomApp(DDDFile *fd, uint32 dipirID, uint32 dddID)
{
	BootInfo *	bootInfo;
	uint32		osVersion;
	RomTag		rt;
	void *		buffer;
	CTX		ctx;

	static const uint32 componentTypes[] =
	{
		ROM_KERNEL_ROM,
		ROM_OPERATOR,
		ROM_FS,
		ROM_KINIT_ROM,
		ROM_OPINIT,
		ROM_FSINIT,
		0
	};

	DipirWillBoot(fd->fd_DipirTemp);
	ctx.ctx_Fd = fd;
	ctx.ctx_FreeP = buffer =
		AllocScratch(fd->fd_DipirTemp, MAX_OS_SIZE, OS_SCRATCH_BUFFER);
	ctx.ctx_FileType = componentTypes;
	bootInfo = LoadOS(NextFile, &ctx, Alloc, &ctx);
	if (bootInfo == NULL || ISEMPTYLIST(&bootInfo->bi_ComponentList))
		return DIPIR_RETURN_TROJAN;
	if (RelocateKernel(fd, bootInfo) < 0)
		return DIPIR_RETURN_TROJAN;
	if (FindRomTag(fd, RT_SUBSYS_ROM, ROM_KERNEL_ROM, 0, &rt) <= 0)
		return DIPIR_RETURN_TROJAN;
	osVersion = MakeInt16(rt.rt_Version, rt.rt_Revision);
	SetBootOS(fd->fd_VolumeLabel, osVersion, bootInfo,
		bootInfo->bi_KernelModule->li->entryPoint,
		rt.rt_OSFlags, rt.rt_OSReservedMem, 0xFFFFFFFF,
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
		return DIPIR_RETURN_OK;
	case DIPIR_LOADROMAPP:
		return LoadRomApp(fd, dipirID, dddID);
	}
	return DIPIR_RETURN_TROJAN;
}
