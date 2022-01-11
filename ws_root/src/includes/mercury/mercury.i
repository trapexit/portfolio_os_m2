
	struct	CloseData
/*	pure constants */
	stbyte	CloseData.tltable_off,			32		/* LINE ALIGNED table */
	stlong	CloseData.fconstTINY_off,		1
	stlong	CloseData.fconst0pt0_off,		1
	stlong	CloseData.fconst0pt25_off,		1
	stlong	CloseData.fconst0pt5_off,		1
	stlong	CloseData.fconst0pt75_off,		1
	stlong	CloseData.fconst1pt0_off,		1
	stlong	CloseData.fconst2pt0_off,		1
	stlong	CloseData.fconst3pt0_off,		1
	stlong	CloseData.fconst8pt0_off,		1
	stlong	CloseData.fconst16pt0_off,		1
	stlong	CloseData.fconst255pt0_off,		1
	stlong	CloseData.fconst12million_off,		1
	stlong	CloseData.fconstasin0_off,		1
	stlong	CloseData.fconstasin1_off,		1
	stlong	CloseData.fconstasin2_off,		1
	stlong	CloseData.fconstasin3_off,		1

/*	ALMOST constants */
	stlong	CloseData.fwclose_off,			1
	stlong	CloseData.fwfar_off,			1
	stlong	CloseData.fscreenwidth_off,		1
	stlong	CloseData.fscreenheight_off,		1

/* filled in in advance */
	stlong	CloseData.fcamskewmatrix_off,		12		/* covers 2 lines, not 3 */
	stlong	CloseData.pxformbuffer_off,		1
	stlong	CloseData.ptexselsnippets_off,		1
	stlong	CloseData.pVIwrite_off,			1
	stlong	CloseData.pVIwritemax_off,		1	
	stlong	CloseData.gstate_off,			1	
	stlong	CloseData.watermark_off,		1	
	stlong	CloseData.frbase_off,			1
	stlong	CloseData.fgbase_off,			1
	stlong	CloseData.fbbase_off,			1
	stlong	CloseData.fabase_off,			1
	stlong	CloseData.frdiffuse_off,		1
	stlong	CloseData.fgdiffuse_off,		1
	stlong	CloseData.fbdiffuse_off,		1
	stlong	CloseData.fshine_off,			1
	stlong	CloseData.frspecular_off,		1
	stlong	CloseData.fgspecular_off,		1
	stlong	CloseData.fbspecular_off,		1
	stlong	CloseData.matflags_off,			1
	stlong	CloseData.fcamx_off,			1
	stlong	CloseData.fcamy_off,			1
	stlong	CloseData.fcamz_off,			1
	stlong	CloseData.flocalcamx_off,		1
	stlong	CloseData.flocalcamy_off,		1
	stlong	CloseData.flocalcamz_off,		1
	stlong	CloseData.flight0_off,			1
	stlong	CloseData.flight1_off,			1
	stlong	CloseData.flight2_off,			1
	stlong	CloseData.flight3_off,			1
	stlong	CloseData.flight4_off,			1
	stlong	CloseData.flight5_off,			1
	stlong	CloseData.flight6_off,			1
	stlong	CloseData.flight7_off,			1
	stlong	CloseData.flight8_off,			1
	stlong	CloseData.flight9_off,			1
	stlong	CloseData.flight10_off,			1
	stlong	CloseData.flight11_off,			1
	stlong	CloseData.flight12_off,			1
	stlong	CloseData.flight13_off,			1
	stlong	CloseData.plightreturn_off,		1
	stlong	CloseData.pdrawroutine_off,		1

/* filled in inside routine */
	stlong	CloseData.drawlrsave_off,		1
	stlong	CloseData.drawlrsave1_off,		1
	stlong	CloseData.drawlrsave2_off,		1
	stlong	CloseData.ppodsave_off,			1
	stlong	CloseData.fxmat00_off,			1
	stlong	CloseData.fxmat01_off,			1
	stlong	CloseData.fxmat02_off,			1
	stlong	CloseData.fxmat10_off,			1
	stlong	CloseData.fxmat11_off,			1
	stlong	CloseData.fxmat12_off,			1
	stlong	CloseData.fxmat20_off,			1
	stlong	CloseData.fxmat21_off,			1
	stlong	CloseData.fxmat22_off,			1
	stlong	CloseData.fxmat30_off,			1
	stlong	CloseData.fxmat31_off,			1
	stlong	CloseData.fxmat32_off,			1
/*	clip routine	*/
	stlong	CloseData.float0,			1
	stlong	CloseData.float1,			1
	stlong	CloseData.float2,			1
	stlong	CloseData.float3,			1
	stlong	CloseData.float4,			1
	stlong	CloseData.float5,			1
	stlong	CloseData.float6,			1
	stlong	CloseData.float7,			1
	stlong	CloseData.float8,			1
	stlong	CloseData.float9,			1
	stlong	CloseData.float10,			1
	stlong	CloseData.float11,			1
	stlong	CloseData.float12,			1
	stlong	CloseData.float13,			1
	stlong	CloseData.float14,			1
	stlong	CloseData.float15,			1
	stlong	CloseData.float16,			1
	stlong	CloseData.float17,			1
	stlong	CloseData.cr,				1
	stlong	CloseData.int17_31,			15

