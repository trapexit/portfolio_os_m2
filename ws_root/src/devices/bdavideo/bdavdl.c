/*  :ts=8 bk=0
 *
 * bdavdl.c:	VDL generation for BDA hardware.
 *
 * @(#) bdavdl.c 96/10/07 1.17
 *
 * Leo L. Schwab					9511.28
 */
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/cache.h>
#include <kernel/super.h>
#include <kernel/lumberjack.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <hardware/m2vdl.h>

#include <graphics/projector.h>

#include "bdavideo.h"

#include "protos.h"


/***************************************************************************
 * #defines
 */
#define	UF_RECOMPILE		1
#define	UF_FORCERECOMPILE	(1<<1)
#define	UF_CALC_DC		(1<<2)
#define	UF_CALC_AV		(1<<3)
#define	UF_CALC_LC		(1<<4)
#define	UF_BESILENT		(1<<5)

#define	UF_FIELDSTALL_BITMAP	(1<<8)
#define	UF_FIELDSTALL_VIEW	(1<<9)
#define	UF_FIELDSTALL_MASK	(UF_FIELDSTALL_BITMAP | UF_FIELDSTALL_VIEW)

#define	UF_FIELDSTALL_ODDLINE	(1<<10)


#ifdef BUILD_BDA2_VIDEO_HACK
#define	CLEARFLAGS		VIEWF_BDA2HACK
#else
#define	CLEARFLAGS		0
#endif


/***************************************************************************
 * Local prototypes.
 */
static Err compilevdl (struct Projector *p);
static Err buildVDLentry (struct PTState *pts,
			  struct Transition *curtrans,
			  int32 top,
			  int32 bot,
			  int32	*ntrans);
static Err calc_dc (struct View *v);
static Err calc_av (struct View *v);
static Err calc_lc (struct View *v);
static void yankfromsignallists (struct PerProj *pp, struct View *v);
static void addviewtosignallists (struct PerProj *pp, struct View *v);
static void fieldstall (struct PerProj *pp, struct View *v, uint32 flags);
static int32 findtopofbottomspan (struct View *v);
static uint32 rescanargs (const struct TagArg *ta);

#ifdef BUILD_BDA2_VIDEO_HACK
static Err warpvdl (VDLHeader *, int32 nlines);
#endif

/***************************************************************************
 * Vobal Glariables.
 */
extern Proj_M2vid		pm2;
extern GraphicsFolioBase	*GBase;


/*  Occasionally used for debugging.
static char	*reasons[] = {
	"<no reason>",
	"PROJOP_MODIFY",
	"PROJOP_ADDVIEW",
	"PROJOP_REMOVEVIEW",
	"PROJOP_ORDERVIEW",
	"PROJOP_UNLOCKDISPLAY",
	"PROJOP_ACTIVATE",
};
*/


/***************************************************************************
 * The beasts of burden.
 */
/*
 * 'p' is the projector affected.
 * 'reason' is a numeric code explaining why this vector was called (view
 *   modified, view added/removed from ViewList, etc.)
 * 'ob' is a pointer to the relevant object whose modification triggered
 *   this call.
 * 'thing' is an optional parameter, which may indicate how 'ob' was
 *   modified (which can determine optimal pathing).
 *
 * And if I get on the ball, all this will actually get implemented.
 ***
 * 9601.15: Here's what you can expect so far (View can also be a ViewList):
 *
 * reason		ob
 * ------		------
 * PROJOP_ADDVIEW	View (added)
 * PROJOP_REMOVEVIEW	View (removed)
 * PROJOP_ORDERVIEW	View (reordered)
 * PROJOP_MODIFY	ModifyInfo
 * PROJOP_UNLOCKDISPLAY	NULL
 * PROJOP_ACTIVATE	NULL
 */
Err
update (p, reason, ob)
struct Projector	*p;
int32			reason;
void			*ob;
{
	View		*v;
	Err		err;
	uint32		flags;

/*printf ("Updating Projector @ 0x%08lx because: %s\n", p, reasons[reason]);
printf ("Object @ 0x%08lx\n", ob);*/

	/*
	 * Why are we here?  (What's our purpose in life?)
	 */
	if (reason == PROJOP_MODIFY) {
		v	= ((ModifyInfo *) ob)->mi_View;
	} else
		v	= ob;

	switch (reason) {
	case PROJOP_MODIFY:
		if (v->v.n_Type == GFX_VIEW_NODE) {
			flags = rescanargs
				 (((ModifyInfo *) ob)->mi_TagArgs);

			if ((v->v_Bitmap != NULL) ^
			    (((ModifyInfo *) ob)->mi_OldView->v_Bitmap != NULL))
				/*
				 * Bitmap was changed from NULL to non-NULL,
				 * or vice-versa.  This requires a recompile.
				 */
				flags |= UF_RECOMPILE;
		} else
			flags = UF_RECOMPILE;

		break;

	case PROJOP_ADDVIEW:
		flags = UF_RECOMPILE | UF_CALC_DC | UF_CALC_AV | UF_CALC_LC;
		break;

	case PROJOP_REMOVEVIEW:
	case PROJOP_ORDERVIEW:
		flags = UF_RECOMPILE;
		break;

	case PROJOP_UNLOCKDISPLAY:
	case PROJOP_ACTIVATE:
	default:
		flags = UF_FORCERECOMPILE;
		break;
	}

	/*
	 * Do only those things that need doing.
	 */
/*printf ("v == 0x%08lx, flags == 0x%x\n", v, flags);*/
	if (v  &&  v->v.n_Type == GFX_VIEW_NODE) {
		yankfromsignallists (p->p_ExtraData, v);

		if (flags & UF_CALC_DC)
			if ((err = calc_dc (v)) < 0)
				return (err);

		if (flags & UF_CALC_AV)
			if ((err = calc_av (v)) < 0)
				return (err);

		if (flags & UF_CALC_LC)
			if ((err = calc_lc (v)) < 0)
				return (err);

		v->v_ViewFlags &= ~VIEWF_UPDATECLEAR_MASK;
	}

	if ((flags & UF_FORCERECOMPILE)  ||  v->v.n_Next) {
		if (p->p_ViewListSema4->sem_NestCnt) {
			/*
			 * Someone locked the display; convert this to a
			 * recompile after the unlock.
			 */
			p->p_PendingRecompile++;

		} else if (p->p_Flags & PROJF_ACTIVE) {
			if (flags & UF_FIELDSTALL_MASK)
				fieldstall (p->p_ExtraData, v, flags);

			if (flags & (UF_RECOMPILE | UF_FORCERECOMPILE)) {
				if ((err = compilevdl (p)) < 0)
					return (err);
				if (v)
					v->v_ViewFlags |= VIEWF_RECOMPILED;
			} else {
/*- - - - - - - - - - - - - - -*/
if ((err = stomplists (p, v, ((ModifyInfo *) ob)->mi_OldView)) < 0)
	return (err);
v->v_ViewFlags |= VIEWF_STOMPED;
/*- - - - - - - - - - - - - - -*/
			}
		}
	}

	if (!(flags & UF_BESILENT)  &&  v)
		addviewtosignallists (p->p_ExtraData, v);

/*printf ("--Leaving projector\n");*/
	return (0);
}


