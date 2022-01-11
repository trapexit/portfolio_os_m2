	.include	"M_LightCommon.i"
	.include	"M_LightSpec.i"

/*
 *	Directional with specular for textured surfaces
 *
 *	One directional light source and fog.  The data it uses is:
 *
 *		flnx	flny	flnz		(light normal)
 *		flred	flgreen	flblue 		(light rgb)
 *
 *	The light is a negated dot product between the light normal and the
 *	vertex normal.
 */
LightDirSpecTexInit:
		mflr		lksave
		bl		M_SetupVectorEntry2

		addi		pdata,pdata,(12-4)

		lwz		itemp0,CloseData.matflags_off(pclosedata)
		cmpi		0,itemp0,specdatadefinedFLAG

		beq		0,dont_initialize_spec

	/* Compute specular parameters only once */
		bl		M_ComputeSpecDataA
		
dont_initialize_spec:	
	/* Copy spec data from the material */
		li		0,10
		mtctr		0
		lwz		itemp0,Pod.pmaterial_off(ppod)
		addi		itemp0,itemp0,Material.specdata_off-4		
.loop1:
		lwzu		pltemp0,4(itemp0)
		stwu		pltemp0,4(pconvlights)
		bdnz+		.loop1

		lfs		fi_colr2,CloseData.frspecular_off(pclosedata)
		lfs		fi_colg2,CloseData.fgspecular_off(pclosedata)
		lfs		fi_colb2,CloseData.fbspecular_off(pclosedata)
		lfs		fi_const255,CloseData.fconst255pt0_off(pclosedata)
		fmuls		fi_colr,fi_colr2,fi_const255
		fmuls		fi_colg,fi_colg2,fi_const255
		fmuls		fi_colb,fi_colb2,fi_const255

		lfs		fi_const0,CloseData.fconst0pt0_off(pclosedata)
		fctiwz		fi_colr,fi_colr
		fctiwz		fi_colg,fi_colg
		fctiwz		fi_colb,fi_colb
		li		intcolindex,CloseData.clipdata_off
		stfs		fi_const0,CloseData.fabase_off(pclosedata)
		stfiwx		fi_colr,intcolindex,pclosedata /* use as temp storage */
		addi		intcolindex,intcolindex,4
		stfiwx		fi_colg,intcolindex,pclosedata /* use as temp storage */
		addi		intcolindex,intcolindex,4
		stfiwx		fi_colb,intcolindex,pclosedata /* use as temp storage */

		addi		intcolindex,intcolindex,4


		lwz		intcolr,CloseData.clipdata_off(pclosedata)
		lwz		intcolg,CloseData.clipdata_off+4(pclosedata)
		lwz		intcolb,CloseData.clipdata_off+8(pclosedata)
		rlwimi		intcolb,intcolg,8,16,23
		rlwimi		intcolb,intcolr,16,8,15
/* write sync command */
		lis		intcolr,0x1000
		ori		intcolr,intcolr,0x14
		stwu		intcolr,4(pVI)
		li		intcolr,0x20
		stwu		intcolr,4(pVI)
/* write to dest blend register command */
		lis		intcolr,0x1000
		ori		intcolr,intcolr,0x8054
		stwu		intcolr,4(pVI)		
