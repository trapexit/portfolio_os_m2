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
	DECFN	M_DrawDynLitTexAA
begincode:
		mflr		3

		stw		3,CloseData.drawlrsave_off(pclosedata)
/*
 * save the result of bounding box checking for pass 2
 */
		mfcr		itemp1
		lwz		itemp0,Pod.flags_off(ppod)
		ori		itemp0,itemp0,0xc0	/* set aa2ndpassFLAG & aapassbbtestFLAG */
		rlwimi		itemp0,itemp1,(29-26),26,27	/* save nocullFLAG & clipFLAG */
		stw		itemp0,Pod.flags_off(ppod)		/* save pod flags */
		stw		ppod,CloseData.ppodsave_off(pclosedata)
/* clear aa edge buffer */
		lwz		itemp0,Pod.paadata_off(ppod)
		cmpi		0,itemp0,0
		bne		0,.yesaa
		b		M_DrawDynLitTex
.yesaa:	
		lwz		itemp1,AAData.paaedgebuffer_off(itemp0)
		lhz		itemp0,AAData.edgecount_off(itemp0)
		addi		itemp0,itemp0,3
		srwi		itemp0,itemp0,2
		mtctr		itemp0
		li		itemp0,0
		subi		itemp1,itemp1,4
.clearaaedgebuf:
		stwu		itemp0,4(itemp1)
		bdnz		.clearaaedgebuf

		lwz		pfirstlight,CloseData.convlightdata_off(pclosedata)
		lwz		pvtx,PodGeometry.pvertex_off(pgeometry)
		lhz		itemp0,PodGeometry.vertexcount_off(pgeometry)
		li		iconst32,32

		srwi		itemp0,itemp0,1
		subi		pvtx,pvtx,4
		mtctr		itemp0
		addi		pxdata,pxformbase,(2*xform)		/* pxdata is same reg as pgeometry! */
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
		fnmsubs		fconst2pt0,fwinv1,fwp1,fconst2pt0	/* fconst2pt0-fwinv1*fwp1Ê*/
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

		stfs		fxpp0,xform.xpp-(2*xform)(pxdata)
		stfs		fypp0,xform.ypp-(2*xform)(pxdata)
		stfs		fwinv0,xform.winv-(2*xform)(pxdata)
		stfs		fxpp1,xform.xpp-xform(pxdata)
		stfs		fypp1,xform.ypp-xform(pxdata)
		stfs		fwinv1,xform.winv-xform(pxdata)
		bf		clipFLAG,.setaabits

/* if clipping, store pre-projected points and check clip bounds */

		stfs		fxp0,xform.xp-(2*xform)(pxdata)
		stfs		fyp0,xform.yp-(2*xform)(pxdata)
		stfs		fwp0,xform.wp-(2*xform)(pxdata)
		stfs		fxp1,xform.xp-xform(pxdata)
		stfs		fyp1,xform.yp-xform(pxdata)
		stfs		fwp1,xform.wp-xform(pxdata)

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
		stw		clip0,xform.flags-(2*xform)(pxdata)
		stw		clip1,xform.flags-xform(pxdata)
		b		.loadrgb
.setaabits:
		li		itemp0,0
		stw		itemp0,xform.flags-(2*xform)(pxdata)
		stw		itemp0,xform.flags-xform(pxdata)
.loadrgb:
		lfs		fracc0,CloseData.frbase_off(pclosedata)
		lfs		fgacc0,CloseData.fgbase_off(pclosedata)
		lfs		fbacc0,CloseData.fbbase_off(pclosedata)
		lfs		faacc0,CloseData.fabase_off(pclosedata)
		lfs		fracc1,CloseData.frbase_off(pclosedata)
		lfs		fgacc1,CloseData.fgbase_off(pclosedata)
		lfs		fbacc1,CloseData.fbbase_off(pclosedata)
		lfs		faacc1,CloseData.fabase_off(pclosedata)

		li		itemp0,64
		dcbt		pxdata,itemp0

		mtlr		pfirstlight

		addi		plightlist,pclosedata,CloseData.convlightdata_off

		blr

	DECFN	M_LightReturnDynLitTexAA

		lwz		itemp1,CloseData.ppodsave_off(pclosedata)
		lwz		itemp0,Pod.paadata_off(itemp1)
		cmpi		0,itemp0,0
		bne		0,.yesaaret
		b		M_LightReturnDynLitTex
.yesaaret:	

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

		stfs		fracc0,xform.r-(2*xform)(pxdata)
		stfs		fgacc0,xform.g-(2*xform)(pxdata)
		stfs		fbacc0,xform.b-(2*xform)(pxdata)
		stfs		faacc0,xform.a-(2*xform)(pxdata)

		stfs		fracc1,xform.r-xform(pxdata)
		stfs		fgacc1,xform.g-xform(pxdata)
		stfs		fbacc1,xform.b-xform(pxdata)
		stfs		faacc1,xform.a-xform(pxdata)

		addi		pxdata,pxdata,(2*xform)
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
		mulli		pxvert,pxvert,xform
		mulli		pxcol,pxcol,xform
		add		pxvert,pxvert,pxformbase
		add		pxcol,pxcol,pxformbase

		lfs		fxpp0,xform.xpp(pxvert)
		lfs		fypp0,xform.ypp(pxvert)
		lfs		fwinv0,xform.winv(pxvert)

		bf		clipFLAG,.storesharedrgb

		lfs		fxp0,xform.xp(pxvert)
		lfs		fyp0,xform.yp(pxvert)
		lfs		fwp0,xform.wp(pxvert)
		lwz		clip0,xform.flags(pxvert)

		stfs		fxp0,xform.xp-(2*xform)(pxdata)
		stfs		fyp0,xform.yp-(2*xform)(pxdata)
		stfs		fwp0,xform.wp-(2*xform)(pxdata)
		stw		clip0,xform.flags-(2*xform)(pxdata)
.storesharedrgb:
		stfs		fxpp0,xform.xpp-(2*xform)(pxdata)
		stfs		fypp0,xform.ypp-(2*xform)(pxdata)
		stfs		fwinv0,xform.winv-(2*xform)(pxdata)
		lfs		fracc0,xform.r(pxcol)
		lfs		fgacc0,xform.g(pxcol)
		lfs		fbacc0,xform.b(pxcol)
		lfs		faacc0,xform.a(pxcol)
		stfs		fracc0,xform.r-(2*xform)(pxdata)
		stfs		fgacc0,xform.g-(2*xform)(pxdata)
		stfs		fbacc0,xform.b-(2*xform)(pxdata)
		stfs		faacc0,xform.a-(2*xform)(pxdata)

		addi		pxdata,pxdata,xform
		bdnz		.sharedloop

/***************************************************
 *
 *	START PASS 2 code
 *
 ***************************************************/
.startpass2:
		lwz		itemp1,CloseData.ppodsave_off(pclosedata)
		lwz		itemp0,Pod.pgeometry_off(itemp1)	
		lwz		pindex,PodGeometry.pindex_off(itemp0)
		lwz		puv,PodGeometry.puv_off(itemp0)
		lwz		paaedge,Pod.paadata_off(itemp1)
		lwz		paaedgebuf,AAData.paaedgebuffer_off(paaedge)
		lwz		paaedge,PodGeometry.paaedge_off(itemp0)
		subi		puv,puv,4
		addi		tltabnodraw,pclosedata,(CloseData.tltable_off + 11)
		addi		tltabdraw1,pclosedata,(CloseData.tltable_off + 6)
		addi		tltabdraw3,pclosedata,(CloseData.tltable_off - 3)
		lhz		itemp0,0(pindex)
		lhz		edgeidx,0(paaedge)

		li		iconstVI3,2
		addis		iconstVI3,iconstVI3,0x200f
		addis		iconstVI1,0,0x2007

		li		tloffset,-1			/* will become 0x1c */

		mtcrf		0x0c,itemp0			/* CR4 and CR5 */
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xform
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
		blt-		startantialiaspass2

selecttexture:
/*	Copy snippet itemp0 in ptexselsnippets */
		lwz		ptexselsnippets,CloseData.ptexselsnippets_off(pclosedata)
		mulli		itemp0,itemp0,MSnippet
		add		itemp0,ptexselsnippets,itemp0
		lfs		fduscale,MSnippet.uscale(itemp0)
		lfs		fdvscale,MSnippet.vscale(itemp0)
		lhz		itemp1,MSnippet.snippet+CltSnippet.size_off(itemp0)	/* count of words in snippet */
		lwz		itemp0,MSnippet.snippet+CltSnippet.data_off(itemp0)	/* pointer to VI */
		mtctr		itemp1
		addi		itemp0,itemp0,-4
.Lloop:		
		lwzu		itemp1,4(itemp0)
		stwu		itemp1,4(pVI)
		bdnz		.Lloop

		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
