#ifndef __HARDWARE_CDE_H
#define __HARDWARE_CDE_H


/******************************************************************************
**
**  @(#) cde.h 96/07/22 1.31
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************
  Defines
*/

typedef struct CDE
{
	uint32	cde_DeviceId;
	uint32	cde_DeviceRev;
};

#define CDE_DEVICEREVMASK	0xFF

/* This is the value CDE stores in the type field of CDE_DEVICE_ID */
#define CDE_DEVICE_ID_TYPE	1
/* Maximum size range of CDE relative to its base */
#define	CDE_DEVICE_RANGE	((1 << 24) - 1)

/* Offset to the corresponding clear registers */
/* Also OR in this value in the real system to access the clear registers */
#define CDE_CLEAR_OFFSET	0x400

/* Misc registers */
#define CDE_DEVICE_ID		0x000
#define CDE_DEVICE_REV		0x004
#define CDE_SDBG_CNTL		0x00C
#define CDE_SDBG_RD		0x010
#define CDE_SDBG_WRT		0x014
#define CDE_INT_STS		0x018
#define CDE_INT_ENABLE		0x01C
#define CDE_RESET_CNTL		0x020
#define CDE_ROM_DISABLE		0x024
#define CDE_CD_CMD_WRT		0x028
#define CDE_CD_STS_RD		0x02C
#define CDE_GPIO1		0x030
#define CDE_GPIO2		0x034
#define CDE_INT_STC		0x418		/* sigh */
#define	CDE_INT_MASKC		0x41C

/* ARM registers */
#define CDE_ARM_BASE		0x100
#define CDE_ARM_FENCE1		0x104
#define CDE_ARM_FENCE2		0x108
#define CDE_ARM_FENCE3		0x10C
#define CDE_ARM_FENCE4		0x110
#define CDE_ARM_CNTL		0x114

/* BIO registers */
#define CDE_DEV_DETECT		0x200
#define CDE_BBLOCK		0x204
#define CDE_BBLOCK_EN		0x208
#define CDE_DEV5_CONF		0x20C
#define CDE_DEV_STATE		0x210
#define CDE_DEV6_CONF		0x214
#define CDE_DEV5_VISA_CONF	0x218
#define CDE_DEV6_VISA_CONF	0x21C
#define CDE_UNIQ_ID_CMD		0x220
#define CDE_UNIQ_ID_RD		0x224
#define CDE_DEV_ERROR		0x228
#define CDE_DEV7_CONF		0x22C
#define CDE_DEV7_VISA_CONF	0x230
#define CDE_DEV0_SETUP		0x240
#define CDE_DEV0_CYCLE_TIME	0x244
#define CDE_DEV1_SETUP		0x248
#define CDE_DEV1_CYCLE_TIME	0x24C
#define CDE_DEV2_SETUP		0x250
#define CDE_DEV2_CYCLE_TIME	0x254
#define CDE_DEV5_SETUP		0x268
#define CDE_DEV5_CYCLE_TIME	0x26C
#define CDE_DEV6_SETUP		0x270
#define CDE_DEV6_CYCLE_TIME	0x274
#define CDE_DEV7_SETUP		0x278
#define CDE_DEV7_CYCLE_TIME	0x27C
#define	CDE_SYSTEM_CONF		0x280
#define	CDE_VISA_DIS		0x284
#define	CDE_MICRO_RWS		0x290
#define	CDE_MICRO_WI		0x294
#define	CDE_MICRO_WOB		0x298
#define	CDE_MICRO_WO		0x29C
#define	CDE_MICRO_STATUS	0x2A0


/* CD DMA registers */
#define CDE_CD_DMA1_CNTL	0x300	/* Control */
#define CDE_CD_DMA1_CPAD	0x308	/* Curr PBus addr */
#define CDE_CD_DMA1_CCNT	0x30C	/* Curr count */
#define CDE_CD_DMA1_NPAD	0x318	/* Next PBus addr */
#define CDE_CD_DMA1_NCNT	0x31C	/* Next count */

#define CDE_CD_DMA2_CNTL	0x320	/* See above */
#define CDE_CD_DMA2_CPAD	0x328
#define CDE_CD_DMA2_CCNT	0x32C
#define CDE_CD_DMA2_NPAD	0x338
#define CDE_CD_DMA2_NCNT	0x33C

