/*  :ts=8 bk=0
 *
 * stompomatic.c:	In-place VDL modification (and other stuff).
 *
 * @(#) stompomatic.c 96/09/30 1.7
 *
 * Leo L. Schwab					9505.23
 *  Converted for Projector use				9603.12
 */
#include <kernel/types.h>
#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <hardware/bda.h>

#include "bdavideo.h"

#include "protos.h"


extern Proj_M2vid	pm2;


Err
stomplists (p, v, ov)
struct Projector	*p;
struct View		*v, *ov;
{
	register uint32	vflags;
	register int	beamishere;
	Transition	*t;
	PerProj		*pp;
	Err		err, (*modfunc)(struct View *, struct Transition *,
					int32, int32);
	int32		delta;
	int32		transtop, transbot;
	int32		topedge, botedge, disphigh;
	int32		beampos;
	int		lacedisp;

	pp = p->p_ExtraData;
	disphigh = p->p_Height;

	lacedisp = (p->p_FieldsPerFrame == 2);
	delta = (int32) v->v_BaseAddr - (int32) ov->v_BaseAddr;
	beampos = GETBEAMPOS ();
	vflags = v->v_ViewFlags;
	transtop = pm2.pm2_StartY;
	for (t = pm2.pm2_Transitions;  t->t_Height;  t++) {
		if (transtop >= disphigh  ||
		    (transbot = transtop + t->t_Height) <= 0)
			/*  Not visible; don't process.  */
			continue;

		/*  Clip to visible limits.  */
		if (transtop < 0)		topedge = 0;
		else				topedge = transtop;

		if (transbot > disphigh)	botedge = disphigh;
		else				botedge = transbot;

#ifdef BUILD_BDA2_VIDEO_HACK
	    if (pp->pp_bda2hack.li_Node.n_Next  &&
	        (v->v_ViewFlags & VIEWF_BDA2HACK))
	    {
		beamishere = beampos + pp->pp_VBlankLines >= disp2hw (pp, topedge)  &&
			     beampos < disp2hw (pp, botedge);
	    } else
#endif
		beamishere = beampos >= disp2hw (pp, topedge)  &&
			     beampos < disp2hw (pp, botedge);

		if (t->t_View == v) {
			if (modfunc =
			     ((BDAVTI *) v->v_ViewTypeInfo)->bv_Modify)
			{
				if ((err = (modfunc) (v, t, 0, delta)) < 0)
					return (err);
				if (lacedisp  &&
				    (err = (modfunc) (v, t, 1, delta)) < 0)
					return (err);
			}
			if (beamishere  &&
			    (v->v_RendSig  ||
			     (vflags & VIEWF_FORCESCHEDSIG_REND)))
			{
				v->v_RendLineIntr.li_Line = botedge;
				vflags |= VIEWF_RENDLINESET;
			}

			if (vflags & VIEWF_DISPSIGLATCHED) {
				v->v_DispLineIntr.li_Line = topedge;
				vflags &= ~VIEWF_DISPSIGLATCHED;
				vflags |= VIEWF_DISPLINESET;
			}

		}

		if (beamishere  &&
		    (v->v_DispSig  ||  (vflags & VIEWF_FORCESCHEDSIG_DISP)))
			vflags |= VIEWF_DISPSIGLATCHED;

		topedge = botedge;
	}
	if ((v->v_RendSig  ||  (vflags & VIEWF_FORCESCHEDSIG_REND))  &&
	    !(vflags & VIEWF_RENDLINESET))
	{
		/*
		 * View not being scanned by beam; send signal now.
		 */
		vflags |= VIEWF_RENDSIGIMMEDIATE;
	}

	v->v_VLine = (BDA_READ (BDAVDU_VLOC) & VDU_VLOC_VCOUNT_MASK) >>
		     VDU_VLOC_VCOUNT_SHIFT;
	v->v_ViewFlags = vflags;

	return (0);
}
