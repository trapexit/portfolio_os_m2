#ifndef	__HARDWARE_M2VDL_H
#define	__HARDWARE_M2VDL_H


/******************************************************************************
**
**  @(#) m2vdl.h 96/05/01 1.8
**
******************************************************************************/


/***************************************************************************
 ***************************************************************************
 *
 * VDL bit and field definitions.
 *
 */

/***************************************************************************
 * DMA Control word.
 */
#define	VDL_DMA_MOD_MASK	0xFF000000
#define	VDL_DMA_ENABLE		0x00200000
#define	VDL_DMA_NOBUCKET	0x00020000	/*Don't xfer lower to upper*/
#define	VDL_DMA_LDLOWER		0x00010000
#define	VDL_DMA_LDUPPER		0x00008000
#define	VDL_DMA_NWORDS_MASK	0x00007E00
#define	VDL_DMA_NLINES_MASK	0x000001FF
#define	VDL_DMA_RESERVED	0x00DC0000

#define	VDL_DMA_NWORDS_SHIFT	9
#define	VDL_DMA_NWORDSFIELD(n)	((n) << VDL_DMA_NWORDS_SHIFT)

#define	VDL_DMA_MOD_SHIFT	24
#define	VDL_DMA_MOD_FIELD(wide)	(((wide) >> 5) << VDL_DMA_MOD_SHIFT)

#define	VDL_DMA_NLINES_SHIFT		0
#define	VDL_DMA_NLINESFIELD(wide)	((wide) << VDL_DMA_NLINES_SHIFT)


/***************************************************************************
 * Display Control word.
 */
#define	VDL_DC			0x80000000
#define	VDL_DC_0		0x00000000
#define	VDL_DC_1		0x10000000
#define	VDL_DC_HINTCTL_MASK	0x00060000
#define	VDL_DC_VINTCTL_MASK	0x00018000
#define	VDL_DC_DITHERCTL_MASK	0x00001800
#define	VDL_DC_MTXBYPCTL_MASK	0x00000600
#define	VDL_DC_RESERVED		0x0FF861FF


#define	VDL_DC_HINTCTL		17
#define	VDL_DC_VINTCTL		15
#define	VDL_DC_DITHERCTL	11
#define	VDL_DC_MTXBYPCTL	9

#define	VDL_CTL_DISABLE		0
#define	VDL_CTL_ENABLE		1
#define	VDL_CTL_NOP		2

#define	VDL_CTL_FIELD(field,ctl)	((ctl) << (field))


#define	VDL_DC_NOP	(VDL_CTL_FIELD (VDL_DC_HINTCTL, VDL_CTL_NOP) | \
			 VDL_CTL_FIELD (VDL_DC_VINTCTL, VDL_CTL_NOP) | \
			 VDL_CTL_FIELD (VDL_DC_DITHERCTL, VDL_CTL_NOP) | \
			 VDL_CTL_FIELD (VDL_DC_MTXBYPCTL, VDL_CTL_NOP))


/***************************************************************************
 * Active Video display control word.
 */
#define	VDL_AV			0xA0000000
#define	VDL_AV_HSTART_MASK	0x1FFC0000	/*  640 resolution	*/
#define	VDL_AV_LD_HSTART	0x00020000
#define	VDL_AV_HWIDTH_MASK	0x0001FFC0	/*  # of pixels to fetch*/
#define	VDL_AV_LD_HWIDTH	0x00000020
#define	VDL_AV_HDOUBLE		0x00000010
#define	VDL_AV_VDOUBLE		0x00000008
#define	VDL_AV_LD_HDOUBLE	0x00000004
#define	VDL_AV_LD_VDOUBLE	0x00000002
#define	VDL_AV_RESERVED		0x00000001


#define	VDL_AV_HSTART		18
#define	VDL_AV_HWIDTH		6

#define	VDL_AV_FIELD(field,val)	((val) << (field))


#define	VDL_AV_NOP		VDL_AV


/***************************************************************************
 * Video Display List Control word.
 */