/* CD DMA Etc... */
#define CDE_CD_DMAx_CNTL(x)	(0x300+(20*(x-1)))
#define CDE_CD_DMAx_CPAD(x)	(0x308+(20*(x-1)))
#define CDE_CD_DMAx_CCNT(x)	(0x30C+(20*(x-1)))
#define CDE_CD_DMAx_NPAD(x)	(0x318+(20*(x-1)))
#define CDE_CD_DMAx_NCNT(x)	(0x31C+(20*(x-1)))

/* DMA registers */
#define CDE_DMA1_CNTL		0x1000	/* Control */
#define CDE_DMA1_CBAD		0x1004	/* Curr BioBus addr */
#define CDE_DMA1_CPAD		0x1008	/* Curr PBus addr */
#define CDE_DMA1_CCNT		0x100C	/* Curr count */
#define CDE_DMA1_NBAD		0x1014	/* Next BioBus addr */
#define CDE_DMA1_NPAD		0x1018	/* Next PBus addr */
#define CDE_DMA1_NCNT		0x101C	/* Next count */

#define CDE_DMA2_CNTL		0x1020	/* See above */
#define CDE_DMA2_CBAD		0x1024
#define CDE_DMA2_CPAD		0x1028
#define CDE_DMA2_CCNT		0x102C
#define CDE_DMA2_NBAD		0x1034
#define CDE_DMA2_NPAD		0x1038
#define CDE_DMA2_NCNT		0x103C

/* DMA Etc... */
#define CDE_DMAx_CNTL(x)	(0x1000+(20*(x-1)))
#define CDE_DMAx_CBAD(x)	(0x1004+(20*(x-1)))
#define CDE_DMAx_CPAD(x)	(0x1008+(20*(x-1)))
#define CDE_DMAx_CCNT(x)	(0x100C+(20*(x-1)))
#define CDE_DMAx_NBAD(x)	(0x1014+(20*(x-1)))
#define CDE_DMAx_NPAD(x)	(0x1018+(20*(x-1)))
#define CDE_DMAx_NCNT(x)	(0x101C+(20*(x-1)))


/* Interrupt status/enable bits - R/S/C */
#define CDE_INT_SENT		0x80000000
#define CDE_SDBG_WRT_DONE	0x10000000
#define CDE_SDBG_RD_DONE	0x08000000
#define CDE_DIPIR		0x04000000
#define CDE_ARM_BOUNDS		0x01000000
#define CDE_DMA2_BLOCKED	0x00400000
#define CDE_DMA1_BLOCKED	0x00200000
#define CDE_ID_READY		0x00100000
#define CDE_ARM_FENCE		0x00080000
#define CDE_3DO_CARD_INT	0x00020000
#define CDE_ARM_INT		0x00010000
#define CDE_CD_DMA2_OF		0x00004000
#define CDE_CD_DMA1_OF		0x00002000
#define CDE_ARM_ABORT		0x00001000
#define CDE_CD_DMA2_DONE	0x00000800
#define CDE_CD_DMA1_DONE	0x00000400
#define CDE_DMA2_DONE		0x00000100
#define CDE_DMA1_DONE		0x00000080
#define CDE_PBUS_ERROR		0x00000040
#define CDE_CD_CMD_WRT_DONE	0x00000020
#define CDE_CD_STS_RD_DONE	0x00000010
#define CDE_CD_STS_FL_DONE	0x00000008
#define CDE_GPIO1_INT		0x00000004
#define CDE_GPIO2_INT		0x00000002
#define CDE_BBUS_ERROR		0x00000001

/* Bits in CDE_RESET_CNTL - S */
#define CDE_SOFT_RESET		0x00000001
#define	CDE_HARD_RESET		0x00000002

/* ROM disable control - S */
#define CDE_ROM_NO_MIRROR	0x00000001

/* Bits in CDE_CD_STS_RD */
#define CDE_CD_STS_READY	0x00000100

/* Bits in CDE_GPIOx */
#define CDE_GPIO_OUT_VALUE	0x00000001
#define CDE_GPIO_DIRECTION	0x00000002
#define CDE_GPIO_OESEL		0x00000004
#define CDE_GPIO_INT_NEG	0x00000008
#define CDE_GPIO_INT_POS	0x00000010
#define CDE_GPIO_RD_VALUE	0x00000020

