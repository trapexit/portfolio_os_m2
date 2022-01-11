/*  :ts=8 bk=0
 *
 * view.c:	Code to manage View Items.
 *
 * @(#) view.c 96/09/20 1.32
 *
 * Leo L. Schwab					9504.11
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/kernel.h>

#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>

#include <string.h>

#include "protos.h"


/***************************************************************************
 * #defines
 */
#define	CVF_RECOMPILE		1
#define	CVF_UPDATEDIMS		(1<<1)
#define	CVF_UPDATEPIXDIMS	(1<<2)
#define	CVF_UPDATEBASEADDR	(1<<3)
#define	CVF_UPDATEVIEWTYPE	(1<<4)
#define	CVF_CHKDIMS		(1<<5)
#define	CVF_CHKBITMAPVIEW	(1<<6)
#define	CVF_CHKDISPLAYABLE	(1<<7)
#define	CVF_BESILENT		(1<<8)


/***************************************************************************
 * Prototypes.
 */
static Err typescompatible (ViewTypeInfo *vti, int32 bmtype);


/***************************************************************************
 * Globals
 */
static View	view_default = {
	{ NULL },	/*  v			*/
	0,		/*  v_LeftEdge		*/
	0,		/*  v_TopEdge		*/
	-1,		/*  v_Width		*/
	-1,		/*  v_Height		*/
	-1,		/*  v_PixWidth		*/
	-1,		/*  v_PixHeight		*/
	0,		/*  v_WinLeft		*/
	0,		/*  v_WinTop		*/
	-1,		/*  v_Type		*/
	0,		/*  v_ViewFlags		*/
	NULL,		/*  *v_Bitmap		*/
	0,		/*  v_BitmapItem	*/
	NULL,		/*  *v_ViewTypeInfo	*/
	NULL,		/*  *v_Projector	*/
	0,		/*  v_Ordinal		*/
	{ NULL },	/*  v_LayerNode		*/
	{ { NULL },
	  INTRTYPE_RENDINTR,
	  0 },		/*  v_RendLineIntr	*/
	{ { NULL },
	  INTRTYPE_DISPINTR,
	  0 },		/*  v_DispLineIntr	*/
	0,		/*  v_RendSig		*/
	0,		/*  v_DispSig		*/
	NULL,		/*  *v_SigTask		*/
	NULL,		/*  *v_RendCBFunc	*/
	NULL,		/*  *v_RendCBData	*/
	NULL,		/*  *v_DispCBFunc	*/
	NULL,		/*  *v_DispCBData	*/
	0,		/*  v_AbsTopEdge	*/
	0,		/*  v_VisTopEdge	*/
	0,		/*  v_VisBotEdge	*/
	0,		/*  v_PrevVisTopEdge	*/
	0,		/*  v_PrevVisBotEdge	*/
	0,		/*  v_AvgMode0		*/
	0,		/*  v_AvgMode1		*/
	0,		/*  v_VLine		*/
	NULL,		/*  v_BaseAddr		*/
        VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT,             /*  v_DispCtl0  */
        VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT,             /*  v_DispCtl1  */
        VDL_AV | VDL_AV_FIELD (VDL_AV_HSTART, 0) |	/*  v_AVCtl     */
                 VDL_AV_FIELD (VDL_AV_HWIDTH, 320) |
                 VDL_AV_HDOUBLE |
                 VDL_AV_LD_HSTART | VDL_AV_LD_HWIDTH | VDL_AV_LD_HDOUBLE,
        VDL_LC | VDL_LC_LD_FBFORMAT | VDL_LC_FBFORMAT_16 |
                 VDL_LC_ONEVINTDIS                      /*  v_ListCtl   */
};



/***************************************************************************
 * Autodoc for View Item.
 */
