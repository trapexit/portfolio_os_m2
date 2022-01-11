/*  :ts=8 bk=0
 *
 * bitmap.c:	Bitmap Item management.
 *
 * @(#) bitmap.c 96/09/17 1.15
 *
 * Leo L. Schwab					9504.13
 */
#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/mem.h>

#include <graphics/gfx_pvt.h>
#include <graphics/bitmap.h>

#include <string.h>

#include "protos.h"


/***************************************************************************
 * Local #defines
 */
#define	CBF_CALCBUFSIZE		1
#define	CBF_VALIDATEBUF		(1<<1)
#define	CBF_CHKCLIP		(1<<2)
#define	CBF_CALCORIGINPTR	(1<<3)
#define	CBF_RESETCLIP		(1<<4)

/*
 * These are the TE and MPEG rendering limits.
 * (The MPEG limits should probably be pulled from SysInfo() or something.)
 */
#define	MIN_XCLIP		1
#define	MAX_XCLIP		0x800
#define	MIN_YCLIP		1
#define	MAX_YCLIP		0x800

#define	MAXWIDE_MPEG		0x800
#define	MAXHIGH_MPEG		0x800


/***************************************************************************
 * Local prototypes.
 */
static Err makedisplayable (struct Bitmap *bm, uint32 *flags);
static Err makerenderable (struct Bitmap *bm, uint32 *flags);
static Err makempegable (struct Bitmap *bm, uint32 *flags);
static void coalign (struct Bitmap *bm);


/***************************************************************************
 * Local gloabls.  (huh?)
 * Maybe I should go for a structure on this Bitmap stuff, huh?
 */
extern BMTypeInfo		bmtypeinfos[];


static Bitmap	default_bitmap = {
	{ NULL },	/*  bm			*/
	320,		/*  bm_Width		*/
	240,		/*  bm_Height		*/
	0,		/*  bm_XOrigin		*/
	0,		/*  bm_YOrigin		*/
	320,		/*  bm_ClipWidth	*/
	240,		/*  bm_ClipHeight	*/
	320 * 240 * 2,	/*  bm_BufferSize	*/
	MEMTYPE_NORMAL,	/*  bm_BufMemType	*/
	0,		/*  bm_BufMemCareBits	*/
	0,		/*  bm_BufMemStateBits	*/
	NULL,		/*  *bm_Buffer		*/
	BMTYPE_16,	/*  bm_Type		*/
	0,		/*  bm_Flags		*/
	NULL,		/*  *bm_OriginPtr	*/
	&bmtypeinfos[BMTYPE_16],	/*  *bm_TypeInfo	*/
	NULL		/*  *bm_CoAlign		*/
};


/***************************************************************************
 * Autodoc for Bitmap Item.
 */
