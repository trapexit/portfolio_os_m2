		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

		.include	"M_Draw.i"
		.include	"M_Sort.i"
		.include	"M_DrawCommon.i"		/* used in EndAsmCode,SaveAndCallC */

/*
 *	Subroutines for M_DrawA
 */
Lcallcasecode:		
		lwz		3,Pod.pcase_off(ppod)
		bt		casecodeisasmFLAG,dontsaveandcall
		b		M_SaveAndCallC
dontsaveandcall:	
		mtctr		3
		bctr

/*	Eight cycle overhead per call.  User subroutine must end in
 *	a blr instruction.
 */
Lcalluserobjectsub:		
		lwz		pltemp1,Pod.puserdata_off(ppod)
		lwz		pltemp1,0(pltemp1)
		mtctr		pltemp1
		bctr

/*	Alter the point (fr24,fr25,fr26), fall through to visibilitysub */
Lvisibilitysubcasex:		
		fmadds		fr24,fr21,fr00,fr24
		fmadds		fr25,fr21,fr01,fr25			/* along x axis */
		fmadds		fr26,fr21,fr02,fr26
		fneg		fr21,fr21
/*	Check the point (fr24,fr25,fr26) vs the fulcrum, accumulate flags */
Lvisibilitysub:		
		mfcr		pltemp0
		and		iflagand,iflagand,pltemp0
		or		iflagor,iflagor,pltemp0
Lvisibilitysubfirsttime:
		fnmsubs		fr30,fr26,fr19,fr24			/* get x' - w' * screenwidth */
		fcmpu		0,fr24,fr27				/* x' vs 0.0 */
		fcmpu		1,fr25,fr27				/* y' vs 0.0 */
		fcmpu		2,fr27,fr30				/* 0.0 vs (x' - w' * screenwidth) */
		fnmsubs		fr30,fr26,fr20,fr25			/* get y' - w' * screenheight */
		fcmpu		3,fr26,fr28				/* w' vs wclose */
		fcmpu		4,fr29,fr26				/* wfar vs w' */
		fcmpu		5,fr27,fr30				/* 0.0 vs (y' - w' * screenheight) */
		blr
/*
 *	M_DrawA		-- called from C
 *
 *	Loop over the sorted pod list.  Code jumps to here from PodPipeEntry
 *
 *	INPUT:	
 *		GPR3:pointer to first pod in sorted zero-terminated linked list
 *		GPR4:pointer to the "closedata" structure
 *
 *
 *	OUTPUT:	none
 *
 */			
		DECFN	M_DrawA

/*	Save off all the registers that the C code uses.
 *	User input in GPR 3,4 remains untouched  */
		mflr		0
		bl		M_StartAsmCode
		lwz		pxformbase,CloseData.pxformbuffer_off(pclosedata)
		lwz		pVI,CloseData.pVIwrite_off(pclosedata)
		lwz		pVIwritemax,CloseData.pVIwritemax_off(pclosedata)
		addi		pVI,pVI,-4

/*	FAKE testing init */
		li		tricount,0

		mr		pnextpod,3
		b		Lpodloopend		
/*
 *	M_DrawPod	-- called from C
 *
 *  Called from C to draw only 1 pod	
 *
 *	INPUT:	
 *		GPR3:pointer to the pod to draw
 *		GPR4:pointer to the "closedata" structure
 *
 *
 *	OUTPUT:	none
 *
 */			

		DECFN	M_DrawPod
		mflr		0
		bl		M_StartAsmCode
		mr		pclosedata,4
		lwz		pxformbase,CloseData.pxformbuffer_off(pclosedata)
		lwz		pVI,CloseData.pVIwrite_off(pclosedata)
		lwz		pVIwritemax,CloseData.pVIwritemax_off(pclosedata)
		addi		pVI,pVI,-4
		li		tricount,0
		mr		ppod,3
		lwz		pltemp0,Pod.flags_off(ppod)
		li		pnextpod,0
		mtcr		pltemp0
		
		lwz		3,Pod.ptexture_off(ppod)
		cmpli		0,3,0
		bt		cr0eq,Lpodloopstartsingle
		
		lwz		itemp0,PodTexture.ptpagesnippets_off(3)
		lwz		itemp0,TpageSnippets.pselectsnippets_off(itemp0)
		stw		itemp0,CloseData.ptexselsnippets_off(pclosedata)

		b		Lpodloopstartsingle

