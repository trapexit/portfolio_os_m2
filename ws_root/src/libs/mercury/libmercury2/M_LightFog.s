	.include	"M_LightCommon.i"
/*
 *	Fog
 *	
 *	The data it uses is:
 *	
 *		Dist1	Dist2			(fog constants)
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
 *		alpha -= prelim value
 */			
LightFogInit:
		mflr		lksave
		subi		pdata,plightdata,4
		li		copycount,2

		b		M_StandardLightReturn

	struct	TransFog
		stlong		TransFog.Code	,	1
		stlong		TransFog.Dist1	,	1
		stlong		TransFog.Dist2	,	1
		stlong		TransFog.Next	,	1
	ends	TransFog

		define		fog_dist1	,	fltemp0
		define		fog_dist2	,	fltemp1
		define		fog_0pt0	,	fltemp2
		define		fog_prelim0	,	fltemp3
		define		fog_prelim1	,	fltemp4

/*	Entry points for both routines */
		b		LightFogInit
	DECFN	M_LightFog

		lfs		fog_dist1,TransFog.Dist1(plightlist)
		lfs		fog_dist2,TransFog.Dist2(plightlist)
		lwzu		itemp0,TransFog.Next(plightlist)
		lfs		fog_0pt0,CloseData.fconst0pt0_off(pclosedata)
		mtlr		itemp0
		fmsubs		fog_prelim0,fog_dist1,fwp0,fog_dist2
		fmsubs		fog_prelim1,fog_dist1,fwp1,fog_dist2
		fsel		fog_prelim0,fog_prelim0,fog_prelim0,fog_0pt0
		fsel		fog_prelim1,fog_prelim1,fog_prelim1,fog_0pt0
		fsubs		faacc0,faacc0,fog_prelim0
		fsubs		faacc1,faacc1,fog_prelim1
		fsel		faacc0,faacc0,faacc0,fog_0pt0
		fsel		faacc1,faacc1,faacc1,fog_0pt0
		blr


