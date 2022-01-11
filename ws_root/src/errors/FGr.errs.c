/*  :ts=8 bk=0
 *
 * FGr.errs.c:	Error strings for the graphics folio
 *
 * @(#) FGr.errs.c 96/06/16 1.10
 *
 * Leo L. Schwab					9504.06
 *  Converted to standalone form			9510.17
 */
#include <kernel/types.h>
#include <graphics/graphics.h>


static const char * const errstrs[] =
{
	"Error?  What error?  You're high...",

	/*  GFX_ERR_INTERNAL	*/
	"Warp core reactor primary coolant failure:  Guru Meditation #80000003.DEADBEEF",
	/*  GFX_ERR_BADDIMS	*/
	"Bad dimensions.",
	/*  GFX_ERR_BADORDEROP	*/
	"Bad operation to OrderViews().",
	/*  GFX_ERR_BADBITMAP	*/
	"Bad Bitmap Item.",
	/*  GFX_ERR_TYPEMISMATCH	*/
	"View type incompatible with Bitmap type.",
	/*  GFX_ERR_VIEWNOTINLIST	*/
	"View not in ViewList.",
	/*  GFX_ERR_BADCLIP	*/
	"Bad clip value(s).",
	/*  GFX_ERR_BADVIEWTYPE	*/
	"Bad View Type.",
	/*  GFX_ERR_BADBITMAPTYPE	*/
	"Bad Bitmap Type.",
	/*  GFX_ERR_BADVERSION	*/
	"Bad version.",
	/*  GFX_ERR_BITMAPUNABLE	*/
	"Bitmap Type unable to support requested characteristics.",
	/*  GFX_ERR_VIEWINCOMPAT	*/
	"View Type incompatible with ViewList/Projector Type.",
	/*  GFX_ERR_VIEWINUSE	*/
	"View currently in use; RemoveView() it first.",
	/*  GFX_ERR_PROJINACTIVE	*/
	"Projector is not active.",

};

static const TagArg errtags[] = {
/*	TAG_ITEM_NAME,		(void *) GRAPHICSFOLIONAME,	*/
	TAG_ITEM_NAME,		(void *) "Graphics Folio",
	ERRTEXT_TAG_OBJID,	(void *) ((ER_FOLI << ERR_IDSIZE) | ER_GRFX),
	ERRTEXT_TAG_MAXERR,	(void *) (sizeof (errstrs) / sizeof (char *)),
	ERRTEXT_TAG_TABLE,	(void *) errstrs,
	TAG_END,		0
};


const TagArg *
main (void)
{
	return (errtags);
}
