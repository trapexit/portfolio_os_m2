#ifndef __HARDWARE_BDA_I
#define __HARDWARE_BDA_I


/******************************************************************************
**
**  @(#) bda.i 96/08/28 1.19
**
******************************************************************************/


#define MAX_PBUS_SLOTS  8

/*
   Various base addresses
*/

#define	BDATE_BASE	(0x00040000)
#define	BDADSP_BASE	(0x00060000)


/*
   Standard BDA device type located at xxx.
*/

#define BDA_DEVICE_ID_TYPE     0
#define UNKNOWN_DEVICE_ID_TYPE (0xff)

#define BDAMCTL_BASE       (0x00020000)
#define BDAMCTL_MCONFIG    (BDAMCTL_BASE + 0)
#define BDAMCTL_MREF       (BDAMCTL_BASE + 4)
#define BDAMCTL_MCNTL      (BDAMCTL_BASE + 8)
#define BDAMCTL_MRESET     (BDAMCTL_BASE + 0x0c)

#define BDAMCFG_LDIA_MASK	0x07000000
#define BDAMCFG_LDOA_MASK	0x00c00000
#define BDAMCFG_RC_MASK		0x003c0000
#define BDAMCFG_RCD_MASK	0x00030000
#define BDAMCFG_SS1_MASK	0x0000e000
#define BDAMCFG_SS0_MASK	0x00001c00
#define BDAMCFG_CL_MASK		0x00000030

#define BDAMCFG_LDIA_SHIFT	24
#define BDAMCFG_LDOA_SHIFT	22
#define BDAMCFG_RC_SHIFT	18
#define BDAMCFG_RCD_SHIFT	16
#define BDAMCFG_SS1_SHIFT	13
#define BDAMCFG_SS0_SHIFT	10
#define BDAMCFG_CL_SHIFT	4

#define BDAMREF_GPIO3_DIR   0x00400000
#define BDAMREF_GPIO3_VALUE 0x00200000

#ifdef  BUILD_BDA1_1

#define BDA11PCTL_BASE       (0x00001000)
#define BDA11PCTL_DEVID      (BDA11PCTL_BASE + 0)
#define BDA11PCTL_PBCONTROL  (BDA11PCTL_BASE + 0x50)
#define BDA11PCTL_PBINTSTAT  (BDA11PCTL_BASE + 0x54)
#define BDA11PCTL_PBINTENSET (BDA11PCTL_BASE + 0x58)
#define BDA11PCTL_ERRADDR    (BDA11PCTL_BASE + 0x5c)
#define BDA11PCTL_ERRSTAT    (BDA11PCTL_BASE + 0x60)

#define BDA20PCTL_BASE       (0x00010000)
#define BDA20PCTL_DEVID      (BDA20PCTL_BASE + 0)
#define BDA20PCTL_PBCONTROL  (BDA20PCTL_BASE + 0x10)
#define BDA20PCTL_PBINTENSET (BDA20PCTL_BASE + 0x40)
#define BDA20PCTL_PBINTSTAT  (BDA20PCTL_BASE + 0x50)
#define BDA20PCTL_ERRSTAT    (BDA20PCTL_BASE + 0x60)
#define BDA20PCTL_ERRADDR    (BDA20PCTL_BASE + 0x70)

#endif

#define BDAPCTL_BASE         (0x00010000)
#define BDAPCTL_DEVID        (BDAPCTL_BASE + 0)
#define BDAPCTL_PBCONTROL    (BDAPCTL_BASE + 0x10)
#define BDAPCTL_PBINTENSET   (BDAPCTL_BASE + 0x40)
#define BDAPCTL_PBINTSTAT    (BDAPCTL_BASE + 0x50)
#define BDAPCTL_ERRSTAT      (BDAPCTL_BASE + 0x60)
#define BDAPCTL_ERRADDR      (BDAPCTL_BASE + 0x70)

