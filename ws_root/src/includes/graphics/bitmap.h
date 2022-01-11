#ifndef	__GRAPHICS_BITMAP_H
#define	__GRAPHICS_BITMAP_H

/***************************************************************************
**
**  @(#) bitmap.h 95/09/21 1.8
**
**  Definitions for the Bitmap Item.
**
****************************************************************************/

#ifndef	__KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


/***************************************************************************
 * The Bitmap Item structure.
 */
typedef struct Bitmap {
	ItemNode	bm;

	uint32		bm_Width;	/*  Total width, in pixels.	*/
	uint32		bm_Height;	/*  Total height, in pixels.	*/

	uint32		bm_XOrigin;	/*  In pixels			*/
	uint32		bm_YOrigin;
	uint32		bm_ClipWidth;
	uint32		bm_ClipHeight;

	uint32		bm_BufferSize;	/*  Size of buffer, in bytes	*/
	uint32		bm_BufMemType;	/*  Required memory type bits	*/
	uint32		bm_BufMemCareBits;	/* For AllocMemMasked()	*/
	uint32		bm_BufMemStateBits;

	void		*bm_Buffer;	/*  Pointer to RAM buffer	*/

	uint32		bm_Type;	/*  Type of bitmap.		*/
#ifndef	EXTERNAL_RELEASE

	uint32		bm_Flags;	/*  Internal flag bits		*/
	void		*bm_OriginPtr;	/*  Pointer to origin pixel	*/
	struct BMTypeInfo	*bm_TypeInfo;	/*  Ptr to BMTypeInfo	*/
	struct Bitmap	*bm_CoAlign;	/*  Bitmap to co-align with	*/

#endif
} Bitmap;


#ifndef	EXTERNAL_RELEASE

/*
 * Flags bits for the bm_Flags field.
 */
#define	BMF_DISPLAYABLE		1
#define	BMF_RENDERABLE		(1<<1)
#define	BMF_MPEGABLE		(1<<2)
#define	BMF_BUMPDIMS		(1<<3)

#define	BMF_ABLEMASK		(BMF_DISPLAYABLE | \
				 BMF_RENDERABLE | \
				 BMF_MPEGABLE)

#endif



/*
 * Types of Bitmap that may be created.
 */
enum	BitmapType {
	BMTYPE_INVALID = 0,
	BMTYPE_16,		/*  M2-style buffer, 16 bpp	*/
	BMTYPE_32,		/*  M2-style buffer, 32 bpp	*/
	BMTYPE_16_ZBUFFER,	/*  16 bpp zbuffer		*/
	MAX_BMTYPE
};


/*
 * Bitmap Item creation tags passed to CreateItem().
 */
enum	BitmapTags {
	BMTAG_WIDTH = TAG_ITEM_LAST + 1,
	BMTAG_HEIGHT,
	BMTAG_TYPE,
	BMTAG_CLIPWIDTH,
	BMTAG_CLIPHEIGHT,
	BMTAG_XORIGIN,
	BMTAG_YORIGIN,
	BMTAG_BUFFER,

	BMTAG_DISPLAYABLE = 1024,	/*  Validate for displayability	*/
	BMTAG_RENDERABLE,		/*  Validate for renderability	*/
	BMTAG_MPEGABLE,			/*  Validate for MPEG decode	*/

	BMTAG_BUMPDIMS = 1024 + 256,	/*  Bump dimensions		*/
	BMTAG_COALIGN,			/*  Co-align Bitmap buffer	*/
	BMTAG_RESETCLIP,		/*  Reset clip values to max	*/

	MAX_BMTAG
};


#endif	/*  __GRAPHICS_BITMAP_H  */
