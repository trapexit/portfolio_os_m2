#ifndef	__GRAPHICS_GFX_PVT_H
#define	__GRAPHICS_GFX_PVT_H

/***************************************************************************
**
**  @(#) gfx_pvt.h 96/08/21 1.24
**
**  Private Display folio stuff.
**
****************************************************************************/


#ifndef	__GRAPHICS_GRAPHICS_H
#include <graphics/graphics.h>
#endif

#ifndef __GRAPHICS_VIEW_H
#include <graphics/view.h>
#endif

#ifndef	__HARDWARE_M2VDL_H
#include <hardware/m2vdl.h>
#endif

#ifndef __KERNEL_FOLIO_H
#include <kernel/folio.h>
#endif

#ifndef __KERNEL_SEMAPHORE_H
#include <kernel/semaphore.h>
#endif



#ifdef BUILD_STRINGS

#define	DIAG(x)		printf x

#ifdef DEBUG
#define	DDEBUG(x)	printf x
#else
#define	DDEBUG(x)
#endif

#else

#define	DIAG(x)
#define	DDEBUG(x)

#endif




#if 0
/*  The old definition, retained for reference.  */
typedef struct ScreenNode {
	ItemNode		scr;
	struct MinNode		lnode;
	int32			width, height;
	int32			pixwidth, pixheight;
	int32			pixxsize, pixysize;
	struct ScreenNode	*receive, *handoff;
	struct List		subscreens;
	uint32			flags;
	uint32			dispmod;	/*  Display modulo bits	*/
	struct Bitmap		*bitmap;
	Item			bmi;
	int32			wintop, winleft;/*  Relative to parent.	*/
	int32			scrtop, scrleft;
	int32			absscrtop;	/*  Absolute from top.	*/
	int32			ordinal;
	int8			type;	/*  Will inherit from Bitmap	*/
	uint32			CLUT[33];
	struct Extra		*extra;
} ScreenNode;
#endif




/*
 * Commented-out fields are "design notes" for optimization of frequent
 * operations, like changing the frame buffer address for double-buffering.
 *
 * Transition and ViewEdge MUST be identical in size.
 */
typedef struct FieldTrans {
	void	*ft_VDL;	/*  Addr of VDL for this transition	*/
} FieldTrans;

typedef struct Transition {
	struct View	*t_View;	/*  Who owns this strip of display */
	int32		t_Height;	/*  Height of this strip	*/
	FieldTrans	t_FT[2];	/*  One for each field		*/
} Transition;


typedef struct ViewEdge {
	struct View	*e_View;	/*  Owner of this edge.		*/
	int32		e_Type;		/*  Type of edge		*/
	FieldTrans	e_FT[2];	/*  Padding; not used		*/
} ViewEdge;

#define	EDGETYPE_TOP	0
#define	EDGETYPE_BOTTOM	1


/*
 * ViewEdges are scanned and overlaid in place with Transitions (conserves
 * memory).  This union is used for allocations only.  ViewEdges and
 * Transitions must be the same size.
 */
typedef union TransEdge {
	struct Transition	t;
	struct ViewEdge		e;
} TransEdge;


/*
 * Token passed around by processtransitions().
 */
typedef struct PTState {
	uint32			pts_Flags;
	int32			pts_Field;
	int32			pts_Top,
				pts_Bot,
				pts_High;
	void			*pts_VDL;
	void			*pts_PrevVDL;
	void			*pts_NextVDL;
	Transition		*pts_Trans;
	struct Projector	*pts_Projector;
} PTState;


#if 0
/* Old version */
typedef struct ViewTypeInfo {
	uint32	vti_Type;		/*  The View type number	*/
	Err	(*vti_Compile)(struct View *, struct PTState *);
	Err	(*vti_Modify)(struct View *, struct Transition *, int32, int32);
	uint32	vti_Flags;
	int32	vti_PixXSize,
		vti_PixYSize;
	int8	*vti_BMTypes;		/*  (In)compatible Bitmap types	*/
	int32	vti_NBMTypes;
	uint32	vti_FixedVDL_DC0, vti_MaskVDL_DC0;
	uint32	vti_FixedVDL_DC1, vti_MaskVDL_DC1;
	uint32	vti_FixedVDL_AV, vti_MaskVDL_AV;
	uint32	vti_FixedVDL_LC, vti_MaskVDL_LC;
} ViewTypeInfo;
#define	VTIF_TWOFIELDS		(1<<1)	/*  Not sure yet...		*/
#define	VTIF_FORCEDOUBLE	(1<<2)	/*  CurPtr & PrevPtr both needed  */
#define	VTIF_BMTYPES_ALLOWED	1	/*  Inclusion instead of exclusion*/
#define	VTIF_PRIVATE		0xFFFF0000	/*  No peeky these bits  */
#endif


