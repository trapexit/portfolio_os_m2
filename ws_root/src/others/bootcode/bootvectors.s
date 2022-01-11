/* @(#) bootvectors.s 96/12/02 1.165 */

/********************************************************************************
*
*	bootvectors.s
*
*	This file contains the ROM based exception table (i.e. exception
*	vectors) for the CPU.  In addition, because the exception table is
*	so sparse, various other bootcode routines are interspersed with the
*	individual segments of exception handling code.  This decreases the
*	overall size of bootcode by a significant amount.  Note that the space
*	between vectors is only 0x100 bytes, and any routine that is tucked in
*	the table must be less than this size minus the size of the exception
*	handling code.
*
*	In keeping with the M2 ABI, all assembly routines in this file are
*	designed so that they only corrupt registers in the r3-r12 range.
*
*	In situations where no stack memory is available but it is necessary
*	to nest subroutines two levels deep, a pseudo stack can be invoked
*	with the "stack" and "unstack" macros.  This simply uses r12 to save
*	and restore the link register.
*
********************************************************************************/

#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>
#include <hardware/bda.i>
#include <hardware/cde.i>
#include <bootcode/boothw.i>
#include "bootmacros.i"


/********************************************************************************
*
*	Exception table
*
*	At power up this table is aliased from its physical location at
*	0x20000000 to the CPU's exception vector space at 0xFFF00000 so that
*	the CPU will start executing from ROM at power up.  Later in the boot
*	process we build a RAM based exception table and set up the CPU to use
*	that table in most (but not all) cases.
*
*	The address jumped to as the result of an exception is derived by
*	concatenating IBA (the CPU's 16 bit Interrupt Base Address prefix)
*	with the 16 bit vector offset for that specific exception (all of
*	which are aligned on 0x100 byte boundaries).  IBA has different
*	values depending on the type of exception and the values of MSR_IP
*	(the CPU's exception prefix bit) and IBR (the CPU's 16 bit Interrupt
*	Base Register).
*
*	- For hard reset (offset = 0x0100), IBA is ALWAYS 0xFFF0, regardless
*	  of the values of MSR_IP or IBR.
*
*	- For soft reset (offset = 0x0100) and machine check (offset = 0x0200),
*	  IBA is 0xFFF0 if MSR_IP is set to 1, or 0x0000 if MSR_IP is set to 0.
*	  IBR is ignored.
*
*	- For ALL OTHER exceptions, IBA is 0xFFF0 if MSR_IP is set to 1, or the
*	  value in IBR if MSR_IP is set to 0.
*
*	For performance reasons (RAM access is faster than ROM), we generally
*	leave MSR_IP set to 0 during normal operation.  In situations where
*	a soft reset or machine check may occur, we temporarily set MSR_IP
*	to 1 in order to properly handle those exceptions.  If either of these
*	occurs unexpectedly when MSR_IP is 0 the CPU vectors off to the nether
*	regions of the address space (IBA = 0x0000) and the system hangs.
*
*	For security reasons, we cannot allow any unexpected exceptions to
*	occur during softreset/dipir.  Therefore we keep MSR_IP set for the
*	duration of dipir (on BOTH the master and slave CPUs) so that any
*	unexpected exceptions will be handled by the ROM vector table.  Only
*	the reset exceptions, the machine check exception, and the external
*	interrupt exception should ever be taken while MSR_IP is set, therefore
*	all the other ROM based exception handlers hang the processor that runs
*	them.
*
********************************************************************************/

	DECFN	ExceptionTable			// Entry point for bootcode

#ifdef	BUILD_DEBUGGER
	b	IdentifyException		// Figure out which exception the debugger really got
#else	/* ifdef BUILD_DEBUGGER */
	b	IllegalException		// Shouldn't get here since this exception is undefined
#endif	/* ifdef BUILD_DEBUGGER */

	DECVAR	BootBSSSize			// BSS section size
	DECVAR	BootBSSAddress			// BSS section address
	DECVAR	SystemROMSize			// Size of system ROM


#ifdef	BUILD_DEBUGGER
/*******************************************************************************/
*
*	IdentifyException
*
*	This routine patches around a Bridgit/debugger limitation which
*	prevents the ROM exception table from being properly accessed under
*	the debugger.  The problem is that under the debugger, fetches to
*	the 0xFFF00000 space are handle by Bridgit rather than being fetched
*	from ROM space.  But Bridgit only has room for 3 instructions, which
*	are aliased into all the exception vectors, so Bridgit can't actually
*	treat each exception differently and jump off to the appropriate
*	handler.
*
*	We patch around this by using the following 3 instructions in Bridgit:
*
*	  lis	r3,(ExceptionTable >> 16)
*	  mtctr	r3
*	  bctrl
*
*	This means that for any exception under the debugger when MSR_IP is
*	set, we jump to the beginning of the exception table, and the link
*	register contains an address that is specific for each exception.
*	We then decode the link register and branch to the appropriate
*	exception vector in the ROM image.
*
*	Input:	lr = Address specific to type of exception
*	Output:	None
*	Mucks:	r3, lr
*	Calls:	ResetHandler
*
********************************************************************************/

IdentifyException:
	mflr	r3				// Get address from link register
	andi.	r3,r3,0x1F00			// Mask off everything but exception vector offset
	oris	r3,r3,(SYSROMIMAGE >> 16)	// Or in the base address of the ROM vector table
	mtlr	r3				// Move resulting address into link register
	blr					// Branch to the appropriate ROM exception vector

#endif	// ifdef BUILD_DEBUGGER


/*******************************************************************************/
*
*	ResetVector
*
*	This is where the processor starts executing when it comes out of
*	reset.  Initially it is fetching from 0xFFF00000 space, so the first
*	thing we do is jump off to the real ROM address space at 0x20000000.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	ResetHandler
*
********************************************************************************/

	.skip	0x0100 - (. - ExceptionTable)	// Set offset to 0x0100

ResetVector:					// Vector for hard and soft resets
	lea	r3,ResetHandler			// Get absolute address of the reset handler
	mtspr	lr,r3				// Place it in the link register
	blr					// Branch to reset handler in real ROM space


/********************************************************************************
*
*	ResetHandler
*
*	This routine identifies the type of reset that has occured and hands
*	off control to the appropriate handler for that type.
*
*	The reset mechanism currently works in this fashion:
*
*	ResetHandler
*	   Identify slave CPUs and make them doze
*	   Check for test tool (only returns if not present)
*	   If here due to soft reset goto SoftReset
*	   Else fall through to HardReset
*	HardReset
*	   Do power-up system initialization
*	   Perform soft reset by writing to CDE_RESET_CNTL
*	SoftReset
*	   Cleanup to allow dipir to run safely
*	   Call LaunchDipir
*
*	The check for soft reset does not use the CPU's HID_NHR bit since
*	that may be easily spoofed.  Instead, we check CDE's BBLOCK_EN
*	register, which is guarenteed to be 1 on hard reset and 0 on soft
*	reset.  Note that this bit does not settle until 256 cycles after
*	soft reset is initiated, so there must be a delay of at least
*	that long before we check the bit.
*
*	Note that the delay described above is also used to allow time for
*	any slave CPUs to enter doze mode before the master CPU is configured
*	and has its caches enabled.  This is necessary since no other CPUs
*	can execute out of ROM (due to CDE limitations) while any one CPU is
*	is executing from ROM with its caches on.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	InitializeCPU
*		IdentifyMasterCPU
*		CheckForTestTool
*		SoftResetHandler
*		HardResetHandler (actually, it just falls through to this)
*
********************************************************************************/

ResetHandler:

	bl	InitializeCPU			// Put CPU in a safe, known state
	bl	IdentifyMasterCPU		// Only let the master CPU proceed beyond this point

	li32	r3,0x100			// Get number of delay loops
	mtctr	r3				// Load number of loops into counter
%loop:						// Wait loop to allow BBLOCK_EN to settle
	bdnz	%loop				// If not done then keep looping

	bl	CheckForTestTool		// Check if test tool is connected
	li32	r3,CDE_BASE			// Get CDE base address
	lwz	r4,CDE_BBLOCK_EN(r3)		// Get current value of blocking enable register
	andi.	r4,r4,CDE_BLOCK_CLEAR		// Was this a soft reset?
	beq	SoftResetHandler		// If so branch to soft reset handler


/********************************************************************************
*
*	HardResetHandler
*
*	This routine takes whatever low level steps are necessary to handle a
*	hard reset, then hands control off to the C based hard reset routine
*	which does the rest.
*
*	Input:	None
*	Output:	None
*	Mucks:	None
*	Calls:	ConfigureBDA
*		ConfigureCDE
*		ConfigureCPU
*		SetupForC
*		Bootstrap
*
********************************************************************************/

HardResetHandler:

	bl	ConfigureBDA			// Do low level BDA setup
	bl	ConfigureCDE			// Do low level CDE setup
	bl	ConfigureCPU			// Do low level CPU setup
	bl	SetupForC			// Set up so we can call C routines

#ifdef BUILD_PFORTHMON
	bl	EnterPForth
#endif

	b	Bootstrap			// Jump to C based bootstrap code to complete boot


/********************************************************************************
*
*	SoftResetHandler
*
*	This routine takes whatever low level steps are necessary to handle a
*	soft reset, then launches dipir to take care of security related issues.
*	If we end up launching an OS, then dipir never returns.
*
*	Input:	None
*	Output:	r3 = Dipir return value
*	Mucks:	SRR0, SRR1
*	Calls:	ConfigureCPU
*		LaunchDipir
*
********************************************************************************/

SoftResetHandler:

	bl	ConfigureCPU			// Do low level CPU setup
#ifdef BUILD_PFORTHMON
	b	ExitSoftReset			// If PForth monitor, just return
#endif
	li32	sp,BOOTDATASTART+BOOTDATASIZE	// Get end of bootcode data/bss area...
	stwu	sp,-16(sp)			// ... and set up a new, safe stack there
	bl	LaunchDipir			// Call routine to launch dipir

ExitSoftReset:
	lea	r5,SoftResetRFI			// Get address to return to from soft reset
	mtsrr0	r5				// Put in SRR0
	mfmsr	r5				// Get current value of MSR
	mtsrr1	r5				// Put in SRR1
	rfi					// Return to caller of soft reset
	isync					// Make sure the above is done


