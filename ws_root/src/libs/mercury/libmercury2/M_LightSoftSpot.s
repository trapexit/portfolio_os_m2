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
 *		flnx		flny		flnz 		(point direction)
 *		flintensity					(intensity)
 *		flcos						(outer cos(angle))
 *		flinvcos					(1/(inner cos(angle)-(outer cos(angle)))
 *		flred		flgreen		flblue 		(light rgb)
 *
 *
 */

LightSoftSpotInit:
		mflr		lksave
		mr		clsave,pconvlights
		bl		M_SetupPointEntry2
		lfs		fi_maxdist,24(plightdata)
		bl		M_CheckDist
		addi		pdata,plightdata,12
		bl		M_SetupVector
		addi		pdata,plightdata,(28-4)
		li		copycount,3
		b		M_SetupRgbReturn

	struct	TransSpt
		stlong		TransSpt.Code	,	1
		stlong		TransSpt.x	,	1
		stlong		TransSpt.y	,	1
		stlong		TransSpt.z	,	1
		stlong		TransSpt.nx	,	1
		stlong		TransSpt.ny	,	1
		stlong		TransSpt.nz	,	1
		stlong		TransSpt.i	,	1
		stlong		TransSpt.cos	,	1
		stlong		TransSpt.invcos	,	1
		stlong		TransSpt.r	,	1
		stlong		TransSpt.g	,	1
		stlong		TransSpt.b	,	1
		stlong		TransSpt.Next	,	1
	ends	TransSpt

		define		spt_pntx	,	fltemp0
		define		spt_pnty	,	fltemp1
		define		spt_pntz	,	fltemp2

		define		spt_deltax0	,	fltemp3
		define		spt_deltay0	,	fltemp4
		define		spt_deltaz0	,	fltemp5

		define		spt_deltax1	,	fltemp0
		define		spt_deltay1	,	fltemp1
		define		spt_deltaz1	,	fltemp2

		define		spt_dot0	,	fwp0
		define		spt_dot1	,	fwp1
		define		spt_dot01	,	fny1
		define		spt_dot11	,	fnz1
		define		spt_dot02	,	spt_deltaz0
		define		spt_dot12	,	spt_deltaz1

		define		spt_dirx	,	fny0
		define		spt_diry	,	fnz0
		define		spt_dirz	,	fnx1

		define		spt_sqrdist0	,	spt_deltax0
		define		spt_sqrdist1	,	spt_deltax1
		define		spt_invdist0	,	spt_deltay0
		define		spt_invdist1	,	spt_deltay1
		define		spt_invdistsqr0	,	spt_deltaz0
		define		spt_invdistsqr1	,	spt_deltaz1
		define		spt_invsqrdist0	,	spt_diry
		define		spt_invsqrdist1	,	spt_dirz

		define		spt_0pt0	,	fnx0
		define		spt_0pt5	,	spt_dirx
		define		spt_1pt0	,	spt_dirx
		define		spt_2pt0	,	spt_dirx
		define		spt_3pt0	,	spt_dirx
		define		spt_tiny	,	spt_dirx

		define		spt_intensity	,	spt_dirx
		define		spt_angle	,	spt_dirx

		define		spt_red		,	spt_deltax0
		define		spt_green	,	spt_deltax1
		define		spt_blue	,	spt_deltay0

/*	Entry points for both routines */
		b		LightSoftSpotInit
	DECFN	M_LightSoftSpot

		lfs		spt_pntx,TransSpt.x(plightlist)
		lfs		spt_pnty,TransSpt.y(plightlist)
		lfs		spt_pntz,TransSpt.z(plightlist)

/*	save off variables to temp storage */

		stfs		fwp0,CloseData.flight0_off(pclosedata)
		stfs		fwp1,CloseData.flight1_off(pclosedata)
		stfs		fnx0,CloseData.flight2_off(pclosedata)

/*	calc deltas and dot products	*/

		fsubs		spt_deltax0,spt_pntx,fx0
		fsubs		spt_deltax1,spt_pntx,fx1
		fsubs		spt_deltay0,spt_pnty,fy0
		fmuls		spt_dot0,spt_deltax0,fnx0

		fmuls		spt_dot1,spt_deltax1,fnx1
		fsubs		spt_deltay1,spt_pnty,fy1
		fmadds		spt_dot0,spt_deltay0,fny0,spt_dot0
		fsubs		spt_deltaz0,spt_pntz,fz0

		fmadds		spt_dot1,spt_deltay1,fny1,spt_dot1
		fsubs		spt_deltaz1,spt_pntz,fz1
		fmadds		spt_dot0,spt_deltaz0,fnz0,spt_dot0
		lfs		spt_0pt0,CloseData.fconst0pt0_off(pclosedata)

		fmadds		spt_dot1,spt_deltaz1,fnz1,spt_dot1

/*	cull if both verts are pointing away */

		fcmpu		0,spt_dot0,spt_0pt0
		fcmpu		1,spt_dot1,spt_0pt0
		bgt		0,.cont
		blt		1,.done
.cont:

/*	save off variables to temp storage */

		stfs		fny0,CloseData.flight3_off(pclosedata)
		stfs		fnz0,CloseData.flight4_off(pclosedata)
		stfs		fnx1,CloseData.flight5_off(pclosedata)
		stfs		fny1,CloseData.flight6_off(pclosedata)
		stfs		fnz1,CloseData.flight7_off(pclosedata)

/*	calc dot product of cone	*/

		lfs		spt_dirx,TransSpt.nx(plightlist)
		lfs		spt_diry,TransSpt.ny(plightlist)
		lfs		spt_dirz,TransSpt.nz(plightlist)

		fmuls		spt_dot01,spt_deltax0,spt_dirx
		fmuls		spt_dot11,spt_deltax1,spt_dirx

		fmadds		spt_dot01,spt_deltay0,spt_diry,spt_dot01
		fmadds		spt_dot11,spt_deltay1,spt_diry,spt_dot11

		fnmadds		spt_dot01,spt_deltaz0,spt_dirz,spt_dot01
		fnmadds		spt_dot11,spt_deltaz1,spt_dirz,spt_dot11

/*	sqrdist = deltax^2 + deltay^2 + deltaz^2	*/

		fmuls		spt_sqrdist0,spt_deltax0,spt_deltax0
		fmuls		spt_sqrdist1,spt_deltax1,spt_deltax1
		lfs		spt_tiny,CloseData.fconstTINY_off(pclosedata)

		fmadds		spt_sqrdist0,spt_deltay0,spt_deltay0,spt_sqrdist0
		fmadds		spt_sqrdist1,spt_deltay1,spt_deltay1,spt_sqrdist1

		fmadds		spt_sqrdist0,spt_deltaz0,spt_deltaz0,spt_sqrdist0
		fmadds		spt_sqrdist1,spt_deltaz1,spt_deltaz1,spt_sqrdist1

/*	formula(1/dist^2)	invsqrdist = (sqrdist * sqrdist) + tiny		*/

		fmadds		spt_invsqrdist0,spt_sqrdist0,spt_sqrdist0,spt_tiny
		fmadds		spt_invsqrdist1,spt_sqrdist1,spt_sqrdist1,spt_tiny

/*	formula(1/dist)	invdist = 1/sqrt(sqrdist)			*/

		frsqrte		spt_invdist0,spt_sqrdist0
		frsqrte		spt_invdist1,spt_sqrdist1

/*	formula(1/dist^2)	invsqrdist = 1/sqrt(invsqrdist)			*/

		frsqrte		spt_invsqrdist0,spt_invsqrdist0
		frsqrte		spt_invsqrdist1,spt_invsqrdist1

/*	formula(1/dist)	invdistsqr = invdist * invdist			*/

		fmuls		spt_invdistsqr0,spt_invdist0,spt_invdist0
		fmuls		spt_invdistsqr1,spt_invdist1,spt_invdist1

/*	formula(1/dist)	invdistsqr = 3.0 - (invdistsqr * sqrdist)		*/

		lfs		spt_3pt0,CloseData.fconst3pt0_off(pclosedata)
		fnmsubs		spt_invdistsqr0,spt_invdistsqr0,spt_sqrdist0,spt_3pt0
		fnmsubs		spt_invdistsqr1,spt_invdistsqr1,spt_sqrdist1,spt_3pt0

/*	formula(1/dist^2)	sqrdist = 2.0 - (invsqrdist * sqrdist)		*/

		lfs		spt_2pt0,CloseData.fconst2pt0_off(pclosedata)
		fnmsubs		spt_sqrdist0,spt_invsqrdist0,spt_sqrdist0,spt_2pt0
		fnmsubs		spt_sqrdist1,spt_invsqrdist1,spt_sqrdist1,spt_2pt0

/*	formula(1/dist^2)	invsqrdist = invsqrdist * sqrdist		*/

		fmuls		spt_invsqrdist0,spt_invsqrdist0,spt_sqrdist0
		fmuls		spt_invsqrdist1,spt_invsqrdist1,spt_sqrdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * invdist		*/

		fmuls		spt_invdistsqr0,spt_invdistsqr0,spt_invdist0
		fmuls		spt_invdistsqr1,spt_invdistsqr1,spt_invdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * 0.5		*/

		lfs		spt_0pt5,CloseData.fconst0pt5_off(pclosedata)
		fmuls		spt_invdistsqr0,spt_invdistsqr0,spt_0pt5
		fmuls		spt_invdistsqr1,spt_invdistsqr1,spt_0pt5

/*	calc intensity * invsqrdist * reflection * spot cone	*/

		lfs		spt_angle,TransSpt.cos(plightlist)
		fmuls		spt_dot0,spt_dot0,spt_invdistsqr0
		fmuls		spt_dot1,spt_dot1,spt_invdistsqr1
		fmsubs		spt_dot01,spt_dot01,spt_invdistsqr0,spt_angle
		fmsubs		spt_dot11,spt_dot11,spt_invdistsqr1,spt_angle

		lfs		spt_intensity,TransSpt.i(plightlist)
		fmuls		spt_invsqrdist0,spt_invsqrdist0,spt_intensity
		fmuls		spt_invsqrdist1,spt_invsqrdist1,spt_intensity
		fsel		spt_dot01,spt_dot01,spt_dot01,spt_0pt0
		fsel		spt_dot11,spt_dot11,spt_dot11,spt_0pt0

		lfs		spt_angle,TransSpt.invcos(plightlist)
		fsel		spt_dot0,spt_dot0,spt_dot0,spt_0pt0
		fsel		spt_dot1,spt_dot1,spt_dot1,spt_0pt0
		fmuls		spt_dot01,spt_dot01,spt_angle
		fmuls		spt_dot11,spt_dot11,spt_angle

		lfs		spt_1pt0,CloseData.fconst1pt0_off(pclosedata)
		fsubs		spt_dot02,spt_invsqrdist0,spt_1pt0
		fsubs		spt_dot12,spt_invsqrdist1,spt_1pt0
		fsel		spt_invsqrdist0,spt_dot02,spt_1pt0,spt_invsqrdist0
		fsel		spt_invsqrdist1,spt_dot12,spt_1pt0,spt_invsqrdist1

		fsubs		spt_dot02,spt_dot01,spt_1pt0
		fsubs		spt_dot12,spt_dot11,spt_1pt0
		fsel		spt_dot01,spt_dot02,spt_1pt0,spt_dot01
		fsel		spt_dot11,spt_dot12,spt_1pt0,spt_dot11

		fmuls		spt_dot0,spt_dot0,spt_invsqrdist0
		fmuls		spt_dot1,spt_dot1,spt_invsqrdist1
		fmuls		spt_dot0,spt_dot0,spt_dot01
		fmuls		spt_dot1,spt_dot1,spt_dot11

		lfs		spt_red,TransSpt.r(plightlist)
		lfs		spt_green,TransSpt.g(plightlist)
		lfs		spt_blue,TransSpt.b(plightlist)

		fmadds		fracc0,spt_dot0,spt_red,fracc0
		fmadds		fgacc0,spt_dot0,spt_green,fgacc0
		fmadds		fbacc0,spt_dot0,spt_blue,fbacc0

		fmadds		fracc1,spt_dot1,spt_red,fracc1
		fmadds		fgacc1,spt_dot1,spt_green,fgacc1
		fmadds		fbacc1,spt_dot1,spt_blue,fbacc1

/*	reload variables from temp storage */

		lfs		fny0,CloseData.flight3_off(pclosedata)
		lfs		fnz0,CloseData.flight4_off(pclosedata)
		lfs		fnx1,CloseData.flight5_off(pclosedata)
		lfs		fny1,CloseData.flight6_off(pclosedata)
		lfs		fnz1,CloseData.flight7_off(pclosedata)
.done:
		lwzu		itemp0,TransSpt.Next(plightlist)

/*	reload variables from temp storage */

		lfs		fwp0,CloseData.flight0_off(pclosedata)
		lfs		fwp1,CloseData.flight1_off(pclosedata)
		lfs		fnx0,CloseData.flight2_off(pclosedata)
		mtlr		itemp0
		blr

