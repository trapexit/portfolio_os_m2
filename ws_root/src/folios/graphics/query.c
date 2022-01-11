/*  :ts=8 bk=0
 *
 * query.c:	Routines to query things graphics knows about.
 *
 * @(#) query.c 96/09/17 1.5
 *
 * Leo L. Schwab					9606.10
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>

#include <graphics/projector.h>

#include "protos.h"


/***************************************************************************
 * Autodoc and code.
 */
/**
|||	AUTODOC -class graphicsfolio -name QueryGraphics
|||	Interrogate the graphics folio for useful information.
|||
|||	  Synopsis
|||
|||	    Err QueryGraphics (const struct TagArg *args)
|||
|||	    Err QueryGraphicsVA (uint32 tag, ...)
|||
|||	  Description
|||
|||	    This routine queries the current state of certain attributes of
|||	    the graphics folio.  Queries take the form of TagArg entries.
|||	    For each TagArg, the argument field is a pointer to the storage
|||	    where the desired information is to be deposited.
|||
|||	  Tag Arguments
|||
|||	    The following tag values may be submitted to this function:
|||
|||	    QUERYGFXTAG_DEFAULTPROJECTOR (Item *)
|||	        Returns the item number of the current default Projector.
|||
|||	  Arguments
|||
|||	    args
|||	        Pointer to an array of TagArgs.  The ta_Arg field of each
|||	        entry is a pointer to the storage where the requested
|||	        information is to be deposited.
|||
|||	  Return Value
|||
|||	    Returns zero upon successful completion or a (negative) error
|||	    code.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>
|||
|||	  See Also
|||
|||	    Projector(@)
|||
**/
static Err
iqg (int32 tag, const void *ret)
{

#ifdef BUILD_PARANOIA
	if (!IsMemWritable (ret, sizeof (uint32)))
		return (GFX_ERR_BADPTR);
#endif

	switch (tag) {
	case QUERYGFXTAG_DEFAULTPROJECTOR:
		*((Item *) ret) = GBASE(gb_DefaultProjector)->p.n_Item;
		break;
	default:
		return (GFX_ERR_BADTAGARG);
	}
	return 0;
}


/*
 * This is not a fully protected walk of the tag list.	However, since this
 * routine executes in user mode, this isn't a security issue.
 */
Err
QueryGraphics (ta)
const struct TagArg 	*ta;
{
	const TagArg	*state;
	Err 		err;

	err = WaitForProjectorLoader();
	if (err < 0)
		return (err);

	if (!(state = ta))
		return (err);

	while (ta = NextTagArg (&state)) {
		if ((err = iqg (ta->ta_Tag, ta->ta_Arg)) < 0)
			break;
	}
	return (err);
}