/*	CACHE-RESIDENT 1024 INSTRUCTIONS START HERE! */

/*	For each pod, we will:
 *		change case and texture if necessary
 *		concatenate object matrix with camera skew matrix
 *		determine visibility of object: none, clip case or fully visible
 *		invert the matrix
 *		concatenate the lights with the inverted matrix
 *		call the draw routine
 */
Lpodloopstart:		
		lwz		pltemp0,Pod.flags_off(ppod)
		lwz		pnextpod,Pod.pnext_off(ppod)
		mtcr		pltemp0

Lpodloopstartsingle:	
/*	Call some code, if the case is different */
		bfl-		samecaseFLAG,Lcallcasecode		

/*	Call some assembly code, if the user requested it for this object */
		btl		callatstartFLAG,Lcalluserobjectsub

/*	Load a texture page into the cache, if it is different */
		bt+		sametextureFLAG,Lnotextureload		

		lwz		3,Pod.ptexture_off(ppod)
		cmpli		0,3,0
		bt		cr0eq,Lnotextureload
		.ifdef STATISTICS
		lwz		itemp0,CloseData.numtexloads(pclosedata)
		addi		itemp0,itemp0,1
		stw		itemp0,CloseData.numtexloads(pclosedata)

		lwz		itemp0,CloseData.numtexbytes(pclosedata)
		lwz		itemp1,PodTexture.texturebytes_off(3)
		add		itemp0,itemp0,itemp1
		stw		itemp0,CloseData.numtexbytes(pclosedata)
		.endif
		lwz		0,0(3)
		mtlr		0
		blrl
Lnotextureload:		
		lwz		pgeometry,Pod.pgeometry_off(ppod)
		lwz		pmatrix,Pod.pmatrix_off(ppod)
		lwz		plights,Pod.plights_off(ppod)
		lwz		pmaterial,Pod.pmaterial_off(ppod)

/*	Concatenate the object matrix and the camera skew matrix.  The
 *	resulting registers should contain nine of the object matrix (the upper
 *	3x3) and all of the concatenated matrix.  This will minimize register
 *	reload later
 */
		bf		preconcatFLAG,doconcat
		lfs		fr00,0x00(pmatrix)
		lfs		fr01,0x04(pmatrix)
		lfs		fr02,0x08(pmatrix)
		lfs		fr03,0x0c(pmatrix)
		lfs		fr04,0x10(pmatrix)
		lfs		fr05,0x14(pmatrix)
		lfs		fr06,0x18(pmatrix)
		lfs		fr07,0x1c(pmatrix)
		lfs		fr08,0x20(pmatrix)
		lfs		fr09,0x24(pmatrix)
		lfs		fr10,0x28(pmatrix)
		lfs		fr11,0x2c(pmatrix)

		fmr		finvmat00,fr00
		fmr		finvmat10,fr01
		fmr		finvmat20,fr02
		fmr		finvmat01,fr03
		fmr		finvmat11,fr04
		fmr		finvmat21,fr05
		fmr		finvmat02,fr06
		fmr		finvmat12,fr07
		fmr		finvmat22,fr08

		b		doneconcat
doconcat:
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
doneconcat:
		lfs		fr21,PodGeometry.xextent_off(pgeometry)	/* start clip loads */
		lfs		fr22,PodGeometry.yextent_off(pgeometry)	/* start clip loads */
		lfs		fr30,PodGeometry.xmin_off(pgeometry)	/* start clip loads */

/*	The visibility calculation works by tracing around the edge of the
 *	object's bounding rectangular solid, checking visibility and setting
 *	flags.  If all eight points are outside the frustrum in a certain
 *	direction, the object is offscreen.  If all points are within the frustrum,
 *  then the object is visible and can be drawn using the simple, non-clip
 *	routine.  Otherwise, the clip flag is set.
 */

/*
 *	Regs at this point are or will be:
 * 			fr19	fscreenwidth
 * 			fr20	fscreenheight
 * 			fr21	fxextent from pgeometry
 * 			fr22	fyextent from pgeometry
 * 			fr23	fzextent from pgeometry
 * 			fr24	x of some corner of the object's bounding rectangular solid
 * 			fr25	y
 * 			fr26	z
 * 			fr27	fconst0pt0
 * 			fr28	fwclose
 * 			fr29	fwfar
 * 			fr30	temporary
 */