/**
|||	AUTODOC -public -class Items -name View
|||	An Item describing a displayed region of imagery.
|||
|||	  Description
|||
|||	    This Item represents an image displayed on the physical monitor.
|||	    It contains information describing its position, colors, depth
|||	    arrangement, display modes, and other characteristics.
|||
|||	  Tag Arguments
|||
|||	    VIEWTAG_VIEWTYPE (int32)
|||	        Specifies the type of View to be constructed.  See "View
|||	        Types" below for a list of currently available types.
|||
|||	    VIEWTAG_WIDTH (uint32)
|||	        Specifies the width of the View, in display coordinates.
|||
|||	    VIEWTAG_HEIGHT (uint32)
|||	        Specifies the height of the View, in display coordinates.
|||
|||	    VIEWTAG_TOPEDGE (uint32)
|||	        Specifies where the top edge of the View is to be located on
|||	        the display, in display coordinates.
|||
|||	    VIEWTAG_LEFTEDGE (uint32)
|||	        Specifies where the left edge of the View is to be located
|||	        on the display, in display coordinates.
|||
|||	    VIEWTAG_WINTOPEDGE (uint32)
|||	        Specifies the topmost pixel from the Bitmap which is to be
|||	        the first pixel displayed at the top edge of the View, in
|||	        pixels.
|||
|||	    VIEWTAG_WINLEFTEDGE (uint32)
|||	        Specifies the leftmost pixel from the Bitmap which is to be
|||	        the first pixel displayed at the left edge of the View, in
|||	        pixels.
|||
|||	    VIEWTAG_BITMAP (Item)
|||	        The Item number of the Bitmap to be displayed through this
|||	        View.
|||
|||	    VIEWTAG_PIXELWIDTH (uint32)
|||	        Specifies the width of the View, in pixels of the underlying
|||	        Bitmap.
|||
|||	    VIEWTAG_PIXELHEIGHT (uint32)
|||	        Specifies the height of the View, in pixels of the
|||	        underlying Bitmap.
|||
|||	    VIEWTAG_AVGMODE (int32)
|||	        Specifies whether Opera-style horizontal and/or vertical
|||	        averaging is to be enabled within this View.  See
|||	        "Pixel Averaging Modes" below for a list of available modes.
|||
|||	    VIEWTAG_RENDERSIGNAL (uint32)
|||	        A bitmask specifying the signals to be asserted when the
|||	        Bitmap underlying the View may be safely rendered into.
|||	        If zero, no signal is to be asserted.
|||
|||	    VIEWTAG_DISPLAYSIGNAL (uint32)
|||	        A bitmask specifying the signals to be asserted when the
|||	        Bitmap underlying the View has been seen by the user at
|||	        least once.  If zero, no signal is to be asserted.
|||
|||	    VIEWTAG_SIGNALTASK (Item)
|||	        The Item number of the task to be signalled when the Bitmap
|||	        may be safely rendered, and when the Bitmap has been seen by
|||	        the user.  If NULL (which is the default), the owner of the
|||	        Item is signalled.
|||
|||	    VIEWTAG_FIELDSTALL_VIEWLINE (int32)
|||	        Causes the operation to block until the video hardware is
|||	        prepared to scan the specified line of the View, expressed
|||	        in display coordinates.  See "Video Field-Sensitive
|||	        Operations" below for more details.
|||
|||	    VIEWTAG_FIELDSTALL_BITMAPLINE (int32)
|||	        Causes the operation to block until the video hardware is
|||	        prepared to scan the specified row of pixels in the View's
|||	        underlying Bitmap.  See "Video Field-Sensitive Operations"
|||	        below for more details.
|||
|||	    VIEWTAG_BESILENT (Boolean)
|||	        Signals are sent for all modifications to a View, including
|||	        its creation.  This can be inconvenient at times.  If this
|||	        Tag is present and its argument is true (non-zero), no
|||	        notification signal will be asserted for the modification in
|||	        which this Tag was present.  This feature applies only to
|||	        specific modifications, not the Item itself.  Modifications
|||	        not specifying this Tag will continue to generate signals.
|||
|||	  Video Field-Sensitive Operations
|||
|||	    Ordinarily, when using VIEWTAG_BITMAP to double-buffer, the new
|||	    imagery will be presented at the next available video field,
|||	    whatever it may be.  Some applications, however, wish to
|||	    page-flip on a particular video field.  The tags
|||	    VIEWTAG_FIELDSTALL_VIEWLINE and VIEWTAG_FIELDSTALL_BITMAPLINE
|||	    provide that support by delaying the changes requested via
|||	    ModifyGraphicsItem() until they are guaranteed to become visible
|||	    on the desired video field.
|||
|||	    Because not all video hardware scans out complete frames in the
|||	    same way, the method by which you specify the desired field is
|||	    done in either a View-oriented or a Bitmap-oriented way.  Rather
|||	    than specifying a hardware-dependent field, you specify either a
|||	    View-relative or Bitmap-relative scanline number.  The idea is
|||	    to specify the scanline number you wish to first appear when the
|||	    changes are finally applied.  This approach has the advantage of
|||	    being independent of the View position.
|||
|||	    The distinction between these two methods is best clarified by
|||	    example.  Imagine a View displaying an full-screen 480-tall
|||	    image on an interlaced Projector.  In this case, a View-relative
|||	    and Bitmap-relative operation would be identical, since the
|||	    hardware is scanning a different set of lines on alternate
|||	    fields.
|||
|||	    Now imagine a View displaying a full-screen 240-tall (low-res)
|||	    image.  In this case, a View-relative operation would yield the
|||	    same results as before, since different lines of the View are
|||	    scanned out on alternate fields.  Recall that View positions and
|||	    dimensions are expressed in display coordinates, which is the
|||	    finest available resolution on the Projector.  Though the pixel
|||	    dimensions are different, the View dimensions are unchanged.
|||	    However, for this case, a Bitmap-relative operation would be an
|||	    expensive no-op, since the same lines of pixels are scanned out
|||	    on each field (the hardware is effectively performing
|||	    "pixel-doubling").
|||
|||	    View-relative operations are specified with the tag
|||	    VIEWTAG_FIELDSTALL_VIEWLINE; the scanline argument is relative
|||	    to the top of the View, expressed in display coordinates.
|||	    Bitmap-relative operations are specified with the tag
|||	    VIEWTAG_FIELDSTALL_BITMAPLINE; the argument is the Y-coordinate
|||	    of the View's underlying Bitmap, expressed in pixels.
|||	    Bitmap-relative operations also take into account the current
|||	    setting of the v_WinTop field in the View.
|||
|||	    Note that this may cause ModifyGraphicsItem() to block for as
|||	    much as one video field until the changes can be made.  Note
|||	    further that the changes are applied (and the call returns) at
|||	    the earliest possible time; you should not assume a particular
|||	    field of your View is visible until you receive the
|||	    DisplaySignal.
|||
|||	    This does not alter the scheduling behavior of View signals;
|||	    they will still be dispatched at the earliest possible time
|||	    (that is, the signals are not sent at the line numbers you
|||	    request).
|||
|||	    Naturally, all this applies to Views on interlaced Projectors.
|||	    For non-interlaced Projectors, use of these tags is an expensive
|||	    no-op.  You can determine the kind of Projector through which
|||	    your View is displayed by inspecting the p_FieldsPerFrame field
|||	    in the Projector Item structure.
|||
|||	  View Types
|||
|||	    The following is an incomplete list of currently available View
|||	    types.  Note that this scheme of View typing/naming is under
|||	    review, and is going to change.  Watch for it.  (See the include
|||	    file <graphics/view.h> for a complete list of available View
|||	    types.)
|||
|||	    VIEWTYPE_16
|||	        View is 16 bits per pixel, 320 * 240 nominal resolution.
|||
|||	    VIEWTYPE_16_LACE
|||	        View is 16 bits per pixel, 320 * 480 nominal resolution.
|||
|||	    VIEWTYPE_16_640
|||	        View is 16 bits per pixel, 640 * 240 nominal resolution.
|||
|||	    VIEWTYPE_16_640_LACE
|||	        View is 16 bits per pixel, 640 * 480 nominal resolution.
|||
|||	    VIEWTYPE_32
|||	        View is 32 bits per pixel, 320 * 240 nominal resolution.
|||
|||	    VIEWTYPE_32_LACE
|||	        View is 32 bits per pixel, 320 * 480 nominal resolution.
|||
|||	    VIEWTYPE_32_640
|||	        View is 32 bits per pixel, 640 * 240 nominal resolution.
|||
|||	    VIEWTYPE_32_640_LACE
|||	        View is 32 bits per pixel, 640 * 480 nominal resolution.
|||
|||	  Pixel Averaging Modes
|||
|||	    The following averaging modes are available.  The AVGMODEs are
|||	    bitfields, and may be set independently of each other:
|||
|||	    AVGMODE_H
|||	        Horizontal interpolation enabled.
|||
|||	    AVGMODE_V
|||	        Vertical interpolation enabled.
|||
|||	    AVG_DSB_0
|||	    AVG_DSB_1
|||	        Constant values to OR with the above AVGMODE values.  If
|||	        AVG_DSB_0 is OR'd, the specified averaging mode is applied
|||	        to pixels whose DSB bit is zero.  Similarly, OR'ing
|||	        AVG_DSB_1 will apply the specified averaging mode to pixels
|||	        whose DSB bit is set to one.
|||
|||	  Caveats
|||
|||	    Bitmap types and View types cannot be freely intermixed.  That
|||	    is, for a Bitmap to be displayed, it must be handed to a View of
|||	    a compatible type.  You can't, for example, display a BMTYPE_16
|||	    through a VIEWTYPE_32.  However, handing a BMTYPE_16 to a
|||	    VIEWTYPE_16_LACE is perfectly cool.  (This mechanic may change.)
|||
|||	    The VIEWTYPE_ names are all going to change to something more
|||	    reasonable.  Watch for it.  (No, really, they will.  Seriously.
|||	    I'm not kidding...)
|||
|||	    VIEWTAG_{WIDTH,HEIGHT} and VIEWTAG_PIXEL{WIDTH,HEIGHT} are
|||	    mutually exclusive.  That is, setting _PIXELWIDTH will undo any
|||	    previous setting to _WIDTH, and vice-versa.
|||
|||	    Opera-style averaging may be unavailable for certain View types.
|||	    No error will be reported; it just won't do anything.
|||
|||	    Setting signalling on a View to signal the Item's owner (the
|||	    default) and then changing the owner of the View Item is
|||	    currently undefined.  Don' do dat.
|||
|||	    The nominal pixel resolutions given for the above View types are
|||	    for NTSC mode.  PAL dimensions are different.  Inspect the
|||	    ViewTypeInfo structure pointed to by the View's v_ViewTypeInfo
|||	    field to discover the View type's full pixel dimensions.
|||
|||	  Folio
|||
|||	    graphics
|||
|||	  Item Type
|||
|||	    GFX_VIEW_NODE
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
|||	    CreateItem(), Bitmap(@), ViewList(@)
|||
**/


