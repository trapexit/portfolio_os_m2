#ifndef __HARDWARE_BDA_H
#define __HARDWARE_BDA_H


/******************************************************************************
**
**  @(#) bda.h 96/07/25 1.38
**
******************************************************************************/


/*
    BDA is split into a number of different subunits, each with seperate
    register sets.  The only approved way (so says me!) to access bda is with
    one of the BDAxxx_READ or BDAxxx_WRITE macros.  This will allow us to
    simulate some portions of BDA with CLIO/MADAM (basically memory controller
    stuff)

    - Kevin Hester
*/

/*
   Standard BDA device type located at xxx.
*/

#define BDA_DEVICE_ID_TYPE     0
#define UNKNOWN_DEVICE_ID_TYPE (0xff)

/************************
Various base addresses
*************************/

#define	BDATE_BASE	(0x00040000)
#define	BDADSP_BASE	(0x00060000)

/************************
   Standard offsets for BDA memcontroller
   use with BDAMCTL_READ/WRITE.
*************************/

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

#define	BDAMREF_DEBUGADDR	0x7F000000  /* Selector if GPIOx_GP == 0 */
#define	BDAMREF_GPIO3_GP	0x00800000  /* General purpose or debug out */
#define	BDAMREF_GPIO3_OUT	0x00400000  /* Output or input */
#define	BDAMREF_GPIO3_VALUE	0x00200000  /* Value if GPIOx_GP == 1 */
#define	BDAMREF_GPIO2_GP	0x00100000  /* General purpose or debug out */
#define	BDAMREF_GPIO2_OUT	0x00080000  /* Output or input */
#define	BDAMREF_GPIO2_VALUE	0x00040000  /* Value if GPIOx_GP == 1 */
#define	BDAMREF_GPIO1_GP	0x00020000  /* General purpose or debug out */
#define	BDAMREF_GPIO1_OUT	0x00010000  /* Output or input */
#define	BDAMREF_GPIO1_VALUE	0x00008000  /* Value if GPIOx_GP == 1 */
#define	BDAMREF_GPIO0_GP	0x00004000  /* General purpose or debug out */
#define	BDAMREF_GPIO0_OUT	0x00002000  /* Output or input */
#define	BDAMREF_GPIO0_VALUE	0x00001000  /* Value if GPIOx_GP == 1 */
#define	BDAMREF_REFRESH		0x00000FFF  /* Memory refresh count */

#define BDAMREF_GPIO_CTRL_MASK(bit)	(3 << (bit * 3 + 13))
#define BDAMREF_GPIO_IN(bit, template)	template &= ~BDAMREF_GPIO_CTRL_MASK(bit)
#define BDAMREF_GPIO_OUT(bit, template)	template |= BDAMREF_GPIO_CTRL_MASK(bit)

#define BDAMREF_GPIO_VAL_MASK(bit)	(1 << (bit * 3 + 12))
#define BDAMREF_GPIO_GET(bit, template)	template & BDAMREF_GPIO_VAL_MASK(bit)
#define BDAMREF_GPIO_CLR(bit, template)	template &= ~BDAMREF_GPIO_VAL_MASK(bit)
#define BDAMREF_GPIO_SET(bit, template)	template |= BDAMREF_GPIO_VAL_MASK(bit)

#define	BDAMREF_GPIO_AUDIOMUTE	0           /* Mute audio: 0=mute, 1=un-mute */
#define	BDAMREF_GPIO_OPERAFLAG	1           /* 0=No Opera connected, 1=Opera connected */
#define	BDAMREF_GPIO_INTERLACE	2           /* Interlace mode */
#define	BDAMREF_GPIO_UNUSED	3           /* not defined */


/*************************
   Standard offsets for BDA powerbus controller
*************************/

#ifdef	BUILD_BDA1_1

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

/* Masks for PowerBus registers */
/* Stuff for the interrupt enable/status registers */

#define BDAINT_EXTD_MASK       0x00000078
#define BDAINT_EXTD1_MASK      0x00000040
#define BDAINT_EXTD2_MASK      0x00000020
#define BDAINT_EXTD3_MASK      0x00000010
#define BDAINT_EXTD4_MASK      0x00000008
#define BDAINT_BRDG_MASK       BDAINT_EXTD3_MASK
#define BDAINT_CDE_MASK        BDAINT_EXTD4_MASK
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