/* store colors */
		stwu		intcolb,4(pVI)
		cmpl		0,pVI,pVIwritemax
		bgel-		M_ClistManager
		lfs		fi_colr,4(pdata)
		lfs		fi_colg,8(pdata)
		lfs		fi_colb,12(pdata)
		lfs		fi_colr2,CloseData.frspecular_off(pclosedata)
		lfs		fi_colg2,CloseData.fgspecular_off(pclosedata)
		lfs		fi_colb2,CloseData.fbspecular_off(pclosedata)
		fmuls		fi_colr2,fi_colr,fi_colr2
		fmuls		fi_colg2,fi_colg,fi_colg2
		fmuls		fi_colb2,fi_colb,fi_colb2
		stfsu		fi_colr2,4(pconvlights)
		stfsu		fi_colg2,4(pconvlights)
		stfsu		fi_colb2,4(pconvlights)
		b		M_SetupRgbReturn3

		define		ds_nx		,	fltemp0
		define		ds_ny		,	fltemp1
		define		ds_nz		,	fltemp2

		define		ds_dot0		,	fwp0
		define		ds_dot1		,	fwp1

		define		ds_0pt0		,	fltemp5
		define		ds_0pt5		,	fltemp5
		define		ds_1pt0		,	fltemp5
		define		ds_2pt0		,	fltemp5
		define		ds_3pt0		,	fltemp5
		define		ds_tiny		,	fltemp5

		define		ds_camx		,	fltemp3
		define		ds_camy		,	fltemp4
		define		ds_camz		,	fltemp5

		define		ds_deltax0	,	fx0
		define		ds_deltay0	,	fy0
		define		ds_deltaz0	,	fz0

		define		ds_deltax1	,	fx1
		define		ds_deltay1	,	fy1
		define		ds_deltaz1	,	fz1

		define		ds_dot00	,	fltemp3
		define		ds_dot10	,	fltemp4

		define		ds_2dot0	,	fltemp5
		define		ds_2dot1	,	fltemp5

		define		ds_nx0		,	fnx0
		define		ds_ny0		,	fny0
		define		ds_nz0		,	fnz0

		define		ds_nx1		,	fnx1
		define		ds_ny1		,	fny1
		define		ds_nz1		,	fnz1

		define		ds_dot01	,	fnx0
		define		ds_dot11	,	fnx1

		define		ds_sqrdist0	,	fx0
		define		ds_sqrdist1	,	fx1
		define		ds_invdist0	,	fy0
		define		ds_invdist1	,	fy1
		define		ds_invdistsqr0	,	fz0
		define		ds_invdistsqr1	,	fz1

		define		ds_alpha0	,	fltemp0
		define		ds_alpha1	,	fltemp1

		define		ds_red		,	fltemp0
		define		ds_green	,	fltemp1
		define		ds_blue		,	fltemp2

		define		ds_out0		,	fltemp5
		define		ds_out1		,	spec_out

/*	Entry points for both routines */
		b		LightDirSpecTexInit
	DECFN	M_LightDirSpecTex

		lfs		ds_nx,DirSpec.nx(plightlist)
		lfs		ds_ny,DirSpec.ny(plightlist)
		lfs		ds_nz,DirSpec.nz(plightlist)

/*	save off variables to temp storage */

		stfs		fx0,CloseData.flight0_off(pclosedata)
		stfs		fy0,CloseData.flight1_off(pclosedata)
		stfs		fz0,CloseData.flight2_off(pclosedata)
		stfs		fnx0,CloseData.flight3_off(pclosedata)
		stfs		fny0,CloseData.flight4_off(pclosedata)
		stfs		fnz0,CloseData.flight5_off(pclosedata)
		stfs		fwp0,CloseData.flight6_off(pclosedata)
		stfs		fx1,CloseData.flight7_off(pclosedata)
		stfs		fy1,CloseData.flight8_off(pclosedata)
		stfs		fz1,CloseData.flight9_off(pclosedata)
		stfs		fnx1,CloseData.flight10_off(pclosedata)
		stfs		fny1,CloseData.flight11_off(pclosedata)
		stfs		fnz1,CloseData.flight12_off(pclosedata)
		stfs		fwp1,CloseData.flight13_off(pclosedata)
/*	dot = -(N.L) since L is negative */
		fmuls		ds_dot0,ds_nx,fnx0
		fmuls		ds_dot1,ds_nx,fnx1
		fmadds		ds_dot0,ds_ny,fny0,ds_dot0
		fmadds		ds_dot1,ds_ny,fny1,ds_dot1
		fnmadds		ds_dot0,ds_nz,fnz0,ds_dot0
		fnmadds		ds_dot1,ds_nz,fnz1,ds_dot1

/*
 *	Camera Vector Vc = P-Cam
 */
		lfs		ds_camx,CloseData.flocalcamx_off(pclosedata)
		lfs		ds_camy,CloseData.flocalcamy_off(pclosedata)
		lfs		ds_camz,CloseData.flocalcamz_off(pclosedata)
		fsubs		ds_deltax0,fx0,ds_camx
		fsubs		ds_deltax1,fx1,ds_camx
		fsubs		ds_deltay0,fy0,ds_camy
		fsubs		ds_deltay1,fy1,ds_camy
		fsubs		ds_deltaz0,fz0,ds_camz
		fsubs		ds_deltaz1,fz1,ds_camz