/*******************************************************************************/
*
*	MachineCheckHandler
*
*	This routine takes whatever low level steps are necessary to handle a
*	machine check.  There are currently two cases of interest:
*
*	1) We got here because IdentifyMasterCPU() tried to read a nonexistant
*	   WhoAmI PAL.  In this case we simply return from the exception with a
*	   value indicating that no WhoAmI PAL is present.
*
*	2) We got here due to an action taken by the OS or dipir.  In this case
*	   we hand control off to a RAM based machine check handler.
*
*	Input:	SRR0 = Link register value to return to after exception
*		SRR1 = Pre-exception MSR value
*	Output:	r5 = Value indicating no WhoAmI PAL (only in WhoAmI case)
*	Mucks:	r3-r4, sprg1-sprg3, SRR0 (only in WhoAmI case)
*	Calls:	ViaIBR
*
********************************************************************************/

	.skip	0x0200 - (. - ExceptionTable)	// Set offset to 0x0200

MachineCheckHandler:
	mtsprg1	r3				// Store current value of r3 to SPRG1
	mtsprg2	r4				// Store current value of r4 to SPRG2
	mfcr	r4				// Get current value of condition register
	mtsprg3	r4				// Store to SPRG3

	mfsrr0	r3				// Get address from interrupted instruction stream
	lea	r4,WhoAmIRead			// Get address from WhoAmI read in IdentifyMasterCPU
	cmp	r3,r4				// Were we trying to identify the CPU?
	beq	ReturnCPUID			// If yes then go return the appropriate CPU ID value

	mfsprg3	r4				// Get original value of condition register
	mtcr	r4				// Restore value to condition register
	mfsprg2	r4				// Restore original value of r4
	mfsprg1	r3				// Restore original value of r3
	ViaIBR	0x0200				// Go to machine check handler

ReturnCPUID:
	li32	r5,WHOAMI_SINGLECPU		// Get return value to indicate single CPU system
	lea	r3,WhoAmIRFI			// Get address to return to
	mtsrr0	r3				// Put the address in SRR0 for use by rfi instruction
	rfi					// Return from WhoAmI read exception
	isync					// Make sure the above is done


/*******************************************************************************/

	.skip	0x0300 - (. - ExceptionTable)	// Set offset to 0x0300
	b	IllegalException		// Shouldn't get data access exceptions with MSR_IP set


/********************************************************************************
*
*	ConfigureBDA
*
*	This routine handles the low level configuration of BDA.  This consists
*	of:
*
*	- Clearing all pending PowerBus related interrupts, since they don't
*	  default to cleared after hard reset.
*
*	- Setting the prefetch enable bit
*
*	- Setting the 1BWR and 4BWR bits in the PCTL register for fast powerbus
*	  transactions.
*
*	- Setting up MCONFIG appropriately.  Note that the SS0 and SS1 fields
*	  are preserved since they hold the default memory size information for
*	  the system; all other fields are hardcoded with the following values
*	  received from Greg Williams:
*
*		LDIA	= 3 (4 if -12 SDRAMs used instead of -10)
*		LDOA	= 1 (2 if -12 SDRAMs used instead of -10)
*		RC	= 8
*		RCD	= 2
*		CL	= 2
*
*	- Setting the TOCYC field in the PCTL register to 0x300 for the debugger
*	  case.  We do this to avoid problems with NuBus timeouts crashing the
*	  debugger when PowerBus times out (e.g. due to a read of WhoAmI on a
*	  Rev G dev card).  This value was chosen based on the default timeout
*	  value of 0x1FFFF being about 4mS, and the nominal timeout for NuBus
*	  being 25.6uS.  In the production case we use the default (i.e. maximum)
*	  value since we don't have to worry about the debugger and don't want
*	  prohibit the use of slow peripherals in the future.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r5
*	Calls:	None
*
********************************************************************************/

ConfigureBDA:

	li32	r3,BDAPCTL_PBCONTROL		// Get PBCONTROL register address
	lwz	r4,0(r3)			// Get current value of PCTL register
	oris	r4,r4,(BDAPCTL_PREF_MASK >> 16)	// Set prefetch enable bit
	li	r5,(BDAPCTL_1BWR_MASK|BDAPCTL_4BWR_MASK)	// Get masks for BWR fields
	andc	r4,r4,r5			// Clear those fields
	ori	r4,r4,0x0110			// Set 1BWR and 4BWR fields both to 1
	li32	r5,BDAPCTL_TOCYC_MASK		// Get mask for timeout field
#ifdef	BUILD_DEBUGGER
	andc	r4,r4,r5			// Clear the timeout field
	oris	r4,r4,((0x300 << BDAPCTL_TOCYC_SHIFT) >> 16)	// Set timeout to safe value for debugger
#else	/* ifdef BUILD_DEBUGGER */
	or	r4,r4,r5			// Set timeout field to its maximum
#endif	/* ifdef BUILD_DEBUGGER */
	stw	r4,0(r3)			// Store new value to PCTL register

	li32	r3,BDAPCTL_ERRSTAT		// Get ERRSTAT register address
	li32	r4,0xFFFFFFFF			// Get value to clear all interrupts
	stw	r4,0(r3)			// Store value to PowerBus error status register

	li32	r3,BDAMCTL_MCONFIG		// Get address of MCONFIG register
	lwz	r4,0(r3)			// Get current value of MCONFIG register
	andi.	r4,r4,(BDAMCFG_SS1_MASK | BDAMCFG_SS0_MASK)	// Clear all fields but SS0 and SS1
	oris	r4,r4,(3 << (BDAMCFG_LDIA_SHIFT - 16))		// Or in LDIA field
	oris	r4,r4,(1 << (BDAMCFG_LDOA_SHIFT - 16))		// Or in LDOA field
	oris	r4,r4,(8 << (BDAMCFG_RC_SHIFT - 16))		// Or in RC field
	oris	r4,r4,(2 << (BDAMCFG_RCD_SHIFT - 16))		// Or in RCD field
	ori	r4,r4,(2 << BDAMCFG_CL_SHIFT)			// Or in CL field
	stw	r4,0(r3)			// Store new value to MCONFIG register

	blr					// Return from routine


/********************************************************************************
*
*	ConfigureCDE
*
*	This routine handles the low level configuration of CDE's communication
*	parameters for the system ROM.  This includes setting up ROM access
*	timing registers so that ROM accesses aren't dog slow, and setting up
*	CDE GPIO1 as a positive-edge triggered IRQ for the system's UART.
*
*	First we AND the default value of DEVSET0 with 0x1800, which preserves
*	the default device width and zeros all the other fields.  Edouard Landau
*	said this should work fine for all typical ROMs.
*
*	Second we set the cycle time field of DEVTIME0 to work with 120nS or
*	faster ROM parts.  The specific value used depends on CDE version, and
*	comes from table 4-15 of the CDE2 chip spec.
*
*	Note that MEI plans to use 120nS ROMs, and has no plans to use burst
*	mode ROMs
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	None
*
********************************************************************************/

ConfigureCDE:

	li32	r3,CDE_BASE			// Get CDE base address
	lwz	r4,CDE_DEV0_SETUP(r3)		// Get default DEVSET0 value
	andi.	r4,r4,0x1800			// Preserve DW and zero other fields
	stw	r4,CDE_DEV0_SETUP(r3)		// Store new value to DEVSET0

	li	r4,5				// Get ROM cycle time for 120nS parts
#ifndef BUILD_MEIBRINGUP
	stw	r4,CDE_DEV0_CYCLE_TIME(r3)	// Store to device 0 cycle time register
#endif

	li	r4,CDE_GPIO_INT_POS		// Get value to set up GPIO as an IRQ
	stw	r4,CDE_GPIO1(r3)		// Store to to CDE GPIO1 control register

	blr					// Return from routine


/*******************************************************************************/

	.skip	0x0400 - (. - ExceptionTable)	// Set offset to 0x0400
	b	IllegalException		// Shouldn't get instruction access exceptions with MSR_IP set


/********************************************************************************
*
*	SetupForC
*
*	This routines first clears all of memory to a known, non-zero pattern,
*	then sets up the stack and BSS for bootcode.  Note that any reserved
*	area at the top of memory (for things like debugger usage or the
*	pseudo-ROM) is NOT cleared.
*
*	Note that it is assumed that the data section for bootcode is empty,
*	and that the BSS starts at the beginning of the bootcode data area.
*	If this is not the case then the system hangs in order to indicate
*	this condition.
*
*	Input:	None
*	Output:	sp = stack pointer
*	Mucks:	r3-r5
*	Calls:	MemorySize
*		MemorySet
*
********************************************************************************/

SetupForC:
	stack

#ifdef	BUILD_DEBUGGER
	li32	r3,0xCC				// Get a suitable pattern
# ifdef	BUILD_FLASH
	li32	r5,DEBUGGERSTART-DRAMSTART	// Get length of RAM area
# else	/* ifdef BUILD_FLASH */
	li32	r5,SYSROMIMAGE-DRAMSTART	// Get length of RAM area
# endif	/* ifdef BUILD_FLASH */
#else	/* ifdef BUILD_DEBUGGER */
	bl	MemorySize			// Get length of RAM area
	mr	r5,r3				// Move length to appropriate register for MemorySet
	li32	r3,0x33				// Get a suitable pattern
#endif	/* ifdef BUILD_DEBUGGER */
	li32	r4,DRAMSTART			// Get starting address of RAM area
	bl	MemorySet			// Initialize entire RAM area

	li32	r3,0x0				// Get value to initialize BSS
	lea	r4,BootBSSAddress		// Get pointer to bootcode BSS section address
	lwz	r4,0(r4)			// Get destination address for bootcode BSS section
	li32	r5,BOOTDATASTART		// Get "no-data" destination for BSS
	cmp	r4,r5				// Is the data section really empty?
	bne	DataSectionNotEmpty		// If not then treat as an error
	lea	r5,BootBSSSize			// Get pointer to bootcode BSS section size
	lwz	r5,0(r5)			// Get length of bootcode BSS section
	bl	MemorySet			// Set bootcode BSS section to desired value

	li32	sp,BOOTDATASTART+BOOTDATASIZE	// Get end of bootcode data/bss area...
	stwu	sp,-16(sp)			// ... and set up a stack so we can call C routines

	unstack
	blr

DataSectionNotEmpty:
	b	DataSectionNotEmpty		// Loop forever