notextureselect:
		lhau		itemp0,2(pindex)
		mr		edgeidx0,edgeidx
		lhzu		edgeidx1,2(paaedge)

		add		vtxoffset0,nextvtxoffset,pxformbase
		andi.		vtxoffset1,itemp0,0x7ff
		lhau		itemp0,2(pindex)
		lhzu		edgeidx,2(paaedge)

		mulli		vtxoffset1,vtxoffset1,xform
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xform
		add		vtxoffset1,vtxoffset1,pxformbase

		lfsu		fdu0,4(puv)
		lfsu		fdv0,4(puv)
		lfsu		fdu1,4(puv)
		lfsu		fdv1,4(puv)

		lfs		fdxp0,xform.xpp(vtxoffset0)
		lfs		fdyp0,xform.ypp(vtxoffset0)
		lfs		fdxp1,xform.xpp(vtxoffset1)
		lfs		fdyp1,xform.ypp(vtxoffset1)

		fmuls		fdu0,fdu0,fduscale
		fmuls		fdv0,fdv0,fdvscale
		fmuls		fdu1,fdu1,fduscale
		fmuls		fdv1,fdv1,fdvscale

		crset		aafirsttriFLAG
		crclr		aaalterprimFLAG
		b		start012specialentry

/***************************************************
 *
 *	stitch code
 *
 ***************************************************/

/* came from 012 */
start021iffan:
		crclr		aafirsttriFLAG

		bf		curfanFLAG,start120
		bt		startnewstripFLAG,startstripfannodraw
start021:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load1
		bf		cr0gt,start012iffan

		bl		markexternedge021

		bf		aanodrawFLAG,startdraw021
		bf		curfanFLAG,start210
		bt		startnewstripFLAG,marklastedge021
		b		start012
startdraw021:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw0
		bl		draw2
		bl		draw1
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont210
		bt		startnewstripFLAG,marklastedge021
cont012:
		bl		load2
		bf		cr0gt,start021iffan

		bl		markexternedge012

		bf		aanodrawFLAG,contdraw012
		bf		curfanFLAG,start120
		bt		startnewstripFLAG,marklastedge012
		b		start021
contdraw012:

		bl		draw2single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont120
		bt		startnewstripFLAG,marklastedge012
		b		cont021

/* came from 021 */
start012iffan:
		crclr	aafirsttriFLAG

		bf		curfanFLAG,start210
		bt		startnewstripFLAG,startstripfannodraw
start012:
		lbzx		tloffset,tltabnodraw,tloffset
start012specialentry:
		bl		load2
		bf		cr0gt,start021iffan

		bf		aafirsttriFLAG,markfirstedgedone
		crclr		aafirsttriFLAG
		bl		markedgebit0
markfirstedgedone:

		bl		markexternedge012

		bf		aanodrawFLAG,startdraw012
		bf		curfanFLAG,start120
		bt		startnewstripFLAG,marklastedge012
		b		start021
startdraw012:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw0
		bl		draw1
		bl		draw2
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont120
		bt		startnewstripFLAG,marklastedge012
cont021:
		bl		load1
		bf		cr0gt,start012iffan

		bl		markexternedge021

		bf		aanodrawFLAG,contdraw021
		bf		curfanFLAG,start210
		bt		startnewstripFLAG,marklastedge021
		b		start012
contdraw021:

		bl		draw1single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont210
		bt		startnewstripFLAG,marklastedge021
		b		cont012

/* came from 102 */
start120iffan:
		crclr		aafirsttriFLAG

		bf		curfanFLAG,start021
		bt		startnewstripFLAG,startstripfannodraw
start120:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load0
		bf		cr0gt,start102iffan

		bl		markexternedge120

		bf		aanodrawFLAG,startdraw120
		bf		curfanFLAG,start201
		bt		startnewstripFLAG,marklastedge120
		b		start102
startdraw120:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw1
		bl		draw2
		bl		draw0
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont201
		bt		startnewstripFLAG,marklastedge120
cont102:
		bl		load2
		bf		cr0gt,start120iffan

		bl		markexternedge102

		bf		aanodrawFLAG,contdraw102
		bf		curfanFLAG,start021
		bt		startnewstripFLAG,marklastedge102
		b		start120
contdraw102:

		bl		draw2single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont021
		bt		startnewstripFLAG,marklastedge102
		b		cont120

/* came from 120 */
start102iffan:
		crclr		aafirsttriFLAG

		bf		curfanFLAG,start201
		bt		startnewstripFLAG,startstripfannodraw
start102:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load2
		bf		cr0gt,start120iffan

		bl		markexternedge102

		bf		aanodrawFLAG,startdraw102
		bf		curfanFLAG,start021
		bt		startnewstripFLAG,marklastedge102
		b		start120
startdraw102:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw1
		bl		draw0
		bl		draw2
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont021
		bt		startnewstripFLAG,marklastedge102
cont120:
		bl		load0
		bf		cr0gt,start102iffan

		bl		markexternedge120

		bf		aanodrawFLAG,contdraw120
		bf		curfanFLAG,start201
		bt		startnewstripFLAG,marklastedge120
		b		start102
contdraw120:

		bl		draw0single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont201
		bt		startnewstripFLAG,marklastedge120
		b		cont102

/* came from 201 */
start210iffan:
		crclr		aafirsttriFLAG

		bf		curfanFLAG,start012
		bt		startnewstripFLAG,startstripfannodraw
start210:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load0
		bf		cr0gt,start201iffan

		bl		markexternedge210

		bf		aanodrawFLAG,startdraw210
		bf		curfanFLAG,start102
		bt		startnewstripFLAG,marklastedge210
		b		start201
startdraw210:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw2
		bl		draw1
		bl		draw0
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont102
		bt		startnewstripFLAG,marklastedge210
cont201:
		bl		load1
		bf		cr0gt,start210iffan

		bl		markexternedge201

		bf		aanodrawFLAG,contdraw201
		bf		curfanFLAG,start012
		bt		startnewstripFLAG,marklastedge201
		b		start210
contdraw201:

		bl		draw1single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont012
		bt		startnewstripFLAG,marklastedge201
		b		cont210

/* came from 210 */
start201iffan:
		crclr		aafirsttriFLAG

		bf		curfanFLAG,start102
		bt		startnewstripFLAG,startstripfannodraw
start201:
		lbzx		tloffset,tltabnodraw,tloffset
		bl		load1
		bf		cr0gt,start210iffan

		bl		markexternedge201

		bf		aanodrawFLAG,startdraw201
		bf		curfanFLAG,start012
		bt		startnewstripFLAG,marklastedge201
		b		start210
startdraw201:

		lbzx		tloffset,tltabdraw3,tloffset
		stwu		iconstVI3,4(pVI)
		bl		draw2
		bl		draw0
		bl		draw1
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont012
		bt		startnewstripFLAG,marklastedge201
cont210:
		bl		load0
		bf		cr0gt,start201iffan

		bl		markexternedge210

		bf		aanodrawFLAG,contdraw210
		bf		curfanFLAG,start102
		bt		startnewstripFLAG,marklastedge210
		b		start201
contdraw210:

		bl		draw0single
		lbzx		tloffset,tltabdraw1,tloffset
		addi		tricount,tricount,1		/* TESTING */
		cmpl		0,pVI,pVIwritemax			/* chs */
		bgel-		M_ClistManager
		bf		curfanFLAG,cont102
		bt		startnewstripFLAG,marklastedge210
		b		cont201

/***************************************************
 *
 *	load code
 *
 ***************************************************/

load0:
		add		vtxoffset0,nextvtxoffset,pxformbase

		lfs		fdxp0,xform.xpp(vtxoffset0)
		lfs		fdyp0,xform.ypp(vtxoffset0)

		lfsu		fdu0,4(puv)
		lfsu		fdv0,4(puv)
		fmuls		fdu0,fdu0,fduscale
		fmuls		fdv0,fdv0,fdvscale

		mr		edgeidx0,edgeidx
		b		loadcont
load1:
		add		vtxoffset1,nextvtxoffset,pxformbase

		lfs		fdxp1,xform.xpp(vtxoffset1)
		lfs		fdyp1,xform.ypp(vtxoffset1)

		lfsu		fdu1,4(puv)
		lfsu		fdv1,4(puv)
		fmuls		fdu1,fdu1,fduscale
		fmuls		fdv1,fdv1,fdvscale

		mr		edgeidx1,edgeidx
		b		loadcont
load2:
		add		vtxoffset2,nextvtxoffset,pxformbase

		lfs		fdxp2,xform.xpp(vtxoffset2)
		lfs		fdyp2,xform.ypp(vtxoffset2)

		lfsu		fdu2,4(puv)
		lfsu		fdv2,4(puv)
		fmuls		fdu2,fdu2,fduscale
		fmuls		fdv2,fdv2,fdvscale

		mr		edgeidx2,edgeidx
loadcont:
		dcbt		pVI,tloffset
		srawi		tloffset,tloffset,3

		lhau		itemp0,2(pindex)
		lhzu		edgeidx,2(paaedge)
		andi.		nextvtxoffset,itemp0,0x7ff
		rlwimi		iconstVI1,itemp0,(17-11),11,11
		mulli		nextvtxoffset,nextvtxoffset,xform

/*	set flags CR4 and CR5 from itemp0 */

		mtcrf		0x0c,itemp0

		crxor		aaalterprimFLAG,curfanFLAG,prevfanFLAG

		crclr		aanodrawFLAG
		bf		clipFLAG,.noclip2

		lwz		iclipflags0,xform.flags(vtxoffset0)
		lwz		iclipflags1,xform.flags(vtxoffset1)
		or		someout,iclipflags0,iclipflags1
		and		itemp0,iclipflags0,iclipflags1
		lwz		iclipflags1,xform.flags(vtxoffset2)
		or.		someout,someout,iclipflags1
		and		itemp0,itemp0,iclipflags1
		bne		0,.clip
		crclr		aanodrawFLAG
