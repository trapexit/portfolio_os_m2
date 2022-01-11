/*  :ts=8 bk=0
 *
 * projector.c:	Projector Item management
 *
 * @(#) projector.c 96/09/17 1.8
 *
 * Leo L. Schwab
 */
#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/mem.h>

#include <graphics/projector.h>

#include <string.h>

#include "protos.h"


/***************************************************************************
 * Autodoc for Projector Item.
 */
/**
|||	AUTODOC -class Items -name Projector
|||	An Item representing a particular physical display type.
|||
|||	  Description
|||
|||	    Projectors are the foundation for hardware-specific display
|||	    manipulation; a form of video "device driver."
|||
|||	    Projectors represent an abstract "display mode," a physical
|||	    method of presenting imagery on a display.  For example, the
|||	    NTSC and PAL video standards are two different methods of
|||	    displaying imagery, each having different characteristics such
|||	    as refresh rate, maximum pixel dimensions, and so on.  Likewise,
|||	    NTSC interlaced and NTSC non-interlaced have important
|||	    differences, such as number of fields per frame, and maximum
|||	    number of displayable lines.
|||
|||	    Projectors represent an abstract description of such
|||	    characteristics.  It's also an "anchor point" for View
|||	    heirarchies.  Indeed, each Projector can be thought of as the
|||	    root ViewList for each supported display type.
|||
|||	    Projectors are created and maintained by the graphics folio.
|||	    Upon startup, the folio looks for, loads, and initializes every
|||	    Projector in the system that can be supported by the underlying
|||	    hardware.
|||
|||	    You cannot create Projectors, but you may open them.  You do not
|||	    need to open a Projector to add Views to it.  However, you must
|||	    open a Projector if you wish to make its View visible by
|||	    activating it (using ActivateProjector()).  Opening the
|||	    Projector is necessary so that the system will be able to
|||	    discover if a Projector is unused, and deactivate it.
|||
|||	    More than one Projector can be associated with a given physical
|||	    display (e.g. NTSC interlaced and NTSC non-interlaced are two
|||	    different Projectors sharing the same monitor).  In such cases,
|||	    only one Projector may be active at a given time.  The View
|||	    heirarchy attached to the active Projector is the one visible;
|||	    View heirarchies attached to inactive Projectors are not visible.
|||
|||	    Views and ViewLists are attached to Projectors using
|||	    AddViewToViewList(), supplying the Item number of the Projector
|||	    as the target ViewList.
|||
|||	    The graphics folio maintains a "default Projector" for use by
|||	    applications which don't care to use a specific Projector.  (As
|||	    an example, the default Projector is the one used when you pass
|||	    the value zero as the target ViewList in the function
|||	    AddViewToViewList().)  The default Projector is selected by the
|||	    user, and should be regarded as the place the user would prefer
|||	    to see an application's Views appear.  Unless you have a
|||	    compelling reason to do otherwise, you should place your Views
|||	    on the default Projector.
|||
|||ifndef EXTERNAL_RELEASE
|||	  Internal System Specifics
|||
|||	    Projectors are implemented using the Portfolio device driver
|||	    facilities, but do not actually employ the I/O model.  The
|||	    device driver facilities were used to leverage off the demand
|||	    loading mechanism and DDF capabilities (allowing us to
|||	    theoretically put any number of drivers on the disc, but only
|||	    those that would actually work would get loaded).  However, the
|||	    I/O model was not used due to concerns about the overhead.
|||	    Communication between the folio and the Projectors is
|||	    accomplished through a set of function pointers in the Projector
|||	    Item.
|||
|||endif
|||	  Tag Arguments
|||
|||	    The following tags may be specified as part of the TagArg list
|||	    supplied to OpenItem():
|||
|||	    PROJTAG_ACTIVATE (boolean)
|||	        When supplied to an OpenItem() call, and when the argument
|||	        to this Tag is true (non-zero), the Projector to opened will
|||	        be activated as well.  This helps you to avoid having to
|||	        call ActivateProjector().  Unlike using ActivateProjector(),
|||	        you will not receive as a return value the Item number of
|||	        the Projector that was deactivated to permit the activation
|||	        of the new one.
|||
|||ifndef EXTERNAL_RELEASE
|||	    The following tags may only be specified at CreateItem() time:
|||
|||	    PROJTAG_WIDTH (uint32)
|||	        The maximum displayable width of the display, in display
|||	        (View positioning) coordinates.
|||
|||	    PROJTAG_HEIGHT (uint32)
|||	        The maximum displayable height of the display, in display
|||	        (View positioning) coordinates.
|||
|||	    PROJTAG_PIXELWIDTH (uint32)
|||	        The maximum displayable width of the display, in the
|||	        smallest pixels available from this Projector.
|||
|||	    PROJTAG_PIXELHEIGHT (uint32)
|||	        The maximum displayable height of the display, in the
|||	        smallest pixels available from this Projector.
|||
|||	    PROJTAG_FIELDSPERSECOND (float32)
|||	        The exact number of fields scanned out to the monitor each
|||	        second.  This is a float so it can describe refresh rates
|||	        that aren't exact integral values (like NTSC's, which is
|||	        59.94).
|||
|||	    PROJTAG_FIELDSPERFRAME (uint8)
|||	        The number of fields required to describe a complete frame
|||	        of imagery.  NTSC is two.  VGA-style displays are typically
|||	        one.
|||
|||	    PROJTAG_XASPECT (uint8)
|||	        The horizontal component of the aspect ratio of this
|||	        Projector's squarest available pixels.
|||
|||	    PROJTAG_YASPECT (uint8)
|||	        The vertical component of the aspect ratio of this
|||	        Projector's squarest available pixels.
|||
|||	    PROJTAG_PROJTYPE (int32)
|||	        An encoded Projector ID number.  Although Projector IDs are
|||	        stored as uint8s, they are presented to this Tag as
|||	        pre-formatted int32s (as if they were View type identifiers).
|||
|||	    PROJTAG_BLANKVIEWTYPE (ViewTypeInfo *)
|||	        A pointer to the ViewTypeInfo structure that should be used
|||	        to compose blank Views.  This is used by the folio's
|||	        compiler service (SuperInternalCompile()) to build
|||	        Transition segments for Views that are malformed or have no
|||	        Bitmap attached.
|||
|||	    PROJTAG_VEC_INIT (Err (*)())
|||	        Pointer to Projector initialization vector.  This is called
|||	        as part of the CreateItem() sequence.  "Global" driver
|||	        and/or hardware stuff should be done in the driver's
|||	        initialization routine.  This vector is called to fill out
|||	        the Projector Item structure itself.
|||
|||	        The init vector is prototyped as follows:
|||
|||	        Err init (struct Projector *p, struct GraphicsFolioBase *gb);
|||
|||	        'p' is a pointer to the Projector Item you are expected to
|||	        initialize.  'gb' is a pointer to graphics' Folio Item
|||	        structure, in case there are any bits you need out of there.
|||
|||	    PROJTAG_VEC_EXPUNGE (Err (*)())
|||	        Pointer to the Projector expunge vector.  This is called as
|||	        part of the DeleteItem() sequence.  "Global" driver teardown
|||	        should not be done here; this is to properly clean up the
|||	        Projector Item itself.
|||
|||	        The expunge vector is prototyped as follows:
|||
|||	        Err expunge (struct Projector *p);
|||
|||	        'p' is a pointer to the Projector about to be deleted.
|||
|||	    PROJTAG_VEC_SCAVENGE (Err (*)()) [CURRENTLY UNIMPLEMENTED]
|||	        Pointer to the Projector's scavenge vector.  This vector is
|||	        intended to be called when the system runs low on memory,
|||	        and needs components to clean up whatever they can.  This
|||	        vector should FreeMem() transient data structures where
|||	        possible.
|||
|||	    PROJTAG_VEC_LAMP (Err (*)())
|||	        Pointer to the Projector's lamp vector.  This vector is
|||	        responsible for doing whatever tricks are necessary to make
|||	        this Projector and its subordinate Views visible on the
|||	        display.  This may possibly entail shutting down another
|||	        Projector using the same video hardware.
|||
|||	        The lamp vector is prototyped as follows:
|||
|||	        Err lamp (struct Projector *p, int32 op);
|||
|||	        Operation of the lamp vector is discussed in a later
|||	        section.
|||
|||	    PROJTAG_VEC_UPDATE (Err (*)())
|||	        Pointer to the Projector's update vector.  This vector is
|||	        called in response to a variety of operations performed
|||	        within the graphics folio.  It gets called when a View is
|||	        modified, moved, depth arranged, made visible, removed, or
|||	        deleted.
|||
|||	        The update vector is prototyped as follows:
|||
|||	        Err update (struct Projector *p, int32 reason, void *ob);
|||
|||	        Operation of the update vector is discussed in a later
|||	        section.
|||
|||	    PROJTAG_VEC_NEXTVIEWTYPE (ViewTypeInfo *(*)())
|||	        Pointer to the Projector's nextviewtype vector.  This is
|||	        called by the folio when it is searching for a particular
|||	        ViewTypeInfo structure.
|||
|||	        The nextviewtype vector is prototyped as follows:
|||
|||	        struct ViewTypeInfo *nextviewtype (struct Projector
|||	        *p, struct ViewTypeInfo *vti);
|||
|||	        'p' points to the Projector whose ViewTypeInfo database is
|||	        being searched.  'vti' points to a ViewTypeInfo structure
|||	        previously retrieved from the vector; if 'vti' is NULL, a
|||	        pointer to the first available ViewTypeInfo structure is
|||	        returned.
|||
|||	        The vector returns a pointer to the next available
|||	        ViewTypeInfo structure.  If there are no ViewTypeInfos left
|||	        for this Projector, NULL is returned.
|||
|||	  Lamp Vector
|||
|||	    The lamp vector is the point of contact for turning Projectors
|||	    on and off.  This is especially important for display hardware
|||	    that is used by multiple Projectors.
|||
|||	    The lamp vector receives two parameters:  A pointer to the
|||	    Projector being "lamped," and an operation code.  The operation
|||	    code can have the following values:
|||
|||	    LAMPOP_ACTIVATE
|||	        Turn the Projector on, making it visible on the display.
|||	        If another Projector is currently using the display
|||	        hardware, it is deactivated first; in such a case, the
|||	        Item number of the deactivated Projector is returned.
|||
|||	    LAMPOP_DEACTIVATE
|||	        Turn the Projector off, removing it from the display.  If
|||	        it's possible to shut down the underlying hardware without
|||	        disturbing the rest of the system, the module should do this
|||	        as well.
|||
|||	    LAMPOP_DEACTIVATE_RESCUE
|||	        An odd variation of LAMPOP_DEACTIVATE; it is invoked
|||	        when the Projector has been CloseItem()ed and the open
|||	        count has decremented to zero.  Upon receipt, the module
|||	        is expected to deactivate the supplied Projector, and
|||	        then look around for other Projectors that appear on the
|||	        same physical display, see which ones still have a non-zero
|||	        open count, and activate one of them.  This exists so that,
|||	        if a program crashes after having activated a non-default
|||	        Projector, the display won't be left showing an unused
|||	        Projector.
|||
|||	    The vector should return zero upon success (or, in the case of
|||	    LAMPOP_ACTIVATE, the item number of the Projector that was
|||	    shoved out of the way), or a negative error code.  If an error
|||	    occurs, the module should attempt to leave the display unchanged.
|||
|||	  Update Vector
|||
|||	    The update vector is the most heavily trafficked routine in the
|||	    Projector.  It is the central contact point between the
|||	    Projector and the core graphics folio for manipulating the
|||	    display.
|||
|||	    The update vector receives three parameters: a pointer to the
|||	    Projector affected, an operation code, and a pointer to some
|||	    pertinent data.  The way the data pointer is interpreted changes
|||	    based on the operation being requested.
|||
|||	    The available operation codes are:
|||
|||	    PROJOP_NOP
|||	        Do nothing; return immediately.  'ob' is undefined.
|||
|||	    PROJOP_MODIFY
|||	        You are being called as part of a ModifyGraphicsItem() or a
|||	        CreateItem() operation.  'ob' points to a ModifyInfo
|||	        structure, which contains a pointer to the View ready to be
|||	        integrated into the display, a pointer to the same View
|||	        *before* it was modified (useful for certain optimizations),
|||	        and a pointer to the base of the TagArg array that was used
|||	        to modify the View (also useful for optimizations).
|||
|||	    PROJOP_ADDVIEW
|||	        A View is being added somewhere in the Projector's View
|||	        heirarchy.  'ob' points to the View being added.
|||
|||	    PROJOP_REMOVEVIEW
|||	        A View is being removed from somewhere in the Projector's
|||	        View heirarchy.  'ob' points to the View being removed.
|||
|||	    PROJOP_ORDERVIEW
|||	        A View is being depth-arranged somewhere in the Projector's
|||	        View heirarchy.  'ob' points to the View being
|||	        depth-arranged.
|||
|||	    PROJOP_UNLOCKDISPLAY
|||	        The Projector has just been been unlocked with
|||	        UnlockDisplay(); the Projector should now integrate all
|||	        outstanding changes into the visible display.  'ob' is
|||	        undefined.
|||
|||	    PROJOP_ACTIVATE
|||	        The Projector has just been activated; it should regenerate
|||	        all its internal data structures and make its View list
|||	        visible on the display.  'ob' is undefined.
|||
|||	    When called, the update vector should interpret the operation
|||	    code and pertinent data and perform whatever operations are
|||	    necessary to make the display an accurate reflection of the
|||	    current state of the View heirarchy.
|||
|||	    Upon successful completion, the vector should return zero, and
|||	    the display should be up to date.  If anything inside the update
|||	    vector fails, the routine should leave the display unchanged and
|||	    return a negative error code.  This error code will be passed
|||	    directly back to the application.
|||
|||	    The vector is responsible for checking the p_ViewListSema4 to
|||	    see if it has been locked.  If so, the display should *not* be
|||	    updated, though the routine should still return a success code.
|||	    The vector is free to update internal state; if such an internal
|||	    update fails, a (negative) error code should be returned.
|||endif
|||	  Caveats
|||
|||	    Passing tags through OpenItem() doesn't actually work yet, as
|||	    the kernel currently disallows it.
|||
|||ifndef EXTERNAL_RELEASE
|||	    I have not yet worked out how to specify PROJTAG_WIDTH and
|||	    PROJTAG_HEIGHT for display/video hardware where Views cannot be
|||	    positioned (like on NickNack, for instance).
|||
|||endif
|||	  Folio
|||
|||	    graphics
|||
|||	  Item Type
|||
|||	    GFX_PROJECTOR_NODE
|||
|||ifndef EXTERNAL_RELEASE
|||	  Create
|||
|||	    CreateItem()
|||
|||	  Delete
|||
|||	    DeleteItem()
|||
|||endif
|||	  Use
|||
|||	    AddViewToViewList(), LockDisplay(), UnlockDisplay(),
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>
|||
|||	  See Also
|||
|||	    CreateItem(), AddViewToViewList(), LockDisplay(),
|||	    UnlockDisplay(), ActivateProjector(),
|||	    DeactivateProjector(), NextProjector()
|||
**/


