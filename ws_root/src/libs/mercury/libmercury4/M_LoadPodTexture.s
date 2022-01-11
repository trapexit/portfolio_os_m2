		.include	"structmacros.i"
		.include	"PPCMacroequ.i"
		.include	"conditionmacros.i"
		.include	"mercury.i"

		.include	"M_Draw.i"

		define		ptpage		,	22
		define		temp		,	22
		define		ptex		,	23
		define		texbytes	,	24
		define		ppip		,	25
		define		pipbytes	,	26
		define		ploadsnips	,	27
		define		pselsnips	,	28
		define		ptexloadVIs	,	29
		define		ppiploadVIs	,	30
		define		linkregsave	,	31
/*
 *	LoadPodTexture
 *	Insert own pointers and sizes into the podtexture template.
 *  Copy the "load texture" VIs to pVI.
 *
 *	INPUT:	
 *		GPR3:pointer the podtexture structure
 *		pVI
 *		pVIwritemax
 *		pclosedata
 *
 *	OUTPUT:	none
 *
 *	Registers 0,3, and 22 to 31 may be used freely, as may others.
 */			
	DECFN	M_LoadPodTexture
		mflr		linkregsave
		lwz		ptpage,PodTexture.ptpagesnippets_off(3)
		lwz		ptex,PodTexture.ptexture_off(3)
		lwz		texbytes,PodTexture.texturebytes_off(3)
		lwz		ppip,PodTexture.ppip_off(3)
		lwz		pipbytes,PodTexture.pipbytes_off(3)

		lwz		ploadsnips,TpageSnippets.ploadsnippets_off(ptpage)
		lwz		pselsnips,TpageSnippets.pselectsnippets_off(ptpage)

		lwz		ptexloadVIs,CltSnippet.data_off(ploadsnips)
		lwz		ppiploadVIs,CltSnippet+CltSnippet.data_off(ploadsnips)

/*	update ptexselsnippets for the texture select in the case code */
		stw		pselsnips,CloseData.ptexselsnippets_off(pclosedata)

/*	modify the load so that this texture is loaded */
		stw		ptex,12(ptexloadVIs)
		stw		texbytes,20(ptexloadVIs)
		stw		ppip,4(ppiploadVIs)
		stw		pipbytes,12(ppiploadVIs)

/*	copy the texture load commands */
		lhz		temp,CltSnippet.size_off(ploadsnips)
		mtctr		temp
		addi		ptexloadVIs,ptexloadVIs,-4
		lwzu		temp,4(ptexloadVIs)
.Ltexloop:			
		stwu		temp,4(pVI)
		lwzu		temp,4(ptexloadVIs)
		bdnz		.Ltexloop			

/*	copy the PIP load commands */
		lhz		temp,CltSnippet+CltSnippet.size_off(ploadsnips)
		mtctr		temp
		addi		ppiploadVIs,ppiploadVIs,-4
		lwzu		temp,4(ppiploadVIs)
.Lpiploop:			
		stwu		temp,4(pVI)
		lwzu		temp,4(ppiploadVIs)		
		bdnz		.Lpiploop			

		cmpl		0,pVI,pVIwritemax		/* chs */
		bgel-		M_ClistManager

		mtlr		linkregsave
		blr
M_ClistManager:
		lea	3,M_ClistManagerC
		b	M_SaveAndCallC

/*	END */
