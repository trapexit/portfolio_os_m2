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

	DECFN	M_DrawAALine
begincode:
		mflr		itemp0
		stw		itemp0,CloseData.drawlrsave2_off(pclosedata)

		mfcr		itemp0
		stw		itemp0,CloseData.cr(pclosedata)

		lfs		f0pt0aa,CloseData.fconst0pt0_off(pclosedata)
		lfs		f1pt0aa,CloseData.fconst1pt0_off(pclosedata)
		lfs		f0pt5aa,CloseData.fconst0pt5_off(pclosedata)

		lfs		fx0aa,xform.xpp(pdraw0AA)
		lfs		fy0aa,xform.ypp(pdraw0AA)
		lfs		fx1aa,xform.xpp(pdraw1AA)
		lfs		fy1aa,xform.ypp(pdraw1AA)

		fcmpu		0,fx0aa,f0pt0aa
		fcmpu		1,fy0aa,f0pt0aa
		fcmpu		2,fx1aa,f0pt0aa
		fcmpu		3,fy1aa,f0pt0aa
		bt		cr0lt,detoct5
		bt		cr1lt,detoct5
		bt		cr2lt,detoct5
		bt		cr3lt,detoct5

		lfs		fw0aa,xform.winv(pdraw0AA)
		lfs		fw1aa,xform.winv(pdraw1AA)

		lwz		itemp1,CloseData.ppodsave_off(pclosedata)
		lwz		itemp0,Pod.paadata_off(itemp1)
		lhz		itemp1,AAData.flags_off(itemp0)
		cmpi		0,itemp1,0
		beq		aaloadcolor

		lfs		fr0aa,AAData.color_off(itemp0)
		lfs		fg0aa,AAData.color_off+4(itemp0)
		lfs		fb0aa,AAData.color_off+8(itemp0)
		fmr		fr1aa,fr0aa
		fmr		fg1aa,fg0aa
		fmr		fb1aa,fb0aa
		b		aasetcolordone

aaloadcolor:
		lfs		fr0aa,xform.r(pdraw0AA)
		lfs		fg0aa,xform.g(pdraw0AA)
		lfs		fb0aa,xform.b(pdraw0AA)

		lfs		fr1aa,xform.r(pdraw1AA)
		lfs		fg1aa,xform.g(pdraw1AA)
		lfs		fb1aa,xform.b(pdraw1AA)

aasetcolordone:
		fsubs		fdxaa,fx1aa,fx0aa
		fsubs		fdyaa,fy1aa,fy0aa
		fsubs		ftmp0aa,f0pt0aa,fdxaa
		fsubs		ftmp1aa,f0pt0aa,fdyaa

		fsel		fdxaa,fdxaa,fdxaa,ftmp0aa
		fsel		fdyaa,fdyaa,fdyaa,ftmp1aa

		fcmpu		0,fx1aa,fx0aa
		crmove		x1gtx0FLAG,cr0gt
		crmove		x1eqx0FLAG,cr0eq
		crmove		x1ltx0FLAG,cr0lt

		fcmpu		0,fy1aa,fy0aa
		crmove		y1gty0FLAG,cr0gt
		crmove		y1eqy0FLAG,cr0eq
		crmove		y1lty0FLAG,cr0lt

		fcmpu		0,fdyaa,fdxaa
		crmove		dygtdxFLAG,cr0gt
		crnot		dyledxFLAG,cr0gt

		li		itemp0,3
		addis		itemp0,itemp0,0x200d
		stwu		itemp0,4(pVI)

		bf		y1eqy0FLAG,detoct0
		bl		drawv0v1
		stfs		f0pt5aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)
		bt		x1ltx0FLAG,oct_eee
/* 
 * y1 == y0, x1 >= x0
 */
