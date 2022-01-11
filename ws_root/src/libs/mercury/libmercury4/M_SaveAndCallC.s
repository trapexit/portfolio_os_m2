		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"
		.include	"M_Draw.i"

/*
 *	SaveAndCallC
 *	Save all necessary registers and call C.  This routine assumes
 *	that no floating point routines need to be saved, but that flags do.
 *
 *	INPUT:	The address to call,in register 3
 *	OUTPUT:	none
 *
 *	NOTE: does not disturb integer registers 3 and 4
 */
		DECFN	M_SaveAndCallC

/*	The registers that need to be saved are:
 *	flags,ppod,pnextpod,pclose,ppVIwrite?,ppVIwritemax?,pVIwrite,pVIwritemax...
 *	...but for now, we'll save everything!  0 is assumed unneccessary.
 */

		subi		sp,sp,4*30
		mflr		0
		stw		0,0(sp)
		mtlr		3
		mfcr		0
		stw		0,4(sp)

		addi		pVI,pVI,4
		stw		pVI,CloseData.pVIwrite_off(pclosedata)
		stw		pVIwritemax,CloseData.pVIwritemax_off(pclosedata)

		stmw		4,8(sp)		/* save 28 words */

/*	Called C code uses this space! */
		subi		sp,sp,16
		mr		3, pclosedata
		blrl
		addi		sp,sp,16

		lwz		0,0(sp)
		lwz		3,4(sp)
		mtlr		0
		mtcr		3

		lmw		4,8(sp)		/* load 28 words */

		lwz		pVI,CloseData.pVIwrite_off(pclosedata)
		lwz		pVIwritemax,CloseData.pVIwritemax_off(pclosedata)
		addi		pVI,pVI,-4

		addi		sp,sp,4*30
		blr
