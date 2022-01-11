	.include	"M_LightCommon.i"

/*
 *	Environment mapping
 *
 *	One directional light source.  The data it uses is:
 *
 *		none
 *
 *	Calculates the reflection vector in local object coordinates
 *	from the camera around vertex normal and then rotates that
 *	vector into world coordinates.
 *	u = unit_atan(x,z)
 *	v = (0.5 * y) + 0.5
 */
LightEnvInit:
		stfsu		fr12,4(pconvlights)
		stfsu		fr13,4(pconvlights)
		stfsu		fr14,4(pconvlights)
		stfsu		fr15,4(pconvlights)
		stfsu		fr16,4(pconvlights)
		stfsu		fr17,4(pconvlights)
		stfsu		fr18,4(pconvlights)
		stfsu		fr19,4(pconvlights)
		stfsu		fr20,4(pconvlights)
		blr

	struct	Environ
		stlong		Environ.Code	,	1
		stlong		Environ.mat00	,	1
		stlong		Environ.mat01	,	1
		stlong		Environ.mat02	,	1
		stlong		Environ.mat10	,	1
		stlong		Environ.mat11	,	1
		stlong		Environ.mat12	,	1
		stlong		Environ.mat20	,	1
		stlong		Environ.mat21	,	1
		stlong		Environ.mat22	,	1
		stlong		Environ.Next	,	1
	ends	Environ


		define		env_camx	,	fltemp0
		define		env_camy	,	fltemp1
		define		env_camz	,	fltemp2

		define		env_deltax0	,	fx0
		define		env_deltay0	,	fy0
		define		env_deltaz0	,	fz0
		define		env_deltax1	,	fx1
		define		env_deltay1	,	fy1
		define		env_deltaz1	,	fz1

		define		env_nx0		,	fx0
		define		env_ny0		,	fy0
		define		env_nz0		,	fz0
		define		env_nx1		,	fx1
		define		env_ny1		,	fy1
		define		env_nz1		,	fz1

		define		env_sqrdist0	,	fltemp0
		define		env_sqrdist1	,	fltemp1
		define		env_invdist0	,	fltemp2
		define		env_invdist1	,	fltemp3
		define		env_invdistsqr0	,	fltemp4
		define		env_invdistsqr1	,	fltemp5

		define		env_dot0	,	fltemp0
		define		env_dot1	,	fltemp1

		define		env_tiny	,	fwp0
		define		env_0pt0	,	fwp0
		define		env_0pt25	,	fltemp0
		define		env_0pt5	,	fwp1
		define		env_0pt75	,	fltemp1
		define		env_1pt0	,	fltemp2
		define		env_2pt0	,	fwp0
		define		env_3pt0	,	fwp0
		define		env_4pt0	,	fwp0

		define		env_mat00	,	fnx0
		define		env_mat01	,	fny0
		define		env_mat02	,	fnz0
		define		env_mat10	,	fnx1
		define		env_mat11	,	fny1
		define		env_mat12	,	fnz1
		define		env_mat20	,	fnx0
		define		env_mat21	,	fny0
		define		env_mat22	,	fnz0

		define		env_asin0	,	fx0
		define		env_asin1	,	fy0
		define		env_asin2	,	fz0
		define		env_asin3	,	fx1

/*	Entry points for both routines */
		b		LightEnvInit
	DECFN	M_LightEnv

		lfs		env_camx,CloseData.flocalcamx_off(pclosedata)
		lfs		env_camy,CloseData.flocalcamy_off(pclosedata)
		lfs		env_camz,CloseData.flocalcamz_off(pclosedata)

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

		fsubs		env_deltax0,env_camx,fx0
		fsubs		env_deltay0,env_camy,fy0
		fsubs		env_deltaz0,env_camz,fz0
		fsubs		env_deltax1,env_camx,fx1
		fsubs		env_deltay1,env_camy,fy1
		fsubs		env_deltaz1,env_camz,fz1

