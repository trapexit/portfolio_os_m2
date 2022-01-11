/*  :ts=8 bk=0
 *
 * compiler.c:	VDL compiler.
 *
 * @(#) compiler.c 96/10/04 1.31
 *
 * Leo L. Schwab					9410.31
 ***************************************************************************
 * Copyright 1995 The 3DO Company.  All Rights Reserved.
 * Confidential and proprietary.  Do not disclose or distribute.
 ***************************************************************************
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <hardware/bda.h>
#include <hardware/m2vdl.h>
#include <hardware/PPCasm.h>

#include <graphics/gfx_pvt.h>
#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>

#include <stdlib.h>

#include "protos.h"


/***************************************************************************
 * #defines
 */
#define	OP_REPORTSIZE	0
#define	OP_COMPILE	1

#define	ADVANCE(p,delta)	((void *) ((uint32) (p) + (delta)))

#define	TRIP(x)		{ _tripaddress = (uint32)(x); _dcbf (&_tripaddress); }


/***************************************************************************
 * Local prototypes.
 */
static int viewedgecmp (struct ViewEdge	*, struct ViewEdge *);
static void sortedges (struct ViewEdge *, int32);


/***************************************************************************
 * Globals.
 */
uint32				_tripaddress;
uint32				_DBUG = 'Foo!';


/***************************************************************************
 * Code.
 */