.noclip2:
		bt		nocullFLAG,.nocull
		fsubs		fdtemp0,fdxp1,fdxp0
		fsubs		fdtemp1,fdyp2,fdyp0
		fsubs		fdtemp2,fdxp2,fdxp0
		fsubs		fdtemp3,fdyp1,fdyp0

		fmuls		fdtemp0,fdtemp0,fdtemp1
		fmuls		fdtemp2,fdtemp2,fdtemp3

		fcmpu		0,fdtemp2,fdtemp0			/* chs */
		beq			.nocull			/* zero tri */
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
		crset		aanodrawFLAG
		b		.noclip2

/***************************************************
 *
 *	draw code
 *
 ***************************************************/

draw0single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw0 */

draw0:	
		lfs		fdwinv,xform.winv(vtxoffset0)
		fmuls		fdu,fdu0,fdwinv
		fmuls		fdv,fdv0,fdwinv

		lfs		fdtemp0,xform.r(vtxoffset0)
		lfs		fdtemp1,xform.g(vtxoffset0)
		lfs		fdtemp2,xform.b(vtxoffset0)
		lfs		fdtemp3,xform.a(vtxoffset0)
	
		stfsu		fdxp0,4(pVI)
		stfsu		fdyp0,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu,4(pVI)
		stfsu		fdv,4(pVI)
		blr

draw1single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw1 */

draw1:	
		lfs		fdwinv,xform.winv(vtxoffset1)
		fmuls		fdu,fdu1,fdwinv
		fmuls		fdv,fdv1,fdwinv

		lfs		fdtemp0,xform.r(vtxoffset1)
		lfs		fdtemp1,xform.g(vtxoffset1)
		lfs		fdtemp2,xform.b(vtxoffset1)
		lfs		fdtemp3,xform.a(vtxoffset1)
	
		stfsu		fdxp1,4(pVI)
		stfsu		fdyp1,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu,4(pVI)
		stfsu		fdv,4(pVI)
		blr

draw2single:	

		stwu		iconstVI1,4(pVI)
/*	fall thru to draw2 */

draw2:	
		lfs		fdwinv,xform.winv(vtxoffset2)
		fmuls		fdu,fdu2,fdwinv
		fmuls		fdv,fdv2,fdwinv

		lfs		fdtemp0,xform.r(vtxoffset2)
		lfs		fdtemp1,xform.g(vtxoffset2)
		lfs		fdtemp2,xform.b(vtxoffset2)
		lfs		fdtemp3,xform.a(vtxoffset2)
	
		stfsu		fdxp2,4(pVI)
		stfsu		fdyp2,4(pVI)
		stfsu		fdtemp0,4(pVI)
		stfsu		fdtemp1,4(pVI)
		stfsu		fdtemp2,4(pVI)
		stfsu		fdtemp3,4(pVI)
		stfsu		fdwinv,4(pVI)
		stfsu		fdu,4(pVI)
		stfsu		fdv,4(pVI)
		blr

/*
 * set anti_aliasing bits
 */
markedgebit0:
		lbzx	itemp0,edgeidx0,paaedgebuf
		cmpi	0,itemp0,1
		bgtlr
		addi	itemp0,itemp0,1
		stbx	itemp0,edgeidx0,paaedgebuf
		blr

markedgebit1:
		lbzx	itemp0,edgeidx1,paaedgebuf
		cmpi	0,itemp0,1
		bgtlr		
		addi	itemp0,itemp0,1
		stbx	itemp0,edgeidx1,paaedgebuf
		blr

markedgebit2:
		lbzx	itemp0,edgeidx2,paaedgebuf
		cmpi	0,itemp0,1
		bgtlr
		addi	itemp0,itemp0,1
		stbx	itemp0,edgeidx2,paaedgebuf
		blr

markexternedge012:
		bt		prevfanFLAG,markexternedge012fan
		bt		prevclockwiseFLAG,markexternedge012stripcw
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit1	/* strip, ccw, alterprim     */
		b		markedgebit2					/* strip, ccw, alterprim not */
markexternedge012stripcw:
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit2	/* strip,  cw, alterprim     */
		b		markedgebit0					/* strip,  cw, alterprim not */
markexternedge012fan:
		bt		prevclockwiseFLAG,markexternedge012fancw
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit2	/*   fan, ccw, alterprim     */
		b		markedgebit1					/*   fan, ccw, alterprim not */
markexternedge012fancw:
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit0	/*   fan,  cw, alterprim     */
		b		markedgebit2					/*   fan,  cw, alterprim not */

markexternedge021:
		bt		prevfanFLAG,markexternedge021fan
		bt		prevclockwiseFLAG,markexternedge021stripcw
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit1	/* strip, ccw, alterprim     */
		b		markedgebit0					/* strip, ccw, alterprim not */
markexternedge021stripcw:
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit2	/* strip,  cw, alterprim     */
		b		markedgebit1					/* strip,  cw, alterprim not */
markexternedge021fan:
		bt		prevclockwiseFLAG,markexternedge021fancw
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit0	/*   fan, ccw, alterprim     */
		b		markedgebit1					/*   fan, ccw, alterprim not */
markexternedge021fancw:
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit1	/*   fan,  cw, alterprim     */
		b		markedgebit2					/*   fan,  cw, alterprim not */

markexternedge102:
		bt		prevfanFLAG,markexternedge102fan
		bt		prevclockwiseFLAG,markexternedge102stripcw
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit2	/* strip, ccw, alterprim     */
		b		markedgebit1					/* strip, ccw, alterprim not */
markexternedge102stripcw:
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit0	/* strip,  cw, alterprim     */
		b		markedgebit2					/* strip,  cw, alterprim not */
markexternedge102fan:
		bt		prevclockwiseFLAG,markexternedge102fancw
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit1	/*   fan, ccw, alterprim     */
		b		markedgebit2					/*   fan, ccw, alterprim not */
markexternedge102fancw:
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit2	/*   fan,  cw, alterprim     */
		b		markedgebit0					/*   fan,  cw, alterprim not */

markexternedge120:
		bt		prevfanFLAG,markexternedge120fan
		bt		prevclockwiseFLAG,markexternedge120stripcw
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit2	/* strip, ccw, alterprim     */
		b		markedgebit0					/* strip, ccw, alterprim not */
markexternedge120stripcw:
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit0	/* strip,  cw, alterprim     */
		b		markedgebit1					/* strip,  cw, alterprim not */
markexternedge120fan:
		bt		prevclockwiseFLAG,markexternedge120fancw
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit0	/*   fan, ccw, alterprim     */
		b		markedgebit2					/*   fan, ccw, alterprim not */
markexternedge120fancw:
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit1	/*   fan,  cw, alterprim     */
		b		markedgebit0					/*   fan,  cw, alterprim not */

markexternedge201:
		bt		prevfanFLAG,markexternedge201fan
		bt		prevclockwiseFLAG,markexternedge201stripcw
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit0	/* strip, ccw, alterprim     */
		b		markedgebit1					/* strip, ccw, alterprim not */
markexternedge201stripcw:
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit1	/* strip,  cw, alterprim     */
		b		markedgebit2					/* strip,  cw, alterprim not */
markexternedge201fan:
		bt		prevclockwiseFLAG,markexternedge201fancw
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit1	/*   fan, ccw, alterprim     */
		b		markedgebit0					/*   fan, ccw, alterprim not */
markexternedge201fancw:
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit2	/*   fan,  cw, alterprim     */
		b		markedgebit1					/*   fan,  cw, alterprim not */

markexternedge210:
		bt		prevfanFLAG,markexternedge210fan
		bt		prevclockwiseFLAG,markexternedge210stripcw
		bt		startnewstripFLAG,markedgebit2
		bt		aaalterprimFLAG,markedgebit0	/* strip, ccw, alterprim     */
		b		markedgebit2					/* strip, ccw, alterprim not */
markexternedge210stripcw:
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit1	/* strip,  cw, alterprim     */
		b		markedgebit0					/* strip,  cw, alterprim not */
markexternedge210fan:
		bt		prevclockwiseFLAG,markexternedge210fancw
		bt		startnewstripFLAG,markedgebit0
		bt		aaalterprimFLAG,markedgebit2	/*   fan, ccw, alterprim     */
		b		markedgebit0					/*   fan, ccw, alterprim not */
markexternedge210fancw:
		bt		startnewstripFLAG,markedgebit1
		bt		aaalterprimFLAG,markedgebit0	/*   fan,  cw, alterprim     */
		b		markedgebit1					/*   fan,  cw, alterprim not */

marklastedge012:
		bt		prevfanFLAG,marklastedge012fan
		bt		prevclockwiseFLAG,marklastedge012stripcw
marklastedge012stripccw:
		bt		startnewstripFLAG,marklastedge012stripccwnew
		bt		aaalterprimFLAG,marklastedge012stripccwap
marklastedge012stripccwnew:
		bl		markedgebit1		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge012stripccwap:
		bl		markedgebit2		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge012stripcw:
		bt		startnewstripFLAG,marklastedge012stripcwnew
		bt		aaalterprimFLAG,marklastedge012stripcwap
marklastedge012stripcwnew:
		bl		markedgebit2		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge012stripcwap:
		bl		markedgebit0		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge012fan:
		bt		prevclockwiseFLAG,marklastedge012fancw
marklastedge012fanccw:
		bt		startnewstripFLAG,marklastedge012fanccwnew
		bt		aaalterprimFLAG,marklastedge012fanccwap
