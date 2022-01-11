/*  :ts=8 bk=0
 *
 * bdavideo.c:	Display driver/Projectors for BDA video.
 *
 * @(#) bdavideo.c 96/08/21 1.14
 *
 * Leo L. Schwab					9511.01
 */
#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernel.h>
#include <kernel/devicecmd.h>
#include <kernel/interrupts.h>
#include <kernel/sysinfo.h>
#include <kernel/cache.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <hardware/bda.h>
#include <hardware/m2vdl.h>
#include <hardware/PPCasm.h>
#include <loader/loader3do.h>

#include <graphics/projector.h>
#include <graphics/bitmap.h>

#include <string.h>

#include "bdavideo.h"

#include "protos.h"


/***************************************************************************
 * #defines
 */
#define	DEFAULT_HSTART_NTSC	98
#define	DEFAULT_HSTART_PAL	136

#define	VBLANKLINES_NTSC	15
#define	VBLANKLINES_PAL		15


/***************************************************************************
 * Local prototypes
 */
static int32 noop(struct IOReq *ior);
static void abortio(struct IOReq *ior);
static long deviceinit(struct Device *dev);
static long driverinit(struct Driver *drv);

static Err buildsystemvdls (struct Proj_M2vid *pm2);
static Err createVBL (struct Proj_M2vid *pm2);
static void adjustVBL (uint32, uint32);
static Err lamp (struct Projector *p, int32 op);
static Err perprojinit (struct PerProj *pp, struct Projector *p);
static Err projinit_ntsc (struct Projector *p, struct GraphicsFolioBase *gb);
static Err projinit_pal (struct Projector *p, struct GraphicsFolioBase *gb);


/***************************************************************************
 * Globals.
 */
extern BDAVTI		bdavtis_ntsc[];
extern BDAVTI		bdavtis_ntsc_nl[];
extern BDAVTI		bdavtis_pal[];
extern BDAVTI		bdavtis_pal_nl[];

GraphicsFolioBase	*GBase;
Proj_M2vid		pm2;
List			perprojlist;


static char		devname[] = "bdavideo";

static DriverCmdTable drivercmds[] = {
	{ GFXCMD_PROJECTORMODULE,	noop },
};


static TagArg	drivertags[] = {
	TAG_ITEM_PRI,		   (void *) 0,
	TAG_ITEM_NAME,		   devname,
        CREATEDRIVER_TAG_CREATEDEV,(void *) ((long)deviceinit),
        CREATEDRIVER_TAG_CREATEDRV,(void *) ((long)driverinit),
	CREATEDRIVER_TAG_ABORTIO,  (void *) abortio,
	CREATEDRIVER_TAG_NUMCMDS,  (void *) (sizeof (drivercmds) /
					     sizeof (DriverCmdTable)),
	CREATEDRIVER_TAG_CMDTABLE, (void *) drivercmds,
	TAG_END,		   0,
};


static TagArg	pt_ntsc[] = {
	{ TAG_ITEM_NAME,		"NTSC" },
	{ PROJTAG_WIDTH,		(void *) 640 },
	{ PROJTAG_HEIGHT,		(void *) 480 },
	{ PROJTAG_FIELDSPERFRAME,	(void *) 2 },
	{ PROJTAG_XASPECT,		(void *) 1 },
	{ PROJTAG_YASPECT,		(void *) 1 },
	{ PROJTAG_PROJTYPE,		(void *) PROJTYPE_NTSC },
	{ PROJTAG_BLANKVIEWTYPE,	(void *) bdavtis_ntsc },
	{ PROJTAG_VEC_INIT,		(void *) projinit_ntsc },
	{ PROJTAG_VEC_EXPUNGE,		(void *) NULL },
	{ PROJTAG_VEC_SCAVENGE,		(void *) NULL },
	{ PROJTAG_VEC_LAMP,		(void *) lamp },
	{ PROJTAG_VEC_UPDATE,		(void *) update },
	{ PROJTAG_VEC_NEXTVIEWTYPE,	(void *) nextvti_ntsc },
	{ TAG_END,			0 }
};

