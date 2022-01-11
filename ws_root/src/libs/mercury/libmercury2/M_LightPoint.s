	.include	"M_LightCommon.i"
/*
 *	Point
 *	A point light source whose light decreases with distance.  The light comes
 *	from a point, and declines based on inverse distance squared.
 *	The light intensity is also multiplied by the cos of the angle of
 *	incidence; that is, if the surface points away, it is not illuminated, and
 *	the more the surface points toward the light, the brighter it is.
 *	The data it uses is:
 *		flx		fly		flz 		(point location)
 *		flintensity					(intensity)
 *		flred		flgreen		flblue 		(light rgb)
 *
 *
 */

LightPointInit:
		mflr		lksave
		mr		clsave,pconvlights
		bl		M_SetupPointEntry2
		lfs		fi_maxdist,12(plightdata)
		bl		M_CheckDist
		addi		pdata,plightdata,(16-4)
		li		copycount,1
		b		M_SetupRgbReturn

	struct	TransPnt
		stlong		TransPnt.Code	,	1
		stlong		TransPnt.x	,	1
		stlong		TransPnt.y	,	1
		stlong		TransPnt.z	,	1
		stlong		TransPnt.i	,	1
		stlong		TransPnt.r	,	1
		stlong		TransPnt.g	,	1
		stlong		TransPnt.b	,	1
		stlong		TransPnt.Next	,	1
	ends	TransPnt

		define		pnt_pntx	,	fltemp0
		define		pnt_pnty	,	fltemp1
		define		pnt_pntz	,	fltemp2

		define		pnt_deltax0	,	fltemp3
		define		pnt_deltay0	,	fltemp4
		define		pnt_deltaz0	,	fltemp5

		define		pnt_deltax1	,	fltemp0
		define		pnt_deltay1	,	fltemp1
		define		pnt_deltaz1	,	fltemp2

		define		pnt_dot0	,	fwp0
		define		pnt_dot1	,	fwp1

		define		pnt_sqrdist0	,	pnt_deltax0
		define		pnt_sqrdist1	,	pnt_deltax1
		define		pnt_invdist0	,	pnt_deltay0
		define		pnt_invdist1	,	pnt_deltay1
		define		pnt_invdistsqr0	,	pnt_deltaz0
		define		pnt_invdistsqr1	,	pnt_deltaz1
		define		pnt_invsqrdist0	,	fnz0
		define		pnt_invsqrdist1	,	fnx1

		define		pnt_0pt0	,	fnx0
		define		pnt_0pt5	,	fny0
		define		pnt_1pt0	,	fny0
		define		pnt_2pt0	,	fny0
		define		pnt_3pt0	,	fny0
		define		pnt_tiny	,	fny0

		define		pnt_intensity	,	fny0

		define		pnt_red		,	pnt_deltax0
		define		pnt_green	,	pnt_deltax1
		define		pnt_blue	,	pnt_deltay0

/*	Entry points for both routines */
		b		LightPointInit
	DECFN	M_LightPoint

		lfs		pnt_pntx,TransPnt.x(plightlist)
		lfs		pnt_pnty,TransPnt.y(plightlist)
		lfs		pnt_pntz,TransPnt.z(plightlist)

/*	save off variables to temp storage */

		stfs		fwp0,CloseData.flight0_off(pclosedata)
		stfs		fwp1,CloseData.flight1_off(pclosedata)
		stfs		fnx0,CloseData.flight2_off(pclosedata)

/*	calc deltas and dot products	*/

		fsubs		pnt_deltax0,pnt_pntx,fx0
		fsubs		pnt_deltax1,pnt_pntx,fx1
		fsubs		pnt_deltay0,pnt_pnty,fy0
		fmuls		pnt_dot0,pnt_deltax0,fnx0

		fmuls		pnt_dot1,pnt_deltax1,fnx1
		fsubs		pnt_deltay1,pnt_pnty,fy1
		fmadds		pnt_dot0,pnt_deltay0,fny0,pnt_dot0
		fsubs		pnt_deltaz0,pnt_pntz,fz0

		fmadds		pnt_dot1,pnt_deltay1,fny1,pnt_dot1
		fsubs		pnt_deltaz1,pnt_pntz,fz1
		fmadds		pnt_dot0,pnt_deltaz0,fnz0,pnt_dot0
		lfs		pnt_0pt0,CloseData.fconst0pt0_off(pclosedata)

		fmadds		pnt_dot1,pnt_deltaz1,fnz1,pnt_dot1

/*	cull if both verts are pointing away */

		fcmpu		0,pnt_dot0,pnt_0pt0
		fcmpu		1,pnt_dot1,pnt_0pt0
		bgt		0,.cont
		blt		1,.done
.cont:

/*	save off variables to temp storage */

		stfs		fny0,CloseData.flight3_off(pclosedata)
		stfs		fnz0,CloseData.flight4_off(pclosedata)
		stfs		fnx1,CloseData.flight5_off(pclosedata)

/*	sqrdist = deltax^2 + deltay^2 + deltaz^2	*/

		lfs		pnt_tiny,CloseData.fconstTINY_off(pclosedata)
		fmadds		pnt_sqrdist0,pnt_deltax0,pnt_deltax0,pnt_tiny
		fmadds		pnt_sqrdist1,pnt_deltax1,pnt_deltax1,pnt_tiny

		fmadds		pnt_sqrdist0,pnt_deltay0,pnt_deltay0,pnt_sqrdist0
		fmadds		pnt_sqrdist1,pnt_deltay1,pnt_deltay1,pnt_sqrdist1

		fmadds		pnt_sqrdist0,pnt_deltaz0,pnt_deltaz0,pnt_sqrdist0
		fmadds		pnt_sqrdist1,pnt_deltaz1,pnt_deltaz1,pnt_sqrdist1

