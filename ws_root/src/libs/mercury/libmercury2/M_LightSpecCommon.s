	.include	"M_LightCommon.i"

/*	Initialization functions for Specular lighting */

	.include	"structmacros.i"
	.include	"PPCMacroequ.i"
	.include	"conditionmacros.i"
	.include	"mercury.i"
	.include	"M_Draw.i"

/*
 *	M_ComputeSpecDataA
 *	Computes the specular spline coefficients
 *	this is easily done in C so we call out to C
 *
 *	INPUT:	the ppod in register 10
 *	OUTPUT:	results are placed in the material of the Pod
 *
 */
		DECFN	M_ComputeSpecDataA

/*	...but for now, we'll save everything!  0 is assumed unneccessary.
 */

		subi		sp,sp,4*32
		mflr		0
		stw		0,0(sp)
		mfcr		0
		stw		0,4(sp)

		stfs		fr12,8(sp)
		stfs		fr13,12(sp)

		stmw		4,16(sp)		/* save 28 words */

/*	Called C code uses this space! */
		subi		sp,sp,16
		mr		3, ppod
		bl		M_ComputeSpecData

		addi		sp,sp,16

		lwz		0,0(sp)
		lwz		3,4(sp)
		mtlr		0
		mtcr		3

		lfs		fr12,8(sp)
		lfs		fr13,12(sp)

		lmw		4,16(sp)		/* load 28 words */
		addi		sp,sp,4*32
		blr