/* Bits in CDE_DEV_DETECT */
#define CDE_DEV5_DETECT		0x00000004
#define CDE_DEV1_PRESENT	0x00000008
#define CDE_DEV2_PRESENT	0x00000010
#define CDE_DEV3_PRESENT	0x00000020
#define CDE_DEV4_PRESENT	0x00000040
#define CDE_DEV5_PRESENT	0x00000080
#define CDE_DEV6_PRESENT	0x00000100
#define CDE_DEV7_PRESENT	0x00000200

/* Bits in CDE_BBLOCK */
#define CDE_DEV5_DIPIR		0x00000004
#define CDE_CDROM_DIPIR		0x00000008
#define CDE_DEV1_BLOCK		0x00000010
#define CDE_DEV2_BLOCK		0x00000020
#define CDE_CDROM_BLOCK		0x00000080
#define CDE_DEV5_BLOCK		0x00000100
#define CDE_DEV5_VISA_PENDING	0x00000400
#define	CDE_DEV6_BLOCK		0x00000800
#define	CDE_DEV7_BLOCK		0x00001000
#define CDE_DEV6_VISA_PENDING	0x00002000
#define CDE_DEV6_DIPIR		0x00004000
#define CDE_DEV7_DIPIR		0x00008000
#define CDE_DEV7_VISA_PENDING	0x00010000

/* Bits in CDE_BBLOCK_EN */
#define CDE_BLOCK_CLEAR		0x00000001

/* Bits in CDE_DEVx_CONF */
#define CDE_ATTR_SPACE		0x00000001
#define CDE_IO_SPACE		0x00000002
#define CDE_IOCONF		0x00000004
#define CDE_IOIS16		0x00000008
#define CDE_BIOBUS_RESET	0x00000020
#define CDE_VISA_CONF_DOWNLOAD	0x00000040
#define CDE_BIOBUS_SAFE		0x00000100

/* Bits in CDE_DEV_STATE */
#define CDE_DEV5_WRTPROT	0x00000001
#define CDE_DEV5_VISA_DIP	0x00000004
#define CDE_DEV6_WRTPROT	0x00000008
#define CDE_DEV6_VISA_DIP	0x00000020
#define CDE_DEV7_WRTPROT	0x00000040
#define CDE_DEV7_VISA_DIP	0x00000100

/* Bits in CDE_DEV_ERROR */
#define	CDE_DEV5_VISA_ERR	0x00000001
#define CDE_DEV7_BLOCK_ERR	0x00000002
#define CDE_DEV6_BLOCK_ERR	0x00000004
#define CDE_DEV5_WRTPROT_ERR	0x00000010
#define CDE_DEV5_BLOCK_ERR	0x00000020
#define CDE_DEV2_BLOCK_ERR	0x00000080
#define CDE_DEV1_BLOCK_ERR	0x00000100
#define	CDE_DEV6_VISA_ERR	0x00000200
#define CDE_DEV6_WRTPROT_ERR	0x00000800
#define	CDE_DEV7_VISA_ERR	0x00001000
#define CDE_DEV7_WRTPROT_ERR	0x00004000

/* Bits in CDE_VISA_DIS */
#define	CDE_DEV0_VISA_DIS	0x00000001
#define	CDE_DEV1_VISA_DIS	0x00000002
#define	CDE_DEV2_VISA_DIS	0x00000004
#define	CDE_DEV3_VISA_DIS	0x00000008
#define	CDE_DEV4_VISA_DIS	0x00000010
#define	CDE_DEV5_VISA_DIS	0x00000020
#define	CDE_DEV6_VISA_DIS	0x00000040
#define	CDE_DEV7_VISA_DIS	0x00000080
/* Bits below are used by dipir */
#define CDE_ROMAPP_BOOT		0x00000100
#define CDE_DIPIR_SPECIAL	0x00000200
#define	CDE_NOT_1ST_DIPIR	0x00000400
#define	CDE_WDATA_OK		0x00000800
#define	CDE_VISA_SCRATCH	0x0000FF00