#define	VDL_LC			0xC0000000
#define	VDL_LC_BYPASSTYPE	0x02000000
#define	VDL_LC_FBFORMAT		0x00800000
#define	VDL_LC_ONEVINTDIS	0x00080000
#define	VDL_LC_RANDOMDITHER	0x00040000
#define	VDL_LC_LD_BYPASSTYPE	0x00002000
#define	VDL_LC_LD_FBFORMAT	0x00001000
#define	VDL_LC_RESERVED		0x1D73CFFF


#define	VDL_LC_BYPASSTYPE_MSB	0x00000000
#define	VDL_LC_BYPASSTYPE_LSB	0x02000000

#define	VDL_LC_FBFORMAT_16	0x00000000
#define	VDL_LC_FBFORMAT_32	0x00800000


#define	VDL_LC_NOP		VDL_LC


#if 0
/***************************************************************************
 * CLUT Control.
 * Retained for reasons of affection...
 */
#define	VDL_CLUT_RGB		0x00000000
#define	VDL_CLUT_R		0x20000000
#define	VDL_CLUT_G		0x40000000
#define	VDL_CLUT_B		0x60000000
#define	VDL_CLUT_BACKGROUND	0xE0000000
#define	VDL_CLUT_IDX_MASK	0x1F000000

#define	VDL_CLUT_IDX_SHIFT	24

/*
 * Example use:
 *
 * foo = VDL_CLUT_ENTRY (VDL_CLUT_RGB, 12, 128, 128, 128);
 */
#define	VDL_CLUT_ENTRY(type,idx,r,g,b)	\
				((type) | \
				 ((idx) << VDL_CLUT_IDX_SHIFT) | \
				 ((r) << 16) | \
				 ((g) << 8) | \
				 (b))
#endif /*  0  */


/***************************************************************************
 * A no-op VDL control word (defined by the docs).
 */
#define	VDL_NOP			0xE1000000


/***************************************************************************
 ***************************************************************************
 *
 * Hardware register bit definitions.
 *
 */

/***************************************************************************
 * BDAVDU_VLOC
 * Video display location register fields.
 */
#define VDU_VLOC_VIDEOFIELD	0x00004000
#define VDU_VLOC_VCOUNT_MASK	0x00003FF8
#define	VDU_VLOC_RESERVED	0xFFFF8007

#define VDU_VLOC_VCOUNT_SHIFT	3


/***************************************************************************
 * BDAVDU_VINT
 * Vertical line interrupt register fields.
 */
#define VDU_VINT_VINT0		0x80000000
#define VDU_VINT_VLINE0_MASK	0x7FF00000
#define VDU_VINT_VINT1		0x00008000
#define VDU_VINT_VLINE1_MASK	0x00007FF0
#define	VDU_VINT_RESERVED	0x000F000F

#define VDU_VINT_VLINE0_SHIFT	20
#define VDU_VINT_VLINE1_SHIFT	4


/***************************************************************************
 * BDAVDU_VDC0
 * BDAVDU_VDC1
 * Video display control register fields.
 *
 * Use the VDL_HVSRC_ and VDL_BLSBSRC_ definitions above to interpret
 * corresponding fields.
 */
#define VDU_VDC_HINT		0x02000000
#define VDU_VDC_VINT		0x01000000
#define VDU_VDC_DITHER		0x00400000
#define VDU_VDC_MTXBYP		0x00200000
#define	VDU_VDC_RESERVED	0xFC9FFFFF


/***************************************************************************
 * BDAVDU_AVDI
 * Active video display info register fields.
 */
#define VDU_AVDI_HSTART_MASK	0xFFE00000
#define VDU_AVDI_HWIDTH_MASK	0x0003FF80
#define VDU_AVDI_HDOUBLE	0x00000008
#define VDU_AVDI_VDOUBLE	0x00000004
#define VDU_AVDI_RESERVED	0x001C0073

#define VDU_AVDI_HSTART_SHIFT	21
#define VDU_AVDI_HEND_SHIFT	7


