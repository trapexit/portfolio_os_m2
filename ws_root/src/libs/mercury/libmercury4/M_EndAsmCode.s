		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

/*
 *	EndAsmCode
 *	Restore saved registers, return to C code
 *
 *	INPUT:	none
 *	OUTPUT:	none
 *
 *	NOTE: does not disturb integer registers 3 and 4
 */
		DECFN		M_EndAsmCode

		lwz		14,StackFrame.SaveCtr(sp)
		lwz		15,StackFrame.SaveLr(sp)
		mtctr		14
		mtlr		15

		lfs		14,StackFrame.Float14(sp)
		lfs		15,StackFrame.Float15(sp)
		lfs		16,StackFrame.Float16(sp)
		lfs		17,StackFrame.Float17(sp)
		lfs		18,StackFrame.Float18(sp)
		lfs		19,StackFrame.Float19(sp)
		lfs		20,StackFrame.Float20(sp)
		lfs		21,StackFrame.Float21(sp)
		lfs		22,StackFrame.Float22(sp)
		lfs		23,StackFrame.Float23(sp)
		lfs		24,StackFrame.Float24(sp)
		lfs		25,StackFrame.Float25(sp)
		lfs		26,StackFrame.Float26(sp)
		lfs		27,StackFrame.Float27(sp)
		lfs		28,StackFrame.Float28(sp)
		lfs		29,StackFrame.Float29(sp)
		lfs		30,StackFrame.Float30(sp)
		lfs		31,StackFrame.Float31(sp)
		lwz		13,StackFrame.Int13(sp)
		lwz		14,StackFrame.Int14(sp)
		lwz		15,StackFrame.Int15(sp)
		lwz		16,StackFrame.Int16(sp)
		lwz		17,StackFrame.Int17(sp)
		lwz		18,StackFrame.Int18(sp)
		lwz		19,StackFrame.Int19(sp)
		lwz		20,StackFrame.Int20(sp)
		lwz		21,StackFrame.Int21(sp)
		lwz		22,StackFrame.Int22(sp)
		lwz		23,StackFrame.Int23(sp)
		lwz		24,StackFrame.Int24(sp)
		lwz		25,StackFrame.Int25(sp)
		lwz		26,StackFrame.Int26(sp)
		lwz		27,StackFrame.Int27(sp)
		lwz		28,StackFrame.Int28(sp)
		lwz		29,StackFrame.Int29(sp)
		lwz		30,StackFrame.Int30(sp)
		lwz		31,StackFrame.Int31(sp)

		addi		sp,sp,StackFrame
		blr