/**
|||	AUTODOC -private -class graphicsfolio -name SuperInternalCompile
|||	Semi-general purpose ViewList heirarchy compiler.
|||
|||	  Synopsis
|||
|||	    Err SuperInternalCompile (struct ViewList   *vl,
|||	                              int32             nviews,
|||	                              struct Transition **tr_retn,
|||	                              int32             tr_size,
|||	                              int32             *starty,
|||	                              uint32            clearflags)
|||
|||	  Description
|||
|||	    This routine takes the supplied ViewList and compiles an array
|||	    of Transitions which may be interpreted by the Projector code to
|||	    build its display.  This function is provided as a convenience
|||	    for Projectors whose underlying hardware can leverage off this
|||	    model.  Projectors not so endowed will have to write their own
|||	    compilers.
|||
|||	    The ViewList is converted into a "top-down" view of the
|||	    heirarchy; the individual visible spans are expressed as
|||	    individual entries in the returned Transition array.
|||
|||	    The argument vl points to the ViewList to be compiled.
|||
|||	    The argument nviews contains a count of the number of Views to
|||	    be found in the ViewList's heirarchy.  This value is used to
|||	    pre-allocate the returned buffer (which is also used internally
|||	    as a workspace).
|||
|||	    The argument tr_retn points to the storage where a pointer to
|||	    the compiled Transition array will be deposited.  The Transition
|||	    array is allocated with MEMTYPE_TRACKSIZE; the Projector module
|||	    is responsible for freeing this memory (using TRACKED_SIZE).
|||
|||	    The argument tr_size is the size each entry in the returned
|||	    Transition array must have, in bytes.
|||
|||	    The argument starty points to the storage where the topmost Y
|||	    coordinate will be deposited.  The Transition array can span a
|||	    range greater than the visible display (encompassing Views that,
|||	    for example, are completely off the display).  The starty value
|||	    indicates the topmost Y coordinate of the first Transition
|||	    entry.
|||
|||	    The argument clearflags is a bit mask indicating which bits in
|||	    the v_ViewFlags field to clear.  The entire supplied View
|||	    heirarchy is traversed; all Views will have their v_ViewFlags
|||	    fields altered.  The bits set in clearflags indicate which bits
|||	    are to be cleared to zero.  This is typically used by Projectors
|||	    wishing to reset some internal state recorded in the upper half
|||	    of the flags word.
|||
|||	  Transition Structure
|||
|||	    Each element of the returned Transition array has at its head a
|||	    Transition structure.  SuperInternalCompile() understands and
|||	    will fill in the following fields in the Transition structure:
|||
|||	    t_View
|||	        Pointer to the View that owns this particular strip of the
|||	        display.  If NULL, no Views cover this span; it's blank.
|||
|||	    t_Height
|||	        The height of this strip of the display, in display (View
|||	        positioning) coordinates.
|||
|||	    By using the value received for starty and the t_Height fields,
|||	    the Projector can move through the Transition array assembling
|||	    strips for display.  The final entry in the array has a t_Height
|||	    value of zero (t_View is undefined for this entry).
|||
|||	  Side-Effects
|||
|||	    Calling this function will also alter the contents of the Views
|||	    attached to the supplied ViewList.  The following fields will be
|||	    affected:
|||
|||	    v_Ordinal
|||	        Set to the View's visible ordinal priority.  A given View
|||	        gets it's v_Ordinal set to n; the View immediately behind
|||	        it is set to n+1, and so on.
|||
|||	    v_LayerNode
|||	        This is used for "stacking" the Views atop one another
|||	        during compilation.  The MinNode structure is effectively
|||	        scrambled after this call completes.
|||
|||	    v_AbsTopEdge
|||	        Set to the View's "absolute" top edge.  This may be
|||	        different from the v_TopEdge field, depending on whether the
|||	        View's parent ViewLists were repositioned.
|||
|||	    v_VisTopEdge
|||	    v_VisBotEdge
|||	        These are effectively "scrambled," but it is done in a
|||	        deterministic way.  v_VisTopEdge is set to one line below
|||	        the bottommost edge of the View (v_AbsTopEdge + v_Height),
|||	        and v_VisTopEdge is set to v_AbsTopEdge.  The Projector may
|||	        reset these values as needed (in particular, it is
|||	        envisioned that the Projector will scan the returned
|||	        Transition array and update these fields as new limits are
|||	        discovered).
|||
|||	    v_PrevVisTopEdge
|||	    v_PrevVisBotEdge
|||	        Set to the value held by v_VisTopEdge and v_VisBotEdge,
|||	        respectively, before the modification described as above.
|||
|||	    v_ViewFlags
|||	        The bits VIEWF_VISIBLE and VIEWF_FULLYVISIBLE are cleared.
|||	        The Projector may re-set these as appropriate.
|||
|||	  Arguments
|||
|||	    vl
|||	        Pointer to the ViewList that is to be compiled.
|||
|||	    nviews
|||	        Count of the number of Views to be found in the ViewList's
|||	        heirarchy.
|||
|||	    tr_retn
|||	        Pointer to storage in which a pointer to the compiled
|||	        Transition array will be deposited.  The Projector is
|||	        responsible for freeing this array (using TRACKED_SIZE).
|||
|||	    tr_size
|||	        The size each element in the Transition array must have, in
|||	        bytes.
|||
|||	    starty
|||	        Pointer to storage in which the topmost Y coordinate of the
|||	        first Transition will be deposited.
|||
|||	    clearflags
|||	        Bitmask indicating which bits in the v_ViewFlags field of
|||	        all Views are to be cleared.  Bits set in this argument
|||	        indicate those that are to be cleared.
|||
|||	  Return Value
|||
|||	    This routine will return zero upon successful completion, or a
|||	    (negative) error code.  If an error is returned, the contents of
|||	    the storage pointed to by tr_retn and starty is undefined.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>, <graphics/view.h>
|||
|||	  See Also
|||
|||	    Projector(@), View(@), ViewList(@)
|||
**/
/*
 * Semi-general-purpose ViewList compiler service.
 * Takes a Projector's root ViewList and creates an array of Transitions
 * which may be used to generate hardware-specific sTUfF (like VDLs).
 * Transitions should be sufficiently abstract objects for most display
 * hardware.  This routine is intended to be called by Projector code
 * modules.  Really weird display hardware should write their own ViewList
 * compilers.
 */
