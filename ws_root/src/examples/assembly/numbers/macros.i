
/******************************************************************************
**
**  @(#) macros.i 96/02/21 1.2
**
**  Some useful macros for use with ppcas
**
******************************************************************************/


# this macro loads a 32-bit constant into a register.
# It is smart enough to do the operation in a single instruction
# when possible.
.macro li32    reg, const32bit
        .if     (\const32bit & 0xFFFF0000)
		.if     ((\const32bit & 0xFFFF8000) - 0xFFFF8000)
			lis     \reg,(((\const32bit)>>16) & 0xFFFF)
                        .if     (\const32bit & 0xFFFF)
                                ori     \reg,\reg,((\const32bit) & 0xFFFF)
                        .endif
                .else
	                li      \reg,(\const32bit & -1)
                .endif
        .else
                .if     (\const32bit & 0x8000)
                        li      \reg,0
                        ori     \reg,\reg,(\const32bit)
                .else
                        li      \reg,((\const32bit) & 0xFFFF)
                .endif
        .endif
.endm

# load the effective address of a global symbol
.macro lea	reg, addr
	lis     \reg,\addr@h
	ori     \reg,\reg,\addr@l
.endm