/*******************************************************************************/
*
*	ExternalInterruptHandler
*
*	This routine takes whatever low level steps are necessary to handle an
*	external interrupt.  There are currently two cases of interest:
*
*	1) This is the master CPU, in which case this might be a security
*	   break so we call IllegalException().
*
*	2) This is the slave CPU, in which case the master is trying to wake
*	   up the slave so we hand control off to a RAM based external interrupt
*	   handler.  In this case we also clear MSR_IP to map future exceptions
*	   to the RAM vector table.
*
*	Input:	SRR0 = Link register value to return to after exception
*		SRR1 = Pre-exception MSR value
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	SlaveCPUTest
*		UseRAMVectors
*		ViaIBR
*
********************************************************************************/

	.skip	0x0500 - (. - ExceptionTable)	// Set offset to 0x0500

ExternalInterruptHandler:
	bl	SlaveCPUTest			// Is this a slave CPU?
	bne	IllegalException		// If not then treat as an illegal exception

	bl	UseRAMVectors			// Switch over to use RAM exception vector table
	ViaIBR	0x0500				// Go to external interrupt handler


/********************************************************************************
*
*	ConfigureCPU
*
*	This routine performs all major CPU configuration required as the
*	result of a reset.
*
*	One of the main goals of this routine is to ensure that there is
*	no way for renegade code to create an exception condition that will
*	cause either the master or slave CPUs to return to that renegade
*	code before dipir has performed all necessary security checks.
*	This is achieved in part by disabling as many exception sources
*	as possible during dipir, and by leaving MSR_IP set such that the
*	exception table is in ROM rather than in RAM during dipir.
*
*	This routine assumes that InitializeCPU() has already:
*
*	- Enabled machine checks
*	- Ensured that MSR_IP is set
*	- Ensured that all other MSR bits are cleared
*	- Ensured that all HID0 bits are cleared
*
*	Only the master CPU returns from this routine.  All slave CPU's
*	executing this routine are put into doze mode, to be woken up later
*	by the master.
*
*	Note that since waking up a slave requires that slave to recognize
*	an interrupt, some interrupts for the slave are enabled by this
*	routine.  For the slave we try to ensure that all possible renegade
*	exceptions are not allowed.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	EnableFPU
*		SetupBATs
*		EnableMMU
*		SlaveCPUTest
*		EnableICache
*		EnableDCache
*
********************************************************************************/

ConfigureCPU:
	stack

	mfspr	r3,HID0				// Get current value of HID0
	oris	r3,r3,(HID_DPM >> 16)		// Set Dynamic Power Management bit
	oris	r3,r3,(HID_DOZE >> 16)		// Set Doze bit so 602 will doze if MSR_POW is set
	mtspr	HID0,r3				// Move new value to HID0
	isync					// Make sure the above is done

	li32	r3,0				// Get value to disable breakpoint exception register
	mtspr	IABR,r3				// Move value to IABR
	isync					// Make sure the above is done

	li32	r3,0				// Get value to disable watchdog timer register
	mtspr	TCR,r3				// Move value to TCR
	isync					// Make sure the above is done

	bl	EnableFPU			// Set up and enable the floating point unit
	bl	SetupBATs			// Set up block address translation registers
	bl	EnableMMU			// Enable MMU services

	li32	r3,VECTORSTART			// Get RAM target address for exception table
	mtspr	IBR,r3				// Move address to IBR

	bl	SlaveCPUTest			// Is this a slave CPU?
	beq	SlaveSpecificSetup		// If yes then go do slave specific setup

#ifndef BUILD_CACHESOFF
	bl	EnableICache			// Set up and enable the instruction cache
	bl	EnableDCache			// Set up and enable the data cache
#endif

	unstack
	blr					// This is the master CPU so return

SlaveSpecificSetup:
	mfmsr	r3				// Get current value of MSR
	ori	r3,r3,MSR_EE			// Set the external interrupt enable bit
	mtmsr	r3				// Move new value to MSR
	isync					// Make sure the above is done
	oris	r4,r3,(MSR_POW >> 16)		// Set power management bit (can't change other bits at same time)
	sync					// Setup for a clean transition to doze mode
	mtmsr	r4				// Move new value to MSR to make slave doze
	isync					// Make sure the above is done

WaitForDoze:
	b	WaitForDoze			// Loop until CPU dozes


/*******************************************************************************/

	.skip	0x0600 - (. - ExceptionTable)	// Set offset to 0x0600
	b	IllegalException		// Shouldn't get alignment exceptions with MSR_IP set


/********************************************************************************
*
*	EnableFPU
*
*	This routine configures and enables the CPU's floating point unit.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

EnableFPU:

	mfmsr	r3				// Get current value of MSR
	ori	r3,r3,MSR_FP			// Enable the floating point unit
	mtmsr	r3				// Move new value back to MSR

	mtfsb0	24				// Disable FPU invalid operation exception
	mtfsb0	25				// Disable FPU overflow exception
	mtfsb0	26				// Disable FPU underflow exception
	mtfsb0	27				// Disable FPU zero divide exception
	mtfsb0	28				// Disable FPU inexact exception
	mtfsb1	29				// Enable non-IEEE FP mode (602 doesn't like IEEE?)
	mtfsb0	30				// Don't round towards infinity...
	mtfsb0	31				// ... round to nearest instead

	li	r3,0				// Get value to clear FPU integer tags...
	mtspr	LT,r3				// ... and move to appropriate register
	li	r3,-1				// Get value to set FPU single precision tags...
	mtspr	fSP,r3				// ... and move to appropriate register

	blr					// Return from routine


/********************************************************************************
*
*	EnableMMU
*
*	This routine enables the CPU's memory management unit.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

EnableMMU:
	mfspr	r3,HID0				// Get current value of HID0
	ori	r3,r3,HID_XMODE			// Set protection-only mode bit
	mtspr	HID0,r3				// Move new value to HID0
	isync					// Make sure the above is done

	mfmsr	r3				// Get the current value of MSR
	ori	r3,r3,MSR_IR+MSR_DR		// Enable instruction and data MMUs
	mtmsr	r3				// Write new value back to MSR
	isync					// Make sure the above is done

	blr					// Return from routine


/********************************************************************************
*
*	DisableMMU
*
*	This routine disables the CPU's memory management unit.
*
*	Input:	None
*	Output:	None
*	Mucks:	r4-r5
*	Calls:	None
*
********************************************************************************/

DisableMMU:
	mfmsr	r5				// Get the current value of MSR
	li	r4,MSR_IR+MSR_DR		// Get enable bits for instruction and data MMUs
	andc	r5,r5,r4			// Clear those bits in MSR value
	mtmsr	r5				// Move new value back to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	RestoreInterrupts
*
*	This routine sets the CPU's interrupt enable bit to match that of
*	an input MSR-formatted value.
*
*	Calling RestoreInterrupts with the value returned by a previous
*	call to DisableInterrupts will restore the interrupt enable bit
*	to its original state.
*
*	Input:	r3 = MSR-formatted value.
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	None
*
********************************************************************************/

	DECFN	RestoreInterrupts

	mfmsr	r4				// Get current value of MSR
	rlwimi	r4,r3,0,16,16			// Set interrupt enable bit to input value
	mtmsr	r4				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	DisableInterrupts
*
*	This routine disables CPU interrupts and returns the original value
*	of MSR.  That value can then be fed to RestoreInterrupts in order to
*	restore the interrupt enable bit to its original state.
*
*	Input:	None
*	Output:	r3 = original value of MSR
*	Mucks:	r4
*	Calls:	None
*
********************************************************************************/

	DECFN	DisableInterrupts

	mfmsr	r3				// Get current value of MSR
	li	r4,MSR_EE			// Get interrupt enable bit
	andc	r4,r3,r4			// Clear the bit
	mtmsr	r4				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	UseROMVectors
*
*	This routine sets MSR_IP so that exceptions will cause the processor
*	to jump into the ROM exception vector table.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

UseROMVectors:
	mfmsr	r3				// Get current value of MSR
	ori	r3,r3,MSR_IP			// Set IP bit to map exception handling to ROM
	mtmsr	r3				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	UseRAMVectors
*
*	This routine clears MSR_IP so that exceptions will cause the processor
*	to jump into the RAM exception vector table.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	None
*
********************************************************************************/

UseRAMVectors:
	mfmsr	r3				// Get current value of MSR
	li32	r4,MSR_IP			// Get IP bit mask
	andc	r3,r3,r4			// Clear MSR_IP to map exceptions to RAM
	mtmsr	r3				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	SlaveCPUTest
*
*	This routine compares the contents of SR14 to the slave CPU ID value
*	and returns the result in the condition register.
*
*	Input:	None
*	Output:	CR = result of comparison (eq indicates slave CPU)
*	Mucks:	r3-r4
*	Calls:	None
*
********************************************************************************/

SlaveCPUTest:
	mfsr	r3,SR14				// Get CPU ID
	li32	r4,WHOAMI_SLAVECPU		// Get slave CPU ID value
	cmp	r3,r4				// Is this a slave CPU?
	blr					// Return from routine


/********************************************************************************
*
*	IllegalException
*
*	This routine does whatever is necessary to deal with the occurance of
*	an illegal exception.  Currently this just means hanging the processor
*	in an infinite loop.
*
*	Input:	None
*	Output:	None
*	Mucks:	None
*	Calls:	None
*
********************************************************************************/

IllegalException:
	b	IllegalException		// Loop forever


/*******************************************************************************/

	.skip	0x0700 - (. - ExceptionTable)	// Set offset to 0x0700
	b	IllegalException		// Shouldn't get program exceptions with MSR_IP set


/********************************************************************************
*
*	EnableICache
*
*	This routine sets up and enables the CPU's instruction cache.  The
*	cache is invalidated first in order to avoid problems with stale
*	or invalid cache entries.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3, r5
*	Calls:	InvalidateICache
*
********************************************************************************/

	DECFN	EnableICache

	mflr	r5				// Save link register
	bl	InvalidateICache		// Invalidate the entire instruction cache
	mtlr	r5				// Restore the link register

	mfspr	r3,HID0				// Get current value of HID0
	ori	r3,r3,HID_ICE			// Set the instruction cache enable bit
	mtspr	HID0,r3				// Move the new value back to HID0
	isync					// Wait for previous instructions to complete

	blr					// Return from routine


