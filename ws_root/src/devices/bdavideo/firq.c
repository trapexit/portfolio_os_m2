/*  :ts=8 bk=0
 *
 * firq.c:	The FIRQ.  You know, the interrupt handler.
 *
 * @(#) firq.c 96/09/19 1.7
 *
 * Leo L. Schwab					9601.10
 */
#include <kernel/types.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <kernel/mem.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>

#include <graphics/projector.h>

#include <stddef.h>
#include <stdio.h>

#include "bdavideo.h"

#include "protos.h"


/***************************************************************************
 * This FIRQ's purpose in life is to see if there are any VDLs to be freed
 * by sniffing the gb_FreeList.  If there are, the daemon gets signalled so
 * it can free them.  Then it signals all the tasks that want to know when
 * their Views have become visible.
 */
extern Proj_M2vid	pm2;
extern List		perprojlist;

uint32			*__tripwires;


static bool cmplineintrs (struct Node *node, struct Node *test);
static void setvline0 (struct PerProj *pp);
/*static void dispatchlinesignal (struct PerProj *pp, struct lineintr *li);*/
static void clobbervidunit (void);


/***************************************************************************
 * FIRQ to change video modes.  SuperSetSysInfo() requires that this routine
 * execute near the top of a video field.
 */
int32
dispmodeFIRQ ()
{
	/*
	 * If there is an outstanding request to do so, change the display
	 * mode from interlaced to non-interlaced, or vice-versa.
	 */
	if (pm2.pm2_ChangeModeSigTask) {
		if (SuperSetSysInfo
		     (pm2.pm2_NewDispMode, (void *) TRUE, 0) == SYSINFO_SUCCESS)
		{
			SuperInternalSignal (pm2.pm2_ChangeModeSigTask,
					     SIGF_ONESHOT);
			pm2.pm2_ChangeModeSigTask = NULL;
		}
	}

	return (0);
}


/***************************************************************************
 * FIRQ to follow the beam around and dispatch signals at the earliest
 * possible time.
 */
int32
beamFIRQ ()
{
	lineintr	*li, *zeromarker;
	PerProj		*pp;

	BDA_CLR (BDAVDU_VINT, VDU_VINT_VINT0);	/*  Clear source.  */

	for (pp = (PerProj *) FIRSTNODE (&perprojlist);
	     NEXTNODE (pp);
	     pp = (PerProj *) NEXTNODE (pp))
	{
		if (!(pp->pp_Projector->p_Flags & PROJF_ACTIVE))
			/*  Don't do nothin' unless we can see it.  */
			continue;

		zeromarker = NULL;
		li = (lineintr *) FIRSTNODE (&pp->pp_SigList);
		if (li->li_Type == INTRTYPE_ZEROMARKER) {
			RemNode ((Node *) li);
			zeromarker = li;

			/*
			 * Vertical blank has transpired; it is now safe to
			 * free VDLs no longer current.  Transfer VDLs from
			 * pending list to free list.  ('li' is being
			 * misused here, 'cuz it's available.)
			 */
			while (li = (lineintr *) RemHead
						  (&pm2.pm2_PendingList))
				AddHead (&pm2.pm2_FreeList, (Node *) li);

			/*
			 * Decrement all field counts.
			 */
			for (li = (lineintr *) FIRSTNODE (&pp->pp_SigList);
			     NEXTNODE (li);
			     li = (lineintr *) NEXTNODE (li))
				li->li_Line -= FIELDINCREMENT;
		}

		setvline0 (pp);

		if (zeromarker) {
			installlineintr (pp, zeromarker, 0);
		}
	}

	return (0);
}


/*
 * This routine pulls lineintrs off the list until it finds one that isn't
 * ready to happen yet.  **Must be called with interrupts disabled.**
 *
 * There is a vulnerability where, if interrupts are held off across a
 * VBlank, signals could be delayed by one field.  (Store last VLOC value
 * read to detect a wraparound?)
 */
static void
setvline0 (pp)
struct PerProj	*pp;
{
	register lineintr	*li;
	register int32		beampos, trig, trighw;
	register uint32		reg;