/***************************************************************************
 * Code.
 */
static Err
icp (
struct Projector	*p,
uint32			*flags,
uint32			t,
uint32			a
)
{
	register uint32	f;

	f = *flags;

	DDEBUG (("Enter icb %lx %lx\n", t, a));
	switch (t) {
	case PROJTAG_WIDTH:
		p->p_Width = a;
		break;
	case PROJTAG_HEIGHT:
		p->p_Height = a;
		break;
	case PROJTAG_PIXELWIDTH:
		p->p_PixWidth = a;
		break;
	case PROJTAG_PIXELHEIGHT:
		p->p_PixHeight = a;
		break;
	case PROJTAG_FIELDSPERSECOND:
		p->p_FieldsPerSecond = ConvertTagData_FP ((void *) a);
		break;
	case PROJTAG_FIELDSPERFRAME:
		p->p_FieldsPerFrame = (uint8) a;
		break;
	case PROJTAG_XASPECT:
		p->p_XAspect = (uint8) a;
		break;
	case PROJTAG_YASPECT:
		p->p_YAspect = (uint8) a;
		break;
	case PROJTAG_PROJTYPE:
		p->p_Type = (uint8) (a >> PROJTYPE_SHIFT);
		break;
	case PROJTAG_BLANKVIEWTYPE:
		p->p_BlankVTI = (ViewTypeInfo *) a;
		break;
	case PROJTAG_VEC_INIT:
		p->p_Init = (Err (*)()) a;
		break;
	case PROJTAG_VEC_EXPUNGE:
		p->p_Expunge = (Err (*)()) a;
		break;
	case PROJTAG_VEC_SCAVENGE:
		p->p_Scavenge = (Err (*)()) a;
		break;
	case PROJTAG_VEC_LAMP:
		p->p_Lamp = (Err (*)()) a;
		break;
	case PROJTAG_VEC_UPDATE:
		p->p_Update = (Err (*)()) a;
		break;
	case PROJTAG_VEC_NEXTVIEWTYPE:
		p->p_NextViewType = (ViewTypeInfo *(*)()) a;
		break;
	default:
		return (GFX_ERR_BADTAGARG);
	}

	*flags = f;
	return (0);
}