static TagArg	pt_ntsc_nolace[] = {
	{ TAG_ITEM_NAME,		"NTSC-nolace" },
	{ PROJTAG_WIDTH,		(void *) 640 },
	{ PROJTAG_HEIGHT,		(void *) 240 },
	{ PROJTAG_FIELDSPERFRAME,	(void *) 1 },
	{ PROJTAG_XASPECT,		(void *) 1 },
	{ PROJTAG_YASPECT,		(void *) 1 },
	{ PROJTAG_PROJTYPE,		(void *) PROJTYPE_NTSC_NOLACE },
	{ PROJTAG_BLANKVIEWTYPE,	(void *) bdavtis_ntsc_nl },
	{ PROJTAG_VEC_INIT,		(void *) projinit_ntsc },
	{ PROJTAG_VEC_EXPUNGE,		(void *) NULL },
	{ PROJTAG_VEC_SCAVENGE,		(void *) NULL },
	{ PROJTAG_VEC_LAMP,		(void *) lamp },
	{ PROJTAG_VEC_UPDATE,		(void *) update },
	{ PROJTAG_VEC_NEXTVIEWTYPE,	(void *) nextvti_ntsc_nl },
	{ TAG_END,			0 }
};


static TagArg	pt_pal[] = {
	{ TAG_ITEM_NAME,		"PAL" },
	{ PROJTAG_WIDTH,		(void *) 768 },
	{ PROJTAG_HEIGHT,		(void *) 576 },
	{ PROJTAG_FIELDSPERFRAME,	(void *) 2 },
	{ PROJTAG_XASPECT,		(void *) 1 },
	{ PROJTAG_YASPECT,		(void *) 1 },
	{ PROJTAG_PROJTYPE,		(void *) PROJTYPE_PAL },
	{ PROJTAG_BLANKVIEWTYPE,	(void *) bdavtis_pal },
	{ PROJTAG_VEC_INIT,		(void *) projinit_pal },
	{ PROJTAG_VEC_EXPUNGE,		(void *) NULL },
	{ PROJTAG_VEC_SCAVENGE,		(void *) NULL },
	{ PROJTAG_VEC_LAMP,		(void *) lamp },
	{ PROJTAG_VEC_UPDATE,		(void *) update },
	{ PROJTAG_VEC_NEXTVIEWTYPE,	(void *) nextvti_pal },
	{ TAG_END,			0 }
};

static TagArg	pt_pal_nolace[] = {
	{ TAG_ITEM_NAME,		"PAL-nolace" },
	{ PROJTAG_WIDTH,		(void *) 768 },
	{ PROJTAG_HEIGHT,		(void *) 288 },
	{ PROJTAG_FIELDSPERFRAME,	(void *) 1 },
	{ PROJTAG_XASPECT,		(void *) 1 },
	{ PROJTAG_YASPECT,		(void *) 1 },
	{ PROJTAG_PROJTYPE,		(void *) PROJTYPE_PAL_NOLACE },
	{ PROJTAG_BLANKVIEWTYPE,	(void *) bdavtis_pal_nl },
	{ PROJTAG_VEC_INIT,		(void *) projinit_pal },
	{ PROJTAG_VEC_EXPUNGE,		(void *) NULL },
	{ PROJTAG_VEC_SCAVENGE,		(void *) NULL },
	{ PROJTAG_VEC_LAMP,		(void *) lamp },
	{ PROJTAG_VEC_UPDATE,		(void *) update },
	{ PROJTAG_VEC_NEXTVIEWTYPE,	(void *) nextvti_pal_nl },
	{ TAG_END,			0 }
};


#if 0
/*
 * A historical artifact dear to my heart...
 */
static uint32		CLUT[] = {
	0x00000000, 0x01080808, 0x02101010, 0x03181818,
	0x04212121, 0x05292929, 0x06313131, 0x07393939,
	0x08424242, 0x094a4a4a, 0x0a525252, 0x0b5a5a5a,
	0x0c636363, 0x0d6b6b6b, 0x0e737373, 0x0f7b7b7b,
	0x10848484, 0x118c8c8c, 0x12949494, 0x139c9c9c,
	0x14a5a5a5, 0x15adadad, 0x16b5b5b5, 0x17bdbdbd,
	0x18c6c6c6, 0x19cecece, 0x1ad6d6d6, 0x1bdedede,
	0x1ce7e7e7, 0x1defefef, 0x1ef7f7f7, 0x1fffffff,
	0xe0000000	/*  Background color  */
};
#endif




