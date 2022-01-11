		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

		.include	"M_PreLight.i"

/*
 *	M_PreLight		-- called from C

 *	Loop over the sorted pod list.  Code jumps to here from PodPipeEntry
 *
 *	INPUT:	
 *		GPR3:pointer to first pod in sorted zero-terminated source linked list
 *		GPR4:pointer to first pod in sorted zero-terminated destination linked list
 *		GPR5:pointer to the "closedata" structure
 *
 *
 *	OUTPUT:	none
 *
 */			
		DECFN	M_PreLight
begincode:
/*	Save off all the registers that the C code uses.
 *	User input in totalcount and puserobjects remains untouched (GPR 3,4) */

		mflr		0
		bl		M_StartAsmCode
		mr		pnextpod2	,	4
		mr		pclosedata	,	5
		mr		pnextpod1	,	3
		b		Lpodloopend

/*	For each pod, we will:
 *		change case and texture if necessary
 *		concatenate object matrix with camera skew matrix
 *		determine visibility of object: none, clip case or fully visible
 *		invert the matrix
 *		concatenate the lights with the inverted matrix
 *		call the draw routine
 */
Lpodloopstart:		
		lwz		pltemp0,Pod.flags_off(ppod1)
		lwz		pnextpod1,Pod.pnext_off(ppod1)
		lwz		pnextpod2,Pod.pnext_off(ppod2)
		mtcr		pltemp0

/*	Call some assembly code, if the user requested it for this object */

		btl		callatstartFLAG,Lcalluserobjectsub

		lwz		pgeometry1,Pod.pgeometry_off(ppod1)
		lwz		pgeometry2,Pod.pgeometry_off(ppod2)
		lwz		pmatrix,Pod.pmatrix_off(ppod1)
		lwz		plights,Pod.plights_off(ppod1)
		lwz		pmaterial,Pod.pmaterial_off(ppod1)

/*	Concatenate the object matrix and the camera skew matrix.  The
 *	resulting registers should contain nine of the object matrix (the upper
 *	3x3) and all of the concatenated matrix.  This will minimize register
 *	reload later
 */

/*
 *	12 13 14		 0  1  2		 0  1  2
 *	15 16 17 times		 3  4  5	 equals	 3  4  5
 *	18 19 20		 6  7  8		 6  7  8
 *	21 22 23		 9 10 11		 9 10 11
 *
 *	temporary locations of results:
 *	21 22 23
 *	24 25 26
 *	27 28 29
 *  -- -- --
 */
		lfs		fr21,36(pmatrix)

/*	Following loads could be stuck into a subroutine */
		lfs		fr00,CloseData.fcamskewmatrix_off(pclosedata)
		lfs		fr01,CloseData.fcamskewmatrix_off+4(pclosedata)
		lfs		fr02,CloseData.fcamskewmatrix_off+8(pclosedata)
		lfs		fr03,CloseData.fcamskewmatrix_off+12(pclosedata)
		lfs		fr04,CloseData.fcamskewmatrix_off+16(pclosedata)
		lfs		fr05,CloseData.fcamskewmatrix_off+20(pclosedata)
		lfs		fr06,CloseData.fcamskewmatrix_off+24(pclosedata)
		lfs		fr07,CloseData.fcamskewmatrix_off+28(pclosedata)
		lfs		fr08,CloseData.fcamskewmatrix_off+32(pclosedata)
		lfs		fr09,CloseData.fcamskewmatrix_off+36(pclosedata)
		lfs		fr10,CloseData.fcamskewmatrix_off+40(pclosedata)
		lfs		fr11,CloseData.fcamskewmatrix_off+44(pclosedata)

		fmadds		fr09,fr21,fr00,fr09
		fmadds		fr10,fr21,fr01,fr10
		lfs		fr22,40(pmatrix)
		fmadds		fr11,fr21,fr02,fr11
		fmadds		fr09,fr22,fr03,fr09
		fmadds		fr10,fr22,fr04,fr10
		lfs		fr23,44(pmatrix)
		fmadds		fr11,fr22,fr05,fr11
		fmadds		fr09,fr23,fr06,fr09
		fmadds		fr10,fr23,fr07,fr10

		lfs		fr12,0(pmatrix)
		fmadds		fr11,fr23,fr08,fr11
		fmuls		fr21,fr12,fr00
		fmuls		fr22,fr12,fr01
		lfs		fr15,12(pmatrix)
		fmuls		fr23,fr12,fr02
		fmuls		fr24,fr15,fr00
		fmuls		fr25,fr15,fr01
		lfs		fr18,24(pmatrix)
		fmuls		fr26,fr15,fr02
		fmuls		fr27,fr18,fr00
		fmuls		fr28,fr18,fr01
		lfs		fr13,4(pmatrix)
		fmuls		fr29,fr18,fr02
		fmadds		fr21,fr13,fr03,fr21
		fmadds		fr22,fr13,fr04,fr22
		lfs		fr16,16(pmatrix)
		fmadds		fr23,fr13,fr05,fr23
		fmadds		fr24,fr16,fr03,fr24
		fmadds		fr25,fr16,fr04,fr25
		lfs		fr19,28(pmatrix)
		fmadds		fr26,fr16,fr05,fr26
		fmadds		fr27,fr19,fr03,fr27
		fmadds		fr28,fr19,fr04,fr28
		lfs		fr14,8(pmatrix)
		fmadds		fr29,fr19,fr05,fr29
		fmadds		fr00,fr14,fr06,fr21
		fmadds		fr01,fr14,fr07,fr22
		lfs		fr17,20(pmatrix)
		fmadds		fr02,fr14,fr08,fr23
		fmadds		fr03,fr17,fr06,fr24
		fmadds		fr04,fr17,fr07,fr25
		lfs		fr20,32(pmatrix)
		fmadds		fr05,fr17,fr08,fr26
		fmadds		fr06,fr20,fr06,fr27
		fmadds		fr07,fr20,fr07,fr28
		fmadds		fr08,fr20,fr08,fr29