/* converted light data! the more lights, the longer -- this
 * should support 8 lights for sure.
 */
	stlong	CloseData.convlightdata_off,		100

/*
 * clipping buffers
 * 2 swapping buffers of 8 pointers each
 * and space for up to 13 vertices at 10 words each
 */
	stlong	CloseData.clipbuf1_off,			8
	stlong	CloseData.clipbuf2_off,			8
	stlong	CloseData.clipdata_off,			130
	stlong	CloseData.fogcolor_off,			1
	stlong	CloseData.srcaddr_off,			1
	stlong	CloseData.depth_off,			1
	stlong	CloseData.aa_off,			1

	.ifdef STATISTICS
	stlong	CloseData.numpods_fast,			1
	stlong	CloseData.numpods_slow,			1
	stlong	CloseData.numtris_fast,			1
	stlong	CloseData.numtris_slow,			1
	stlong	CloseData.numtexloads,			1
	stlong	CloseData.numtexbytes,			1
	.endif

    ends	CloseData

	struct AAData
	stword	AAData.edgecount_off,			1
	stword	AAData.flags_off,			1
	stlong	AAData.color_off,			3
	stlong	AAData.paaedgebuffer_off,		1
	ends AAData

	struct	Pod
	stlong	Pod.flags_off,				1
	stlong	Pod.pnext_off,				1
	stlong	Pod.pcase_off,				1
	stlong	Pod.ptexture_off,			1
	stlong	Pod.pgeometry_off,			1
	stlong	Pod.pmatrix_off,			1
	stlong	Pod.plights_off,			1
	stlong	Pod.puserdata_off,			1		/* first data is subroutine */
	stlong	Pod.pmaterial_off,			1
	stlong	Pod.paadata_off,			1
	ends Pod 

/* flags word in Pod */
	define	samecaseFLAG		,		8
	define	sametextureFLAG		,		9

/* used in pploop, can be thrown away */
	define	callatstartFLAG		,		16
	define	casecodeisasmFLAG	,		17
	define	usercheckedclipFLAG	,		18
	define	preconcatFLAG		,		19

/*	podgeometry index bits */
	define	prevclockwiseFLAG 	,		16
	define	prevfanFLAG		,		17
	define	curfanFLAG		,		18
	define	startnewstripFLAG	,		19
	define	selecttextureFLAG	,		20
/* flags in clipping overwrite specular and hithernocull flags */
	define	croldbit		,		21
	define	crnewbit		,		22
	define	crlastbit		,		23

/* okay to overwrite these in per vertex Draw routines */
	define	hithernocullFLAG	,		23

/* flags for anti_aliasing */
	define	aapassbbtestFLAG	,		25
	define	aasavenocullFLAG	,		26
	define	aasaveclipFLAG		,		27
	define	aanodrawFLAG		,		24
	define	aafirsttriFLAG		,		25
	define	aatriculledFLAG		,		26
	define	aaalterprimFLAG		,		27

/* need to preserve these flags in per vertex Draw routines */

	define	nocullFLAG		,		28
	define	frontcullFLAG		,		29
	define	clipFLAG		,		30
	define	callatendFLAG		,		31

/* anti_alias flags */

	define	x1gtx0FLAG		,		24
	define	x1eqx0FLAG		,		25
	define	x1ltx0FLAG		,		26
	define	y1gty0FLAG		,		27
	define	y1eqy0FLAG		,		28
	define	y1lty0FLAG		,		29
	define	dygtdxFLAG		,		30
	define	dyledxFLAG		,		31


	define	AA2NDPASS,	1
	define	AALINEDRAW,	2

	struct	PodGeometry	
	stlong	PodGeometry.xmin_off,			1 
	stlong	PodGeometry.ymin_off,			1 
	stlong	PodGeometry.zmin_off,			1 
	stlong	PodGeometry.xextent_off,		1 
	stlong	PodGeometry.yextent_off,		1 
	stlong	PodGeometry.zextent_off,		1
	stlong	PodGeometry.pvertex_off,		1 
	stlong	PodGeometry.pshared_off,		1 
	stword	PodGeometry.vertexcount_off,		1 
	stword	PodGeometry.sharedcount_off,		1 
	stlong	PodGeometry.pindex_off,			1 
	stlong	PodGeometry.puv_off,			1 
	stlong	PodGeometry.paaedge_off,		1
	ends	PodGeometry 

	struct  Material
	stlong	Material.base_off,			4
	stlong	Material.diffuse_off,			3
	stlong	Material.shine_off,			1
	stlong	Material.specular_off,			3
	stlong	Material.flags_off,			1
	stlong	Material.specdata_off,			10
	ends	Material

	struct	PodTexture
	stlong	PodTexture.proutine_off,		1
	stlong	PodTexture.ptpagesnippets_off, 		1
	stlong	PodTexture.ptexture_off, 		1
	stlong	PodTexture.texturecount_off, 		1
	stlong	PodTexture.texturebytes_off, 		1
	stlong	PodTexture.ppip_off, 			1
	stlong	PodTexture.pipbytes_off, 		1
	ends	PodTexture

	struct TpageSnippets
	stlong	TpageSnippets.loadcount_off, 		1
	stlong	TpageSnippets.ploadsnippets_off, 	1
	stlong	TpageSnippets.pselectsnippets_off, 	1
	ends	TpageSnippets

