#ifndef __CLTMACROMACROS_H
#define __CLTMACROMACROS_H

extern uint32 		M_IntensityTexture[];

#define M_Reserve(size) \
	if((pc->pVIwrite + size + 1) > pc->pVIwritemax) \
		M_ClistManagerC(pc);
	
#define M_TBNoTex_Size          6
#define M_TBNoTex(pp) \
	*((*pp)++) = CLT_ClearRegistersHeader(TXTADDRCNTL, 1); \
	*((*pp)++) = CLT_Bits(TXTADDRCNTL, TEXTUREENABLE, 1); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTTABCNTL, 1); \
	*((*pp)++) = CLT_SetConst(TXTTABCNTL, COLOROUT, PRIMCOLOR) | CLT_SetConst(TXTTABCNTL, ALPHAOUT, PRIMALPHA); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBDISCARDCONTROL, 1); \
	*((*pp)++) = CLT_Bits(DBDISCARDCONTROL, SSB0, 1)

#define M_TBLitTex_Size	        6
#define M_TBLitTex(pp) \
	CLT_TXTTABCNTL(pp, \
		       CLT_Const(TXTTABCNTL, FIRSTCOLOR, PRIMCOLOR),\
		       CLT_Const(TXTTABCNTL, SECONDCOLOR, TEXCOLOR),\
		       CLT_Const(TXTTABCNTL, THIRDCOLOR, PRIMCOLOR),\
		       CLT_Const(TXTTABCNTL, FIRSTALPHA, PRIMALPHA),\
		       CLT_Const(TXTTABCNTL, SECONDALPHA, PRIMALPHA),\
		       CLT_Const(TXTTABCNTL, COLOROUT, BLEND),\
		       CLT_Const(TXTTABCNTL, ALPHAOUT, PRIMALPHA),\
		       CLT_Const(TXTTABCNTL, BLENDOP, MULT)); \
	CLT_ESCNTL(pp,0, 0, 0); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBDISCARDCONTROL, 1); \
	*((*pp)++) = CLT_Bits(DBDISCARDCONTROL, SSB0, 1)

#define M_TBTex_Size            4
#define M_TBTex(pp) \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTTABCNTL, 1); \
	*((*pp)++) = CLT_SetConst(TXTTABCNTL, COLOROUT, TEXCOLOR) | CLT_SetConst(TXTTABCNTL, ALPHAOUT, TEXALPHA); \
	CLT_ESCNTL(pp,0, 0, 0); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBDISCARDCONTROL, 1); \
	*((*pp)++) = CLT_Bits(DBDISCARDCONTROL, SSB0, 1)

