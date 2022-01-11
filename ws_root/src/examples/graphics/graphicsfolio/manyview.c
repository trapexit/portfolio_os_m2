/***************************************************************************
**
**  @(#) manyview.c 96/06/14 1.1
**
****************************************************************************/

/**
|||	AUTODOC -public -class Examples -group Graphics -name manyview
|||	Illustrates how to create and manipulate multiple Views.
|||
|||	  Synopsis
|||
|||	    manyview
|||
|||	  Description
|||
|||	    This program creates a number of Bitmaps, Views, and a ViewList.
|||	    It attaches them to the default Projector, and then slides them
|||	    around.  It illustrates the use of ModifyGraphicsItem() for
|||	    moving Views and changing their pixel averaging modes, as well
|||	    as the use of LockDisplay() and UnlockDisplay() for optimizing
|||	    VDL recompiles.
|||
|||	    Two of the Views display the same Bitmap, but in different ways:
|||	    One displays a small portion of it as low-res (320*240), the
|||	    other shows the entire Bitmap as hi-res (640*480).  The low-res
|||	    "window" pans around the Bitmap; as it does so, a bounding box
|||	    is drawn in the Bitmap (visible in the hi-res View) showing
|||	    which part of the Bitmap is being shown through the lo-res View.
|||
|||	  User Control
|||
|||	    The buttons on the control pad have the following functions:
|||
|||	    A Button
|||	        Cycles the rearmost full-screen View through the four pixel
|||	        averaging modes:  none, horizontal, vertical, and horizontal
|||	        and vertical simultaneously.
|||
|||	    Play/Pause
|||	        Freeze/unfreeze the movement of the Views.
|||
|||	    Stop
|||	        Quits the program.
|||
|||	  Associated Files
|||
|||	    manyview.c
|||
|||	  Location
|||
|||	    examples/graphics/graphicsfolio
|||
|||	  See Also
|||
|||	    basicview(@), autosizeview(@)
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernelnodes.h>
#include <kernel/mem.h>
#include <kernel/time.h>
#include <misc/event.h>

#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <graphics/projector.h>

#include <stdio.h>
#include <stdlib.h>


/***************************************************************************
 * #defines
 */
/*
 * This example *just happens* to know that the display coordinate
 * resolution is 640 * 480.  This obviously becomes bogus once you move to
 * PAL.
 *
 * Eventually, there will be a system call that will report the
 * characteristics of the prevailing display mode.  When that happens,
 * these #defines will be removed, and the system-reported values will be
 * used.
 *
 * In other words, don't assume 640 * 480 just because this example did.
 */
#define	DISPWIDE		640
#define	DISPHIGH		480


/***************************************************************************
 * Structure definitions.
 */
/*
 * A remarkably primitive rendering context for the remarkably primitive
 * rendering primitives.
 */
typedef struct RastPort {
	struct View	*rp_View;
	struct Bitmap	*rp_Bitmap;
	Item		rp_ViewItem;
	Item		rp_BitmapItem;
	int32		rp_X, rp_Y;
	uint32		rp_ColorR, rp_ColorG, rp_ColorB;
	uint32		rp_Color32;
	uint16		rp_Color16;
	uint8		rp_DrawMode;
} RastPort;

#define	DRAWMODE_JAM1	0
#define	DRAWMODE_XOR	1


/*
 * A simple structure containing basic information to create Bitmaps and
 * Views.
 */
struct vdef {
	int32	bmwidth, bmheight;
	int32	bmtype;
	int32	vwidth, vheight;
	int32	vtype;
};


/***************************************************************************
 * Prototypes.
 */
