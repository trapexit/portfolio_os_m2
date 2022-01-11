/*
 *	@(#) diplib.reloc.c 96/07/22 1.6
 *	Copyright 1996, The 3DO Company
 *
 * Relocate kernel.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "loader/loader3do.h"
#include "kernel/list.h"
#include "kernel/listmacros.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"


extern const DipirRoutines *dipr;

/*****************************************************************************
*/
static void *
Alloc(void *vdt, uint32 size)
{
	void *buffer;
	DipirTemp *dt = vdt;

	size = (size + 7) & ~7;
	buffer = AllocScratch(dt, size, 0);
	(*dt->dt_BootGlobals->bg_AddBootAlloc)(buffer, size, BA_OSDATA);
	return buffer;
}

/*****************************************************************************
*/
static int32
Free(void *vdt, void *buffer, uint32 size)
{
	BootAlloc *ba;
	DipirTemp *dt = vdt;

	size = (size + 7) & ~7;
	ScanList(&dt->dt_BootGlobals->bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Start == buffer && ba->ba_Size == size)
		{
			(*dt->dt_BootGlobals->bg_DeleteBootAlloc)
				(ba->ba_Start, ba->ba_Size, ba->ba_Flags);
			return 0;
		}
	}
	return -1;
}

/*****************************************************************************
*/
DIPIR_RETURN
RelocateKernel(DDDFile *fd, BootInfo *bootInfo)
{
	OSComponentNode *node;

	/* Kernel is always first in the component list. */
	node = (OSComponentNode *) FIRSTNODE(&bootInfo->bi_ComponentList);
	bootInfo->bi_KernelModule = 
		RelocateBinary((struct Elf32_Ehdr *) node->cn_Addr,
			Alloc, Free, fd->fd_DipirTemp);
	if (bootInfo->bi_KernelModule == NULL)
	{
		PRINTF(("Relocate kernel failed!\n"));
		return DIPIR_RETURN_TROJAN;
	}

	return DIPIR_RETURN_OK;
}