/***************************************************************************
 * Notes to myself....
 *
 * Device/driver, upon launch, will create all valid Projectors (via
 * CreateItem()) that can possibly run on available hardware.  Creation of
 * Projectors will involve call-backs into the device/driver (this is needed
 * so that Projector Items will have no "transitory" states of existence;
 * when CreateItem() returns, the Projector must be fully initialized and
 * valid).
 *
 * Some Projectors will have common VBL intervals.  VBL intervals will be
 * created at driver initialization, in a semi-separate step.  VBL intervals
 * may one day be reconfigurable (possibly through new graphics calls, or by
 * calling ModifyGraphicsItem() on the Projector) which will vector through
 * the Projector.
 */

/*
 * More notes to myself:
 *
 * Create VBL interval first.  Then build Projector Items.  If one Projector
 * fails creation, the other can live on with the VBL handler.
 ****
 * In case a Projector is deactivated (changing from NTSC to PAL, for
 * example), the VBL interval hangs around (not a lot of RAM).  The
 * Projector startup routine must be able to recognize that a VBL already
 * exists for it and use it.
 */


/***************************************************************************
 * Driver code.
 */
int
main (void)
{
	return (CreateItemVA (MKNODEID (NST_KERNEL, DRIVERNODE),
			      CREATEDRIVER_TAG_MODULE, FindCurrentModule(),
			      TAG_JUMP, drivertags));
}


static int32
noop (
struct IOReq *ior
)
{
	ior->io_Error = GFX_ERR_NOTSUPPORTED;
	return (1);
}

/*
 * Note that interrupts are already disabled when abortio is called.
 * See internalAbortIO() in sendio.c for details.
 */
static void
abortio (
struct IOReq *ior
)
{
	SuperCompleteIO (ior);
}


/*
 * Device/Driver Related Code.
 */
static Item
deviceinit (
struct Device *dev
)
{
	return (dev->dev.n_Item);
}

static long
driverinit (
struct Driver *drv
)
{
	Err		err;
	DispModeInfo	dmi;

	/*
	 * Explicit bind to graphics folio.  Get pointer to GBase.
	 */
	if ((err = ImportByName (FindCurrentModule(), "graphics")) < 0)
		return (err);
	if ((err = FindNamedItem (MKNODEID (NST_KERNEL, FOLIONODE),
				  "graphics")) < 0)
		return (err);
	GBase = LookupItem (err);

	SuperQuerySysInfo (SYSINFO_TAG_GRAFDISP, &dmi, sizeof (dmi));

	/*
	 * Initialize lists.
	 */
	InitList (&perprojlist, NULL);
	InitList (&pm2.pm2_PendingList, NULL);
	InitList (&pm2.pm2_FreeList, NULL);

	if ((err = buildsystemvdls (&pm2)) < 0)
		return (err);

/*  For occasional debugging...
if ((err = inittrip ()) < 0)
 return (err);
 */

	/*
	 * Launch FIRQs.
	 */
	if ((err = SuperCreateFIRQ
		    (devname, 210, dispmodeFIRQ, INT_BDA_V1)) < 0)
		return (err);
	pm2.pm2_DispFIRQ = err;

	if ((err = SuperCreateFIRQ
		    (devname, 200, beamFIRQ, INT_BDA_V0)) < 0)
		return (err);
	pm2.pm2_BeamFIRQ = err;

	BDA_WRITE (BDAVDU_VINT,
		   BDA_READ (BDAVDU_VINT) & ~VDU_VINT_VLINE0_MASK);

	EnableInterrupt (INT_BDA_V0);

	/*
	 * Create Projector Items.
	 */
	if (dmi & SYSINFO_NTSC_SUPPORTED) {
		if ((err = CreateItemVA
			    (MKNODEID (NST_GRAPHICS, GFX_PROJECTOR_NODE),
			     PROJTAG_FIELDSPERSECOND, ConvertFP_TagData (59.94),
			     TAG_JUMP, pt_ntsc)) < 0)
		{
			PrintfSysErr (err);
			return (err);
		}

		if ((err = CreateItemVA
			    (MKNODEID (NST_GRAPHICS, GFX_PROJECTOR_NODE),
			     PROJTAG_FIELDSPERSECOND, ConvertFP_TagData (59.94),
			     TAG_JUMP, pt_ntsc_nolace)) < 0)
		{
			PrintfSysErr (err);
			return (err);
		}
	}

	if (dmi & SYSINFO_PAL_SUPPORTED) {
		if ((err = CreateItemVA
			    (MKNODEID (NST_GRAPHICS, GFX_PROJECTOR_NODE),
			     PROJTAG_FIELDSPERSECOND, ConvertFP_TagData (50.0),
			     TAG_JUMP, pt_pal)) < 0)
		{
			PrintfSysErr (err);
			return (err);
		}

		if ((err = CreateItemVA
			    (MKNODEID (NST_GRAPHICS, GFX_PROJECTOR_NODE),
			     PROJTAG_FIELDSPERSECOND, ConvertFP_TagData (50.0),
			     TAG_JUMP, pt_pal_nolace)) < 0)
		{
			PrintfSysErr (err);
			return (err);
		}
	}

	return (drv->drv.n_Item);
}


