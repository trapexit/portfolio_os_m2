#ifndef __BOOTCODE_BOOTHW_I
#define __BOOTCODE_BOOTHW_I


/******************************************************************************
**
**  @(#) boothw.i 96/06/11 1.6
**
**  Hardware definitions which should only be used by bootcode.
**  Other parts of the system should use SysInfo to get this information.
**
******************************************************************************/


CDE_BASE 		.equ	0x04000000	// Base address of CDE
PCMCIA_BASE 		.equ	0x30000000	// Starting address of PCMCIA space

DSPX_INTERRUPT_SET	.equ	0x00064000	// DSP interrupt set register
DSPX_INTERRUPT_CLR	.equ	0x00064004	// DSP interrupt clear register

VID_THRESHOLD		.equ	0xD		// Upper bound of "safe" video region


#endif /* __BOOTCODE_BOOTHW_I */