/*	Now that we'll be drawing the object, let's save off the concatenated
 *	matrix!
 */
		stfs		fr00,CloseData.fxmat00_off(pclosedata)
		stfs		fr01,CloseData.fxmat01_off(pclosedata)
		stfs		fr02,CloseData.fxmat02_off(pclosedata)
		stfs		fr03,CloseData.fxmat10_off(pclosedata)
		stfs		fr04,CloseData.fxmat11_off(pclosedata)
		stfs		fr05,CloseData.fxmat12_off(pclosedata)
		stfs		fr06,CloseData.fxmat20_off(pclosedata)
		stfs		fr07,CloseData.fxmat21_off(pclosedata)
		stfs		fr08,CloseData.fxmat22_off(pclosedata)
		stfs		fr09,CloseData.fxmat30_off(pclosedata)
		stfs		fr10,CloseData.fxmat31_off(pclosedata)
		stfs		fr11,CloseData.fxmat32_off(pclosedata)

/*	The calculation trashed registers fr19 to fr23; reload them */
		lfs		fr21,36(pmatrix)
		lfs		fr22,40(pmatrix)
		lfs		fr23,44(pmatrix)

/*	Now we invert the matrix in 12 to 23.  If the upper 3x3 submatrix is
 *	not unit and orthogonal, the inverted light stuff doesn't work, so
 *	we assume it is unit and orthogonal.
 *
 *	12 13 14		 12 15 18				finvmat00 finvmat01 finvmat02
 *	15 16 17 becomes	 13 16 19 which we call			finvmat10 finvmat11 finvmat12
 *	18 19 20		 14 17 20				finvmat20 finvmat21 finvmat22
 *	21 22 23		 24 25 26				finvmat30 finvmat31 finvmat32
 */

/*	finvmat30 = -fr21*finvmat00-fr22*finvmat10-fr23*finvmat20 */
/*	finvmat31 = -fr21*finvmat01-fr22*finvmat11-fr23*finvmat21 */
/*	finvmat32 = -fr21*finvmat02-fr22*finvmat12-fr23*finvmat22 */

		fmuls		finvmat30,fr21,finvmat00
		fmuls		finvmat31,fr21,finvmat01
		fmuls		finvmat32,fr21,finvmat02
		lfs		fr19,28(pmatrix)		/* fr19 is also called finvmat12 */
		fmadds		finvmat30,fr22,finvmat10,finvmat30
		fmadds		finvmat31,fr22,finvmat11,finvmat31
		fmadds		finvmat32,fr22,finvmat12,finvmat32
		lfs		fr20,32(pmatrix)		/* fr20 is also called finvmat22 */
		fnmadds		finvmat30,fr23,finvmat20,finvmat30
		fnmadds		finvmat31,fr23,finvmat21,finvmat31
		fnmadds		finvmat32,fr23,finvmat22,finvmat32

		addi		pconvlights,pclosedata,CloseData.convlightdata_off-4
/*
 *	set closedata object material properties
 */
		li		pltemp0,11

