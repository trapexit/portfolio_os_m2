/*  :ts=8 bk=0
 *
 * viewlist.c:	Code to manage ViewList Items.
 *
 * @(#) viewlist.c   96/08/03  1.7
 *
 * Leo L. Schwab					9504.11
 */
#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/super.h>

#include <graphics/projector.h>
#include <graphics/view.h>

#include <string.h>

#include "protos.h"


/***************************************************************************
 * Autodoc for ViewList Item.
 */
/**
|||	AUTODOC -public -class Items -name ViewList
|||	An anchor for a heirarchy of sub-Views.
|||
|||	  Description
|||
|||	    This Item is used as an anchor point for a heirarchy of
|||	    subordinate View and ViewList Items.  With a ViewList, an entire
|||	    heirarchy of Views may be moved or depth-arranged as a single
|||	    unit.  This permits the application -- or the user -- to move an
|||	    application's Views without disturbing their relative positions
|||	    and layering priority.
|||
|||	    ViewLists do not themselves display any imagery.
|||
|||	  Tag Arguments
|||
|||	    VIEWTAG_LEFTEDGE (int32)
|||	        Position of the left-most edge of the ViewList, in display
|||	        coordinates.  This value will be used to offset all
|||	        subordinate View and ViewList Items.
|||
|||	    VIEWTAG_TOPEDGE (int32)
|||	        Position of the top-most edge of the ViewList, in display
|||	        coordinates.  This value will be used to offset all
|||	        subordinate View and ViewList Items.
|||
|||	  Note
|||
|||	    You are strongly urged to give your root ViewList Item a
|||	    human-readable name (using TAG_ITEM_NAME) so that they may be
|||	    meaningfully interpreted and manipulated by the user.
|||
|||	  Folio
|||
|||	    graphics
|||
|||	  Item Type
|||
|||	    GFX_VIEWLIST_NODE
|||
|||	  Create
|||
|||	    CreateItem()
|||
|||	  Delete
|||
|||	    DeleteItem()
|||
|||	  Use
|||
|||	    AddViewToViewList(), OrderViews(), RemoveView()
|||
|||	  Associated Files
|||
|||	    <graphics/view.h>
|||
|||	  See Also
|||
|||	    CreateItem(), AddViewToViewList(), OrderViews(), RemoveView(),
|||	    View(@)
|||
**/
static Err
icvl
(
struct ViewList	*vl,
uint32		*flags,
uint32		t,
uint32		a
)
{
	TOUCH (flags);
	switch (t) {
	case VIEWTAG_LEFTEDGE:
		vl->vl_LeftEdge = a;
		break;
	case VIEWTAG_TOPEDGE:
		vl->vl_TopEdge = a;
		break;
	default:
		return (GFX_ERR_BADTAGARG);
	}
	return (0);
}



Item
createviewlist (
struct ViewList	*vl,
struct TagArg	*ta
)
{
	InitList (&vl->vl_SubViews, vl->vl.n_Name);
	vl->vl_TopEdge = vl->vl_LeftEdge = 0;

	return (modifyviewlist (vl, ta));
}


Item
modifyviewlist (
struct ViewList	*clientvl,
struct TagArg	*ta
)
{
	Projector	*p;
	ViewList	vl;
	ModifyInfo	mi;
	Err		err;

	memcpy (&vl, clientvl, sizeof (ViewList));

	if ((err = TagProcessor (&vl, ta, icvl, NULL)) < 0)
		return (err);

	/*
	 * Swap validated workspace into client ViewList and attempt update.
	 */
	memswap (clientvl, &vl, sizeof (ViewList));

	if (p = clientvl->vl_Projector) {
		mi.mi_View = (View *) clientvl;
		mi.mi_OldView = (View *) &vl;
		mi.mi_TagArgs = ta;
		if ((err = (p->p_Update) (p, PROJOP_MODIFY, &mi)) < 0) {
			/*
			 * Phooey and double phooey!
			 */
			memcpy (clientvl, &vl, sizeof (ViewList));
			return (err);
		}
	}

	return (clientvl->vl.n_Item);
}


Err
deleteviewlist (
struct ViewList	*vl
)
{
	register View	*v;

	/*
	 * Disconnect all subviews.
	 */
	while (v = (View *) RemHead (&vl->vl_SubViews)) {
		v->v.n_Next = v->v.n_Prev = NULL;
		if (v->v.n_Type == GFX_VIEWLIST_NODE) {
			/*
			 * See if we're about to orphan a ViewList.
			 * If so, NULL out the Projector field.
			 */
			if (IsEmptyList (&((ViewList *) v)->vl_SubViews))
				((ViewList *) v)->vl_Projector = NULL;
		}
	}

	/*
	 * Remove from any super-view.
	 */
	if (vl->vl.n_Next) {
		RemNode ((Node *) vl);
		(vl->vl_Projector->p_Update) (vl->vl_Projector,
					      PROJOP_REMOVEVIEW,
					      vl);
	}

	return (0);
}


Item
findviewlist (
struct TagArg	*ta
)
{
	TOUCH (ta);
	return (GFX_ERR_NOTSUPPORTED);
}


Item
openviewlist (
struct ViewList	*vl,
struct TagArg	*ta
)
{
	TOUCH (ta);
	return (vl->vl.n_Item);
}


Err
closeviewlist (
struct ViewList	*v,
struct Task	*t
)
{
	TOUCH (v);
	TOUCH (t);
	return (0);
}