static Err
compilevdl (p)
struct Projector	*p;
{
	PTState		pts;
	MinNode		*vdlbuf0, *vdlbuf1, *oldvdl;
	Transition	*t;
	Err		err;
	int32		vdlsize;
	int32		starty;
	int32		ntrans0, ntrans1;
	uint32		oldints;
	int		lacedisp;

	/*
	 * Preinitialize
	 */
	vdlbuf0 = vdlbuf1 = (MinNode *) NULL;
	ntrans0 = ntrans1 = 0;
	starty = 0;
	lacedisp = (p->p_FieldsPerFrame == 2);

	/*
	 * Flush freelist.
	 */
	while (oldints = Disable (),
	       oldvdl = (MinNode *) RemHead (&pm2.pm2_FreeList),
	       Enable (oldints),
	       oldvdl)
		SuperFreeMem (oldvdl, -1);

	/*
	 * Call-back to graphics folio for generic compilation of ViewList
	 * into Transitions.
	 */
/*printf ("SuperInternalCompile()...");*/
	if ((err = SuperInternalCompile (&p->p_ViewList,
					 p->p_ViewCount,
					 &t,
					 sizeof (Transition),
					 &starty,
					 CLEARFLAGS)) < 0)
		return (err);

/*printf ("Internal compile results: t = 0x%08lx, starty = %d\n", t, starty);*/

	pts.pts_Flags		= 0;
	pts.pts_VDL		= 0;
	pts.pts_Projector	= p;

	/*
	 * Compute required size of VDL.  Allocate buffer for it.
	 */
	pts.pts_Field = -1;	/*  Just report sizes.  */
	if ((err = processtransitions (t, starty, &pts)) < 0)
		goto ackphft;	/*  Look down.  */
	vdlsize = (int32) pts.pts_VDL;
	vdlsize += sizeof (MinNode) +
		   sizeof (FullVDL);	/* Just in case; get smarter later */
/*printf ("Sized transitions: %d bytes\n", vdlsize);*/

	if (!(vdlbuf0 = SuperAllocMem (vdlsize, MEMTYPE_TRACKSIZE))) {
		err = GFX_ERR_NOMEM;
		goto ackphft;	/*  Look down.  */
	}
	vdlbuf0++;	/*  Skip over MinNode	*/
/*printf ("Allocated VDL RAM (field 0).\n");*/

	/*
	 * Compute field zero.
	 */
	pts.pts_VDL	=
	pts.pts_NextVDL	= vdlbuf0;
	pts.pts_PrevVDL	= 0;
	pts.pts_Field	= 0;
	if ((err = processtransitions (t, starty, &pts)) < 0)
		goto ackphft;	/*  Look down.  */
	ntrans0 = err;
/*printf ("Processed transitions field 0 (ntrans0 = %d).\n", ntrans0);*/

	if (lacedisp) {
		/*
		 * Compute field one (for interlaced displays).
		 */
		if (!(vdlbuf1 = SuperAllocMem (vdlsize, MEMTYPE_TRACKSIZE)))
		{
			err = GFX_ERR_NOMEM;
			goto ackphft;	/*  Look down.  */
		}
		vdlbuf1++;
/*printf ("Allocated VDL RAM (field 1).\n");*/

		pts.pts_VDL	=
		pts.pts_NextVDL	= vdlbuf1;
		pts.pts_PrevVDL	= 0;
		pts.pts_Field	= 1;
		if ((err = processtransitions (t, starty, &pts)) < 0)
			goto ackphft;	/*  Look down.  */
		ntrans1 = err;
/*printf ("Processed transitions field 1 (ntrans1 = %d).\n", ntrans1);*/
	} else {
		vdlbuf1 = vdlbuf0;
		ntrans1 = ntrans0;
	}