/**
|||	AUTODOC -public -class Items -name Bitmap
|||	An Item describing an image buffer in RAM.
|||
|||	  Description
|||
|||	    This Item represents a region of memory containing imagery of
|||	    specific dimensions and format.  It is used for rendering and
|||	    display operations.
|||
|||	  Tag Arguments
|||
|||	    BMTAG_WIDTH (uint32)
|||	        Width of the Bitmap, in pixels.  Note that the system may
|||	        modify this value, depending on Tag settings below.
|||
|||	    BMTAG_HEIGHT (uint32)
|||	        Height of the Bitmap, in pixels.  Note that the system may
|||	        modify this value, depending on Tag settings below.
|||
|||	    BMTAG_TYPE (uint32)
|||	        Type of Bitmap desired.  See below for a description of
|||	        available Bitmap types.
|||
|||	    BMTAG_CLIPWIDTH (uint32)
|||	        Horizontal width of clip region, in pixels.
|||
|||	    BMTAG_CLIPHEIGHT (uint32)
|||	        Vertical height of clip region, in pixels.
|||
|||	    BMTAG_XORIGIN (uint32)
|||	        Horizontal rendering and clip region offset, in pixels.
|||
|||	    BMTAG_YORIGIN (uint32)
|||	        Vertical rendering and clip region offset, in pixels.
|||
|||	    BMTAG_BUFFER (void *)
|||	        Pointer to beginning (upper-left pixel) of image buffer in
|||	        RAM.
|||
|||	    BMTAG_DISPLAYABLE (Boolean)
|||	        If true (non-zero), specifies that the Bitmap is intended
|||	        for display.  The system will perform more rigorous checks
|||	        to assure the hardware can display the buffer described by
|||	        the Tag arguments.
|||
|||	    BMTAG_RENDERABLE (Boolean)
|||	        If true (non-zero), specifies that the Bitmap is intended to
|||	        receive triangle engine rendering output.  The system will
|||	        perform more rigorous checks to assure the hardware can
|||	        render into the buffer described by the Tag arguments.
|||
|||	    BMTAG_MPEGABLE (Boolean)
|||	        If true (non-zero), specifies that the Bitmap is intended to
|||	        receive the output of an MPEG decompression operation.  The
|||	        system will perform more rigorous checks to assure the MPEG
|||	        hardware can decompress into the buffer described by the Tag
|||	        arguments.
|||
|||	    BMTAG_BUMPDIMS (Boolean)
|||	        Depending on the intended use, the hardware requires the
|||	        Bitmap to be of specific dimensions.  If the supplied
|||	        dimensions are unsupported by the hardware, and the argument
|||	        to this Tag is true (non-zero), the system will bump the
|||	        width and height to the next highest values that the
|||	        hardware can handle (each dimension is considered
|||	        independently).  This assures that your Bitmap will be at
|||	        least as large as you request.  The updated dimensions will
|||	        be written to the Bitmap Item.
|||
|||	    BMTAG_COALIGN (Item)
|||	        While the hardware can operate on image- and Z-buffers
|||	        arbitrarily aligned, greatest performance is realized when
|||	        the image- and Z-buffers start on "alternate" 4K pages (e.g.
|||	        the image-buffer starts on an even-numbered page and the
|||	        Z-buffer on an odd-numbered page).  If the argument to this
|||	        Tag is a valid Bitmap Item, and that Bitmap has a buffer
|||	        attached, the bm_BufMemCareBits and bm_BufMemStateBits
|||	        fields will be set to indicate the state the bits in the
|||	        buffer pointer (passed in with BMTAG_BUFFER) must have to
|||	        achieve optimum alignment with the other Bitmap (the
|||	        argument to this Tag).  A block of memory satisfying these
|||	        requirements may be procured from AllocMemMasked().
|||	        Co-alignment "phase" reported by the system through these
|||	        fields varies depending on the Bitmaps being co-aligned.  If
|||	        both Bitmaps are Z-buffers, the buffers will be "in phase"
|||	        (e.g. both buffers will start in even-numbered pages).  If
|||	        neither Bitmap is a Z-buffer, the buffers will also be in
|||	        phase.  If one is a Z-buffer and the other is not, then the
|||	        buffers will be "out of phase" (e.g. one buffer will start
|||	        in an odd-numbered page and the other in an even-numbered
|||	        page).  Read the folio chapter for a more detailed
|||	        explanation.
|||
|||	    BMTAG_RESETCLIP (Boolean)
|||	        If this Tag is true (non-zero), the clip region boundaries
|||	        are reset to the full dimensions of the Bitmap.
|||
|||	    The following Bitmap types are available:
|||
|||	    BMTYPE_16
|||	        Buffer is 16 bits per pixel (15 RGB, 1 DSB).
|||
|||	    BMTYPE_32
|||	        Buffer is 32 bits per pixel (24 RGB, 7 alpha, 1 DSB).
|||
|||	    BMTYPE_16_ZBUFFER
|||	        Buffer is 16 bits per pixel, intended as a Z-buffer for
|||	        rendering operations.
|||
|||	  Folio
|||
|||	    graphics
|||
|||	  Item Type
|||
|||	    GFX_BITMAP_NODE
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
|||	    PixelAddress()
|||
|||	  Associated Files
|||
|||	    <graphics/bitmap.h>
|||
|||	  See Also
|||
|||	    CreateItem()
|||
**/


/***************************************************************************
 * Encryp^H^H^H^H^H^HCiphe^H^H^H^H^H^HCode.
 * (Maybe I need some time off...)
 ****
 * For those of you wondering why this code is so goofy (what with copying
 * into a working buffer and all), it's so that Bitmap Items won't be
 * destroyed as a result of invalid parameters or other
 * creation/modification errors. The intended result is that a Bitmap Item
 * will always be consistent and valid no matter what happens.
 */