/***************************************************************************
 * BDAVDU_VDLI
 * Video display list info register fields.
 *
 * Careful of the VDLI_ definitions, which look like, but are NOT the same
 * as, their counterparts above.
 */
#define	VDU_VDLI_BYPASSTYPE		0x10000000
#define	VDU_VDLI_FBFORMAT		0x04000000
#define VDU_VDLI_ONEVINTDIS		0x00400000
#define VDU_VDLI_RANDOMDITHER		0x00200000
#define VDU_VDLI_RESERVED		0xEB9FFFFF

#define	VDU_VDLI_BYPASSTYPE_MSB		0
#define	VDU_VDLI_BYPASSTYPE_LSB		0x10000000

#define	VDU_VDLI_FBFORMAT_16		0
#define	VDU_VDLI_FBFORMAT_32		0x04000000


/***************************************************************************
 * BDAVDU_VCFG
 * Video configuration register fields.
 */
#define	VDU_VCFG_SLOWMEM	0x80000000
#define	VDU_VCFG_HSYNCPHASE	0x40000000
#define	VDU_VCFG_FIFHP_MASK	0x00000003
#define	VDU_VCFG_RESERVED	0x3FFFFFFC


#define VDU_VCFG_FIFHP_SHIFT	0

#define VDU_VCFG_FIFHP_32	(0 << VDU_DSNP_FIFHP_SHIFT)
#define VDU_VCFG_FIFHP_64	(1 << VDU_DSNP_FIFHP_SHIFT)
#define VDU_VCFG_FIFHP_96	(2 << VDU_DSNP_FIFHP_SHIFT)
#define VDU_VCFG_FIFHP_ALWAYS	(3 << VDU_DSNP_FIFHP_SHIFT)


/***************************************************************************
 * BDAVDU_VRST
 * Video reset register fields.
 */
#define	VDU_VRST_DVERESET	0x00000002
#define	VDU_VRST_VIDRESET	0x00000001
#define	VDU_VRST_RESERVED	0xFFFFFFFC


/***************************************************************************
 ***************************************************************************
 *
 * Helpful structure definitions.
 *
 */
/*
 * Common header to all VDL lists.  These four fields are required by
 * the Opera hardware, in this order.  (They're also kinda required on
 * M2 hardware as well.)
 */
typedef struct VDLHeader {
	uint32			DMACtl;
	void			*LowerPtr;
	void			*UpperPtr;
	struct VDLHeader	*NextVDL;
} VDLHeader;


/*
 * The most common flavor of VDL.  The 33rd color is the background register.
 * (Well, not so common anymore since $(UNNAMED_ENGINEER) yanked the CLUTs
 * due to space constraints.)
 */
typedef struct FullVDL {
	struct VDLHeader	fv;
	uint32			fv_DispCtl0;
	uint32			fv_DispCtl1;
	uint32			fv_AVCtl;
	uint32			fv_ListCtl;
	uint32			fv_Colors[33];
	uint32			__pad0;
} FullVDL;

/*  Fencepost is to drop __pad0 field.  */
#define	VDL_NWORDS_FULL		(sizeof (FullVDL) / sizeof (uint32) - 1)
#define	VDL_NWORDS_FULL_FMT	(VDL_DMA_NWORDSFIELD (VDL_NWORDS_FULL))


/*
 * This flavor is used only by the system internally for stuff like the
 * forced CLUT transfer at VBlank and whatnot...
 */
typedef struct ShortVDL {
	struct VDLHeader	sv;
	uint32			sv_DispCtl0;
	uint32			sv_DispCtl1;
	uint32			sv_AVCtl;
	uint32			sv_ListCtl;
} ShortVDL;

#define	VDL_NWORDS_SHORT	(sizeof (ShortVDL) / sizeof (uint32))
#define	VDL_NWORDS_SHORT_FMT	(VDL_DMA_NWORDSFIELD (VDL_NWORDS_SHORT))


#endif	/*  __HARDWARE_M2VDL_H  */