	err = 0;
ackphft:
	if (!err) {
		/*
		 * Here's what be happenin':
		 * Free old transitions, save current transitions.
		 * Patch new VDLs into system links.
		 * Schedule old VDLs for deletion by placing on _PendingList.
		 */
		SuperFreeMem (pm2.pm2_Transitions, TRACKED_SIZE);
		pm2.pm2_Transitions = t;
		pm2.pm2_StartY = starty;

		/*  Next operations atomic.  */
		oldints = Disable ();

		/*
		 * Patch in new VDLs.
		 */
		pm2.pm2_PatchVDL0->NextVDL = (VDLHeader *) vdlbuf1;
		pm2.pm2_PatchVDL1->NextVDL = (VDLHeader *) vdlbuf0;
		/*  Write back cache  */
#if 0
		_dcbst (&pm2.pm2_PatchVDL0->NextVDL);
		_dcbst (&pm2.pm2_PatchVDL1->NextVDL);
#endif
#ifdef BUILD_BDA2_VIDEO_HACK
		/*
		 * ### HACK for BDA2.0
		 * If there was only one transition, extend the VDL
		 * and stuff it directly into the hardware.
		 * (This apparent field phase mismatch is actually correct.)
		 */
	    {
	    	PerProj	*pp;

	    	pp = p->p_ExtraData;

		if (pp->pp_bda2hack.li_Node.n_Next) {
			/*
			 * The hack has been installed; warp the VDLs.
			 */
			if (ntrans0 == 1) {
				if (warpvdl ((VDLHeader *) vdlbuf0,
					     pp->pp_VBlankLines) >= 0)
					BDA_WRITE (BDAVDU_FV1A,
						   (vuint32) vdlbuf0);
			} else
				BDA_WRITE (BDAVDU_FV1A,
					   (vuint32) pm2.pm2_ForcedVDL1);

			if (ntrans1 == 1) {
				if (lacedisp) {
					if (warpvdl ((VDLHeader *) vdlbuf1,
						     pp->pp_VBlankLines) >= 0)
						goto write1;  /* Look down. */
				} else
write1:					BDA_WRITE (BDAVDU_FV0A,
						   (vuint32) vdlbuf1);
			} else
				BDA_WRITE (BDAVDU_FV0A,
					   (vuint32) pm2.pm2_ForcedVDL0);
		}
	    }
#else
		TOUCH (ntrans0);
		TOUCH (ntrans1);
#endif

		FlushDCacheAll (0);

		/*
		 * Schedule old VDLs for deletion on next VBlank.
		 * The pre-decrement is to get back to the MinNodes
		 * just before the VDLs.
		 */
		if (oldvdl = (MinNode *) pm2.pm2_VDL0)
			AddTail (&pm2.pm2_PendingList, (Node *) (--oldvdl));
		if ((oldvdl = (MinNode *) pm2.pm2_VDL1)  &&
		    pm2.pm2_VDL1 != pm2.pm2_VDL0)
			AddTail (&pm2.pm2_PendingList, (Node *) (--oldvdl));

		Enable (oldints);

		pm2.pm2_VDL0 = (VDLHeader *) vdlbuf0;
		pm2.pm2_VDL1 = (VDLHeader *) vdlbuf1;
	} else {
		/*
		 * Something went wrong; free VDL and Transition buffers.
		 * Leave display unchanged.
		 */
/*printf ("Compiler fell over 0x%08lx\n", err);*/
		if (vdlbuf0)			SuperFreeMem (--vdlbuf0, -1);
		if (vdlbuf1  &&  lacedisp)	SuperFreeMem (--vdlbuf1, -1);
		SuperFreeMem (t, TRACKED_SIZE);
	}

/*printf ("Leaving compiler.\n\n");*/
	return (err);
}


Err
processtransitions (curtrans, transtop, pts)
struct Transition	*curtrans;
int32			transtop;
struct PTState		*pts;
{
	Transition	blanktrans;
	Err		err;
	int32		top, bot, transbot;
	int32		disphigh;
	int32		ntrans;
#ifdef BUILD_BDA2_VIDEO_HACK
	View		*hackview = NULL;
#endif

	err = 0;
	ntrans = 0;
	disphigh = pts->pts_Projector->p_Height;
	blanktrans.t_View = NULL;
	blanktrans.t_Height = 0;

	/*
	 * Handle leading blanks.
	 */
	if (transtop > 0) {
		blanktrans.t_Height = transtop > disphigh  ?  disphigh  :
							      transtop;
		if ((err = buildVDLentry (pts,
					  &blanktrans,
					  0, blanktrans.t_Height,
					  &ntrans)) < 0)
			return (err);
	}

	while (curtrans->t_Height) {
/*printf ("curtrans, .owner, top, .height:  0x%08lx, 0x%08lx, %d, %d\n", curtrans, curtrans->t_View, transtop, curtrans->t_Height);*/
		if (transtop < disphigh  &&
		    (transbot = transtop + curtrans->t_Height) > 0)
		{
			/*
			 * Bottom of transition is within visible display.
			 * Clip to visible limits.
			 */
			if (transtop < 0)	top = 0;
			else			top = transtop;

			if (transbot > disphigh)	bot = disphigh;
			else				bot = transbot;

			if ((err = buildVDLentry
				    (pts, curtrans, top, bot, &ntrans)) < 0)
				return (err);
#ifdef BUILD_BDA2_VIDEO_HACK
			hackview = curtrans->t_View;
#endif
		} else {
 			if (pts->pts_Field >= 0)
				curtrans->t_FT[pts->pts_Field].ft_VDL = NULL;
		}

		transtop = transbot;
		curtrans++;
	}

	/*
	 * If transitions don't fill display, fill to bottom of display with
	 * blank.
	 */
	if (transtop < disphigh) {
		blanktrans.t_Height = disphigh - transtop;

		if ((err = buildVDLentry (pts,
					  &blanktrans,
					  transtop, disphigh,
					  &ntrans)) < 0)
			return (err);
	}

	/*
	 * Patch to field terminator.
	 */
	if (pts->pts_Field >= 0) {
		((VDLHeader *) pts->pts_PrevVDL)->NextVDL = pm2.pm2_EndField;
	}
/*printf ("EOT\n\n");*/