Err
modifyprojector (
struct Projector	*clientp,
struct TagArg		*args
)
{
	Projector	p;
	Err		err;
	uint32		flags;

	memcpy (&p, clientp, sizeof (Projector));

	if ((err = TagProcessor (&p, args, icp, &flags)) < 0)
		return (err);

	/*
	 * If PixWidth unset by client, make it the same as positional
	 * width (zero is an unreasonable value).
	 */
	if (!p.p_PixWidth)
		p.p_PixWidth = p.p_Width;
	if (!p.p_PixHeight)
		p.p_PixHeight = p.p_Height;

	memcpy (clientp, &p, sizeof (Projector));

	return (p.p.n_Item);
}


Item
createprojector (
struct Projector	*p,
struct TagArg		*ta
)
{
	Err	err;

	/*
	 * Apply client tags.
	 */
	if ((err = modifyprojector (p, ta)) < 0)
		return (err);

	/*
	 * A few more things...
	 */
	if ((err = SuperCreateSemaphore (p->p.n_Name, 0)) < 0) {
		deleteprojector (p);
		return (err);
	}
	p->p_ViewListSema4 = LookupItem (err);

	InitList (&p->p_ViewList.vl_SubViews, NULL);
	p->p_ViewList.vl_Projector = p;

	/*
	 * Perform module-specific initialization.
	 */
	if (p->p_Init)
		if ((err = (p->p_Init)
			    (p, (GraphicsFolioBase *) &GBASE(gb))) < 0)
		{
			deleteprojector (p);
			return (err);
		}

	SuperInternalLockSemaphore (GBASE(gb_ProjectorListSema4), SEM_WAIT);
	AddTail (&GBASE(gb_ProjectorList), (Node *) p);
	SuperInternalUnlockSemaphore (GBASE(gb_ProjectorListSema4));

	return (p->p.n_Item);
}