oct_www:
/*		bf		prevclockwiseFLAG,oct_www_ccw	*/
/*oct_www_cw:	*/
/*		fsubs	ftmp0aa,fy0aa,f1pt0aa	*/
/*		fsubs	ftmp1aa,fy1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		b		detoct3	*/
/*oct_www_ccw:	*/
		fadds		ftmp0aa,fy0aa,f1pt0aa
		fadds		ftmp1aa,fy1aa,f1pt0aa
		stfs		ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)
		b		detoct5
/*
 * y1 = y0, x1 < x0
 */
oct_eee:
/*		bf		prevclockwiseFLAG,oct_eee_ccw	*/
/*oct_eee_cw:	*/
/*		fadds	ftmp0aa,fy0aa,f1pt0aa	*/
/*		fadds	ftmp1aa,fy1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		b		detoct5	*/
/*oct_eee_ccw:	*/
		fsubs		ftmp0aa,fy0aa,f1pt0aa
		fsubs		ftmp1aa,fy1aa,f1pt0aa
		stfs		ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)
		b		detoct3

detoct0:
		bf		x1eqx0FLAG,detoct1
		bl		drawv0v1
		stfs		f0pt5aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt5aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)
		bt		y1lty0FLAG,oct_sss
/*
 * x1 == x0, y1 >= y0
 */
oct_nnn:
/*		bf		prevclockwiseFLAG,oct_nnn_ccw	*/
/*oct_nnn_cw:	*/
/*		fadds	ftmp0aa,fx0aa,f1pt0aa	*/
/*		fadds	ftmp1aa,fx1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		b		detoct5	*/
/*oct_nnn_ccw:	*/
		fsubs		ftmp0aa,fx0aa,f1pt0aa
		fsubs		ftmp1aa,fx1aa,f1pt0aa
		stfs		ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)
		b		detoct3
/*
 * x1 == x0, y1 < y0
 */
oct_sss:
/*		bf		prevclockwiseFLAG,oct_sss_ccw	*/
/*oct_sss_cw:	*/
/*		fsubs	ftmp0aa,fx0aa,f1pt0aa	*/
/*		fsubs	ftmp1aa,fx1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		b		detoct3	*/
/*oct_sss_ccw:	*/
		fadds		ftmp0aa,fx0aa,f1pt0aa
		fadds		ftmp1aa,fx1aa,f1pt0aa
		stfs		ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)
		b		detoct5

detoct1:
		bf		y1lty0FLAG,detoct2
		bt		x1ltx0FLAG,detoct10
		bt		dyledxFLAG,oct_ene
/*
 * y1 < y0, x1 > x0, dy > dx
 */
oct_nne:
		bl		drawv1v0

/*		bf		prevclockwiseFLAG,oct_nne_ccw	*/
/*oct_nne_cw:	*/
/*		fsubs	ftmp1aa,fx1aa,f1pt0aa	*/
/*		fsubs	ftmp0aa,fx0aa,f1pt0aa	*/
/*		stfs	ftmp1aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp0aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct3	*/
/*oct_nne_ccw:	*/
		fadds		ftmp1aa,fx1aa,f1pt0aa
		fadds		ftmp0aa,fx0aa,f1pt0aa
		stfs		ftmp1aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp0aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct5
/*
 * y1 < y0, x1 > x0, dy <= dx
 */
oct_ene:
		bl		drawv0v1

/*		bf		prevclockwiseFLAG,oct_ene_ccw	*/
/*oct_ene_cw:	*/
/*		fsubs	ftmp0aa,fy0aa,f1pt0aa	*/
/*		fsubs	ftmp1aa,fy1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct3	*/
/*oct_ene_ccw:	*/
		fadds		ftmp0aa,fy0aa,f1pt0aa
		fadds		ftmp1aa,fy1aa,f1pt0aa
		stfs		ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct5
detoct10:
		bt		dygtdxFLAG,oct_nnw
/*
 * y1 < y0, x1 < x0, dy <= dx
 */
oct_wnw:
		bl		drawv1v0