static Err
icb (
struct Bitmap	*bm,
uint32		*flags,
uint32		t,
uint32		a
)
{
	register uint32	f;

	f = *flags;

	DDEBUG (("Enter icb %lx %lx\n", t, a));
	switch (t) {
	case BMTAG_WIDTH:
		bm->bm_Width = a;
		f |= CBF_CALCBUFSIZE | CBF_CALCORIGINPTR | CBF_VALIDATEBUF |
		     CBF_CHKCLIP;
		break;
	case BMTAG_HEIGHT:
		bm->bm_Height = a;
		f |= CBF_CALCBUFSIZE | CBF_CALCORIGINPTR | CBF_VALIDATEBUF |
		     CBF_CHKCLIP;
		break;
	case BMTAG_TYPE:
		if (a < 0  ||  a >= MAX_BMTYPE)
			return (GFX_ERR_BADBITMAPTYPE);
		bm->bm_Type = a;
		bm->bm_TypeInfo = &bmtypeinfos[a];
		f |= CBF_CALCBUFSIZE | CBF_CALCORIGINPTR | CBF_VALIDATEBUF |
		     CBF_CHKCLIP;
		break;
	case BMTAG_CLIPWIDTH:
		bm->bm_ClipWidth = a;
		f |= CBF_CHKCLIP;
		f &= ~CBF_RESETCLIP;
		break;
	case BMTAG_CLIPHEIGHT:
		bm->bm_ClipHeight = a;
		f |= CBF_CHKCLIP;
		f &= ~CBF_RESETCLIP;
		break;
	case BMTAG_XORIGIN:
		bm->bm_XOrigin = a;
		f |= CBF_CHKCLIP | CBF_CALCORIGINPTR;
		f &= ~CBF_RESETCLIP;
		break;
	case BMTAG_YORIGIN:
		bm->bm_YOrigin = a;
		f |= CBF_CHKCLIP | CBF_CALCORIGINPTR;
		f &= ~CBF_RESETCLIP;
		break;
	case BMTAG_BUFFER:
		bm->bm_Buffer = (void *) a;
		f |= CBF_VALIDATEBUF | CBF_CALCORIGINPTR;
		break;
	case BMTAG_DISPLAYABLE:
		if (a) {
			bm->bm_Flags |= BMF_DISPLAYABLE;
			f |= CBF_VALIDATEBUF;
		} else
			bm->bm_Flags &= ~BMF_DISPLAYABLE;
		break;
	case BMTAG_RENDERABLE:
		if (a) {
			bm->bm_Flags |= BMF_RENDERABLE;
			f |= CBF_VALIDATEBUF;
		} else
			bm->bm_Flags &= ~BMF_RENDERABLE;
		break;
	case BMTAG_MPEGABLE:
		if (a) {
			bm->bm_Flags |= BMF_MPEGABLE;
			f |= CBF_VALIDATEBUF;
		} else
			bm->bm_Flags &= ~BMF_MPEGABLE;
		break;
	case BMTAG_BUMPDIMS:
		if (a)
			bm->bm_Flags |= BMF_BUMPDIMS;
		else
			bm->bm_Flags &= ~BMF_BUMPDIMS;
		break;
	case BMTAG_COALIGN:
		if (!(bm->bm_CoAlign =
		       CheckItem ((Item) a, NST_GRAPHICS, GFX_BITMAP_NODE)))
			if (a)
				return (GFX_ERR_BADBITMAP);
		break;
	case BMTAG_RESETCLIP:
		if (a)
			f |= CBF_RESETCLIP | CBF_CALCORIGINPTR;
		break;
	default:
		return (GFX_ERR_BADTAGARG);
	}

	*flags = f;
	return (0);
}


