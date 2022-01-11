#ifndef	__GRAPHICS_VIEW_H
#define	__GRAPHICS_VIEW_H

/***************************************************************************
**
**  @(#) view.h 96/09/20 1.21
**
**  Definitions for View and ViewList Items.
**
****************************************************************************/

#ifndef	__KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef	__KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif


#ifndef EXTERNAL_RELEASE
/***************************************************************************
 * Line interrupt structure.
 */
typedef struct lineintr {
	struct MinNode	li_Node;
	uint16		li_Type;
	uint16		li_Line;
} lineintr;

enum lineintrtypes {
	INTRTYPE_INVALID,
	INTRTYPE_ZEROMARKER,
	INTRTYPE_RENDINTR,
	INTRTYPE_DISPINTR,
	MAX_INTRTYPE
};

#define	LI_LINE_MASK		0x3FFF
#define	LI_FIELD_MASK		0xC000

#define	FIELDINCREMENT		0x4000

#ifdef BUILD_BDA2_VIDEO_HACK
#define	INTRTYPE_BDA2HACK	INTRTYPE_INVALID
#endif


#endif
/***************************************************************************
 * The View Item structure.
 */
typedef struct View {
	struct ItemNode		v;
	int32			v_LeftEdge;
	int32			v_TopEdge;
	int32			v_Width;
	int32			v_Height;
	int32			v_PixWidth;
	int32			v_PixHeight;
	int32			v_WinLeft;
	int32			v_WinTop;
	int32			v_Type;
	uint32			v_ViewFlags;

	struct Bitmap		*v_Bitmap;
	Item			v_BitmapItem;

	struct ViewTypeInfo	*v_ViewTypeInfo;
	struct Projector	*v_Projector;
#ifndef	EXTERNAL_RELEASE

	int32			v_Ordinal;

	struct MinNode		v_LayerNode;

	struct lineintr		v_RendLineIntr;
	struct lineintr		v_DispLineIntr;
	uint32			v_RendSig;
	uint32			v_DispSig;
	struct Task		*v_SigTask;

	void			(*v_RendCBFunc)(struct View *, void *);
	void			*v_RendCBData;
	void			(*v_DispCBFunc)(struct View *, void *);
	void			*v_DispCBData;

	int32			v_AbsTopEdge;
	int16			v_VisTopEdge;
	int16			v_VisBotEdge;
	int16			v_PrevVisTopEdge;
	int16			v_PrevVisBotEdge;
	int8			v_AvgMode0,
				v_AvgMode1;

	int16			v_VLine;	/*  Line # where VDL stomped */

	void			*v_BaseAddr;	/* Addr. of upper left pixel */

	uint32			v_DispCtl0;	/*  Pre-computed VDL words.  */
	uint32			v_DispCtl1;
	uint32			v_AVCtl;
	uint32			v_ListCtl;

#endif
} View;


/*
 * Definitions for v_ViewFlags.
 */
#define	VIEWF_VISIBLE		1
#define	VIEWF_FULLYVISIBLE	(1<<1)
#define	VIEWF_PENDINGSIG_REND	(1<<2)
#define	VIEWF_PENDINGSIG_DISP	(1<<3)
#define	VIEWF_RESERVED_PROJECTOR 0xFFFF0000

#ifndef EXTERNAL_RELEASE
#define	VIEWF_FORCESCHEDSIG_REND (1<<15)
#define	VIEWF_FORCESCHEDSIG_DISP (1<<14)
#endif

/***************************************************************************
 * The ViewList Item structure.
 */
typedef struct ViewList {
	struct ItemNode		vl;
	int32			vl_LeftEdge;
	int32			vl_TopEdge;
#ifndef	EXTERNAL_RELEASE

	struct List		vl_SubViews;	/*  Don't sniff this.	*/
	struct Projector	*vl_Projector;

#endif
} ViewList;


/***************************************************************************
 * TagArgs for creating/modifying a View or ViewList.
 */
enum ViewTag {
	VIEWTAG_VIEWTYPE = TAG_ITEM_LAST + 1,
	VIEWTAG_WIDTH,
	VIEWTAG_HEIGHT,
	VIEWTAG_TOPEDGE,
	VIEWTAG_LEFTEDGE,
	VIEWTAG_WINTOPEDGE,
	VIEWTAG_WINLEFTEDGE,
	VIEWTAG_BITMAP,
	VIEWTAG_PIXELWIDTH,
	VIEWTAG_PIXELHEIGHT,
	VIEWTAG_AVGMODE,
	VIEWTAG_RENDERSIGNAL,
	VIEWTAG_DISPLAYSIGNAL,
	VIEWTAG_SIGNALTASK,
	VIEWTAG_FIELDSTALL_BITMAPLINE,
	VIEWTAG_FIELDSTALL_VIEWLINE,

	VIEWTAG_BESILENT = 1024,
	MAX_VIEWTAG
};
#ifndef	EXTERNAL_RELEASE