/***************************************************************************
 * Item management.
 */
static Err
icv
(
struct View	*v,
uint32		*flags,
uint32		t,
uint32		a
)
{
	register int32	f;

/*printf ("Tag, arg: %d, 0x%08lx\n", t, a);*/
	f = *flags;
	switch (t) {
	case VIEWTAG_VIEWTYPE:
		v->v_Type = a;
		f |= CVF_RECOMPILE | CVF_CHKBITMAPVIEW | CVF_CHKDIMS |
		     CVF_UPDATEVIEWTYPE;
		break;
	case VIEWTAG_WIDTH:
		v->v_Width = a;
		f |= CVF_RECOMPILE | CVF_UPDATEPIXDIMS | CVF_CHKDIMS;
		f &= ~CVF_UPDATEDIMS;
		break;
	case VIEWTAG_HEIGHT:
		v->v_Height = a;
		f |= CVF_RECOMPILE | CVF_UPDATEPIXDIMS | CVF_CHKDIMS;
		f &= ~CVF_UPDATEDIMS;
		break;
	case VIEWTAG_PIXELWIDTH:
		v->v_PixWidth = a;
		f |= CVF_RECOMPILE | CVF_UPDATEDIMS | CVF_CHKDIMS;
		f &= ~CVF_UPDATEPIXDIMS;
		break;
	case VIEWTAG_PIXELHEIGHT:
		v->v_PixHeight = a;
		f |= CVF_RECOMPILE | CVF_UPDATEDIMS | CVF_CHKDIMS;
		f &= ~CVF_UPDATEPIXDIMS;
		break;
	case VIEWTAG_TOPEDGE:
		v->v_TopEdge = a;
		f |= CVF_RECOMPILE;
		break;
	case VIEWTAG_LEFTEDGE:
		v->v_LeftEdge = a;
		f |= CVF_RECOMPILE;
		break;
	case VIEWTAG_WINTOPEDGE:
		v->v_WinTop = a;
		f |= CVF_CHKDIMS | CVF_UPDATEBASEADDR;
		break;
	case VIEWTAG_WINLEFTEDGE:
		v->v_WinLeft = a;
		f |= CVF_CHKDIMS | CVF_UPDATEBASEADDR;
		break;
	case VIEWTAG_BITMAP:
		if (v->v_Bitmap =
		     CheckItem ((Item) a, NST_GRAPHICS, GFX_BITMAP_NODE))
			v->v_BitmapItem = a;
		else {
			if (a)
				return (GFX_ERR_BADBITMAP);
			v->v_BitmapItem = 0;
		}
		f |= CVF_CHKDIMS | CVF_CHKBITMAPVIEW | CVF_CHKDISPLAYABLE |
		     CVF_UPDATEBASEADDR;
		break;
	case VIEWTAG_RENDERSIGNAL:
		v->v_RendSig = a;
		break;
	case VIEWTAG_DISPLAYSIGNAL:
		v->v_DispSig = a;
		break;
	case VIEWTAG_SIGNALTASK:
		if (!(v->v_SigTask = CheckItem
				      ((Item) a, NST_KERNEL, TASKNODE)))
			return (GFX_ERR_BADITEM);
		break;
	case VIEWTAG_BESILENT:
		if (a)
			f |= CVF_BESILENT;
		/*  Fallthrough  */
	case VIEWTAG_FIELDSTALL_BITMAPLINE:
	case VIEWTAG_FIELDSTALL_VIEWLINE:
		/*
		 * There's nothing the folio can do with these other than
		 * pass it on to the Projector.
		 */
		break;
	case VIEWTAG_AVGMODE:
		if (a & AVG_DSB_1)
			v->v_AvgMode1 = a & AVGMODE_MASK;
		else
			v->v_AvgMode0 = a & AVGMODE_MASK;
		break;
	default:
		DDEBUG (("Unknown tag 0x%lx\n", t));
		return (GFX_ERR_BADTAGARG);
		break;
	}
	*flags = f;
	return (0);
}

