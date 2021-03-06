	.include	"structmacros.i"
	.include	"PPCMacroequ.i"
	.include	"conditionmacros.i"
	.include	"mercury.i"

	.include	"M_DrawCommon.i"

/*
 *	Relevant FLAG inputs, all in CR6 and CR7
 *		clipFLAG	must preserve
 *		callatendFLAG	must preserve
 *		nocullFLAG	must preserve
 *		frontcullFLAG	must preserve
 */
	DECFN	M_DrawDynLitTrans
begincode:
		mflr	3

		stw		3,CloseData.drawlrsave_off(pclosedata)

		lwz		pfirstlight,CloseData.convlightdata_off(pclosedata)
		lwz		pvtx,PodGeometry.pvertex_off(pgeometry)
		lhz		itemp0,PodGeometry.vertexcount_off(pgeometry)
		li		iconst32,32

		srwi		itemp0,itemp0,1
		subi		pvtx,pvtx,4
		mtctr		itemp0

		addi		pxdata,pxformbase,(2*xformtr)		/* pxdata is same reg as pgeometry! */
xformlooptop:
		lfs		fxmat00,CloseData.fxmat00_off(pclosedata)
		lfs		fxmat10,CloseData.fxmat10_off(pclosedata)
		lfs		fxmat20,CloseData.fxmat20_off(pclosedata)
		lfs		fxmat30,CloseData.fxmat30_off(pclosedata)
		lfs		fxmat01,CloseData.fxmat01_off(pclosedata)
		lfs		fxmat11,CloseData.fxmat11_off(pclosedata)
		lfs		fxmat21,CloseData.fxmat21_off(pclosedata)
		lfs		fxmat31,CloseData.fxmat31_off(pclosedata)
		lfs		fxmat02,CloseData.fxmat02_off(pclosedata)
		lfs		fxmat12,CloseData.fxmat12_off(pclosedata)
		lfs		fxmat22,CloseData.fxmat22_off(pclosedata)
		lfs		fxmat32,CloseData.fxmat32_off(pclosedata)
		lfs		fconstTINY,CloseData.fconstTINY_off(pclosedata)
		lfs		fconst2pt0,CloseData.fconst2pt0_off(pclosedata)

		lfsu		fx0,4(pvtx)
		lfsu		fy0,4(pvtx)
		lfsu		fz0,4(pvtx)
		lfsu		fnx0,4(pvtx)
		lfsu		fny0,4(pvtx)
		lfsu		fnz0,4(pvtx)
		lfsu		fx1,4(pvtx)
		lfsu		fy1,4(pvtx)
		lfsu		fz1,4(pvtx)
		lfsu		fnx1,4(pvtx)
		lfsu		fny1,4(pvtx)
		lfsu		fnz1,4(pvtx)
		lfs		fconst12million,CloseData.fconst12million_off(pclosedata)

		dcbt		0,pxdata

		fmadds		fwp0,fx0,fxmat02,fxmat32
		fmadds		fwp1,fx1,fxmat02,fxmat32
		fmadds		fwp0,fy0,fxmat12,fwp0
		fmadds		fwp1,fy1,fxmat12,fwp1
		fmadds		fwp0,fz0,fxmat22,fwp0
		fmadds		fwp1,fz1,fxmat22,fwp1

		fmadds		fxp0,fx0,fxmat00,fxmat30
		fmadds		fxp1,fx1,fxmat00,fxmat30
		fmadds		fyp0,fx0,fxmat01,fxmat31
		fmadds		fyp1,fx1,fxmat01,fxmat31

		fmadds		fwinv0,fwp0,fwp0,fconstTINY
		fmadds		fwinv1,fwp1,fwp1,fconstTINY
		fmadds		fyp0,fy0,fxmat11,fyp0
		fmadds		fyp1,fy1,fxmat11,fyp1

		dcbt		pxdata,iconst32

		fmadds		fxp0,fy0,fxmat10,fxp0
		fmadds		fxp1,fy1,fxmat10,fxp1
		frsqrte		fwinv0,fwinv0
		frsqrte		fwinv1,fwinv1

		fnmsubs		fconstTINY,fwinv0,fwp0,fconst2pt0	/* fconst2pt0-fwinv0*fwp0 */
		fnmsubs		fconst2pt0,fwinv1,fwp1,fconst2pt0	/* fconst2pt0-fwinv1*fwp1?*/
		fmadds		fyp0,fz0,fxmat21,fyp0
		fmadds		fyp1,fz1,fxmat21,fyp1

		fmuls		fwinv0,fwinv0,fconstTINY
		fmuls		fwinv1,fwinv1,fconst2pt0
		fmadds		fxp0,fz0,fxmat20,fxp0
		fmadds		fxp1,fz1,fxmat20,fxp1

		fmadds		fxpp0,fxp0,fwinv0,fconst12million
		fmadds		fxpp1,fxp1,fwinv1,fconst12million
		fmadds		fypp0,fyp0,fwinv0,fconst12million
		fmadds		fypp1,fyp1,fwinv1,fconst12million

		fsubs		fxpp0,fxpp0,fconst12million
		fsubs		fypp0,fypp0,fconst12million
		fsubs		fxpp1,fxpp1,fconst12million
		fsubs		fypp1,fypp1,fconst12million

		stfs		fxpp0,xformtr.xpp-(2*xformtr)(pxdata)
		stfs		fypp0,xformtr.ypp-(2*xformtr)(pxdata)
		stfs		fwinv0,xformtr.winv-(2*xformtr)(pxdata)
		stfs		fxpp1,xformtr.xpp-xformtr(pxdata)
		stfs		fypp1,xformtr.ypp-xformtr(pxdata)
		stfs		fwinv1,xformtr.winv-xformtr(pxdata)
		bf		clipFLAG,.loadrgb