/*	sqrdist = deltax^2 + deltay^2 + deltaz^2	*/

		lfs		env_tiny,CloseData.fconstTINY_off(pclosedata)
		fmadds		env_sqrdist0,env_deltax0,env_deltax0,env_tiny
		fmadds		env_sqrdist1,env_deltax1,env_deltax1,env_tiny

		fmadds		env_sqrdist0,env_deltay0,env_deltay0,env_sqrdist0
		fmadds		env_sqrdist1,env_deltay1,env_deltay1,env_sqrdist1

		fmadds		env_sqrdist0,env_deltaz0,env_deltaz0,env_sqrdist0
		fmadds		env_sqrdist1,env_deltaz1,env_deltaz1,env_sqrdist1

/*	formula(1/dist)	invdist = 1/sqrt(sqrdist)			*/

		frsqrte		env_invdist0,env_sqrdist0
		frsqrte		env_invdist1,env_sqrdist1

/*	formula(1/dist)	invdistsqr = invdist * invdist			*/

		fmuls		env_invdistsqr0,env_invdist0,env_invdist0
		fmuls		env_invdistsqr1,env_invdist1,env_invdist1

/*	formula(1/dist)	invdistsqr = 3.0 - (invdistsqr * sqrdist)	*/

		lfs		env_3pt0,CloseData.fconst3pt0_off(pclosedata)
		fnmsubs		env_invdistsqr0,env_invdistsqr0,env_sqrdist0,env_3pt0
		fnmsubs		env_invdistsqr1,env_invdistsqr1,env_sqrdist1,env_3pt0

/*	formula(1/dist)	invdistsqr = invdistsqr * invdist		*/

		fmuls		env_invdistsqr0,env_invdistsqr0,env_invdist0
		fmuls		env_invdistsqr1,env_invdistsqr1,env_invdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * 0.5			*/

		lfs		env_0pt5,CloseData.fconst0pt5_off(pclosedata)
		fmuls		env_invdistsqr0,env_invdistsqr0,env_0pt5
		fmuls		env_invdistsqr1,env_invdistsqr1,env_0pt5

/*	normalize deltax, deltay, and deltaz					*/

		fmuls		env_nx0,env_deltax0,env_invdistsqr0
		fmuls		env_ny0,env_deltay0,env_invdistsqr0
		fmuls		env_nz0,env_deltaz0,env_invdistsqr0
		fmuls		env_nx1,env_deltax1,env_invdistsqr1
		fmuls		env_ny1,env_deltay1,env_invdistsqr1
		fmuls		env_nz1,env_deltaz1,env_invdistsqr1

/*	reflection = 2(N.L)N-L						*/

		fmuls		env_dot0,env_nx0,fnx0
		fmuls		env_dot1,env_nx1,fnx1
		fmadds		env_dot0,env_ny0,fny0,env_dot0
		fmadds		env_dot1,env_ny1,fny1,env_dot1
		fmadds		env_dot0,env_nz0,fnz0,env_dot0
		fmadds		env_dot1,env_nz1,fnz1,env_dot1

		lfs		env_2pt0,CloseData.fconst2pt0_off(pclosedata)
		fmuls		env_dot0,env_2pt0,env_dot0
		fmuls		env_dot1,env_2pt0,env_dot1

		fmsubs		env_nx0,env_dot0,fnx0,env_nx0
		fmsubs		env_ny0,env_dot0,fny0,env_ny0
		fmsubs		env_nz0,env_dot0,fnz0,env_nz0
		fmsubs		env_nx1,env_dot1,fnx1,env_nx1
		fmsubs		env_ny1,env_dot1,fny1,env_ny1
		fmsubs		env_nz1,env_dot1,fnz1,env_nz1

