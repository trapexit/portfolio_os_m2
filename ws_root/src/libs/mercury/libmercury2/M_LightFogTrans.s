	.include	"M_LightCommon.i"
/*
 *	FogTrans
 *	
 *	The data it uses is:
 *	
 *		fog0	fog1			(fog constants)
 *
 *	The fog is a linear function on w, where 1.0 shows
 *	the object fully and 0.0 is completely fogged.
 *
 *	If the fog is 1 at fognear and 0 at fogfar, then:
 *	
 *		Dist1 = hither/(fogfar - fognear)
 *		Dist2 = fognear/(fogfar - fognear)
 *	
 *		prelim value = Dist1 * w - Dist2, which is 0 at fognear and 1 at fogfar
 *		clamp prelim value to 0.0
 *		ucoord -= prelim value
 */			
LightFogTransInit:
		mflr		lksave
		subi		pdata,plightdata,4
		li		copycount,2

		b		M_StandardLightReturn

	struct	TransFogU
		stlong		TransFogU.Code	,	1
		stlong		TransFogU.Dist1	,	1
		stlong		TransFogU.Dist2	,	1
		stlong		TransFogU.Next	,	1
	ends	TransFogU

		define		fog_dist1	,	fltemp0
		define		fog_dist2	,	fltemp1
		define		fog_0pt0	,	fltemp2
		define		fog_prelim0	,	fltemp3
		define		fog_prelim1	,	fltemp4

/*	Entry points for both routines */
		b		LightFogTransInit
	DECFN	M_LightFogTrans

		lfs		fog_dist1,TransFogU.Dist1(plightlist)
		lfs		fog_dist2,TransFogU.Dist2(plightlist)
		lwzu		itemp0,TransFogU.Next(plightlist)
		lfs		fog_0pt0,CloseData.fconst0pt0_off(pclosedata)
		mtlr		itemp0
		fmsubs		fog_prelim0,fog_dist1,fwp0,fog_dist2
		fmsubs		fog_prelim1,fog_dist1,fwp1,fog_dist2
		fsel		fog_prelim0,fog_prelim0,fog_prelim0,fog_0pt0
		fsel		fog_prelim1,fog_prelim1,fog_prelim1,fog_0pt0
		fsubs		fu0,fu0,fog_prelim0
		fsubs		fu1,fu1,fog_prelim1
		fsel		fu0,fu0,fu0,fog_0pt0
		fsel		fu1,fu1,fu1,fog_0pt0
		blr