/* manyview.c */
int main(int ac, char **av);
void bounceview(struct RastPort *rp, int32 *d);
void bouncesubview(struct RastPort *rp, int32 *d);
void bouncewindow(struct RastPort *rp);
void drawrastport(struct RastPort *rp, uint32 r, uint32 g, uint32 b);
void buildrastports(RastPort *rps, struct vdef *defs, int32 n);
Err initrastport(struct RastPort *rp, Item vi);
void setfgpen(struct RastPort *rp, uint32 r, uint32 g, uint32 b);
void rectfill(struct RastPort *rp, int32 x, int32 y, int32 w, int32 h);
void rf_16(struct RastPort *rp, int32 x, int32 y, int32 w, int32 h);
void rf_32(struct RastPort *rp, int32 x, int32 y, int32 w, int32 h);
void moveto(struct RastPort *rp, int32 x, int32 y);
void drawto(struct RastPort *rp, int32 x, int32 y);
void writepixel(struct RastPort *rp, int32 x, int32 y);
void colorwheel(struct RastPort *rp, uint32 rot);
uint32 getjoybits(long shiftbits);
void openstuff(void);
void closestuff(void);
void die(Err err, char *str);


/***************************************************************************
 * Global variables.
 */
Item		vblIO;

struct vdef	vdefs[] = {
 {
 	-1, -1, BMTYPE_16,
	-1, -1, VIEWTYPE_16
 }, {
	-1, 100, BMTYPE_16,
	-1, 100, VIEWTYPE_16_640_LACE
 }, {
	-1, 150, BMTYPE_16,
 	-1, 150, VIEWTYPE_16_640_LACE
 },
};
#define	NVDEFS		(sizeof (vdefs) / sizeof (struct vdef))

RastPort	rports[NVDEFS + 1];
RastPort	*rpback = &rports[0],
		*rphires = &rports[1],
		*rpm1 = &rports[2],
		*rpm2 = &rports[3];

int32		dispwide, disphigh;


/***************************************************************************
 * Code.
 */