/*
 *	Specular tune is - N . Vc
 */
		fmuls		ds_dot00,ds_deltax0,fnx0
		fmuls		ds_dot10,ds_deltax1,fnx1
		fmadds		ds_dot00,ds_deltay0,fny0,ds_dot00
		fmadds		ds_dot10,ds_deltay1,fny1,ds_dot10
		fnmadds		ds_dot00,ds_deltaz0,fnz0,ds_dot00
		fnmadds		ds_dot10,ds_deltaz1,fnz1,ds_dot10

/*
 *	Reflection is R = L-2(N.L)N = L + 2 dot.N
 */
		lfs		ds_2pt0,CloseData.fconst2pt0_off(pclosedata)
		fmuls		ds_2dot0,ds_2pt0,ds_dot0

		fmadds		ds_nx0,ds_2dot0,fnx0,ds_nx
		fmadds		ds_ny0,ds_2dot0,fny0,ds_ny
		fmadds		ds_nz0,ds_2dot0,fnz0,ds_nz

		lfs		ds_2pt0,CloseData.fconst2pt0_off(pclosedata)
		fmuls		ds_2dot1,ds_2pt0,ds_dot1

		fmadds		ds_nx1,ds_2dot1,fnx1,ds_nx
		fmadds		ds_ny1,ds_2dot1,fny1,ds_ny
		fmadds		ds_nz1,ds_2dot1,fnz1,ds_nz

/*
 *	Specular contribution is - R . Vc
 */
		fmuls		ds_dot01,ds_deltax0,ds_nx0
		fmuls		ds_dot11,ds_deltax1,ds_nx1

		fmadds		ds_dot01,ds_deltay0,ds_ny0,ds_dot01
		fmadds		ds_dot11,ds_deltay1,ds_ny1,ds_dot11

		fnmadds		ds_dot01,ds_deltaz0,ds_nz0,ds_dot01
		fnmadds		ds_dot11,ds_deltaz1,ds_nz1,ds_dot11

 		lfs		ds_0pt0,CloseData.fconst0pt0_off(pclosedata)

 		fsel		ds_dot01,ds_dot0,ds_dot01,ds_0pt0
 		fsel		ds_dot11,ds_dot1,ds_dot11,ds_0pt0

/*	cull if both verts are pointing away */
		fcmpu		0,ds_dot01,ds_0pt0
		fcmpu		1,ds_dot11,ds_0pt0
		bgt		0,.cont
		ble		1,.skip
.cont:

		fsel		ds_dot0,ds_dot0,ds_dot0,ds_0pt0
		fsel		ds_dot1,ds_dot1,ds_dot1,ds_0pt0

		fsel		ds_dot00,ds_dot00,ds_dot00,ds_0pt0
		fsel		ds_dot10,ds_dot10,ds_dot10,ds_0pt0

		fsel		ds_dot01,ds_dot01,ds_dot01,ds_0pt0
		fsel		ds_dot11,ds_dot11,ds_dot11,ds_0pt0


/*	sqrdist = deltax^2 + deltay^2 + deltaz^2	*/

 		lfs		ds_tiny,CloseData.fconstTINY_off(pclosedata)
		fmadds		ds_sqrdist0,ds_deltax0,ds_deltax0,ds_tiny
		fmadds		ds_sqrdist1,ds_deltax1,ds_deltax1,ds_tiny

		fmadds		ds_sqrdist0,ds_deltay0,ds_deltay0,ds_sqrdist0
		fmadds		ds_sqrdist1,ds_deltay1,ds_deltay1,ds_sqrdist1

		fmadds		ds_sqrdist0,ds_deltaz0,ds_deltaz0,ds_sqrdist0
		fmadds		ds_sqrdist1,ds_deltaz1,ds_deltaz1,ds_sqrdist1

/*	formula(1/dist)	invdist = 1/sqrt(sqrdist)			*/

		frsqrte		ds_invdist0,ds_sqrdist0
		frsqrte		ds_invdist1,ds_sqrdist1

/*	formula(1/dist)	invdistsqr = invdist * invdist			*/

		fmuls		ds_invdistsqr0,ds_invdist0,ds_invdist0
		fmuls		ds_invdistsqr1,ds_invdist1,ds_invdist1