/********************************************************************************
*
*	EnableDCache
*
*	This routine sets up and enables the CPU's data cache.  The cache
*	is invalidated first in order to avoid problems with stale or invalid
*	cache entries.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3, r5
*	Calls:	InvalidateDCache
*
********************************************************************************/

EnableDCache:
	mflr	r5				// Save link register
	bl	InvalidateDCache		// Invalidate the entire data cache
	mtlr	r5				// Restore the link register

	mfspr	r3,HID0				// Get current value of HID0
	ori	r3,r3,HID_DCE			// Set the data cache enable bit
	mtspr	HID0,r3				// Move the new value back to HID0
	isync					// Make sure the above is done

	blr					// Return from routine


/********************************************************************************
*
*	DisableICache
*
*	This routine disables and invalidates the CPU's instruction cache.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r5
*	Calls:	InvalidateICache
*
********************************************************************************/

	DECFN	DisableICache

	mfspr	r3,HID0				// Get current value of HID0
	li32	r4,HID_ICE			// Get instruction cache enable bit
	andc	r3,r3,r4			// Clear the enable bit
	mtspr	HID0,r3				// Move new value to HID0
	isync					// Make sure the above is done

	mflr	r5				// Save link register
	bl	InvalidateICache		// Invalidate the entire instruction cache
	mtlr	r5				// Restore the link register

	blr					// Return from routine


/********************************************************************************
*
*	DisableDCache
*
*	This routine disables and invalidates the CPU's data cache.  If there
*	is currently modified data within the data cache, you must explicitly
*	flush it before calling this routine.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r5
*	Calls:	InvalidateDCache
*
********************************************************************************/

DisableDCache:
	mfspr	r3,HID0				// Get current value of HID0
	li32	r4,HID_DCE			// Get data cache enable bit
	andc	r3,r3,r4			// Clear the enable bit
	mtspr	HID0,r3				// Move new value to HID0
	isync					// Make sure the above is done

	mflr	r5				// Save link register
	bl	InvalidateDCache		// Invalidate the entire data cache
	mtlr	r5				// Restore link register

	blr					// Return from routine


/********************************************************************************
*
*	EnableMachineChecks
*
*	This sets MSR_ME in order to enable machine check exceptions.  We also
*	used to set HID0_EMCP, but I'm not sure why since that only affects the
*	external Machine Check Pin, which is not connected in our systems.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

EnableMachineChecks:
	mfmsr	r3				// Get current value of MSR
	ori	r3,r3,MSR_ME			// Set the machine check enable bit
	mtmsr	r3				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	DisableMachineChecks
*
*	This clears MSR_ME in order to disable machine check exceptions.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4
*	Calls:	None
*
********************************************************************************/

DisableMachineChecks:
	mfmsr	r3				// Get current value of MSR
	li32	r4,MSR_ME			// Get machine check exception enable bit
	andc	r3,r3,r4			// Clear the bit
	mtmsr	r3				// Move new value to MSR
	isync					// Make sure the above is done
	blr					// Return from routine


/*******************************************************************************/

	.skip	0x0800 - (. - ExceptionTable)	// Set offset to 0x0800
	b	IllegalException		// Shouldn't get FP unavailable exceptions with MSR_IP set


/********************************************************************************
*
*	void FlushDCache(uint32 address, uint32 length)
*	void FlushDCacheAll(void)
*
*	FlushDCache flushes all data cache blocks intersected by the given
*	range, and loads ALL blocks in the range back into the cache.  The
*	reload is a side effect of support for FlushDCacheAll.
*
*	FlushDCacheAll flushes ALL data from the cache by performing a
*	FlushDCache on a memory range of the same size as the data cache.
*	This works because it causes any cached blocks within that range
*	to be written out, then forces all other blocks to be written out
*	by completely filling the cache with blocks from within the already
*	flushed range.
*
*	Input:	r3 = start of range, r4 = length of range
*	Output:	None
*	Mucks:	r3-r5
*	Calls:	None
*
********************************************************************************/

	DECFN	FlushDCacheAll			// Flush the entire data cache

	li32	r3,DRAMSTART			// Set address to start of system RAM
	li	r4,4096				// Set length to size of data cache

	DECFN	FlushDCache			// Only flush block for a given range

	add	r5,r3,r4			// Add length to starting address ...
	addi	r5,r5,-1			// ... and subtract one to get last address
	li	r4,0x1f				// Get mask to round to beginning of block
	andc	r3,r3,r4			// Get address of first block
	andc	r5,r5,r4			// Get address of last block
	sub	r4,r5,r3			// Subtract first from last...
	srawi	r4,r4,5				// ... divided by block size...
	addi	r4,r4,1				// ... and add one to get number of blocks
	mtctr	r4				// Load number of blocks into counter
%loop:
	dcbf	0,r3				// Flush a block out to memory
	dcbt    0,r3				// Then load back into cache
	addi	r3,r3,32			// Set pointer to next block
	bdnz	%loop				// If not done then flush another block

	sync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	void InvalidateICache(void)
*
*	This routine invalidates all entries in the CPU's instruction cache.
*	This is done by pulsing the instruction cache's flash invalidate bit.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

	DECFN	InvalidateICache

	mfspr	r3,HID0				// Get current value of HID0
	ori	r3,r3,HID_ICFI			// Set the instruction cache flash invalidate bit
	mtspr	HID0,r3				// Store back to HID0 to start invalidate pulse
	isync					// Make sure the above is done
	xori	r3,r3,HID_ICFI			// Clear the flash invalidate bit
	mtspr	HID0,r3				// Store back to HID0 to end pulse
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	InvalidateDCache
*
*	This routine invalidates all entries in the CPU's data cache.  This
*	is done by pulsing the data cache's flash invalidate bit.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3
*	Calls:	None
*
********************************************************************************/

InvalidateDCache:

	mfspr	r3,HID0				// Get current value of HID0
	ori	r3,r3,HID_DCI			// Set the data cache flash invalidate bit
	mtspr	HID0,r3				// Store back to HID0 to start invalidate pulse
	isync					// Make sure the above is done
	xori	r3,r3,HID_DCI			// Clear the flash invalidate bit
	mtspr	HID0,r3				// Store back to HID0 to end pulse
	isync					// Make sure the above is done
	blr					// Return from routine


/********************************************************************************
*
*	Exception Handler
*
*	This code fragment is a generic exception handler which hands control
*	to the kernel as quickly as possible.  Bootcode copies this routine
*	into all but the first two entries of the RAM exception table.  The
*	following explaination applies only to non-reset exceptions.
*
*	ExceptionStart is the actual exception handler.  When copied to RAM,
*	the fragment is aligned such that ExceptionStart is at the actual
*	exception vector (i.e the fragment is placed at the vector address
*	MINUS the difference between ExceptionPreamble and ExceptionStart).
*
*	If an exception occurs while MSR_IP is 0, the CPU vectors to the
*	appropriate handler in RAM using IBR as a base address.  In this
*	case we enter the exception handler at ExceptionStart and proceed.
*
*	If an exception occurs while MSR_IP is 1, the CPU first vectors to
*	the appropriate handler in the ROM exception table, which may jump
*	to a corresponding handler in RAM based on the current value of IBR.
*	Getting the RAM address and jumping to it requires the use of scratch
*	registers, whose values are temporarily stored in SPRG1 and SPRG2.
*	These registers must be restored before entering the RAM handler, so
*	the ROM handler jumps to ExceptionPreamble, which does the restore
*	then falls through to ExceptionStart.
*
*	Initially this handler loops forever.  The kernel is responsible for
*	activating the handler later by finding the branch instruction at the
*	end and replacing it with a branch to the honest-to-goodness exception
*	handler in the kernel.
*
*	Input:	None
*	Output:	SPRG1 = entry value of r3
*		SPRG2 = entry value of lr
*	Mucks:	r3, lr
*	Calls:	None
*
********************************************************************************/

	DECFN	ExceptionPreamble		// Cleanup up after ROM exception handler

	mfsprg	r3,2				// Get original value of counter
	mtctr	r3				// Restore to counter register
	mfsprg	r3,1				// Restore original value of r3

	DECFN	ExceptionStart			// Actually handle the exception

	mtsprg	1,r3				// Save r3 so we can use it for scratch
	mfmsr	r3				// Get current value of MSR
	ori	r3,r3,MSR_IR+MSR_DR		// Enable instruction/data translation
	mtmsr	r3				// Write new value back to MSR
	isync					// Make sure the above is done

	mflr	r3				// Get current value of link register
	mtsprg	2,r3				// Move to SPRG2 since kernel needs it
	b	ExceptionStart			// Loop forever

	DECFN	ExceptionEnd


/*******************************************************************************/

	.skip	0x0900 - (. - ExceptionTable)	// Set offset to 0x0900
	b	IllegalException		// Shouldn't get decrementer exceptions with MSR_IP set


/********************************************************************************
*
*	GetBATSizeField
*
*	This routine accepts a 32 bit memory range size and returns the
*	corresponding BAT size-field value for that range.  The value is
*	determined by rounding the range size up to a power of two and then
*	using the value that corresponds to that power of two.  If the size
*	is greater than 256MB then the value for 256MB is used.  If the value
*	is less than 128KB then the value for 128KB is used.
*
*	Input:  r3 = Range size in bytes
*	Output: r3 = Corresponding BAT size field value
*	Mucks:	r4
*	Calls:	None
*
********************************************************************************/

GetBATSizeField:

	subi	r3,r3,1				// Subtract one from number
	cntlzw	r3,r3				// Get number of leading zeros
	cmpdi	r3,3				// Is range bigger than 256MB?
	bgt	CheckLowerLimit			// If not then skip over-limit code...
	li32	r3,4				// ... otherwise force size to 256MB

CheckLowerLimit:
	cmpdi	r3,15				// Is range smaller than or equal to 128KB?
	blt	GetValue			// If not then skip under-limit code...
	li32	r3,15				// ... otherwise force size to 128KB

GetValue:
	subi	r3,r3,4				// Subtract four to get entry number
	slwi	r3,r3,1				// Multiply by two to get table index
	lea	r4,BATSizeFieldValues		// Get address of lookup table
	lhzx	r3,r4,r3			// Look up size field value
	blr					// Return from routine