Item
createview (
struct View	*v,
struct TagArg	*ta
)
{
	Item	retval;

	memcpy (((ItemNode *) v) + 1,
		((ItemNode *) &view_default) + 1,
		sizeof (View) - sizeof (ItemNode));

	if ((retval = (Item) modifyview (v, ta)) >= 0)
		v->v_Projector->p_ViewCount++;

	return (retval);
}


Err
modifyview (cview, ta)
struct View	*cview;
struct TagArg	*ta;
{
	ModifyInfo	mi;
	View		v;
	Err		err;
	Bitmap		*bm;
	uint32		flags, oldints;

	/*
	 * Copy client's view to working buffer.
	 */
	memcpy (&v, cview, sizeof (View));
	flags = 0;

/*printf ("Scanning View Tags...\n");*/
	if ((err = TagProcessor (&v, ta, icv, &flags)) < 0) {
/*printf ("Ack!  TagProcessor returned 0x%08lx\n", err);*/
		return (err);
	}

	bm = v.v_Bitmap;

	if (v.v_Type < 0) {
		/*
		 * Assign default view type.
		 */
		if (bm)
			v.v_Type = bm->bm_TypeInfo->bti_DefaultViewType;
		else
			v.v_Type = VIEWTYPE_BLANK;
		flags |= CVF_RECOMPILE | CVF_CHKBITMAPVIEW | CVF_CHKDIMS |
			 CVF_UPDATEVIEWTYPE;
	}
	/*
	 * Assign default dimensions.
	 */
	if (v.v_Width < 0  &&  v.v_PixWidth < 0) {
/*
 * There's a compiler bug in here somewhere.  This printf() defeats it.
 */
/*printf ("bm(w) = 0x%08lx\n", bm);*/
		v.v_PixWidth = bm  ?  bm->bm_Width  :  0;
		flags |= CVF_UPDATEDIMS;
	}
	if (v.v_Height < 0  &&  v.v_PixHeight < 0) {
/*printf ("bm(h) = 0x%08lx\n", bm);*/
		v.v_PixHeight = bm  ?  bm->bm_Height  :  0;
		flags |= CVF_UPDATEDIMS;
	}

	if (!v.v_SigTask) {
		/* ### Amend SetItemOwner vector appropriately.  */
		if (!v.v.n_Owner)
			/*
			 * Ack!  CreateItem doesn't set n_Owner until after
			 * our folio-specific stuff is finished.  Use
			 * CURRENTTASK.
			 */
			v.v_SigTask = CURRENTTASK;
		else
			v.v_SigTask = LookupItem (v.v.n_Owner);
	}

	if (flags & CVF_UPDATEVIEWTYPE) {
		if (!(v.v_Type >> PROJTYPE_SHIFT))
			v.v_Type |= GBASE(gb_DefaultProjector)->p_Type <<
				    PROJTYPE_SHIFT;

		if ((err = lookupviewtype (v.v_Type,
					   &v.v_ViewTypeInfo,
					   &v.v_Projector)) < 0)
			return (err);
	}

	if ((flags & CVF_CHKDISPLAYABLE)  &&  bm)
		if (!(bm->bm_Flags & BMF_DISPLAYABLE))
			return (GFX_ERR_BADBITMAP);