int
main (ac, av)
int	ac;
char	**av;
{
	Item	vi, vli;
	Err	err;
	int32	dh, dl, dm1, dm2;
	int32	vltop;
	int32	avgbits;
	uint32	joybits;

	/*
	 * Initialize some global stuff.
	 */
	TOUCH (ac);	/*  Keep the compiler quiet.  */
	TOUCH (av);
	openstuff ();

	/*
	 * Create the Bitmaps and Views and draw something in them.
	 */
	buildrastports (rports, vdefs, NVDEFS);
	drawrastport (rpback, 0, 0x10000000, ~0);
	drawrastport (rphires, 0, 0x50000000, 0);
	drawrastport (rpm1, 0x60000000, 0, 0x80000000);

	/*
	 * Sneak the display dimensions out of the View's Projector.
	 */
	dispwide = rpback->rp_View->v_Projector->p_Width;
	disphigh = rpback->rp_View->v_Projector->p_Height;

	/*
	 * Create another View based on the Bitmap for an already-created
	 * View.  This new View will be low-res, 240-tall, and show a
	 * sub-window of the underlying Bitmap.
	 *
	 * This View will be a "mirror" of the original View, since it will
	 * be showing the same Bitmap as the original, but in a different
	 * way.
	 */
	if ((vi = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_BITMAP, rpm1->rp_BitmapItem,
				VIEWTAG_VIEWTYPE, VIEWTYPE_16,
				VIEWTAG_WIDTH, dispwide,
				VIEWTAG_HEIGHT, 100,
				TAG_END)) < 0)
		die (vi, "Can't create mirror View.\n");
	initrastport (rpm2, vi);

	/*
	 * Create a ViewList, to which we'll anchor the two mirror Views
	 * (rpm1 and rpm2).
	 */
	if ((vli = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEWLIST_NODE),
				 VIEWTAG_TOPEDGE, 0,
				 TAG_END)) < 0)
		die (vli, "Can't create ViewList.\n");

	/*
	 * Add the mirror Views to our ViewList.
	 */
	if ((err = AddViewToViewList (rpm1->rp_ViewItem, vli)) < 0  ||
	    (err = AddViewToViewList (rpm2->rp_ViewItem, vli)) < 0)
		die (err, "AddView to sublist failed.\n");

	/*
	 * Add the Views and ViewList to the default display.
	 */
	if ((err = AddViewToViewList (rpback->rp_ViewItem, 0)) < 0  ||
	    (err = AddViewToViewList (rphires->rp_ViewItem, 0)) < 0  ||
	    (err = AddViewToViewList (vli, 0)) < 0)
		die (err, "AddView to default display failed (somewhere).\n");


	/*
	 * Now we get to have some fun.
	 */
	vltop = 0;
	dh = 1;
	dl = 3;
	dm1 = -1;
	dm2 = 1;
	avgbits = 0;
	while (1) {
		/*
		 * We're about to do a lot of View modification.  Each such
		 * modification would ordinarily require a full recompilation
		 * of the VDL list.  So we lock down the default display,
		 * deferring such recompiles until our modifications are
		 * complete.
		 */
		LockDisplay (0);

		/*
		 * Bounce the Views around.
		 */
		bounceview (rphires, &dh);
		bouncesubview (rpm1, &dm1);
		bouncesubview (rpm2, &dm2);
		bouncewindow (rpm2);

		/*
		 * Bounce the ViewList around.  This will drag its sub-Views
		 * with it.
		 */
		vltop += dl;
		if (vltop < 0) {
			vltop = 0;
			dl = -dl;
		} else if (vltop >= disphigh) {
			vltop = disphigh - 1;
			dl = -dl;
		}
		if ((err = ModifyGraphicsItemVA (vli,
						 VIEWTAG_TOPEDGE, vltop,
						 TAG_END)) < 0)
			die (err, "ViewList bounce modify failed.\n");

		/*
		 * We're done fiddling with the Views; unlock the display.
		 * Since there are pending changes, this will trigger a
		 * (single) recompile of the VDL list, integrating all our
		 * changes (poof!).
		 */
		UnlockDisplay (0);


		/*
		 * Wait for one field (elsewise we run too fast).
		 * (We could also use View signals here.)
		 */
		WaitTimeVBL (vblIO, 1);

		/*
		 * Look for user input.  If the pause button is pressed,
		 * stop bouncing the Views.  If the stop button is pressed,
		 * exit the program.
		 */
		if ((joybits = getjoybits (0)) & ControlStart) {
			do
				WaitTimeVBL (vblIO, 8);
			while (!(getjoybits (0) & ControlStart));
		}
		if (joybits & ControlA) {
			if (++avgbits > 3)
				avgbits = 0;
			if ((err = ModifyGraphicsItemVA
				    (rpback->rp_ViewItem,
				     VIEWTAG_AVGMODE, avgbits,
				     VIEWTAG_AVGMODE, AVG_DSB_1 | avgbits,
				     TAG_END)) < 0)
				die (err, "Can't turn on VAVG.\n");
		}

		if (joybits & ControlX)
			break;
	}
	printf ("Normal exit.\n");
	return (0);
}


/***************************************************************************
 * The cutesey bouncing/drawing routines.
 */
/*
 * This routine takes a View and bounces it against the top/bottom of the
 * display.
 */
void
bounceview (rp, d)
struct RastPort	*rp;
int32		*d;
{
	View	*v;
	int32	val, high;

	v = rp->rp_View;
	val = *d + v->v_TopEdge;
	high = v->v_Height;

	if (val < 0) {
		val = 0;
		*d = -*d;
	} else if (val + high >= disphigh) {
		val = disphigh - high;
		*d = -*d;
	}

	if ((val = ModifyGraphicsItemVA (rp->rp_ViewItem,
					 VIEWTAG_TOPEDGE, val,
					 TAG_END)) < 0)
		die (val, "Bounce modify failed.\n");
}

/*
 * This routine takes a View and bounces it "around" zero.  That is, it is
 * never further than 20 pixels from zero (relative to the ViewList) to the
 * nearest edge of the View.
 */
