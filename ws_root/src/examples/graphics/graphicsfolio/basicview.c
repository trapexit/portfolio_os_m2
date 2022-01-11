
/******************************************************************************
**
**  @(#) basicview.c 96/06/14 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class Examples -group Graphics -name basicview
|||	Illustrates how to create a basic View.
|||
|||	  Synopsis
|||
|||	    basicview
|||
|||	  Description
|||
|||	    This program demonstrates the creation of a Bitmap, its buffer,
|||	    and a View.  The program places the View on the display, fills
|||	    it with blue, sleeps for about five seconds, then tears
|||	    everything down and exits.
|||
|||	    For the View creation, this example leverages off the system
|||	    default View settings, based on the Bitmap supplied.  These
|||	    defaults may be escaped by supplying additional TagArgs
|||	    specifying the desired characteristics.
|||
|||	  Associated Files
|||
|||	    basicview.c
|||
|||	  Location
|||
|||	    examples/graphics/graphicsfolio
|||
|||	  See Also
|||
|||	    autosizeview(@)
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <stdio.h>
#include <stdlib.h>


/***************************************************************************
 * Prototypes.
 */
int main(int ac, char **av);
Item allocbitmap(int32 wide, int32 high, int32 type);
void fillrast16(struct Bitmap *bm, uint32 r, uint32 g, uint32 b);
void openstuff(void);
void closestuff(void);
void die(Err err, char *str);


/***************************************************************************
 * Global variables.
 */
Item	timerio;


/***************************************************************************
 * Code.
 */
int
main (ac, av)
int	ac;
char	**av;
{
	Bitmap	*bm;
	Item	vi, bmi;

	TOUCH (ac);
	TOUCH (av);

	/*
	 * Open some rudimentary stuff.
	 */
	openstuff ();

	/*
	 * Create the Bitmap Item that will be displayed.
	 */
	bmi = allocbitmap (320, 240, BMTYPE_16);
	bm = LookupItem (bmi);

	/*
	 * Create the View through which the Bitmap will be made visible.
	 * Because we haven't explicitly specified a View type, a default
	 * type will be assigned (320-wide).
	 */
	if ((vi = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_BITMAP, bmi,
				TAG_END)) < 0)
		die (vi, "Can't create View.\n");

	/*
	 * Add the View to the display (using the idiomatic value of zero
	 * to place it on the default Projector).
	 */
	AddViewToViewList (vi, 0);

	/*
	 * At this point, the View is now visible.  Typically, you'll begin
	 * performing complex graphics operations yielding stunning titles.
	 * For this example, we're just going to fill the Bitmap with a
	 * solid color to prove it's actually there, then twiddle our thumbs
	 * for a while...
	 */
	fillrast16 (bm, 0, 0, ~0);
	WaitTimeVBL (timerio, 300);

	/*
	 * We could in fact simply exit and let the OS clean up everything
	 * for us.  The following few lines of code illustrate the "proper"
	 * way of cleaning up.
	 ***
	 * Removing the View has the effect of removing it from the display.
	 * Deleting the View through DeleteItem() will have the same effect,
	 * so for this example, the RemoveView() is redundant.
	 */
	RemoveView (vi);
	DeleteItem (vi);

	/*
	 * Free the buffer we allocated for the Bitmap.
	 */
	FreeMem (bm->bm_Buffer, bm->bm_BufferSize);

	/*
	 * Delete the Bitmap Item.  Note that deleting the Bitmap Item does
	 * not free any attendant buffer, which is why it was done separately
	 * above.
	 */
	DeleteItem (bmi);

	printf ("Normal exit.\n");

	closestuff ();
	return (0);
}



Item
allocbitmap (wide, high, type)
int32	wide, high, type;
{
	Bitmap	*bm;
	Item	bmi;
	Err	err;
	void	*buf;

	/*
	 * Create Bitmap Item.
	 * The Bitmap Item structure will be filled in according to our
	 * needs.  We can then inspect the structure to discover memory
	 * requirements, correct widths, etc.
	 */
	bmi = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
			    BMTAG_WIDTH, wide,
			    BMTAG_HEIGHT, high,
			    BMTAG_TYPE, type,
			    BMTAG_DISPLAYABLE, TRUE,
			    BMTAG_RENDERABLE, TRUE,
			    BMTAG_BUMPDIMS, TRUE,
			    TAG_END);

	if (bmi < 0)
		die (bmi, "Bitmap creation failed.\n");
	bm = LookupItem (bmi);

	/*
	 * Create the Bitmap buffer.
	 * This is done most commonly through the kernel call
	 * AllocMemMasked(), which insures that the buffer will be aligned
	 * properly.
	 */
	if (!(buf = AllocMemMasked (bm->bm_BufferSize,
				    bm->bm_BufMemType,
				    bm->bm_BufMemCareBits,
				    bm->bm_BufMemStateBits)))
		die (0, "Can't allocate Bitmap buffer.\n");

	/*
	 * Attach buffer to Bitmap.
	 */
	if ((err = ModifyGraphicsItemVA (bmi,
					 BMTAG_BUFFER, buf,
					 TAG_END)) < 0)
		die (err, "Can't modify Bitmap Item.\n");

	/*
	 * Success.  Return the Bitmap Item number.
	 */
	return (bmi);
}


/***************************************************************************
 * This incredibly lame routine fills a Bitmap with the specified color.
 * The color is specified as three 32-bit unsigned values (overkill).
 * Assumes a 16-bit buffer.
 */
void
fillrast16 (bm, r, g, b)
struct Bitmap   *bm;
uint32          r, g, b;
{
        register int    i;
        int32           size;
        uint32          color, *buf;

        size = bm->bm_BufferSize;
        buf = bm->bm_Buffer;

        color = ((r & 0xF8000000) >> 1)  |
                ((g & 0xF8000000) >> 6)  |
                ((b & 0xF8000000) >> 11);
        color = color | (color >> 16);
        for (i = size / sizeof (uint32);  --i >= 0; )
                *buf++ = color;
}


/***************************************************************************
 * Housekeeping.
 */
void
openstuff ()
{
	Err	err;

	/*
	 * Open the graphics folio.
	 * This step must be done before any other graphics folio function
	 * may be used.
	 */
	if ((err = OpenGraphicsFolio ()) < 0)
		die (err, "Can't open graphics folio.\n");

	/*
	 * What we'll use later to twiddle our thumbs.
	 */
	if ((timerio = CreateTimerIOReq ()) < 0)
		die (timerio, "Can't create timer I/O.\n");
}

void
closestuff ()
{
	if (timerio >= 0)	DeleteTimerIOReq (timerio);

	CloseGraphicsFolio ();
}

void
die (err, str)
Err	err;
char	*str;
{
	if (err < 0)
		PrintfSysErr (err);
	printf ("%s", str);
	closestuff ();
	exit (20);
}