BATSizeFieldValues:
	.short	UBAT_BL_256M			// BAT size field for 128MB < size
	.short	UBAT_BL_128M			// BAT size field for 64MB < size <= 128MB
	.short	UBAT_BL_64M			// BAT size field for 32MB < size <= 64MB
	.short	UBAT_BL_32M			// BAT size field for 16MB < size <= 32MB
	.short	UBAT_BL_16M			// BAT size field for 8MB < size <= 16MB
	.short	UBAT_BL_8M			// BAT size field for 4MB < size <= 8MB
	.short	UBAT_BL_4M			// BAT size field for 2MB < size <= 4MB
	.short	UBAT_BL_2M			// BAT size field for 1MB < size <= 2MB
	.short	UBAT_BL_1M			// BAT size field for 512KB < size <= 1MB
	.short	UBAT_BL_512K			// BAT size field for 256KB < size <= 512KB
	.short	UBAT_BL_256K			// BAT size field for 128KB < size <= 256KB
	.short	UBAT_BL_128K			// BAT size field for 0 < size <= 128KB


/********************************************************************************
*
*	SaveContext
*
*	This routine saves the current context so we can use a "clean" context
*	for dipir.  The original context is restored later with RestoreContext
*	if necessary.
*
*	Input:	r12 = Link register value from calling routine
*	Output:	None
*	Mucks:	r3-r4, r7-r11
*	Calls:	None
*
********************************************************************************/

SaveContext:

	mfmsr	r11				// Get current value of MSR
	mfspr	r10,HID0			// Get current value of HID0
	mfsr	r9,SR15				// Get current value of SR15
	mr	r8,r2				// Get current value of r2
	mr	r7,r0				// Get current value of r0
	addi	sp,sp,-100			// Allocate stack space for context
	stmw	r7,0(sp)			// Save context in allocated space
	mtsr	SR15,sp				// Save stack pointer in SR15
	isync					// Make sure the above is done

	lea	r3,BootSavedBATs		// Get pointer to BAT storage area
	subi	r3,r3,4				// Decrement by 4 so we can use stwu

	mfspr	r4,IBAT0U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT0L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT1U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT1L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT2U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT2L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT3U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,IBAT3L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT0U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT0L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT1U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT1L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT2U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT2L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT3U			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	mfspr	r4,DBAT3L			// Get a BAT setting
	stwu	r4,4(r3)			// Save it and increment pointer
	isync					// Make sure the above is done

	blr					// Return from routine


/********************************************************************************
*
*	ReadSerialWord
*
*	This routine reads and returns a word from the diagnostic serial port.
*
*	Input:	r8 = CDE base address
*		r9 = Serial read-done flag bit mask
*	Output:	r3 = Word read from serial port
*	Mucks:	None
*	Calls:	ReadSerialByte
*
********************************************************************************/

ReadSerialWord:
	stack
	bl	ReadSerialByte			// Get first byte of word
	bl	ReadSerialByte			// Get second byte of word
	bl	ReadSerialByte			// Get third byte of word
	bl	ReadSerialByte			// Get fourth byte of word
	unstack
	blr


/********************************************************************************
*
*	ReadSerialByte
*
*	This routine reads and returns a byte from the diagnostic serial port.
*	Note that the byte that is read is also echoed back out over the serial
*	port.
*
*	Input:	r3 = Word template
*		r8 = CDE base address
*		r9 = Serial read-done flag bit mask
*	Output:	r3 = Left shifted template with serial port byte inserted
*	Mucks:	r4
*	Calls:	None
*
********************************************************************************/

ReadSerialByte:
	lwz	r4,CDE_INT_STS(r8)		// Get serial port status
	and.	r4,r4,r9			// Have we received a byte?
	beq	ReadSerialByte			// If not then look again

	lwz	r4,CDE_SDBG_RD(r8)		// Get received byte
	stw	r4,CDE_SDBG_WRT(r8)		// Echo received byte back out
	rlwimi	r4,r3,8,0,23			// Shift template and prepend to byte
	mr	r3,r4				// Copy result back into template

	stw	r9,CDE_INT_STC(r8)		// Clear the read done bit
	blr					// Return from routine


/********************************************************************************
*
*	CheckSumError
*
*	This routine continuously outputs the specified error code via the
*	serial diagnostic port.
*
*	Input:	r3 = Error code
*		r8 = CDE base address
*	Output:	None (never returns)
*	Mucks:	None
*	Calls:	None
*
********************************************************************************/

CheckSumError:
	stw	r3,CDE_SDBG_WRT(r8)		// Store error code to serial write register
	b	CheckSumError			// Repeat this loop forever


/*******************************************************************************/

	.skip	0x0C00 - (. - ExceptionTable)	// Set offset to 0x0C00
	b	IllegalException		// Shouldn't get system call exceptions with MSR_IP set


/*******************************************************************************/

	.skip	0x0D00 - (. - ExceptionTable)	// Set offset to 0x0D00
	b	IllegalException		// Shouldn't get trace exceptions with MSR_IP set


/********************************************************************************
*
*	SetupBATs
*
*	This routine sets up the CPU's block address translation registers.
*	The BATs are cleared first, as required by the chip spec.  The data
*	cache must be off since we need to read CDE and BDA registers.
*
*	The regions covered by the BATs are as follows:
*
*		IBAT0 - system ROM
*		DBAT0 - system ROM
*
*		IBAT1 - DRAM (supervisor)
*		DBAT1 - DRAM
*
*		IBAT2 - upper half of DRAM (user)
*		DBAT2 - Hardware registers (BDA at 0x00020000, CDE at 0x04000000)
*
*		IBAT3 - PCMCIA
*		DBAT3 - PCMCIA
*
*	Note that IBAT2 covers the upper half of DRAM for user mode with SE off
*	because we know there are no esa/dsa instructions outside the kernel.
*	For the lower half of DRAM, where users might need SE on, we rely on TLBs.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r5,r8
*	Calls:	GetBATSizeField
*		MemorySize
*
********************************************************************************/

SetupBATs:
	mflr	r8				// Save link register

	li	r3,0				// Get value to clear BATs (as per spec)
	mtspr	IBAT0U,r3			// Clear upper instruction BAT 0
	mtspr	IBAT0L,r3			// Clear lower instruction BAT 0
	mtspr	IBAT1U,r3			// Clear upper instruction BAT 1
	mtspr	IBAT1L,r3			// Clear lower instruction BAT 1
	mtspr	IBAT2U,r3			// Clear upper instruction BAT 2
	mtspr	IBAT2L,r3			// Clear lower instruction BAT 2
	mtspr	IBAT3U,r3			// Clear upper instruction BAT 3
	mtspr	IBAT3L,r3			// Clear lower instruction BAT 3

	mtspr	DBAT0U,r3			// Clear upper data BAT 0
	mtspr	DBAT0L,r3			// Clear lower data BAT 0
	mtspr	DBAT1U,r3			// Clear upper data BAT 1
	mtspr	DBAT1L,r3			// Clear lower data BAT 1
	mtspr	DBAT2U,r3			// Clear upper data BAT 2
	mtspr	DBAT2L,r3			// Clear lower data BAT 2
	mtspr	DBAT3U,r3			// Clear upper data BAT 3
	mtspr	DBAT3L,r3			// Clear lower data BAT 3

	// ---- Set up IBAT0 and DBAT0 (system ROM)

	lea	r3,SystemROMSize		// Get pointer to system ROM size
	lwz	r3,0(r3)			// Get size of system ROM
	bl	GetBATSizeField			// Get corresponding BAT size field value
	li32	r4,SYSROMSTART			// Get range start for IBAT0U...
	or	r3,r3,r4			// ... set to cover appropriate memory range...
	ori	r3,r3,UBAT_VS|UBAT_VP		// ... allow supervisor & user access...
	mtspr	IBAT0U,r3			// ... store to IBAT0U
	mtspr	DBAT0U,r3			// ... store to DBAT0U

#ifndef BUILD_PCDEBUGGER
	ori	r3,r4,LBAT_PP_READ		// ... enable read-only ...
#else
        ori     r3,r4,LBAT_PP_READWR            // ... enable read-write ...
#endif
	mtspr	IBAT0L,r3			// ... store to IBAT0L
	ori	r3,r3,LBAT_WIMG_GUARD		// ... make region guarded...
	mtspr	DBAT0L,r3			// ... store to DBAT0L

	// ---- Set up IBAT1 and DBAT1 (DRAM)

	bl	MemorySize			// Get size of memory in bytes
	mr	r5,r3				// Save a copy for use in setting up IBAT2
	bl	GetBATSizeField			// Get corresponding BAT size field value
	li32	r4,DRAMSTART			// Get range start for IBAT1U & DBAT1U...
	or	r3,r3,r4			// ... set to cover appropriate memory range...
	ori	r3,r3,UBAT_VS			// ... allow supervisor access...
	mtspr	DBAT1U,r3			// Store value to DBAT1U
	mtspr	IBAT1U,r3			// Store value to IBAT1U

	ori	r3,r4,LBAT_PP_READWR 		// ... enable read-write ...
	mtspr	DBAT1L,r3			// ... store to DBAT1L
	ori	r3,r3,LBAT_SE			// ... set special execute protection bit...
	mtspr	IBAT1L,r3			// ... store to IBAT1L...

	// ---- Set up IBAT2 (upper half of DRAM)

	srwi	r5,r5,1				// Divide memory size by 2
	mr	r3,r5				// Duplicate the result
	bl	GetBATSizeField			// Get corresponding BAT size field value
	li32	r4,DRAMSTART			// Get starting address of DRAM
	add	r4,r4,r5			// Calculate range start for IBAT2...
	or	r3,r3,r4			// ... set to cover appropriate memory range...
	ori	r3,r3,UBAT_VP			// ... allow user access
	mtspr	IBAT2U,r3			// ... store to IBAT2U

	ori	r3,r4,LBAT_PP_READWR 		// ... enable read-write ...
	mtspr	IBAT2L,r3			// ... store to IBAT2L

	// ---- Set up DBAT2 (hardware registers in low memory)

	li32	r4,0				// Get range start for DBAT2U...
	ori	r3,r4,UBAT_BL_128M|UBAT_VS	// ... set to cover 128MB...
						// ... allow supervisor access...
	mtspr	DBAT2U,r3			// ... store to DBAT2U

	ori	r3,r4,LBAT_WIMG_INHIB|LBAT_WIMG_GUARD|LBAT_PP_READWR
						// ... inhibit caching...
						// ... make region guarded...
						// ... allow reads and writes...
	mtspr	DBAT2L,r3			// ... store to DBAT2L

	// ---- Set up IBAT3 and DBAT3 (PCMCIA space)

	li32	r4,PCMCIA_BASE			// Get range start for DBAT3U...
	ori	r3,r4,UBAT_BL_256M|UBAT_VS	// ... set to cover 256MB...
						// ... allow supervisor access...
	mtspr	DBAT3U,r3			// ... store to DBAT3U
	ori	r3,r3,UBAT_VP			// ... allow user execute
	mtspr	IBAT3U,r3			// ... store to IBAT3U

	ori	r3,r4,LBAT_WIMG_INHIB|LBAT_PP_READWR
						// ... inhibit caching...
						// ... allow reads and writes...
	mtspr   IBAT3L,r3                       // ... store to IBAT3L
	ori     r3,r3,LBAT_WIMG_GUARD           // ... make region guarded...
	mtspr   DBAT3L,r3                       // ... store to DBAT3L

	isync					// Make sure the above is done
	mtlr	r8				// Restore link register
	blr					// Return from routine


