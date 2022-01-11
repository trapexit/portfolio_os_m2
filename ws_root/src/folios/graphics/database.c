/*  :ts=8 bk=0
 *
 * database.c:	Viewtype database searching routines.
 *
 * @(#) database.c 96/08/01 1.16
 *
 * Leo L. Schwab					9512.05
 */
#include <kernel/types.h>
#include <kernel/super.h>

#include <graphics/projector.h>

#include "protos.h"


/**
|||	AUTODOC -class graphicsfolio -name NextViewTypeInfo
|||	Return a pointer to next ViewTypeInfo structure for a Projector.
|||
|||	  Synopsis
|||
|||	    struct ViewTypeInfo *
|||	     NextViewTypeInfo (Item projectorItem,
|||	                       const struct ViewTypeInfo *vti)
|||
|||	  Description
|||
|||	    This function is used to iterate through the list of
|||	    ViewTypeInfo descriptor structures available for a given
|||	    Projector.
|||
|||	    Given a pointer to a ViewTypeInfo structure and a Projector item
|||	    number, the function will return a pointer to the next available
|||	    ViewTypeInfo structure.  If there are no further ViewTypeInfos,
|||	    NULL will be returned.
|||
|||	    To obtain a pointer to the first ViewTypeInfo structure for a
|||	    given Projector, supply the value NULL for the parameter 'vti'.
|||
|||	    The parameter 'projectorItem' may either be a valid Projector
|||	    item number, or zero.  If zero, the default Projector is used.
|||
|||	    Combining this function with NextProjector() will permit you to
|||	    inspect the entire ViewTypeInfo database.
|||
|||	  Arguments
|||
|||	    projectorItem
|||	        Item number of a Projector, or zero for the default
|||	        Projector.
|||
|||	    vti
|||	        Pointer to a ViewTypeInfo structure, or NULL.
|||
|||	  Return Value
|||
|||	    A pointer to the next available ViewTypeInfo structure, or NULL
|||	    if the end of the list has been reached or if the Projector Item
|||	    was invalid.
|||
|||	  Example
|||
|||	    // This example searches the ViewTypeInfo structures available
|||	    // from the default Projector.
|||	    ViewTypeInfo        *vti
|||
|||	    vti = NULL;
|||	    while (vti = NextViewTypeInfo (0, vti)) {
|||
|||	        // At this point, you can inspect the contents of the
|||	        // ViewTypeInfo structure.
|||	    }
|||
|||	  Notes
|||
|||	    No significance should be inferred from the order in which
|||	    ViewTypeInfos are returned.
|||
|||	  Warning
|||
|||	    Very little checking is performed on the ViewTypeInfo structure
|||	    pointer you supply; GIGO is the rule here.
|||
|||	  Associated Files
|||
|||	    <graphics/view.h>, <graphics/projector.h>
|||
|||	  See Also
|||
|||	    NextProjector(), Projector(@)
|||
**/
struct ViewTypeInfo *
NextViewTypeInfo (pi, vti)
Item				pi;
const struct ViewTypeInfo	*vti;
{
	Projector	*p;

	if (pi)	{
		if (!(p = CheckItem (pi, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
			return (NULL);
	} else {
		if (WaitForProjectorLoader() < 0)
			return (NULL);

		p = GBASE(gb_DefaultProjector);
	}

	return ((p->p_NextViewType) (p, vti));
}


/***************************************************************************
 * Full database search.
 */
Err
lookupviewtype (vid, vtiptr, pptr)
int32			vid;
struct ViewTypeInfo	**vtiptr;
struct Projector	**pptr;
{
	Projector	*p;
	ViewTypeInfo	*vti;
	uint32		ptype;

	ptype = (vid & VIEWTYPE_PROJTYPE_MASK) >> VIEWTYPE_PROJTYPE_SHIFT;


	SuperInternalLockSemaphore (GBASE(gb_ProjectorListSema4), SEM_WAIT | SEM_SHAREDREAD);

	for (p = (Projector *) FIRSTNODE (&GBASE(gb_ProjectorList));
	     NEXTNODE (p);
	     p = (Projector *) NEXTNODE (p))
	{
		if (p->p_Type == ptype) {
			vti = NULL;
			while (vti = (p->p_NextViewType) (p, vti)) {
				if (vti->vti_Type == vid) {
					*vtiptr = vti;
					*pptr = p;
                                        SuperInternalUnlockSemaphore (GBASE(gb_ProjectorListSema4));
					return (0);
				}
			}
		}
	}

	SuperInternalUnlockSemaphore (GBASE(gb_ProjectorListSema4));

	return (GFX_ERR_BADVIEWTYPE);
}