#define M_TBFog_Size            29
#define M_TBFog(pp, fogcolor) \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTCONST2, 2); \
	*((*pp)++) = fogcolor; \
	*((*pp)++) = fogcolor; \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTADDRCNTL, 1); \
	*((*pp)++) = CLA_TXTADDRCNTL(1, \
			CLT_Const(TXTADDRCNTL, MINFILTER, BILINEAR), \
			CLT_Const(TXTADDRCNTL, INTERFILTER, BILINEAR), \
			CLT_Const(TXTADDRCNTL, MAGFILTER, BILINEAR), \
			0); \
	*((*pp)++) = CLT_WriteRegistersHeader(ESCNTL, 1); \
	*((*pp)++) = CLA_ESCNTL(1, 0, 0); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTEXPTYPE, 1); \
	*((*pp)++) = CLA_TXTEXPTYPE(8, 7, 0, 1, 1, 1, 1); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTLDCNTL, 1); \
	*((*pp)++) = CLA_TXTLDCNTL(0, CLT_Const(TXTLDCNTL, LOADMODE, MMDMA), 0); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTCOUNT, 1); \
	*((*pp)++) = CLA_TXTCOUNT(17*4); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTUVMAX, 1); \
	*((*pp)++) = CLA_TXTUVMAX(16, 0); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTUVMASK, 1); \
	*((*pp)++) = CLA_TXTUVMASK(0x3ff, 0x3ff); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTLODBASE0, 1); \
	*((*pp)++) = 0; \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTPIPCNTL, 1); \
	*((*pp)++) = CLA_TXTPIPCNTL(CLT_Const(TXTPIPCNTL,PIPSSBSELECT,TEXTURE), \
			    CLT_Const(TXTPIPCNTL,PIPALPHASELECT,TEXTURE), \
			    CLT_Const(TXTPIPCNTL,PIPCOLORSELECT,TEXTURE), \
			    0); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTLDSRCADDR, 1); \
	*((*pp)++) = (uint32)&M_IntensityTexture[0]; \
	*((*pp)++) = CLT_WriteRegistersHeader(DCNTL,1); \
	*((*pp)++) = CLT_Bits(DCNTL, TLD, 1); \
	*((*pp)++) = CLT_WriteRegistersHeader(TXTTABCNTL, 1); \
	*((*pp)++) = CLA_TXTTABCNTL(CLT_Const(TXTTABCNTL, FIRSTCOLOR, CONSTCOLOR), \
		       CLT_Const(TXTTABCNTL, SECONDCOLOR, PRIMCOLOR), \
		       CLT_Const(TXTTABCNTL, THIRDCOLOR, TEXCOLOR), \
		       CLT_Const(TXTTABCNTL, FIRSTALPHA, PRIMALPHA),\
		       CLT_Const(TXTTABCNTL, SECONDALPHA, PRIMALPHA),\
		       CLT_Const(TXTTABCNTL, COLOROUT, BLEND), \
		       CLT_Const(TXTTABCNTL, ALPHAOUT, PRIMALPHA), \
		       CLT_Const(TXTTABCNTL, BLENDOP, LERP)); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBDISCARDCONTROL, 1); \
	*((*pp)++) = CLT_Bits(DBDISCARDCONTROL, SSB0, 1)

#define M_DBInit_Size		10
#define M_DBInit(pp, xmin, ymin, xmax, ymax) \
	*((*pp)++) = CLT_WriteRegistersHeader(DBUSERCONTROL,2); \
	*((*pp)++) = (CLT_Bits(DBUSERCONTROL,BLENDEN,0) | \
		      CLT_Bits(DBUSERCONTROL,ZBUFFEN,1) | \
		      CLT_Bits(DBUSERCONTROL,ZOUTEN,1) | \
		      CLT_Bits(DBUSERCONTROL,WINCLIPINEN,0) | \
		      CLT_Bits(DBUSERCONTROL,WINCLIPOUTEN,1) | \
		      CLT_Bits(DBUSERCONTROL,DITHEREN,0) | \
		      CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,ALPHA) | \
		      CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,RED) | \
		      CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,GREEN) | \
		      CLT_SetConst(DBUSERCONTROL,DESTOUTMASK,BLUE)); \
	*((*pp)++) = CLA_DBZOFFSET(0,0); \
	*((*pp)++) = CLT_WriteRegistersHeader(DBXWINCLIP,2);\
	*((*pp)++) = CLA_DBXWINCLIP(xmin,xmax); \
	*((*pp)++) = CLA_DBYWINCLIP(ymin,ymax); \
	*((*pp)++) = CLT_WriteRegistersHeader(DBSSBDSBCNTL,1); \
	*((*pp)++) = (CLT_Bits(DBSSBDSBCNTL,DSBCONST,1) | \
		      CLT_SetConst(DBSSBDSBCNTL,DSBSELECT,CONST)); \
	*((*pp)++) = CLT_WriteRegistersHeader(DBZCNTL, 1); \
	*((*pp)++) = CLA_DBZCNTL(0, 0, 0, 0, 1, 1)