/********************************************************************************
*
*	RestoreContext
*
*	This routine restores a context that was previously saved using
*	SaveContext.  It is important that this routine NOT modify r3 since
*	that register is used to pass back the dipir return value.
*
*	Note that the BATs are cleared prior to restoring them as required
*	by the chip spec.
*
*	Input:	None
*	Output:	None
*	Mucks:	r4-r5, r7-r12
*	Calls:	DisableMMU
*
********************************************************************************/

RestoreContext:
	stack
	bl	DisableMMU			// Disable MMU before changing BATs
	unstack

	li	r5,0				// Get value to clear BATs
	mtspr	IBAT0U,r5			// Store to BAT register
	mtspr	IBAT0L,r5			// Store to BAT register
	mtspr	IBAT1U,r5			// Store to BAT register
	mtspr	IBAT1L,r5			// Store to BAT register
	mtspr	IBAT2U,r5			// Store to BAT register
	mtspr	IBAT2L,r5			// Store to BAT register
	mtspr	IBAT3U,r5			// Store to BAT register
	mtspr	IBAT3L,r5			// Store to BAT register
	mtspr	DBAT0U,r5			// Store to BAT register
	mtspr	DBAT0L,r5			// Store to BAT register
	mtspr	DBAT1U,r5			// Store to BAT register
	mtspr	DBAT1L,r5			// Store to BAT register
	mtspr	DBAT2U,r5			// Store to BAT register
	mtspr	DBAT2L,r5			// Store to BAT register
	mtspr	DBAT3U,r5			// Store to BAT register
	mtspr	DBAT3L,r5			// Store to BAT register
	isync					// Make sure the above is done

	lea	r5,BootSavedBATs		// Get pointer to BAT storage area
	subi	r5,r5,4				// Decrement by 4 so we can use can use lwzu

	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT0U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT0L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT1U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT1L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT2U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT2L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT3U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	IBAT3L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT0U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT0L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT1U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT1L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT2U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT2L,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT3U,r4			// Restore to BAT register
	lwzu	r4,4(r5)			// Get BAT setting from pointer and increment
	mtspr	DBAT3L,r4			// Restore to BAT register
	isync					// Make sure the above is done

	mfsr	sp,SR15				// Get stack pointer from SR15
	lmw	r7,0(sp)			// Get context off stack
	addi	sp,sp,100			// Restore stack pointer
	mr	r0,r7				// Restore r0
	mr	r2,r8				// Restore r2
	mtsr	SR15,r9				// Restore SR15
	mtspr	HID0,r10			// Restore HID0
	mtmsr	r11				// Restore MSR
	isync					// Make sure the above is done

	blr					// Return from routine


/*******************************************************************************/

	.skip	0x1000 - (. - ExceptionTable)	// Set offset to 0x1000
	b	IllegalException		// Shouldn't get instruction translations miss exceptions with MSR_IP set


/********************************************************************************
*
*	WaitFieldTop(void)
*
*	This routine waits until the video raster is at the top of the next
*	video field.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r5
*	Calls:	None
*
********************************************************************************/

WaitFieldTop:
	li32	r3,BDAVDU_VLOC			// Get address of VCOUNT register
	li32	r4,0x3ff8			// Get mask for line count field

%loop1:						// Wait for beginning of line
	lwz	r5,0(r3)			// Get current value of VCOUNT
	and	r5,r5,r4			// Get current line number
	cmpi	r5,(VID_THRESHOLD<<3)		// Are we beyond the threshold?
	ble+	%loop1				// If not then keep waiting

%loop2:						// Wait for top of next field
	lwz	r5,0(r3)			// Get current value of VCOUNT
	and	r5,r5,r4			// Get current line number
	cmpi	r5,(VID_THRESHOLD<<3)		// Are we at top of next field?
	bge+	%loop2				// If not then keep waiting

	blr					// Return from routine


/********************************************************************************
*
*	ulong FieldCount(void)
*
*	This routine uses the CPU's time base counter to measure the time for
*	a specified number of video fields, as indicated by the system's VCOUNT
*	register.  The raw time in time base counts is returned.
*
*	Input:	r3 = Number of fields to time
*	Output:	r3 = Raw time base count
*	Mucks:	TBL/TBU
*	Calls:	WaitFieldTop
*
********************************************************************************/

	DECFN	FieldCount
	stack

	mtctr	r3				// Load counter with number of fields
	bl	WaitFieldTop			// Wait til top of next video field

	li	r3,0				// Get value to init time base register
	mttbl	r3				// Store to lower have of register
	mttbu	r3				// Store to upper have of register

FieldLoop:					// Wait specified number of fields
	bl	WaitFieldTop			// Wait til top of next video field
	bdnz+	FieldLoop			// Decrement counter; wait another loop if not done

	mftb	r3				// Get lower 32 bits of time base reg
	unstack
	blr					// Return from routine


/********************************************************************************
*
*	ulong CPUCount(ulong loops)
*
*	This routine uses the CPU's time base counter to measure the execution
*	time for a specified number of NOP instructions.  Rather than directly
*	timing the NOPs, the routine times the specified number of passes
*	through two loops which differ only by a single NOP.  The difference
*	is taken as the time used to execute the specified number of NOPs.
*	The raw time in time base counts is returned.
*
*	Note that the CPU's instruction cache must be on for the routine to
*	yield useful results.
*
*	Input:	r3 = Number of loops
*	Output:	r3 = Raw time base count
*	Mucks:	r4-r6
*	Calls:	None
*
********************************************************************************/

	DECFN	CPUCount

	mr	r4,r3				// Get copy of r3 to use as loop counter
	li	r5,0				// Get value to initialize time base counter
	mttbl	r5				// Initialize lower half of counter
	mttbu	r5				// Initialize upper half of counter

%loop4:
	subic.	r4,r4,1				// Decrement loop counter by one
	nop					// Wait a bit
	nop					// Wait a bit
	nop					// Wait a bit
	bgt+	%loop4				// If more loops left then do another

	mftb	r6				// Get lower 32 bits of CPU's timer

	mr	r4,r3				// Get copy of r3 to use as loop counter
	li	r5,0				// Get value to initialize time base counter
	mttbl	r5				// Initialize lower half of counter
	mttbu	r5				// Initialize upper half of counter

%loop5:
	subic.	r4,r4,1				// Decrement loop counter by one
	nop					// Wait a bit
	nop					// Wait a bit
	nop					// Wait a bit
	nop					// This is the extra NOP
	bgt+	%loop5				// If more loops left then do another

	mftb	r3				// Get lower 32 bits of CPU's timer
	subf	r3,r6,r3			// Calculate difference due to extra NOP
	blr					// Return from routine


/*******************************************************************************/

	.skip	0x1100 - (. - ExceptionTable)	// Set offset to 0x1100
	b	IllegalException		// Shouldn't get data load translation miss exceptions with MSR_IP set


/********************************************************************************
*
*	IdentifyMasterCPU
*
*	This routine attempts to read the WhoAmI PAL in order to identify
*	the master CPU, and the slave CPU if it is present.  On single CPU
*	systems there is no WhoAmI PAL so the read generates a machine check
*	exception, which is used as an indicator that there is only one CPU.
*	On a dual CPU system the read returns a unique flag depending on
*	which CPU initiated the read, thus allowing the CPUs to identify
*	themselves.
*
*	Once the CPU is identified, a CPU ID code is stored in SR14 (segment
*	register 14) so that the CPU may properly identify itself in the
*	future.  This works because we run all CPUs in protection-only mode
*	(aka X mode), in which case SR1 thru SR15 are not used by the MMU.
*
*	Currently it is assumed that if the WhoAmI PAL is present then there
*	are exactly two CPUs in the system.  At some point in the future we
*	may make this routine more general purpose to allow several slaves.
*
*	All slave CPUs are configured as required for the OS, then put in doze
*	mode.  Only the master CPU actually returns from this routine.
*
*	Note that after the WhoAmI PAL read is completed, we disable machine
*	check exceptions in all CPUs.  This is done to ensure that no unexpected
*	machine check exceptions will occur that might hand control over to
*	renegade code in RAM.
*
*	Input:	None
*	Output:	SR14 = CPU ID
*	Mucks:	r3-r11
*	Calls:	DisableMachineChecks
*		SlaveCPUTest
*		ConfigureCPU (only for slave CPUs)
*
********************************************************************************/

IdentifyMasterCPU:
	stack

	li32	r4,WHOAMI_BASE			// Get address of WhoAmI CPU ID register
	lwz	r5,0(r4)			// Attempt to read the register
WhoAmIRead:
	isync					// Make sure the above is done
	andis.	r5,r5,(WHOAMI_CPUMASK >> 16)	// Mask off meaningless bits

WhoAmIRFI:
	mtsr	SR14,r5				// Store CPU ID to SR14 so CPU can identify itself later

	bl	DisableMachineChecks		// Disable machine check exceptions
	bl	SlaveCPUTest 			// Is this a slave CPU?
	beq	ConfigureCPU			// If yes then go do slave CPU setup (never returns)

	unstack	
	blr