/***************************************************************************
 * Projector code.
 */

/***************************************************************************
 * Vertical interval constructors.
 */
static Err
buildsystemvdls (pm2)
struct Proj_M2vid	*pm2;
{
	ShortVDL	*last;
	Err		err;

	if ((err = createVBL (pm2)) < 0)
		return (err);

	/*
	 * Create VDL which terminates the display.
	 */
	if (!(last = SuperAllocMem (sizeof (ShortVDL), MEMTYPE_FILL)))
		return (GFX_ERR_NOMEM);

	last->sv.DMACtl	  = VDL_DMA_LDLOWER | VDL_DMA_LDUPPER |
			    VDL_NWORDS_SHORT_FMT |
			    0;
	last->sv.UpperPtr = GBase->gb_VRAMBase;
	last->sv.LowerPtr = GBase->gb_VRAMBase;
	last->sv.NextVDL  = NULL;

	last->sv_DispCtl0 = VDL_DC | VDL_DC_0 | VDL_DC_NOP;
	last->sv_DispCtl1 = VDL_DC | VDL_DC_1 | VDL_DC_NOP;
	last->sv_AVCtl    = VDL_AV |
			    VDL_AV_FIELD (VDL_AV_HSTART, 0) |
			    VDL_AV_FIELD (VDL_AV_HWIDTH, 0) |
			    VDL_AV_HDOUBLE |
			    VDL_AV_LD_HSTART | VDL_AV_LD_HWIDTH |
			    VDL_AV_LD_HDOUBLE | VDL_AV_LD_VDOUBLE;
	last->sv_ListCtl  = VDL_LC |
			    VDL_LC_LD_FBFORMAT | VDL_LC_FBFORMAT_16 |
			    VDL_LC_ONEVINTDIS;

	pm2->pm2_EndField = &last->sv;

	pm2->pm2_PatchVDL0->NextVDL =
	pm2->pm2_PatchVDL1->NextVDL = pm2->pm2_EndField;

	WriteBackDCache (0, last, sizeof (ShortVDL));

	return (0);
}


/*
 * ### There are only supposed to be "two" VDL components for the vertical
 * blank; the first to delay the requisite number of lines, and the second
 * as a patch point to splice in the client's display.
 *
 * Due to a hardware weirdness of pre-fetching the next VDL, we've
 * constructed three components until pass-2 silicon arrives (where the
 * weirdness goes away).
 ****
 * 9511.09: Could I statically build this?  Hmm...
 */
static Err
createVBL (pm2)
struct Proj_M2vid	*pm2;
{
	ShortVDL	*sv;
	uint32		dmactl;
	int32		vdlsize;
	void		*vdlbase;

	vdlsize = sizeof (ShortVDL) * 6;

	if (!(sv = vdlbase = SuperAllocMem (vdlsize, MEMTYPE_FILL)))
		return (GFX_ERR_NOMEM);

	/*
	 * Build template.
	 */
	dmactl		= VDL_DMA_LDLOWER | VDL_DMA_LDUPPER |
			  VDL_NWORDS_SHORT_FMT;
	sv->sv.UpperPtr	= GBase->gb_VRAMBase;
	sv->sv.LowerPtr	= GBase->gb_VRAMBase;
	sv->sv.NextVDL	= NULL;