	if ((flags & CVF_CHKBITMAPVIEW)  &&  bm) {
		if ((err = typescompatible (v.v_ViewTypeInfo, bm->bm_Type)))
			return (err);
	}
	if (flags & CVF_UPDATEPIXDIMS) {
		register ViewTypeInfo	*vti;

		vti = v.v_ViewTypeInfo;
		v.v_PixWidth =
		 (v.v_Width + vti->vti_PixXSize - 1) / vti->vti_PixXSize;
		v.v_PixHeight =
		 (v.v_Height + vti->vti_PixYSize - 1) / vti->vti_PixYSize;
	}
	if (flags & CVF_UPDATEDIMS) {
		register ViewTypeInfo	*vti;

		vti = v.v_ViewTypeInfo;
		v.v_Width = v.v_PixWidth * vti->vti_PixXSize;
		v.v_Height = v.v_PixHeight * vti->vti_PixYSize;
	}
	if ((flags & CVF_CHKDIMS)  &&  bm) {
		if (v.v_PixWidth + v.v_WinLeft > bm->bm_Width  ||
		    v.v_PixHeight + v.v_WinTop > bm->bm_Height) {
/*printf ("bm{wide,high} %d, %d, ", bm->bm_Width, bm->bm_Height);*/
/*printf ("v{wide,high} %d, %d\n", v.v_PixWidth, v.v_PixHeight);*/
			return (GFX_ERR_BADDIMS);
		}
	}

	if (flags & CVF_UPDATEBASEADDR)
		v.v_BaseAddr =
		 bm  ?
		 InternalPixelAddr (bm, v.v_WinLeft, v.v_WinTop)  :
		 NULL;

	/*
	 * Swap the contents of the workspace View with that of the
	 * client View.  Then attempt to have the Projector update it.
	 * If it fails, we restore the contents of the client View
	 * and return the error.
	 *
	 * I would have done a bizarre ViewList hack, but the
	 * compiler needs to know the true address of the View so
	 * my way-cool optimizations will work.
	 */

	/*
	 * Switch the vessel with the pestle for the flagon with the dragon;
	 * 'v' will contain the original unaltered View.
	 * Copy back the View signal nodes, as they may have been processed
	 * since we copied the client View.
	 * (This operation must be protected, as interrupts may alter nodes
	 * while we're relocating them.)
	 */
	oldints = Disable ();
	memswap (cview, &v, sizeof (View));
	cview->v_RendLineIntr = v.v_RendLineIntr;
	cview->v_DispLineIntr = v.v_DispLineIntr;
	Enable (oldints);

	/*
	 * How does it look over *here*?
	 */
/*printf ("--Diving into projector\n");*/
	mi.mi_View = cview;
	mi.mi_OldView = &v;
	mi.mi_TagArgs = ta;
	err = (cview->v_Projector->p_Update) (cview->v_Projector,
					      PROJOP_MODIFY,
					      &mi);

	if (err < 0) {
		/*
		 * Nerts.  Restore the View and return the error.
		 */
		memcpy (cview, &v, sizeof (View));
		return (err);
	}

/*printf ("modify returning\n");*/
	return (cview->v.n_Item);
}


Err
deleteview (v)
struct View	*v;
{
	uint32	oldints;

	v->v_Projector->p_ViewCount--;
	v->v_RendSig =		/*  Prevent stacking to signal lists as	*/
	v->v_DispSig = 0;	/*  a result of removing the View.	*/

	removeview (v);

	/*
	 * Technically speaking, the folio really shouldn't know how the
	 * Projector is implementing View signals; the Projector should be
	 * looking after all that stuff itself.  Oh well, maybe next time...
	 */
	oldints = Disable ();
	if (v->v_RendLineIntr.li_Node.n_Next)
		RemNode ((Node *) &v->v_RendLineIntr);
	if (v->v_DispLineIntr.li_Node.n_Next)
		RemNode ((Node *) &v->v_DispLineIntr);
	Enable (oldints);

	return (0);
}


Item
findview (
struct TagArg	*ta
)
{
	TOUCH (ta);

	return (GFX_ERR_NOTSUPPORTED);
}


Item
openview (
struct View	*v,
struct TagArg	*ta
)
{
	TOUCH (ta);

	return (v->v.n_Item);
}


Err
closeview (
struct View	*v,
struct Task	*t
)
{
	TOUCH (v);
	TOUCH (t);

	return (0);
}


/***************************************************************************
 * Other client routines.
 */