/*
 * This routine is coded to perform cleanup of partially-built Items.
 * The module expunge vector must also be able to handle partial cleanups.
 * ### Also handle removing Views from ViewList?
 */
Err
deleteprojector (
struct Projector	*p
)
{
	Err	err;

	if (p->p.n_Next) {
		SuperInternalLockSemaphore (GBASE(gb_ProjectorListSema4),
					    SEM_WAIT);
		RemNode ((Node *) p);
		SuperInternalUnlockSemaphore (GBASE(gb_ProjectorListSema4));
		p->p.n_Next = p->p.n_Prev = NULL;
	}

	if (p->p_Expunge)
		if ((err = (p->p_Expunge) (p)) < 0)
			return (err);

	if (p->p_ViewListSema4)
		SuperDeleteSemaphore (p->p_ViewListSema4->s.n_Item);

	return (0);
}


Item
findprojector (
struct TagArg	*ta
)
{
	ItemNode	in, *it;
	Err		err;

	memset (&in, 0, sizeof (in));

	if ((err = TagProcessorNoAlloc (&in, ta, NULL, 0)) < 0)
		return (err);

	if (!in.n_Name)
		return (GFX_ERR_NOTFOUND);

	SuperInternalLockSemaphore (GBASE(gb_ProjectorListSema4), SEM_WAIT | SEM_SHAREDREAD);
	it = (ItemNode *) FindNamedNode (&GBASE(gb_ProjectorList), in.n_Name);
	SuperInternalUnlockSemaphore (GBASE(gb_ProjectorListSema4));

	if (!it)
		return (GFX_ERR_NOTFOUND);

	return (it->n_Item);
}