	sv->sv_DispCtl0	= VDL_DC | VDL_DC_0 | VDL_DC_NOP;
	sv->sv_DispCtl1	= VDL_DC | VDL_DC_1 | VDL_DC_NOP;
	sv->sv_AVCtl	= VDL_AV |
			  VDL_AV_FIELD (VDL_AV_HSTART, 0) |
			  VDL_AV_FIELD (VDL_AV_HWIDTH, 0) |
			  VDL_AV_HDOUBLE |
			  VDL_AV_LD_HSTART | VDL_AV_LD_HWIDTH |
			  VDL_AV_LD_HDOUBLE | VDL_AV_LD_VDOUBLE;
	sv->sv_ListCtl  = VDL_LC |
			  VDL_LC_LD_FBFORMAT | VDL_LC_FBFORMAT_16;

	memcpy (sv + 1, sv, sizeof (*sv));
	memcpy (sv + 2, sv, sizeof (*sv));
	memcpy (sv + 3, sv, sizeof (*sv) * 3);

	/*
	 * Set up vertical interval.
	 */
	pm2->pm2_ForcedVDL0 = (VDLHeader *) sv;
	sv->sv_DispCtl0	= VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT;
	sv->sv_DispCtl1	= VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT;
	sv->sv.DMACtl	= dmactl | 1;	/*  Set later	*/
	sv->sv.NextVDL	= (VDLHeader *) (sv + 1);
	sv++;
	sv->sv.DMACtl	= dmactl | 1;	/*### BDA1.1 prefetch dummy	*/
	sv->sv.NextVDL	= (VDLHeader *) (sv + 1);
	sv++;
	pm2->pm2_PatchVDL0 = (VDLHeader *) sv;
	sv->sv.DMACtl	= dmactl | 1;
	sv++;


	pm2->pm2_ForcedVDL1 = (VDLHeader *) sv;
	sv->sv_DispCtl0	= VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT;
	sv->sv_DispCtl1	= VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT;
	sv->sv.DMACtl	= dmactl | 1;	/*  Set later	*/
	sv->sv.NextVDL	= (VDLHeader *) (sv + 1);
	sv++;
	sv->sv.DMACtl	= dmactl | 1;	/*### BDA1.1 prefetch dummy	*/
	sv->sv.NextVDL	= (VDLHeader *) (sv + 1);
	sv++;
	pm2->pm2_PatchVDL1 = (VDLHeader *) sv;
	sv->sv.DMACtl	= dmactl | 1;


	WriteBackDCache (0, vdlbase, vdlsize);

	return (0);
}


/*
 * Used to convert vertical interval to active Projector's requirements.
 * Makes boatload of assumptions about how vertical interval was built.
 */
static void
adjustVBL (nlines, hstart)
uint32	nlines, hstart;
{
	register int	i;
	ShortVDL	*sv;

	sv = (ShortVDL *) pm2.pm2_ForcedVDL0;

	sv->sv.DMACtl = (sv->sv.DMACtl & ~VDL_DMA_NLINES_MASK) | nlines - 2;

	for (i = 3;  --i >= 0;  sv++)
		sv->sv_AVCtl = (sv->sv_AVCtl & ~VDL_AV_HSTART_MASK) |
				VDL_AV_FIELD (VDL_AV_HSTART, hstart);

	/*  And again for the other one.  */
	sv = (ShortVDL *) pm2.pm2_ForcedVDL1;

	sv->sv.DMACtl = (sv->sv.DMACtl & ~VDL_DMA_NLINES_MASK) | nlines - 2;

	for (i = 3;  --i >= 0;  sv++)
		sv->sv_AVCtl = (sv->sv_AVCtl & ~VDL_AV_HSTART_MASK) |
				VDL_AV_FIELD (VDL_AV_HSTART, hstart);

	/*  Write to RAM  */
	WriteBackDCache
	 (0, pm2.pm2_ForcedVDL0, sizeof (ShortVDL) * 6);
}



/***************************************************************************
 * Turn Projector on and off.
 */