	if (err < 0)
		return (err);
	else {
#ifdef BUILD_BDA2_VIDEO_HACK
		if (ntrans == 1  &&  pts->pts_Field >= 0  &&  hackview)
			hackview->v_ViewFlags |= VIEWF_BDA2HACK;
#endif
		return (ntrans);
	}
}

static Err
buildVDLentry (pts, curtrans, top, bot, ntrans)
struct PTState		*pts;
struct Transition	*curtrans;
int32			top, bot;
int32			*ntrans;
{
	BDAVTI	*bv;
	View	*owner;
	Err	err;

	if (owner = curtrans->t_View)
		if ((err = SuperInternalMarkView (owner, top, bot)))
			return (err);

	if (!owner  ||  !owner->v_Bitmap)
		/*
		 * No View or no Bitmap attached; convert to blank.
		 */
		bv = (BDAVTI *) pts->pts_Projector->p_BlankVTI;
	else
		bv = (BDAVTI *) owner->v_ViewTypeInfo;

	/*
	 * Compile VDL instructions.
	 */
	pts->pts_Trans = curtrans;
	pts->pts_High = (pts->pts_Bot = bot) -
			(pts->pts_Top = top);
	err = (bv->bv_Compile) (owner, pts);
	if (err < 0)
		return (err);

	else if (!err) {
		if (pts->pts_Field >= 0) {
/*- - - - - - - - - - -*/
curtrans->t_FT[pts->pts_Field].ft_VDL = pts->pts_VDL;
((VDLHeader *) (pts->pts_VDL))->NextVDL = (VDLHeader *) pts->pts_NextVDL;
(*ntrans)++;
/*- - - - - - - - - - -*/
		}

		pts->pts_PrevVDL = pts->pts_VDL;
		pts->pts_VDL = pts->pts_NextVDL;
	}
	return (0);
}



/*
 * cvu == create vdl unit
 * Error returns:
 * A negative error code is a real live bad-news error.
 * A positive error code means no VDL was generated and the pointers
 * shouldn't be advanced.
 * Zero means no error.
 */

Err
cvu_16_32 (v, pts)
struct View		*v;
struct PTState		*pts;
{
	PerProj		*pp;
	ShortVDL	*sv;
	Transition	*t;
	int32		top, left, high, field;
	uint32		avbits;
	int		lacedisp;

	t = pts->pts_Trans;
	sv = pts->pts_VDL;
	top = pts->pts_Top;
	high = pts->pts_High;
	field = pts->pts_Field;
	pp = pts->pts_Projector->p_ExtraData;

	if (field < 0)
		/*  Only calculating sizes.  */
		goto advance;	/*  Look down.  */

	if (lacedisp = (pts->pts_Projector->p_FieldsPerFrame == 2)) {
		/*
		 * Compute values for two-field (interlaced) displays.
		 */
		if (high & 1)
			/*
			 * Factor in field-dependent fencepost for odd
			 * heights.
			 */
			high += !((top & 1) ^ field);
		if (!(high >>= 1)) {
			/*
			 * Oops!  Nothing to do.  Don't advance pointers.
			 */
			t->t_FT[field].ft_VDL = NULL;
			return (1);
		}

		/*  Calc exact line # for top edge.  */
		if ((top & 1) ^ field)
			top++;

		top = ((top - v->v_AbsTopEdge) >> 1) + v->v_WinTop;
	} else {
		/*
		 * Compute values for single-field ("progressive") displays.
		 */
		if (!high) {
			/*  Nothing to do; don't advance pointers.  */
			t->t_FT[field].ft_VDL = NULL;
			return (1);
		}
		top = top - v->v_AbsTopEdge + v->v_WinTop;
	}

	/*
	 * Create VDL.
	 * ### Explore pre-computing the modulo field.
	 */
	sv->sv.DMACtl	= VDL_DMA_ENABLE | VDL_DMA_LDLOWER |
			  VDL_NWORDS_SHORT_FMT |
			  VDL_DMA_MOD_FIELD
			   (v->v_Bitmap->bm_Width *
			    v->v_Bitmap->bm_TypeInfo->bti_PixelSize) |
			  VDL_DMA_NLINESFIELD (high);
	sv->sv_DispCtl0	= v->v_DispCtl0;
	sv->sv_DispCtl1	= v->v_DispCtl1;
	sv->sv_ListCtl	= v->v_ListCtl;

	avbits = v->v_AVCtl;
	left = v->v_WinLeft;
	if (v->v_LeftEdge < 0)
		left -= v->v_LeftEdge / v->v_ViewTypeInfo->vti_PixXSize;

	if (((v->v_AvgMode0 | v->v_AvgMode1) & AVGMODE_V)  &&
	    ((v->v_TopEdge & 1) ^ field ^ 1)  &&
	    lacedisp)
	{
		/*
		 * Compiler module is completely responsible for handling
		 * bit settings for vertical interpolation.
		 * (Vertical interpolation only makes sense for two-field
		 * (interlaced) displays.)
		 */
		sv->sv.DMACtl	|= VDL_DMA_LDUPPER;
		if (v->v_AvgMode0 & AVGMODE_V)
			sv->sv_DispCtl0	|= VDL_CTL_FIELD (VDL_DC_VINTCTL,
							  VDL_CTL_ENABLE);
		if (v->v_AvgMode1 & AVGMODE_V)
			sv->sv_DispCtl1	|= VDL_CTL_FIELD (VDL_DC_VINTCTL,
							  VDL_CTL_ENABLE);
		avbits |= VDL_AV_VDOUBLE;
	}
	sv->sv.LowerPtr	=
	sv->sv.UpperPtr = InternalPixelAddr (v->v_Bitmap, left, top);

