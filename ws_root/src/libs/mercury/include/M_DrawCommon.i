/* ----------------------
 *	FPRs
 * ---------------------- */

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

/*	Pass 2		*/

		define		fdxp0		,	0
		define		fdyp0		,	1
		define		fdxp1		,	2
		define		fdyp1		,	3
		define		fdxp2		,	4
		define		fdyp2		,	5
		define		fdwinv		,	6
		define		fdu0		,	7
		define		fdv0		,	8
		define		fdu1		,	9
		define		fdv1		,	10
		define		fdu2		,	11
		define		fdv2		,	12
		define		fduscale	,	13
		define		fdvscale	,	14
		define		fconst0pt5	,	15	/* env mapping */
		define		fconst1pt0	,	16	/* env mapping */
		define		fdtemp0		,	17	/* temp */
		define		fdtemp1		,	18	/* temp */
		define		fdtemp2		,	19	/* temp */
		define		fdtemp3		,	20	/* temp */
		define		fdu		,	21	/* temp */
		define		fdv		,	22	/* temp */

/*	Pass 2 clipping	*/

		define		fcx0		,	0
		define		fcy0		,	1
		define		fcw0		,	2
		define		fcr0		,	3
		define		fcg0		,	4
		define		fcb0		,	5
		define		fca0		,	6
		define		fcu0		,	7
		define		fcv0		,	8
		define		fcx1		,	9
		define		fcy1		,	10
		define		fcw1		,	11
		define		fcr1		,	12
		define		fcg1		,	13
		define		fcb1		,	14
		define		fca1		,	15
		define		fcu1		,	16
		define		fcv1		,	17
		define		fclipratio	,	18
		define		fcxc		,	19
		define		fcyc		,	20
		define		fcwc		,	21

		define		fcx10		,	0
		define		fcy10		,	1
		define		fcw10		,	2
		define		fcr10		,	3
		define		fcg10		,	4
		define		fcb10		,	5
		define		fca10		,	6
		define		fcu10		,	7
		define		fcv10		,	8

		define		fcrc		,	3
		define		fcgc		,	4
		define		fcbc		,	5
		define		fcac		,	6
		define		fcuc		,	7
		define		fcvc		,	8

		define		fcullx0		,	0
		define		fcully0		,	1
		define		fcullx1		,	2
		define		fcully1		,	3
		define		fcullx2		,	4
		define		fcully2		,	5
		define		fcullx3		,	2
		define		fcully3		,	3
		define		fdeltax0	,	2
		define		fdeltay0	,	3
		define		fdeltax1	,	4
		define		fdeltay1	,	5

/* antialiasing */

		define		fx0aa		,	6
		define		fy0aa		,	7
		define		fw0aa		,	8
		define		fr0aa		,	9
		define		fg0aa		,	10
		define		fb0aa		,	11
		define		fu0aa		,	12
		define		fv0aa		,	15

		define		fx1aa		,	16
		define		fy1aa		,	17
		define		fw1aa		,	18
		define		fr1aa		,	19
		define		fg1aa		,	20
		define		fb1aa		,	21
		define		fu1aa		,	22
		define		fv1aa		,	23

		define		fdxaa		,	24
		define		fdyaa		,	25
		define		f0pt0aa		,	26
		define		f1pt0aa		,	27
		define		f0pt5aa		,	28

		define		ftmp0aa		,	29
		define		ftmp1aa		,	30

/* ----------------------
 *	GPRs
 * ---------------------- */

/*	from M_Draw	*/
		define		pclosedata	,	4
		define		pnextpod	,	5
		define		pVI		,	6
		define		pVIwritemax	,	7
		define		pgeometry	,	8
		define		pxformbase	,	9
		define		ppod		,	10
		define		tricount	,	15

/*	xform loop	*/
		define		pvtx		,	10
		define		pxdata		,	11
		define		pfirstlight	,	12
		define		plightlist	,	13
/*		define		tricount	,	15 */
		define		clipmask	,	16
		define		clip0		,	17
		define		clip1		,	18
		define		iconst32	,	19
		define		itemp0		,	21

/*	shared loop	*/
		define		pshared		,	10
/*		define		pxdata		,	11 */
		define		pxvert		,	12
		define		pxcol		,	13
/*		define		geoflags	,	14 */
/*		define		itemp0		,	21 */

/*	Second pass	*/

		define		pindex		,	10
		define		puv		,	11
		define		vtxoffset0	,	12
		define		vtxoffset1	,	13
		define		vtxoffset2	,	14