Item
openprojector (
struct Projector	*p,
const struct TagArg	*ta
)
{
	const TagArg	*state;
	Err		err;

	/*
	 * Not using TagProcessor() because that would permit the client
	 * to change the name of the Projector.  Bad bad bad...
	 */
	state = ta;
	while (ta = NextTagArg (&state)) {
		if (ta->ta_Tag != PROJTAG_ACTIVATE)
			return (GFX_ERR_BADTAGARG);
		else
			if (ta->ta_Arg) {
				if (!p->p_Lamp)
					return (GFX_ERR_INTERNAL);
				else if ((err = (p->p_Lamp)
						 (p, LAMPOP_ACTIVATE)) < 0)
					return (err);
			}
	}

	return (p->p.n_Item);
}


Err
closeprojector (
struct Projector	*p,
struct Task		*t
)
{
	TOUCH (t);

	if (!p->p.n_OpenCount) {
		/*
		 * No one has it open.  Deactivate the Projector, with
		 * a little note saying that we'd like another one turned
		 * on (whoever has a positive n_OpenCount).
		 */
		if (p->p_Lamp)
			(p->p_Lamp) (p, LAMPOP_DEACTIVATE_RESCUE);
	}

	return (0);
}

Err
setownerprojector (
struct Projector	*n,
Item			newOwner,
struct Task		*task
)
{
	TOUCH (n);
	TOUCH (newOwner);
	TOUCH (task);

	return (0);
}