	while (1) {
		li = (lineintr *) FIRSTNODE (&pp->pp_SigList);

		if (!NEXTNODE (li))
			/*  Nothing to do.  */
			break;

		/*
		 * Check to see if it should already happen.
		 */
		trig = disp2hw (pp, li->li_Line);
		if (li->li_Type == INTRTYPE_ZEROMARKER)
			trighw = 1;	/*  Zero's not good for the HW	*/
		else
			trighw = disp2hw (pp, li->li_Line & LI_LINE_MASK);
		beampos = GETBEAMPOS ();

#ifdef BUILD_BDA2_VIDEO_HACK
		/*
		 * If the BDA 2.0 hack is enabled, and this View's VDL has
		 * been warped, back off its idea about where the display
		 * signal interrupt should happen.  (Please, don't ask how
		 * this works; it's ugly enough without having to explain
		 * it.)
		 */
		if (pp->pp_bda2hack.li_Node.n_Next  &&
		    li->li_Type == INTRTYPE_DISPINTR)
		{
			View	*v;

			v = (View *) ((uint32) li -
				      offsetof (View, v_DispLineIntr));

			if (v->v_ViewFlags & VIEWF_BDA2HACK) {
				trig -= pp->pp_VBlankLines;
				if ((trighw -= pp->pp_VBlankLines) <= 0)
					trighw = 1;
			}
		}
#endif

		if (beampos >= trig)
			goto sendsig;	/*  Look down.  */

		/*
		 * Schedule interrupt.
		 */
		reg = BDA_READ (BDAVDU_VINT);
		reg &= ~VDU_VINT_VLINE0_MASK;
		reg |= trighw << VDU_VINT_VLINE0_SHIFT;
		BDA_WRITE (BDAVDU_VINT, reg);

		/*
		 * Final check to see if we barely missed it.
		 */
		beampos = GETBEAMPOS ();
		if (beampos < trig)
			/*
			 * Nope, we're still in good shape.  Leave interrupt
			 * to happen in its own time.
			 */
			break;

		/*
		 * Urk!  It just happened.  Clear the interrupt...
		 */
		BDA_CLR (BDAVDU_VINT, VDU_VINT_VINT0);

		/*
		 * ...Send the signal, and scan the list again.
		 */
sendsig:	RemNode ((Node *) li);
		li->li_Node.n_Next =
		li->li_Node.n_Prev = NULL;

#ifdef BUILD_BDA2_VIDEO_HACK
	    if (li->li_Type == INTRTYPE_BDA2HACK) {
		clobbervidunit ();
		installlineintr (pp, li, 1);	/* This shouldn't recurse. */
	    } else
#endif
		dispatchlinesignal (pp, li);
	}
}


/*
 * 'li' node is assumed to be removed from all lists.
 * (The VIEWF_PENDINGSIG_* flags are cleared before the signals are sent,
 * as certain software makes decisions on whether or not to wait depending
 * on the state of the flag.)
 */
void
dispatchlinesignal (pp, li)
struct PerProj	*pp;
struct lineintr	*li;
{
	register View		*v;
	register Projector	*p;