Err
modifybitmap (
struct Bitmap	*clientbm,
struct TagArg	*args
)
{
	Bitmap		bm;
	Err		err;
	uint32		flags;

	memcpy (&bm, clientbm, sizeof (Bitmap));

	/*
	 * This is kinda sleazy, but I really like auto-adjusting clips.
	 */
	if (bm.bm_XOrigin == 0  &&  bm.bm_ClipWidth == bm.bm_Width  &&
	    bm.bm_YOrigin == 0  &&  bm.bm_ClipHeight == bm.bm_Height)
		flags = CBF_RESETCLIP;
	else
		flags = 0;

/*printf ("CBM TagProcessor; clientbm = 0x%08lx\n", clientbm);*/
	if ((err = TagProcessor (&bm, args, icb, &flags)) < 0)
		return (err);

/*printf ("RESETCLIP?\n");*/
	if (flags & CBF_RESETCLIP) {
		bm.bm_XOrigin	=
		bm.bm_YOrigin	= 0;
		bm.bm_ClipWidth	= bm.bm_Width;
		bm.bm_ClipHeight= bm.bm_Height;
		flags &= ~CBF_CHKCLIP;
	}

	/*
	 * Make sure the client wasn't insane.
	 */
	if (((~bm.bm_TypeInfo->bti_Flags) & BMF_ABLEMASK) & bm.bm_Flags)
		return (GFX_ERR_BITMAPUNABLE);

	bm.bm_BufMemCareBits  =
	bm.bm_BufMemStateBits = 0;

	if (bm.bm_CoAlign)
		coalign (&bm);

	if (bm.bm_Flags & BMF_DISPLAYABLE)
		if ((err = makedisplayable (&bm, &flags)) < 0)
			return (err);

	if (bm.bm_Flags & BMF_RENDERABLE)
		if ((err = makerenderable (&bm, &flags)) < 0)
			return (err);

	if (bm.bm_Flags & BMF_MPEGABLE)
		if ((err = makempegable (&bm, &flags)) < 0)
			return (err);


/*printf ("CHKCLIP?\n");*/
	if (flags & CBF_CHKCLIP)
		if (bm.bm_XOrigin < 0  ||
		    bm.bm_YOrigin < 0  ||
		    bm.bm_XOrigin + bm.bm_ClipWidth > bm.bm_Width  ||
		    bm.bm_YOrigin + bm.bm_ClipHeight > bm.bm_Height)
			return (GFX_ERR_BADCLIP);

/*printf ("CALCORIGINPTR?\n");*/
	if ((flags & CBF_CALCORIGINPTR)  &&  bm.bm_Buffer)
		if (!(bm.bm_OriginPtr = InternalPixelAddr (&bm,
							   bm.bm_XOrigin,
							   bm.bm_YOrigin)))
			return (GFX_ERR_BADCLIP);

/*printf ("CALCBUFSIZE?\n");*/
	if (flags & CBF_CALCBUFSIZE)
		/* ### consider BMTYPE_INVALID */
		if ((err = (bm.bm_TypeInfo->bti_SizeFunc) (&bm)) < 0)
			return (err);

/*printf ("VALIDATEBUF?\n");*/
	if ((flags & CBF_VALIDATEBUF)  &&  bm.bm_Buffer) {
		if (bm.bm_Flags & (BMF_MPEGABLE | BMF_RENDERABLE)) {
			/*
			 * Make sure the buffer is writeable by the client.
			 */
			if (!IsMemWritable (bm.bm_Buffer, bm.bm_BufferSize))
				return (GFX_ERR_BADPTR);

		} else if (bm.bm_Flags & BMF_DISPLAYABLE) {
			/*
			 * Just make sure the memory actually exists.
			 */
			if (!IsMemReadable (bm.bm_Buffer, bm.bm_BufferSize))
				return (GFX_ERR_BADPTR);
		}
	}


/*printf ("memcpy(); clientbm = 0x%08lx\n", clientbm);*/
	memcpy (clientbm, &bm, sizeof (Bitmap));

/*printf ("Outta here.\n");*/
	return (bm.bm.n_Item);
}


Item
createbitmap (
struct Bitmap	*bm,
struct TagArg	*ta
)
{
	Err	err;

	memcpy (((ItemNode *) bm) + 1, ((ItemNode *) &default_bitmap) + 1,
		sizeof (Bitmap) - sizeof (ItemNode));

	if ((err = modifybitmap (bm, ta)) >= 0)
		AddHead (&GBASE(gb_BitmapList), (Node *) bm);
	return (err);
}


Err
deletebitmap (
struct Bitmap	*bm
)
{
	/*
	 * ### It's still more complicated than this!!  Is a View using it?
	 * ### MemLocks?  MPEG?  And what about...  Naomi?
	 */
	if (bm->bm_Flags & BMF_RENDERABLE)
		AbortIOReqsUsingItem (bm->bm.n_Item, bm->bm.n_Type);

	RemNode ((Node *) bm);

	return (0);
}


