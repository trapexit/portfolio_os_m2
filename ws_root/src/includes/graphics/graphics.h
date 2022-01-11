#ifndef	__GRAPHICS_GRAPHICS_H
#define	__GRAPHICS_GRAPHICS_H

/***************************************************************************
**
**  @(#) graphics.h 96/07/30 1.19
**
**  General definitions for the graphics folio.
**
****************************************************************************/


#ifndef	__KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef	__KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef	__GRAPHICS_BITMAP_H
#include <graphics/bitmap.h>
#endif


/***************************************************************************
 * The name of the rose^H^H^H^Hfolio.
 */
#define	GRAPHICSFOLIONAME	"graphics"
#define	TEDEVICENAME		"triangleengine"


/***************************************************************************
 * Node type numbers maintained by this folio.
 */
enum GraphicsFolioNode {
	GFX_BITMAP_NODE = 1,
	GFX_VIEW_NODE,
	GFX_VIEWLIST_NODE,
	GFX_PROJECTOR_NODE
};


/***************************************************************************
 * Order ops for OrderViews().
 */
enum OrderOps {
	ORDEROP_INVALID = 0,
	ORDEROP_BEFORE,
	ORDEROP_AFTER,
	MAX_ORDEROP
};


/***************************************************************************
 * Tags for QueryGraphics().
 */
enum QueryGraphicsTags {
	QUERYGFXTAG_END = 0,
	QUERYGFXTAG_DEFAULTPROJECTOR,
	MAX_QUERYGFXTAG
};


/***************************************************************************
 * Typedefs.
 */
typedef uint32	*CmdListP;


/***************************************************************************
 * Graphics folio error codes.
 * (Should any of this be in kernel/operror.h?)
 */
#define	GERR_BASE	0

#define	GERR(svr,class,err)	\
		MAKEERR (ER_FOLI, ER_GRFX, svr, ER_E_SSTM, class, err)


#define	GFX_ERR_NOMEM		GERR (ER_SEVERE, ER_C_STND, ER_NoMem)
#define	GFX_ERR_BADPTR		GERR (ER_SEVERE, ER_C_STND, ER_BadPtr)
#define	GFX_ERR_BADITEM		GERR (ER_SEVERE, ER_C_STND, ER_BadItem)
#define	GFX_ERR_BADSUBTYPE	GERR (ER_SEVERE, ER_C_STND, ER_BadSubType)
#define	GFX_ERR_NOTSUPPORTED	GERR (ER_SEVERE, ER_C_STND, ER_NotSupported)
#define	GFX_ERR_NOTOWNER	GERR (ER_SEVERE, ER_C_STND, ER_NotOwner)
#define	GFX_ERR_NOTFOUND	GERR (ER_SEVERE, ER_C_STND, ER_NotFound)
#define	GFX_ERR_BADTAGARG	GERR (ER_SEVERE, ER_C_STND, ER_BadTagArg)
#define	GFX_ERR_BADTAGARGVAL	GERR (ER_SEVERE, ER_C_STND, ER_BadTagArgVal)
#define	GFX_ERR_BADCOMMAND	GERR (ER_SEVERE, ER_C_STND, ER_BadCommand)
#define	GFX_ERR_ABORTED		GERR (ER_SEVERE, ER_C_STND, ER_Aborted)
#define	GFX_ERR_IOINCOMPLETE	GERR (ER_SEVERE, ER_C_STND, ER_IOIncomplete)

#define	GFX_ERR_INTERNAL	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 1)
#define	GFX_ERR_BADDIMS		GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 2)
#define	GFX_ERR_BADORDEROP	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 3)
#define	GFX_ERR_BADBITMAP	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 4)
#define	GFX_ERR_TYPEMISMATCH	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 5)
#define	GFX_ERR_VIEWNOTINLIST	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 6)
#define	GFX_ERR_BADCLIP		GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 7)
#define	GFX_ERR_BADVIEWTYPE	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 8)
#define	GFX_ERR_BADBITMAPTYPE	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 9)
#define	GFX_ERR_BADVERSION	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 10)
#define	GFX_ERR_BITMAPUNABLE	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 11)
#define	GFX_ERR_VIEWINCOMPAT	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 12)
#define	GFX_ERR_VIEWINUSE	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 13)
#define	GFX_ERR_PROJINACTIVE	GERR (ER_SEVERE, ER_C_NSTND, GERR_BASE + 14)


/***************************************************************************
 * Function prototypes (folio and link library).
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Core folio routines.
 */
Err ModifyGraphicsItem (Item dispItem, const struct TagArg *args);
Err AddViewToViewList (Item viewItem, Item viewListItem);
Err RemoveView (Item view);
Err OrderViews (Item victim, int32 op, Item radix);
Err LockDisplay (Item display);
Err UnlockDisplay (Item display);
void *PixelAddress (Item bitmapItem, uint32 x, uint32 y);
Item ActivateProjector (Item projectorItem);
Err DeactivateProjector (Item projectorItem);
Item NextProjector (Item projectorItem);
struct ViewTypeInfo *NextViewTypeInfo (Item projectorItem,
				       const struct ViewTypeInfo *vti);
Err QueryGraphics (const struct TagArg *args);


/*
 * Convenience/link library routines.
 */
Err	OpenGraphicsFolio (void);
Err	CloseGraphicsFolio (void);
Err	ModifyGraphicsItemVA (Item dispItem, uint32 tag, ...);
Item	CreateTEIOReq (void);
Err	DeleteTEIOReq (Item ioreq);
Err	QueryGraphicsVA (uint32 tag, ...);


#ifdef __cplusplus
}
#endif


#endif	/*  __GRAPHICS_GRAPHICS_H  */
