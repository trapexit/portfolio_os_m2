/*  :ts=8 bk=0
 *
 * db_ntsc_nl.c:	View type database, NTSC non-interlaced
 *
 * @(#) db_ntsc_nl.c 96/06/14 1.1
 *
 * Leo L. Schwab					9606.04
 */
#include <kernel/types.h>

#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>

#include "bdavideo.h"

#include "protos.h"


/***************************************************************************
 * ViewTypeInfo database.
 * This array needs to track all the VIEWTYPE_ enums.
 */
static int32	vt16types[] = { BMTYPE_16 };
static int32	vt32types[] = { BMTYPE_32 };

#define	N16TYPES	(sizeof (vt16types) / sizeof (int32))
#define	N32TYPES	(sizeof (vt32types) / sizeof (int32))

#if 0
/*  for reference */
typedef struct BDAVTI {
	int32	vti_Type;		/*  View Type			*/
	uint32	vti_Flags;		/*  Some flags			*/
	uint16	vti_MaxPixWidth,	/*  Maximum pixel width		*/
		vti_MaxPixHeight;	/*  Maximum pixel height	*/
	uint8	vti_XAspect,		/*  Pixel aspect ratio		*/
		vti_YAspect;
	uint8	vti_NBMTypes;		/*  Number of Bitmap Types	*/
	int32	*vti_BMTypes;		/*  (In)Compatible Bitmap Types */
	uint8	vti_PixXSize,
		vti_PixYSize;
	Err			(*bv_Compile)(struct View *,
					      struct PTState *);
	Err			(*bv_Modify)(struct View *,
					     struct Transition *,
					     int32,
					     int32);
	uint8			bv_NBMTypes;
	uint8			*bv_BMTypes;
	uint32			bv_FixedVDL_DC0, bv_MaskVDL_DC0;
	uint32			bv_FixedVDL_DC1, bv_MaskVDL_DC1;
	uint32			bv_FixedVDL_AV, bv_MaskVDL_AV;
	uint32			bv_FixedVDL_LC, bv_MaskVDL_LC;
} BDAVTI;
#endif


BDAVTI	bdavtis_ntsc_nl[] = {
 {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_INVALID,
	0,
	0, 0,
	0, 0,
	0, NULL,
	2, 2,
	cvu_blank,
	NULL,
	VDL_DC | VDL_DC_0 | VDL_DC_DEFAULT,
			~0,

	VDL_DC | VDL_DC_1 | VDL_DC_DEFAULT,
			~0,

	VDL_AV_NOP,
			~0,

	VDL_LC_NOP,
			~0
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_16,
	VTIF_BMTYPES_ALLOWED | VTIF_ABLE_AVG_H,
	320, 240,
	1, 1,
	N16TYPES, vt16types,
	2, 1,
	cvu_16_32,
	mvu_16_32_dual,
	0,
			0,

	0,
			0,

	VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_FBFORMAT_16 | VDL_LC_LD_FBFORMAT,
			VDL_LC_FBFORMAT | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_32,
	VTIF_BMTYPES_ALLOWED | VTIF_ABLE_AVG_H,
	320, 240,
	1, 1,
	N32TYPES, vt32types,
	2, 1,
	cvu_16_32,
	mvu_16_32_dual,
	0,
			0,

	0,
			0,

	VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_BYPASSTYPE_LSB | VDL_LC_FBFORMAT_32 |
	 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT,
			VDL_LC_BYPASSTYPE | VDL_LC_FBFORMAT |
			 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_16_640,
	VTIF_BMTYPES_ALLOWED,
	640, 240,
	1, 2,
	N16TYPES, vt16types,
	1, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE),
			VDL_DC_HINTCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE),
			VDL_DC_HINTCTL_MASK,

	VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_FBFORMAT_16 | VDL_LC_LD_FBFORMAT,
			VDL_LC_FBFORMAT | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_32_640,
	VTIF_BMTYPES_ALLOWED,
	640, 240,
	1, 2,
	N32TYPES, vt32types,
	1, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE),
			VDL_DC_HINTCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE),
			VDL_DC_HINTCTL_MASK,

	VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_BYPASSTYPE_LSB | VDL_LC_FBFORMAT_32 |
	 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT,
			VDL_LC_BYPASSTYPE | VDL_LC_FBFORMAT |
			 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_YUV_16,
	VTIF_BMTYPES_ALLOWED,
	320, 240,
	1, 1,
	N16TYPES, vt16types,
	2, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_MTXBYPCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_MTXBYPCTL_MASK,

	VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_FBFORMAT_16 | VDL_LC_LD_FBFORMAT,
			VDL_LC_FBFORMAT | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_YUV_32,
	VTIF_BMTYPES_ALLOWED,
	320, 240,
	1, 1,
	N32TYPES, vt32types,
	2, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_MTXBYPCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_MTXBYPCTL_MASK,

	VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_BYPASSTYPE_LSB | VDL_LC_FBFORMAT_32 |
	 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT,
			VDL_LC_BYPASSTYPE | VDL_LC_FBFORMAT |
			 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_YUV_16_640,
	VTIF_BMTYPES_ALLOWED,
	640, 240,
	1, 2,
	N16TYPES, vt16types,
	1, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
	 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_HINTCTL_MASK | VDL_DC_MTXBYPCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
	 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_HINTCTL_MASK | VDL_DC_MTXBYPCTL_MASK,

	VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_FBFORMAT_16 | VDL_LC_LD_FBFORMAT,
			VDL_LC_FBFORMAT | VDL_LC_LD_FBFORMAT
 }, {
	PROJTYPE_NTSC_NOLACE | VIEWTYPE_YUV_32_640,
	VTIF_BMTYPES_ALLOWED,
	640, 240,
	1, 2,
	N32TYPES, vt32types,
	1, 1,
	cvu_16_32,
	mvu_16_32_dual,
	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
	 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_HINTCTL_MASK | VDL_DC_MTXBYPCTL_MASK,

	VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_DISABLE) |
	 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_ENABLE),
			VDL_DC_HINTCTL_MASK | VDL_DC_MTXBYPCTL_MASK,

	VDL_AV_LD_HDOUBLE,
			VDL_AV_HDOUBLE | VDL_AV_LD_HDOUBLE,

	VDL_LC_BYPASSTYPE_LSB | VDL_LC_FBFORMAT_32 |
	 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT,
			VDL_LC_BYPASSTYPE | VDL_LC_FBFORMAT |
			 VDL_LC_LD_BYPASSTYPE | VDL_LC_LD_FBFORMAT
 }
};

#define NVTYPES	(sizeof (bdavtis_ntsc_nl) / sizeof (BDAVTI))


struct ViewTypeInfo *
nextvti_ntsc_nl (p, vti)
struct Projector	*p;
struct ViewTypeInfo	*vti;
{
	TOUCH (p);

	if (vti) {
		BDAVTI	*bv;

		bv = (BDAVTI *) vti;
		if (bv - bdavtis_ntsc_nl < NVTYPES - 1)
			return ((ViewTypeInfo *) (bv + 1));
		else
			return (NULL);
	} else
		return (&bdavtis_ntsc_nl->bv);
}