void
bouncesubview (rp, d)
struct RastPort	*rp;
int32		*d;
{
	View	*v;
	int32	top, bot, high;

	v = rp->rp_View;
	top = *d + v->v_TopEdge;
	high = v->v_Height;
	bot = top + high;

	if (bot < -20) {
		top = -20 - high;
		*d = -*d;
	} else if (top > 20) {
		top = 20;
		*d = -*d;
	}

	if ((bot = ModifyGraphicsItemVA (rp->rp_ViewItem,
					 VIEWTAG_TOPEDGE, top,
					 TAG_END)) < 0)
		die (bot, "Sub-bounce modify failed.\n");
}

/*
 * This routine adjusts the WINLEFTEDGE and WINTOPEDGE paramaters of the
 * View to "pan and scan" the underlying Bitmap.  A rectangle is also drawn
 * into the Bitmap to indicate the region of the Bitmap currently displayed
 * through the View.
 */
void
bouncewindow (rp)
struct RastPort	*rp;
{
	static int32	dwx = 2,
			dwy = 1;
	static int	undraw;
	View		*v;
	int32		wx, wy;
	int32		tmp;

	v = rp->rp_View;
	wx = v->v_WinLeft;
	wy = v->v_WinTop;

	/*
	 * Erase previous bounding rectangle.  (Yes, CadTrac, I'm using
	 * XOR to do it.  Bite me.)
	 */
	setfgpen (rp, ~0, ~0, ~0);
	rp->rp_DrawMode = DRAWMODE_XOR;
	if (undraw) {
		moveto (rp, wx - 1, wy - 1);
		drawto (rp, wx + v->v_PixWidth, wy - 1);
		drawto (rp, wx + v->v_PixWidth, wy + v->v_PixHeight);
		drawto (rp, wx - 1, wy + v->v_PixHeight);
		drawto (rp, wx - 1, wy - 1);
	}

	/*
	 * Move/bounce viewing region.
	 */
	wx += dwx;
	wy += dwy;

	if (wx < 0) {
		wx = 0;
		dwx = -dwx;
	} else if (wx > (tmp = rp->rp_Bitmap->bm_Width - v->v_PixWidth)) {
		wx = tmp;
		dwx = -dwx;
	}
	if (wy < 0) {
		wy = 0;
		dwy = -dwy;
	} else if (wy > (tmp = rp->rp_Bitmap->bm_Height - v->v_PixHeight)) {
		wy = tmp;
		dwy = -dwy;
	}

	/*
	 * Update View with new viewing region.
	 */
	if ((tmp = ModifyGraphicsItemVA (rp->rp_ViewItem,
					 VIEWTAG_WINLEFTEDGE, wx,
					 VIEWTAG_WINTOPEDGE, wy,
					 TAG_END)) < 0)
		die (tmp, "Win modify failed.\n");

	/*
	 * Draw new bounding rectangle.
	 */
	moveto (rp, wx - 1, wy - 1);
	drawto (rp, wx + v->v_PixWidth, wy - 1);
	drawto (rp, wx + v->v_PixWidth, wy + v->v_PixHeight);
	drawto (rp, wx - 1, wy + v->v_PixHeight);
	drawto (rp, wx - 1, wy - 1);
	undraw = TRUE;

	rp->rp_DrawMode = DRAWMODE_JAM1;
}


/*
 * This routine draws the mandala pattern in the Bitmap.
 */
void
drawrastport (rp, r, g, b)
struct RastPort	*rp;
uint32		r, g, b;
{
#define	NDIVS	40
	Bitmap	*bm;
	int32	wide, high;
	int32	x, y;
	int	i;

	bm = rp->rp_Bitmap;
	wide = bm->bm_Width;
	high = bm->bm_Height;

	setfgpen (rp, r, g, b);
	rectfill (rp, 0, 0, wide, high);

	for (i = NDIVS;  --i >= 0; ) {
		x = i * wide / NDIVS;
		y = i * high / NDIVS;
		colorwheel (rp, i * 0x10000 / NDIVS);

		moveto (rp, 0, y);
		drawto (rp, x, high - 1);

		moveto (rp, wide - 1, high - 1 - y);
		drawto (rp, wide - 1 - x, 0);

		moveto (rp, 0, y);
		drawto (rp, wide - 1 - x, 0);

		moveto (rp, wide - 1, high - 1 - y);
		drawto (rp, x, high - 1);
	}
}



