/*  :ts=8 bk=0
 *
 * util.c:	Miscellaneous utility functions.
 *
 * @(#) util.c   96/08/03  1.11
 *
 * Leo L. Schwab					9504.11
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/semaphore.h>
#include <kernel/super.h>

#include <graphics/projector.h>
#include <graphics/bitmap.h>

#include "protos.h"


/***************************************************************************
 * Display locking routines.
 */
/**
|||	AUTODOC -public -class graphicsfolio -name LockDisplay
|||	Lock down changes to a Projector.
|||
|||	  Synopsis
|||
|||	    Err LockDisplay (Item projector)
|||
|||	  Description
|||
|||	    This routine locks the current state of supplied Projector.
|||	    When this routine is called, further changes applied via
|||	    ModifyGraphicsItem(), AddViewToViewList(), and/or OrderViews()
|||	    do not update the display managed by the Projector.  When
|||	    UnlockDisplay() is finally called, all changes subsequent to the
|||	    call to LockDisplay() will be integrated into the display.
|||
|||	    Each call to AddViewToViewList(), OrderViews(), and many changes
|||	    via ModifyGraphicsItem() require the system to completely
|||	    recompile the VDL list used by the hardware to generate the
|||	    display.  If many Views are to be updated as part of a single
|||	    operation, many such recompilations are performed, which can be
|||	    time-consuming.  LockDisplay() permits the client to defer the
|||	    recompilation until all such modifications are complete.
|||	    UnlockDisplay() will then cause all changes to be compiled and
|||	    made visible in a single operation.
|||
|||	    The argument projector refers to which Projector you wish to
|||	    lock down.  The idiomatic value zero (0) locks down the default
|||	    Projector.
|||
|||	    This call nests; each call to LockDisplay() must have a
|||	    corresponding call to UnlockDisplay().
|||
|||	    Any number of tasks may lock a Projector at a given time.
|||
|||	    View signals will not be sent while the Projector lock is in
|||	    force.  When the lock is released and the changes integrated,
|||	    the signals will be sent.
|||
|||	  Arguments
|||
|||	    projector
|||	        The Item number of the Projector to be locked down (zero for
|||	        default Projector).
|||
|||	  Return Value
|||
|||	    1
|||	        You successfully procured a lock on the Projector.
|||
|||	    0
|||	        The lock did not succeed.
|||
|||	    negative
|||	        Some other error.
|||
|||	  Caveats
|||
|||	    There is no way to tell if a Projector is currently locked by
|||	    someone.
|||
|||	    There is currently nothing to prevent a hostile program from
|||	    locking a Projector and never unlocking it, thereby preventing
|||	    any other task from using it.  Be good.  It would be a real
|||	    *shame* if your program did such a thing...
|||
|||	    This routine should probably be renamed to LockProjector.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>, <graphics/projector.h>
|||
|||	  See Also
|||
|||	    UnlockDisplay(), Projector(@)
|||
**/
Err
SuperLockDisplay (display)
Item	display;
{
	Projector	*p;

	if (display) {
		if (!(p = CheckItem
			   (display, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
			return (GFX_ERR_BADITEM);
	} else
		p = GBASE(gb_DefaultProjector);

	return (SuperInternalLockSemaphore (p->p_ViewListSema4,
					    SEM_SHAREDREAD));
}


/**
|||	AUTODOC -public -class graphicsfolio -name UnlockDisplay
|||	Release Projector lock; integrate all pending changes.
|||
|||	  Synopsis
|||
|||	    Err UnlockDisplay (Item projector)
|||
|||	  Description
|||
|||	    This routine is the companion to LockDisplay().  When called,
|||	    the prior lock made to the supplied Projector is released.  This
|||	    call will also trigger a recompilation of the display if:  it is
|||	    the last lock released, there are outstanding changes to be
|||	    integrated, and the Projector is currently active.  After such
|||	    recompilation, View signals will be sent if appropriate.
|||
|||	    You cannot unlock a Projector you haven't locked.
|||
|||	  Arguments
|||
|||	    projector
|||	        The Item number of the Projector to be unlocked (zero for
|||	        default Projector).
|||
|||	  Return Value
|||
|||	    This routine will return zero upon successful completion, or a
|||	    (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>, <graphics/projector.h>
|||
|||	  See Also
|||
|||	    LockDisplay(), Projector(@)
|||
**/
Err
SuperUnlockDisplay (display)
Item	display;
{
	Projector	*p;
	Err		err;

	if (display) {
		if (!(p = CheckItem
			   (display, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
			return (GFX_ERR_BADITEM);
	} else
		p = GBASE(gb_DefaultProjector);

	if ((err = SuperInternalUnlockSemaphore (p->p_ViewListSema4)) < 0)
		return (err);

	if (!p->p_ViewListSema4->sem_NestCnt  &&
	    p->p_PendingRecompile)
	{
		err = (p->p_Update) (p, PROJOP_UNLOCKDISPLAY, NULL);

		p->p_PendingRecompile = 0;   /*  Release FIRQ.	*/
		return (err);
	}
	return (0);
}


/***************************************************************************
 * Miscellaneous utilities.
 */
Err
unimplemented ()
{
	return (GFX_ERR_NOTSUPPORTED);
}