Item
findbitmap (
struct TagArg	*ta
)
{
	TOUCH (ta);
	return (GFX_ERR_NOTSUPPORTED);
}


Item
openbitmap (
struct Bitmap	*bm,
struct TagArg	*ta
)
{
	TOUCH (bm);
	TOUCH (ta);
	return (GFX_ERR_NOTSUPPORTED);
}


Err
closebitmap (
struct Bitmap	*bm,
struct Task	*t
)
{
	TOUCH (bm);
	TOUCH (t);
	return (GFX_ERR_NOTSUPPORTED);
}





/***************************************************************************
 * Bitmap invalidator for the TEGraphics folio.
 * It is presumed that the client making use of this call understands under
 * what conditions invalidation is The Thing To Do.
 */
Err
InternalInvalidateBitmap (
struct Bitmap	*bm
)
{
	bm->bm_Buffer = NULL;
	return (0);
}

/***************************************************************************
 * Buffer size calculators.
 */
Err
bmsize (
struct Bitmap *bm
)
{
	bm->bm_BufferSize = bm->bm_Width * bm->bm_Height *
			    bm->bm_TypeInfo->bti_PixelSize;
	bm->bm_BufMemType = MEMTYPE_NORMAL;

	return (0);
}

Err
lrformsize (
struct Bitmap *bm
)
{
	bm->bm_BufferSize = bm->bm_Width * ((bm->bm_Height + 1) >> 1) *
			    sizeof (uint32);
	bm->bm_BufMemType = MEMTYPE_NORMAL;

	return (0);
}


/***************************************************************************
 * Extra Bitmap validation for certain conditions.
 */
static Err
makedisplayable (
struct Bitmap	*bm,
uint32		*flags
)
{
	uint32	wide, quantum;

	/*
	 * Make sure base address is aligned for hardware.
	 */
	if (bm->bm_BufMemCareBits < sizeof (uint32) - 1) {
		bm->bm_BufMemCareBits = sizeof (uint32) - 1;
		bm->bm_BufMemStateBits = 0;
	}

	if ((uint32) bm->bm_Buffer & (sizeof (uint32) - 1))
		return (GFX_ERR_BADPTR);

	/*
	 * VDL modulos are expressed in multiples of 32 bytes.
	 * Determine the number of pixels this represents, then make
	 * sure the width conforms to this quantization.
	 * (If quantum doesn't compute to a power of two, I'm screwed.)
	 */
	quantum = 32 / bm->bm_TypeInfo->bti_PixelSize;
	wide = bm->bm_Width;
	if (wide & (quantum - 1))
		if (bm->bm_Flags & BMF_BUMPDIMS) {
			wide = (wide + quantum) & ~(quantum - 1);
			bm->bm_Width = wide;
			*flags |= CBF_CALCBUFSIZE | CBF_VALIDATEBUF;
		} else
			return (GFX_ERR_BADDIMS);

	/*
	 * Confirm width can be handled by the hardware.
	 * (And just what is the minimum width, anyway?)
	 */
	if (wide < quantum  ||
	    wide > (VDL_DMA_MOD_MASK >> VDL_DMA_MOD_SHIFT << 5) + 1)
		return (GFX_ERR_BADDIMS);

	return (0);
}


static Err
makerenderable (
struct Bitmap	*bm,
uint32		*flags
)
{
	uint32	wide, quantum;

	/*
	 * Make sure base address is aligned for hardware.
	 */
	if (bm->bm_BufMemCareBits < sizeof (uint32) - 1) {
		bm->bm_BufMemCareBits = sizeof (uint32) - 1;
		bm->bm_BufMemStateBits = 0;
	}

	if ((uint32) bm->bm_Buffer & (sizeof (uint32) - 1))
		return (GFX_ERR_BADPTR);

	/*
	 * Triangle engine modulos are expressed in whole 32-bit words.
	 * Determine the number of pixels this represents, then make
	 * sure the width conforms to this quantization.
	 * (Gee, this code looks awful similar to the display quantizer...)
	 */
	quantum = sizeof (uint32) / bm->bm_TypeInfo->bti_PixelSize;
	wide = bm->bm_Width;
	if (wide & (quantum - 1))
		if (bm->bm_Flags & BMF_BUMPDIMS) {
			wide = (wide + quantum) & ~(quantum - 1);
			bm->bm_Width = wide;
			*flags |= CBF_CALCBUFSIZE | CBF_VALIDATEBUF;
		} else
			return (GFX_ERR_BADDIMS);

	/*
	 * Check clip values against hardware restrictions.
	 */
	if (bm->bm_ClipWidth < MIN_XCLIP  ||
	    bm->bm_ClipWidth > MAX_XCLIP  ||
	    bm->bm_ClipHeight < MIN_YCLIP  ||
	    bm->bm_ClipHeight > MAX_YCLIP)
		return (GFX_ERR_BADCLIP);

	return (0);
}