	sv->sv_AVCtl = avbits | VDL_AV_FIELD (VDL_AV_HSTART,
					      pp->pp_HStart +
					       (v->v_LeftEdge < 0  ?
						0  :
						v->v_LeftEdge));

advance:
	pts->pts_NextVDL = sv + 1;
	return (0);
}


Err
cvu_16_32_lace (v, pts)
struct View	*v;
struct PTState	*pts;
{
	PerProj		*pp;
	ShortVDL	*sv;
	Transition	*t;
	int32		top, left, high, field;

	t = pts->pts_Trans;
	sv = pts->pts_VDL;
	top = pts->pts_Top;
	high = pts->pts_High;
	field = pts->pts_Field;
	pp = pts->pts_Projector->p_ExtraData;

	if (field < 0)
		goto advance;	/*  Look down.  */

	if (high & 1)
		/*  Factor in field-dependent fencepost for odd heights.  */
		high += !((top & 1) ^ field);
	if (!(high >>= 1)) {
		t->t_FT[field].ft_VDL = NULL;
		return (1);
	}

	/*  Calc exact line # for top edge.  */
	if ((top & 1) ^ field)
		top++;

	top = top - v->v_AbsTopEdge + v->v_WinTop;
	left = v->v_WinLeft;
	if (v->v_LeftEdge < 0)
		left -= v->v_LeftEdge / v->v_ViewTypeInfo->vti_PixXSize;

	/*
	 * Create VDL.
	 */
	sv->sv.DMACtl	= VDL_DMA_ENABLE | VDL_DMA_LDLOWER |
			  VDL_NWORDS_SHORT_FMT |
			  VDL_DMA_MOD_FIELD
			   (v->v_Bitmap->bm_Width *
			    v->v_Bitmap->bm_TypeInfo->bti_PixelSize * 2) |
			  VDL_DMA_NLINESFIELD (high);
	sv->sv_DispCtl0	= v->v_DispCtl0;
	sv->sv_DispCtl1	= v->v_DispCtl1;
	sv->sv_AVCtl	= v->v_AVCtl |
			  VDL_AV_FIELD (VDL_AV_HSTART,
					pp->pp_HStart +
					 (v->v_LeftEdge < 0  ?
					  0  :
					  v->v_LeftEdge));
	sv->sv_ListCtl	= v->v_ListCtl;

	sv->sv.LowerPtr	=
	sv->sv.UpperPtr = InternalPixelAddr (v->v_Bitmap, left, top);

#if 0
/*  Old LRFORM method  */
	otheraddr = (void *) ((uint32) bufaddr ^ 2);
	if (field) {
		if (pts->pts_Flags & VTIF_FORCEDOUBLE)
			sv->sv.DMACtl	|= VDL_DMA_LDUPPER | VDL_DMA_NOBUCKET;
		sv->sv.LowerPtr	= otheraddr;
		sv->sv.UpperPtr	= bufaddr;
	} else {
		sv->sv.LowerPtr	= bufaddr;
		sv->sv.UpperPtr	= otheraddr;
	}
	pts->pts_Flags |= VTIF_FORCEDOUBLE;
#endif

advance:
	pts->pts_NextVDL = sv + 1;

	return (0);
}


/*
 * This routine implements blank display regions by turning off video DMA.
 * I remember a note somewhere about the Creative board observing this
 * wrong, redisplaying old captured data instead.  If this is the case,
 * I can simply display low VRAM with a CLUT of all zeros.
 **
 * 9503.31:  Okay, it doesn't turn DMA off anymore...
 **
 * 9510.16:  Um, it's turning off DMA again.  If something like the Creative
 * board can't handle this, we're screwed.
 **
 * 9601.13:  Consider yanking need for View at all by hard-coding VDL words.
 * (Should work, since blank is blank.)
 **
 * 9606.29:  Huzzah!  internal_blank View went away; this now creates blank
 * spans without any outside help (so to speak).
 */
Err
cvu_blank (v, pts)
struct View	*v;
struct PTState	*pts;
{
	ShortVDL	*sv;
	int32		high;

	TOUCH (v);
	sv = pts->pts_VDL;
	high = pts->pts_High;

	if (pts->pts_Field < 0)
		goto advance;	/*  Look down.  */

	if (pts->pts_Projector->p_FieldsPerFrame == 2) {
		/*
		 * Compute values for two-field (interlaced) displays.
		 */
		if (high & 1)
			/*
			 * Factor in field-dependent fencepost for odd
			 * heights.
			 */
			high += !((pts->pts_Top & 1) ^ pts->pts_Field);
		if (!(high >>= 1))
			return (1);
	}

	/*
	 * Create VDL.
	 */
	sv->sv.DMACtl	= VDL_DMA_LDLOWER |
			  VDL_NWORDS_SHORT_FMT |
			  VDL_DMA_MOD_FIELD (0) |
			  VDL_DMA_NLINESFIELD (high);
	sv->sv.LowerPtr	=
	sv->sv.UpperPtr	= GBase->gb_VRAMBase;

	sv->sv_DispCtl0	= VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT;
	sv->sv_DispCtl1	= VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT;
	sv->sv_AVCtl	= VDL_AV_NOP;
	sv->sv_ListCtl	= VDL_LC | VDL_LC_ONEVINTDIS;

advance:
	pts->pts_NextVDL = sv + 1;

	return (0);
}

/*
 * mvu == modify VDL unit.
 */