/* if clipping, store pre-projected points and check clip bounds */

		stfs		fxp0,xformtr.xp-(2*xformtr)(pxdata)
		stfs		fyp0,xformtr.yp-(2*xformtr)(pxdata)
		stfs		fwp0,xformtr.wp-(2*xformtr)(pxdata)
		stfs		fxp1,xformtr.xp-xformtr(pxdata)
		stfs		fyp1,xformtr.yp-xformtr(pxdata)
		stfs		fwp1,xformtr.wp-xformtr(pxdata)

		lfs		cliphither,CloseData.fwclose_off(pclosedata)
		lfs		clip0pt0,CloseData.fconst0pt0_off(pclosedata)
		lfs		clipw,CloseData.fscreenwidth_off(pclosedata)
		lfs		cliph,CloseData.fscreenheight_off(pclosedata)
		fcmpu		4,cliphither,fwp0	/* if hither > fwp0 */
		fcmpu		0,clip0pt0,fxpp0	/* if 0.0  > fxpp0 */
		fcmpu		1,fxpp0,clipw		/* if fxpp0 > width */
		fcmpu		2,clip0pt0,fypp0	/* if 0.0  > fypp0 */
		fcmpu		3,fypp0,cliph		/* if fypp0 > height */
		mfcr		clip0
		fcmpu		4,cliphither,fwp1	/* if hither > fwp1 */
		fcmpu		0,clip0pt0,fxpp1	/* if 0.0  > fxpp1 */
		fcmpu		1,fxpp1,clipw		/* if fxpp1 > width */
		fcmpu		2,clip0pt0,fypp1	/* if 0.0  > fypp1 */
		fcmpu		3,fypp1,cliph		/* if fypp1 > height */
		mfcr		clip1
		lis		clipmask,0x4444
		addi		clipmask,clipmask,0x4000
		and		clip0,clip0,clipmask
		and		clip1,clip1,clipmask
		stw		clip0,xformtr.flags-(2*xformtr)(pxdata)
		stw		clip1,xformtr.flags-xformtr(pxdata)
.loadrgb:
		lfs		fracc0,CloseData.frbase_off(pclosedata)
		lfs		fgacc0,CloseData.fgbase_off(pclosedata)
		lfs		fbacc0,CloseData.fbbase_off(pclosedata)
		lfs		faacc0,CloseData.fabase_off(pclosedata)
		lfs		fu0,CloseData.fconst1pt0_off(pclosedata)
		lfs		fracc1,CloseData.frbase_off(pclosedata)
		lfs		fgacc1,CloseData.fgbase_off(pclosedata)
		lfs		fbacc1,CloseData.fbbase_off(pclosedata)
		lfs		faacc1,CloseData.fabase_off(pclosedata)
		lfs		fu1,CloseData.fconst1pt0_off(pclosedata)

		li		itemp0,64
		dcbt		pxdata,itemp0

		mtlr		pfirstlight

		addi		plightlist,pclosedata,CloseData.convlightdata_off

		blr

	DECFN	M_LightReturnDynLitTrans

		lfs		fltemp1,CloseData.fconst1pt0_off(pclosedata)

		fsubs		fltemp2,fracc0,fltemp1
		fsubs		fltemp3,fgacc0,fltemp1
		fsubs		fltemp4,fbacc0,fltemp1

		fsel		fracc0,fltemp2,fltemp1,fracc0
		fsel		fgacc0,fltemp3,fltemp1,fgacc0
		fsel		fbacc0,fltemp4,fltemp1,fbacc0

		fsubs		fltemp2,fracc1,fltemp1
		fsubs		fltemp3,fgacc1,fltemp1
		fsubs		fltemp4,fbacc1,fltemp1

		fsel		fracc1,fltemp2,fltemp1,fracc1
		fsel		fgacc1,fltemp3,fltemp1,fgacc1
		fsel		fbacc1,fltemp4,fltemp1,fbacc1

		stfs		fracc0,xformtr.r-(2*xformtr)(pxdata)
		stfs		fgacc0,xformtr.g-(2*xformtr)(pxdata)
		stfs		fbacc0,xformtr.b-(2*xformtr)(pxdata)
		stfs		faacc0,xformtr.a-(2*xformtr)(pxdata)
		stfs		fu0,xformtr.u-(2*xformtr)(pxdata)

		stfs		fracc1,xformtr.r-xformtr(pxdata)
		stfs		fgacc1,xformtr.g-xformtr(pxdata)
		stfs		fbacc1,xformtr.b-xformtr(pxdata)
		stfs		faacc1,xformtr.a-xformtr(pxdata)
		stfs		fu1,xformtr.u-xformtr(pxdata)

		addi		pxdata,pxdata,(2*xformtr)
		bdnz		xformlooptop