marklastedge012fanccwnew:
		bl		markedgebit2		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge012fanccwap:
		bl		markedgebit1		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge012fancw:
		bt		startnewstripFLAG,marklastedge012fancwnew
		bt		aaalterprimFLAG,marklastedge012fancwap
marklastedge012fancwnew:
		bl		markedgebit0		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge012fancwap:
		bl		markedgebit2		/*   fan,  cw, alterprim     */
		b		startstripfan

marklastedge021:
		bt		prevfanFLAG,marklastedge021fan
		bt		prevclockwiseFLAG,marklastedge021stripcw
marklastedge021stripccw:
		bt		startnewstripFLAG,marklastedge021stripccwnew
		bt		aaalterprimFLAG,marklastedge021stripccwap
marklastedge021stripccwnew:
		bl		markedgebit1		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge021stripccwap:
		bl		markedgebit0		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge021stripcw:
		bt		startnewstripFLAG,marklastedge021stripcwnew
		bt		aaalterprimFLAG,marklastedge021stripcwap
marklastedge021stripcwnew:
		bl		markedgebit2		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge021stripcwap:
		bl		markedgebit1		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge021fan:
		bt		prevclockwiseFLAG,marklastedge021fancw
marklastedge021fanccw:
		bt		startnewstripFLAG,marklastedge021fanccwnew
		bt		aaalterprimFLAG,marklastedge021fanccwap
marklastedge021fanccwnew:
		bl		markedgebit0		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge021fanccwap:
		bl		markedgebit1		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge021fancw:
		bt		startnewstripFLAG,marklastedge021fancwnew
		bt		aaalterprimFLAG,marklastedge021fancwap
marklastedge021fancwnew:
		bl		markedgebit1		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge021fancwap:
		bl		markedgebit2		/*   fan,  cw, alterprim     */
		b		startstripfan

marklastedge102:
		bt		prevfanFLAG,marklastedge102fan
		bt		prevclockwiseFLAG,marklastedge102stripcw
marklastedge102stripccw:
		bt		startnewstripFLAG,marklastedge102stripccwnew
		bt		aaalterprimFLAG,marklastedge102stripccwap
marklastedge102stripccwnew:
		bl		markedgebit2		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge102stripccwap:
		bl		markedgebit1		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge102stripcw:
		bt		startnewstripFLAG,marklastedge102stripcwnew
		bt		aaalterprimFLAG,marklastedge102stripcwap
marklastedge102stripcwnew:
		bl		markedgebit0		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge102stripcwap:
		bl		markedgebit2		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge102fan:
		bt		prevclockwiseFLAG,marklastedge102fancw
marklastedge102fanccw:
		bt		startnewstripFLAG,marklastedge102fanccwnew
		bt		aaalterprimFLAG,marklastedge102fanccwap
marklastedge102fanccwnew:
		bl		markedgebit1		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge102fanccwap:
		bl		markedgebit2		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge102fancw:
		bt		startnewstripFLAG,marklastedge102fancwnew
		bt		aaalterprimFLAG,marklastedge102fancwap
marklastedge102fancwnew:
		bl		markedgebit2		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge102fancwap:
		bl		markedgebit0		/*   fan,  cw, alterprim     */
		b		startstripfan

marklastedge120:
		bt		prevfanFLAG,marklastedge120fan
		bt		prevclockwiseFLAG,marklastedge120stripcw
marklastedge120stripccw:
		bt		startnewstripFLAG,marklastedge120stripccwnew
		bt		aaalterprimFLAG,marklastedge120stripccwap
marklastedge120stripccwnew:
		bl		markedgebit2		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge120stripccwap:
		bl		markedgebit0		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge120stripcw:
		bt		startnewstripFLAG,marklastedge120stripcwnew
		bt		aaalterprimFLAG,marklastedge120stripcwap
marklastedge120stripcwnew:
		bl		markedgebit0		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge120stripcwap:
		bl		markedgebit1		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge120fan:
		bt		prevclockwiseFLAG,marklastedge120fancw
marklastedge120fanccw:
		bt		startnewstripFLAG,marklastedge120fanccwnew
		bt		aaalterprimFLAG,marklastedge120fanccwap
marklastedge120fanccwnew:
		bl		markedgebit0		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge120fanccwap:
		bl		markedgebit2		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge120fancw:
		bt		startnewstripFLAG,marklastedge120fancwnew
		bt		aaalterprimFLAG,marklastedge120fancwap
marklastedge120fancwnew:
		bl		markedgebit1		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge120fancwap:
		bl		markedgebit0		/*   fan,  cw, alterprim     */
		b		startstripfan

marklastedge201:
		bt		prevfanFLAG,marklastedge201fan
		bt		prevclockwiseFLAG,marklastedge201stripcw
marklastedge201stripccw:
		bt		startnewstripFLAG,marklastedge201stripccwnew
		bt		aaalterprimFLAG,marklastedge201stripccwap
marklastedge201stripccwnew:
		bl		markedgebit0		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge201stripccwap:
		bl		markedgebit1		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge201stripcw:
		bt		startnewstripFLAG,marklastedge201stripcwnew
		bt		aaalterprimFLAG,marklastedge201stripcwap
marklastedge201stripcwnew:
		bl		markedgebit1		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge201stripcwap:
		bl		markedgebit2		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge201fan:
		bt		prevclockwiseFLAG,marklastedge201fancw
marklastedge201fanccw:
		bt		startnewstripFLAG,marklastedge201fanccwnew
		bt		aaalterprimFLAG,marklastedge201fanccwap
marklastedge201fanccwnew:
		bl		markedgebit1		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge201fanccwap:
		bl		markedgebit0		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge201fancw:
		bt		startnewstripFLAG,marklastedge201fancwnew
		bt		aaalterprimFLAG,marklastedge201fancwap
marklastedge201fancwnew:
		bl		markedgebit2		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge201fancwap:
		bl		markedgebit1		/*   fan,  cw, alterprim     */
		b		startstripfan

marklastedge210:
		bt		prevfanFLAG,marklastedge210fan
		bt		prevclockwiseFLAG,marklastedge210stripcw
marklastedge210stripccw:
		bt		startnewstripFLAG,marklastedge210stripccwnew
		bt		aaalterprimFLAG,marklastedge210stripccwap
marklastedge210stripccwnew:
		bl		markedgebit0		/* strip, ccw, alterprim not */
		b		startstripfan
marklastedge210stripccwap:
		bl		markedgebit2		/* strip, ccw, alterprim     */
		b		startstripfan
marklastedge210stripcw:
		bt		startnewstripFLAG,marklastedge210stripcwnew
		bt		aaalterprimFLAG,marklastedge210stripcwap
marklastedge210stripcwnew:
		bl		markedgebit1		/* strip,  cw, alterprim not */
		b		startstripfan
marklastedge210stripcwap:
		bl		markedgebit0		/* strip,  cw, alterprim     */
		b		startstripfan
marklastedge210fan:
		bt		prevclockwiseFLAG,marklastedge210fancw
marklastedge210fanccw:
		bt		startnewstripFLAG,marklastedge210fanccwnew
		bt		aaalterprimFLAG,marklastedge210fanccwap
marklastedge210fanccwnew:
		bl		markedgebit2		/*   fan, ccw, alterprim not */
		b		startstripfan
marklastedge210fanccwap:
		bl		markedgebit0		/*   fan, ccw, alterprim     */
		b		startstripfan
marklastedge210fancw:
		bt		startnewstripFLAG,marklastedge210fancwnew
		bt		aaalterprimFLAG,marklastedge210fancwap
marklastedge210fancwnew:
		bl		markedgebit0		/*   fan,  cw, alterprim not */
		b		startstripfan