/***************************************************************************
 * Other client routines.
 */
/**
|||	AUTODOC -class graphicsfolio -name NextProjector
|||	Return Item number of next available Projector.
|||
|||	  Synopsis
|||
|||	    Item NextProjector (Item projectorItem)
|||
|||	  Description
|||
|||	    This function is used to navigate the list of Projectors
|||	    available in the system.
|||
|||	    Given a valid Projector Item number, the function will return
|||	    the Item number of the next available Projector in the system.
|||	    If there are no further Projectors, zero will be returned.
|||
|||	    To obtain the Item number of the first available Projector, pass
|||	    in a value of zero.
|||
|||	  Arguments
|||
|||	    projectorItem
|||	        Item number of a Projector, or zero.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Item number of the next available Projector.
|||
|||	    0
|||	        End of list; no further Projectors are available.
|||
|||	    < 0
|||	        An error code.
|||
|||	  Example
|||
|||	    Projector   *p
|||	    Item        pi;
|||
|||	    pi = 0;
|||	    while ((pi = NextProjector (pi)) > 0) {
|||	        p = LookupItem (pi);
|||
|||	        // At this point, you can inspect the contents of the
|||	        // Projector structure.
|||	    }
|||
|||	  Notes
|||
|||	    No significance should be inferred from the order in which
|||	    Projectors are returned by the system.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>
|||
|||	  See Also
|||
|||	    Projector(@)
|||
**/
Item
NextProjector (pi)
Item	pi;
{
	Projector	*p;
	Item		retval;

	LockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item,
		       SEM_WAIT|SEM_SHAREDREAD);

	if (pi) {
		/*
		 * Return next Projector in list, if any.
		 */
		if (p = CheckItem (pi, NST_GRAPHICS, GFX_PROJECTOR_NODE))
			p = (Projector *) NEXTNODE (p);
		else {
			retval = GFX_ERR_BADITEM;
			goto ackphft;	/*  Look down.  */
		}
	} else
		/*
		 * Return first Projector in list.
		 */
		p = (Projector *) FIRSTNODE (&GBASE(gb_ProjectorList));

	if (NEXTNODE (p))
		retval = p->p.n_Item;
	else
		retval = 0;