/*
 * This is for Projectors who want to re-scan the TagArgs, and pipe them
 * through a table rather than a switch{case}.  Projectors must know to
 * break the table at VIEWTAG_BESILENT.
 */
#define	VIEWTAG_firstviewtag	VIEWTAG_VIEWTYPE

#endif


/***************************************************************************
 * The types of View that may be created.
 */
enum ViewType {
	VIEWTYPE_INVALID = 0,
	VIEWTYPE_16,
	VIEWTYPE_16_LACE,
	VIEWTYPE_32,
	VIEWTYPE_32_LACE,
	VIEWTYPE_16_640,
	VIEWTYPE_16_640_LACE,
	VIEWTYPE_32_640,
	VIEWTYPE_32_640_LACE,
	VIEWTYPE_YUV_16,
	VIEWTYPE_YUV_16_LACE,
	VIEWTYPE_YUV_32,
	VIEWTYPE_YUV_32_LACE,
	VIEWTYPE_YUV_16_640,
	VIEWTYPE_YUV_16_640_LACE,
	VIEWTYPE_YUV_32_640,
	VIEWTYPE_YUV_32_640_LACE,
	MAX_VIEWTYPE
};

#define	VIEWTYPE_PROJTYPE_MASK	0x7F000000
#define	VIEWTYPE_TRANSLUCENT	0x00000400
#define	VIEWTYPE_SQUAREPIX	0x00000200
#define	VIEWTYPE_SPECIALPIXRES	0x00000100
#define	VIEWTYPE_YRES_MASK	0x000000F0
#define	VIEWTYPE_XRES_MASK	0x0000000F
#define	VIEWTYPE_RESERVED	0x80FFF800

#define	VIEWTYPE_PROJTYPE_SHIFT	24

#define	VIEWTYPE_XRES_320	0x00000001
#define	VIEWTYPE_XRES_512	0x00000002
#define	VIEWTYPE_XRES_640	0x00000003
#define	VIEWTYPE_XRES_800	0x00000004
#define	VIEWTYPE_XRES_1024	0x00000005
#define	VIEWTYPE_XRES_1152	0x00000006
#define	VIEWTYPE_XRES_1280	0x00000007
#define	VIEWTYPE_XRES_1600	0x00000008

#define	VIEWTYPE_YRES_240	0x00000010
#define	VIEWTYPE_YRES_384	0x00000020
#define	VIEWTYPE_YRES_480	0x00000030
#define	VIEWTYPE_YRES_600	0x00000040
#define	VIEWTYPE_YRES_768	0x00000050
#define	VIEWTYPE_YRES_870	0x00000060
#define	VIEWTYPE_YRES_960	0x00000070
#define	VIEWTYPE_YRES_1024	0x00000080
#define	VIEWTYPE_YRES_1200	0x00000090


#ifndef	EXTERNAL_RELEASE
#define	VIEWTYPE_BLANK	VIEWTYPE_INVALID	/*  Shh!  Don't tell!	*/
#endif

/***************************************************************************
 * View averaging modes.
 */
#define	AVGMODE_H	1		/*  Horizontal averaging	*/
#define	AVGMODE_V	(1<<1)		/*  Vertical averaging		*/
#ifndef EXTERNAL_RELEASE
#define	AVGMODE_MASK	3
#endif

#define	AVG_DSB_0	0
#define	AVG_DSB_1	(1<<16)


/***************************************************************************
 * The ViewTypeInfo structure (public portion).
 * Describes various features available for a particular View type.
 *
 * The vti_BMTypes field points to a table of BMTYPE_ values which are
 * compatible/incompatible with this particular viewtype.  If the
 * VTIF_BMTYPES_ALLOWED flag is set, the table enumerates compatible types.
 * If unset, the table enumerates incompatible types.
 */
typedef struct ViewTypeInfo {
	int32	vti_Type;		/*  View Type			*/
	uint32	vti_Flags;		/*  Some flags			*/
	uint16	vti_MaxPixWidth,	/*  Maximum pixel width		*/
		vti_MaxPixHeight;	/*  Maximum pixel height	*/
	uint8	vti_XAspect,		/*  Pixel aspect ratio		*/
		vti_YAspect;
	uint8	vti_NBMTypes;		/*  Number of Bitmap Types	*/
	int32	*vti_BMTypes;		/*  (In)Compatible Bitmap Types */
#ifndef EXTERNAL_RELEASE

	uint8	vti_PixXSize,		/*  Pixel/Display coord factor	*/
		vti_PixYSize;

#endif
} ViewTypeInfo;

#define	VTIF_BMTYPES_ALLOWED	1	/* Inclusion instead of exclusion */
#define	VTIF_ABLE_AVG_H		(1<<1)	/* Can do horizontal pixel avg	  */
#define	VTIF_ABLE_AVG_V		(1<<2)	/* Can do vertical pixel avg	  */

#define	VTIF_RESERVED		0xFFFF0000	/* Internal bits; no peeky */


#endif	/*  __GRAPHICS_VIEW_H  */
