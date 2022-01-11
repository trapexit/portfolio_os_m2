/*
 * @(#) graphicsenv.c 96/09/16 1.7
 *
 * Formatted for 8 space Tab stops
 */

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <graphics:graphics.h>
#include <graphics:view.h>
#include <graphics:bitmap.h>
#include <graphics:clt:gstate.h>
#include <kernel:task.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <graphics/clt/gstate.h>
#include <kernel/task.h>
#endif
#include <stdio.h>
#include "graphicsenv.h"

/* Graphics environment constants */
#define USE_Z_BUFFERING     TRUE
#define kNumCmdListBufs     2
#define kCmdListBufSize     2200	/* Don't need much since programs are MP now */


/* Error codes */
#define Hg_ErrId            MakeErrId('H','g')
#define Hg_NoMem            MakeErr(ER_TASK, Hg_ErrId, ER_SEVERE, \
				    ER_E_APPL, ER_C_STND, ER_NoMem)


/* ---------------------------------------------------------------------
Function: GraphicsEnv_Create
          Create a GraphicsEnv object, which maintains info about the
          View, Bitmaps, and GState
Input:    (none)
Output:   (none)
Returns:  Pointer to GraphicsEnv object, or NULL if failed.
*/
GraphicsEnv* GraphicsEnv_Create(void)
{
    GraphicsEnv* genv;

    genv = AllocMem(sizeof(GraphicsEnv), MEMTYPE_NORMAL);
    
    if (genv) {
	genv->d = NULL;
	genv->gs = NULL;
    }

    return genv;
}


/* ---------------------------------------------------------------------
Function: GraphicsEnv_Init
          Initialize the fields of an already created GraphicsEnv.
          This routine creates a View, the Bitmaps, and GState
Input:    width:  The desired width of the display.  0 means use default
          height: The desired height of the display.  0 means use default
          depth:  The desired bit depth of the display.  0 means use default
Output:   genv:   A GraphicsEnv object as created with GraphicsEnv_Create()
Returns:  Portfolio error code or 0 if success
*/
Err GraphicsEnv_Init(GraphicsEnv* genv, uint32 width, uint32 height, uint32 depth)
{
    Err retVal, err;
    uint32 bitmapType;

    err = OpenGraphicsFolio();
    if (err < 0) {
	retVal = err;
	goto Failed_OpenGfx;
    }

    /* Create and setup the view */
    genv->d = Display_Create();
    if (genv->d == NULL) {
	retVal = Hg_NoMem;
	goto Failed_DisplayCreate;
    }
    if (width != 0)    genv->d->width = width;
    if (height != 0)   genv->d->height = height;
    if (depth != 0)    genv->d->depth = depth;
    genv->d->signal = AllocSignal(0);
    if (genv->d->signal == 0) {
	retVal = -1;
	goto Failed_AllocSignal;
    }
    err = Display_CreateView(genv->d);
    if (err < 0) {
	retVal = err;
	goto Failed_CreateView;
    }

    /* Allocate bitmaps */
    bitmapType = (genv->d->depth == 16 ? BMTYPE_16 : BMTYPE_32);
    err = GS_AllocBitmaps(genv->bitmaps, genv->d->width, genv->d->height,
			  bitmapType, genv->d->numScreens, USE_Z_BUFFERING);
    if (err < 0) {
	retVal = err;
	goto Failed_AllocBitmaps;
    }
    /* In the following call to MGIVA(), we have to also write out some other
     * tag in the View, other than the VIEWTAG_BITMAP.  This causes the VDL
     * list to be recompiled, which in turn enables video output.  When we
     * put the view on the view list, it didn't have a bitmap, so the Gfx
     * Folio didn't activate it.  If we had just done the VIEWTAG_BITMAP tag,
     * then the Graphics Folio would shortcut this common case and not do an
     * actual recompile of the VDL list (for better performance).
     */
    err = ModifyGraphicsItemVA(genv->d->view,
			       VIEWTAG_BITMAP, genv->bitmaps[0],
			       VIEWTAG_TOPEDGE, 0,
			       TAG_END);
    if (err < 0) PrintfSysErr(err);

    /* Create and setup the GState */
    genv->gs = GS_Create();
    if (genv->gs < 0) {
	retVal = Hg_NoMem;
	goto Failed_GSCreate;
    }
    err = GS_AllocLists(genv->gs, kNumCmdListBufs, kCmdListBufSize);
    if (err < 0) {
	retVal = err;
	goto Failed_GSAllocLists;
    }
    GS_SetVidSignal(genv->gs, genv->d->signal);
    GS_SetDestBuffer(genv->gs, genv->bitmaps[0]);
    if (USE_Z_BUFFERING)
	GS_SetZBuffer(genv->gs, genv->bitmaps[genv->d->numScreens]);

    /* Everything went OK */
    retVal = 0;
    printf("Using %d-bit display, size=%d x %d, %d bitmaps, %s\n",
	   genv->d->depth,genv->d->width,genv->d->height,genv->d->numScreens,
	   (USE_Z_BUFFERING ? "Z-buffered" : "No Z-buffer"));
    goto Exit;

Failed_GSAllocLists:
    GS_Delete(genv->gs);
Failed_GSCreate:
    GS_FreeBitmaps(genv->bitmaps, genv->d->numScreens + (USE_Z_BUFFERING?1:0));
Failed_AllocBitmaps:
Failed_CreateView:
    FreeSignal(genv->d->signal);
Failed_AllocSignal:
    Display_Delete(genv->d);
Failed_DisplayCreate:
    CloseGraphicsFolio();
Failed_OpenGfx:
Exit:
    return retVal;
}