/*	rotate reflection into global coordinates				*/

		lfs		env_mat00,Environ.mat00(plightlist)
		lfs		env_mat01,Environ.mat01(plightlist)
		lfs		env_mat02,Environ.mat02(plightlist)
		lfs		env_mat10,Environ.mat10(plightlist)
		lfs		env_mat11,Environ.mat11(plightlist)
		lfs		env_mat12,Environ.mat12(plightlist)
		fmuls		fltemp0,env_nx0,env_mat00
		fmuls		fltemp1,env_nx0,env_mat01
		fmuls		fltemp2,env_nx0,env_mat02
		fmuls		fltemp3,env_nx1,env_mat00
		fmuls		fltemp4,env_nx1,env_mat01
		fmuls		fltemp5,env_nx1,env_mat02
		lfs		env_mat20,Environ.mat20(plightlist)
		lfs		env_mat21,Environ.mat21(plightlist)
		lfs		env_mat22,Environ.mat22(plightlist)
		fmadds		fltemp0,env_ny0,env_mat10,fltemp0
		fmadds		fltemp1,env_ny0,env_mat11,fltemp1
		fmadds		fltemp2,env_ny0,env_mat12,fltemp2
		fmadds		fltemp3,env_ny1,env_mat10,fltemp3
		fmadds		fltemp4,env_ny1,env_mat11,fltemp4
		fmadds		fltemp5,env_ny1,env_mat12,fltemp5
		fmadds		env_nx0,env_nz0,env_mat20,fltemp0
		fmadds		env_ny0,env_nz0,env_mat21,fltemp1
		fmadds		env_nz0,env_nz0,env_mat22,fltemp2
		fmadds		env_nx1,env_nz1,env_mat20,fltemp3
		fmadds		env_ny1,env_nz1,env_mat21,fltemp4
		fmadds		env_nz1,env_nz1,env_mat22,fltemp5

/*	calc v coordinate						*/

		fnmsubs		fv0,env_ny0,env_0pt5,env_0pt5
		fnmsubs		fv1,env_ny1,env_0pt5,env_0pt5

/*	sqrdist = env_nx^2 + env_nz^2					*/

		lfs		env_tiny,CloseData.fconstTINY_off(pclosedata)
		fmadds		env_sqrdist0,env_nx0,env_nx0,env_tiny
		fmadds		env_sqrdist1,env_nx1,env_nx1,env_tiny

		fmadds		env_sqrdist0,env_nz0,env_nz0,env_sqrdist0
		fmadds		env_sqrdist1,env_nz1,env_nz1,env_sqrdist1

/*	formula(1/dist)	invdist = 1/sqrt(sqrdist)			*/

		frsqrte		env_invdist0,env_sqrdist0
		frsqrte		env_invdist1,env_sqrdist1

/*	formula(1/dist)	invdistsqr = invdist * invdist			*/

		fmuls		env_invdistsqr0,env_invdist0,env_invdist0
		fmuls		env_invdistsqr1,env_invdist1,env_invdist1

/*	formula(1/dist)	invdistsqr = 3.0 - (invdistsqr * sqrdist)	*/

		lfs		env_3pt0,CloseData.fconst3pt0_off(pclosedata)
		fnmsubs		env_invdistsqr0,env_invdistsqr0,env_sqrdist0,env_3pt0
		fnmsubs		env_invdistsqr1,env_invdistsqr1,env_sqrdist1,env_3pt0

/*	formula(1/dist)	invdistsqr = invdistsqr * invdist		*/

		fmuls		env_invdistsqr0,env_invdistsqr0,env_invdist0
		fmuls		env_invdistsqr1,env_invdistsqr1,env_invdist1

/*	formula(1/dist)	invdistsqr = invdistsqr * 0.5			*/

		fmuls		env_invdistsqr0,env_invdistsqr0,env_0pt5
		fmuls		env_invdistsqr1,env_invdistsqr1,env_0pt5

/*	calc u coordinate						*/

		fabs		fltemp0,env_nx0
		fabs		fltemp1,env_nz0
		fabs		fltemp2,env_nx1
		fabs		fltemp3,env_nz1

		fcmpu		2,fltemp0,fltemp1	/* cmp |x|,|z| */
		fcmpu		3,fltemp2,fltemp3
		lfs		env_0pt0,CloseData.fconst0pt0_off(pclosedata)
		bge		2,.nx0genz0