/***************************************************
 *
 *	Copy shared vertices and colors
 *
 ***************************************************/

		lhz		itemp0,PodGeometry.sharedcount_off(pgeometry)
		lwz		pshared,PodGeometry.pshared_off(pgeometry)
		cmpi		0,itemp0,0
		subi		pshared,pshared,2
		mtctr		itemp0
		beq		0,.startpass2
.sharedloop:
		lhzu		pxvert,2(pshared)
		lhzu		pxcol,2(pshared)
		mulli		pxvert,pxvert,xformtr
		mulli		pxcol,pxcol,xformtr
		add		pxvert,pxvert,pxformbase
		add		pxcol,pxcol,pxformbase

		lfs		fxpp0,xformtr.xpp(pxvert)
		lfs		fypp0,xformtr.ypp(pxvert)
		lfs		fwinv0,xformtr.winv(pxvert)

		bf		clipFLAG,.storesharedrgb

		lfs		fxp0,xformtr.xp(pxvert)
		lfs		fyp0,xformtr.yp(pxvert)
		lfs		fwp0,xformtr.wp(pxvert)
		lwz		clip0,xformtr.flags(pxvert)

		stfs		fxp0,xformtr.xp-(2*xformtr)(pxdata)
		stfs		fyp0,xformtr.yp-(2*xformtr)(pxdata)
		stfs		fwp0,xformtr.wp-(2*xformtr)(pxdata)
		stw		clip0,xformtr.flags-(2*xformtr)(pxdata)
.storesharedrgb:
		stfs		fxpp0,xformtr.xpp-(2*xformtr)(pxdata)
		stfs		fypp0,xformtr.ypp-(2*xformtr)(pxdata)
		stfs		fwinv0,xformtr.winv-(2*xformtr)(pxdata)
		lfs		fracc0,xformtr.r(pxcol)
		lfs		fgacc0,xformtr.g(pxcol)
		lfs		fbacc0,xformtr.b(pxcol)
		lfs		faacc0,xformtr.a(pxcol)
		lfs		fu0,xformtr.u(pxvert)
		stfs		fracc0,xformtr.r-(2*xformtr)(pxdata)
		stfs		fgacc0,xformtr.g-(2*xformtr)(pxdata)
		stfs		fbacc0,xformtr.b-(2*xformtr)(pxdata)
		stfs		faacc0,xformtr.a-(2*xformtr)(pxdata)
		stfs		fu0,xformtr.u-(2*xformtr)(pxdata)

		addi		pxdata,pxdata,xformtr
		bdnz		.sharedloop

/***************************************************
 *
 *	START PASS 2 code
 *
 ***************************************************/
.startpass2:
		lfs		fduscale,CloseData.fconst16pt0_off(pclosedata)
		lfs		fdv,CloseData.fconst0pt0_off(pclosedata)
		lwz		pindex,PodGeometry.pindex_off(pgeometry)
		addi		tltabnodraw,pclosedata,(CloseData.tltable_off + 11)
		addi		tltabdraw1,pclosedata,(CloseData.tltable_off + 6)
		addi		tltabdraw3,pclosedata,(CloseData.tltable_off - 3)
		lhz		itemp0,0(pindex)

		li		iconstVI3,2
		addis		iconstVI3,iconstVI3,0x200f
		addis		iconstVI1,0,0x2007

		li		tloffset,-1			/* will become 0x1c */

		mtcrf		0x0c,itemp0			/* CR4 and CR5 */
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xformtr
/*
 *	nextvtxoffset set to first point's value
 *	flags set based on first point's index
 */
startstripfannodraw:
		lbzx		tloffset,tltabnodraw,tloffset
startstripfan:
		bf		selecttextureFLAG,notextureselect
		lhau		itemp0,2(pindex)
		cmpi		0,itemp0,0				/* chs */
		bge+		selecttexture

/*	all done */
alldone:
		lwz		3,CloseData.drawlrsave_off(pclosedata)
		mtlr		3
		blr
selecttexture:
/*	no texture loads */
notextureselect:
		lhau		itemp0,2(pindex)
		add		vtxoffset0,nextvtxoffset,pxformbase
		andi.		vtxoffset1,itemp0,0x7ff
		lhau		itemp0,2(pindex)
		mulli		vtxoffset1,vtxoffset1,xformtr
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xformtr
		add		vtxoffset1,vtxoffset1,pxformbase

		lfs		fdu0,xformtr.u(vtxoffset0)
		lfs		fdu1,xformtr.u(vtxoffset1)
		lfs		fdxp0,xformtr.xpp(vtxoffset0)
		lfs		fdyp0,xformtr.ypp(vtxoffset0)
		lfs		fdxp1,xformtr.xpp(vtxoffset1)
		lfs		fdyp1,xformtr.ypp(vtxoffset1)
		lwz		iclipflags0,xformtr.flags(vtxoffset0)
		lwz		iclipflags1,xformtr.flags(vtxoffset1)
		fmuls		fdu0,fdu0,fduscale
		fmuls		fdu1,fdu1,fduscale
		b		start012specialentry

