	.include	"M_LightCommon.i"

/*
 *	Directional
 *
 *	One directional light source.  The data it uses is:
 *
 *		flnx	flny	flnz		(light normal)
 *		flred	flgreen	flblue 		(light rgb)
 *
 *	Calculates the negated dot product between the light normal and the
 *	vertex normal, clamps value to zero, then multiplies by the lights
 *	color and adds to the accumulators.
 */
LightDirInit:
		mflr		lksave
		bl		M_SetupVectorEntry2

		addi		pdata,pdata,(12-4)

		b		M_SetupRgbReturn2

	struct	TransDir
		stlong		TransDir.Code	,	1
		stlong		TransDir.nx	,	1
		stlong		TransDir.ny	,	1
		stlong		TransDir.nz	,	1
		stlong		TransDir.r	,	1
		stlong		TransDir.g	,	1
		stlong		TransDir.b	,	1
		stlong		TransDir.Next	,	1
	ends	TransDir

		define		dir_nx		,	fltemp0
		define		dir_ny		,	fltemp1
		define		dir_nz		,	fltemp2
		define		dir_dot0	,	fltemp3
		define		dir_dot1	,	fltemp4
		define		dir_fconst0pt0	,	fltemp5

		define		dir_red		,	fltemp0
		define		dir_green	,	fltemp1
		define		dir_blue	,	fltemp2

/*	Entry points for both routines */
		b		LightDirInit
	DECFN	M_LightDir

		lfs		dir_nx,TransDir.nx(plightlist)
		lfs		dir_ny,TransDir.ny(plightlist)
		lfs		dir_nz,TransDir.nz(plightlist)
		lfs		dir_fconst0pt0,CloseData.fconst0pt0_off(pclosedata)
		fmuls		dir_dot0,dir_nx,fnx0
		fmuls		dir_dot1,dir_nx,fnx1
		fmadds		dir_dot0,dir_ny,fny0,dir_dot0
		fmadds		dir_dot1,dir_ny,fny1,dir_dot1
		fnmadds		dir_dot0,dir_nz,fnz0,dir_dot0
		fnmadds		dir_dot1,dir_nz,fnz1,dir_dot1
		fsel		dir_dot0,dir_dot0,dir_dot0,dir_fconst0pt0
		fsel		dir_dot1,dir_dot1,dir_dot1,dir_fconst0pt0
		lfs		dir_red,TransDir.r(plightlist)
		lfs		dir_green,TransDir.g(plightlist)
		lfs		dir_blue,TransDir.b(plightlist)
		lwzu		itemp0,TransDir.Next(plightlist)
		mtlr		itemp0
		fmadds		fracc0,dir_dot0,dir_red,fracc0
		fmadds		fracc1,dir_dot1,dir_red,fracc1
		fmadds		fgacc0,dir_dot0,dir_green,fgacc0
		fmadds		fgacc1,dir_dot1,dir_green,fgacc1
		fmadds		fbacc0,dir_dot0,dir_blue,fbacc0
		fmadds		fbacc1,dir_dot1,dir_blue,fbacc1
		blr