/* Stuff for the PBCTL register */
#define BDAPCTL_PREF_MASK      0x80000000
#define BDAPCTL_TOCYC_MASK     0x7fffc000
#define BDAPCTL_1BWR_MASK      0x00000300
#define BDAPCTL_4BWR_MASK      0x00000070
#define BDAPCTL_TOCYC_SHIFT    (31-17)

/*************************
   That whacky VDU
**************************/

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


/* Masks for VDU registers */
/* You are encouraged to use the definitions in m2vdl.h.  */

#define BDAVINT0_MASK	0x80000000
#define BDAVINT1_MASK	0x00008000
#define BDAVLINE0_MASK	0x7ff00000
#define BDAVLINE1_MASK	0x00007ff0

/*****************************
   Cel engine
   (Can we get rid of this yet?)
******************************/

#define BDACEL_BASE	(0x00050000)

#define BDACEL_STATBITS	(BDACEL_BASE + 0x28)

#define BDACEL_SPRSTRT	(BDACEL_BASE + 0x100)
#define BDACEL_SPRSTOP	(BDACEL_BASE + 0x104)
#define BDACEL_SPRCNTU	(BDACEL_BASE + 0x108)
#define BDACEL_SPRPAUS	(BDACEL_BASE + 0x10c)
#define BDACEL_CCOBCTL0	(BDACEL_BASE + 0x110)

#define BDACEL_PPMPC	(BDACEL_BASE + 0x120)

#define BDACEL_REGCTL0	(BDACEL_BASE + 0x130)
#define BDACEL_REGCTL1	(BDACEL_BASE + 0x134)
#define BDACEL_REGCTL2	(BDACEL_BASE + 0x138)
#define BDACEL_REGCTL3	(BDACEL_BASE + 0x13c)
#define BDACEL_XYPOSH	(BDACEL_BASE + 0x140)
#define BDACEL_XYPOSL	(BDACEL_BASE + 0x144)
#define BDACEL_LINEDXYH	(BDACEL_BASE + 0x148)
#define BDACEL_LINEDXYL	(BDACEL_BASE + 0x14c)
#define BDACEL_DXYH	(BDACEL_BASE + 0x150)
#define BDACEL_DXYL	(BDACEL_BASE + 0x154)
#define BDACEL_DDXYH	(BDACEL_BASE + 0x158)
#define BDACEL_DDXYL	(BDACEL_BASE + 0x15c)

#define BDACEL_PIPTABLE	(BDACEL_BASE + 0x180)

#define BDACEL_DMACELCNTL	(BDACEL_BASE + 0x5a0)
#define BDACEL_DMAFIRSTCCOB	(BDACEL_BASE + 0x5a4)
#define BDACEL_DMAPIP	(BDACEL_BASE + 0x5a8)
#define BDACEL_DMACELDATA	(BDACEL_BASE + 0x5ac)
#define BDACEL_DATAA	(BDACEL_BASE + 0x5b0)
#define BDACEL_LENA	(BDACEL_BASE + 0x5b4)
#define BDACEL_DATAB	(BDACEL_BASE + 0x5b8)
#define BDACEL_LENB	(BDACEL_BASE + 0x5bc)

#define BDACEL_BULL_CNTL	(BDACEL_BASE + 0x1000)
#define BDACEL_BULL_INTBITSSET	(BDACEL_BASE + 0x1010)
#define BDACEL_BULL_INTBITSCLR	(BDACEL_BASE + 0x1020)
#define BDACEL_CEI_INTERRUPT	(BDACEL_BASE + 0x1030)
#define BDACEL_CEL_RESET	(BDACEL_BASE + 0x1040)

#define BDACEL_RCACHE0	(BDACEL_BASE + 0x2000)
#define BDACEL_RCACHE1	(BDACEL_BASE + 0x2004)
#define BDACEL_RCACHE2	(BDACEL_BASE + 0x2008)
#define BDACEL_RCACHE3	(BDACEL_BASE + 0x200c)

#define BDACEL_WCACHE0	(BDACEL_BASE + 0x2020)
#define BDACEL_WCACHE1	(BDACEL_BASE + 0x2024)
#define BDACEL_WCACHE2	(BDACEL_BASE + 0x2028)
#define BDACEL_WCACHE3	(BDACEL_BASE + 0x202c)

#define BDACEL_RLRU0	(BDACEL_BASE + 0x2040)
#define BDACEL_RLRU1	(BDACEL_BASE + 0x2044)
#define BDACEL_RLRU2	(BDACEL_BASE + 0x2048)
#define BDACEL_RLRU3	(BDACEL_BASE + 0x204c)

