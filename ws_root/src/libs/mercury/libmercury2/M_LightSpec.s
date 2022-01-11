	.include	"M_LightCommon.i"
	.include	"M_LightSpec.i"

	define		start,	fx0
	define		mid,	fx1
	define		a,	fy0
	define		b,	fy1
	define		c,	fz0
	define		d,	fz1


/*
 *	Utilities used by all specular lighting at draw time
 *	Can use fltemp0 .. fltemp5
 *	Input is spec_in
 *	Output is spec_out
 */

	DECFN	M_LightSpec
	lfs	start,DirSpec.specdata+SpecData.start(plightlist)
	lfs	mid,DirSpec.specdata+SpecData.mid(plightlist)
	fcmpu	0,spec_in,start
	fcmpu	1,spec_in,mid
	fcmpu	2,spec_in,spec_const1
	blt	0,ret0
	bge	2,ret1
	
	blt	1,spline0

spline1:	
	lfs	d,DirSpec.specdata+SpecData.d1(plightlist)
	lfs	c,DirSpec.specdata+SpecData.c1(plightlist)
	lfs	b,DirSpec.specdata+SpecData.b1(plightlist)
	lfs	a,DirSpec.specdata+SpecData.a1(plightlist)
	b	doit

spline0:	
	lfs	d,DirSpec.specdata+SpecData.d0(plightlist)
	lfs	c,DirSpec.specdata+SpecData.c0(plightlist)
	lfs	b,DirSpec.specdata+SpecData.b0(plightlist)
	lfs	a,DirSpec.specdata+SpecData.a0(plightlist)
doit:	
	
	fmadds	d,spec_in,c,d
	fmuls	c,spec_in,spec_in
	fmadds	d,b,c,d
	fmuls	c,c,spec_in
	fmadds	spec_out,a,c,d
	blr
	
ret0:	fmr	spec_out, spec_const0
	blr


ret1:	fmr	spec_out, spec_const1
	blr