/*		bf		prevclockwiseFLAG,oct_wnw_ccw	*/
/*oct_wnw_cw:	*/
/*		fadds	ftmp1aa,fy1aa,f1pt0aa	*/
/*		fadds	ftmp0aa,fy0aa,f1pt0aa	*/
/*		stfs	ftmp1aa,(-(4*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp0aa,(-(2*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct5	*/
/*oct_wnw_ccw:	*/
		fsubs		ftmp1aa,fy1aa,f1pt0aa
		fsubs		ftmp0aa,fy0aa,f1pt0aa
		stfs		ftmp1aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp0aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct3
/*
 * y1 < y0, x1 < x0, dy > dx
 */
oct_nnw:
		bl		drawv1v0

/*		bf		prevclockwiseFLAG,oct_nnw_ccw	*/
/*oct_nnw_cw:	*/
/*		fsubs	ftmp1aa,fx1aa,f1pt0aa	*/
/*		fsubs	ftmp0aa,fx0aa,f1pt0aa	*/
/*		stfs	ftmp1aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp0aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct3	*/
/*oct_nnw_ccw:	*/
		fadds		ftmp1aa,fx1aa,f1pt0aa
		fadds		ftmp0aa,fx0aa,f1pt0aa
		stfs		ftmp1aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp0aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct5

detoct2:
		bf		y1gty0FLAG,detoct3
		bt		x1ltx0FLAG,detoct20
		bt		dygtdxFLAG,oct_sse
/*
 * y1 > y0, x1 > x0, dy <= dx
 */
oct_ese:
		bl		drawv0v1

/*		bf		prevclockwiseFLAG,oct_ese_ccw	*/
/*oct_ese_cw:	*/
/*		fsubs	ftmp0aa,fy0aa,f1pt0aa	*/
/*		fsubs	ftmp1aa,fy1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct3	*/
/*oct_ese_ccw:	*/
		fadds		ftmp0aa,fy0aa,f1pt0aa
		fadds		ftmp1aa,fy1aa,f1pt0aa
		stfs		ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct5
/*
 * y1 > y0, x1 > x0, dy > dx
 */
oct_sse:
		bl		drawv0v1

/*		bf		prevclockwiseFLAG,oct_sse_ccw	*/
/*oct_sse_cw:	*/
/*		fadds	ftmp0aa,fx0aa,f1pt0aa	*/
/*		fadds	ftmp1aa,fx1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct5	*/
/*oct_sse_ccw:	*/
		fsubs		ftmp0aa,fx0aa,f1pt0aa
		fsubs		ftmp1aa,fx1aa,f1pt0aa
		stfs		ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct3
detoct20:
		bt		dyledxFLAG,oct_wsw
/*
 * y1 > y0, x1 < x0, dy > dx
 */
oct_ssw:
		bl		drawv0v1