/*	transform camera into local coordinates */

		lfs		fr09,CloseData.fcamx_off(pclosedata)
		lfs		fr10,CloseData.fcamy_off(pclosedata)
		lfs		fr11,CloseData.fcamz_off(pclosedata)

		fmadds		fr00,fr09,finvmat00,finvmat30
		fmadds		fr01,fr09,finvmat01,finvmat31
		fmadds		fr02,fr09,finvmat02,finvmat32
		fmadds		fr00,fr10,finvmat10,fr00
		fmadds		fr01,fr10,finvmat11,fr01
		fmadds		fr02,fr10,finvmat12,fr02
		fmadds		fr00,fr11,finvmat20,fr00
		fmadds		fr01,fr11,finvmat21,fr01
		fmadds		fr02,fr11,finvmat22,fr02

		stfs		fr00,CloseData.flocalcamx_off(pclosedata)
		stfs		fr01,CloseData.flocalcamy_off(pclosedata)
		stfs		fr02,CloseData.flocalcamz_off(pclosedata)

		mtctr		pltemp0
		subi		pmaterial,pmaterial,4
		addi		pltemp0,pclosedata,CloseData.frbase_off-4
.loop:
		lfsu		fr21,4(pmaterial)
		stfsu		fr21,4(pltemp0)
		bdnz+		.loop

		subi		plights,plights,4

		b		Llightloopentry
Llightlooptop:
		stwu		pltemp0,4(pconvlights)
		blrl
Llightloopentry:		
		lwzu		pltemp0,4(plights)
		lwzu		plightdata,4(plights)
		addic.		pltemp1,pltemp0,-4			/* initialization routine at -4 */
		mtlr		pltemp1
		bgt+		Llightlooptop		

/*	For each register under 14 not used, the return address can be
 *	pushed forward an instruction, for a maximum of 6 instructions.
 */
		lea		pltemp0,LightReturn
		stwu		pltemp0,4(pconvlights)

/*	All right! Time to call that draw routine!  The user may have a routine
 *	before or after this to prep stuff.
 */
		lwz		pfirstlight,CloseData.convlightdata_off(pclosedata)

		lwz		pvtx1,PodGeometry.pvertex_off(pgeometry1)
		lwz		pvtx2,PodGeometry.pvertex_off(pgeometry2)
		lhz		itemp0,PodGeometry.vertexcount_off(pgeometry1)

		srwi		itemp0,itemp0,1
		li		iconst32,32
		subi		pvtx1,pvtx1,4
		mtctr		itemp0
