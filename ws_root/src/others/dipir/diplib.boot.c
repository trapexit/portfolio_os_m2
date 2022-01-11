/*
 *	@(#) diplib.boot.c 96/07/17 1.1
 *	Copyright 1996, The 3DO Company
 *
 * Booting functions.
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
void
DipirWillBoot(DipirTemp *dt)
{
	BootAlloc *ba;
	bootGlobals *bg = dt->dt_BootGlobals;

Again:
	ScanList(&bg->bg_BootAllocs, ba, BootAlloc)
	{
		if (ba->ba_Flags & BA_OSDATA)
		{
			PRINTF(("Deleting BootAlloc %x,%x,%x\n",
				ba->ba_Start, ba->ba_Size, ba->ba_Flags));
			(*bg->bg_DeleteBootAlloc)
				(ba->ba_Start, ba->ba_Size, ba->ba_Flags);
			goto Again;
		}
	}
	WillBoot();
}