/* Corresponds to paolo's snippet */
/* Size of 8 bytes is hard-wired into the code, which should be OK
 * as a snippet really is just an address plus a count
 */
	struct	CltSnippet
	stlong	CltSnippet.data_off,			1
	stbyte	CltSnippet.size_off,			2
	stbyte	CltSnippet.allocated_off,		2
	ends	CltSnippet 

	struct	MSnippet				
	stbyte	MSnippet.snippet		CltSnippet
	stlong	MSnippet.uscale,			1
	stlong	MSnippet.vscale,			1
	ends	MSnippet 

/* 	Standard Stack Frame (from Greg Omi) */
	struct	StackFrame
	stlong	StackFrame.BackChain,			4
	stlong	StackFrame.SaveCtr,			1
	stlong	StackFrame.SaveLr,			1
	stlong	StackFrame.Float00,			1
	stlong	StackFrame.Float01,			1
	stlong	StackFrame.Float02,			1
	stlong	StackFrame.Float03,			1
	stlong	StackFrame.Float04,			1
	stlong	StackFrame.Float05,			1
	stlong	StackFrame.Float06,			1
	stlong	StackFrame.Float07,			1
	stlong	StackFrame.Float08,			1
	stlong	StackFrame.Float09,			1
	stlong	StackFrame.Float10,			1
	stlong	StackFrame.Float11,			1
	stlong	StackFrame.Float12,			1
	stlong	StackFrame.Float13,			1
	stlong	StackFrame.Float14,			1
	stlong	StackFrame.Float15,			1
	stlong	StackFrame.Float16,			1
	stlong	StackFrame.Float17,			1
	stlong	StackFrame.Float18,			1
	stlong	StackFrame.Float19,			1
	stlong	StackFrame.Float20,			1
	stlong	StackFrame.Float21,			1
	stlong	StackFrame.Float22,			1
	stlong	StackFrame.Float23,			1
	stlong	StackFrame.Float24,			1
	stlong	StackFrame.Float25,			1
	stlong	StackFrame.Float26,			1
	stlong	StackFrame.Float27,			1
	stlong	StackFrame.Float28,			1
	stlong	StackFrame.Float29,			1
	stlong	StackFrame.Float30,			1
	stlong	StackFrame.Float31,			1
	stlong	StackFrame.Int00,			1
	stlong	StackFrame.Int03,			1
	stlong	StackFrame.Int04,			1
	stlong	StackFrame.Int05,			1
	stlong	StackFrame.Int06,			1
	stlong	StackFrame.Int07,			1
	stlong	StackFrame.Int08,			1
	stlong	StackFrame.Int09,			1
	stlong	StackFrame.Int10,			1
	stlong	StackFrame.Int11,			1
	stlong	StackFrame.Int12,			1
	stlong	StackFrame.Int13,			1
	stlong	StackFrame.Int14,			1
	stlong	StackFrame.Int15,			1
	stlong	StackFrame.Int16,			1
	stlong	StackFrame.Int17,			1
	stlong	StackFrame.Int18,			1
	stlong	StackFrame.Int19,			1
	stlong	StackFrame.Int20,			1
	stlong	StackFrame.Int21,			1
	stlong	StackFrame.Int22,			1
	stlong	StackFrame.Int23,			1
	stlong	StackFrame.Int24,			1
	stlong	StackFrame.Int25,			1
	stlong	StackFrame.Int26,			1
	stlong	StackFrame.Int27,			1
	stlong	StackFrame.Int28,			1
	stlong	StackFrame.Int29,			1
	stlong	StackFrame.Int30,			1
	stlong	StackFrame.Int31,			1
	ends 	StackFrame

	define	PCLK,	0x8000
	define	PFAN,	0x4000
	define	CFAN,	0x2000
	define	NEWS,	0x1000
	define	STXT,	0x0800

	struct	BBox
	stlong	BBox.xmin_off,				1 
	stlong	BBox.ymin_off,				1 
	stlong	BBox.zmin_off,				1 
	stlong	BBox.xextent_off,			1 
	stlong	BBox.yextent_off,			1 
	stlong	BBox.zextent_off,			1
	ends	BBox 

	struct	BBoxList
	stlong	BBoxList.flags_off,			1
	stlong	BBoxList.pnext_off,			1
	stlong	BBoxList.pbbox_off,			1
	stlong	BBoxList.pmatrix_off,			1
	ends 	BBoxList 

/*	BBox flags		*/

	define	xminFlag			,	1
	define	xmaxFlag			,	5
	define	yminFlag			,	9
	define	ymaxFlag			,	13
	define	hitherFlag			,	17
	define	yonFlag				,	21

	define	 specdatadefinedFLAG		,	1