/*	formula(1/dist)	invdistsqr = 3.0 - (invdistsqr * sqrdist)		*/

		lfs		ds_3pt0,CloseData.fconst3pt0_off(pclosedata)
		fnmsubs		ds_invdistsqr0,ds_invdistsqr0,ds_sqrdist0,ds_3pt0
		fnmsubs		ds_invdistsqr1,ds_invdistsqr1,ds_sqrdist1,ds_3pt0

/*	formula(1/dist)	invdistsqr = invdistsqr * invdist		*/

		fmuls		ds_invdistsqr0,ds_invdistsqr0,ds_invdist0
		fmuls		ds_invdistsqr1,ds_invdistsqr1,ds_invdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * 0.5		*/

		lfs		ds_0pt5,CloseData.fconst0pt5_off(pclosedata)
		fmuls		ds_invdistsqr0,ds_invdistsqr0,ds_0pt5
		fmuls		ds_invdistsqr1,ds_invdistsqr1,ds_0pt5

/*	normalize dot products					*/

		fmuls		ds_dot00,ds_dot00,ds_invdistsqr0
		fmuls		ds_dot10,ds_dot10,ds_invdistsqr1

		fmuls		spec_in,ds_dot01,ds_invdistsqr0
		fmuls		ds_dot11,ds_dot11,ds_invdistsqr1

		lfs		spec_const1,CloseData.fconst1pt0_off(pclosedata)
		lfs		spec_const0,CloseData.fconst0pt0_off(pclosedata)

		bl		M_LightSpec
		fmr		ds_out0,spec_out
		fmr		spec_in,ds_dot11
		bl		M_LightSpec		

		fmuls		ds_out0,ds_out0,ds_dot00
		fmuls		spec_out,spec_out,ds_dot10

		fadds		faacc0,faacc0,ds_out0
		fadds		faacc1,faacc1,spec_out

		fsubs		ds_alpha0,faacc0,spec_const1
		fsubs		ds_alpha1,faacc1,spec_const1
		fsel		faacc0,ds_alpha0,spec_const1,faacc0
		fsel		faacc1,ds_alpha1,spec_const1,faacc1
		b		skipskip
.skip:
		fsel		ds_dot0,ds_dot0,ds_dot0,ds_0pt0
		fsel		ds_dot1,ds_dot1,ds_dot1,ds_0pt0
skipskip:	
		lfs		ds_red,DirSpec.r(plightlist)
		lfs		ds_green,DirSpec.g(plightlist)
		lfs		ds_blue,DirSpec.b(plightlist)
		lwzu		itemp0,DirSpec.Next(plightlist)
		mtlr		itemp0
		fmadds		fracc0,ds_dot0,ds_red,fracc0
		fmadds		fracc1,ds_dot1,ds_red,fracc1
		fmadds		fgacc0,ds_dot0,ds_green,fgacc0
		fmadds		fgacc1,ds_dot1,ds_green,fgacc1
		fmadds		fbacc0,ds_dot0,ds_blue,fbacc0
		fmadds		fbacc1,ds_dot1,ds_blue,fbacc1

/*	reload variables from temp storage */

		lfs		fx0,CloseData.flight0_off(pclosedata)
		lfs		fy0,CloseData.flight1_off(pclosedata)
		lfs		fz0,CloseData.flight2_off(pclosedata)
		lfs		fnx0,CloseData.flight3_off(pclosedata)
		lfs		fny0,CloseData.flight4_off(pclosedata)
		lfs		fnz0,CloseData.flight5_off(pclosedata)
		lfs		fwp0,CloseData.flight6_off(pclosedata)
		lfs		fx1,CloseData.flight7_off(pclosedata)
		lfs		fy1,CloseData.flight8_off(pclosedata)
		lfs		fz1,CloseData.flight9_off(pclosedata)
		lfs		fnx1,CloseData.flight10_off(pclosedata)
		lfs		fny1,CloseData.flight11_off(pclosedata)
		lfs		fnz1,CloseData.flight12_off(pclosedata)
		lfs		fwp1,CloseData.flight13_off(pclosedata)
		blr


M_ClistManager:
		lea	3,M_ClistManagerC
		b	M_SaveAndCallC