/*
 * Information about Bitmap types.
 */
typedef struct BMTypeInfo {
	Err	(*bti_SizeFunc)(struct Bitmap *);
	void	*(*bti_PxAddrFunc)(struct Bitmap *, uint32, uint32);
	uint32	bti_Flags;		/*  Takes BMF_ flags.		*/
	uint8	bti_PixelSize;		/*  Size of one pixel, in bytes	*/
	uint8	bti_DefaultViewType;
} BMTypeInfo;


/***************************************************************************
 * Default VDL display control bits.
 */
#define	VDL_DC_DEFAULT	(VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE) | \
			 VDL_CTL_FIELD (VDL_DC_VINTCTL, VDL_CTL_DISABLE) | \
			 VDL_CTL_FIELD (VDL_DC_DITHERCTL, VDL_CTL_DISABLE) | \
			 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_DISABLE))

#define	VDL_LC_DEFAULT	(VDL_LC_BYPASSTYPE_MSB | VDL_LC_LD_BYPASSTYPE | \
			 VDL_LC_ONEVINTDIS)


#define	VBLANKLINES	15


/***************************************************************************
 * Folio base.
 */
typedef struct GraphicsFolioBase {
	Folio		gb;

	Item		gb_FolioErrors;

	void		*gb_VRAMBase;	/*  Base of displayable RAM	*/

	Err		gb_ProjectorLoaderResult;

	VDLHeader	*gb_ForcedVDL0;	/*  Ptr to forced transfer VDL	*/
	VDLHeader	*gb_ForcedVDL1;
	VDLHeader	*gb_PatchVDL0;	/*  Ptr to patch target VDL	*/
	VDLHeader	*gb_PatchVDL1;
	VDLHeader	*gb_EndField;	/*  VDL to terminate field	*/

	VDLHeader	*gb_VDL0;	/*  VDL for field zero		*/
	VDLHeader	*gb_VDL1;	/*  VDL for field one		*/

	int32		gb_DisplayWidth;	/*  Finest addressable	*/
	int32		gb_DisplayHeight;	/*  resolution.		*/
	uint32		gb_HStart;	/*  Global HStart		*/

	ViewList	gb_MasterList;	/*  Master display list		*/
	int32		gb_MasterCount;	/*  # of View Items created	*/

	TransEdge	*gb_CurTEList;	/*  TransEdge list now visible	*/
	int32		gb_TEListSize;	/*  List size in bytes		*/

	List		gb_BitmapList;	/*  All Bitmap Items		*/

	Item		gb_Device;	/*  For the TEGraphics device	*/
	Item		gb_Driver;

	Item		gb_DisplayFIRQ;

	List		gb_ProjectorList;	/*  List of Projectors	*/
	struct Projector *gb_DefaultProjector;

	Semaphore	*gb_ProjectorListSema4;

#if 0
	/*  More later...  */
	/*  Just for testing...  */
	uint32		gb_VBLCount;
	uint32		gb_PrevField;
	uint32		gb_Misfires;
#endif

} GraphicsFolioBase;


#ifdef DYNAMIC_FOLIO_BASE

extern struct GraphicsFolioBase	*GBase;
#define	GBASE(x)	GBase->x

#else

extern struct GraphicsFolioBase	GB;
#define	GBASE(x)	GB.x

#endif


/***************************************************************************
 * Prototypes for internal goodies.
 */
Err SuperInternalCompile (struct ViewList *vl,
			  int32 nviews,
			  struct Transition **tr_retn,
			  int32 tr_size,
			  int32 *starty,
			  uint32 clearflags);
void *InternalPixelAddr (struct Bitmap *bm, uint32 x, uint32 y);
Item SetDefaultProjector (Item projectorItem);
Err SuperInternalMarkView (struct View *v, int32 top, int32 bot);
void SuperInternalSetRendCallBack (struct View *v,
				   void (*vector)(struct View *, void *),
				   void *ptr);
void SuperInternalSetDispCallBack (struct View *v,
				   void (*vector)(struct View *, void *),
				   void *ptr);


#endif	/*  __GRAPHICS_GFX_PVT_H  */