#define BDAINT_EXTD_MASK       0x00000078
#define BDAINT_PVIOL_MASK      0x00000080
#define BDAINT_WVIOL_MASK      0x00000100
#define BADINT_TO_MASK         0x00000200
#define BDAINT_INTERNAL_MASK   0xfffffc00
#define BDAINT_TRIWINCLIP_MASK 0x80000000
#define BDAINT_TRILISTEND_MASK 0x40000000
#define BDAINT_TRIIMINST_MASK  0x20000000
#define BDAINT_TRIDFINST_MASK  0x10000000
#define BDAINT_TRIGEN_MASK     0x08000000
#define BDAINT_MPEG_MASK       0x04000000
#define BDAINT_DSP_MASK        0x02000000
#define BDAINT_VINT0_MASK      0x01000000
#define BDAINT_VINT1_MASK      0x00800000
#define BDAINT_MYSTERY_MASK    0x00400000
#define BDAINT_CEL_MASK        0x00200000

#define BDAPCTL_PREF_MASK      0x80000000
#define BDAPCTL_TOCYC_MASK     0x7fffc000
#define BDAPCTL_1BWR_MASK      0x00000300
#define BDAPCTL_4BWR_MASK      0x00000070
#define BDAPCTL_TOCYC_SHIFT    (31-17)

#define BDAVDU_BASE	(0x00030000)
#define BDAVDU_VLOC	(BDAVDU_BASE + 0)
#define BDAVDU_VINT	(BDAVDU_BASE + 4)
#define BDAVDU_VDC0	(BDAVDU_BASE + 8)
#define BDAVDU_VDC1	(BDAVDU_BASE + 0x0c)
#define BDAVDU_FV0A	(BDAVDU_BASE + 0x10)
#define BDAVDU_FV1A	(BDAVDU_BASE + 0x14)
#define BDAVDU_AVDI	(BDAVDU_BASE + 0x1c)
#define BDAVDU_VDLI	(BDAVDU_BASE + 0x20)
#define BDAVDU_VCFG	(BDAVDU_BASE + 0x24)
#define BDAVDU_DMT0	(BDAVDU_BASE + 0x28)
#define BDAVDU_DMT1	(BDAVDU_BASE + 0x2c)
#define BDAVDU_LFSR	(BDAVDU_BASE + 0x30)
#define BDAVDU_VRST	(BDAVDU_BASE + 0x34)

#define BDAVINT0_MASK      0x80000000
#define BDAVINT1_MASK      0x00008000
#define BDAVLINE0_MASK     0x7ff00000
#define BDAVLINE1_MASK     0x00007ff0

#define BDACP_BASE         (0x00070000)
#define BDACP_CPIS         (BDACP_BASE + 0)
#define BDACP_CPIE         (BDACP_BASE + 4)
#define BDACP_CPCFG1       (BDACP_BASE + 8)
#define BDACP_CPCFG2       (BDACP_BASE + 0x0c)
#define BDACP_CPST         (BDACP_BASE + 0x10)
#define BDACP_CPIA         (BDACP_BASE + 0x14)
#define BDACP_CPOA         (BDACP_BASE + 0x18)
#define BDACP_CPCT         (BDACP_BASE + 0x1c)

#endif


/*****************************
  The MPEG unit
******************************/

#define BDAMPEG_BASE	(0x00080000)
#define BDAMPEG_DEVID	(BDAMPEG_BASE + 0)

/* WHOAMI register definitions (MP) */
#define WHOAMI_BASE         (0x10000000)

/* The WHOAMI register currently returns one valid bit, the high
 * bit, setting it for accesses from the slave CPU and clearing it
 * for accesses from the master CPU.
 */
#define WHOAMI_CPUMASK      (0x80000000)
#define WHOAMI_CPUMASKBIT	0

#define	WHOAMI_SINGLECPU	0xFFFFFFFF
#define	WHOAMI_MASTERCPU	0x00000000
#define	WHOAMI_SLAVECPU		0x80000000