marklastedge210fancwap:
		bl		markedgebit1		/*   fan,  cw, alterprim     */
		b		startstripfan

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
		stfs		14,CloseData.float14(pclosedata)
		stfs		15,CloseData.float15(pclosedata)
		stfs		16,CloseData.float16(pclosedata)
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
		lfs		fcvc,CloseData.float8(pclosedata)
		bl		.copyverts
		mr		srcptr,vtxoffset1
		lfs		fcuc,CloseData.float9(pclosedata)
		lfs		fcvc,CloseData.float10(pclosedata)
		bl		.copyverts
		mr		srcptr,vtxoffset2
		lfs		fcuc,CloseData.float11(pclosedata)
		lfs		fcvc,CloseData.float12(pclosedata)
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
		lfs		fcvc,vertex.v(newptr)
		fnmsubs		fcwc,fcw1,fcwc,fconst2pt0	/* fconst2pt0-(w1*wc) */
		fmuls		fcwc,fcw1,fcwc
		fmadds		fcxc,fcxc,fcwc,fconst12million
		fmadds		fcyc,fcyc,fcwc,fconst12million
		fsubs		fcxc,fcxc,fconst12million
		fsubs		fcyc,fcyc,fconst12million
		fmuls		fcuc,fcuc,fcwc
		fmuls		fcvc,fcvc,fcwc
		stfs		fcxc,vertex.x(newptr)
		stfs		fcyc,vertex.y(newptr)
		stfs		fcwc,vertex.w(newptr)
		stfs		fcuc,vertex.u(newptr)
		stfs		fcvc,vertex.v(newptr)
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
		lfs		fcv0,vertex.v(newptr)
		stfsu		fcx0,4(pVI)
		stfsu		fcy0,4(pVI)
		stfsu		fcr0,4(pVI)
		stfsu		fcg0,4(pVI)
		stfsu		fcb0,4(pVI)
		stfsu		fca0,4(pVI)
		stfsu		fcw0,4(pVI)
		stfsu		fcu0,4(pVI)
		stfsu		fcv0,4(pVI)		
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
		lfs		14,CloseData.float14(pclosedata)
		lfs		15,CloseData.float15(pclosedata)
		lfs		16,CloseData.float16(pclosedata)
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
		fnmsubs		fcvc,fclipratio,fcv10,fcv1
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
		lfs		fcv0,vertex.v(oldptr)
		lfs		fcx1,vertex.x(newptr)
		lfs		fcy1,vertex.y(newptr)
		lfs		fcw1,vertex.w(newptr)
		lfs		fcr1,vertex.r(newptr)
		lfs		fcg1,vertex.g(newptr)
		lfs		fcb1,vertex.b(newptr)
		lfs		fca1,vertex.a(newptr)
		lfs		fcu1,vertex.u(newptr)
		lfs		fcv1,vertex.v(newptr)
		fsubs		fcx10,fcx1,fcx0
		fsubs		fcy10,fcy1,fcy0
		fsubs		fcw10,fcw1,fcw0
		fsubs		fcr10,fcr1,fcr0
		fsubs		fcg10,fcg1,fcg0
		fsubs		fcb10,fcb1,fcb0
		fsubs		fca10,fca1,fca0
		fsubs		fcu10,fcu1,fcu0
		fsubs		fcv10,fcv1,fcv0
		blr
/*
 *	copy verts into clipdata buffer
 */
.copyverts:
		lfs		fcxc,xform.xp(srcptr)
		lfs		fcyc,xform.yp(srcptr)
		lfs		fcwc,xform.wp(srcptr)
		lfs		fcrc,xform.r(srcptr)
		lfs		fcgc,xform.g(srcptr)
		lfs		fcbc,xform.b(srcptr)
		lfs		fcac,xform.a(srcptr)
		lwz		flags,xform.flags(srcptr)
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
		stfsu		fcvc,4(storeptr)
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
/***************************************************
 *
 *	START ANTIALIASING PASS 2 code
 *
 ***************************************************/
startantialiaspass2:
		lwz		itemp1,CloseData.ppodsave_off(pclosedata)
		lwz		itemp0, Pod.paadata_off(itemp1)
		li		3,AA2NDPASS
		lwz		itemp0,AAData.flags_off(itemp0)
		or		3,3,itemp0
		stw		3,CloseData.aa_off(pclosedata)
		lwz		3,Pod.pcase_off(itemp1)
		bl		M_SaveAndCallC
		li		3,0
		stw		3,CloseData.aa_off(pclosedata)

		lwz		itemp0,Pod.flags_off(itemp1)
		mtcr		itemp0

		lwz		pgeometry,Pod.pgeometry_off(itemp1)
		lwz		pindex,PodGeometry.pindex_off(pgeometry)
		lwz		puv,PodGeometry.puv_off(pgeometry)
		lwz		paaedgebuf,Pod.paadata_off(itemp1)
		lwz		paaedge,PodGeometry.paaedge_off(pgeometry)
		lwz		paaedgebuf,AAData.paaedgebuffer_off(paaedgebuf)

		lhz		itemp0,0(pindex)
		lhz		edgeidx,0(paaedge)

		mtcrf		0x0c,itemp0			/* CR4 and CR5 */
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xform
/*
 *	nextvtxoffset set to first point's value
 *	flags set based on first point's index
 */
startstripfannodrawAA:
startstripfanAA:
		bf		selecttextureFLAG,notextureselectAA
		lhau		itemp0,2(pindex)
		cmpi		0,itemp0,0				/* chs */
		blt+		alldoneAA

selecttextureAA:
/*	Copy snippet itemp0 in ptexselsnippets */
		lwz		ptexselsnippets,CloseData.ptexselsnippets_off(pclosedata)
		mulli		itemp0,itemp0,MSnippet
		add		itemp0,ptexselsnippets,itemp0
		lfs		fduscale,MSnippet.uscale(itemp0)
		lfs		fdvscale,MSnippet.vscale(itemp0)
		lhz		itemp1,MSnippet.snippet+CltSnippet.size_off(itemp0)	/* count of words in snippet */
		lwz		itemp0,MSnippet.snippet+CltSnippet.data_off(itemp0)	/* pointer to VI */
		mtctr		itemp1
		addi		itemp0,itemp0,-4
.LloopAA:		
		lwzu		itemp1,4(itemp0)
		stwu		itemp1,4(pVI)
		bdnz		.LloopAA

		cmpl		0,pVI,pVIwritemax
		bgel-		M_ClistManager
notextureselectAA:
		lhau		itemp0,2(pindex)
		mr		edgeidx0,edgeidx
		lhzu		edgeidx1,2(paaedge)

		add		vtxoffset0,nextvtxoffset,pxformbase
		andi.		vtxoffset1,itemp0,0x7ff
		lhau		itemp0,2(pindex)
		lhzu		edgeidx,2(paaedge)

		mulli		vtxoffset1,vtxoffset1,xform
		andi.		nextvtxoffset,itemp0,0x7ff
		mulli		nextvtxoffset,nextvtxoffset,xform
		add		vtxoffset1,vtxoffset1,pxformbase

		lfs		fdxp0,xform.xpp(vtxoffset0)
		lfs		fdyp0,xform.ypp(vtxoffset0)
		lfs		fdxp1,xform.xpp(vtxoffset1)
		lfs		fdyp1,xform.ypp(vtxoffset1)

		mr		puv0,puv
		addi		puv1,puv,8
		addi		puv,puv,16

		crset		aafirsttriFLAG
		crclr		aatriculledFLAG
		crclr		aaalterprimFLAG
		b		start012specialentryAA

/***************************************************
 *
 *	stitch code
 *
 ***************************************************/