/*		define		tricount	,	15 */
		define		someout		,	16
		define		ptexselsnippets	,	16
		define		itemp1		,	16

/*		define		itemp0		,	21 */
		define		nextvtxoffset	,	22
		define		iconstVI1	,	23
		define		iconstVI3	,	24
/*	touch loads	*/
		define		tltabnodraw	,	25
		define		tltabdraw1	,	26
		define		tltabdraw3	,	27
		define		tloffset	,	28

/*	clip test */
		define		iclipflags0	,	29
		define		iclipflags1	,	30
		define		iclipflags2	,	31

/*	Second pass clipping	*/

/*		define		someout		,	16 */
		define		savelr		,	17
		define		savelr2		,	18
		define		int_one		,	19
		define		flags		,	20
/*		define		itemp0		,	21 */

		define		mask		,	22
		define		routineptr	,	23
		define		count		,	24
		define		srcptr		,	25
		define		destptr		,	26
		define		lastsrc		,	27
		define		lastdest	,	28
		define		newptr		,	29
		define		oldptr		,	30
		define		storeptr	,	31

/*	antialiasing pass 1 */

		define		paaedgebuf	,	31	/* iclipflags2 */
		define		paaedge		,	8	/* pgeometry */
		define		edgeidx0	,	17
		define		edgeidx1	,	18
		define		edgeidx2	,	19
		define		edgeidx		,	20

/* 	antialiasing pass 2 */

		define		pdraw0AA		,	23
		define		pdraw1AA		,	24
		define		puv0AA			,	25
		define		puv0			,	26
		define		puv1			,	27
		define		puv2			,	28
		define		puv1AA			,	29

/*
 *	Structs
 */
/*	transformed */

	struct		xform
		stlong		xform.xpp	,	1
		stlong		xform.ypp	,	1
		stlong		xform.winv	,	1
		stlong		xform.r		,	1
		stlong		xform.g		,	1
		stlong		xform.b		,	1
		stlong		xform.a		,	1
		stlong		xform.xp	,	1
		stlong		xform.yp	,	1
		stlong		xform.wp	,	1
		stlong		xform.flags	,	1
	ends	xform

	struct		xformtr
		stlong		xformtr.xpp	,	1
		stlong		xformtr.ypp	,	1
		stlong		xformtr.winv	,	1
		stlong		xformtr.r	,	1
		stlong		xformtr.g	,	1
		stlong		xformtr.b	,	1
		stlong		xformtr.a	,	1
		stlong		xformtr.xp	,	1
		stlong		xformtr.yp	,	1
		stlong		xformtr.wp	,	1
		stlong		xformtr.flags	,	1
		stlong		xformtr.u	,	1
	ends	xformtr

	struct		xformen
		stlong		xformen.xpp	,	1
		stlong		xformen.ypp	,	1
		stlong		xformen.winv	,	1
		stlong		xformen.r	,	1
		stlong		xformen.g	,	1
		stlong		xformen.b	,	1
		stlong		xformen.a	,	1
		stlong		xformen.xp	,	1
		stlong		xformen.yp	,	1
		stlong		xformen.wp	,	1
		stlong		xformen.flags	,	1
		stlong		xformen.u	,	1
		stlong		xformen.v	,	1
	ends	xformen

/*	clipping */
	struct	vertex
		stlong		vertex.x	,	1
		stlong		vertex.y	,	1
		stlong		vertex.w	,	1
		stlong		vertex.r	,	1
		stlong		vertex.g	,	1
		stlong		vertex.b	,	1
		stlong		vertex.a	,	1
		stlong		vertex.flags	,	1
		stlong		vertex.u	,	1
		stlong		vertex.v	,	1
	ends	vertex

/* antialiasing */

	struct		outvtxtexaa
		stlong		outvtxtexaa.x	,	1
		stlong		outvtxtexaa.y	,	1
		stlong		outvtxtexaa.r	,	1
		stlong		outvtxtexaa.g	,	1
		stlong		outvtxtexaa.b	,	1
		stlong		outvtxtexaa.a	,	1
		stlong		outvtxtexaa.w	,	1
		stlong		outvtxtexaa.u	,	1
		stlong		outvtxtexaa.v	,	1
	ends	outvtxtexaa

	struct		outvtxaa
		stlong		outvtxaa.x	,	1
		stlong		outvtxaa.y	,	1
		stlong		outvtxaa.r	,	1
		stlong		outvtxaa.g	,	1
		stlong		outvtxaa.b	,	1
		stlong		outvtxaa.a	,	1
		stlong		outvtxaa.w	,	1
	ends	outvtxaa