/***************************************************************************
 * Bitmap and View construction.
 *
 * This routine takes the rudimentary description in the 'vdef' structure
 * and uses it to build Bitmaps and Views of the appropriate size, filling
 * in the parallel RastPort structure array.
 */
void
buildrastports (rps, defs, n)
RastPort	*rps;
struct vdef	*defs;
int32		n;
{
	register int	i;
	Item		prevbm;
	Err		err;
	char		*buf;

	/*
	 * Create Views, Bitmaps, and the frame buffers.
	 */
	prevbm = 0;
	for (i = 0;  i < n;  i++) {
		ViewTypeInfo	*vti;
		Bitmap		*bm;
		Item		vi;
		Item		bmi;
		int32		wide, high;

		/*
		 * Create View.
		 */
		vi = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEW_NODE),
				   VIEWTAG_VIEWTYPE, defs[i].vtype,
				   TAG_END);
		if (vi < 0)
			die (vi, "Can't create View.\n");

		rps[i].rp_View = LookupItem (vi);
		rps[i].rp_ViewItem = vi;

		/*
		 * Use the View we just created to sense and automatically
		 * adjust for display size.
		 * If the dimension is negative, that means we want the
		 * maximum possible size available for this View.  If it's
		 * positive, then we want that many pixels.
		 */
		vti = rps[i].rp_View->v_ViewTypeInfo;
		if ((wide = defs[i].bmwidth) < 0)
			wide = vti->vti_MaxPixWidth;
		if ((high = defs[i].bmheight) < 0)
			high = vti->vti_MaxPixHeight;

		/*
		 * Now that we have our dimensions, create the Bitmap and
		 * its buffer.
		 */
		bmi = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
				    BMTAG_WIDTH, wide,
				    BMTAG_HEIGHT, high,
				    BMTAG_TYPE, defs[i].bmtype,
				    BMTAG_DISPLAYABLE, TRUE,
				    BMTAG_RENDERABLE, TRUE,
				    BMTAG_BUMPDIMS, TRUE,
				    BMTAG_COALIGN, prevbm,
				    TAG_END);
		if (bmi < 0)
			die (bmi, "Can't create Bitmap Item.\n");
		bm = LookupItem (bmi);

		if (!(buf = AllocMemMasked (bm->bm_BufferSize,
					    bm->bm_BufMemType,
					    bm->bm_BufMemCareBits,
					    bm->bm_BufMemStateBits)))
			die (0, "Can't allocate Bitmap buffer.\n");

		if ((err = ModifyGraphicsItemVA (bmi,
						 BMTAG_BUFFER, buf,
						 TAG_END)) < 0)
			die (err, "Bitmap modify failed.\n");

		rps[i].rp_Bitmap = bm;
		rps[i].rp_BitmapItem = bmi;
		prevbm = bmi;


		/*
		 * Now install the Bitmap into the View, and set the
		 * corresponding size.  (Use the View size requests this
		 * time.)
		 */
		if ((wide = defs[i].vwidth) < 0)
			wide = vti->vti_MaxPixWidth;
		if ((high = defs[i].vheight) < 0)
			high = vti->vti_MaxPixHeight;
		if ((err = ModifyGraphicsItemVA (vi,
						 VIEWTAG_BITMAP, bmi,
						 VIEWTAG_PIXELWIDTH, wide,
						 VIEWTAG_PIXELHEIGHT, high,
						 TAG_END)) < 0)
			die (err, "Can't set View dimensions.\n");

		/*
		 * Initialize remainder of RastPort.
		 */
		rps[i].rp_X = rps[i].rp_Y = 0;
		rps[i].rp_ColorR = rps[i].rp_ColorG = rps[i].rp_ColorB = 0;
		rps[i].rp_Color32 = 0;
		rps[i].rp_Color16 = 0;
		rps[i].rp_DrawMode = 0;
	}
}