static Err
makempegable (
struct Bitmap	*bm,
uint32		*flags
)
{
	uint32	val;

	/*
	 * Make sure base address is aligned for hardware (32-byte boundary).
	 * ### Get this from SysInfo().
	 */
	if (bm->bm_BufMemCareBits < 32 - 1) {
		bm->bm_BufMemCareBits = 32 - 1;
		bm->bm_BufMemStateBits = 0;
	}

	if ((uint32) bm->bm_Buffer & (32 - 1))
		return (GFX_ERR_BADPTR);

	/*
	 * MPEG requires an even multiple of 16 pixels in both dimensions.
	 * ### Quantum must be fetched from SysInfo().
	 */
	val = bm->bm_Width;
	if (val & 0xF)
		if (bm->bm_Flags & BMF_BUMPDIMS) {
			bm->bm_Width = (val + 0xF) & ~0xF;
			*flags |= CBF_CALCBUFSIZE | CBF_VALIDATEBUF;
		} else
			return (GFX_ERR_BADDIMS);

	val = bm->bm_Height;
	if (val & 0xF)
		if (bm->bm_Flags & BMF_BUMPDIMS) {
			bm->bm_Width = (val + 0xF) & ~0xF;
			*flags |= CBF_CALCBUFSIZE | CBF_VALIDATEBUF;
		} else
			return (GFX_ERR_BADDIMS);

	/*
	 * Check clip values against hardware restrictions.
	 */
	if (bm->bm_Width < 16  ||
	    bm->bm_Width > MAXWIDE_MPEG  ||
	    bm->bm_Height < 16  ||
	    bm->bm_Height > MAXHIGH_MPEG)
		return (GFX_ERR_BADDIMS);

	return (0);
}


/*
 * Get these from the system somewhere...
 * Lots of twos-complement assumptions here.
 */
#define	PAGE_SIZE	4096
#define	PAGE_MASK	(PAGE_SIZE - 1)
#define	CARE_MASK	((PAGE_MASK << 1) + 1)

static void
coalign (
struct Bitmap	*bm
)
{
	uint32	buf;

	if (buf = (uint32) bm->bm_CoAlign->bm_Buffer) {
		buf &= CARE_MASK;

		if (bm->bm_Type == BMTYPE_16_ZBUFFER)
			/*
			 * Flip sense of page bit so it ends up on
			 * alternate bank.
			 */
			buf ^= PAGE_SIZE;

		bm->bm_BufMemCareBits = CARE_MASK;
		bm->bm_BufMemStateBits = buf;
	}
}


/***************************************************************************
 * Pixel address computation.
 */
/**
|||	AUTODOC -public -class graphicsfolio -name PixelAddress
|||	Return the address of a given pixel.
|||
|||	  Synopsis
|||
|||	    void *PixelAddress (Item bitmapItem, uint32 x, uint32 y)
|||
|||	  Description
|||
|||	    Returns a pointer to the pixel in the specified Bitmap located
|||	    at the supplied coordinates.  The coordinates are specified in
|||	    pixels.
|||
|||	    The returned pointer may be of varying alignments depending on
|||	    the Bitmap's type and characteristics, so try not to assume too
|||	    much about the returned pointer's characteristics, or what it
|||	    points to.
|||
|||	  Arguments
|||
|||	    bitmapItem
|||	        Item number of the Bitmap.
|||
|||	    x
|||	        X-coordinate of the pixel of interest.
|||
|||	    y
|||	        Y-coordinate of the pixel of interest.
|||
|||	  Return Value
|||
|||	    The address of the pixel at the specified coordinates, or NULL
|||	    if an error occurred.
|||
|||	  Caveats
|||
|||	    The coordinates passed to this routine are NOT affected by any
|||	    prevailing settings in the Bitmap's bm_XOrigin and bm_YOrigin
|||	    fields, and are always reckoned from the absolute upper-left
|||	    corner of the buffer.
|||
|||	  Associated Files
|||
|||	    <graphics/bitmap.h>
|||
**/