/***************************************************
 *
 *	stitch code
 *
 ***************************************************/

/* came from 012 */
start021iffan:
		bf		curfanFLAG,start120
		bt		startnewstripFLAG,startstripfannodraw
start021:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load1
		bf		cr0gt,start012iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw0
		bl		draw2
		bl		draw1
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont210
		bt		startnewstripFLAG,startstripfan
cont012:
		bl		load2
		bf		cr0gt,start021iffan
		bl		draw2single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont120
		bt		startnewstripFLAG,startstripfan
		b		cont021

/* came from 021 */
start012iffan:
		bf		curfanFLAG,start210
		bt		startnewstripFLAG,startstripfannodraw
start012:
		lbzx		tloffset,tltabnodraw,tloffset
start012specialentry:
		bl		load2
		bf		cr0gt,start021iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw0
		bl		draw1
		bl		draw2
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont120
		bt		startnewstripFLAG,startstripfan
cont021:
		bl		load1
		bf		cr0gt,start012iffan
		bl		draw1single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont210
		bt		startnewstripFLAG,startstripfan
		b		cont012

/* came from 102 */
start120iffan:
		bf		curfanFLAG,start021
		bt		startnewstripFLAG,startstripfannodraw
start120:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load0
		bf		cr0gt,start102iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw1
		bl		draw2
		bl		draw0
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont201
		bt		startnewstripFLAG,startstripfan
cont102:
		bl		load2
		bf		cr0gt,start120iffan
		bl		draw2single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont021
		bt		startnewstripFLAG,startstripfan
		b		cont120

/* came from 120 */
start102iffan:
		bf		curfanFLAG,start201
		bt		startnewstripFLAG,startstripfannodraw
start102:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load2
		bf		cr0gt,start120iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw1
		bl		draw0
		bl		draw2
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont021
		bt		startnewstripFLAG,startstripfan
cont120:
		bl		load0
		bf		cr0gt,start102iffan
		bl		draw0single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont201
		bt		startnewstripFLAG,startstripfan
		b		cont102

/* came from 201 */
start210iffan:
		bf		curfanFLAG,start012
		bt		startnewstripFLAG,startstripfannodraw
start210:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load0
		bf		cr0gt,start201iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw2
		bl		draw1
		bl		draw0
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont102
		bt		startnewstripFLAG,startstripfan
cont201:
		bl		load1
		bf		cr0gt,start210iffan
		bl		draw1single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont012
		bt		startnewstripFLAG,startstripfan
		b		cont210

/* came from 210 */
start201iffan:
		bf		curfanFLAG,start102
		bt		startnewstripFLAG,startstripfannodraw
start201:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load1
		bf		cr0gt,start210iffan
		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw2
		bl		draw0
		bl		draw1
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont012
		bt		startnewstripFLAG,startstripfan
cont210:
		bl		load0
		bf		cr0gt,start201iffan
		bl		draw0single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont102
		bt		startnewstripFLAG,startstripfan
		b		cont201

/***************************************************
 *
 *	load code
 *
 ***************************************************/

load0:
		add		vtxoffset0,nextvtxoffset,pxformbase

		lfs		fdxp0,xformtr.xpp(vtxoffset0)
		lfs		fdyp0,xformtr.ypp(vtxoffset0)
		lwz		iclipflags0,xformtr.flags(vtxoffset0)

		lfs		fdu0,xformtr.u(vtxoffset0)
		fmuls		fdu0,fdu0,fduscale
		b		loadcont
load1:
		add		vtxoffset1,nextvtxoffset,pxformbase

		lfs		fdxp1,xformtr.xpp(vtxoffset1)
		lfs		fdyp1,xformtr.ypp(vtxoffset1)
		lwz		iclipflags1,xformtr.flags(vtxoffset1)

		lfs		fdu1,xformtr.u(vtxoffset1)
		fmuls		fdu1,fdu1,fduscale
		b		loadcont
load2:
		add		vtxoffset2,nextvtxoffset,pxformbase

		lfs		fdxp2,xformtr.xpp(vtxoffset2)
		lfs		fdyp2,xformtr.ypp(vtxoffset2)
		lwz		iclipflags2,xformtr.flags(vtxoffset2)

		lfs		fdu2,xformtr.u(vtxoffset2)
		fmuls		fdu2,fdu2,fduscale
loadcont:
		dcbt		pVI,tloffset
		srawi		tloffset,tloffset,3

		lhau		itemp0,2(pindex)
		andi.		nextvtxoffset,itemp0,0x7ff
		rlwimi		iconstVI1,itemp0,(17-11),11,11
		mulli		nextvtxoffset,nextvtxoffset,xformtr