/*	formula(1/dist^2)	invsqrdist = (sqrdist * sqrdist) + tiny		*/

		fmuls		pnt_invsqrdist0,pnt_sqrdist0,pnt_sqrdist0
		fmuls		pnt_invsqrdist1,pnt_sqrdist1,pnt_sqrdist1

/*	formula(1/dist)	invdist = 1/sqrt(sqrdist)			*/

		frsqrte		pnt_invdist0,pnt_sqrdist0
		frsqrte		pnt_invdist1,pnt_sqrdist1

/*	formula(1/dist^2)	invsqrdist = 1/sqrt(invsqrdist)			*/

		frsqrte		pnt_invsqrdist0,pnt_invsqrdist0
		frsqrte		pnt_invsqrdist1,pnt_invsqrdist1

/*	formula(1/dist)	invdistsqr = invdist * invdist			*/

		fmuls		pnt_invdistsqr0,pnt_invdist0,pnt_invdist0
		fmuls		pnt_invdistsqr1,pnt_invdist1,pnt_invdist1

/*	formula(1/dist)	invdistsqr = 3.0 - (invdistsqr * sqrdist)		*/

		lfs		pnt_3pt0,CloseData.fconst3pt0_off(pclosedata)
		fnmsubs		pnt_invdistsqr0,pnt_invdistsqr0,pnt_sqrdist0,pnt_3pt0
		fnmsubs		pnt_invdistsqr1,pnt_invdistsqr1,pnt_sqrdist1,pnt_3pt0

/*	formula(1/dist^2)	sqrdist = 2.0 - (invsqrdist * sqrdist)		*/

		lfs		pnt_2pt0,CloseData.fconst2pt0_off(pclosedata)
		fnmsubs		pnt_sqrdist0,pnt_invsqrdist0,pnt_sqrdist0,pnt_2pt0
		fnmsubs		pnt_sqrdist1,pnt_invsqrdist1,pnt_sqrdist1,pnt_2pt0

/*	formula(1/dist^2)	invsqrdist = invsqrdist * sqrdist		*/

		fmuls		pnt_invsqrdist0,pnt_invsqrdist0,pnt_sqrdist0
		fmuls		pnt_invsqrdist1,pnt_invsqrdist1,pnt_sqrdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * invdist		*/

		fmuls		pnt_invdistsqr0,pnt_invdistsqr0,pnt_invdist0
		fmuls		pnt_invdistsqr1,pnt_invdistsqr1,pnt_invdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * 0.5		*/

		lfs		pnt_0pt5,CloseData.fconst0pt5_off(pclosedata)
		fmuls		pnt_invdistsqr0,pnt_invdistsqr0,pnt_0pt5
		fmuls		pnt_invdistsqr1,pnt_invdistsqr1,pnt_0pt5

/*	calc intensity * invsqrdist * reflection	*/

		lfs		pnt_intensity,TransPnt.i(plightlist)
		fmuls		pnt_dot0,pnt_dot0,pnt_invdistsqr0
		fmuls		pnt_dot1,pnt_dot1,pnt_invdistsqr1

		fmuls		pnt_invsqrdist0,pnt_invsqrdist0,pnt_intensity
		fmuls		pnt_invsqrdist1,pnt_invsqrdist1,pnt_intensity

		fsel		pnt_dot0,pnt_dot0,pnt_dot0,pnt_0pt0
		fsel		pnt_dot1,pnt_dot1,pnt_dot1,pnt_0pt0

/*	clamp intensity to 1.0 */

		lfs		pnt_1pt0,CloseData.fconst1pt0_off(pclosedata)
		fsubs		fltemp0,pnt_invsqrdist0,pnt_1pt0
		fsubs		fltemp4,pnt_invsqrdist1,pnt_1pt0

		fsel		pnt_invsqrdist0,fltemp0,pnt_1pt0,pnt_invsqrdist0
		fsel		pnt_invsqrdist1,fltemp4,pnt_1pt0,pnt_invsqrdist1

/*	multiply by dot product */

		fmuls		pnt_dot0,pnt_dot0,pnt_invsqrdist0
		fmuls		pnt_dot1,pnt_dot1,pnt_invsqrdist1

		lfs		pnt_red,TransPnt.r(plightlist)
		lfs		pnt_green,TransPnt.g(plightlist)
		lfs		pnt_blue,TransPnt.b(plightlist)

		fmadds		fracc0,pnt_dot0,pnt_red,fracc0
		fmadds		fracc1,pnt_dot1,pnt_red,fracc1
		fmadds		fgacc0,pnt_dot0,pnt_green,fgacc0
		fmadds		fgacc1,pnt_dot1,pnt_green,fgacc1
		fmadds		fbacc0,pnt_dot0,pnt_blue,fbacc0
		fmadds		fbacc1,pnt_dot1,pnt_blue,fbacc1

/*	reload variables from temp storage */

		lfs		fny0,CloseData.flight3_off(pclosedata)
		lfs		fnz0,CloseData.flight4_off(pclosedata)
		lfs		fnx1,CloseData.flight5_off(pclosedata)
.done:
		lwzu		itemp0,TransPnt.Next(plightlist)

/*	reload variables from temp storage */

		lfs		fwp0,CloseData.flight0_off(pclosedata)
		lfs		fwp1,CloseData.flight1_off(pclosedata)
		lfs		fnx0,CloseData.flight2_off(pclosedata)
		mtlr		itemp0
		blr