/* ---------------------------------------------------------------------
Function: GraphicsEnv_Delete
          Free memory associated with a GraphicsEnv object
Input:    genv: Ptr to a GraphicsEnv object
Output:   (none)
Returns:  (none)
*/
void GraphicsEnv_Delete(GraphicsEnv* genv)
{
    if (genv->d) {
	GS_FreeBitmaps(genv->bitmaps,
		       genv->d->numScreens + (USE_Z_BUFFERING ? 1 : 0));
	Display_Delete(genv->d);
    }
    FreeMem(genv, sizeof(GraphicsEnv));
}


/* --------------------------------------------------------------------- */
/* Display Folio utilities */
/* --------------------------------------------------------------------- */


/* List of available view types.  Order is important. */
static uint32 viewTypes[] = {
    VIEWTYPE_16,
    VIEWTYPE_16_LACE,
    VIEWTYPE_32,
    VIEWTYPE_32_LACE,
    VIEWTYPE_16_640,
    VIEWTYPE_16_640_LACE,
    VIEWTYPE_32_640,
    VIEWTYPE_32_640_LACE
};


/* ---------------------------------------------------------------------
Function: Display_Create
          Allocate and initialize a Display structure
Input:    (none)
Output:   (none)
Returns:  Display structure, filled in with some defaults, or NULL if failure
*/
Display* Display_Create(void)
{
    Display* d;

    d = AllocMem(sizeof(Display), MEMTYPE_NORMAL);
    if (d == NULL)
	return NULL;
    d->width = 640;
    d->height = 480;
    d->depth = 32;
    d->numScreens = 2;
    d->signal = 0;
    d->interpolated = FALSE;
    d->view = -1;
    return d;
}

/* ---------------------------------------------------------------------
Function: Display_CreateView
          Create a View suitable for displaying the types of bitmaps that the
          Display structure describes

Input:    d->width
          d->height
          d->depth
          d->signal
	  d->interpolated
Output:   d->view
Returns:  Portfolio error, or 0 if successful
*/
Err Display_CreateView(Display* d)
{
    uint32 whichView = 0;
    Err    retVal, err;
    View*  view;
    int32  avgModeTagArg0, avgModeTagArg1;

    /* Determine which type of view we need from the list above */
    if (d->width == 640)        whichView |= 4;
    if (d->depth == 32)         whichView |= 2;
    if (d->height == 480)       whichView |= 1;

    /* Create the view onto which bitmaps will be placed */
    d->view = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			   VIEWTAG_VIEWTYPE,     viewTypes[whichView],
			   TAG_END);
    if (d->view < 0) {
	retVal = d->view;
	goto Failed_CreateView;
    }

    /* Determine what the real dimensions of this View are */
    view = (View*)LookupItem(d->view);
    d->width = view->v_ViewTypeInfo->vti_MaxPixWidth;
    d->height = view->v_ViewTypeInfo->vti_MaxPixHeight;
    err = ModifyGraphicsItemVA(d->view,
			       VIEWTAG_PIXELWIDTH, d->width,
			       VIEWTAG_PIXELHEIGHT, d->height,
			       TAG_END);
    if (err < 0)
	goto Failed_MGIPixWidth;

    /* Enable video interpolation, if desired */
    if (d->interpolated) {
	avgModeTagArg0 = AVG_DSB_0;
	avgModeTagArg1 = AVG_DSB_1;
	if (d->width < 512) {	/*  Kinda arbitrary number  */
	    avgModeTagArg0 |= AVGMODE_H;
	    avgModeTagArg1 |= AVGMODE_H;
	}
	if (d->height < 350) {	/*  ...And another one  */
	    avgModeTagArg0 |= AVGMODE_V;
	    avgModeTagArg1 |= AVGMODE_V;
	}
	err = ModifyGraphicsItemVA(d->view,
				   VIEWTAG_AVGMODE, avgModeTagArg0,
				   VIEWTAG_AVGMODE, avgModeTagArg1,
				   TAG_END);
	if (err < 0)
	    goto Failed_MGIInterp;
    }

    /* Enable the RenderSignal if one has been allocated.  This signal bit
       is used to sync the CPU to the display for tear-free double-buffering */
    if (d->signal) {
	err = ModifyGraphicsItemVA(d->view,
				   VIEWTAG_DISPLAYSIGNAL, d->signal,
				   VIEWTAG_BESILENT,     0,
				   TAG_END);
	if (err < 0)
	    goto Failed_MGISignal;
    }

    err = AddViewToViewList(d->view, 0);
    if (err < 0) {
	goto Failed_AddView;
    }

    retVal = 0;
    goto Exit;

    /* If we had any errors, these Failed_XXX sections will cleanup and exit */
Failed_AddView:
Failed_MGISignal:
Failed_MGIInterp:
Failed_MGIPixWidth:
    retVal = err;
    DeleteItem(d->view);
Failed_CreateView:
Exit:
    return retVal;
}


/* ---------------------------------------------------------------------
Function: Display_Delete
          Free all memory associated with a Display object

Input:    d
Output:   (none)
Returns:  (none)
*/
void Display_Delete(Display* d)
{
    if (d->view >= 0)
	DeleteItem(d->view);
    FreeMem(d, sizeof(Display));
}