/*	set flags CR4 and CR5 from itemp0 */

		mtcrf		0x0c,itemp0
		bf		clipFLAG,.noclip2

		or		someout,iclipflags0,iclipflags1
		and		itemp0,iclipflags0,iclipflags1
		or.		someout,someout,iclipflags2
		and		itemp0,itemp0,iclipflags2
		bne		0,.clip
.noclip2:
		bt		nocullFLAG,.nocull
		fsubs		fdtemp0,fdxp1,fdxp0
		fsubs		fdtemp1,fdyp2,fdyp0
		fsubs		fdtemp2,fdxp2,fdxp0
		fsubs		fdtemp3,fdyp1,fdyp0

		fmuls		fdtemp0,fdtemp0,fdtemp1
		fmuls		fdtemp2,fdtemp2,fdtemp3

		fcmpu		0,fdtemp2,fdtemp0			/* chs */
		beqlr		0
		crxor		cr0gt,cr0gt,prevclockwiseFLAG
		crxor		cr0gt,cr0gt,frontcullFLAG
		.ifdef	STATISTICS
		mflr		itemp1
		btl		cr0gt,inccount
		mtlr		itemp1
		.endif
		blr
.nocull:
		crset		cr0gt
		.ifdef	STATISTICS
		mflr		itemp1
		bl		inccount
		mtlr		itemp1
		.endif
		blr
		.ifdef STATISTICS
inccount:	bt		clipFLAG,countclip
		lwz		itemp0,CloseData.numtris_fast(pclosedata)
		addi		itemp0,itemp0,1
		stw		itemp0,CloseData.numtris_fast(pclosedata)
		blr
countclip:	
		lwz		itemp0,CloseData.numtris_slow(pclosedata)
		addi		itemp0,itemp0,1
		stw		itemp0,CloseData.numtris_slow(pclosedata)
		blr
		.endif
.clip:
		cmpi		0,itemp0,0
		bne		.allout
		mflr		itemp0
		stw		itemp0,CloseData.drawlrsave2_off(pclosedata)
		bl		clip
		cmpl		0,pVI,pVIwritemax
		bgel-		M_ClistManager
		lwz		itemp0,CloseData.drawlrsave2_off(pclosedata)
		mtlr		itemp0
.allout:
		crclr		cr0gt
		blr

/***************************************************
 *
 *	draw code
 *
 ***************************************************/

draw0single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw0 */

draw0:	
		lfs		fdwinv,xformtr.winv(vtxoffset0)

		lfs		fdtemp0,xformtr.r(vtxoffset0)
		lfs		fdtemp1,xformtr.g(vtxoffset0)
		lfs		fdtemp2,xformtr.b(vtxoffset0)
		lfs		fdtemp3,xformtr.a(vtxoffset0)
	
		stfsu		fdxp0,4(pVI)
		stfsu		fdyp0,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu0,4(pVI)
		stfsu		fdv,4(pVI)
		blr

draw1single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw1 */

draw1:	
		lfs		fdwinv,xformtr.winv(vtxoffset1)

		lfs		fdtemp0,xformtr.r(vtxoffset1)
		lfs		fdtemp1,xformtr.g(vtxoffset1)
		lfs		fdtemp2,xformtr.b(vtxoffset1)
		lfs		fdtemp3,xformtr.a(vtxoffset1)
	
		stfsu		fdxp1,4(pVI)
		stfsu		fdyp1,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu1,4(pVI)
		stfsu		fdv,4(pVI)
		blr

draw2single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw2 */

draw2:	
		lfs		fdwinv,xformtr.winv(vtxoffset2)

		lfs		fdtemp0,xformtr.r(vtxoffset2)
		lfs		fdtemp1,xformtr.g(vtxoffset2)
		lfs		fdtemp2,xformtr.b(vtxoffset2)
		lfs		fdtemp3,xformtr.a(vtxoffset2)
	
		stfsu		fdxp2,4(pVI)
		stfsu		fdyp2,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu2,4(pVI)
		stfsu		fdv,4(pVI)
		blr

/***************************************************
 *
 *	full clipping code
 *
 ***************************************************/
clip:	
		stfs		0,CloseData.float0(pclosedata)
		stfs		1,CloseData.float1(pclosedata)
		stfs		2,CloseData.float2(pclosedata)
		stfs		3,CloseData.float3(pclosedata)
		stfs		4,CloseData.float4(pclosedata)
		stfs		5,CloseData.float5(pclosedata)
		stfs		6,CloseData.float6(pclosedata)
		stfs		7,CloseData.float7(pclosedata)
		stfs		8,CloseData.float8(pclosedata)
		stfs		9,CloseData.float9(pclosedata)
		stfs		10,CloseData.float10(pclosedata)
		stfs		11,CloseData.float11(pclosedata)
		stfs		12,CloseData.float12(pclosedata)
		stfs		13,CloseData.float13(pclosedata)
		stfs		15,CloseData.float15(pclosedata)
		stfs		17,CloseData.float17(pclosedata)
		stmw		17,CloseData.int17_31(pclosedata)
		mflr		savelr
		li		int_one,1
		addi		storeptr,pclosedata,CloseData.clipdata_off-4
		addi		lastsrc,pclosedata,CloseData.clipbuf2_off-4
		addi		lastdest,pclosedata,CloseData.clipbuf1_off-4
		mr		destptr,lastdest
