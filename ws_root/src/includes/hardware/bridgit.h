#ifndef __HARDWARE_BRIDGIT_H
#define __HARDWARE_BRIDGIT_H


/******************************************************************************
**
**  @(#) bridgit.h 96/02/20 1.15
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* bridgit as a structure for good software programming */

typedef struct Bridgit
{
	uint32	br_DeviceId;
	uint32	br_SoftwareRev;
	uint32	br_Drvr_Strt;
	uint32	br_Reserved0;
	uint32	br_RstVec0;
	uint32	br_RstVec1;
	uint32	br_RstVec2;
	uint32	br_CntlReg;	/* set */
	uint32	br_ResCntl;	/* set */
	uint32	br_IntStatus;	/* set */
	uint32	br_IntEnable;	/* set */
	uint32	br_McDipir;	/* set */
	uint32	br_ClrDipir;	/* set */
	uint32	br_MiscReg;
	uint32	br_PollReg;
	uint32	br_DMAChanCntrl;/* set */
	/* Have to figure out the rest to get to the clear regs */
} Bridgit;


/*****************************************************************************
  Defines
*/

/* This is the value Bridgit stores in the type field of BR_DEVICE_ID */
#define BR_DEVICE_ID_TYPE	2

/* Offset to the corresponding clear registers */
/* Also OR in this value in the real system to access the clear registers */
#define BR_CLEAR_OFFSET		0x400

/* Misc registers */
#define BR_DEVICE_ID		0x000
#define BR_DEVICE_REV		0x004
#define BR_DRVR_STRT		0x008
#define BR_RST_VEC0		0x010
#define BR_RST_VEC1		0x014
#define BR_RST_VEC2		0x018
#define BR_CNTL_REG		0x01C
#define BR_RES_CNTL		0x020
#define BR_INT_STS		0x024
#define BR_INT_ENABLE		0x028
#define BR_MC_DIPIR		0x02C
#define BR_CL_DIPIR		0x030
#define BR_MISC_REG		0x034
#define BR_POLL_REG		0x038

/* Byte-wise accessible mailbox addresses */
#define BR_PPC_MAILBOX		(BR_MISC_REG+0)
#define BR_NB_MAILBOX		(BR_MISC_REG+1)

#ifndef BRIDGIT_SIMULATION
/* FIFO registers */
#define BR_CMD_FIFO		0x200
#define BR_STS_FIFO		0x204
#define BR_DATA_FIFO		0x208
#define BR_WRT_FIFO0		0x20C
#define BR_WRT_FIFO1		0x210

/* DMA registers */
#define BR_DMA_CNTL		0x214
#define BR_DMA_CADD		0x218
#define BR_DMA_CCNT		0x21C
#define BR_DMA_NADD		0x220
#define BR_DMA_NCNT		0x224
#endif


/* Control register bits - S/C */
#define BR_CMD_FLUSH		0x00008000
#define BR_STS_FLUSH		0x00004000
#define BR_DATA_FLUSH		0x00002000
#define BR_WRT_FLUSH		0x00001000

/* Reset control bits - S */
#define BR_SOFT_RST		0x00000001

/* Interrupt status/enable bits - R/S/C */
#define BR_INT_SENT		0x80000000
#define BR_DMA_DONE		0x40000000
#define BR_DIPIR		0x20000000
#define BR_CMD_RDY		0x10000000
#define BR_STS_RDY		0x08000000
#define BR_DATA_RDY		0x04000000
#define BR_WRT_RDY		0x02000000
#define BR_POLL_INT		0x01000000
#define BR_WR_FIFO_OF		0x00800000
#define BR_EXP_RESET		0x00400000
#define BR_EXP_POWER		0x00200000
#define BR_BUS_ERR_INT		0x00100000
#define BR_NUBUS_INT		0x00080000
#define BR_LA_INT		0x00040000

/* Media change/dipir bits - S/C */
#define BR_DIPIR_EVENT		0x00000001

/* Clear dipir bits - S */
#define BR_BLOCK_CLEAR		0x00000001

/* Poll register - S/C */
#define BR_POLL_MED_CHNG	0x80000000
#define BR_POLL_WRT_VALID	0x40000000
#define BR_POLL_DATA_VALID	0x20000000
#define BR_POLL_STS_VALID	0x10000000
#define BR_POLL_PWR_RESET	0x08000000
#define BR_POLL_WRT_INT_EN	0x04000000
#define BR_POLL_DATA_INT_EN	0x02000000
#define BR_POLL_STS_INT_EN	0x01000000

/* DMA control */
#define BR_DMA_RESET		0x00000200
#define BR_DMA_GLOBAL		0x00000100
#define BR_DMA_CURR_VALID	0x00000080
#define BR_DMA_NEXT_VALID	0x00000040
#define BR_DMA_GO_FOREVER	0x00000020
#define BR_PB_CHANNEL_MASK	0x0000001F

/* Miscellaneous bits */
#define BR_MISC_DEBUGGER	0x00008000

/* Maximums for FIFOs */
#define BR_CMD_FIFO_SIZE	8
#define BR_STS_FIFO_SIZE	16


/*****************************************************************************
  Macros
*/


/*****************************************************************************
  Types
*/


/*****************************************************************************
  Functions - really macros
*/

#define BRIDGIT_READ(base,addr)		\
	(*(vuint32 *)((uint32)(base)|(addr)))
#define BRIDGIT_READ_BYTE(base,addr)	\
	(*(volatile uint8 *)((uint32)(base)|(addr)))

#define BRIDGIT_WRITE(base,addr,value)		\
	(*(vuint32 *)((uint32)(base)|(addr))) = value
#define BRIDGIT_WRITE_BYTE(base,addr,value)	\
	(*(volatile uint8 *)((uint32)(base)|(addr))) = value

#ifndef BRIDGIT_SIMULATION

#define BRIDGIT_SET(base,addr,bits)	\
	BRIDGIT_WRITE(base, addr, bits)
#define BRIDGIT_CLR(base,addr,bits)	\
	BRIDGIT_WRITE(base, (addr) | BR_CLEAR_OFFSET, bits)

#define BRIDGIT_SET_CLR(base,a,b)		\
	{BRIDGIT_SET(base, a, b); BRIDGIT_CLR(base, a, b);}

#define BRIDGIT_CMD_FIFO_FLUSH(base)	\
	BRIDGIT_SET_CLR(base, BR_CNTL_REG, BR_CMD_FLUSH);
#define BRIDGIT_STS_FIFO_FLUSH(base)	\
	BRIDGIT_SET_CLR(base, BR_CNTL_REG, BR_STS_FLUSH);
#define BRIDGIT_RD_FIFO_FLUSH(base)	\
	BRIDGIT_SET_CLR(base, BR_CNTL_REG, BR_DATA_FLUSH);
#define BRIDGIT_WRT_FIFO_FLUSH(base)	\
	BRIDGIT_SET_CLR(base, BR_CNTL_REG, BR_WRT_FLUSH);

#define BRIDGIT_CMD_FIFO_READ(base)	\
	BRIDGIT_READ_BYTE(base, BR_CMD_FIFO)
#define BRIDGIT_STS_FIFO_WRITE(base,d)	\
	BRIDGIT_WRITE_BYTE(base, BR_STS_FIFO, d)
#define BRIDGIT_RD_FIFO_WRITE(base,d)	\
	BRIDGIT_WRITE_BYTE(base, BR_DATA_FIFO, d)

#endif /* BRIDGIT_SIMULATION */

/* kevinh - a lazy base address for bridgit */

#define BRIDGIT_BASE 0x03000000

#endif /* __HARDWARE_BRIDGIT_H */