/**
|||	AUTODOC -public -class graphicsfolio -name AddViewToViewList
|||	Add a View to a list of Views.
|||
|||	  Synopsis
|||
|||	    Err AddViewToViewList (Item viewItem, Item viewListItem)
|||
|||	  Description
|||
|||	    This routine attaches a View or ViewList Item to a ViewList
|||	    or Projector Item.  The new View/ViewList is placed in front of
|||	    all other Views currently within the target ViewList/Projector.
|||
|||	    The argument viewItem may be either a View Item or a ViewList
|||	    Item.
|||
|||	    The argument viewListItem can be one of three things:  A
|||	    ViewList Item, a Projector Item, or zero:
|||
|||	    ViewList Item
|||	        viewItem is placed in front of all other Views currently
|||	        within the target ViewList.
|||
|||	    Projector Item
|||	        viewItem is placed at the top level of the Projector's view
|||	        heirarchy.
|||
|||	    Zero (0)
|||	        viewItem is added to the top level of the default Projector
|||	        (which is typically the active display).
|||
|||	    In all cases, viewItem is added as the frontmost View/ViewList
|||	    in the target ViewList/Projector.  viewListItem need not be
|||	    currently attached to either a parent ViewList or Projector (so
|||	    you can assemble your view heirarchy before posting it to the
|||	    display).
|||
|||	    If the target ViewList is in a currently visible heirarchy, or
|||	    the target Projector is currently active, then viewItem becomes
|||	    visible on the physical display when this call returns.  If
|||	    viewItem is a ViewList, all subordinate Views and ViewLists
|||	    become visible on the display.
|||
|||	  Arguments
|||
|||	    viewItem
|||	        The Item number of the View or ViewList to be added.
|||
|||	    viewListItem
|||	        The Item number of the ViewList or Projector to which
|||	        viewItem is to be attached.  If zero, viewItem is added to
|||	        the default Projector.
|||
|||	  Return Value
|||
|||	    This routine will return zero upon successful completion, or a
|||	    (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/view.h>
|||
|||	  See Also
|||
|||	    RemoveView(), OrderViews(), Projector(@), View(@), ViewList(@)
|||
**/
Err
SuperAddViewToViewList (viewItem, viewListItem)
Item	viewItem, viewListItem;
{
	Projector	*p;
	ViewList	*vl;
	View		*v;
	Node		*n;	/*  View or ViewList	*/
	int		recompile;

	recompile = FALSE;

	/*
	 * Validate viewListItem.
	 * ### ownership checks
	 */
	if (!viewListItem) {
		vl = &GBASE(gb_DefaultProjector)->p_ViewList;
		recompile = TRUE;
	} else {
		Node	*foo;

		if (!(foo = LookupItem (viewListItem))  ||
		    foo->n_SubsysType != NST_GRAPHICS)
			return (GFX_ERR_BADITEM);

		if (foo->n_Type == GFX_VIEWLIST_NODE) {
			vl = (ViewList *) foo;
			if (vl->vl.n_Next)
				recompile = TRUE;
		} else if (foo->n_Type == GFX_PROJECTOR_NODE) {
			vl = &((Projector *) foo)->p_ViewList;
			recompile = TRUE;
		} else
			return (GFX_ERR_BADITEM);
	}

	/*
	 * Validate viewItem.
	 */
	if (!(n = LookupItem (viewItem))  ||
	    n->n_SubsysType != NST_GRAPHICS)
		return (GFX_ERR_BADITEM);

	/*
	 * Check for compatibility, and assign View/Projector types.
	 */
	if (n->n_Type == GFX_VIEW_NODE) {
		/*
		 * View being added to ViewList.
		 * If ViewList has a type, check it matches the View.
		 * If not, assign ViewList type of View.
		 */
		v = (View *) n;
		if (vl->vl_Projector) {
			if (v->v_Projector != vl->vl_Projector)
				return (GFX_ERR_VIEWINCOMPAT);
		} else
			/*
			 * Parent ViewList is typeless; assign it type of
			 * View to be attached.
			 */
			vl->vl_Projector = v->v_Projector;
		p = v->v_Projector;

	} else if (n->n_Type == GFX_VIEWLIST_NODE) {
		/*
		 * ViewList being added to ViewList.
		 * If parent ViewList has type, and child has none, assign
		 *  it type of parent.
		 * If child has type, check for match.
		 * If parent has no type, back-propogate child's type until
		 *  top of tree or mismatch.
		 */
		ViewList	*chvl;

		chvl = (ViewList *) n;
		if (vl->vl_Projector) {
			if (chvl->vl_Projector) {
				if (chvl->vl_Projector != vl->vl_Projector)
					return (GFX_ERR_VIEWINCOMPAT);
			} else
				chvl->vl_Projector = vl->vl_Projector;
		} else {
			if (chvl->vl_Projector) {
				Err	err;

				if ((err = backpropogate
					    (vl, chvl->vl_Projector)) < 0)
					return (err);
			}
		}
		p = chvl->vl_Projector;
	} else
		return (GFX_ERR_BADITEM);

	/*
	 * Fiddle with lists.
	 */
	if (n->n_Next)
		return (GFX_ERR_VIEWINUSE);

	AddHead (&vl->vl_SubViews, n);

	if (recompile)
		(p->p_Update) (p, PROJOP_ADDVIEW, n);

	return (0);
}


/*
 * And you may ask yourself, what's this for?
 *
 * Since v_Projector pointers are *always* assumed to be valid for a
 * Projector's entire View tree, we must make certain no one manages to slip
 * in a View of incompatible type.  A client could create two ViewLists,
 * make one the child of the other (both still have NULL type), then attach
 * a View to the child ViewList, and attach the parent ViewList to an
 * incompatible Projector.  Without the below code to back-propogate
 * Projector types, such an act would go boom.  Very bad.  Must avoid.
 */
Err
backpropogate (vl, p)
struct ViewList		*vl;
struct Projector	*p;
{
	while (1) {
		/*
		 * If parent ViewList already has type, check it.
		 * If no type, assign it.
		 */
		if (vl->vl_Projector) {
			if (vl->vl_Projector == p)
				/*
				 * We've looked at everything we must.
				 */
				return (0);
			else
				return (GFX_ERR_VIEWINCOMPAT);
		} else
			vl->vl_Projector = p;

		if (!vl->vl.n_Next)
			/*
			 * Not linked; no further parents to scan.
			 */
			break;

		/*
		 * Seek back to beginning of parent list.
		 */
		while (PREVNODE (vl))
			vl = (ViewList *) PREVNODE (vl);

		/*
		 * Adjust pointer to point to base of parent ViewList.
		 */
		vl = (ViewList *)
		 ((uint32) vl - offsetof (ViewList, vl_SubViews.ListAnchor));
	}
	return (0);
}