/*
 *	move verts into clipdata buffer
 */
		li		count,3
		mr		srcptr,vtxoffset0
		lfs		fcuc,CloseData.float7(pclosedata)
		bl		.copyverts
		mr		srcptr,vtxoffset1
		lfs		fcuc,CloseData.float9(pclosedata)
		bl		.copyverts
		mr		srcptr,vtxoffset2
		lfs		fcuc,CloseData.float11(pclosedata)
		bl		.copyverts
/*
 *	wmin clip
 *	x,y,u,v,and w are in homogenous coordinates because
 *	x,y,u,and v would not be valid if w at or behind
 * 	the hither plane.
 */
		li		mask,0x4000
		lea		routineptr,.clip_wmin
		bl		.do_clip
		blt		1,.exit_clip
/*
 *	project points
 *	now it's safe to project x and y. we need
 *	to do this in order to clip to x and y in
 *	screen coordinates. we don't invert w yet
 *	because we still want to do linear interpolation
 *	of u,v, and w.
 */
		li		someout,0
		mtctr		count
		li		someout,0
		mr		srcptr,lastdest
		lfs		fconst2pt0,CloseData.fconst2pt0_off(pclosedata)
		lfs		fconst12million,CloseData.fconst12million_off(pclosedata)
.project_loop:
		lwzu		newptr,4(srcptr)
		lfs		fcwc,vertex.w(newptr)
		fmuls		fcw1,fcwc,fcwc
		lfs		fcxc,vertex.x(newptr)
		lfs		fcyc,vertex.y(newptr)
		frsqrte		fcw1,fcw1
		lfs		fcuc,vertex.u(newptr)
		fnmsubs		fcwc,fcw1,fcwc,fconst2pt0	/* fconst2pt0-(w1*wc) */
		fmuls		fcwc,fcw1,fcwc
		fmadds		fcxc,fcxc,fcwc,fconst12million
		fmadds		fcyc,fcyc,fcwc,fconst12million
		fsubs		fcxc,fcxc,fconst12million
		fsubs		fcyc,fcyc,fconst12million
		stfs		fcxc,vertex.x(newptr)
		stfs		fcyc,vertex.y(newptr)
		stfs		fcwc,vertex.w(newptr)
		stfs		fcuc,vertex.u(newptr)
		bl		.clip_test
		or		someout,someout,flags
		stw		flags,vertex.flags(newptr)
.project_next:
		bdnz+		.project_loop
/*
 *	cull triangle
 *	now that we have valid projected x and y
 *	coordinates we can cull test the traingle
 */
		bt		nocullFLAG,.nocull2
		cmpi		1,count,3
		addi		srcptr,lastdest,4	/* pre increment */
		lwz		newptr,0x0(srcptr)
		lfs		fcullx0,vertex.x(newptr)
		lfs		fcully0,vertex.y(newptr)
		lwz		newptr,0x4(srcptr)
		lfs		fcullx1,vertex.x(newptr)
		lfs		fcully1,vertex.y(newptr)
		lwz		newptr,0x8(srcptr)
		lfs		fcullx2,vertex.x(newptr)
		lfs		fcully2,vertex.y(newptr)

		fsubs		fdeltax0,fcullx1,fcullx0
		fsubs		fdeltay0,fcully1,fcully0
		fsubs		fdeltax1,fcullx2,fcullx0
		fsubs		fdeltay1,fcully2,fcully0

		fmuls		fdeltax0,fdeltax0,fdeltay1
		fmuls		fdeltay0,fdeltax1,fdeltay0

		fcmpu		0,fdeltay0,fdeltax0
		crxor		cr0gt,cr0gt,prevclockwiseFLAG
		crxor		cr0gt,cr0gt,frontcullFLAG
		bgt		0,.nocull2
		beq		1,.exit_clip

		lwz		newptr,0x0(srcptr)
		lwz		itemp0,0xc(srcptr)
		lfs		fcullx0,vertex.x(newptr)
		lfs		fcully0,vertex.y(newptr)
		lfs		fcullx3,vertex.x(itemp0)
		lfs		fcully3,vertex.y(itemp0)
		lwz		newptr,0x8(srcptr)

		fsubs		fdeltax0,fcullx3,fcullx0
		fsubs		fdeltay0,fcully3,fcully0

		fmuls		fdeltay0,fdeltax1,fdeltay0
		fmuls		fdeltax0,fdeltax0,fdeltay1

		fcmpu		0,fdeltax0,fdeltay0

		stw		newptr,0x4(srcptr)
		stw		itemp0,0x8(srcptr)
		li		count,3
		beq		0,.exit_clip
		crxor		cr0gt,cr0gt,prevclockwiseFLAG
		crxor		cr0gt,cr0gt,frontcullFLAG
		bf		cr0gt,.exit_clip
.nocull2:
/*
 *	xmin clip
 */
		lis		mask,0x4000
		lea		routineptr,.clip_xmin
		bl		.do_clip
		blt		1,.exit_clip
		beq		.build