/*		bf		prevclockwiseFLAG,oct_ssw_ccw	*/
/*oct_ssw_cw:	*/
/*		fadds	ftmp0aa,fx0aa,f1pt0aa	*/
/*		fadds	ftmp1aa,fx1aa,f1pt0aa	*/
/*		stfs	ftmp0aa,(-(3*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	ftmp1aa,(-(1*outvtxaa)+4+outvtxaa.x)(pVI)	*/
/*		stfs	f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct5	*/
/*oct_ssw_ccw:	*/
		fsubs		ftmp0aa,fx0aa,f1pt0aa
		fsubs		ftmp1aa,fx1aa,f1pt0aa
		stfs		ftmp0aa,(-(4*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		ftmp1aa,(-(2*outvtxaa)+4+outvtxaa.x)(pVI)
		stfs		f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct3
/*
 * y1 > y0, x1 < x0, dy <= dx
 */
oct_wsw:
		bl		drawv1v0

/*		bf		prevclockwiseFLAG,oct_wsw_ccw	*/
/*oct_wsw_cw:	*/
/*		fadds	ftmp1aa,fy1aa,f1pt0aa	*/
/*		fadds	ftmp0aa,fy0aa,f1pt0aa	*/
/*		stfs	ftmp1aa,(-(4*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	ftmp0aa,(-(2*outvtxaa)+4+outvtxaa.y)(pVI)	*/
/*		stfs	f0pt0aa,(-(4*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		stfs	f0pt0aa,(-(2*outvtxaa)+4+outvtxaa.a)(pVI)	*/
/*		b		detoct5	*/
/*oct_wsw_ccw:	*/
		fsubs		ftmp1aa,fy1aa,f1pt0aa
		fsubs		ftmp0aa,fy0aa,f1pt0aa
		stfs		ftmp1aa,(-(3*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		ftmp0aa,(-(1*outvtxaa)+4+outvtxaa.y)(pVI)
		stfs		f0pt0aa,(-(3*outvtxaa)+4+outvtxaa.a)(pVI)
		stfs		f0pt0aa,(-(1*outvtxaa)+4+outvtxaa.a)(pVI)
		b		detoct3
detoct3:
		fcmpu		0,ftmp0aa,f0pt0aa
		fcmpu		1,ftmp1aa,f0pt0aa
		bt		cr0lt,detoct4
		bt		cr1lt,detoct4
detoct5:
		cmpl		0,pVI,pVIwritemax
		bgel-		M_ClistManager
		
		lwz		itemp0,CloseData.cr(pclosedata)
		mtcr		itemp0

		lwz		itemp0,CloseData.drawlrsave2_off(pclosedata)
		mtlr		itemp0
		blr

detoct4:
		subi		pVI,pVI,4*outvtxaa+4
		b		detoct5

drawv0v1:
		stfsu		fx0aa,4(pVI)
		stfsu		fy0aa,4(pVI)
		stfsu		fr0aa,4(pVI)
		stfsu		fg0aa,4(pVI)
		stfsu		fb0aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw0aa,4(pVI)

		stfsu		fx0aa,4(pVI)
		stfsu		fy0aa,4(pVI)
		stfsu		fr0aa,4(pVI)
		stfsu		fg0aa,4(pVI)
		stfsu		fb0aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw0aa,4(pVI)

		stfsu		fx1aa,4(pVI)
		stfsu		fy1aa,4(pVI)
		stfsu		fr1aa,4(pVI)
		stfsu		fg1aa,4(pVI)
		stfsu		fb1aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw1aa,4(pVI)

		stfsu		fx1aa,4(pVI)
		stfsu		fy1aa,4(pVI)
		stfsu		fr1aa,4(pVI)
		stfsu		fg1aa,4(pVI)
		stfsu		fb1aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw1aa,4(pVI)
		blr

drawv1v0:
		stfsu		fx1aa,4(pVI)
		stfsu		fy1aa,4(pVI)
		stfsu		fr1aa,4(pVI)
		stfsu		fg1aa,4(pVI)
		stfsu		fb1aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw1aa,4(pVI)

		stfsu		fx1aa,4(pVI)
		stfsu		fy1aa,4(pVI)
		stfsu		fr1aa,4(pVI)
		stfsu		fg1aa,4(pVI)
		stfsu		fb1aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw1aa,4(pVI)

		stfsu		fx0aa,4(pVI)
		stfsu		fy0aa,4(pVI)
		stfsu		fr0aa,4(pVI)
		stfsu		fg0aa,4(pVI)
		stfsu		fb0aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw0aa,4(pVI)

		stfsu		fx0aa,4(pVI)
		stfsu		fy0aa,4(pVI)
		stfsu		fr0aa,4(pVI)
		stfsu		fg0aa,4(pVI)
		stfsu		fb0aa,4(pVI)
		stfsu		f1pt0aa,4(pVI)
		stfsu		fw0aa,4(pVI)
		blr

M_ClistManager:
		lea		3,M_ClistManagerC
		b		M_SaveAndCallC

endcode:
		.space		4096-(endcode-begincode)

/*	END  */ 