/**
|||	AUTODOC -public -class graphicsfolio -name RemoveView
|||	Remove a View from its ViewList.
|||
|||	  Synopsis
|||
|||	    Err RemoveView (Item viewItem)
|||
|||	  Description
|||
|||	    This routine removes a View or ViewList from whichever ViewList
|||	    or Projector it may be currently attached to.  If the
|||	    View/ViewList is not currently attached to a ViewList/Projector,
|||	    nothing is done.
|||
|||	    The argument viewItem may be either a View Item or a ViewList
|||	    Item.
|||
|||	    If viewItem is currently visible, it will be removed from the
|||	    display.  If viewItem is a ViewList, all subordinate Views and
|||	    ViewLists vanish, too.
|||
|||	  Arguments
|||
|||	    viewItem
|||	        Item number of the View or ViewList to be removed.
|||
|||	  Return Value
|||
|||	    This routine will return zero upon successful completion, or a
|||	    (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/view.h>
|||
|||	  See Also
|||
|||	    AddViewToViewList(), OrderViews(), View(@), ViewList(@)
|||
**/
Err
SuperRemoveView (viewItem)
Item	viewItem;
{
	register Node	*v;

	/* ### ownership checks.  */
	if (!(v = LookupItem (viewItem)))
		return (GFX_ERR_BADITEM);

	if (v->n_SubsysType != NST_GRAPHICS  ||
	    (v->n_Type != GFX_VIEW_NODE  &&
	     v->n_Type != GFX_VIEWLIST_NODE))
		return (GFX_ERR_BADITEM);

	return (removeview ((View *) v));
}


Err
removeview (v)
struct View	*v;
{
	if (v->v.n_Next) {
		/*
		 * It's in a ViewList somewhere; possibly visible.
		 * Update the display.  (ERROR DELIBERATELY IGNORED.)
		 */
		Projector	*p;

		RemNode ((Node *) v);

		if (v->v.n_Type == GFX_VIEW_NODE)
			p = v->v_Projector;
		else
			p = ((ViewList *) v)->vl_Projector;

		(p->p_Update) (p, PROJOP_REMOVEVIEW, v);

		/*
		 * We NULL these out here because the Projector will
		 * optimize away recompiles for disconnected Views, and we
		 * want it to rebuild the display without this View(List).
		 */
		v->v.n_Next = v->v.n_Prev = NULL;
	}
		/*  Should I return an error elsewise?  */

	if (v->v.n_Type == GFX_VIEWLIST_NODE)
		if (IsEmptyList (&((ViewList *) v)->vl_SubViews))
			((ViewList *) v)->vl_Projector = NULL;

	return (0);
}