/*
 *	xmax clip
 */
		lis		mask,0x0400
		lea		routineptr,.clip_xmax
		bl		.do_clip
		blt		1,.exit_clip
		beq		.build
/*
 *	ymin clip
 */
		lis		mask,0x0040
		lea		routineptr,.clip_ymin
		bl		.do_clip
		blt		1,.exit_clip
		beq		.build
/*
 *	ymax clip
 */
		lis		mask,0x0004
		lea		routineptr,.clip_ymax
		bl		.do_clip
		blt		1,.exit_clip
/*
 *	done clipping
 *	now lets make a fan from the vertices
 *	we've just generated. We have to invert w
 *	at this time.
 */
/*	header word	*/
.build:
		subi		itemp0,count,1
		addis		itemp0,itemp0,0x201f
		stwu		itemp0,4(pVI)
		subi		itemp0,count,2
		.ifdef STATISTICS
		lwz		itemp1,CloseData.numtris_slow(pclosedata)
		add		itemp1,itemp1,itemp0
		stw		itemp1,CloseData.numtris_slow(pclosedata)
		.endif
		rlwinm		itemp0,itemp0,16,0,15
		add		tricount,tricount,itemp0
		bl		.swap
.build_loop:
		lwzu		newptr,4(srcptr)
		lfs		fcx0,vertex.x(newptr)
		lfs		fcy0,vertex.y(newptr)
		lfs		fcr0,vertex.r(newptr)
		lfs		fcg0,vertex.g(newptr)
		lfs		fcb0,vertex.b(newptr)
		lfs		fca0,vertex.a(newptr)
		lfs		fcw0,vertex.w(newptr)
		lfs		fcu0,vertex.u(newptr)
		stfsu		fcx0,4(pVI)
		stfsu		fcy0,4(pVI)
		stfsu		fcr0,4(pVI)
		stfsu		fcg0,4(pVI)
		stfsu		fcb0,4(pVI)
		stfsu		fca0,4(pVI)
		stfsu		fcw0,4(pVI)
		stfsu		fcu0,4(pVI)
		stfsu		fdv,4(pVI)		
		bdnz+		.build_loop
.exit_clip:
		mtlr		savelr
		lfs		0,CloseData.float0(pclosedata)
		lfs		1,CloseData.float1(pclosedata)
		lfs		2,CloseData.float2(pclosedata)
		lfs		3,CloseData.float3(pclosedata)
		lfs		4,CloseData.float4(pclosedata)
		lfs		5,CloseData.float5(pclosedata)
		lfs		6,CloseData.float6(pclosedata)
		lfs		7,CloseData.float7(pclosedata)
		lfs		8,CloseData.float8(pclosedata)
		lfs		9,CloseData.float9(pclosedata)
		lfs		10,CloseData.float10(pclosedata)
		lfs		11,CloseData.float11(pclosedata)
		lfs		12,CloseData.float12(pclosedata)
		lfs		13,CloseData.float13(pclosedata)
		lfs		15,CloseData.float15(pclosedata)
		lfs		17,CloseData.float17(pclosedata)
		lmw		17,CloseData.int17_31(pclosedata)
		blr
/*
 *	general clip routine
 *	a clip mask and routine pointer
 *	are passed in.
 */
.do_clip:
		mflr		savelr2
		crclr		crlastbit
		and.		itemp0,someout,mask
		beq		.clip_done
		bl		.swap
		bl		.load
		bt		crnewbit,.clip_store
		b		.clip_next
.clip_loop:
		bl		.load
		cror		cr0eq,croldbit,crnewbit
		bf		cr0eq,.clip_next		/* both out */
		crand		cr0eq,croldbit,crnewbit
		bt		cr0eq,.clip_store		/* both in */
/*	call clip rountine	*/
		mtlr		routineptr
		blr
.clip_return:
		bf		crnewbit,.clip_next
.clip_store:
		bt		crlastbit,.clip_done
		stwu		newptr,4(destptr)
		addi		count,count,1
.clip_next:
		mr		oldptr,newptr
		crmove		croldbit,crnewbit
		bdnz+		.clip_loop
		bt		crlastbit,.clip_done
		mr		srcptr,lastsrc
		crset		crlastbit
		mtctr		int_one
		b		.clip_loop
.clip_done:
		xor.		someout,someout,itemp0
		cmpi		1,count,3
		mtlr		savelr2
		blr
/*
 *	clip routines pointed to by routine pointer
 */
.clip_wmin:
		lfs		fcwc,CloseData.fwclose_off(pclosedata)
		bl		.clipload

		fsubs		fclipratio,fcw1,fcwc
		fdivs		fclipratio,fclipratio,fcw10
		fnmsubs		fcxc,fclipratio,fcx10,fcx1
		fnmsubs		fcyc,fclipratio,fcy10,fcy1
		b		.clip_common2
.clip_xmin:
		lfs		fcxc,CloseData.fconst0pt0_off(pclosedata)
		bl		.clipload
		b		.clipx
.clip_xmax:
		lfs		fcxc,CloseData.fscreenwidth_off(pclosedata)
		bl		.clipload
