 	.include 	"structmacros.i"			/* assembler convert helpers (chs 12/14/95) */
	.include	"PPCMacroequ.i"
	.include	"conditionmacros.i"
	.include	"mercury.i"
	.include	"M_Draw.i"
	.include	"M_DrawCommon.i"

/*	INITIALIZATION REGISTERS */
/*	GPRs 4 to 22 are defined in podloop.i.  All GPRs 23 and up may
 *	be used during initialization.
 *	FPRs 0 to 11 are the concatenated matrix, which is not needed but must
 *	be preserved.  FPRs 12 to 20 and 24 to 26 are the inverted matrix,
 *	defined in podloop.i. FPRs 21 to 23 and 27 to 31 may be used during
 *	initialization.
 */
	define		intcolindex	,	24
	define		intcolr		,	25
	define		intcolg		,	26
	define		intcolb		,	27
	define		pdata		,	28
	define		copycount	,	29
	define		lksave		,	30
	define		clsave		,	31

	define		fi_x		,	21
	define		fi_y		,	22
	define		fi_z		,	23
	define		fi_xprime	,	27
	define		fi_yprime	,	28
	define		fi_zprime	,	29

	define		fi_maxdist	,	30
	define		fi_dist		,	31

	define		fi_colr		,	21
	define		fi_colg		,	22
	define		fi_colb		,	23
	define		fi_colr2	,	27
	define		fi_colg2	,	28
	define		fi_colb2	,	29
	define		fi_const255	,	30
	define		fi_const0	,	30
	define		fi_const1	,	28

/*	LIGHT CALCULATION REGISTERS */
/*	The light routines can use fltemp0 to fltemp5.
 *	They can also use integer regs ?? (well, itemp0 anyway)
 */