Err
mvu_16_32_dual (v, t, field, delta)
struct View		*v;
struct Transition	*t;
int32			field, delta;
{
	ShortVDL	*sv;

	TOUCH (v);
	if (sv = t->t_FT[field].ft_VDL) {
		sv->sv.LowerPtr = (void *)((uint32) sv->sv.LowerPtr + delta);
		sv->sv.UpperPtr = (void *)((uint32) sv->sv.UpperPtr + delta);
		_dcbst (&sv->sv.LowerPtr);	/*  Cache dump  */
		_dcbst (&sv->sv.UpperPtr);
	}

	return (0);
}


/***************************************************************************
 * VDL control word generators.
 */
static Err
calc_dc (v)
struct View	*v;
{
	BDAVTI	*bv;
	uint32	dc0, dc1;

	/*
	 * VINTCTL is set in the compiler.
	 */
	dc0 = (VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT) & ~VDL_DC_VINTCTL_MASK;
	if (v->v_AvgMode0 & AVGMODE_H)
		dc0 = (dc0 & ~VDL_DC_HINTCTL_MASK) |
		      VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_ENABLE);

	dc1 = (VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT) & ~VDL_DC_VINTCTL_MASK;
	if (v->v_AvgMode1 & AVGMODE_H)
		dc1 = (dc1 & ~VDL_DC_HINTCTL_MASK) |
		      VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_ENABLE);

	bv = (BDAVTI *) v->v_ViewTypeInfo;

	if (bv->bv_MaskVDL_DC0) {
		dc0 &= ~bv->bv_MaskVDL_DC0;
		dc0 |= bv->bv_FixedVDL_DC0;
	}

	if (bv->bv_MaskVDL_DC1) {
		dc1 &= ~bv->bv_MaskVDL_DC1;
		dc1 |= bv->bv_FixedVDL_DC1;
	}

	v->v_DispCtl0 = dc0;
	v->v_DispCtl1 = dc1;

	return (0);
}

static Err
calc_av (v)
struct View	*v;
{
	BDAVTI	*bv;
	int32	hstart, hwidth;
	uint32	avctl;

	hstart = ((PerProj *) v->v_Projector->p_ExtraData)->pp_HStart +
		 v->v_LeftEdge;
	if (hstart > (VDL_AV_HSTART_MASK >> VDL_AV_HSTART))
		return (GFX_ERR_INTERNAL);

	bv = (BDAVTI *) v->v_ViewTypeInfo;

	/*
	 * Clip HWIDTH against bounds of Projector.
	 */
	/*  Clip against left edge.  */
	hwidth = v->v_PixWidth * bv->bv.vti_PixXSize;
	if (v->v_LeftEdge < 0)
		hwidth += v->v_LeftEdge;

	/*  Clip against right edge.  */
	if (hwidth + v->v_LeftEdge > (int32) v->v_Projector->p_Width)
		hwidth -= hwidth + v->v_LeftEdge -
			  (int32) v->v_Projector->p_Width;

	hwidth /= bv->bv.vti_PixXSize;

	/*
	 * HSTART and VDOUBLE are set in the compiler, since it can be
	 * smarter about these things.
	 ***
	 * ewhac 9609.26:  I'm not sure that's necessary anymore...
	 */
	avctl = VDL_AV |
		VDL_AV_FIELD (VDL_AV_HWIDTH, hwidth) |
		VDL_AV_LD_HSTART | VDL_AV_LD_HWIDTH |
		VDL_AV_LD_HDOUBLE | VDL_AV_LD_VDOUBLE;

	if (bv->bv_MaskVDL_AV) {
		avctl &= ~bv->bv_MaskVDL_AV;
		avctl |= bv->bv_FixedVDL_AV;
	}

	v->v_AVCtl = avctl;

	return (0);
}

static Err
calc_lc (v)
struct View	*v;
{
	BDAVTI	*bv;
	uint32	lcctl;

	lcctl = VDL_LC | VDL_LC_DEFAULT;

	bv = (BDAVTI *) v->v_ViewTypeInfo;

	if (bv->bv_MaskVDL_LC) {
		lcctl &= ~bv->bv_MaskVDL_LC;
		lcctl |= bv->bv_FixedVDL_LC;
	}

	v->v_ListCtl = lcctl;

	return (0);
}


/***************************************************************************
 * Remove and install View into lists for later signal dispatching.
 */
static void
yankfromsignallists (pp, v)
struct PerProj	*pp;
struct View	*v;
{
	register lineintr	*li;
	register uint32		oldints;

	/*
	 * Modifying a View aborts all pending signals.
	 * Remove signalling nodes from any list they may be in.
	 */
	oldints = Disable ();

	li = &v->v_DispLineIntr;
	if (li->li_Node.n_Next)
		removelineintr (pp, li);

	li = &v->v_RendLineIntr;
	if (li->li_Node.n_Next)
		removelineintr (pp, li);

	v->v_ViewFlags &= ~(VIEWF_PENDINGSIG_REND | VIEWF_PENDINGSIG_DISP);

	Enable (oldints);
}


static void
addviewtosignallists (pp, v)
struct PerProj	*pp;
struct View	*v;
{
	register struct lineintr	*li;
	register uint32			vflags;
	uint32				oldints;

	if (v->v.n_Type != GFX_VIEW_NODE)
		/*  Only Views can generate signals.  */
		return;

	oldints = Disable ();

	li = &v->v_RendLineIntr;

	vflags = v->v_ViewFlags;