.clipx:
		fsubs		fclipratio,fcx1,fcxc
		fdivs		fclipratio,fclipratio,fcx10

		fnmsubs		fcyc,fclipratio,fcy10,fcy1
		fnmsubs		fcwc,fclipratio,fcw10,fcw1
		b		.clip_common
.clip_ymin:
		lfs		fcyc,CloseData.fconst0pt0_off(pclosedata)
		bl		.clipload
		b		.clipy
.clip_ymax:
		lfs		fcyc,CloseData.fscreenheight_off(pclosedata)
		bl		.clipload
.clipy:
		fsubs		fclipratio,fcy1,fcyc
		fdivs		fclipratio,fclipratio,fcy10

		fnmsubs		fcxc,fclipratio,fcx10,fcx1
		fnmsubs		fcwc,fclipratio,fcw10,fcw1
.clip_common:
		bl		.clip_test
.clip_common2:
		fnmsubs		fcrc,fclipratio,fcr10,fcr1
		fnmsubs		fcgc,fclipratio,fcg10,fcg1
		fnmsubs		fcbc,fclipratio,fcb10,fcb1
		fnmsubs		fcac,fclipratio,fca10,fca1
		fnmsubs		fcuc,fclipratio,fcu10,fcu1
		addi		count,count,1
		bl		.storevertex
		b		.clip_return
/*
 *	load vertices and generate deltas
 */
.clipload:
		lfs		fcx0,vertex.x(oldptr)
		lfs		fcy0,vertex.y(oldptr)
		lfs		fcw0,vertex.w(oldptr)
		lfs		fcr0,vertex.r(oldptr)
		lfs		fcg0,vertex.g(oldptr)
		lfs		fcb0,vertex.b(oldptr)
		lfs		fca0,vertex.a(oldptr)
		lfs		fcu0,vertex.u(oldptr)
		lfs		fcx1,vertex.x(newptr)
		lfs		fcy1,vertex.y(newptr)
		lfs		fcw1,vertex.w(newptr)
		lfs		fcr1,vertex.r(newptr)
		lfs		fcg1,vertex.g(newptr)
		lfs		fcb1,vertex.b(newptr)
		lfs		fca1,vertex.a(newptr)
		lfs		fcu1,vertex.u(newptr)
		fsubs		fcx10,fcx1,fcx0
		fsubs		fcy10,fcy1,fcy0
		fsubs		fcw10,fcw1,fcw0
		fsubs		fcr10,fcr1,fcr0
		fsubs		fcg10,fcg1,fcg0
		fsubs		fcb10,fcb1,fcb0
		fsubs		fca10,fca1,fca0
		fsubs		fcu10,fcu1,fcu0
		blr
/*
 *	copy verts into clipdata buffer
 */
.copyverts:
		lfs		fcxc,xformtr.xp(srcptr)
		lfs		fcyc,xformtr.yp(srcptr)
		lfs		fcwc,xformtr.wp(srcptr)
		lfs		fcrc,xformtr.r(srcptr)
		lfs		fcgc,xformtr.g(srcptr)
		lfs		fcbc,xformtr.b(srcptr)
		lfs		fcac,xformtr.a(srcptr)
		lwz		flags,xformtr.flags(srcptr)
/*
 *	store vertex to clipdata buffer
 */
.storevertex:
		stfsu		fcxc,4(storeptr)
		stwu		storeptr,4(destptr)	/* store pointer to vertex */
		stfsu		fcyc,4(storeptr)
		stfsu		fcwc,4(storeptr)
		stfsu		fcrc,4(storeptr)
		stfsu		fcgc,4(storeptr)
		stfsu		fcbc,4(storeptr)
		stfsu		fcac,4(storeptr)
		stwu		flags,4(storeptr)
		stfsu		fcuc,4(storeptr)
		blr
/*
 *	swap buffers
 */
.swap:
		mtctr		count
		mr		destptr,lastsrc
		mr		srcptr,lastdest
		mr		lastdest,destptr
		mr		lastsrc,srcptr
		li		count,0
		blr
/*
 *	load vertex pointer and test flags
 */
.load:
		lwzu		newptr,4(srcptr)
		lwz		flags,vertex.flags(newptr)
		and.		flags,flags,mask
		crmove		crnewbit,cr0eq
		blr
/*
 *	generate the clip flags
 */
.clip_test:
		lfs		fcw1,CloseData.fconst0pt0_off(pclosedata)
		lfs		fcx1,CloseData.fscreenwidth_off(pclosedata)
		lfs		fcy1,CloseData.fscreenheight_off(pclosedata)
		fcmpu		0,fcw1,fcxc		/* if 0.0 > xc */
		fcmpu		1,fcxc,fcx1		/* if xc > width */
		fcmpu		2,fcw1,fcyc		/* if 0.0 > yc */
		fcmpu		3,fcyc,fcy1		/* if yc > height */
		mfcr		flags
		andis.		flags,flags,0x4444
		blr
M_ClistManager:
		lea	3,M_ClistManagerC
		b	M_SaveAndCallC
endcode:
		.space		4096-(endcode-begincode)

/*	END  */ 


