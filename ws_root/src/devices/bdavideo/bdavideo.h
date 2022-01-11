/*  :ts=8 bk=0
 *
 * bdavideo.h:	Common structures for BDA video Projectors.
 *
 * @(#) bdavideo.h 96/09/19 1.5
 *
 * Leo L. Schwab					9603.17
 */
#ifndef __BDAVIDEO_H
#define __BDAVIDEO_H

#ifndef __GRAPHICS_GFX_PVT_H
#include <graphics/gfx_pvt.h>
#endif


/***************************************************************************
 * Two ill-named structures.
 *
 * Proj_M2vid is the common "global" structure utilized no matter which
 * Projector is currently active.  This implies that VDLs are not saved
 * across Projector activations.  It is statically declared.
 *
 * PerProj is data specific to each Projector.  It is dynamically allocated
 * and attached to the Projector's p_ExtraData field.
 */
typedef struct Proj_M2vid {
	Transition	*pm2_Transitions;

	VDLHeader	*pm2_ForcedVDL0;/*  Hardware loads at vblank	*/
	VDLHeader	*pm2_ForcedVDL1;
	VDLHeader	*pm2_PatchVDL0;	/*  VDL to patch for display	*/
	VDLHeader	*pm2_PatchVDL1;
	VDLHeader	*pm2_EndField;	/*  Field terminator		*/

	VDLHeader	*pm2_VDL0;	/*  Currently visible VDLs	*/
	VDLHeader	*pm2_VDL1;

	Projector	*pm2_ActiveProjector;

	int32		pm2_StartY;

	struct Task * volatile	pm2_ChangeModeSigTask;
	uint32		pm2_NewDispMode;

	List		pm2_PendingList;
	List		pm2_FreeList;

	Item		pm2_DispFIRQ;
	Item		pm2_BeamFIRQ;
} Proj_M2vid;

typedef struct PerProj {
	MinNode		pp;
	List		pp_SigList;
	List		pp_StalledSigs;

	struct lineintr	pp_ZeroMarker;

	Projector	*pp_Projector;	/*  Backpointer for FIRQ.	*/
	Proj_M2vid	*pp_pm2;
	int32		pp_VBlankLines;	/*  # of lines in VBlank interval  */
	uint32		pp_HStart;

#ifdef BUILD_BDA2_VIDEO_HACK
	struct lineintr	pp_bda2hack;
#endif
} PerProj;


/***************************************************************************
 * ViewTypeInfo specific to BDA video.
 */
typedef struct BDAVTI {
	struct ViewTypeInfo	bv;
	Err			(*bv_Compile)(struct View *,
					      struct PTState *);
	Err			(*bv_Modify)(struct View *,
					     struct Transition *,
					     int32,
					     int32);
	uint32			bv_FixedVDL_DC0, bv_MaskVDL_DC0;
	uint32			bv_FixedVDL_DC1, bv_MaskVDL_DC1;
	uint32			bv_FixedVDL_AV, bv_MaskVDL_AV;
	uint32			bv_FixedVDL_LC, bv_MaskVDL_LC;
} BDAVTI;

/*
 * bv.vti_Flags values.
 */
#define	BVF_BMTYPES_ALLOWED	(1<<31)


/***************************************************************************
 * Special lineintr structure for field stalls.
 * Extends the 'lineintr' structure in <graphics/view.h> (which really
 * ought not to be there.  Oh well...).
 */
typedef struct stalllineintr {
	struct lineintr	sli_li;
	struct Task	*sli_Task;
} stalllineintr;

#define	INTRTYPE_STALL	MAX_INTRTYPE


/***************************************************************************
 * View flags for this Projector.
 */
#define	VIEWF_DISPSIGLATCHED	0x00010000
#define	VIEWF_RENDSIGIMMEDIATE	0x00020000
#define	VIEWF_RENDLINESET	0x00040000
#define	VIEWF_DISPLINESET	0x00080000
#define	VIEWF_RECOMPILED	0x00100000
#define	VIEWF_STOMPED		0x00200000
#define	VIEWF_BDA2HACK		0x00400000

#define	VIEWF_UPDATECLEAR_MASK	0x003F0000


/***************************************************************************
 * Useful macro(s).
 */
#define	GETBEAMPOS()	((BDA_READ (BDAVDU_VLOC) & VDU_VLOC_VCOUNT_MASK) >> \
			 VDU_VLOC_VCOUNT_SHIFT)
#define	GETFIELD()	((BDA_READ (BDAVDU_VLOC) & VDU_VLOC_VIDEOFIELD) != 0)


#endif