	if (v->v_RendSig  ||  v->v_DispSig  ||
	    (vflags & (VIEWF_FORCESCHEDSIG_REND | VIEWF_FORCESCHEDSIG_DISP)))
	{
/*- - - - - - -*/
if (!v->v.n_Next  ||
    !(vflags & VIEWF_VISIBLE)  ||
    !(v->v_Projector->p_Flags & PROJF_ACTIVE))
{
	/*
	 * If the view is not attached to a ViewList, or it's
	 * invisible, send the Rendersignal now and don't
	 * schedule a Displaysignal (since the View isn't
	 * being scanned by the video beam).
	 */
	dispatchlinesignal (pp, li);
	goto outahere;	/*  Look down.  */
}


/*
 * Handle Rendersignals.
 * ('li' still pointing at _RendLineIntr)
 */
if (v->v_RendSig  ||  (vflags & VIEWF_FORCESCHEDSIG_REND)) {
	if (vflags & VIEWF_STOMPED) {
		/*
		 * The stomper did most of the work for us.  All we need do
		 * now is inspect the clues it left us and do the
		 * appropriate thing.
		 */
		if (vflags & VIEWF_RENDSIGIMMEDIATE) {
			/*
			 * Beam is not scanning this View; signal now.
			 */
			dispatchlinesignal (pp, li);
		}
		else if (vflags & VIEWF_RENDLINESET) {
			/*
			 * Beam is in this View; send signal this field at
			 * bottom of current Transition (li_Line has been
			 * set by the stomper).
			 */
			installlineintr (pp, li, 0);
			goto setpendingrend;	/*  Look down.  */
		}
	}
	else if (vflags & VIEWF_RECOMPILED) {
		/*
		 * Display has been recompiled.  Send signal when the beam
		 * can no longer display any part of the View for remainder
		 * of this field.
		 */
		if (GETBEAMPOS () < disp2hw (pp, v->v_PrevVisBotEdge)) {
			/*
			 * Beam is above bottom line of bottom-most
			 * Transition for this View (before recompilation).
			 * Signal when beam passes it.
			 */
			li->li_Line = v->v_VisBotEdge;
			installlineintr (pp, li, 0);
setpendingrend:		vflags |= VIEWF_PENDINGSIG_REND;
		} else
			/*
			 * Beam is can no longer scan View during this
			 * field; send signal now.
			 */
			dispatchlinesignal (pp, li);
	}
}


/*
 * Handle Displaysignals
 */
li = &v->v_DispLineIntr;
if (v->v_DispSig  ||  (vflags & VIEWF_FORCESCHEDSIG_DISP)) {
	if (vflags & VIEWF_STOMPED) {
		/*
		 * Send signal at top of next Transition for this View.
		 * If no further Transitions for this field, schedule for
		 * next field.  (The stomper has done most of this work
		 * for us.)
		 */
		if (vflags & VIEWF_DISPLINESET) {
			/*
			 * Already worked out for us (li_Line set by
			 * stomper).
			 */
			installlineintr (pp, li, 0);
			goto setpendingdisp;	/*  Look down.  */
		}
		else if (vflags & VIEWF_VISIBLE) {
			/*
			 * No further Transitions this field; schedule at
			 * topmost Transition for next field.  (The test for
			 * the _VISIBLE flag is (theoretically) redundant.)
			 */
			li->li_Line = v->v_VisTopEdge;
			installlineintr (pp, li, 1);
			goto setpendingdisp;	/*  Look down.  */
		}
		/*
		 * No signal is sent if the View is invisible.
		 */
	} else {
		/*
		 * Display has been recompiled.  Send signal at top of
		 * topmost Transition at next field.
		 */
		if (vflags & VIEWF_VISIBLE) {
			li->li_Line = v->v_VisTopEdge;
			installlineintr (pp, li, 1);
setpendingdisp:		vflags |= VIEWF_PENDINGSIG_DISP;
		}
		/*
		 * No signal is sent if the View is invisible.
		 */
	}
}
/*- - - - - - -*/
	}
outahere:
	v->v_ViewFlags = vflags;
	Enable (oldints);
}





/***************************************************************************
 * This function is for delaying things until we're ready to stomp or
 * replace the VDL lists.  This is used for clients who wish to do field-
 * sensitive page-flipping.
 *
 * This function will not return until it is safe to stomp/replace the lists.
 *
 * ### FIXME:  WARNING!!!
 * With the addition of this facility, this entire facility is no longer
 * single-threaded!!  The code has NOT been updated to be multi-threaded.
 * Thus, it is possible for a client to use VIEWTAG_FIELDSTALL_* and enter
 * a wait state while another client dives in and makes changes.  The code
 * has NO knowledge of this possibility.  Further, you can't put a sempahore
 * on it, because the $%*#&!! triangle engine device wants to call this from
 * within an interrupt.
 *
 * This is probably something that should be addressed by my successor, if
 * any.
 *	-- ewhac 9609.19
 */
static void
fieldstall (pp, v, flags)
struct PerProj	*pp;
struct View	*v;
uint32		flags;
{
	stalllineintr	sli;
	uint32		ybias, fieldreqd;

	if (pp->pp_Projector->p_FieldsPerFrame == 1)
		/*
		 * Not an interlaced Projector; don't stall.
		 */
		return;

	if (flags & UF_FIELDSTALL_BITMAP) {
		if (v->v_ViewTypeInfo->vti_PixYSize != 1)
			/*
			 * Not an interlaced View; don't stall.
			 */
			return;

		ybias = v->v_WinTop;
	} else
		ybias = 0;

	ybias += (flags & UF_FIELDSTALL_ODDLINE) != 0;

	/*
	 * The extra '1' at the end is to invert us to the hardware's notion
	 * of fields 0 and 1.
	 */
	fieldreqd = (ybias & 1) ^ (v->v_TopEdge & 1) ^ 1;

	sli.sli_li.li_Type = INTRTYPE_STALL;
	sli.sli_Task = CURRENTTASK;
/*printf ("fieldreqd, thisfield:  %d, %d\n", fieldreqd, GETFIELD ());*/
	if (fieldreqd == GETFIELD ()) {
		/*
		 * We stall no matter what; it's just a question of where.
		 */
		if (flags & (UF_RECOMPILE | UF_FORCERECOMPILE))
			sli.sli_li.li_Line = 0;
		else
			sli.sli_li.li_Line = findtopofbottomspan (v);

		installlineintr (pp, (lineintr *) &sli, 1);

	} else {
		/*
		 * The *next* field is the one we care about.
		 * We may or may not stall...
		 */
		if (flags & (UF_RECOMPILE | UF_FORCERECOMPILE)) {
			/*
			 * A recompiled list gets plugged in at the next
			 * VBlank, which is exactly what we want.
			 */
			return;
		} else {
			sli.sli_li.li_Line = findtopofbottomspan (v);
			if (GETBEAMPOS () >= disp2hw (pp, sli.sli_li.li_Line))
				/*
				 * We can do it right now.
				 */
				return;

			/*
			 * We need to wait for the beam to fall below the
			 * last span before we can stomp the lists.
			 */
			installlineintr (pp, (lineintr *) &sli, 0);
		}
	}
	while (sli.sli_Task)
		WaitSignal (SIGF_ONESHOT);
}