/* Bits in CDE_DEVx_SETUP */
#define	CDE_WRITEN_HOLD		0x00000003
#define	CDE_WRITEN_SETUP	0x0000001C
#define	CDE_READ_HOLD		0x00000060
#define	CDE_READ_SETUP		0x00000380
#define	CDE_PAGEMODE		0x00000400
#define	CDE_DATAWIDTH		0x00001800
#define	 CDE_DATAWIDTH_8	0x00000000
#define	 CDE_DATAWIDTH_16	0x00000800
#define	CDE_READ_SETUP_IO	0x0000E000
#define	CDE_MODEA		0x00010000
#define	CDE_HIDEA		0x00020000

/* Bits in CDE_DEVx_CYCLE_TIME */
#define	CDE_CYCLE_TIME		0x0000FFFF
#define	CDE_PAGEMODE_CYCLE_TIME	0x00FF0000

/* Bits in CDE_DMAx_CNTL */
#define CDE_DMA_DIRECTION	0x00000400	/* PowerBus to BioBus if set */
#define CDE_DMA_RESET		0x00000200	/* Reset engine if set */
#define CDE_DMA_GLOBAL		0x00000100	/* snoopable trans if set */
#define CDE_DMA_CURR_VALID	0x00000080	/* current setup valid if set */
#define CDE_DMA_NEXT_VALID	0x00000040	/* next setup valid if set */
#define CDE_DMA_GO_FOREVER	0x00000020	/* copy next to current if set*/
#define CDE_PB_CHANNEL_MASK	0x0000001F	/* powerbus channel to use */

/* MicroSlot status register bits */
#define CDE_MICRO_STAT_CLK_MASK	0x00000003
#define  CDE_MICRO_STAT_1MHz	0x00000000
#define  CDE_MICRO_STAT_2MHz	0x00000001
#define  CDE_MICRO_STAT_4MHz	0x00000002
#define  CDE_MICRO_STAT_8MHz	0x00000003
#define CDE_MICRO_STAT_RESET	0x00000004
#define CDE_MICRO_STAT_CARDLESS	0x00000008
#define CDE_MICRO_STAT_ATTN	0x00000010
#define CDE_MICRO_STAT_INFULL	0x00000020
#define CDE_MICRO_STAT_OUTFULL	0x00000040

/* BIOBUS Address Constants */
#define	BIO_SLOT_SIZE		0x04000000	/* Size of a BIOBUS slot */
#define	BIO_DEV_0		0x20000000	/* BIOBUS slot 0 */
#define	BIO_DEV_1		0x24000000	/* BIOBUS slot 1 */
#define	BIO_DEV_2		0x28000000	/* BIOBUS slot 2 */
#define	BIO_DEV_3		0x2c000000	/* BIOBUS slot 3 */
#define	BIO_DEV_4		0x30000000	/* BIOBUS slot 4 */
#define	BIO_DEV_5		0x34000000	/* BIOBUS slot 5 */
#define	BIO_DEV_6		0x38000000	/* BIOBUS slot 6 */
#define	BIO_DEV_7		0x3c000000	/* BIOBUS slot 7 */

/*****************************************************************************
  Macros
*/
#define	IS_BIO_ADDR(addr)	((BIO_DEV_0 <= (uint32)addr) && (uint32)addr < (BIO_DEV_7 + BIO_SLOT_SIZE))

/*****************************************************************************
  Types
*/


/*****************************************************************************
  Functions - really macros
*/

#define CDE_READ(base,addr)		\
	(*(vuint32 *)((uint32)(base)|(addr)))
#define CDE_READ_BYTE(base,addr)	\
	(*(vuint8 *)((uint32)(base)|(addr)))
#define CDE_WRITE(base,addr,value)	\
	(*(vuint32 *)((uint32)(base)|(addr))) = (uint32)(value)
#define CDE_WRITE_BYTE(base,addr,value)	\
	(*(vuint8 *)((uint32)(base)|(addr))) = (uint8)(value)
#define CDE_SET(base,addr,bits)	\
	CDE_WRITE(base, addr, bits)
#define CDE_CLR(base,addr,bits)	\
	CDE_WRITE(base, (addr) | CDE_CLEAR_OFFSET, bits)


#endif /* __HARDWARE_CDE_H */
