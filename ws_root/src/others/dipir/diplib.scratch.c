/*
 *	@(#) diplib.scratch.c 96/07/16 1.5
 *	Copyright 1996, The 3DO Company
 *
 * OS scratch buffer functions.
 * This is a diplib function, callable from device-dipirs.
 */

#include "kernel/types.h"
#include "kernel/list.h"
#include "dipir.h"
#include "notsysrom.h"
#include "diplib.h"

extern const DipirRoutines *dipr;

/*****************************************************************************
*/
void *
AllocScratch(DipirTemp *dt, uint32 size, void *minAddr)
{
	void *startp;
	void *endp;
	void *endRAM;
	BootAlloc *ba;
	bootGlobals *bg = dt->dt_BootGlobals;

	endRAM = (uint8*)bg->bg_SystemRAM.mr_Start + bg->bg_SystemRAM.mr_Size;
	ScanList(&bg->bg_BootAllocs, ba, BootAlloc)
	{
		startp = (uint8*)ba->ba_Start + ba->ba_Size;
		endp = ISNODE(&bg->bg_BootAllocs, ba->ba_Next) ? 
				ba->ba_Next->ba_Start : endRAM;
		if (startp < minAddr)
			startp = minAddr;
		if (startp >= endp || (uint8*)endp - (uint8*)startp < size)
			continue;
		PRINTF(("AllocScratch(%x) ret %x\n", size, startp));
		return startp;
	}
	return NULL;
}