#define M_DBNoBlend_Size        2
#define M_DBNoBlend(pp) \
	*((*pp)++) = CLT_ClearRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,BLENDEN,1) | CLT_Bits(DBUSERCONTROL,SRCEN,1)

#define M_DBFog_Size            13
#define M_DBFog(pp, fogcolor) \
	*((*pp)++) = CLT_WriteRegistersHeader(DBCONSTIN, 8); \
	*((*pp)++) = fogcolor; \
	*((*pp)++) = (CLT_SetConst(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR) | \
	      CLT_SetConst(DBAMULTCNTL, AMULTCOEFSELECT, TEXALPHA)| \
	      CLT_Bits(DBAMULTCNTL, AMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = (CLT_SetConst(DBBMULTCNTL, BINPUTSELECT, CONSTCOLOR) | \
	 CLT_SetConst(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHACOMPLEMENT) | \
	 CLT_Bits(DBBMULTCNTL, BMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = CLT_SetConst(DBALUCNTL, ALUOPERATION, A_PLUS_BCLAMP) | CLT_Bits(DBALUCNTL, FINALDIVIDE, 0); \
	*((*pp)++) = CLT_SetRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,BLENDEN,1); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,SRCEN,1)

#define M_DBSpec_Size           12
#define M_DBSpec(pp) \
	*((*pp)++) = CLT_WriteRegistersHeader(DBAMULTCNTL, 7); \
	*((*pp)++) = (CLT_SetConst(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR) | \
	      CLT_SetConst(DBAMULTCNTL, AMULTCOEFSELECT, CONST)| \
	      CLT_Bits(DBAMULTCNTL, AMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = (CLT_SetConst(DBBMULTCNTL, BINPUTSELECT, CONSTCOLOR) | \
	      CLT_SetConst(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHA) | \
	      CLT_Bits(DBBMULTCNTL, BMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = CLT_SetConst(DBALUCNTL, ALUOPERATION, A_PLUS_BCLAMP) | CLT_Bits(DBALUCNTL, FINALDIVIDE, 0); \
	*((*pp)++) = CLT_SetRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,BLENDEN,1); \
	*((*pp)++) = CLT_ClearRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,SRCEN,1)

#define M_DBTrans_Size          15
#define M_DBTrans(pp, srcaddr, fbwidth, deptheq32) \
	*((*pp)++) = CLT_WriteRegistersHeader(DBSRCCNTL, 4); \
	*((*pp)++) = CLA_DBSRCCNTL(1, deptheq32); \
	*((*pp)++) = srcaddr; \
	*((*pp)++) = fbwidth; \
	*((*pp)++) = 0; \
	*((*pp)++) = CLT_SetRegistersHeader(DBUSERCONTROL,1); \
	*((*pp)++) = CLT_Bits(DBUSERCONTROL,BLENDEN,1) | CLT_Bits(DBUSERCONTROL,SRCEN,1); \
	*((*pp)++) = CLT_WriteRegistersHeader(DBAMULTCNTL, 7); \
	*((*pp)++) = (CLT_SetConst(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR) | \
	      CLT_SetConst(DBAMULTCNTL, AMULTCOEFSELECT, TEXALPHA)| \
	      CLT_Bits(DBAMULTCNTL, AMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBAMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = (CLT_SetConst(DBBMULTCNTL, BINPUTSELECT, SRCCOLOR) | \
	      CLT_SetConst(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHACOMPLEMENT) | \
	      CLT_Bits(DBBMULTCNTL, BMULTRJUSTIFY, 0)); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB0(0xff,0xff,0xff); \
	*((*pp)++) = CLA_DBBMULTCONSTSSB1(0xff,0xff,0xff); \
	*((*pp)++) = CLT_SetConst(DBALUCNTL, ALUOPERATION, A_PLUS_BCLAMP) | CLT_Bits(DBALUCNTL, FINALDIVIDE, 0)

#endif /*__CLTMACROMACROS_H */