xformlooptop:
		lfs		fxmat00,CloseData.fxmat00_off(pclosedata)
		lfs		fxmat10,CloseData.fxmat10_off(pclosedata)
		lfs		fxmat20,CloseData.fxmat20_off(pclosedata)
		lfs		fxmat30,CloseData.fxmat30_off(pclosedata)
		lfs		fxmat01,CloseData.fxmat01_off(pclosedata)
		lfs		fxmat11,CloseData.fxmat11_off(pclosedata)
		lfs		fxmat21,CloseData.fxmat21_off(pclosedata)
		lfs		fxmat31,CloseData.fxmat31_off(pclosedata)
		lfs		fxmat02,CloseData.fxmat02_off(pclosedata)
		lfs		fxmat12,CloseData.fxmat12_off(pclosedata)
		lfs		fxmat22,CloseData.fxmat22_off(pclosedata)
		lfs		fxmat32,CloseData.fxmat32_off(pclosedata)
		lfs		fconstTINY,CloseData.fconstTINY_off(pclosedata)
		lfs		fconst2pt0,CloseData.fconst2pt0_off(pclosedata)

		lfsu		fx0,4(pvtx1)
		lfsu		fy0,4(pvtx1)
		lfsu		fz0,4(pvtx1)
		lfsu		fnx0,4(pvtx1)
		lfsu		fny0,4(pvtx1)
		lfsu		fnz0,4(pvtx1)
		lfsu		fx1,4(pvtx1)
		lfsu		fy1,4(pvtx1)
		lfsu		fz1,4(pvtx1)
		lfsu		fnx1,4(pvtx1)
		lfsu		fny1,4(pvtx1)
		lfsu		fnz1,4(pvtx1)
		lfs		fconst12million,CloseData.fconst12million_off(pclosedata)

		dcbt		0,pvtx2

		fmadds		fwp0,fx0,fxmat02,fxmat32
		fmadds		fwp1,fx1,fxmat02,fxmat32
		fmadds		fwp0,fy0,fxmat12,fwp0
		fmadds		fwp1,fy1,fxmat12,fwp1
		fmadds		fwp0,fz0,fxmat22,fwp0
		fmadds		fwp1,fz1,fxmat22,fwp1

		fmadds		fxp0,fx0,fxmat00,fxmat30
		fmadds		fxp1,fx1,fxmat00,fxmat30
		fmadds		fyp0,fx0,fxmat01,fxmat31
		fmadds		fyp1,fx1,fxmat01,fxmat31

		fmadds		fwinv0,fwp0,fwp0,fconstTINY
		fmadds		fwinv1,fwp1,fwp1,fconstTINY
		fmadds		fyp0,fy0,fxmat11,fyp0
		fmadds		fyp1,fy1,fxmat11,fyp1

		dcbt		pvtx2,iconst32

		fmadds		fxp0,fy0,fxmat10,fxp0
		fmadds		fxp1,fy1,fxmat10,fxp1
		frsqrte		fwinv0,fwinv0
		frsqrte		fwinv1,fwinv1

		fnmsubs		fconstTINY,fwinv0,fwp0,fconst2pt0	/* fconst2pt0-fwinv0*fwp0 */
		fnmsubs		fconst2pt0,fwinv1,fwp1,fconst2pt0	/* fconst2pt0-fwinv1*fwp1Ê*/
		fmadds		fyp0,fz0,fxmat21,fyp0
		fmadds		fyp1,fz1,fxmat21,fyp1

		fmuls		fwinv0,fwinv0,fconstTINY
		fmuls		fwinv1,fwinv1,fconst2pt0
		fmadds		fxp0,fz0,fxmat20,fxp0
		fmadds		fxp1,fz1,fxmat20,fxp1

		fmadds		fxpp0,fxp0,fwinv0,fconst12million
		fmadds		fxpp1,fxp1,fwinv1,fconst12million
		fmadds		fypp0,fyp0,fwinv0,fconst12million
		fmadds		fypp1,fyp1,fwinv1,fconst12million

		fsubs		fxpp0,fxpp0,fconst12million
		fsubs		fypp0,fypp0,fconst12million
		fsubs		fxpp1,fxpp1,fconst12million
		fsubs		fypp1,fypp1,fconst12million

		lfs		fracc0,CloseData.frbase_off(pclosedata)
		lfs		fgacc0,CloseData.fgbase_off(pclosedata)
		lfs		fbacc0,CloseData.fbbase_off(pclosedata)
		lfs		fracc1,CloseData.frbase_off(pclosedata)
		lfs		fgacc1,CloseData.fgbase_off(pclosedata)
		lfs		fbacc1,CloseData.fbbase_off(pclosedata)

		mtlr		pfirstlight
		addi		plightlist,pclosedata,CloseData.convlightdata_off

		blr
LightReturn:
		lfs		fltemp1,CloseData.fconst1pt0_off(pclosedata)

		fsubs		fltemp2,fracc0,fltemp1
		fsubs		fltemp3,fgacc0,fltemp1
		fsubs		fltemp4,fbacc0,fltemp1

		fsel		fracc0,fltemp2,fltemp1,fracc0
		fsel		fgacc0,fltemp3,fltemp1,fgacc0
		fsel		fbacc0,fltemp4,fltemp1,fbacc0

		fsubs		fltemp2,fracc1,fltemp1
		fsubs		fltemp3,fgacc1,fltemp1
		fsubs		fltemp4,fbacc1,fltemp1

		fsel		fracc1,fltemp2,fltemp1,fracc1
		fsel		fgacc1,fltemp3,fltemp1,fgacc1
		fsel		fbacc1,fltemp4,fltemp1,fbacc1

		stfs		fracc0,12(pvtx2)
		stfs		fgacc0,16(pvtx2)
		stfs		fbacc0,20(pvtx2)

		stfs		fracc1,36(pvtx2)
		stfs		fgacc1,40(pvtx2)
		stfs		fbacc1,44(pvtx2)

		addi		pvtx2,pvtx2,48
		bdnz		xformlooptop
Lpodloopend:		
		mr		ppod2,pnextpod2
		mr.		ppod1,pnextpod1
		bne+		Lpodloopstart		
		b		M_EndAsmCode
	
/*	Eight cycle overhead per call.  User subroutine must end in
 *	a blr instruction.
 */
Lcalluserobjectsub:		
		lwz		pltemp1,Pod.puserdata_off(ppod1)
		lwz		pltemp1,0(pltemp1)
		mtctr		pltemp1
		bctr
endcode:
		.space		4096-(endcode-begincode)
/*	END */
	