Err
SuperInternalCompile (vl, nviews, tr_retn, tr_size, starty, clearflags)
struct ViewList		*vl;
int32			nviews;
struct Transition	**tr_retn;
int32			tr_size;
int32			*starty;
uint32			clearflags;
{
	TransEdge	*transedges;
	ViewEdge	*curedge;
	Transition	*curtrans;
	View		*ev, *level0;
	List		layerlist;
	int32		cury;
	int		count, i;

	/* ### CHECK THAT THE CALLER IS PRIVILEGED! */

	/*
	 * Allocate TransEdge array.
	 */
	if (!(transedges = SuperAllocMem (tr_size * (nviews + 1) * 2,
					  MEMTYPE_TRACKSIZE)))
		/*
		 * You know something really weird?  This is the only error
		 * this routine can generate.
		 */
		return (GFX_ERR_NOMEM);
	*tr_retn = &transedges->t;

	/*
	 * Number views from 0 to N, topmost layer to bottommost layer.
	 * Also compute absolute top edge of everything, and find out how
	 * many Views are *really* in the heirarchy.
	 */
	curedge = &transedges->e;
	count = ordinateviews (vl, &curedge, 0, 0, clearflags);
	count <<= 1;

	if (!count) {
		/*
		 * No Views; display is blank.
		 */
		transedges->t.t_Height = 0;
		return (0);
	}


	/*
	 * Sort table of view edges.
	 * This could consume significant CPU.
	 */
	sortedges (&transedges->e, count);
/*printf ("Sorted edges.\n");*/
/*curedge = &transedges->e;*/
/*for (i = 0;  i < count;  i++)*/
/* printf ("edge[%d].{v,type}:  0x%08lx, %d\n",*/
/* 	 i, curedge[i].e_View, curedge[i].e_Type);*/


	/*
	 * level0 is topmost layer.  level{n} is Nth layer down from level0.
	 * ev is the view of the current edge (Edge View).
	 * cury is the Y coordinate of the current (uncompleted) transition.
	 *
	 * As ViewEdges are scanned, Transitions are computed.  There are
	 * always more ViewEdges than Transitions; therefore, as
	 * Transitions are computed, ViewEdges will be overwritten (hence
	 * the TransEdge union (local 135)).
	 */
	InitList (&layerlist, NULL);
	level0 = NULL;
	curtrans = NULL;
	cury = 0;	/*  Make compiler shut up.  */

	for (i = count, curedge = &transedges->e;  --i >= 0;  curedge++) {
/*printf ("Edge: 0x%08lx, %d\n", curedge->e_View, curedge->e_Type);*/
		ev = curedge->e_View;
		if (curedge->e_Type == EDGETYPE_TOP) {
			/*
			 * Top edge of View.  This means that we must
			 * either begin a new transition, or simply priority-
			 * insert the new View into the layer list.
			 */
			if (!level0) {
				/*
				 * Make yogurt (thanks, Reichart).
				 */
/*printf ("Yogurt at 0x%08lx...", ev);*/
				if (curtrans) {
					/*
					 * Complete uncovered (blank)
					 * Transition and add this View to
					 * toplevel.
					 */
					goto completetrans;  /* Look down. */
				} else {
					/*
					 * True coldstart.  Set starty.
					 * Then add this View to toplevel.
					 */
					curtrans = &transedges->t;
					*starty = ev->v_AbsTopEdge;

					goto newtopview;  /*  Look down.  */
				}
			}

			if (ev->v_Ordinal < level0->v_Ordinal) {
				/*
				 * New View has higher priority.
				 * Complete this Transition and switch to
				 * new one.
				 */
/*printf ("Inserting new view at toplevel...");*/
completetrans:
				if (curtrans->t_Height =
				     ev->v_AbsTopEdge - cury)
					/*
					 * Advance to next Transition only if
					 * the previous one consumed some
					 * height.
					 */
					curtrans = ADVANCE
						    (curtrans, tr_size);
newtopview:
				curtrans->t_View = level0 = ev;
				cury = ev->v_AbsTopEdge;
				AddHead (&layerlist,
					 (Node *) &ev->v_LayerNode);
/*printf ("Done.\n");*/
			} else {
				/*
				 * Priority insert 'ev' into View layer
				 * list.  (Maybe convert this to
				 * UniversalInsertNode()?)
				 */
				register View	*dummy, *test;

/*printf ("Priority-inserting view...");*/
				for (dummy = (View *) FIRSTNODE (&layerlist);
				     NEXTNODE (dummy);
				     dummy = (View *) NEXTNODE (dummy))
				{
					test = (View *)
						((uint32) dummy -
					 	 offsetof (View, v_LayerNode));

					if (test->v_Ordinal >= ev->v_Ordinal)
						break;
				}
				InsertNodeBefore ((Node *) dummy,
						  (Node *) &ev->v_LayerNode);
/*printf ("Done.\n");*/
			}
		} else {
			/*
			 * Bottom edge of View.  This means that we must
			 * end the current transition and begin a new one
			 * (if the expiring View is the top one), or
			 * simply remove it from the layer list.
			 * Either way, it leaves the list.
			 */
/*printf ("Expiring 0x%08lx...", ev);*/
			RemNode ((Node *) &ev->v_LayerNode);
			if (ev == level0) {
				/*
				 * Top view has expired.  Descend to
				 * whatever's beneath it.
				 */
				int32	ybot;
/*printf ("(toplevel)");*/

				ybot = level0->v_AbsTopEdge +
				       level0->v_Height;

				if (curtrans->t_Height = ybot - cury)
					/*
					 * Advance to next transition only
					 * if the previous one consumed some
					 * height.
					 */
					curtrans =
					 ADVANCE (curtrans, tr_size);

				cury = ybot;

				ev = (View *) FIRSTNODE (&layerlist);
				if (NEXTNODE (ev)) {
					/*
					 * Convert layerlist node into
					 * View.  Reset level0.
					 */
					ev = (View *)
					     ((uint32) ev -
					      offsetof (View, v_LayerNode));

					curtrans->t_View = level0 = ev;
				} else {
					/*
					 * Popped all Views off layer list.
					 * This happens either at the end of
					 * the edge list, or within a region
					 * covered by no Views.  Set t_Height
					 * to zero in case this is the last
					 * Transition.
					 */
					curtrans->t_Height = 0;
					curtrans->t_View = level0 = ev = NULL;
					TOUCH (ev);	/*  Sheesh  */
				}
/*printf ("Done\n");*/
			}
		}
	}

	return (0);
}