/**
|||	AUTODOC -public -class graphicsfolio -name OrderViews
|||	Depth-arrange a View relative to another View.
|||
|||	  Synopsis
|||
|||	    Err OrderViews (Item victim, int32 op, Item radix)
|||
|||	  Description
|||
|||	    This routine depth-arranges Views by reordering them within the
|||	    ViewList; Views earlier in the list sit atop Views later in the
|||	    list.
|||
|||	    The victim View is the View to be reordered.  The radix View is
|||	    the View against which the victim View will be sorted, or zero.
|||	    The op argument tells how the victim View is to be placed
|||	    relative to the radix View.
|||
|||	    If radix is a valid View Item, and if op is set to
|||	    ORDEROP_BEFORE, the victim View will be placed immediately
|||	    before the radix View.  If op is set to ORDEROP_AFTER, the
|||	    victim View will be placed immediately behind the radix View.
|||	    The radix View must reside in a ViewList.  The victim View need
|||	    not be attached to the same ViewList, or indeed to any ViewList.
|||	    Upon completion, the victim View will reside in the same
|||	    ViewList as does the radix View.
|||
|||	    If radix is zero, the victim View is moved to the beginning or
|||	    the end of the ViewList in which it currently resides, resulting
|||	    in a "pull to front/push to back" operation.  If op is set to
|||	    ORDEROP_BEFORE, the victim View is placed before all Views.
|||	    If op is ORDEROP_AFTER, the victim View is placed after all
|||	    Views in the current ViewList (at the back).
|||
|||	    Both victim and radix may be either Views or ViewLists
|||	    themselves.
|||
|||	    If radix is a valid View Item, and is not installed in any
|||	    ViewList, an error is returned.  If radix is zero, and the
|||	    victim View is not installed in any ViewList, an error is
|||	    returned.
|||
|||	    By reading the argument list left to right, the semantics of
|||	    this routine become obvious (i.e. "victim BEFORE radix", etc).
|||
|||	  Big Fat Warning
|||
|||	    DO NOT use this function as a foundation for double-buffering.
|||	    This routine forces a recompilation of the entire display list,
|||	    which is very time-consuming.  Use ModifyGraphicsItem() instead
|||	    to change the Bitmap Item used by the View.
|||
|||	  Arguments
|||
|||	    victim
|||	        Item number of the View or ViewList to depth-arrange.
|||
|||	    op
|||	        Depth arranging operation; may be either ORDEROP_BEFORE or
|||	        ORDEROP_AFTER.
|||
|||	    radix
|||	        Item number of the View or ViewList against which to
|||	        depth-arrange the victim.  A zero value indicates the
|||	        victim is to be moved to the front/back of its current
|||	        ViewList.
|||
|||	  Return Value
|||
|||	    This routine will return zero upon successful completion, or a
|||	    (negative) error code.  In case of an error, the View ordering
|||	    is left unchanged.
|||
|||	  Associated Files
|||
|||	    <graphics/view.h>
|||
|||	  See Also
|||
|||	    AddViewToViewList(), RemoveView(), ViewList(@)
|||
**/
Err
SuperOrderViews (victim, op, radix)
Item	victim, radix;
int32	op;
{
	Projector	*pv, *pr;
	Node		*v, *r;

	/*
	 * Validate items.
	 * ### ownership checks.
	 */
	if (!(v = LookupItem (victim))  ||
	    v->n_SubsysType != NST_GRAPHICS)
		return (GFX_ERR_BADITEM);

	if (v->n_Type == GFX_VIEW_NODE)
		pv = ((View *) v)->v_Projector;
	else if (v->n_Type == GFX_VIEWLIST_NODE)
		pv = ((ViewList *) v)->vl_Projector;
	else
		return (GFX_ERR_BADITEM);

	if (radix) {
		if (!(r = LookupItem (radix))  ||
		    r->n_SubsysType != NST_GRAPHICS)
			return (GFX_ERR_BADITEM);

		if (r->n_Type == GFX_VIEW_NODE)
			pr = ((View *) r)->v_Projector;
		else if (r->n_Type != GFX_VIEWLIST_NODE)
			pr = ((ViewList *) r)->vl_Projector;
		else
			return (GFX_ERR_BADITEM);

		if (pr != pv)
			return (GFX_ERR_VIEWINCOMPAT);
	} else
		r = NULL;


	/*
	 * Validate op.
	 */
	if (op != ORDEROP_BEFORE  &&  op != ORDEROP_AFTER)
		return (GFX_ERR_BADORDEROP);


	/*
	 * Fiddle with lists.
	 */
	if (r) {
		/*
		 * We have an actual radix; order against it.
		 */
		if (!r->n_Next)
			return (GFX_ERR_VIEWNOTINLIST);

		if (v->n_Next) {
			/*
			 * This is a little complicated.  If moving the View
			 * v would result in an "orphan" ViewList, then
			 * NULL out the Projector field in the ViewList.
			 */
			ViewList	*vl;

			/*
			 * Find ViewList.
			 */
			vl = (ViewList *) v->n_Prev;

			while (PREVNODE (vl))
				vl = (ViewList *) PREVNODE (vl);
			vl = (ViewList *)
			 ((uint32) vl - offsetof (ViewList,
			 			  vl_SubViews.ListAnchor));

			/*
			 * Pull View.
			 */
			RemNode (v);

			/*
			 * Check if ViewList is orphaned.
			 */
			if (IsEmptyList (&vl->vl_SubViews)  &&
			    !vl->vl.n_Next)
				vl->vl_Projector = NULL;
		}

		if (op == ORDEROP_BEFORE)
			InsertNodeBefore (r, v);
		else
			InsertNodeAfter (r, v);
	} else {
		/*
		 * Idiomatic NULL radix.  Put View at head/tail of list.
		 * (Projector checking not required, since it's not
		 * leaving this ViewList.)
		 */
		if (!v->n_Next)
			return (GFX_ERR_VIEWNOTINLIST);

		/*
		 * Seek back to list header.
		 */
		r = v;
		while (PREVNODE (r))
			r = PREVNODE (r);
		r = (Node *) ((uint32) r - offsetof (List, ListAnchor));

		/*
		 * r now points to list header.  Stuff View in requested
		 * position.
		 */
		RemNode (v);
		if (op == ORDEROP_BEFORE)
			AddHead ((List *) r, v);
		else
			AddTail ((List *) r, v);
	}

	(pv->p_Update) (pv, PROJOP_ORDERVIEW, v);

	return (0);
}


/***************************************************************************
 * Internal, supervisor-only routines.
 * Only trusted routines may call these.
 */
void
SuperInternalSetRendCallBack (v, vector, ptr)
register struct View	*v;
void			(*vector)(struct View *, void *);
void			*ptr;
{
	if (v->v_RendCBFunc = vector)
		v->v_ViewFlags |= VIEWF_FORCESCHEDSIG_REND;
	else
		v->v_ViewFlags &= ~VIEWF_FORCESCHEDSIG_REND;

	v->v_RendCBData = ptr;
}

void
SuperInternalSetDispCallBack (v, vector, ptr)
register struct View	*v;
void			(*vector)(struct View *, void *);
void			*ptr;
{
	if (v->v_DispCBFunc = vector)
		v->v_ViewFlags |= VIEWF_FORCESCHEDSIG_DISP;
	else
		v->v_ViewFlags &= ~VIEWF_FORCESCHEDSIG_DISP;

	v->v_DispCBData = ptr;
}


/***************************************************************************
 * Support routines.
 */
static Err
typescompatible (vti, bmtype)
ViewTypeInfo	*vti;
int32		bmtype;
{
	register int	i, match;
	int32		*test;

	if (!(test = vti->vti_BMTypes))
		return (!(vti->vti_Flags & VTIF_BMTYPES_ALLOWED));

	match = FALSE;
	for (i = vti->vti_NBMTypes;  --i >= 0; )
		if (*test++ == bmtype) {
			match = TRUE;
			break;
		}

	/*
	 * Why didn't ANSI specify a ^^ operator...  *sigh*
	 */
	if (match ^ !(vti->vti_Flags & VTIF_BMTYPES_ALLOWED))
		return (0);
	else
		return (GFX_ERR_TYPEMISMATCH);
}