/********************************************************************************
*
*	InitializeCPU
*
*	Make sure the CPU is in a safe, known state by making sure MSR and HID0
*	are in the same state they would be in after a hard reset.
*
*	In the hard reset case everything is by definition in a safe, known
*	state; InitializeCPU effectively just sets things up for the CPU
*	identification routine that follows it.
*
*	In the soft reset case these registers are NOT automatically put in
*	a safe, known state (with the exception of some of the MSR bits), so
*	it is important for security reasons to verify that whoever called
*	softreset cleaned things up first.  If they didn't then we assume this
*	is a security violation and jump to the illegal exception handler.
*
*	In addition, we check the slave external interrupt control line (BDA
*	GPIO[3]) to make sure it is not still active (low).  If it IS then we
*	again assume this is a security violation.  In reality this test just
*	checks the direction to make sure this bit is an input, since testing
*	the various cases when it is an output is more complex.
*
*	This routine also enables machine checks, and patches around a 602 bug
*	in which MSR[IP] is interpreted as set (regardless of its real value)
*	prior to the first rfi after a hard reset.  The work around is simply
*	to perform a NULL (i.e. no real action taken) rfi.
*
*	Note that it is NOT the purpose of this routine to completely set up
*	the CPU for use by the rest of the system; that task is handled a
*	little later in the boot process by ConfigureCPU().  This routine is
*	only intended to clean things up enough to allow us to identify the
*	master and slave CPUs without risking any security breaks.
*
*	Input:	None
*	Output:	None
*	Mucks:	r3-r4, SRR0, SRR1
*	Calls:	EnableMachineChecks
*
********************************************************************************/

InitializeCPU:
	stack

	mfmsr	r3				// Get current value of MSR
	li32	r4,MSR_ME			// Get machine check enable bit mask
	andc	r3,r3,r4			// Clear machine check enable bit
	cmpi	r3,MSR_IP			// Is the result the MSR hard reset state?
	bne	IllegalException		// If not then treat as an illegal exception

	mfspr	r3,HID0				// Get current value of HID0
	li32	r4,HID_EMCP | HID_DOZE | HID_DPM | HID_XMODE	// Get bits that might be set
	andc	r3,r3,r4			// Clear those bits
	cmpi	r3,HID_WIMG_GUARD		// Is the result the HID0 hard reset state?
	bne	IllegalException		// If not then treat as an illegal exception

	li32	r3,BDAMCTL_MREF			// Get address of BDA GPIO register
	lwz	r3,0(r3)			// Get current value of register
	andis.	r3,r3,(BDAMREF_GPIO3_DIR >> 16)	// Is the slave external exception control bit an input?
	bne	IllegalException		// If not then treat as an illegal exception

	bl	EnableMachineChecks		// Enable machine check exceptions

	lea	r3,RFIHack			// Get address to jump to in rfi hack
	mtsrr0	r3				// Put address in SRR0 for use by rfi instruction
	mfmsr	r4				// Get current value of MSR
	mtsrr1	r4				// Put value in SRR1 for use by rfi instruction
	rfi					// Execute RFI to make IP work correctly
	isync					// Make sure the above is done
RFIHack:

	unstack
	blr


/*******************************************************************************/

	.skip	0x1200 - (. - ExceptionTable)	// Set offset to 0x1200
	b	IllegalException		// Shouldn't get data store translation miss exceptions with MSR_IP set


/********************************************************************************
*
*	CheckForTestTool
*
*	This routine checks to see if a diagnostic test tool is connected to
*	the diagnostic serial port.  If not, then the routine simply returns.
*	Otherwise the routine either downloads code from the tool (if it was
*	a hard reset) or returns to the soft reset caller (if it was a soft
*	reset).
*
*	The protocol for this procedure is:
*
*	- Clear the read done bit.
*	- Send 0x3C out to signal the test tool.
*	- Wait up to 100mS for test tool to reply with 0xA5.  If we time out
*	  or receive some other value then assume no test tool and continue
*	  with regular boot.
*	- If we got here due to a soft reset then just return to caller.
*	- Read destination address (1 word long) from serial port.
*	- Read word count (1 word long) from serial port.
*	- Download specified number of words from serial port and place in
*	  memory starting at specified destination.
*	- Read checksum (1 word long) from serial port.
*	- If checksum doesn't match calculated checksum then loop forever,
*	  otherwise jump to destination.
*
*	The baud rate used for this entire process is the default baud
*	rate.  At current M2 clock speeds this is 9600 baud.
*
*	Note that wherever a word is read from the serial port, the most
*	significant byte should be sent first, then the second most
*	significant, and so on.
*
*	The checksum must cover the destination and word count, in addition
*	to the downloaded data.  This is a simple 32-bit checksum.  The
*	checksum is checked against two checksums that are calculated during
*	the download: one based on the the data received at the serial port
*	and one based on data that is read back from memory after it has been
*	written.  If their is a mismatch in either case, we fall into an
*	infinite loop that continuously sends out the appropriate error code
*	(NAK for a transfer error, ! for a memory error).
*
*	Each byte that is read by this routine is echoed back out via the
*	serial port in case the test tool wants to use this as a handshake
*	to throttle its data transfer.  This can also be used to check the
*	integrity of the data as it is being transferred.
*
*	Input:	SRR0, SRR1 contain the LR and MSR after a softreset
*	Output:	None
*	Mucks:	r3-r11
*	Calls:	ReadSerialWord
*
********************************************************************************/

#define	DD_DEFAULT_BAUD		0x3640		// Diagnostic serial port default baud setting
#define	DD_FLAG_BYTE		0x3C		// Byte value used to signal test tool
#define	DD_REPLY_LOOPS		0x11600		// Number of loops to wait for test tool reply
#define	DD_ACKNOWLEDGE_BYTE	0xA5		// Byte value expected back from test tool
#define	DD_TRANSFER_ERROR	0x15		// Transfer checksum error code (NAK)
#define	DD_MEMORY_ERROR		0x21		// Memory checksum error code (NAK)

CheckForTestTool:

	li32	r8,CDE_BASE			// Get base address of CDE
	li32	r9,CDE_SDBG_RD_DONE		// Get bit mask for serial read done bit
	stw	r9,CDE_INT_STC(r8)		// Make sure read done bit is cleared

	lwz	r10,CDE_SDBG_CNTL(r8)		// Save current value of control register
	li32	r3,DD_DEFAULT_BAUD		// Get value to set default baud rate
	stw	r3,CDE_SDBG_CNTL(r8)		// Store new value to control register

	li32	r3,DD_FLAG_BYTE			// Get flag byte to send to test tool
	stw	r3,CDE_SDBG_WRT(r8)		// Send out flag byte

	li32	r3,DD_REPLY_LOOPS		// Get number of loops to wait for reply
	mtctr	r3				// Load number of loops into counter
WaitForReply:
	lwz	r3,CDE_INT_STS(r8)		// Get serial port status
	and.	r3,r3,r9			// Have we received a reply?
	bne	CheckReply			// If so then go check the reply
	bdnz	WaitForReply			// If we haven't timed out, keep waiting
	b	NoTestTool			// Timed out so no test tool present

CheckReply:
	lwz	r3,CDE_SDBG_RD(r8)		// Get reply byte
	stw	r3,CDE_SDBG_WRT(r8)		// Echo received byte back out
	cmpi	r3,DD_ACKNOWLEDGE_BYTE		// Did we get the test tool reply flag?
	bne	NoTestTool			// If not then no test tool present

	stw	r9,CDE_INT_STC(r8)		// Clear read done so we can get more data
	lwz	r3,CDE_BBLOCK_EN(r8)		// Get current value of blocking enable register
	andi.	r3,r3,CDE_BLOCK_CLEAR		// Was this a hard reset?
	bne	DownloadCode			// If so then download diagnostic code
	rfi					// Otherwise return to caller of soft reset
	isync					// Make sure the above is done

DownloadCode:
	bl	ReadSerialWord			// Read destination from serial port
	mr	r11,r3				// Save for later use
	mr	r5,r3				// Set up destination pointer
	addi	r5,r5,-4			// Predecrement pointer so we can use stwu
	mr	r6,r3				// Initialize transfer checksum accumulator

	bl	ReadSerialWord			// Read word count from serial port
	mtctr	r3				// Put number of words in counter
	add	r6,r6,r3			// Add word count to transfer checksum accumulator
	mr	r7,r6				// Initialize memory checksum accumulator

DownloadWord:
	bl	ReadSerialWord			// Read one word of diagnostic code
	stwu	r3,4(r5)			// Store word to pointer and increment
	add	r6,r6,r3			// Add word to transfer checksum accumulator
	lwz	r3,0(r5)			// Read back word
	add	r7,r7,r3			// Add word to memory checksum accumulator
	bdnz	DownloadWord			// If more words left then keep downloading

	bl	ReadSerialWord			// Read checksum from serial port
	mr	r4,r3				// Move result into r4 so we can use r3
	li32	r3,DD_TRANSFER_ERROR		// Get error code for transfer checksum error
	cmp	r6,r4				// Does checksum match our transfer calculation?
	bne	CheckSumError			// If not then jump to error reporting routine
	li32	r3,DD_MEMORY_ERROR		// Get error code for memory checksum error
	cmp	r7,r4				// Does checksum match our memory calculation?
	bne	CheckSumError			// If not then jump to error reporting routine

	mtlr	r11				// Load destination into link register
	blr					// Jump to code at destination

NoTestTool:
	stw	r10,CDE_SDBG_CNTL(r8)		// Restore control register to original state
	blr					// Return from routine


/*******************************************************************************/

	.skip	0x1300 - (. - ExceptionTable)	// Set offset to 0x1300
	b	IllegalException		// Shouldn't get instruction address breakpoint exceptions with MSR_IP set


/********************************************************************************
*
*	PerformSoftReset
*
*	This routine performs a soft reset by writing the appropriate bit to the
*       ResetControl register in CDE.  This is necessary in order to clear the
*       BBLOCK_EN bit _prior_ to entering dipir.
*
*	The routine also handles any necessary context save/restore and the like
*	required to safely enter and recover from softreset.  This includes:
*
*	- Putting HID0 in its hardreset state
*	- Make the slave interrupt control GPIO an input to ensure that nothing
*	  can cause an unexpected interrupt of the slave
*
*	Input:	None
*	Output:	r3 = Dipir return value
*	Mucks:	r4-r6
*	Calls:	SaveContext
*		FlushDCacheAll
*		DisableICache
*		DisableDCache
*		UseROMVectors
*		RestoreContext
*
********************************************************************************/

	DECFN	PerformSoftReset

	mflr	r12				// Get current value of link register
	bl	SaveContext			// Save the current context so we can restore it after dipir

	li32	r3,BDAMCTL_MREF			// Get address of BDA GPIO register
	lwz	r4,0(r3)			// Get current value of register
	li32	r5,BDAMREF_GPIO3_DIR		// Get direction bit mask for GPIO3
	andc	r4,r4,r5			// Clear the direction bit to make GPIO3 an input
	stw	r4,0(r3)			// Store new value to BDA GPIO register