/***************************************************************************
 * Remarkably primitive rendering primitives.
 *
 * Some very simple CPU-based rendering routines.  I wrote these because I
 * was too lazy to bother to learn about the 2D API.
 *
 * (The rectangle primitives don't do XOR.  Extending them to cover this
 * functionality is left as an exercise for the reader :-). )
 */
Err
initrastport (rp, vi)
struct RastPort	*rp;
Item		vi;
{
	View	*v;

	if (!(v = rp->rp_View = LookupItem (vi)))
		return (BADITEM);

	rp->rp_ViewItem = vi;
	rp->rp_Bitmap = v->v_Bitmap;
	rp->rp_BitmapItem = v->v_BitmapItem;

	rp->rp_X = rp->rp_Y = 0;
	rp->rp_ColorR = rp->rp_ColorG = rp->rp_ColorB = 0;
	rp->rp_Color32 = 0;
	rp->rp_Color16 = 0;
	rp->rp_DrawMode = 0;

	return (0);
}


void
setfgpen (rp, r, g, b)
struct RastPort	*rp;
uint32		r, g, b;
{
	rp->rp_ColorR = r;
	rp->rp_ColorG = g;
	rp->rp_ColorB = b;

	rp->rp_Color32 = ((r & 0xFF000000) >> 8) |
			 ((g & 0xFF000000) >> 16) |
			 ((b & 0xFF000000) >> 24);
	rp->rp_Color16 = ((r & 0xF8000000) >> 17) |
			 ((g & 0xF8000000) >> 22) |
			 ((b & 0xF8000000) >> 27);
}


void
rectfill (rp, x, y, w, h)
struct RastPort	*rp;
int32	x, y, w, h;
{
	if (rp->rp_Bitmap->bm_Type == BMTYPE_16)
		rf_16 (rp, x, y, w, h);
	else if (rp->rp_Bitmap->bm_Type == BMTYPE_32)
		rf_32 (rp, x, y, w, h);
}

void
rf_16 (rp, x, y, w, h)
struct RastPort	*rp;
int32	x, y, w, h;
{
	uint16	*pix, *line;
	int32	cw, ch, stride;

	if (!(line = PixelAddress (rp->rp_BitmapItem, x, y)))
		die (0, "What pixel?\n");

	stride = rp->rp_Bitmap->bm_Width;

	for (ch = h;  --ch >= 0; ) {
		pix = line;
		for (cw = w;  --cw >= 0; )
			*pix++ = rp->rp_Color16;
		line += stride;
	}
}

void
rf_32 (rp, x, y, w, h)
struct RastPort	*rp;
int32	x, y, w, h;
{
	uint32	*pix, *line;
	int32	cw, ch, stride;

	if (!(line = PixelAddress (rp->rp_BitmapItem, x, y)))
		die (0, "What pixel?\n");

	stride = rp->rp_Bitmap->bm_Width;

	for (ch = h;  --ch >= 0; ) {
		pix = line;
		for (cw = w;  --cw >= 0; )
			*pix++ = rp->rp_Color32;
		line += stride;
	}
}


void
moveto (rp, x, y)
struct RastPort	*rp;
int32		x, y;
{
	rp->rp_X = x;
	rp->rp_Y = y;
}

void
drawto (rp, x, y)
struct RastPort	*rp;
int32	x, y;
{
	int32	dx, dy, xi, yi, e;
	int	i, swap;

	dx = x - rp->rp_X;
	dy = y - rp->rp_Y;
	xi = dx < 0  ?  -1  :  (dx > 0  ?  1  :  0);
	yi = dy < 0  ?  -1  :  (dy > 0  ?  1  :  0);
	if (dx < 0)	dx = -dx;
	if (dy < 0)	dy = -dy;

	if (dy > dx) {
		int32	tmp;

		tmp = dx;  dx = dy;  dy = tmp;
		swap = TRUE;
	} else
		swap = FALSE;

	e = dy + dy - dx;

	i = x;  x = rp->rp_X;  rp->rp_X = i;
	i = y;  y = rp->rp_Y;  rp->rp_Y = i;
	for (i = dx + 1;  --i >= 0; ) {
		writepixel (rp, x, y);
		while (e >= 0) {
			if (swap)	x += xi;
			else		y += yi;

			e -= dx + dx;
		}
		if (swap)	y += yi;
		else		x += xi;

		e += dy + dy;
	}
}