static Err
lamp (p, op)
struct Projector	*p;
int32			op;
{
	uint32	oldints, reg;
	Err	retval;

	retval = 0;
	switch (op) {
	case LAMPOP_ACTIVATE:
	 {
		/*
		 * Turn projector on.
		 * This routine assumes the hardware has been bludgeoned
		 * into quiescence (with the notable exception that it also
		 * has to be able to take over from the bootcode).
		 */
		PerProj	*pp;
		List	*list;
		BootAlloc *ba;
		void	(*DeleteBootAlloc)(void *addr, uint32 sz, uint32 flags);

		if (p->p_Flags & PROJF_ACTIVE)
			/*  Already active; nothing to do.  */
			return (0);

		if (pm2.pm2_ActiveProjector  &&
		    pm2.pm2_ActiveProjector != p)
		{
			/*
			 * There is already a Projector active on this
			 * hardware.  Shut it down first.
			 */
			retval = pm2.pm2_ActiveProjector->p.n_Item;

			lamp (pm2.pm2_ActiveProjector, LAMPOP_DEACTIVATE);
		}
		/*
		 * Set the video encoder to output the desired display type.
		 */
		pm2.pm2_NewDispMode = (p->p_FieldsPerFrame == 2)  ?
				      SYSINFO_TAG_INTERLACED  :
				      SYSINFO_TAG_PROGRESSIVE;
		pm2.pm2_ChangeModeSigTask = CURRENTTASK;

		while (pm2.pm2_ChangeModeSigTask)
			WaitSignal (SIGF_ONESHOT);

		/*
		 * Fiddle with the vertical interval.
		 */
		pp = p->p_ExtraData;
		adjustVBL (pp->pp_VBlankLines, pp->pp_HStart);

		/*
		 * Setup the hardware.
		 */
		oldints = Disable ();

		BDA_WRITE (BDAVDU_FV0A, (vuint32) pm2.pm2_ForcedVDL0);
		BDA_WRITE (BDAVDU_FV1A, (vuint32) pm2.pm2_ForcedVDL1);

		reg = BDA_READ (BDAVDU_VRST);
		reg &= ~(VDU_VRST_VIDRESET & VDU_VRST_DVERESET);
		BDA_WRITE (BDAVDU_VRST, reg);

		Enable (oldints);

		/*
		 * Tell the system we have assumed control of the hardware.
		 * Also free up the memory used for boot graphics, if any.
		 */
		SuperSetSysInfo (SYSINFO_TAG_GRAFBUSY, (void *) TRUE, 0);

		SuperQuerySysInfo (SYSINFO_TAG_DELETEBOOTALLOC,
			(void *) &DeleteBootAlloc, sizeof (DeleteBootAlloc));
	ScanTheList:
		SuperQuerySysInfo (SYSINFO_TAG_BOOTALLOCLIST,
			(void *) &list, sizeof (list));
		ScanList(list, ba, BootAlloc)
		{
			if (ba->ba_Flags & BA_GRAPHICS)
			{
				SuperFreeRawMem(ba->ba_Start, ba->ba_Size);
				(*DeleteBootAlloc)(ba->ba_Start, ba->ba_Size,
							ba->ba_Flags);
				goto ScanTheList;
			}
		}

		/*
		 * Mark the Projector as active, and regenerate the display.
		 */
		p->p_Flags |= PROJF_ACTIVE;
		pm2.pm2_ActiveProjector = p;

		update (p, PROJOP_ACTIVATE, NULL);

		break;
	 }
	case LAMPOP_DEACTIVATE:
	 {
		/*
		 * Turn projector off.
		 * Shut down vidunit DMA, and yank the VDLs.
		 */
		if (!(p->p_Flags & PROJF_ACTIVE))
			/*  It's already off; nothing to do.  */
			return (0);

		if (p == pm2.pm2_ActiveProjector) {
#if 0
			/*
			 * Turning off vidunit DMA seems to break the VBL
			 * interrupt, which I need to keep running.  So it
			 * appears I can't do this.
			 */
			oldints = Disable ();

			reg = BDA_READ (BDAVDU_VRST);
			reg |= VDU_VRST_VIDRESET;
			BDA_WRITE (BDAVDU_VRST, reg);

			Enable (oldints);
#endif

			pm2.pm2_PatchVDL0->NextVDL = pm2.pm2_EndField;
			pm2.pm2_PatchVDL1->NextVDL = pm2.pm2_EndField;
			_dcbst (&pm2.pm2_PatchVDL0->NextVDL);
			_dcbst (&pm2.pm2_PatchVDL1->NextVDL);

			pm2.pm2_ActiveProjector = NULL;
		}
		p->p_Flags &= ~PROJF_ACTIVE;

		/*
		 * At this point, it would be safe to pull pm2_VDL[01] and
		 * drop them on the pending list.  Still debating whether I
		 * should do this.
		 */
		break;
	 }
	case LAMPOP_DEACTIVATE_RESCUE:
	 {
	 	Projector	*newproj;
		PerProj		*pp;

		for (pp = (PerProj *) FIRSTNODE (&perprojlist);
		     NEXTNODE (pp);
		     pp = (PerProj *) NEXTNODE (pp))
		{
			newproj = pp->pp_Projector;

			if (newproj->p.n_OpenCount) {
				retval = lamp (newproj, LAMPOP_ACTIVATE);
				break;
			}
		}

		/*
		 * Even if we find nothing, we still have to kill this one.
		 */
		lamp (p, LAMPOP_DEACTIVATE);
		break;
	 }
	default:
		return (GFX_ERR_INTERNAL);
	}
	return (retval);
}