/* |nx0| < |nz0| */
		fmuls		fltemp0,env_nx0,env_invdistsqr0
		fcmpu		4,env_nz0,env_0pt0
		b		.test1
.nx0genz0:
		fmuls		fltemp0,env_nz0,env_invdistsqr0
		fcmpu		4,env_nx0,env_0pt0
.test1:
		bge		3,.nx1genz1
/* |nx1| < |nz1| */
		fmuls		fltemp1,env_nx1,env_invdistsqr1
		fcmpu		5,env_nz1,env_0pt0
		b		.arcsin
.nx1genz1:
		fmuls		fltemp1,env_nz1,env_invdistsqr1
		fcmpu		5,env_nx1,env_0pt0
.arcsin:
		fmuls		fltemp2,fltemp0,fltemp0		/* x^2 */
		fmuls		fltemp3,fltemp1,fltemp1
		lfs		env_asin0,CloseData.fconstasin0_off(pclosedata)
		lfs		env_asin1,CloseData.fconstasin1_off(pclosedata)
		lfs		env_asin2,CloseData.fconstasin2_off(pclosedata)
		lfs		env_asin3,CloseData.fconstasin3_off(pclosedata)
		fmuls		fltemp4,fltemp2,fltemp0		/* x^3 */
		fmuls		fltemp5,fltemp3,fltemp1
		fmuls		fu0,fltemp0,env_asin0		/* x * asin0 */
		fmuls		fu1,fltemp1,env_asin0
		fmuls		fltemp0,fltemp2,fltemp4		/* x^5 */
		fmuls		fltemp1,fltemp3,fltemp5
		fmadds		fu0,fltemp4,env_asin1,fu0	/* x^3 * asin1 */
		fmadds		fu1,fltemp5,env_asin1,fu1
		fmuls		fltemp4,fltemp2,fltemp0		/* x^7 */
		fmuls		fltemp5,fltemp3,fltemp1
		fmadds		fu0,fltemp0,env_asin2,fu0	/* x^5 * asin2 */
		fmadds		fu1,fltemp1,env_asin2,fu1
		fmadds		fu0,fltemp4,env_asin3,fu0	/* x^7 * asin3 */
		fmadds		fu1,fltemp5,env_asin3,fu1

/* correct for quadrant */

		lfs		env_0pt0,CloseData.fconst0pt0_off(pclosedata)
		lfs		env_0pt25,CloseData.fconst0pt25_off(pclosedata)
		lfs		env_0pt75,CloseData.fconst0pt75_off(pclosedata)
		lfs		env_1pt0,CloseData.fconst1pt0_off(pclosedata)
		fcmpu		0,fu0,env_0pt0
		fcmpu		1,fu1,env_0pt0
		bge		2,.nx0genz02
/* |nx0| < |nz0| */
		bge		4,.zpos
		fsubs		fu0,env_0pt5,fu0
		b		.test12
.zpos:
		bge		0,.test12
		fadds		fu0,env_1pt0,fu0
		b		.test12
.nx0genz02:
		bge		4,.xpos
		fadds		fu0,env_0pt75,fu0
		b		.test12
.xpos:
		fsubs		fu0,env_0pt25,fu0
.test12:
		bge		3,.nx1genz12
/* |nx1| < |nz1| */
		bge		5,.zpos2
		fsubs		fu1,env_0pt5,fu1
		b		.done
.zpos2:
		bge		1,.done
		fadds		fu1,env_1pt0,fu1
		b		.done
.nx1genz12:
		bge		5,.xpos2
		fadds		fu1,env_0pt75,fu1
		b		.done
.xpos2:
		fsubs		fu1,env_0pt25,fu1
.done:
		lwzu		itemp0,Environ.Next(plightlist)
		mtlr		itemp0
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
