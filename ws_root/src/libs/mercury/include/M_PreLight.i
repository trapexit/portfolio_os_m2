/* ----------------------
 *	FPRs
 * ---------------------- */

/*	Register assignments for pod loop and inverse light stuff */

		define		fr00		,	0
		define		fr01		,	1
		define		fr02		,	2
		define		fr03		,	3
		define		fr04		,	4
		define		fr05		,	5
		define		fr06		,	6
		define		fr07		,	7
		define		fr08		,	8
		define		fr09		,	9
		define		fr10		,	10
		define		fr11		,	11
		define		fr12		,	12
		define		fr13		,	13
		define		fr14		,	14
		define		fr15		,	15
		define		fr16		,	16
		define		fr17		,	17
		define		fr18		,	18
		define		fr19		,	19
		define		fr20		,	20
		define		fr21		,	21
		define		fr22		,	22
		define		fr23		,	23
		define		fr24		,	24
		define		fr25		,	25
		define		fr26		,	26
		define		fr27		,	27
		define		fr28		,	28
		define		fr29		,	29
		define		fr30		,	30
		define		fr31		,	31

		define		finvmat00	,	12
		define		finvmat10	,	13
		define		finvmat20	,	14
		define		finvmat30	,	24
		define		finvmat01	,	15
		define		finvmat11	,	16
		define		finvmat21	,	17
		define		finvmat31	,	25
		define		finvmat02	,	18
		define		finvmat12	,	19
		define		finvmat22	,	20
		define		finvmat32	,	26

/*	Pass 1 		*/

		define		fxmat00		,	0
		define		fxmat01		,	1
		define		fxmat02		,	2
		define		fxmat10		,	3
		define		fxmat11		,	4
		define		fxmat12		,	5
		define		fxmat20		,	6
		define		fxmat21		,	7
		define		fxmat22		,	8
		define		fxmat30		,	9
		define		fxmat31		,	10
		define		fxmat32		,	11
		define		fx0		,	12
		define		fy0		,	13
		define		fz0		,	14
		define		fnx0		,	15
		define		fny0		,	16
		define		fnz0		,	17
		define		fwp0		,	18
		define		fx1		,	19
		define		fy1		,	20
		define		fz1		,	21
		define		fnx1		,	22
		define		fny1		,	23
		define		fnz1		,	24
		define		fwp1		,	25
		define		fxpp0		,	26
		define		fxpp1		,	27
		define		fypp0		,	28
		define		fconst2pt0	,	29
		define		fconstTINY	,	30
		define		fconst12million	,	31

/* 	order is important on these reused registers */

		define		fxp0		,	2
		define		fyp0		,	0
		define		fwinv0		,	1
		define		fxp1		,	11
		define		fyp1		,	9
		define		fwinv1		,	10
		define		fypp1		,	29

/*	Pass 1 clipping	*/

		define		clip0pt0	,	30
		define		cliphither	,	31
		define		clipw		,	1
		define		cliph		,	10

/*	Pass 1 lighting	*/

		define		fbacc1		,	0
		define		faacc1		,	1
		define		fu0		,	2
		define		fv0		,	3
		define		fu1		,	4
		define		fv1		,	5

		define		fltemp0		,	6
		define		fltemp1		,	7
		define		fltemp2		,	8
		define		fltemp3		,	9
		define		fltemp4		,	10
		define		fltemp5		,	11

/*		define		fx0		,	12	*
 *		define		fy0		,	13	*
 *		define		fz0		,	14	*
 *		define		fnx0		,	15	*
 *		define		fny0		,	16	*
 *		define		fnz0		,	17	*
 *		define		fwp0		,	18	*
 *		define		fx1		,	19	*
 *		define		fy1		,	20	*
 *		define		fz1		,	21	*
 *		define		fnx1		,	22	*
 *		define		fny1		,	23	*
 *		define		fnz1		,	24	*
 *		define		fwp1		,	25	*/

		define		fracc0		,	26
		define		fgacc0		,	27
		define		fbacc0		,	28
		define		faacc0		,	29
		define		fracc1		,	30
		define		fgacc1		,	31

/* ----------------------
 *	GPRs
 * ---------------------- */

		define		pclosedata	,	4
		define		pnextpod1	,	5
		define		pnextpod2	,	6
		define		pgeometry1	,	7
		define		pgeometry2	,	8
		define		ppod1		,	9
		define		ppod2		,	10
		define		pmatrix		,	11
		define		plights		,	12
		define		pmaterial	,	13
		define		pconvlights	,	14
		define		plightdata	,	16
		define		pltemp0		,	17
		define		pltemp1		,	18
		define		pvtx1		,	19
		define		pvtx2		,	20
		define		iconst32	,	21
		define		itemp0		,	22

		define		pfirstlight	,	12
		define		plightlist	,	13