void *
PixelAddress (Item bmitem, uint32 x, uint32 y)
{
	Bitmap	*bm;

	if (!(bm = CheckItem (bmitem, NST_GRAPHICS, GFX_BITMAP_NODE)))
		return (NULL);

	if (x >= bm->bm_Width  ||  y >= bm->bm_Height)
		return (NULL);

	if (!bm->bm_Buffer  ||  bm->bm_Type == BMTYPE_INVALID)
		return (NULL);

	return ((bm->bm_TypeInfo->bti_PxAddrFunc) (bm, x, y));
}


/**
|||	AUTODOC -private -class graphicsfolio -name InternalPixelAddr
|||	Return the address of a given pixel.
|||
|||	  Synopsis
|||
|||	    void *InternalPixelAddr (struct Bitmap *bm, uint32 x, uint32 y)
|||
|||	  Description
|||
|||	    Exactly like the public PixelAddress() function, except that you
|||	    pass in a pointer to a Bitmap structure rather than a Bitmap
|||	    Item.
|||
|||	  Arguments
|||
|||	    bm
|||	        Pointer to the Bitmap.
|||
|||	    x
|||	        X-coordinate of the pixel of interest.
|||
|||	    y
|||	        Y-coordinate of the pixel of interest.
|||
|||	  Return Value
|||
|||	    The address of the pixel at the specified coordinates, or NULL
|||	    if an error occurred.
|||
|||	  Associated Files
|||
|||	    <graphics/bitmap.h>, <graphics/gfx_pvt.h>
|||
|||	  See Also
|||
|||	    PixelAddress()
|||
**/

void *
InternalPixelAddr (bm, x, y)
struct Bitmap	*bm;
uint32		x, y;
{
	return ((bm->bm_TypeInfo->bti_PxAddrFunc) (bm, x, y));
}



void *
simple_addr (bm, x, y)
struct Bitmap	*bm;
uint32		x, y;
{
	uint32	baseaddr, pixsiz;

	baseaddr = (uint32) bm->bm_Buffer;
	pixsiz = bm->bm_TypeInfo->bti_PixelSize;

	return ((void *) (baseaddr + (y * bm->bm_Width + x) * pixsiz));
}

void *
lrform_addr (bm, x, y)
struct Bitmap	*bm;
uint32		x, y;
{
	register uint32	baseaddr, addr;
	register int32	linewidth;

	linewidth = bm->bm_Width;
	baseaddr = addr = (uint32) bm->bm_Buffer;
	addr += (y >> 1) * linewidth * sizeof (uint32);
	if (y & 1) {
		if ((uint32) baseaddr & 2)
			addr += linewidth * sizeof (uint32) - sizeof (uint16);
		else
			addr += sizeof (uint16);
	}
	addr += x * sizeof (uint32);

	return ((void *) addr);
}


/***************************************************************************
 * The MemLock handler.
 */
void
memlockhandler (
struct Task	*t,
const void	*addr,
uint32		length
)
{
	Bitmap	*bm;
	uint32	low, high, buf;

	TOUCH (t);
	low = (uint32) addr;
	high = low + length;

	for (bm = (Bitmap *) FIRSTNODE (&GBASE(gb_BitmapList));
	     NEXTNODE (bm);
	     bm = (Bitmap *) NEXTNODE (bm))
	{
		if (buf = (uint32) bm->bm_Buffer)
			if (buf < high  &&  buf + bm->bm_BufferSize > low) {
				if (bm->bm_Flags & BMF_RENDERABLE)
					AbortIOReqsUsingItem (bm->bm.n_Item,
							      bm->bm.n_Type);
				if (bm->bm_Flags &
				    (BMF_RENDERABLE | BMF_MPEGABLE))
					bm->bm_Buffer = NULL;
			}
	}
}
