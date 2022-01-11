		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

		.include	"M_BoundsTest.i"

/*
 *	M_BoundsTest		-- called from C

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
		DECFN	M_BoundsTest
begincode:
/*	Save off all the registers that the C code uses.
 *	User input in totalcount and puserobjects remains untouched (GPR 3,4) */
		mflr		0
		bl		M_StartAsmCode
		mr		pclosedata,4

		lfs		fr05,CloseData.fcamskewmatrix_off+20(pclosedata)
		lfs		fr06,CloseData.fcamskewmatrix_off+24(pclosedata)
		lfs		fr07,CloseData.fcamskewmatrix_off+28(pclosedata)
		lfs		fr08,CloseData.fcamskewmatrix_off+32(pclosedata)
		lfs		fr09,CloseData.fcamskewmatrix_off+36(pclosedata)
		lfs		fr10,CloseData.fcamskewmatrix_off+40(pclosedata)
		lfs		fr11,CloseData.fcamskewmatrix_off+44(pclosedata)
		mr		pnextbbox,3
		b		Lbboxloopend		
Lbboxloopstart:		
		lfs		fr00,CloseData.fcamskewmatrix_off(pclosedata)
		lfs		fr01,CloseData.fcamskewmatrix_off+4(pclosedata)
		lfs		fr02,CloseData.fcamskewmatrix_off+8(pclosedata)
		lfs		fr03,CloseData.fcamskewmatrix_off+12(pclosedata)
		lfs		fr04,CloseData.fcamskewmatrix_off+16(pclosedata)
		lwz		pnextbbox,BBoxList.pnext_off(pbboxlist)
		lwz		pbbox,BBoxList.pbbox_off(pbboxlist)
		lwz		pmatrix,BBoxList.pmatrix_off(pbboxlist)
		lwz		iflags,BBoxList.flags_off(pbboxlist)
		andi.		iflags,iflags,0xff
		mtcr		iflags

/*	Concatenate the object matrix and the camera skew matrix.
 *
 *	12 13 14		 0  1  2		 12 13 14
 *	15 16 17 times		 3  4  5	 equals	 15 16 17
 *	18 19 20		 6  7  8		 18 19 20
 *	21 22 23		 9 10 11		 21 22 23
 *
 */
		lfs		fr12,0(pmatrix)
		lfs		fr13,4(pmatrix)
		lfs		fr14,8(pmatrix)
		lfs		fr15,12(pmatrix)
		lfs		fr16,16(pmatrix)
		lfs		fr17,20(pmatrix)
		lfs		fr18,24(pmatrix)
		lfs		fr19,28(pmatrix)
		lfs		fr20,32(pmatrix)
		lfs		fr21,36(pmatrix)
		lfs		fr22,40(pmatrix)
		lfs		fr23,44(pmatrix)

		fmuls		fr24,fr12,fr00
		fmuls		fr25,fr12,fr01
		fmuls		fr26,fr12,fr02
		fmadds		fr24,fr13,fr03,fr24
		fmadds		fr25,fr13,fr04,fr25
		fmadds		fr26,fr13,fr05,fr26
		fmadds		fr12,fr14,fr06,fr24
		fmadds		fr13,fr14,fr07,fr25
		fmadds		fr14,fr14,fr08,fr26

		fmuls		fr24,fr15,fr00
		fmuls		fr25,fr15,fr01
		fmuls		fr26,fr15,fr02
		fmadds		fr24,fr16,fr03,fr24
		fmadds		fr25,fr16,fr04,fr25
		fmadds		fr26,fr16,fr05,fr26
		fmadds		fr15,fr17,fr06,fr24
		fmadds		fr16,fr17,fr07,fr25
		fmadds		fr17,fr17,fr08,fr26

		fmuls		fr24,fr18,fr00
		fmuls		fr25,fr18,fr01
		fmuls		fr26,fr18,fr02
		fmadds		fr24,fr19,fr03,fr24
		fmadds		fr25,fr19,fr04,fr25
		fmadds		fr26,fr19,fr05,fr26
		fmadds		fr18,fr20,fr06,fr24
		fmadds		fr19,fr20,fr07,fr25
		fmadds		fr20,fr20,fr08,fr26

		fmadds		fr24,fr21,fr00,fr09
		fmadds		fr25,fr21,fr01,fr10
		fmadds		fr26,fr21,fr02,fr11
		fmadds		fr24,fr22,fr03,fr24
		fmadds		fr25,fr22,fr04,fr25
		fmadds		fr26,fr22,fr05,fr26
		fmadds		fr21,fr23,fr06,fr24
		fmadds		fr22,fr23,fr07,fr25
		fmadds		fr23,fr23,fr08,fr26