#define BDACEL_WLRU0	(BDACEL_BASE + 0x2060)
#define BDACEL_WLRU1	(BDACEL_BASE + 0x2064)
#define BDACEL_WLRU2	(BDACEL_BASE + 0x2068)
#define BDACEL_WLRU3	(BDACEL_BASE + 0x206c)

#define BDACEL_BULL_RAPSNOOP_BIT	0x00000040
#define BDACEL_BULL_DATASNOOP_BIT	0x00000020
#define BDACEL_BULL_CCOBSNOOP_BIT	0x00000010
#define BDACEL_BULL_ALADININT_BIT	0x00000008
#define BDACEL_BULL_RCACHEINT_BIT	0x00000004
#define BDACEL_BULL_WCACHEINT_BIT	0x00000002
#define BDACEL_BULL_CELCOMINT_BIT	0x00000001



/*****************************
   Those amazing control ports
******************************/

#define BDACP_BASE         (0x00070000)
#define BDACP_CPIS         (BDACP_BASE + 0)
#define BDACP_CPIE         (BDACP_BASE + 4)
#define BDACP_CPCFG1       (BDACP_BASE + 8)
#define BDACP_CPCFG2       (BDACP_BASE + 0x0c)
#define BDACP_CPST         (BDACP_BASE + 0x10)
#define BDACP_CPIA         (BDACP_BASE + 0x14)
#define BDACP_CPOA         (BDACP_BASE + 0x18)
#define BDACP_CPCT         (BDACP_BASE + 0x1c)

#define BDACP_INT_INPUT_OVER_MASK       0x4
#define BDACP_INT_OUTPUT_UNDER_MASK     0x2
#define BDACP_INT_DMA_COMPLETE_MASK     0x1

/*****************************
   The MPEG unit
******************************/

#define	BDAMPEG_BASE	(0x00080000)
#define	BDAMPEG_DEVID	(BDAMPEG_BASE + 0)


#ifdef SIM_BDA
typedef void (*sim_bda_write_type)(uint32 regoffset, uint32 value);
typedef uint32 (*sim_bda_read_type)(uint32 regoffset);

#define sim_bda_write_vect ((sim_bda_write_type) 0x4ff00120)
#define sim_bda_read_vect  ((sim_bda_read_type) 0x4ff00130)

#define BDA_WRITE(regoffset, value) (*sim_bda_write_vect(regoffset, value)
#define BDA_READ(regoffset)         (*sim_bda_read_vect)(regoffset)
#else	/* !SIM_BDA */

/* DO NOT CALL BDA_READ_LOW - this is for kevin hester only */
#define BDA_READ_LOW(regoffset)     (*((vuint32 *) (regoffset)))
#define BDA_WRITE_LOW(regoffset, value) (*((vuint32 *) (regoffset)))=(value)


#ifdef BDA_CTL_BROKEN
extern uint32 __bdaread(uint32 regoffset);
/* We supposedly don't need this hack for 1.1 parts */
#define BDA_READ(regoffset)         __bdaread(regoffset)
/* We do an extra read for BDA bug avoidance - kevinh */
#define BDA_WRITE(regoffset, value) { 	  \
					  *((vuint32 *) (regoffset)) = value;  \
					  BDA_READ(regoffset); \
				    }
#else
#define BDA_READ(regoffset)	    BDA_READ_LOW(regoffset)
#define BDA_WRITE(regoffset, value) BDA_WRITE_LOW(regoffset, value)
#endif


#endif	/* SIM_BDA */

#define BDA_CLR(regoffset,value)   BDA_WRITE((regoffset) + 0x400, value)

/* Number of PowerBus slots */
#define	MAX_PBUS_SLOTS		8

/* Get the base address of a PowerBus slot. */
#define	PBUSADDR(pslot)		(((uint32)(pslot)) << 24)

/* WHOAMI register definitions (MP) */
#define WHOAMI_BASE			(0x10000000)

/* The WHOAMI register currently returns one valid bit, the high
 * bit, setting it for accesses from the slave CPU and clearing it
 * for accesses from the master CPU.
 */
#define WHOAMI_CPUMASK		(0x80000000)
#define WHOAMI_CPUMASKBIT	0

#define	WHOAMI_SINGLECPU	0xFFFFFFFF
#define	WHOAMI_MASTERCPU	0x00000000
#define	WHOAMI_SLAVECPU		0x80000000


#endif