/* came from 012 */
start021iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge021AA
		bt		aatriculledFLAG,nodrawcullededge021AA
		bt		prevclockwiseFLAG,drawcullededge021AA
		mr		pdraw0AA,vtxoffset1	/* ccw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv1
		mr		puv1AA,puv0
		b		drawcullededge021doneAA
drawcullededge021AA:
		mr		pdraw0AA,vtxoffset0	/* cw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
drawcullededge021doneAA:
		bl		M_DrawAALineTex
nodrawcullededge021AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG

start021ifclipAA:
		bf		curfanFLAG,start120AA
		bt		startnewstripFLAG,startstripfannodrawAA
start021AA:
		bl		load1AA
		bf		cr0gt,start012iffanAA
		bt		aanodrawFLAG,start012ifclipAA

		bf		aatriculledFLAG,nodrawculledbackedge021AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge021AA
		mr		pdraw0AA,vtxoffset2	/* ccw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv2
		mr		puv1AA,puv0
		b		drawculledbackedge021doneAA
drawculledbackedge021AA:
		mr		pdraw0AA,vtxoffset0	/* cw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv0
		mr		puv1AA,puv2
drawculledbackedge021doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge021AA:

		bl		drawexternedge021AA

		bf		curfanFLAG,cont210AA
		bt		startnewstripFLAG,drawlastedge021AA
cont012AA:
		bl		load2AA
		bf		cr0gt,start021iffanAA
		bt		aanodrawFLAG,start021ifclipAA

		bl		drawexternedge012AA

		bf		curfanFLAG,cont120AA
		bt		startnewstripFLAG,drawlastedge012AA
		b		cont021AA

/* came from 021 */
start012iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge012AA
		bt		aatriculledFLAG,nodrawcullededge012AA
		bt		prevclockwiseFLAG,drawcullededge012AA
		mr		pdraw0AA,vtxoffset0	/* ccw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv0
		mr		puv1AA,puv2
		b		drawcullededge012doneAA
drawcullededge012AA:
		mr		pdraw0AA,vtxoffset2	/* cw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv2
		mr		puv1AA,puv0
drawcullededge012doneAA:
		bl		M_DrawAALineTex
nodrawcullededge012AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG
start012ifclipAA:
		bf		curfanFLAG,start210AA
		bt		startnewstripFLAG,startstripfannodrawAA
start012AA:
start012specialentryAA:
		bl		load2AA
		bf		cr0gt,start021iffanAA
		bt		aanodrawFLAG,start021ifclipAA

		bf		aafirsttriFLAG,nodrawfirstedgeAA
		lbzx		itemp0,edgeidx0,paaedgebuf
		cmpi		0,itemp0,1
		bne		nodrawfirstedgeAA
		mr		pdraw0AA,vtxoffset0		
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
		bl		M_DrawAALineTex
		b		nodrawculledbackedge012AA
nodrawfirstedgeAA:
		bf		aatriculledFLAG,nodrawculledbackedge012AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge012AA
		mr		pdraw0AA,vtxoffset0	/* ccw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
		b		drawculledbackedge012doneAA
drawculledbackedge012AA:
		mr		pdraw0AA,vtxoffset1	/* cw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv1
		mr		puv1AA,puv0
drawculledbackedge012doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge012AA:
		crclr		aafirsttriFLAG

		bl		drawexternedge012AA

		bf		curfanFLAG,cont120AA
		bt		startnewstripFLAG,drawlastedge012AA
cont021AA:
		bl		load1AA
		bf		cr0gt,start012iffanAA
		bt		aanodrawFLAG,start012ifclipAA

		bl		drawexternedge021AA

		bf		curfanFLAG,cont210AA
		bt		startnewstripFLAG,drawlastedge021AA
		b		cont012AA

/* came from 102 */
start120iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge120AA
		bt		aatriculledFLAG,nodrawcullededge120AA
		bt		prevclockwiseFLAG,drawcullededge120AA
		mr		pdraw0AA,vtxoffset1	/* ccw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv1
		mr		puv1AA,puv0
		b		drawcullededge120doneAA
drawcullededge120AA:
		mr		pdraw0AA,vtxoffset0	/* cw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
drawcullededge120doneAA:
		bl		M_DrawAALineTex
nodrawcullededge120AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG
start120ifclipAA:
		bf		curfanFLAG,start021AA
		bt		startnewstripFLAG,startstripfannodrawAA
start120AA:
		bl		load0AA
		bf		cr0gt,start102iffanAA
		bt		aanodrawFLAG,start102ifclipAA

		bf		aatriculledFLAG,nodrawculledbackedge120AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge120AA
		mr		pdraw0AA,vtxoffset1	/* ccw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv1
		mr		puv1AA,puv2
		b		drawculledbackedge120doneAA
drawculledbackedge120AA:
		mr		pdraw0AA,vtxoffset2	/* cw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv2
		mr		puv1AA,puv1
drawculledbackedge120doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge120AA:

		bl		drawexternedge120AA

		bf		curfanFLAG,cont201AA
		bt		startnewstripFLAG,drawlastedge120AA
cont102AA:
		bl		load2AA
		bf		cr0gt,start120iffanAA

		bt		aanodrawFLAG,start120ifclipAA

		bl		drawexternedge102AA

		bf		curfanFLAG,cont021AA
		bt		startnewstripFLAG,drawlastedge102AA
		b		cont120AA

/* came from 120 */
start102iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge102AA
		bt		aatriculledFLAG,nodrawcullededge102AA
		bt		prevclockwiseFLAG,drawcullededge102AA
		mr		pdraw0AA,vtxoffset2	/* ccw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv2
		mr		puv1AA,puv1
		b		drawcullededge102doneAA
drawcullededge102AA:
		mr		pdraw0AA,vtxoffset1	/* cw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv1
		mr		puv1AA,puv2
drawcullededge102doneAA:
		bl		M_DrawAALineTex
nodrawcullededge102AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG
start102ifclipAA:
		bf		curfanFLAG,start201AA
		bt		startnewstripFLAG,startstripfannodrawAA
start102AA:
		bl		load2AA
		bf		cr0gt,start120iffanAA
		bt		aanodrawFLAG,start120ifclipAA

		bf		aatriculledFLAG,nodrawculledbackedge102AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge102AA
		mr		pdraw0AA,vtxoffset0	/* ccw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
		b		drawculledbackedge102doneAA
drawculledbackedge102AA:
		mr		pdraw0AA,vtxoffset1	/* cw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv1
		mr		puv1AA,puv0
drawculledbackedge102doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge102AA:

		bl		drawexternedge102AA

		bf		curfanFLAG,cont021AA
		bt		startnewstripFLAG,drawlastedge102AA
cont120AA:
		bl		load0AA
		bf		cr0gt,start102iffanAA
		bt		aanodrawFLAG,start102ifclipAA

		bl		drawexternedge120AA

		bf		curfanFLAG,cont201AA
		bt		startnewstripFLAG,drawlastedge120AA
		b		cont102AA

/* came from 201 */
start210iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge210AA
		bt		aatriculledFLAG,nodrawcullededge210AA
		bt		prevclockwiseFLAG,drawcullededge210AA
		mr		pdraw0AA,vtxoffset0	/* ccw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv0
		mr		puv1AA,puv2
		b		drawcullededge210doneAA
drawcullededge210AA:
		mr		pdraw0AA,vtxoffset2	/* cw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv2
		mr		puv1AA,puv0
drawcullededge210doneAA:
		bl		M_DrawAALineTex
nodrawcullededge210AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG
start210ifclipAA:
		bf		curfanFLAG,start012AA
		bt		startnewstripFLAG,startstripfannodrawAA
start210AA:
		bl		load0AA
		bf		cr0gt,start201iffanAA
		bt		aanodrawFLAG,start201ifclipAA

		bf		aatriculledFLAG,nodrawculledbackedge210AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge210AA
		mr		pdraw0AA,vtxoffset1	/* ccw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv1
		mr		puv1AA,puv2
		b		drawculledbackedge210doneAA
drawculledbackedge210AA:
		mr		pdraw0AA,vtxoffset2	/* ccw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv2
		mr		puv1AA,puv1
drawculledbackedge210doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge210AA:

		bl		drawexternedge210AA

		bf		curfanFLAG,cont102AA
		bt		startnewstripFLAG,drawlastedge210AA
cont201AA:
		bl		load1AA
		bf		cr0gt,start210iffanAA
		bt		aanodrawFLAG,start210ifclipAA

		bl		drawexternedge201AA

		bf		curfanFLAG,cont012AA
		bt		startnewstripFLAG,drawlastedge201AA
		b		cont210AA

/* came from 210 */
start201iffanAA:
		bt		aafirsttriFLAG,nodrawcullededge201AA
		bt		aatriculledFLAG,nodrawcullededge201AA
		bt		prevclockwiseFLAG,drawcullededge201AA
		mr		pdraw0AA,vtxoffset2	/* ccw */
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv2
		mr		puv1AA,puv1
		b		drawcullededge201doneAA
drawcullededge201AA:
		mr		pdraw0AA,vtxoffset1	/* cw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv1
		mr		puv1AA,puv2
drawcullededge201doneAA:
		bl		M_DrawAALineTex
nodrawcullededge201AA:
		crset		aatriculledFLAG
		crclr		aafirsttriFLAG
start201ifclipAA:
		bf		curfanFLAG,start102AA
		bt		startnewstripFLAG,startstripfannodrawAA
start201AA:
		bl		load1AA
		bf		cr0gt,start210iffanAA
		bt		aanodrawFLAG,start210ifclipAA

		bf		aatriculledFLAG,nodrawculledbackedge201AA
		crclr		aatriculledFLAG
		bt		prevclockwiseFLAG,drawculledbackedge201AA
		mr		pdraw0AA,vtxoffset2	/* ccw */
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv2
		mr		puv1AA,puv0
		b		drawculledbackedge201doneAA
drawculledbackedge201AA:
		mr		pdraw0AA,vtxoffset0	/* cw */
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv0
		mr		puv1AA,puv2
drawculledbackedge201doneAA:
		bl		M_DrawAALineTex
nodrawculledbackedge201AA:	

		bl		drawexternedge201AA

		bf		curfanFLAG,cont012AA
		bt		startnewstripFLAG,drawlastedge201AA
cont210AA:
		bl		load0AA
		bf		cr0gt,start201iffanAA
		bt		aanodrawFLAG,start201ifclipAA

		bl		drawexternedge210AA

		bf		curfanFLAG,cont102AA
		bt		startnewstripFLAG,drawlastedge210AA
		b		cont201AA

/*
 * aa line drawing routine
 */
drawedge01AA:
		lbzx		itemp0,edgeidx0,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset0
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv0
		mr		puv1AA,puv1
		b		drawedgeAA

drawedge02AA:
		lbzx		itemp0,edgeidx0,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset0
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv0
		mr		puv1AA,puv2
		b		drawedgeAA

drawedge10AA:
		lbzx		itemp0,edgeidx1,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset1
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv1
		mr		puv1AA,puv0
		b		drawedgeAA

drawedge12AA:
		lbzx		itemp0,edgeidx1,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset1
		mr		pdraw1AA,vtxoffset2
		mr		puv0AA,puv1
		mr		puv1AA,puv2
		b		drawedgeAA

drawedge20AA:
		lbzx		itemp0,edgeidx2,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset2
		mr		pdraw1AA,vtxoffset0
		mr		puv0AA,puv2
		mr		puv1AA,puv0
		b		drawedgeAA

drawedge21AA:
		lbzx		itemp0,edgeidx2,paaedgebuf
		cmpi		0,itemp0,1
		bnelr
		mr		pdraw0AA,vtxoffset2
		mr		pdraw1AA,vtxoffset1
		mr		puv0AA,puv2
		mr		puv1AA,puv1
		b		drawedgeAA

drawedgeAA:
		mflr		itemp0
		stw		itemp0,CloseData.drawlrsave1_off(pclosedata)
		bl		M_DrawAALineTex
		lwz		itemp0,CloseData.drawlrsave1_off(pclosedata)
		mtlr		itemp0
		blr

drawexternedge012AA:
		bt		prevfanFLAG,drawexternedge012fanAA
		bt		prevclockwiseFLAG,drawexternedge012stripcwAA
		bt		startnewstripFLAG,drawedge20AA
		bt		aaalterprimFLAG,drawedge12AA	/* strip, ccw, alterprim     */
		b		drawedge20AA			/* strip, ccw, alterprim not */
drawexternedge012stripcwAA:
		bt		startnewstripFLAG,drawedge02AA
		bt		aaalterprimFLAG,drawedge21AA	/* strip,  cw, alterprim     */
		b		drawedge02AA			/* strip,  cw, alterprim not */