DipirAgain:
	bl	FlushDCacheAll			// Flush the entire data cache
	bl	DisableDCache			// Disable the data cache
	bl	DisableICache			// Disable the instruction cache
	bl	UseROMVectors			// Set up to use ROM exception vector table

	li32	r3,HID_WIMG_GUARD		// Get HID0 hard reset value
	mtspr	HID0,r3				// Move value to HID0
	isync					// Make sure the above is done

	li32	r4,CDE_BASE			// Get base address of CDE
	li	r5,CDE_SOFT_RESET		// Get offset to soft reset register
	stw	r5,CDE_RESET_CNTL(r4)		// Trigger soft reset
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur
	isync					// Wait a while to allow soft reset to occur

SoftResetRFI:
	cmpi	r3,0				// Do we need to call dipir again?
	beq-	DipirAgain			// If yes then go back and trigger another soft reset

	bl	RestoreContext			// Restore original context
	mtlr	r12				// Restore original link register value
	blr					// Return from routine


/*******************************************************************************/

	.skip	0x1400 - (. - ExceptionTable)	// Set offset to 0x1400
	b	IllegalException		// Shouldn't get system management interrupt exceptions with MSR_IP set


/********************************************************************************
*
*	MemorySet
*
*	This routine writes a given byte pattern into every byte of a given
*	memory range.  It is loosely based on the kernel's memset routine,
*	written by Martin T.  The general algorithm is:
*
*	If (length > 32)
*	    Set enough to align to 32 byte boundary
*	    While (length > 32)
*		Set 32 bytes
*		Decrement length by 32
*	While (length > 0)
*	    Set 1 byte
*	    Decrement length by 1
*
*	Input:	r3 = pattern (byte)
*		r4 = target address
*		r5 = length in bytes
*	Output:	None
*	Mucks:	r3-r6
*	Calls:	None
*
********************************************************************************/

	DECFN	MemorySet

	cmpi	r5,32				// Is length less than 32 bytes?
	blt-	SetByteInit			// If yes, skip 32-byte inner loop

	andi.	r6,r4,0x1F			// Extract lowest 5 bits of target
	beq-	SetBlockInit			// If aligned to 32 bytes, skip alignment code
	subfic	r6,r6,32			// Calculate bytes to get aligned
	mtctr	r6				// Load counter with that amount
	subf	r5,r6,r5			// Decrement length by same amount
	addi	r4,r4,-1			// Predecrement target so we can use stbu
SetAlignLoop:
	stbu	r3,1(r4)			// Increment target and set byte at target
	bdnz+	SetAlignLoop			// Decrement counter; repeat loop if not done
	addi	r4,r4,1				// Align target to 32 byte boundary
	cmpi	r5,32				// Is length less than 32 bytes?
	blt-	SetByteInit			// If yes, skip 32-byte inner loop

SetBlockInit:
        rlwimi	r3,r3,8,16,23			// Copy pattern to 2nd byte (r3 = r3 | (r3 << 8))
	rlwimi	r3,r3,16,0,15			// Copy pattern to upper half (r3 = r3 | (r3 << 16))
	addi	r4,r4,-4			// Predecrement target so we can use stwu
	srawi	r6,r5,5				// Calculate number of blocks (bytes / 32)
	mtctr	r6				// Load counter with that amount
SetBlockLoop:
	stw	r3,4(r4)			// Set word at target + 4
	stw	r3,8(r4)			// Set word at target + 8
	stw	r3,12(r4)			// Set word at target + 12
	stw	r3,16(r4)			// Set word at target + 16
	stw	r3,20(r4)			// Set word at target + 20
	stw	r3,24(r4)			// Set word at target + 24
	stw	r3,28(r4)			// Set word at target + 28
	stwu	r3,32(r4)			// Increment target by 32 and set word at target
	bdnz+	SetBlockLoop			// Decrement counter; repeat loop if not done
	andi.	r5,r5,0x1F			// Calculate bytes left over
	addi	r4,r4,4				// Align target to next unset byte

SetByteInit:
	cmpi	r5,0				// Are there any bytes to set?
	beqlr-					// If not then return from routine
	addi	r4,r4,-1			// Predecrement target so we can use stbu
	mtctr	r5				// Load counter with number of remaining bytes
SetByteLoop:
	stbu	r3,1(r4)			// Increment target and set byte at target
	bdnz+	SetByteLoop			// Decrement counter; repeat loop if not done

        blr					// Return from routine


/********************************************************************************
*
*	uint32 MemorySize(void)
*
*	This routine returns the amount of DRAM in the system.  Note that
*	only sizes of 0, 2, 4, 6, 8, 10, 12, 16, 18, 20, 24, and 32 megabytes
*	are technically feasible (based on the config bits in BDA).
*
*	Input:  None
*	Output: r3 = amount of DRAM
*	Mucks:	r4-r5
*	Calls:	None
*
********************************************************************************/

	DECFN	MemorySize

	li32	r3,BDAMCTL_MCONFIG		// Get address of memory config register
	lwz	r3,0(r3)			// Get value from that register
	extrwi	r4,r3,3,16			// Get BDAMCFG_SS1
	extrwi	r5,r3,3,19			// Get BDAMCFG_SS0

	lea	r3,SSTable			// Get address of size lookup table
	lbzx	r4,r3,r4			// Look up size based on SS1
	lbzx	r5,r3,r5			// Look up size based on SS0
	add	r3,r4,r5			// Add to get size in MBytes
	slwi	r3,r3,20			// Shift to get size in bytes
	blr					// Return from routine

SSTable:					// Lookup table for MemorySize
	.byte	0
	.byte	2
	.byte	4
	.byte	4
	.byte	4
	.byte	8
	.byte	16
	.byte	0


/*******************************************************************************/

	.skip	0x1500 - (. - ExceptionTable)	// Set offset to 0x1500
	b	IllegalException		// Shouldn't get watchdog timer exceptions with MSR_IP set


/********************************************************************************
*
*	MemoryMove
*
*	This routine moves a given number of bytes from a given source address
*	to a given target address.  It is based on the MemorySet routine. The
*	general algorithm is:
*
*	If (length > 32)
*	    Move enough to align source to 32 byte boundary
*	    While (length > 32)
*		Move 32 bytes
*		Decrement length by 32
*	While (length > 0)
*	    Move 1 byte
*	    Decrement length by 1
*
*	Input:	r3 = source address
*		r4 = target address
*		r5 = length in bytes
*	Output:	None
*	Mucks:	r3-r6
*	Calls:	None
*
********************************************************************************/

	DECFN	MemoryMove

	cmpi	r5,32				// Is length less than 32 bytes?
	blt-	MoveByteInit			// If yes, skip 32-byte inner loop

	andi.	r6,r3,0x1F			// Extract lowest 5 bits of source
	beq-	MoveBlockInit			// If aligned to 32 bytes, skip alignment code
	subfic	r6,r6,32			// Calculate bytes to get aligned
	mtctr	r6				// Load counter with that amount
	subf	r5,r6,r5			// Decrement length by same amount
	addi	r3,r3,-1			// Predecrement target so we can use lbzu
	addi	r4,r4,-1			// Predecrement target so we can use stbu
MoveAlignLoop:
	lbzu	r6,1(r3)			// Increment source and read byte
	stbu	r6,1(r4)			// Increment target and write byte
	bdnz+	MoveAlignLoop			// Decrement counter; repeat loop if not done
	addi	r3,r3,1				// Undo predecrement of source
	addi	r4,r4,1				// Undo predecrement of target
	cmpi	r5,32				// Is length less than 32 bytes?
	blt-	MoveByteInit			// If yes, skip 32-byte inner loop

MoveBlockInit:
	addi	r3,r3,-4			// Predecrement source so we can use lwzu
	addi	r4,r4,-4			// Predecrement target so we can use stwu
	srawi	r6,r5,5				// Calculate number of blocks (bytes / 32)
	mtctr	r6				// Load counter with that amount
MoveBlockLoop:
	lwz	r6,4(r3)			// Read word at source + 4
	stw	r6,4(r4)			// Write word to target + 4
	lwz	r6,8(r3)			// Read word at source + 8
	stw	r6,8(r4)			// Write word to target + 8
	lwz	r6,12(r3)			// Read word at source + 12
	stw	r6,12(r4)			// Write word to target + 12
	lwz	r6,16(r3)			// Read word at source + 16
	stw	r6,16(r4)			// Write word to target + 16
	lwz	r6,20(r3)			// Read word at source + 20
	stw	r6,20(r4)			// Write word to target + 20
	lwz	r6,24(r3)			// Read word at source + 24
	stw	r6,24(r4)			// Write word to target + 24
	lwz	r6,28(r3)			// Read word at source + 28
	stw	r6,28(r4)			// Write word to target + 28
	lwzu	r6,32(r3)			// Increment source by 32 and read word
	stwu	r6,32(r4)			// Increment target by 32 and write word
	bdnz+	MoveBlockLoop			// Decrement counter; repeat loop if not done
	andi.	r5,r5,0x1F			// Calculate bytes left over
	addi	r3,r3,4				// Undo predecrement of source
	addi	r4,r4,4				// Undo predecrement of target

MoveByteInit:
	cmpi	r5,0				// Are there any bytes to move?
	beqlr-					// If not then return from routine
	addi	r3,r3,-1			// Predecrement source so we can use lbzu
	addi	r4,r4,-1			// Predecrement target so we can use stbu
	mtctr	r5				// Load counter with number of remaining bytes
MoveByteLoop:
	lbzu	r6,1(r3)			// Increment source and read byte
	stbu	r6,1(r4)			// Increment target and write byte
	bdnz+	MoveByteLoop			// Decrement counter; repeat loop if not done

        blr					// Return from routine


/*******************************************************************************/

	.skip	0x1600 - (. - ExceptionTable)	// Set offset to 0x1600
	b	IllegalException		// Shouldn't get emulation trap exceptions with MSR_IP set


/*******************************************************************************/