ackphft:
	UnlockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item);
	return (retval);
}


/**
|||	AUTODOC -class graphicsfolio -name ActivateProjector
|||	Activate a Projector, making it visible.
|||
|||	  Synopsis
|||
|||	    Item ActivateProjector (Item projectorItem)
|||
|||	  Description
|||
|||	    This function will activate the specified Projector, making it
|||	    visible on the display hardware.  The Projector Item being
|||	    activated must be opened beforehand by the calling task using
|||	    OpenItem().
|||
|||	    Some Projectors "share" hardware.  For a given piece of
|||	    hardware, only one Projector may be active at a time.  If
|||	    another Projector is using the hardware needed by the one you
|||	    wish to activate, it will be deactivated first.  In such a case,
|||	    the Item number of the deactivated Projector is returned.
|||
|||	    It is safe to call ActivateProjector() on a Projector that is
|||	    already active.
|||
|||	  Notes
|||
|||	    Views posted to a given Projector are not visible if the
|||	    Projector is not active.  Thus, if, in activating a Projector
|||	    you deactivate a different one, the Views on the previous
|||	    Projector cease to be visible.
|||
|||	    There is a further weird case involving the folio's "default
|||	    Projector" (the Projector used by the folio when none is
|||	    explicitly specified).  The folio makes no attempt to track
|||	    which Projectors are visible (i.e. the default Projector will
|||	    not "move" to the newly activated one).  Thus, if you happen to
|||	    shut off the default Projector, all Views posted to it cease
|||	    to be visible.  Further, subsequent programs posting Views to
|||	    the default Projector will also not be visible.
|||
|||	  Caveats
|||
|||	    Other applications can call ActivateProjector() behind your
|||	    back, too, making your Views invisible.  Don't "fight" with
|||	    other applications to keep your preferred Projector active.
|||
|||	    There is no way to tell which Projectors are sharing a given
|||	    piece of hardware.
|||
|||	  Warning
|||
|||	    Unless you have a REAL GOOD REASON, you shouldn't be calling
|||	    this function.  Use the default Projector if at all possible.
|||	    The only one who should be Projector-surfing is the user.
|||
|||	    If you insist on using a particular Projector, use
|||	    QueryGraphics() to obtain the item number of the default
|||	    Projector and reactivate it before you exit.
|||
|||	  Arguments
|||
|||	    projectorItem
|||	        Item number of the Projector to activate.
|||
|||	  Return Value
|||
|||	    > 0
|||	        Item number of the Projector that was deactivated to make
|||	        this one active.  The specified Projector was successfully
|||	        activated.
|||
|||	    0
|||	        The specified Projector was successfully activated.
|||
|||	    < 0
|||	        An error code.  The operation failed; the display remains
|||	        unchanged.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>
|||
|||	  See Also
|||
|||	    OpenItem(), DeactivateProjector(), Projector(@)
|||
**/
Item
SuperActivateProjector (pi)
Item	pi;
{
	Projector	*p;
	Item		err;

	if (!(p = CheckItem (pi, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
		return (GFX_ERR_BADITEM);

	if ((err = IsItemOpened (CURRENTTASKITEM, pi)) < 0)
		return (err);

	if (!p->p_Lamp)
		err = GFX_ERR_INTERNAL;
	else
		err = (p->p_Lamp) (p, LAMPOP_ACTIVATE);

	return (err);
}


/**
|||	AUTODOC -class graphicsfolio -name DeactivateProjector
|||	Deactivate a Projector, making it no longer visible.
|||
|||	  Synopsis
|||
|||	    Err DeactivateProjector (Item projectorItem)
|||
|||	  Description
|||
|||	    This routine deactivates the specified Projector.  All Views
|||	    posted to the Projector will no longer be visible on the
|||	    display.  The Projector Item being deactivated must be opened
|||	    beforehand by the calling task using OpenItem().
|||
|||	    Deactivating a Projector does not cause another one to be
|||	    activated.
|||
|||	    It is safe to call DeactivateProjector() on a Projector that is
|||	    already inactive.
|||
|||	  Arguments
|||
|||	    projectorItem
|||	        Item number of the Projector to be deactivated.
|||
|||	  Return Value
|||
|||	    Returns zero on successful completion, or a (negative) error
|||	    code.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>
|||
|||	  See Also
|||
|||	    OpenItem(), ActivateProjector(), Projector(@)
|||
**/
Err
SuperDeactivateProjector (pi)
Item	pi;
{
	Projector	*p;
	Err		err;

	if (!(p = CheckItem (pi, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
		return (GFX_ERR_BADITEM);

	if ((err = IsItemOpened (CURRENTTASKITEM, pi)) < 0)
		return (err);

	if (!p->p_Lamp)
		return (GFX_ERR_INTERNAL);

	return ((p->p_Lamp) (p, LAMPOP_DEACTIVATE));
}


/**
|||	AUTODOC -private -class graphicsfolio -name SetDefaultProjector
|||	Set a Projector to be the default.
|||
|||	  Synopsis
|||
|||	    Err SetDefaultProjector (Item projectorItem)
|||
|||	  Description
|||
|||	    This function changes the folio's default Projector (the one
|||	    used by the folio when none is explicitly specified).  If
|||	    successful, the Item number is returned to you.  Otherwise, a
|||	    negative error code results and nothing is changed.
|||
|||	    The new Projector is OpenItem()ed, then the old one is
|||	    CloseItem()ed.  If the old Projector's open count goes to zero,
|||	    then it is deactivated in favor of the new one, and the display
|||	    will change to the new Projector mode.
|||
|||	  Caveats
|||
|||	    Switching default Projectors while other tasks have Views posted
|||	    to the old default will not harm anything, but it may make it
|||	    difficult for the user to see the old Views (since posting Views
|||	    doesn't bump the open count).
|||
|||	  Warning
|||
|||	    This is a global system-wide setting.  This function exists for
|||	    establishing a user's preferences, and should NOT be used by
|||	    general applications.
|||
|||	  Arguments
|||
|||	    projectorItem
|||	        Item number of the Projector to make the default.
|||
|||	  Return Value
|||
|||	    Returns the Item number you passed in upon successful
|||	    completion, or a (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/projector.h>
|||
|||	  See Also
|||
|||	    Projector(@)
|||
**/
Err
SuperSetDefaultProjector (pi)
Item	pi;
{
	Projector	*p;
	Err		err;
	uint8		oldpriv;
	static TagArg	activatetags[] = {
		PROJTAG_ACTIVATE, (void *) TRUE,
		TAG_END, 0
	};

	if (!(p = CheckItem (pi, NST_GRAPHICS, GFX_PROJECTOR_NODE)))
		return (GFX_ERR_BADITEM);

	if (!(p->p_Flags & PROJF_ACTIVE))
		return (GFX_ERR_PROJINACTIVE);

	if (p == GBASE(gb_DefaultProjector))
		/*  This already is the default.  */
		return (pi);

	/*
	 * Checks complete.  Open new Projector and activate it.  If that
	 * works, close the old one.
	 */
	oldpriv = PromotePriv (CURRENTTASK);

	if ((err = OpenItemAsTask
		    (pi, activatetags, KB_FIELD(kb_OperatorTask))) >= 0)
	{
		CloseItemAsTask (GBASE(gb_DefaultProjector)->p.n_Item,
				 KB_FIELD(kb_OperatorTask));
		/*  Return value ignored; not much I could do with it.  */

		GBASE(gb_DefaultProjector) = p;
	}

	DemotePriv (CURRENTTASK, oldpriv);

	return (err);
}