int32
ordinateviews (vl, edges, ordinal, yref, clearflags)
struct ViewList	*vl;
struct ViewEdge	**edges;
int32		ordinal, yref;
uint32		clearflags;
{
	register View		*n;
	register ViewEdge	*e;

	yref += vl->vl_TopEdge;

	for (n = (View *) FIRSTNODE (&vl->vl_SubViews);
	     NEXTNODE (n);
	     n = (View *) NEXTNODE (n))
	{
		if (n->v.n_Type == GFX_VIEWLIST_NODE)
			/*
			 * Recursively descend into ViewList.
			 */
			ordinal = ordinateviews ((ViewList *) n,
						 edges,
						 ordinal,
						 yref,
						 clearflags);
		else if (n->v_Height > 0) {
			/*
			 * Add this View to edge list.
			 * Only views having positive non-zero height are
			 * included in the edge list.
			 */
			e = *edges;

			e->e_View	= n;
			(e++)->e_Type	= EDGETYPE_TOP;

			e->e_View	= n;
			(e++)->e_Type	= EDGETYPE_BOTTOM;

			*edges = e;

			/*
			 * Set edge values (bucketing over old ones).
			 * VisTop and VisBot are set "backwards" to
			 * establish "practical maxima" (rather than by
			 * faking it with MAXINT and the like).  Apologies
			 * for the cascading assignment.
			 */
			n->v_PrevVisTopEdge = n->v_VisTopEdge;
			n->v_PrevVisBotEdge = n->v_VisBotEdge;

			n->v_VisTopEdge =
			 (n->v_VisBotEdge =
			  n->v_AbsTopEdge = yref + n->v_TopEdge) +
			 n->v_Height;

			n->v_Ordinal = ordinal++;
			n->v_ViewFlags &= ~(VIEWF_VISIBLE |
					    VIEWF_FULLYVISIBLE |
					    clearflags);
		}
	}
	return (ordinal);
}


