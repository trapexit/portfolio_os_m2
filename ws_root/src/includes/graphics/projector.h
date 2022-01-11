#ifndef	__GRAPHICS_PROJECTOR_H
#define	__GRAPHICS_PROJECTOR_H

/***************************************************************************
**
**  @(#) projector.h 96/07/30 1.4
**
**  Definitions for Projector Items and their ilk.
**
****************************************************************************/


#ifndef	__KERNEL_NODES_H
#include <kernel/nodes.h>
#endif
#ifndef EXTERNAL_RELEASE
#ifndef __GRAPHICS_GFX_PVT_H
#include <graphics/gfx_pvt.h>
#endif
#endif


/***************************************************************************
 * The Projector Item structure.
 */
typedef struct Projector {
	OpeningItemNode	p;

	uint32		p_Width,	/*  View positioning coordinates*/
			p_Height;
	uint32		p_PixWidth,	/*  Finest available resolution	*/
			p_PixHeight;
	float32		p_FieldsPerSecond;	/*  Exact ratio		*/

	uint8		p_FieldsPerFrame;
	uint8		p_XAspect;	/*  Core X/Y ratio (most square)*/
	uint8		p_YAspect;

	uint8		p_Type;		/*  Projector ID		*/

	uint32		p_Flags;
#ifndef	EXTERNAL_RELEASE

	Err		(*p_Init)(struct Projector *p,
				  struct GraphicsFolioBase *gb);
	Err		(*p_Expunge)(struct Projector *p);
					/*  Turn projector on/off	*/
	Err		(*p_Lamp)(struct Projector *p, int32 op);
	Err		(*p_Scavenge)();
	Err		(*p_Update)(struct Projector *p,
				    int32 reason,
				    void *ob);
	ViewTypeInfo	*(*p_NextViewType)(struct Projector *p,
					   struct ViewTypeInfo *vti);

	ViewList	p_ViewList;	/*  Anchor for visible Views	*/
	uint32		p_ViewCount;	/*  Count of attached Views	*/
	ViewTypeInfo	*p_BlankVTI;	/*  This projector's blank	*/

	Semaphore	*p_ViewListSema4;
	uint8		p_PendingRecompile;

	void		*p_ExtraData;

#endif
} Projector;

/*
 * Flag definitions for p_Flags.
 */
#define	PROJF_ACTIVE		1	/*  This Projector is active.	*/


enum ProjectorTags {
	PROJTAG_ACTIVATE = TAG_ITEM_LAST + 1
};

#ifndef EXTERNAL_RELEASE

enum InternalProjectorTags {
	PROJTAG_WIDTH = 16384,
	PROJTAG_HEIGHT,
	PROJTAG_PIXELWIDTH,
	PROJTAG_PIXELHEIGHT,
	PROJTAG_FIELDSPERSECOND,
	PROJTAG_FIELDSPERFRAME,
	PROJTAG_XASPECT,
	PROJTAG_YASPECT,
	PROJTAG_PROJTYPE,
	PROJTAG_BLANKVIEWTYPE,
	PROJTAG_VEC_INIT,
	PROJTAG_VEC_EXPUNGE,
	PROJTAG_VEC_SCAVENGE,
	PROJTAG_VEC_LAMP,
	PROJTAG_VEC_UPDATE,
	PROJTAG_VEC_NEXTVIEWTYPE,
	MAX_PROJTAG
};


enum ProjOps {
	PROJOP_NOP,
	PROJOP_MODIFY,
	PROJOP_ADDVIEW,
	PROJOP_REMOVEVIEW,
	PROJOP_ORDERVIEW,
	PROJOP_UNLOCKDISPLAY,
	PROJOP_ACTIVATE,
	MAX_PROJOP
};

enum LampOps {
	LAMPOP_NOP,
	LAMPOP_ACTIVATE,
	LAMPOP_DEACTIVATE,
	LAMPOP_DEACTIVATE_RESCUE,
	MAX_LAMPOP
};

/*
 * Datagram passed to p_Update for PROJOP_MODIFY.
 */
typedef struct ModifyInfo {
	struct View	*mi_View;	/*  Candidate View		*/
	struct View	*mi_OldView;	/*  Previous state of View	*/
	struct TagArg	*mi_TagArgs;	/*  Args that modified View	*/
} ModifyInfo;

#endif

/*
 * First pass at new View type numbering scheme...
 *
 * 31:		reserved
 * 30-24:	Projector type ID
 * 23-11:	reserved
 * 10:		Translucency request
 * 9:		1:1 pixel aspect request
 * 8:		pixel res exception; interpret bits 7-0 specially if set
 * 7-4:		Y pixel res (from table)
 * 3-0:		X pixel res (from table)
 */
#define	PROJTYPE_SHIFT		24
#define	PROJTYPE_MASK		0x7F000000

#define	PROJTYPE_DEFAULT	0
#define	PROJTYPE_NTSC		(16 << PROJTYPE_SHIFT)
#define	PROJTYPE_NTSC_NOLACE	(17 << PROJTYPE_SHIFT)
#define	PROJTYPE_PAL		(18 << PROJTYPE_SHIFT)
#define	PROJTYPE_PAL_NOLACE	(19 << PROJTYPE_SHIFT)


#endif
