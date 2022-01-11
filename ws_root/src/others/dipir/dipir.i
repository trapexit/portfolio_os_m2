//
//	@(#) dipir.i 96/05/07 1.5
//	Copyright 1995, The 3DO Company
//
//	Assembly language definitions of structures defined in dipir.h.
//

// WARNING: layout of these structures must match definitions in dipir.h


#define jmp_buf_size		24
#define	JMP_ABORT		1

.struct RegBlock
	rb_GPRs			.long 32
	rb_CR			.long 1
	rb_XER			.long 1
	rb_LR			.long 1
	rb_CTR			.long 1
	rb_PC			.long 1
	rb_MSR			.long 1
.ends

.struct DipirTemp
	dt_Version		.long	1
	dt_DipirRoutines	.long	1
	dt_BootGlobals		.long	1
	dt_JmpBuf		.long	jmp_buf_size
#ifdef DIPIR_INTERRUPTS
	dt_InterruptSaveArea	.byte	RegBlock
#endif /* DIPIR_INTERRUPTS */
	// More members not needed by asm code, so not defined here.
.ends