drawexternedge012fanAA:
		bt		prevclockwiseFLAG,drawexternedge012fancwAA
		bt		startnewstripFLAG,drawedge12AA
		bt		aaalterprimFLAG,drawedge20AA	/*   fan, ccw, alterprim     */
		b		drawedge12AA			/*   fan, ccw, alterprim not */
drawexternedge012fancwAA:
		bt		startnewstripFLAG,drawedge21AA
		bt		aaalterprimFLAG,drawedge02AA	/*   fan,  cw, alterprim     */
		b		drawedge21AA			/*   fan,  cw, alterprim not */

drawexternedge021AA:
		bt		prevfanFLAG,drawexternedge021fanAA
		bt		prevclockwiseFLAG,drawexternedge021stripcwAA
		bt		startnewstripFLAG,drawedge01AA
		bt		aaalterprimFLAG,drawedge12AA	/* strip, ccw, alterprim     */
		b		drawedge01AA			/* strip, ccw, alterprim not */
drawexternedge021stripcwAA:
		bt		startnewstripFLAG,drawedge10AA
		bt		aaalterprimFLAG,drawedge21AA	/* strip,  cw, alterprim     */
		b		drawedge10AA			/* strip,  cw, alterprim not */
drawexternedge021fanAA:
		bt		prevclockwiseFLAG,drawexternedge021fancwAA
		bt		startnewstripFLAG,drawedge12AA
		bt		aaalterprimFLAG,drawedge01AA	/*   fan, ccw, alterprim     */
		b		drawedge12AA			/*   fan, ccw, alterprim not */
drawexternedge021fancwAA:
		bt		startnewstripFLAG,drawedge21AA
		bt		aaalterprimFLAG,drawedge10AA	/*   fan,  cw, alterprim     */
		b		drawedge21AA			/*   fan,  cw, alterprim not */

drawexternedge102AA:
		bt		prevfanFLAG,drawexternedge102fanAA
		bt		prevclockwiseFLAG,drawexternedge102stripcwAA
		bt		startnewstripFLAG,drawedge12AA
		bt		aaalterprimFLAG,drawedge20AA	/* strip, ccw, alterprim     */
		b		drawedge12AA			/* strip, ccw, alterprim not */
drawexternedge102stripcwAA:
		bt		startnewstripFLAG,drawedge21AA
		bt		aaalterprimFLAG,drawedge02AA	/* strip,  cw, alterprim     */
		b		drawedge21AA			/* strip,  cw, alterprim not */
drawexternedge102fanAA:
		bt		prevclockwiseFLAG,drawexternedge102fancwAA
		bt		startnewstripFLAG,drawedge20AA
		bt		aaalterprimFLAG,drawedge12AA	/*   fan, ccw, alterprim     */
		b		drawedge20AA			/*   fan, ccw, alterprim not */
drawexternedge102fancwAA:
		bt		startnewstripFLAG,drawedge02AA
		bt		aaalterprimFLAG,drawedge21AA	/*   fan,  cw, alterprim     */
		b		drawedge02AA			/*   fan,  cw, alterprim not */

drawexternedge120AA:
		bt		prevfanFLAG,drawexternedge120fanAA
		bt		prevclockwiseFLAG,drawexternedge120stripcwAA
		bt		startnewstripFLAG,drawedge01AA
		bt		aaalterprimFLAG,drawedge20AA	/* strip, ccw, alterprim     */
		b		drawedge01AA			/* strip, ccw, alterprim not */
drawexternedge120stripcwAA:
		bt		startnewstripFLAG,drawedge10AA
		bt		aaalterprimFLAG,drawedge02AA	/* strip,  cw, alterprim     */
		b		drawedge10AA			/* strip,  cw, alterprim not */
drawexternedge120fanAA:
		bt		prevclockwiseFLAG,drawexternedge120fancwAA
		bt		startnewstripFLAG,drawedge20AA
		bt		aaalterprimFLAG,drawedge01AA	/*   fan, ccw, alterprim     */
		b		drawedge20AA			/*   fan, ccw, alterprim not */
drawexternedge120fancwAA:
		bt		startnewstripFLAG,drawedge02AA
		bt		aaalterprimFLAG,drawedge10AA	/*   fan,  cw, alterprim     */
		b		drawedge02AA			/*   fan,  cw, alterprim not */

drawexternedge201AA:
		bt		prevfanFLAG,drawexternedge201fanAA
		bt		prevclockwiseFLAG,drawexternedge201stripcwAA
		bt		startnewstripFLAG,drawedge12AA
		bt		aaalterprimFLAG,drawedge01AA	/* strip, ccw, alterprim     */
		b		drawedge12AA			/* strip, ccw, alterprim not */
drawexternedge201stripcwAA:
		bt		startnewstripFLAG,drawedge21AA
		bt		aaalterprimFLAG,drawedge10AA	/* strip,  cw, alterprim     */
		b		drawedge21AA			/* strip,  cw, alterprim not */
drawexternedge201fanAA:
		bt		prevclockwiseFLAG,drawexternedge201fancwAA
		bt		startnewstripFLAG,drawedge01AA
		bt		aaalterprimFLAG,drawedge12AA	/*   fan, ccw, alterprim     */
		b		drawedge01AA			/*   fan, ccw, alterprim not */
drawexternedge201fancwAA:
		bt		startnewstripFLAG,drawedge10AA
		bt		aaalterprimFLAG,drawedge21AA	/*   fan,  cw, alterprim     */
		b		drawedge10AA			/*   fan,  cw, alterprim not */

drawexternedge210AA:
		bt		prevfanFLAG,drawexternedge210fanAA
		bt		prevclockwiseFLAG,drawexternedge210stripcwAA
		bt		startnewstripFLAG,drawedge20AA
		bt		aaalterprimFLAG,drawedge01AA	/* strip, ccw, alterprim     */
		b		drawedge20AA			/* strip, ccw, alterprim not */
drawexternedge210stripcwAA:
		bt		startnewstripFLAG,drawedge02AA
		bt		aaalterprimFLAG,drawedge10AA	/* strip,  cw, alterprim     */
		b		drawedge02AA			/* strip,  cw, alterprim not */
drawexternedge210fanAA:
		bt		prevclockwiseFLAG,drawexternedge210fancwAA
		bt		startnewstripFLAG,drawedge01AA
		bt		aaalterprimFLAG,drawedge20AA	/*   fan, ccw, alterprim     */
		b		drawedge01AA			/*   fan, ccw, alterprim not */
drawexternedge210fancwAA:
		bt		startnewstripFLAG,drawedge10AA
		bt		aaalterprimFLAG,drawedge02AA	/*   fan,  cw, alterprim     */
		b		drawedge10AA			/*   fan,  cw, alterprim not */

drawlastedge012AA:
		bt		prevfanFLAG,drawlastedge012fanAA
		bt		prevclockwiseFLAG,drawlastedge012stripcwAA
drawlastedge012stripccwAA:
		bt		startnewstripFLAG,drawlastedge012stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge012stripccwapAA