/*	The visibility calculation works by tracing around the edge of the
 *	object's bounding rectangular solid, checking visibility and setting
 *	flags.  If all eight points are outside the frustrum in a certain
 *	direction, the object is offscreen.  If all points are within the frustrum,
 *  then the object is visible and can be drawn using the simple, non-clip
 *	routine.  Otherwise, the clip flag is set.
 */

/*
 *	Regs at this point are or will be:
 * 			fr24	x of some corner of the object's bounding rectangular solid
 * 			fr25	y
 * 			fr26	z
 * 			fr27	xextent
 * 			fr28	yextent
 * 			fr29	zextent
 * 			fr30	temporary
 * 			fr31	temporary
 */
/*	Start by transforming the minimum point in the object's bounding box */
TestBounds:
		lfs		fr27,BBox.xmin_off(pbbox)
		lfs		fr28,BBox.ymin_off(pbbox)
		lfs		fr29,BBox.zmin_off(pbbox)
		lfs		fr00,CloseData.fwclose_off(pclosedata)
		lfs		fr01,CloseData.fconst0pt0_off(pclosedata)
		lfs		fr02,CloseData.fscreenwidth_off(pclosedata)
		lfs		fr03,CloseData.fscreenheight_off(pclosedata)
		lfs		fr04,CloseData.fwfar_off(pclosedata)

		fmadds		fr24,fr12,fr27,fr21			/* x' */
		fmadds		fr25,fr13,fr27,fr22			/* y' */
		fmadds		fr26,fr14,fr27,fr23			/* z' */

		fmadds		fr24,fr15,fr28,fr24
		fmadds		fr25,fr16,fr28,fr25
		fmadds		fr26,fr17,fr28,fr26

		fmadds		fr24,fr18,fr29,fr24
		fmadds		fr25,fr19,fr29,fr25
		fmadds		fr26,fr20,fr29,fr26

		lfs		fr27,BBox.xextent_off(pbbox)
		lfs		fr28,BBox.yextent_off(pbbox)
		lfs		fr29,BBox.zextent_off(pbbox)

		li		iflagand,-1
	
		bl		Lvisibilitysubfirsttime		
		bl		Lvisibilitysubcasex			/* move along +x axis of rectangular solid */
		fmadds		fr24,fr28,fr15,fr24
		fmadds		fr25,fr28,fr16,fr25			/* along +y axis */ 
		fmadds		fr26,fr28,fr17,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* -x */
		fmadds		fr24,fr29,fr18,fr24
		fmadds		fr25,fr29,fr19,fr25			/* +z */
		fmadds		fr26,fr29,fr20,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* +x */
		fnmsubs		fr24,fr28,fr15,fr24
		fnmsubs		fr25,fr28,fr16,fr25			/* -y */
		fnmsubs		fr26,fr28,fr17,fr26
		bl		Lvisibilitysub		
		bl		Lvisibilitysubcasex			/* -x */

		lis		pltemp1,0x4444				/* new version by CHS */
		ori		pltemp1,pltemp1,0x4400

		mfcr		pltemp0
		and		iflagand,iflagand,pltemp0
		and		iflagand,iflagand,pltemp1
		or		iflags,iflags,iflagand
		stw		iflags,BBoxList.flags_off(pbboxlist)
Lbboxloopend:		
		mr.		pbboxlist,pnextbbox
		bne+		Lbboxloopstart		

/*	Restore registers, return to C code */
		b		M_EndAsmCode

/*	Alter the point (fr24,fr25,fr26), fall through to visibilitysub */
Lvisibilitysubcasex:		
		fmadds		fr24,fr27,fr12,fr24
		fmadds		fr25,fr27,fr13,fr25			/* along x axis */
		fmadds		fr26,fr27,fr14,fr26
		fneg		fr27,fr27
/*	Check the point (fr24,fr25,fr26) vs the fulcrum, accumulate flags */
Lvisibilitysub:		
		mfcr		pltemp0
		and		iflagand,iflagand,pltemp0
Lvisibilitysubfirsttime:
		fmuls		fr31,fr02,fr26	/* (width * w) */
		fmuls		fr30,fr03,fr26	/* (height * w) */
		fcmpu		0,fr01,fr24	/* if 0.0  > fxpp0 */
		fcmpu		1,fr24,fr31	/* if fxpp0 > (width * w) */
		fcmpu		2,fr01,fr25	/* if 0.0  > fypp0 */
		fcmpu		3,fr25,fr30	/* if fypp0 > (height * w) */
		fcmpu		4,fr00,fr26	/* if hither > fwp0 */
		fcmpu		5,fr26,fr04	/* if fwp0 > yon */
		blr
endcode:
		.space		4096-(endcode-begincode)


/*	END */
	