/*
 * For a given View, returns the top edge of the bottom-most visible span.
 */
static int32
findtopofbottomspan (v)
struct View	*v;
{
	register Transition	*t;
	register int32		topedge, edgeval;

	edgeval = -1;
	topedge = pm2.pm2_StartY;
	for (t = pm2.pm2_Transitions;  t->t_Height;  t++) {
		if (t->t_View == v)
			edgeval = topedge;

		topedge += t->t_Height;
	}

	return (edgeval);
}


/***************************************************************************
 * TagArg re-scanner.
 * This helps us determine what we have to do, and do only those things.
 */
static uint8	states[] = {
/* VIEWTAG_VIEWTYPE	*/	UF_RECOMPILE |  UF_CALC_DC |
						UF_CALC_AV |
						UF_CALC_LC,
/* VIEWTAG_WIDTH	*/	UF_RECOMPILE |  UF_CALC_AV,
/* VIEWTAG_HEIGHT	*/	UF_RECOMPILE,
/* VIEWTAG_TOPEDGE	*/	UF_RECOMPILE,
/* VIEWTAG_LEFTEDGE	*/	UF_RECOMPILE |  UF_CALC_AV,
/* VIEWTAG_WINTOPEDGE	*/	0,
/* VIEWTAG_WINLEFTEDGE	*/	0,
/* VIEWTAG_BITMAP	*/	0,
/* VIEWTAG_PIXELWIDTH	*/	UF_RECOMPILE |  UF_CALC_AV,
/* VIEWTAG_PIXELHEIGHT	*/	UF_RECOMPILE,
/* VIEWTAG_AVGMODE	*/	UF_RECOMPILE |	/*  Some day, maybe not... */
						UF_CALC_DC |
						UF_CALC_AV,
/* VIEWTAG_RENDERSIGNAL	*/	0,
/* VIEWTAG_DISPLAYSIGNAL*/	0,
/* VIEWTAG_SIGNALTASK	*/	0,
/* VIEWTAG_FIELDSTALL.. */	0,
/* VIEWTAG_FIELDSTALL.. */	0,
};
#define	NSTATES		(sizeof (states) / sizeof (uint8))


static uint32
rescanargs (
const struct TagArg	*ta
)
{
	const TagArg	*state;
	register uint32	tag, flags;

	state = ta;
	flags = 0;
	while (ta = NextTagArg (&state)) {
		tag = ta->ta_Tag;
		if (tag == VIEWTAG_BESILENT) {
			if (ta->ta_Arg)
				flags |= UF_BESILENT;
		} else if (tag == VIEWTAG_FIELDSTALL_BITMAPLINE  ||
			   tag == VIEWTAG_FIELDSTALL_VIEWLINE)
		{
			flags &= ~UF_FIELDSTALL_MASK;
			if (tag == VIEWTAG_FIELDSTALL_BITMAPLINE)
				flags |= UF_FIELDSTALL_BITMAP;
			else
				flags |= UF_FIELDSTALL_VIEW;

			if ((uint32) ta->ta_Arg & 1)
				flags |= UF_FIELDSTALL_ODDLINE;
		} else {
			tag -= VIEWTAG_firstviewtag;
			if (tag >= 0  &&  tag < NSTATES)
				flags |= states[tag];
		}
	}
	return (flags);
}

#ifdef BUILD_BDA2_VIDEO_HACK

/***************************************************************************
 * ### FIXME ### FIXME ###
 * DISGUSTING HACK!!
 * This is for BDA2.0 silicon.  Future silicon better not have this problem.
 * (And BDA2.0 better not end up in consumer units.)
 * (Assumes someone else flushes the cache.)
 */
static Err
warpvdl (vdlbuf, nlines)
VDLHeader	*vdlbuf;
int32		nlines;
{
	int32	bpr;
	uint32	lwr, upr;

	bpr = ((vdlbuf->DMACtl & VDL_DMA_MOD_MASK) >> VDL_DMA_MOD_SHIFT) << 5;

	lwr = (int32) vdlbuf->LowerPtr - bpr * nlines;
	upr = (int32) vdlbuf->UpperPtr - bpr * nlines;

	if ((!IsMemReadable ((void *) lwr, sizeof (int32)))  ||
	    (!IsMemReadable ((void *) upr, sizeof (int32))))
		return (GFX_ERR_NOMEM);

	vdlbuf->LowerPtr = (void *) lwr;
	vdlbuf->UpperPtr = (void *) upr;
	vdlbuf->DMACtl += nlines;	/*  Just happens to work.  */

	return (0);
}
#endif