/*	Start by transforming the minimum point in the object's bounding box */
		bt		usercheckedclipFLAG,Ldonewithclipcheck		

		lfs		fr29,PodGeometry.ymin_off(pgeometry)
		fmadds		fr24,fr00,fr30,fr09			/* x' */
		fmadds		fr25,fr01,fr30,fr10			/* y' */
		fmadds		fr26,fr02,fr30,fr11			/* z' */
		fmadds		fr24,fr03,fr29,fr24
		lfs		fr30,PodGeometry.zmin_off(pgeometry)
		fmadds		fr25,fr04,fr29,fr25
		fmadds		fr26,fr05,fr29,fr26
		fmadds		fr24,fr06,fr30,fr24
		fmadds		fr25,fr07,fr30,fr25
		lfs		fr23,PodGeometry.zextent_off(pgeometry)
		fmadds		fr26,fr08,fr30,fr26

		lfs		fr27,CloseData.fconst0pt0_off(pclosedata)
		lfs		fr28,CloseData.fwclose_off(pclosedata)
		lfs		fr29,CloseData.fwfar_off(pclosedata)
		lfs		fr19,CloseData.fscreenwidth_off(pclosedata)
		lfs		fr20,CloseData.fscreenheight_off(pclosedata)

		mfcr		saveflags
	
		li		iflagor,0
		li		iflagand,-1
	
		bl		Lvisibilitysubfirsttime		
		bl		Lvisibilitysubcasex			/* move along +x axis of rectangular solid */
		fmadds		fr24,fr22,fr03,fr24
		fmadds		fr25,fr22,fr04,fr25			/* along +y axis */ 
		fmadds		fr26,fr22,fr05,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* -x */
		fmadds		fr24,fr23,fr06,fr24
		fmadds		fr25,fr23,fr07,fr25			/* +z */
		fmadds		fr26,fr23,fr08,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* +x */
		fnmsubs		fr24,fr22,fr03,fr24
		fnmsubs		fr25,fr22,fr04,fr25			/* -y */
		fnmsubs		fr26,fr22,fr05,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* -x */

		lis		pltemp1,0x8888				/* new version by CHS */
		ori		pltemp1,pltemp1,0x8800

		mfcr		pltemp0
		and		iflagand,iflagand,pltemp0
		and.		iflagand,iflagand,pltemp1
		or		iflagor,iflagor,pltemp0
		bne-		Lpodloopend				/* all points off in some direction */
		mtcr		saveflags
		andis.		pltemp0,iflagor,0x0008
		crand		hithernocullFLAG,hithernocullFLAG,cr0gt
		crnot		cr0eq,frontcullFLAG
		crand		hithernocullFLAG,hithernocullFLAG,cr0eq
		and.		iflagor,iflagor,pltemp1
		cror		nocullFLAG,nocullFLAG,hithernocullFLAG
		crnot		clipFLAG,eq				/* some point off in some direction */
		.ifdef STATISTICS
		bt		clipFLAG,countclip
		lwz		itemp0,CloseData.numpods_fast(pclosedata)
		addi		itemp0,itemp0,1
		stw		itemp0,CloseData.numpods_fast(pclosedata)
		b		donecount
countclip:	
		lwz		itemp0,CloseData.numpods_slow(pclosedata)
		addi		itemp0,itemp0,1
		stw		itemp0,CloseData.numpods_slow(pclosedata)
donecount:	
		.endif
Ldonewithclipcheck:		

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
		li		pltemp0,Material.specdata_off

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

		srawi		pltemp0,pltemp0,2
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
		lwz		pltemp0,CloseData.plightreturn_off(pclosedata)
		lwz		pltemp1,CloseData.pdrawroutine_off(pclosedata)	/* from below */
		stwu		pltemp0,4(pconvlights)

/*	All right! Time to call that draw routine!  The user may have a routine
 *	before or after this to prep stuff.
 */
		mtlr		pltemp1
		blrl
		btl		callatendFLAG,Lcalluserobjectsub		

Lpodloopend:		
		mr.		ppod,pnextpod
		bne+		Lpodloopstart		

/*	Restore registers, return to C code */
/*	FAKE testing return */
		mr		3,tricount
		addi		pVI,pVI,4
		stw		pVI,CloseData.pVIwrite_off(pclosedata)
		stw		pVIwritemax,CloseData.pVIwritemax_off(pclosedata)
		b		M_EndAsmCode

/*	END */
	
