/*  :ts=8 bk=0
 *
 * init.c:	Initialization sequences.  (Maybe even purgeable?)
 *
 * @(#) init.c 96/07/08 1.22
 *
 * Leo L. Schwab					9504.10
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/interrupts.h>
#include <kernel/memlock.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>

#include <graphics/gfx_pvt.h>

#include <string.h>
#include <stdio.h>

#include "protos.h"


/***************************************************************************
 * Globals.
 */
extern char			folioname[];


/* Declarations for Mem Lock Stuff */
static MemLockHandler		teMemLock;


/***************************************************************************
 * Code.
 */
Err
initgbase ()
{
	MemRegion	*m;
	Err		err;

	/*
	 * Initialize lists.
	 */
	InitList (&GBASE(gb_BitmapList), NULL);
	InitList (&GBASE(gb_ProjectorList), NULL);

	/*
	 * Create semaphores.
	 */
	if ((err = SuperCreateSemaphore ("Projector List Lock", 0)) < 0)
		return (err);
	GBASE(gb_ProjectorListSema4) = LookupItem (err);

#if 0
	/*
	 * Misc stuff.
	 */
	GBASE(gb_VBLCount) =
	GBASE(gb_Misfires) =
	GBASE(gb_PrevField) = 0;
#endif

	/*
	 * Locate beginning of displayable RAM (which I incorrectly refer
	 * to as VRAM).
	 */
        m = (MemRegion *)KernelBase->kb_MemRegion;

	GBASE(gb_VRAMBase) = m->mr_MemBase;

	/*
	 * Install MemLock handler.
	 */
	teMemLock.mlh_CallBack = memlockhandler;
	SuperInternalInsertMemLockHandler (&teMemLock);

	return (0);
}