void
writepixel (rp, x, y)
struct RastPort	*rp;
int32	x, y;
{
	void	*px;

	if (!(px = PixelAddress (rp->rp_BitmapItem, x, y)))
		return;

	if (rp->rp_Bitmap->bm_Type == BMTYPE_16) {
		if (rp->rp_DrawMode)
			*(uint16 *) px ^= rp->rp_Color16;
		else
			*(uint16 *) px = rp->rp_Color16;

	} else if (rp->rp_Bitmap->bm_Type == BMTYPE_32) {
		if (rp->rp_DrawMode)
			*(uint32 *) px ^= rp->rp_Color32;
		else
			*(uint32 *) px = rp->rp_Color32;
	}
}


/***************************************************************************
 * A cheesy little colorwheel routine (just 'cuz).  'rot' expresses
 * rotation around the colorwheel as a fixed point fraction.  0x10000 equals
 * 360 degrees.
 *
 * Math/graphics whizes will note that the conversion isn't perfect.
 */
void
colorwheel (rp, rot)
struct RastPort	*rp;
uint32		rot;
{
	int32	phase;
	uint32	third, sixth;
	uint32	r, g, b;


	rot &= 0xFFFF;
	phase = 6 * rot / 0x10000;
	third = 0x10000 / 3;
	sixth = third / 2;
	r = g = b = 0;

	if (rot < third) {
		r = 255;
		if (phase & 1)
			g = 256 * (rot - sixth) / sixth;
		else
			b = 256 * (sixth - rot) / sixth;
	} else if (rot >= 2 * third) {
		b = 255;
		if (phase & 1)
			r = 256 * (rot - 0x10000 + sixth) / sixth;
		else
			g = 256 * (0x10000 - sixth - rot) / sixth;
	} else {
		g = 255;
		if (phase & 1)
			b = 256 * (rot - third - sixth) / sixth;
		else
			r = 256 * (third + sixth - rot) / sixth;
	}
	if (r > 255)	r = 255;
	if (g > 255)	g = 255;
	if (b > 255)	b = 255;
	setfgpen (rp, r << 24, g << 24, b << 24);
}


/***************************************************************************
 * Returns newly pressed bits in joystick.
 */
uint32
getjoybits (shiftbits)
long	shiftbits;
{
	static uint32		prevbits;
	register uint32		newbits, changed;
	ControlPadEventData	cped;

	/*  Which bits have changed since last test?  */
	if (GetControlPad (1, FALSE, &cped) < 0) {
		printf ("GetControlPad() failed.\n");
		return (0);
	}
	newbits = cped.cped_ButtonBits;
	changed = newbits ^ prevbits;

	/*  Return only positive transitions.  */
	changed = changed & newbits;

	/*  OR in current state of "shift" bits.  */
	changed |= newbits & shiftbits;
	prevbits = newbits;
	return (changed);
}


/***************************************************************************
 * Housekeeping.
 */
void
openstuff ()
{
	if (OpenGraphicsFolio () < 0)
		die (0, "Can't open graphics folio.\n");

	/*
	 * Initialize event system.
	 */
	if (InitEventUtility (1, 0, LC_Observer) < 0)
		die (0, "Failed to InitEventUtility\n");

	if ((vblIO = CreateTimerIOReq ()) < 0)
		die (vblIO, "Can't create timer I/O.\n");
}

void
closestuff ()
{
	if (vblIO > 0)	DeleteTimerIOReq (vblIO);

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
