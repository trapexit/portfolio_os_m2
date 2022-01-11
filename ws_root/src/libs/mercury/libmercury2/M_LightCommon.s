	.include	"M_LightCommon.i"

/*	Light setup functions that are called before the main draw routine and shouldn't
	be packed w/ the real lighting code */

/*	Vectors are rotated using the upper 3x3 matrix -- the bottom three
 *	entries aren't used.
 */

	DECFN	M_SetupVectorEntry2
		mr		pdata,plightdata
	DECFN	M_SetupVector
		lfs		fi_x,0(pdata)
		lfs		fi_y,4(pdata)
		fmuls		fi_xprime,fi_x,finvmat00
		fmuls		fi_yprime,fi_x,finvmat01
		fmuls		fi_zprime,fi_x,finvmat02
	DECFN	M_SetupJoin
		fmadds		fi_xprime,fi_y,finvmat10,fi_xprime
		fmadds		fi_yprime,fi_y,finvmat11,fi_yprime
		lfs		fi_z,8(pdata)
		fmadds		fi_zprime,fi_y,finvmat12,fi_zprime
		fmadds		fi_xprime,fi_z,finvmat20,fi_xprime
		fmadds		fi_yprime,fi_z,finvmat21,fi_yprime
		fmadds		fi_zprime,fi_z,finvmat22,fi_zprime
		stfsu		fi_xprime,4(pconvlights)
		stfsu		fi_yprime,4(pconvlights)
		stfsu		fi_zprime,4(pconvlights)
		blr

/*	Points are rotated using the entire matrix, so the code needs
 *	a fmadds to begin with, not a fmuls
 */
	DECFN	M_SetupPointEntry2
		mr		pdata,plightdata
	DECFN	M_SetupPoint
		lfs		fi_x,0(pdata)
		lfs		fi_y,4(pdata)
		fmadds		fi_xprime,fi_x,finvmat00,finvmat30
		fmadds		fi_yprime,fi_x,finvmat01,finvmat31
		fmadds		fi_zprime,fi_x,finvmat02,finvmat32
		b		M_SetupJoin

/*	Return from the light code, but first copy the words that didn't
 *	need to be transformed (rgb values and others)
 */
	DECFN	M_StandardLightReturn
		mtctr		copycount
.Lloop:		
		lwzu		pltemp0,4(pdata)
		stwu		pltemp0,4(pconvlights)
		bdnz+		.Lloop		

		mtlr		lksave
		blr
/*
 *	multiply diffuse rgb times light rgb
 *	then return.
 */
	DECFN	M_SetupRgbReturn
		mtctr		copycount
.loop1:
		lwzu		pltemp0,4(pdata)
		stwu		pltemp0,4(pconvlights)
		bdnz+		.loop1

	DECFN	M_SetupRgbReturn2
		lfsu		fi_colr,4(pdata)
		lfsu		fi_colg,4(pdata)
		lfsu		fi_colb,4(pdata)
	DECFN	M_SetupRgbReturn3
		lfs		fi_colr2,CloseData.frdiffuse_off(pclosedata)
		lfs		fi_colg2,CloseData.fgdiffuse_off(pclosedata)
		lfs		fi_colb2,CloseData.fbdiffuse_off(pclosedata)
		fmuls		fi_colr,fi_colr,fi_colr2
		fmuls		fi_colg,fi_colg,fi_colg2
		fmuls		fi_colb,fi_colb,fi_colb2
		stfsu		fi_colr,4(pconvlights)
		stfsu		fi_colg,4(pconvlights)
		stfsu		fi_colb,4(pconvlights)

		mtlr		lksave
		blr
/*
 *	if dist^2 > (maxdist + extents^2) then don't bother lighting this object
 *	maxdist should be intensity * 256 * brightest color component for 32 bit color mode
 *	maxdist should be intensity * 32 * brightest color component for 16 bit color mode
 */

	DECFN	M_CheckDist
		lfs		fi_x,PodGeometry.xextent_off(pgeometry)
		lfs		fi_y,PodGeometry.yextent_off(pgeometry)
		lfs		fi_z,PodGeometry.zextent_off(pgeometry)
		fmuls		fi_dist,fi_xprime,fi_xprime
		fmadds		fi_dist,fi_yprime,fi_yprime,fi_dist
		fmadds		fi_dist,fi_zprime,fi_zprime,fi_dist
		fmadds		fi_maxdist,fi_x,fi_x,fi_maxdist
		fmadds		fi_maxdist,fi_y,fi_y,fi_maxdist
		fmadds		fi_maxdist,fi_z,fi_z,fi_maxdist
		fcmpu		0,fi_dist,fi_maxdist
		bltlr		0
		subi		pconvlights,clsave,4
		mtlr		lksave
		blr

/*	END  */
	
	
