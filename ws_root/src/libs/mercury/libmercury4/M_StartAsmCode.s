		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

/*
 *	StartAsmCode
 *	Save off all the registers that the C code uses
 *
 *	INPUT:	GPR 0 -- saved lr
 *	OUTPUT:	none
 */
		DECFN		M_StartAsmCode
		subi		sp,sp,StackFrame
	
		stfs		14,StackFrame.Float14(sp)
		stfs		15,StackFrame.Float15(sp)
		stfs		16,StackFrame.Float16(sp)
		stfs		17,StackFrame.Float17(sp)
		stfs		18,StackFrame.Float18(sp)
		stfs		19,StackFrame.Float19(sp)
		stfs		20,StackFrame.Float20(sp)
		stfs		21,StackFrame.Float21(sp)
		stfs		22,StackFrame.Float22(sp)
		stfs		23,StackFrame.Float23(sp)
		stfs		24,StackFrame.Float24(sp)
		stfs		25,StackFrame.Float25(sp)
		stfs		26,StackFrame.Float26(sp)
		stfs		27,StackFrame.Float27(sp)
		stfs		28,StackFrame.Float28(sp)
		stfs		29,StackFrame.Float29(sp)
		stfs		30,StackFrame.Float30(sp)
		stfs		31,StackFrame.Float31(sp)
		stw		13,StackFrame.Int13(sp)
		stw		14,StackFrame.Int14(sp)
		stw		15,StackFrame.Int15(sp)
		stw		16,StackFrame.Int16(sp)
		stw		17,StackFrame.Int17(sp)
		stw		18,StackFrame.Int18(sp)
		stw		19,StackFrame.Int19(sp)
		stw		20,StackFrame.Int20(sp)
		stw		21,StackFrame.Int21(sp)
		stw		22,StackFrame.Int22(sp)
		stw		23,StackFrame.Int23(sp)
		stw		24,StackFrame.Int24(sp)
		stw		25,StackFrame.Int25(sp)
		stw		26,StackFrame.Int26(sp)
		stw		27,StackFrame.Int27(sp)
		stw		28,StackFrame.Int28(sp)
		stw		29,StackFrame.Int29(sp)
		stw		30,StackFrame.Int30(sp)
		stw		31,StackFrame.Int31(sp)

		mfctr		13
		stw		13,StackFrame.SaveCtr(sp)
		stw		0,StackFrame.SaveLr(sp)
		blr