/***************************************************************************
 * Projector-specific initialization call-backs (at CreateItem() time).
 * Each does both interlaced and non-interlaced flavors.
 */
static Err
perprojinit (pp, p)
struct PerProj		*pp;
struct Projector	*p;
{
	uint32	oldints;

	InitList (&pp->pp_SigList, NULL);
	InitList (&pp->pp_StalledSigs, NULL);
	pp->pp_Projector		= p;
	pp->pp_pm2			= &pm2;
	pp->pp_ZeroMarker.li_Type	= INTRTYPE_ZEROMARKER;
	pp->pp_ZeroMarker.li_Line	= LI_LINE_MASK;  /* "End" of field */
	AddHead (&pp->pp_SigList, (Node *) &pp->pp_ZeroMarker);

#ifdef BUILD_BDA2_VIDEO_HACK
	/*
	 * Disgusting hack for BDA 2.0.
	 */
    {
	BDAInfo bda;

	QUERY_SYS_INFO (SYSINFO_TAG_BDA, bda);
	if ((bda.bda_ID & 0xFF000000) == 0xFF000000) {
		pp->pp_bda2hack.li_Type = INTRTYPE_BDA2HACK;
		pp->pp_bda2hack.li_Line = pp->pp_Projector->p_Height + 3;
		AddHead (&pp->pp_SigList, (Node *) &pp->pp_bda2hack);
	} else
		pp->pp_bda2hack.li_Node.n_Next =
		pp->pp_bda2hack.li_Node.n_Prev = NULL;
    }
#endif

	oldints = Disable ();
	AddTail (&perprojlist, (Node *) pp);
	Enable (oldints);

	p->p_ExtraData = pp;

	return (0);
}

static Err
projinit_ntsc (p, gb)
struct Projector		*p;
struct GraphicsFolioBase	*gb;
{
	PerProj		*pp;
	HSTARTInfo	hstart;

	TOUCH (gb);

	if (!(pp = SuperAllocMem (sizeof (*pp), MEMTYPE_NORMAL)))
		return (GFX_ERR_NOMEM);

	SuperQuerySysInfo (SYSINFO_TAG_HSTART_NTSC, &hstart, sizeof (hstart));

	perprojinit (pp, p);
	pp->pp_HStart		= hstart;
	pp->pp_VBlankLines	= VBLANKLINES_NTSC;

	return (0);
}

static Err
projinit_pal (p, gb)
struct Projector		*p;
struct GraphicsFolioBase	*gb;
{
	PerProj		*pp;
	HSTARTInfo	hstart;

	TOUCH (gb);

	if (!(pp = SuperAllocMem (sizeof (*pp), MEMTYPE_NORMAL)))
		return (GFX_ERR_NOMEM);

	SuperQuerySysInfo (SYSINFO_TAG_HSTART_PAL, &hstart, sizeof (hstart));

	perprojinit (pp, p);
	pp->pp_HStart		= hstart;
	pp->pp_VBlankLines	= VBLANKLINES_PAL;

	return (0);
}