drawlastedge012stripccwnewAA:
		bl		drawedge12AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge012stripccwapAA:
		bl		drawedge20AA	/* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge012stripcwAA:
		bt		startnewstripFLAG,drawlastedge012stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge012stripcwapAA
drawlastedge012stripcwnewAA:
		bl		drawedge21AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge012stripcwapAA:
		bl		drawedge02AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge012fanAA:
		bt		prevclockwiseFLAG,drawlastedge012fancwAA
drawlastedge012fanccwAA:
		bt		startnewstripFLAG,drawlastedge012fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge012fanccwapAA
drawlastedge012fanccwnewAA:
		bl		drawedge20AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge012fanccwapAA:
		bl		drawedge12AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge012fancwAA:
		bt		startnewstripFLAG,drawlastedge012fancwnewAA
		bt		aaalterprimFLAG,drawlastedge012fancwapAA
drawlastedge012fancwnewAA:
		bl		drawedge02AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge012fancwapAA:
		bl		drawedge21AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

drawlastedge021AA:
		bt		prevfanFLAG,drawlastedge021fanAA
		bt		prevclockwiseFLAG,drawlastedge021stripcwAA
drawlastedge021stripccwAA:
		bt		startnewstripFLAG,drawlastedge021stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge021stripccwapAA
drawlastedge021stripccwnewAA:
		bl		drawedge12AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge021stripccwapAA:
		bl		drawedge01AA	/* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge021stripcwAA:
		bt		startnewstripFLAG,drawlastedge021stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge021stripcwapAA
drawlastedge021stripcwnewAA:
		bl		drawedge21AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge021stripcwapAA:
		bl		drawedge10AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge021fanAA:
		bt		prevclockwiseFLAG,drawlastedge021fancwAA
drawlastedge021fanccwAA:
		bt		startnewstripFLAG,drawlastedge021fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge021fanccwapAA
drawlastedge021fanccwnewAA:
		bl		drawedge01AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge021fanccwapAA:
		bl		drawedge12AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge021fancwAA:
		bt		startnewstripFLAG,drawlastedge021fancwnewAA
		bt		aaalterprimFLAG,drawlastedge021fancwapAA
drawlastedge021fancwnewAA:
		bl		drawedge10AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge021fancwapAA:
		bl		drawedge21AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

drawlastedge102AA:
		bt		prevfanFLAG,drawlastedge102fanAA
		bt		prevclockwiseFLAG,drawlastedge102stripcwAA
drawlastedge102stripccwAA:
		bt		startnewstripFLAG,drawlastedge102stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge102stripccwapAA
drawlastedge102stripccwnewAA:
		bl		drawedge20AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge102stripccwapAA:
		bl		drawedge12AA	/* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge102stripcwAA:
		bt		startnewstripFLAG,drawlastedge102stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge102stripcwapAA
drawlastedge102stripcwnewAA:
		bl		drawedge02AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge102stripcwapAA:
		bl		drawedge21AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge102fanAA:
		bt		prevclockwiseFLAG,drawlastedge102fancwAA
drawlastedge102fanccwAA:
		bt		startnewstripFLAG,drawlastedge102fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge102fanccwapAA
drawlastedge102fanccwnewAA:
		bl		drawedge12AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge102fanccwapAA:
		bl		drawedge20AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge102fancwAA:
		bt		startnewstripFLAG,drawlastedge102fancwnewAA
		bt		aaalterprimFLAG,drawlastedge102fancwapAA
drawlastedge102fancwnewAA:
		bl		drawedge21AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge102fancwapAA:
		bl		drawedge02AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

drawlastedge120AA:
		bt		prevfanFLAG,drawlastedge120fanAA
		bt		prevclockwiseFLAG,drawlastedge120stripcwAA
drawlastedge120stripccwAA:
		bt		startnewstripFLAG,drawlastedge120stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge120stripccwapAA
drawlastedge120stripccwnewAA:
		bl		drawedge20AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge120stripccwapAA:
		bl		drawedge01AA    /* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge120stripcwAA:
		bt		startnewstripFLAG,drawlastedge120stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge120stripcwapAA
drawlastedge120stripcwnewAA:
		bl		drawedge02AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge120stripcwapAA:
		bl		drawedge10AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge120fanAA:
		bt		prevclockwiseFLAG,drawlastedge120fancwAA
drawlastedge120fanccwAA:
		bt		startnewstripFLAG,drawlastedge120fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge120fanccwapAA
drawlastedge120fanccwnewAA:
		bl		drawedge01AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge120fanccwapAA:
		bl		drawedge20AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge120fancwAA:
		bt		startnewstripFLAG,drawlastedge120fancwnewAA
		bt		aaalterprimFLAG,drawlastedge120fancwapAA
drawlastedge120fancwnewAA:
		bl		drawedge10AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge120fancwapAA:
		bl		drawedge02AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

drawlastedge201AA:
		bt		prevfanFLAG,drawlastedge201fanAA
		bt		prevclockwiseFLAG,drawlastedge201stripcwAA
drawlastedge201stripccwAA:
		bt		startnewstripFLAG,drawlastedge201stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge201stripccwapAA
drawlastedge201stripccwnewAA:
		bl		drawedge01AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge201stripccwapAA:
		bl		drawedge12AA	/* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge201stripcwAA:
		bt		startnewstripFLAG,drawlastedge201stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge201stripcwapAA
drawlastedge201stripcwnewAA:
		bl		drawedge10AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge201stripcwapAA:
		bl		drawedge21AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge201fanAA:
		bt		prevclockwiseFLAG,drawlastedge201fancwAA
drawlastedge201fanccwAA:
		bt		startnewstripFLAG,drawlastedge201fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge201fanccwapAA
drawlastedge201fanccwnewAA:
		bl		drawedge12AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge201fanccwapAA:
		bl		drawedge01AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge201fancwAA:
		bt		startnewstripFLAG,drawlastedge201fancwnewAA
		bt		aaalterprimFLAG,drawlastedge201fancwapAA
drawlastedge201fancwnewAA:
		bl		drawedge21AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge201fancwapAA:
		bl		drawedge10AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

drawlastedge210AA:
		bt		prevfanFLAG,drawlastedge210fanAA
		bt		prevclockwiseFLAG,drawlastedge210stripcwAA
drawlastedge210stripccwAA:
		bt		startnewstripFLAG,drawlastedge210stripccwnewAA
		bt		aaalterprimFLAG,drawlastedge210stripccwapAA
drawlastedge210stripccwnewAA:
		bl		drawedge01AA	/* strip, ccw, alterprim not */
		b		startstripfanAA
drawlastedge210stripccwapAA:
		bl		drawedge20AA	/* strip, ccw, alterprim     */
		b		startstripfanAA
drawlastedge210stripcwAA:
		bt		startnewstripFLAG,drawlastedge210stripcwnewAA
		bt		aaalterprimFLAG,drawlastedge210stripcwapAA
drawlastedge210stripcwnewAA:
		bl		drawedge10AA	/* strip,  cw, alterprim not */
		b		startstripfanAA
drawlastedge210stripcwapAA:
		bl		drawedge02AA	/* strip,  cw, alterprim     */
		b		startstripfanAA
drawlastedge210fanAA:
		bt		prevclockwiseFLAG,drawlastedge210fancwAA
drawlastedge210fanccwAA:
		bt		startnewstripFLAG,drawlastedge210fanccwnewAA
		bt		aaalterprimFLAG,drawlastedge210fanccwapAA
drawlastedge210fanccwnewAA:
		bl		drawedge20AA	/*   fan, ccw, alterprim not */
		b		startstripfanAA
drawlastedge210fanccwapAA:
		bl		drawedge01AA	/*   fan, ccw, alterprim     */
		b		startstripfanAA
drawlastedge210fancwAA:
		bt		startnewstripFLAG,drawlastedge210fancwnewAA
		bt		aaalterprimFLAG,drawlastedge210fancwapAA
drawlastedge210fancwnewAA:
		bl		drawedge02AA	/*   fan,  cw, alterprim not */
		b		startstripfanAA
drawlastedge210fancwapAA:
		bl		drawedge10AA	/*   fan,  cw, alterprim     */
		b		startstripfanAA

/***************************************************
 *
 *	load code
 *
 ***************************************************/

load0AA:
		add		vtxoffset0,nextvtxoffset,pxformbase

		lfs		fdxp0,xform.xpp(vtxoffset0)
		lfs		fdyp0,xform.ypp(vtxoffset0)

		mr		puv0,puv
		mr		edgeidx0,edgeidx
		b		loadcontAA
load1AA:
		add		vtxoffset1,nextvtxoffset,pxformbase

		lfs		fdxp1,xform.xpp(vtxoffset1)
		lfs		fdyp1,xform.ypp(vtxoffset1)

		mr		puv1,puv
		mr		edgeidx1,edgeidx
		b		loadcontAA
load2AA:
		add		vtxoffset2,nextvtxoffset,pxformbase

		lfs		fdxp2,xform.xpp(vtxoffset2)
		lfs		fdyp2,xform.ypp(vtxoffset2)

		mr		puv2,puv
		mr		edgeidx2,edgeidx
loadcontAA:
		addi		puv,puv,8

		lhau		itemp0,2(pindex)
		andi.		nextvtxoffset,itemp0,0x7ff
		rlwimi		iconstVI1,itemp0,(17-11),11,11
		mulli		nextvtxoffset,nextvtxoffset,xform

		lhzu		edgeidx,2(paaedge)

/*	set flags CR4 and CR5 from itemp0 */

		mtcrf		0x0c,itemp0

		crxor		aaalterprimFLAG,curfanFLAG,prevfanFLAG

		crclr		aanodrawFLAG
		bf		clipFLAG,.noclip2AA

		lwz		iclipflags0,xform.flags(vtxoffset0)
		lwz		iclipflags1,xform.flags(vtxoffset1)
		or		someout,iclipflags0,iclipflags1
		and		itemp0,iclipflags0,iclipflags1
		lwz		iclipflags1,xform.flags(vtxoffset2)
		or.		someout,someout,iclipflags1
		and		itemp0,itemp0,iclipflags1
		bne		0,.clipAA
		crclr		aanodrawFLAG
.noclip2AA:
		bt		nocullFLAG,.nocullAA
		fsubs		fdtemp0,fdxp1,fdxp0
		fsubs		fdtemp1,fdyp2,fdyp0
		fsubs		fdtemp2,fdxp2,fdxp0
		fsubs		fdtemp3,fdyp1,fdyp0

		fmuls		fdtemp0,fdtemp0,fdtemp1
		fmuls		fdtemp2,fdtemp2,fdtemp3

		fcmpu		0,fdtemp2,fdtemp0			/* chs */
		beq		.nocullAA			/* zero tri */
		crxor		cr0gt,cr0gt,prevclockwiseFLAG
		crxor		cr0gt,cr0gt,frontcullFLAG
		blr
.nocullAA:
		crset		cr0gt
		blr
.clipAA:
		crset		aanodrawFLAG
		b		.noclip2AA

/*	anti_alias pass 2 done */
alldoneAA:
		li		3,0
		lwz		itemp1,CloseData.ppodsave_off(pclosedata)
		stw		3,CloseData.aa_off(pclosedata)
		lwz		3,Pod.pcase_off(itemp1)
		bl		M_SaveAndCallC
		lwz		3,CloseData.drawlrsave_off(pclosedata)
		mtlr		3
		blr
M_ClistManager:
		lea		3,M_ClistManagerC
		b		M_SaveAndCallC
endcode:
		.space		8192-(endcode-begincode)

/*	END  */ 

