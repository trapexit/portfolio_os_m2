#ifndef __HARDWARE_CDE_I
#define __HARDWARE_CDE_I


/******************************************************************************
**
**  @(#) cde.i 96/04/24 1.19
**
******************************************************************************/

#define CDE_CLEAR_OFFSET	0x400		// offset to "clear" regs

#define CDE_DEVICE_ID		0x000
#define CDE_VERSION		0x004
#define CDE_SDBG_CNTL		0x00C		// Serial debug control register
#define CDE_SDBG_RD		0x010		// Serial debug read data
#define CDE_SDBG_WRT		0x014		// Serial debug write data
#define CDE_INT_STS		0x018		// offset for status reg
#define CDE_INT_ENABLE		0x01C
#define CDE_RESET_CNTL		0x020
#define CDE_GPIO1		0x030		// GPIO1 control register (UART interrupt)
#define CDE_GPIO2		0x034		// GPIO1 control register
#define CDE_BBLOCK_EN		0x208		// Blocking enable register
#define CDE_DEV0_SETUP		0x240		// rom setup
#define CDE_DEV0_CYCLE_TIME	0x244		// rom cycle time
#define CDE_SYSTEM_CONF		0x280		// system configuration register
#define CDE_VISA_DIS		0x284		// Register where secure bits reside
#define CDE_INT_STC		0x418		// something else

#define CDE_SDBG_WRT_DONE	0x10000000	// bit mask
#define CDE_SDBG_RD_DONE	0x08000000	// bit mask
#define CDE_SOFT_RESET		0x00000001

#define CDE_BLOCK_CLEAR		0x00000001	// Blocking enable bit in blocking register

#define CDE_ARM_INT		0x00010000

/* Bits in CDE_GPIOx */
#define CDE_GPIO_OUT_VALUE	0x00000001
#define CDE_GPIO_DIRECTION	0x00000002
#define CDE_GPIO_OESEL		0x00000004
#define CDE_GPIO_INT_NEG	0x00000008
#define CDE_GPIO_INT_POS	0x00000010
#define CDE_GPIO_RD_VALUE	0x00000020


#endif /* __HARDWARE_CDE_I */
