/*
 * Definitions for specular lighting functions
 *
 */

		define		spec_in		,	fny0
		define		spec_out	,	fny1

		define		spec_const0	,	fnz0
		define		spec_const1	,	fnz1

	struct	DirSpec
		stlong		DirSpec.Code,		1
		stlong		DirSpec.nx	,	1
		stlong		DirSpec.ny	,	1
		stlong		DirSpec.nz	,	1
		stlong		DirSpec.specdata,	10
		stlong		DirSpec.sr	,	1
		stlong		DirSpec.sg	,	1
		stlong		DirSpec.sb	,	1
		stlong		DirSpec.r	,	1
		stlong		DirSpec.g	,	1
		stlong		DirSpec.b	,	1
		stlong		DirSpec.Next,		1
	ends	DirSpec


	struct SpecData
		stlong		SpecData.start	,	1
		stlong		SpecData.mid	,	1
		stlong		SpecData.a0	,	1
		stlong		SpecData.b0	,	1
		stlong		SpecData.c0	,	1
		stlong		SpecData.d0	,	1
		stlong		SpecData.a1	,	1
		stlong		SpecData.b1	,	1
		stlong		SpecData.c1	,	1
		stlong		SpecData.d1	,	1
	ends SpecData