	if (li->li_Type == INTRTYPE_RENDINTR) {
		/*
		 * Rendersignals can get stalled if the Projector is locked
		 * down (due to recompilation or a LockDisplay() call).
		 ***
		 * ### FIXME:  I've done all the 'stalling' backwards for
		 * the case of a locked Projector.  Think about it more
		 * carefully.  (Summary:  Rendersignals don't block, while
		 * Displaysignals do.)
		 */
		v = (View *) ((uint32) li - offsetof (View, v_RendLineIntr));
		p = pp->pp_Projector;

		if (!p->p_ViewListSema4->sem_NestCnt  &&
		    !p->p_PendingRecompile)
		{
			v->v_ViewFlags &= ~VIEWF_PENDINGSIG_REND;
			SuperInternalSignal (v->v_SigTask, v->v_RendSig);
			if (v->v_RendCBFunc) {
				(v->v_RendCBFunc) (v, v->v_RendCBData);
				v->v_RendCBFunc = NULL;
			}
		}
		else
			AddHead (&pp->pp_StalledSigs, (Node *) li);

	} else if (li->li_Type == INTRTYPE_DISPINTR) {
		/*
		 * Display signals always get sent (cuz, like, the View's
		 * getting displayed, whether the Projector's locked or not.
		 ***
		 * ### FIXME: There is actually a case where a DisplaySignal
		 * should not be sent: LockDisplay(), modify signalled view,
		 * fiddle around for a few fields, UnlockDisplay().  How to
		 * handle this?
		 *	ewhac 9605.20
		 ***
		 * Hmmm, maybe if I created two more lists where lineintrs
		 * get queued while the Projector is locked, then bucketed
		 * over when it's unlocked...
		 ***
		 * In any case, the first statement was wrong;
		 * Displaysignals indicate when the *changes* become visible.
		 * ewhac 9608.12
		 */
		v = (View *) ((uint32) li - offsetof (View, v_DispLineIntr));

		v->v_ViewFlags &= ~VIEWF_PENDINGSIG_DISP;
		SuperInternalSignal (v->v_SigTask, v->v_DispSig);
		if (v->v_DispCBFunc) {
			(v->v_DispCBFunc) (v, v->v_DispCBData);
			v->v_DispCBFunc = NULL;
		}
	} else if (li->li_Type == INTRTYPE_STALL) {
		/*
		 * Tell ourselves that now's a good time to fiddle with the
		 * VDLs.
		 */
		stalllineintr	*sli;

		sli = (stalllineintr *) li;
		SuperInternalSignal (sli->sli_Task, SIGF_ONESHOT);
		sli->sli_Task = NULL;
	}
}



static bool
cmplineintrs (node, test)
struct Node	*node, *test;
{
	/*
	 * Smaller li_Line values are earlier in the list.
	 */
	return (((struct lineintr *) test)->li_Line >=
		((struct lineintr *) node)->li_Line);
}


Err
installlineintr (pp, node, field)
struct PerProj	*pp;
struct lineintr	*node;
int		field;
{
	if (field)
		node->li_Line += FIELDINCREMENT;

	UniversalInsertNode (&pp->pp_SigList, (Node *) node, cmplineintrs);

	if (node == (lineintr *) FIRSTNODE (&pp->pp_SigList))
		setvline0 (pp);

	return (0);
}


void
removelineintr (pp, li)
struct PerProj	*pp;
struct lineintr	*li;
{
	register lineintr	*oldhead;

	oldhead = (lineintr *) FIRSTNODE (&pp->pp_SigList);

	RemNode ((Node *) li);
	li->li_Node.n_Next =
	li->li_Node.n_Prev = NULL;

	if (li == oldhead)
		setvline0 (pp);
}



int32
disp2hw (pp, val)
struct PerProj	*pp;
int32		val;
{
	if (pp->pp_Projector->p_FieldsPerFrame == 2)
		val >>= 1;

	return (val + pp->pp_VBlankLines);
}


Err
inittrip ()
{
	if (!(__tripwires = AllocMemMasked (sizeof (uint32) * 8,
					    MEMTYPE_FILL | 0,
					    0x1F,
					    0x00)))
		return (GFX_ERR_NOMEM);
printf ("__tripwires at 0x%08lx\n", __tripwires);
	return (0);
}

void
trip (key, val)
uint32	key, val;
{
	__tripwires[7] = __tripwires[5];
	__tripwires[6] = __tripwires[4];
	__tripwires[5] = __tripwires[3];
	__tripwires[4] = __tripwires[2];
	__tripwires[3] = __tripwires[1];
	__tripwires[2] = __tripwires[0];
	__tripwires[1] = val;
	__tripwires[0] = key;

	_dcbst (__tripwires);
}


#ifdef BUILD_BDA2_VIDEO_HACK
/***************************************************************************
 * ### WARNING!!  EVIL REVOLTING HACK!!
 * This routine clobbers the vidUnit over the head at the bottom of every
 * field.
 */
static void
clobbervidunit ()
{
	uint32	reset, vint;

	reset = BDA_READ (BDAVDU_VRST);
	vint = BDA_READ (BDAVDU_VINT) & ~(VDU_VINT_VINT0 | VDU_VINT_VINT1);
	BDA_WRITE (BDAVDU_VRST, reset | VDU_VRST_VIDRESET);
	BDA_WRITE (BDAVDU_VRST, reset);
	BDA_WRITE (BDAVDU_VINT, vint);
}
#endif
