/*
 *	@(#) diplib.comp.c 96/07/17 1.6
 *	Copyright 1996, The 3DO Company
 *
 * Process OS components in a component file.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

typedef struct CTX
{
	uint8 *		ctx_FreeP;
	CompHeader *	ctx_Comp;
} CTX;

extern const DipirRoutines *dipr;

/*****************************************************************************
 Alloc function, for LoadOS().
 Allocates consecutive memory starting after the component file image.
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
 Next-file function, for LoadOS().
 Get the next component from a component file image.
*/
static int32
NextFile(void *fileCtx, void **pAddr)
{
	CTX *ctx = fileCtx;
	CompHeader *comp;

	comp = ctx->ctx_Comp;
	if (comp->comp_Addr == 0)
		return 0;
	ctx->ctx_Comp = (CompHeader *)
		(((uint8*)comp->comp_Data) + comp->comp_Size);
	*pAddr = comp->comp_Data;
	return comp->comp_Size;
}

/*****************************************************************************
 Relocate all components in a component file.
*/
BootInfo *
LinkCompFile(DDDFile *fd, CompFileHeader *fh, uint32 fileSize)
{
	CTX ctx;
	BootInfo *bootInfo;

	ctx.ctx_Comp = (CompHeader *)(fh + 1);
	ctx.ctx_FreeP = ((uint8 *) fh) + fileSize;
	bootInfo = LoadOS(NextFile, &ctx, Alloc, &ctx);
	if (bootInfo == NULL || ISEMPTYLIST(&bootInfo->bi_ComponentList))
		return NULL;
	if (RelocateKernel(fd, bootInfo) < 0)
		return NULL;
	(*fd->fd_DipirTemp->dt_BootGlobals->bg_AddBootAlloc)
		(fh, ROUNDUP((uint8*)ctx.ctx_FreeP - (uint8*)fh, 4096), BA_OSDATA|BA_PREMULTITASK);
	return bootInfo;
}