/**
|||	AUTODOC -private -class graphicsfolio -name SuperInternalMarkView
|||	Marks a View as (possibly fully) visible given the supplied bounds.
|||
|||	  Synopsis
|||
|||	    Err SuperInternalMarkView (struct View      *v,
|||	                               int32            top,
|||	                               int32            bot);
|||
|||	  Description
|||
|||	    This is a cheap support routine for Projectors.  Often times,
|||	    for View signal scheduling, it is convenient to know the upper-
|||	    and bottom-most visible lines of a View, and whether or not it's
|||	    visible.
|||
|||	    This routine will set the v_VisTopEdge and v_VisBotEdge fields
|||	    of the View structure to the new supplied limits.  If
|||	    v_VisTopEdge is greater than 'top', it receives the new value.
|||	    If v_VisBotEdge is less than 'bot', it receives the new value.
|||
|||	    The VIEWF_VISIBLE flag is set in the v_ViewFlags field.  If
|||	    'top' and 'bot' describe the entire height of the View,
|||	    VIEWF_FULLYVISIBLE is also set.
|||
|||	    A Projector would typically call this function while iterating
|||	    through the Transition array returned by SuperInternalCompile().
|||
|||	  Example
|||
|||	    // This example is grossly simplified, but should give you an
|||	    // idea of how this function gets used.  'myviewlist' and
|||	    // 'numviews' are presumed to be already initialized.  Error
|||	    // checking is not present for clarity.
|||
|||	    ViewList    myviewlist;
|||	    Transition  *trans;
|||	    int32       starty;
|||	    int32       numviews;
|||	    int32       transtop, transbot;
|||
|||	    SuperInternalCompile (&myviewlist,
|||	                          numviews,
|||	                          &trans,
|||	                          sizeof (struct Transition),
|||	                          &starty,
|||	                          0);
|||
|||	    transtop = starty;
|||	    while (trans->t_Height) {
|||	            transbot = transtop + trans->t_Height;
|||	            if (trans->t_Owner) {
|||	                    SuperInternalMarkView (trans->t_Owner,
|||	                                           transtop,
|||	                                           transbot);
|||	            }
|||
|||	            // Lots of other stuff to build the display...
|||
|||	            transtop = transbot;
|||	    }
|||
|||	  Arguments
|||
|||	    v
|||	        Pointer to the View to be tested/updated.
|||
|||	    top
|||	        New top visibility value to be tested/set.
|||
|||	    bot
|||	        New bottom visibility value to be tested/set.
|||
|||	  Return Value
|||
|||	    Zero upon success, or a negative error code.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>, <graphics/view.h>
|||
|||	  See Also
|||
|||	    Projector(@), View(@)
|||
**/
Err
SuperInternalMarkView (v, top, bot)
struct View	*v;
int32		top, bot;
{
	if (top < v->v_VisTopEdge)
		v->v_VisTopEdge = top;

	if (bot > v->v_VisBotEdge)
		v->v_VisBotEdge = bot;

	v->v_ViewFlags |= VIEWF_VISIBLE;
	if (bot - top == v->v_Height)
		v->v_ViewFlags |= VIEWF_FULLYVISIBLE;

	return (0);
}


/***************************************************************************
 * Housekeeping and miscellaneous.
 */
static int
viewedgecmp (e1, e2)
struct ViewEdge	*e1, *e2;
{
	int32	edge1, edge2;

	edge1 = e1->e_View->v_AbsTopEdge;
	if (e1->e_Type)
		edge1 += e1->e_View->v_Height;

	edge2 = e2->e_View->v_AbsTopEdge;
	if (e2->e_Type)
		edge2 += e2->e_View->v_Height;

	if ((edge1 -= edge2) == 0)
		return (e2->e_Type - e1->e_Type);  /* Bottoms before tops. */
	else
		return (edge1);
}

/*
 * I'd like to change this to an insertion sort someday; qsort() is overkill.
 */
static void
sortedges (edges, nentries)
struct ViewEdge	*edges;
int32		nentries;
{
	qsort ((char *) edges,
	       nentries,
	       sizeof (ViewEdge),
	       (int (*)(const void *, const void *)) viewedgecmp);
}
