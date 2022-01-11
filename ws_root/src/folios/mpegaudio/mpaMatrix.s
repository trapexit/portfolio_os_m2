/**********************************************************************
 *
 *	mpaMatrix.s
 *
 *	MPEG audio matrix (DCT) function  - uses a 32 point fast
 *	cosine transform as described by Byeong Gi Lee in IEEE
 *	Transactions ASSP-32 Part 2, August 1984, "A New Algorithm
 *	to Compute the Discrete Cosine Transform".
 *
 *********************************************************************/

#include "mpaMatrix.i"

/*#define NO_TOUCHLOADS 1*/

/**********************************************************************
 *
 *	mpaMatrix
 *
 *	This assembly routine does the matrixing for sblimit=8,12,27,30
 *	and places the data into an output sample array in
 *	preparation for windowing.  Takes in sblimit frequency samples,
 *	produces 32 output samples, uses 31 terms of 1/2*cos(n*PI/64)
 *
 *	NOTE:	for notes on operations, see butterly diagram of
 *	matrixing (MS Excel document)
 *
 *********************************************************************/
/*********************************************************************
 *
 *	mpaMatrix entry point
 *
 *********************************************************************/

	DECFN	mpaMatrix

/* preserve registers   */
	mflr	SaveLR
/* init pointers and stuff */
	li	groupCount,	36	/* 36 groups to process */
/* now begin */
	cmpi	sblimit, 27		/* compare sblimit to 27 */
	blt	Limit12			/* if less than do a smaller loop */
Limit30:	
/*** do odd output samples first ***/
/** do inner order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f8,	8*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample  8 */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample  9 */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 10 */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 11 */
	lfs	f12,	12*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 12 */
	lfs	f13,	13*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 13 */
	lfs	f14,	14*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 14 */
	lfs	f15,	15*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 15 */
/* load in matrix coefficients, high */
	lfs	f16,	16*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 16 */
	lfs	f17,	17*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 17 */
	lfs	f18,	18*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 18 */
	lfs	f19,	19*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 19 */
	lfs	f20,	20*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 20 */
	lfs	f21,	21*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 21 */
	lfs	f22,	22*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 22 */
	lfs	f23,	23*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 23 */
/* load in matrix multipliers for stage 0 mults */
	lfs	f24,	COS17_64(matrixCoef)	/* 1/(2*cos(17*pi/64)) */
	lfs	f25,	COS19_64(matrixCoef)	/* 1/(2*cos(19*pi/64)) */
	lfs	f26,	COS21_64(matrixCoef)	/* 1/(2*cos(21*pi/64)) */
	lfs	f27,	COS23_64(matrixCoef)	/* 1/(2*cos(23*pi/64)) */
	lfs	f28,	COS25_64(matrixCoef)	/* 1/(2*cos(25*pi/64)) */
	lfs	f29,	COS27_64(matrixCoef)	/* 1/(2*cos(27*pi/64)) */
	lfs	f30,	COS29_64(matrixCoef)	/* 1/(2*cos(29*pi/64)) */
	lfs	f31,	COS31_64(matrixCoef)	/* 1/(2*cos(31*pi/64)) */
/* touchload for inputs */
	addi	touchInOffset,	r0, 24*AUDIO_FLOAT_SIZE	/* offset for sample 24 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/* compute bottom half stage 0 plus/minus for odd output samples */
	fsubs	f16,	f15, f16		/* stage 0, result 24 */
	fsubs	f17,	f14, f17		/* stage 0, result 25 */
	fsubs	f18,	f13, f18		/* stage 0, result 27 */
	fsubs	f19,	f12, f19		/* stage 0, result 26 */
	fsubs	f20,	f11, f20		/* stage 0, result 30 */
	fsubs	f21,	f10, f21		/* stage 0, result 31 */
	fsubs	f22,	f9,  f22		/* stage 0, result 29 */
	fsubs	f23,	f8,  f23		/* stage 0, result 28 */
/* compute bottom half stage 0 mults for odd output samples */
	fmuls	f23,	f23, f24		/* stage 0m, result 28 */
	fmuls	f22,	f22, f25		/* stage 0m, result 29 */
	fmuls	f21,	f21, f26		/* stage 0m, result 31 */
	fmuls	f20,	f20, f27		/* stage 0m, result 30 */
	fmuls	f19,	f19, f28		/* stage 0m, result 26 */
	fmuls	f18,	f18, f29		/* stage 0m, result 27 */
	fmuls	f17,	f17, f30		/* stage 0m, result 25 */
	fmuls	f16,	f16, f31		/* stage 0m, result 24 */
/** do outer order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f0,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f1,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* load in matrix coefficients, high */
	lfs	f24,	24*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 24 */
	lfs	f25,	25*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 25 */
	lfs	f26,	26*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 26 */
/* compute upper half stage 0 plus/minus for odd output samples */
	fsubs	f7,	f7, f24			/* stage 0, result 20 */
	fsubs	f6,	f6, f25			/* stage 0, result 21 */
	fsubs	f5,	f5, f26			/* stage 0, result 23 */
	cmpwi	sblimit, 27			/* check for sblimit = 27 */
	beq	Skip27				/* if true skip next stuff */
	lfs	f27,	27*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 27 */
	lfs	f28,	28*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 28 */
	lfs	f29,	29*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 29 */
	fsubs	f4,	f4, f27			/* stage 0, result 22 */
	fsubs	f3,	f3, f28			/* stage 0, result 18 */
	fsubs	f2,	f2, f29			/* stage 0, result 19 */
Skip27:	
/* load in matrix multipliers */
	lfs	f24,	COS1_64(matrixCoef)	/* 1/(2*cos(1*pi/64))  */
	lfs	f25,	COS3_64(matrixCoef)	/* 1/(2*cos(3*pi/64))  */
	lfs	f26,	COS5_64(matrixCoef)	/* 1/(2*cos(5*pi/64))  */
	lfs	f27,	COS7_64(matrixCoef)	/* 1/(2*cos(7*pi/64))  */
	lfs	f28,	COS9_64(matrixCoef)	/* 1/(2*cos(9*pi/64))  */
	lfs	f29,	COS11_64(matrixCoef)	/* 1/(2*cos(11*pi/64)) */
	lfs	f30,	COS13_64(matrixCoef)	/* 1/(2*cos(13*pi/64)) */
	lfs	f31,	COS15_64(matrixCoef)	/* 1/(2*cos(15*pi/64)) */
/* touchload for inputs */
	addi	touchInOffset,	r0, 40*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 8 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/** compute stage 0m and stage 1 **/
	fmsubs	f8,	f24, f0, f16		/* stage 1, result 24 */
	fmsubs	f9,	f25, f1, f17		/* stage 1, result 25 */
	fmsubs	f10,	f26, f2, f18		/* stage 1, result 27 */
	fmsubs	f11,	f27, f3, f19		/* stage 1, result 26 */
	fmsubs	f12,	f28, f4, f20		/* stage 1, result 30 */
	fmsubs	f13,	f29, f5, f21		/* stage 1, result 31 */
	fmsubs	f14,	f30, f6, f22		/* stage 1, result 29 */
	fmsubs	f15,	f31, f7, f23		/* stage 1, result 28 */
	fmadds	f0,	f24, f0, f16		/* stage 1, result 16 */
	fmadds	f1,	f25, f1, f17		/* stage 1, result 17 */
	fmadds	f2,	f26, f2, f18		/* stage 1, result 19 */
	fmadds	f3,	f27, f3, f19		/* stage 1, result 18 */
	fmadds	f4,	f28, f4, f20		/* stage 1, result 22 */
	fmadds	f5,	f29, f5, f21		/* stage 1, result 23 */
	fmadds	f6,	f30, f6, f22		/* stage 1, result 21 */
	fmadds	f7,	f31, f7, f23		/* stage 1, result 20 */
/* init touchload offset for outputs */
	li	touchOutOffset,	73*AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_odd

/*** do even output samples ***/
/** do inner order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f31,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f30,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* load in matrix coefficients, midlow */ 
	lfs	f8,	8*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 8 */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 9 */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 10 */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 11 */
	lfs	f12,	12*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 12 */
	lfs	f13,	13*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 13 */
	lfs	f14,	14*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 14 */
	lfs	f15,	15*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 15 */
/* load in matrix coefficients, midhigh */
	lfs	f16,	16*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 16 */
	lfs	f17,	17*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 17 */
	lfs	f18,	18*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 18 */
	lfs	f19,	19*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 19 */
	lfs	f20,	20*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 20 */
	lfs	f21,	21*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 21 */
	lfs	f22,	22*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 22 */
	lfs	f23,	23*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 23 */
/* load in matrix coefficients, high */
	cmpwi	sblimit, 27			/* check for sblimit=27 */
	beq	Skip27B				/* if true skip */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f27,	27*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 27 */
	lfs	f28,	28*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 28 */
	lfs	f29,	29*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 29 */
	fadds	f27,	f4,  f27		/* stage 0, result  6 */
	fadds	f28,	f3,  f28		/* stage 0, result  2 */
	fadds	f29,	f2,  f29		/* stage 0, result  3 */
	b	Skip27C
Skip27B:	
	lfs	f29,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f28,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f27,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
Skip27C:	
	lfs	f24,	24*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 24 */
	lfs	f25,	25*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 25 */
	lfs	f26,	26*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 26 */
/* touchload for inputs */
	addi	touchInOffset,	r0, 48*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 16 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp		/* do touch load */
#endif
/* compute stage 0 plus/minus for even output samples */
	fadds	f24,	f7,  f24		/* stage 0, result  4 */
	fadds	f25,	f6,  f25		/* stage 0, result  5 */
	fadds	f26,	f5,  f26		/* stage 0, result  7 */
	fadds	f16,	f15, f16		/* stage 0, result  8 */
	fadds	f17,	f14, f17		/* stage 0, result  9 */
	fadds	f19,	f12, f19		/* stage 0, result 10 */
	fadds	f18,	f13, f18		/* stage 0, result 11 */
	fadds	f23,	f8,  f23		/* stage 0, result 12 */
	fadds	f22,	f9,  f22		/* stage 0, result 13 */
	fadds	f20,	f11, f20		/* stage 0, result 14 */
	fadds	f21,	f10, f21		/* stage 0, result 15 */
/* touchload for inputs */
	addi	touchInOffset,	r0, 32*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 0 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/* compute plus/minus stage 1 */
	fadds	f0,	f31, f16		/* stage 1, result  0 */
	fadds	f1,	f30, f17		/* stage 1, result  1 */
	fadds	f2,	f29, f18		/* stage 1, result  3 */
	fadds	f3,	f28, f19		/* stage 1, result  2 */
	fadds	f4,	f27, f20		/* stage 1, result  6 */
	fadds	f5,	f26, f21		/* stage 1, result  7 */
	fadds	f6,	f25, f22		/* stage 1, result  5 */
	fadds	f7,	f24, f23		/* stage 1, result  4 */
	fsubs	f8,	f31, f16		/* stage 1, result  8 */
	fsubs	f9,	f30, f17		/* stage 1, result  9 */
	fsubs	f10,	f29, f18		/* stage 1, result 11 */
	fsubs	f11,	f28, f19		/* stage 1, result 10 */
	fsubs	f12,	f27, f20		/* stage 1, result 14 */
	fsubs	f13,	f26, f21		/* stage 1, result 15 */
	fsubs	f14,	f25, f22		/* stage 1, result 13 */
	fsubs	f15,	f24, f23		/* stage 1, result 12 */
/* init touchload offset for outputs */
	li	touchOutOffset,	AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_even
/* loop if necessary */
	addic.	groupCount,	groupCount,	-1
	addi	inputSamp,	inputSamp,	32*AUDIO_FLOAT_SIZE
	addi	outputSamp,	outputSamp,	2*AUDIO_FLOAT_SIZE
	bgt	Limit30
	addic.	nch,	nch,	-1
	addi	outputSamp,	outputSamp,	1080*AUDIO_FLOAT_SIZE
	addi 	groupCount,	r0,	36	
	bgt	Limit30
/* jump to end */
	b	EndMatrix

Limit12:	
	cmpi	sblimit,	12		/* compare sblimit to 12 */
	blt	Limit8				/* if less than do a smaller loop */
/* load in matrix coefficients, low */
	lfs	f8,	8*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample  8 */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample  9 */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 10 */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 11 */
/* load in matrix multipliers for stage 0 mults */
	lfs	f24,	COS17_64(matrixCoef)	/* 1/(2*cos(17*pi/64)) */
	lfs	f25,	COS19_64(matrixCoef)	/* 1/(2*cos(19*pi/64)) */
	lfs	f26,	COS21_64(matrixCoef)	/* 1/(2*cos(21*pi/64)) */
	lfs	f27,	COS23_64(matrixCoef)	/* 1/(2*cos(23*pi/64)) */
/* compute bottom half stage 0 mults for odd output samples */
	fmuls	f23,	f8, f24			/* stage 0m, result 28 */
	fmuls	f22,	f9, f25			/* stage 0m, result 29 */
	fmuls	f21,	f10, f26		/* stage 0m, result 31 */
	fmuls	f20,	f11, f27		/* stage 0m, result 30 */
/** do outer order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f0,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f1,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* load in matrix multipliers */
	lfs	f24,	COS1_64(matrixCoef)	/* 1/(2*cos(1*pi/64))  */
	lfs	f25,	COS3_64(matrixCoef)	/* 1/(2*cos(3*pi/64))  */
	lfs	f26,	COS5_64(matrixCoef)	/* 1/(2*cos(5*pi/64))  */
	lfs	f27,	COS7_64(matrixCoef)	/* 1/(2*cos(7*pi/64))  */
	lfs	f28,	COS9_64(matrixCoef)	/* 1/(2*cos(9*pi/64))  */
	lfs	f29,	COS11_64(matrixCoef)	/* 1/(2*cos(11*pi/64)) */
	lfs	f30,	COS13_64(matrixCoef)	/* 1/(2*cos(13*pi/64)) */
	lfs	f31,	COS15_64(matrixCoef)	/* 1/(2*cos(15*pi/64)) */
/* touchload for inputs */
	addi	touchInOffset,	r0, 40*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 8 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/** compute stage 0m and stage 1 **/
	fmuls	f8,	f24, f0			/* stage 1, result 24 */
	fmuls	f9,	f25, f1			/* stage 1, result 25 */
	fmuls	f10,	f26, f2			/* stage 1, result 27 */
	fmuls	f11,	f27, f3			/* stage 1, result 26 */
	fmsubs	f12,	f28, f4, f20		/* stage 1, result 30 */
	fmsubs	f13,	f29, f5, f21		/* stage 1, result 31 */
	fmsubs	f14,	f30, f6, f22		/* stage 1, result 29 */
	fmsubs	f15,	f31, f7, f23		/* stage 1, result 28 */
	fmr	f0,	f8			/* stage 1, result 16 */
	fmr	f1,	f9			/* stage 1, result 17 */
	fmr	f2,	f10			/* stage 1, result 19 */
	fmr	f3,	f11			/* stage 1, result 18 */
	fmadds	f4,	f28, f4, f20		/* stage 1, result 22 */
	fmadds	f5,	f29, f5, f21		/* stage 1, result 23 */
	fmadds	f6,	f30, f6, f22		/* stage 1, result 21 */
	fmadds	f7,	f31, f7, f23		/* stage 1, result 20 */
/* init touchload offset for outputs */
	li	touchOutOffset,	73*AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_odd

/*** do even output samples ***/
/** do inner order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f0,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f1,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* load in matrix coefficients, midlow */ 
	lfs	f16,	8*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 8 */
	lfs	f17,	9*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 9 */
	lfs	f18,	10*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 10 */
	lfs	f19,	11*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample 11 */
/* touchload for inputs */
	addi	touchInOffset,	r0, 32*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 0 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/* compute plus/minus stage 1 */
	fmr	f8,	f0			/* stage 1, result  8 */
	fmr	f9,	f1			/* stage 1, result  9 */
	fmr	f10,	f2			/* stage 1, result 11 */
	fmr	f11,	f3			/* stage 1, result 10 */
	fsubs	f12,	f4, f19			/* stage 1, result 14 */
	fsubs	f13,	f5, f18			/* stage 1, result 15 */
	fsubs	f14,	f6, f17			/* stage 1, result 13 */
	fsubs	f15,	f7, f16			/* stage 1, result 12 */
	fadds	f4,	f4, f19			/* stage 1, result  6 */
	fadds	f5,	f5, f18			/* stage 1, result  7 */
	fadds	f6,	f6, f17			/* stage 1, result  5 */
	fadds	f7,	f7, f16			/* stage 1, result  4 */
/* init touchload offset for outputs */
	li	touchOutOffset,	AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_even
/* loop if necessary */
	addic.	groupCount,	groupCount,	-1
	addi	inputSamp,	inputSamp,	32*AUDIO_FLOAT_SIZE
	addi	outputSamp,	outputSamp,	2*AUDIO_FLOAT_SIZE
	bgt	Limit12
	addic.	nch,	nch,	-1
	addi	outputSamp,	outputSamp,	1080*AUDIO_FLOAT_SIZE
	addi 	groupCount,	r0,	36	
	bgt	Limit12
/* jump to end */
	b	EndMatrix

Limit8:	
/** do outer order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f0,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f1,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* load in matrix multipliers */
	lfs	f24,	COS1_64(matrixCoef)	/* 1/(2*cos(1*pi/64))  */
	lfs	f25,	COS3_64(matrixCoef)	/* 1/(2*cos(3*pi/64))  */
	lfs	f26,	COS5_64(matrixCoef)	/* 1/(2*cos(5*pi/64))  */
	lfs	f27,	COS7_64(matrixCoef)	/* 1/(2*cos(7*pi/64))  */
	lfs	f28,	COS9_64(matrixCoef)	/* 1/(2*cos(9*pi/64))  */
	lfs	f29,	COS11_64(matrixCoef)	/* 1/(2*cos(11*pi/64)) */
	lfs	f30,	COS13_64(matrixCoef)	/* 1/(2*cos(13*pi/64)) */
	lfs	f31,	COS15_64(matrixCoef)	/* 1/(2*cos(15*pi/64)) */
/* touchload for inputs */
	addi	touchInOffset,	r0, 32*AUDIO_FLOAT_SIZE	/* offset for next matrix sample 0 cache-line */
#ifndef NO_TOUCHLOADS
	dcbt	touchInOffset,	inputSamp	/* do touch load */
#endif
/** compute stage 0m and stage 1 **/
	fmuls	f8,	f24, f0			/* stage 1, result 24 */
	fmuls	f9,	f25, f1			/* stage 1, result 25 */
	fmuls	f10,	f26, f2			/* stage 1, result 27 */
	fmuls	f11,	f27, f3			/* stage 1, result 26 */
	fmuls	f12,	f28, f4			/* stage 1, result 30 */
	fmuls	f13,	f29, f5			/* stage 1, result 31 */
	fmuls	f14,	f30, f6			/* stage 1, result 29 */
	fmuls	f15,	f31, f7			/* stage 1, result 28 */
	fmr	f0,	f8			/* stage 1, result 16 */
	fmr	f1,	f9			/* stage 1, result 17 */
	fmr	f2,	f10			/* stage 1, result 19 */
	fmr	f3,	f11			/* stage 1, result 18 */
	fmr	f4,	f12			/* stage 1, result 22 */
	fmr	f5,	f13			/* stage 1, result 23 */
	fmr	f6,	f14			/* stage 1, result 21 */
	fmr	f7,	f15			/* stage 1, result 20 */
/* init touchload offset for outputs */
	li	touchOutOffset,	73*AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_odd

/*** do even output samples ***/
/** do inner order samples, stage 0 **/
/* load in matrix coefficients, low */
	lfs	f0,	0*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 0 */
	lfs	f1,	1*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 1 */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 2 */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 3 */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 4 */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 5 */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 6 */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(inputSamp)	/* input sample	 7 */
/* compute plus/minus stage 1 */
	fmr	f8,	f0			/* stage 1, result  8 */
	fmr	f9,	f1			/* stage 1, result  9 */
	fmr	f10,	f2			/* stage 1, result 11 */
	fmr	f11,	f3			/* stage 1, result 10 */
	fmr	f12,	f4			/* stage 1, result 14 */
	fmr	f13,	f5			/* stage 1, result 15 */
	fmr	f14,	f6			/* stage 1, result 13 */
	fmr	f15,	f7			/* stage 1, result 12 */
/* init touchload offset for outputs */
	li	touchOutOffset,	AUDIO_FLOAT_SIZE
/** for odd and even samples stages 1m through 7 are identical **/
	bl	Stages1m_7
/** compute final sums and store **/
	bl	Stage8_even
/* loop if necessary */
	addic.	groupCount,	groupCount,	-1
	addi	inputSamp,	inputSamp,	32*AUDIO_FLOAT_SIZE
	addi	outputSamp,	outputSamp,	2*AUDIO_FLOAT_SIZE
	bgt	Limit8
	addic.	nch,	nch,	-1
	addi	outputSamp,	outputSamp,	1080*AUDIO_FLOAT_SIZE
	addi 	groupCount,	r0,	36	
	bgt	Limit8
/* jump to end */
	b	EndMatrix

Stages1m_7:					/* f0-f15 contain input samples */
/* load in matrix multipliers */
	lfs	f24,	COS1_32(matrixCoef)	/* 1/(2*cos(1*pi/32))  */
	lfs	f25,	COS3_32(matrixCoef)	/* 1/(2*cos(3*pi/32))  */
	lfs	f26,	COS5_32(matrixCoef)	/* 1/(2*cos(5*pi/32))  */
	lfs	f27,	COS7_32(matrixCoef)	/* 1/(2*cos(7*pi/32))  */
	lfs	f28,	COS9_32(matrixCoef)	/* 1/(2*cos(9*pi/32))  */
	lfs	f29,	COS11_32(matrixCoef)	/* 1/(2*cos(11*pi/32)) */
	lfs	f30,	COS13_32(matrixCoef)	/* 1/(2*cos(13*pi/32)) */
	lfs	f31,	COS15_32(matrixCoef)	/* 1/(2*cos(15*pi/32)) */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 1 mult bottom half */
	fmuls	f28,	f12, f28		/* stage 1m, result 14 */
	fmuls	f29,	f13, f29		/* stage 1m, result 15 */
	fmuls	f30,	f14, f30		/* stage 1m, result 13 */
	fmuls	f31,	f15, f31		/* stage 1m, result 12 */
/* stage 1 mult top half with stage 2 plus/minus */
	fmsubs	f16,	f27, f11, f28		/* stage 2, result 14 */
	fmsubs	f17,	f26, f10, f29		/* stage 2, result 15 */
	fmsubs	f18,	f25, f9, f30		/* stage 2, result 13 */
	fmsubs	f19,	f24, f8, f31		/* stage 2, result 12 */
	fmadds	f20,	f27, f11, f28		/* stage 2, result 10 */
	fmadds	f21,	f26, f10, f29		/* stage 2, result 11 */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 1 mult top half with stage 2 plus/minus */
	fmadds	f22,	f25, f9, f30		/* stage 2, result  9 */
	fmadds	f23,	f24, f8, f31		/* stage 2, result  8 */
	fsubs	f24,	f0, f7			/* stage 2, result  4 */
	fsubs	f25,	f1, f6			/* stage 2, result  5 */
	fsubs	f26,	f2, f5			/* stage 2, result  7 */
	fsubs	f27,	f3, f4			/* stage 2, result  6 */
	fadds	f28,	f0, f7			/* stage 2, result  0 */
	fadds	f29,	f1, f6			/* stage 2, result  1 */
	fadds	f30,	f2, f5			/* stage 2, result  3 */
	fadds	f31,	f3, f4			/* stage 2, result  2 */
/* load in matrix multipliers */
	lfs	f3,	COS7_16(matrixCoef)	/* 1/(2*cos(7*pi/16)) */
	lfs	f2,	COS5_16(matrixCoef)	/* 1/(2*cos(5*pi/16)) */
	lfs	f1,	COS3_16(matrixCoef)	/* 1/(2*cos(3*pi/16)) */
	lfs	f0,	COS1_16(matrixCoef)	/* 1/(2*cos(1*pi/16)) */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 2 mult bottom quarters */
	fmuls	f27,	f3, f27			/* stage 2m, result  6 */
	fmuls	f26,	f2, f26			/* stage 2m, result  7 */
	fmuls	f16,	f3, f16			/* stage 2m, result 14 */
	fmuls	f17,	f2, f17			/* stage 2m, result 15 */
/* stage 2 mult top quarters, stage 3 plus/minus */
	fmadds	f4,	f24, f0, f27		/* stage 3, result  4 */
	fmadds	f5,	f25, f1, f26		/* stage 3, result  5 */
	fmsubs	f6,	f24, f0, f27		/* stage 3, result  6 */
	fmsubs	f7,	f25, f1, f26		/* stage 3, result  7 */
	fmadds	f12,	f19, f0, f16		/* stage 3, result 12 */
	fmadds	f13,	f18, f1, f17		/* stage 3, result 13 */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 2 mult top quarters, stage 3 plus/minus */
	fmsubs	f14,	f19, f0, f16		/* stage 3, result 14 */
	fmsubs	f15,	f18, f1, f17		/* stage 3, result 15 */
	fadds	f0,	f28, f31		/* stage 3, result  0 */
	fadds	f1,	f29, f30		/* stage 3, result  1 */
	fsubs	f2,	f28, f31		/* stage 3, result  2 */
	fsubs	f3,	f29, f30		/* stage 3, result  3 */
	fadds	f8,	f23, f20		/* stage 3, result  8 */
	fadds	f9,	f22, f21		/* stage 3, result  9 */
	fsubs	f10,	f23, f20		/* stage 3, result 10 */
	fsubs	f11,	f22, f21		/* stage 3, result 11 */
/* load in matrix multipliers */
	lfs	f28,	COS3_8(matrixCoef)	/* 1/(2*cos(3*pi/8)) */
	lfs	f29,	COS1_8(matrixCoef)	/* 1/(2*cos(1*pi/8)) */
	lfs	f30,	COS1_4(matrixCoef)	/* 1/(2*cos(1*pi/4)) */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 3 mult bottom eights */
	fmuls	f3,	f28, f3			/* stage 3m, result  3 */
	fmuls	f7,	f28, f7			/* stage 3m, result  7 */
	fmuls	f11,	f28, f11		/* stage 3m, result 11 */
	fmuls	f15,	f28, f15		/* stage 3m, result 15 */
/* stage 3 mult top eights, stage 4 plus/minus */
	fsubs	f16,	f0, f1			/* stage 4, result  1 */
	fmsubs	f17,	f2, f29, f3		/* stage 4, result  3 */
	fsubs	f18,	f4, f5			/* stage 4, result  5 */
	fmsubs	f19,	f6, f29, f7		/* stage 4, result  7 */
	fsubs	f20,	f8, f9			/* stage 4, result  9 */
	fmsubs	f21,	f10, f29, f11		/* stage 4, result 11 */
	fsubs	f22,	f12, f13		/* stage 4, result 13 */
	fmsubs	f23,	f14, f29, f15		/* stage 4, result 15 */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 3 mult top eights, stage 4 plus/minus */
	fadds	f0,	f0, f1			/* stage 4, result  0 */
	fmadds	f2,	f2, f29, f3		/* stage 4, result  2 */
	fadds	f4,	f4, f5			/* stage 4, result  4 */
	fmadds	f6,	f6, f29, f7		/* stage 4, result  6 */
	fadds	f8,	f8, f9			/* stage 4, result  8 */
	fmadds	f10,	f10, f29, f11		/* stage 4, result 10 */
	fadds	f12,	f12, f13		/* stage 4, result 12 */
	fmadds	f14,	f14, f29, f15		/* stage 4, result 14 */
/* stage 4 mults */
	fmuls	f1,	f16, f30		/* stage 4m, result  1 */
	fmuls	f3,	f17, f30		/* stage 4m, result  3 */
	fmuls	f5,	f18, f30		/* stage 4m, result  5 */
	fmuls	f7,	f19, f30		/* stage 4m, result  7 */
	fmuls	f9,	f20, f30		/* stage 4m, result  9 */
	fmuls	f11,	f21, f30		/* stage 4m, result 11 */
	fmuls	f13,	f22, f30		/* stage 4m, result 13 */
	fmuls	f15,	f23, f30		/* stage 4m, result 15 */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	addi	touchOutOffset,	touchOutOffset, 144*AUDIO_FLOAT_SIZE	/* offset for next output sample cache-line */
/* stage 5 adds */
	fadds	f2,	f2, f3			/* stage 5, result  2 */
	fadds	f6,	f6, f7			/* stage 5, result  6 */
	fadds	f10,	f10, f11		/* stage 5, result 10 */
	fadds	f14,	f14, f15		/* stage 5, result 14 */
/* stage 6 adds */
	fadds	f4,	f4, f6			/* stage 6, result  4 */
	fadds	f6,	f6, f5			/* stage 6, result  6 */
	fadds	f5,	f5, f7			/* stage 6, result  5 */
	fadds	f12,	f12, f14		/* stage 6, result 12 */
	fadds	f14,	f14, f13		/* stage 6, result 14 */
	fadds	f13,	f13, f15		/* stage 6, result 13 */
/* stage 7 adds */
	fadds	f8,	f8, f12			/* stage 7, result  8 */
	fadds	f12,	f12, f10		/* stage 7, result 12 */
	fadds	f10,	f10, f14		/* stage 7, result 10 */
	fadds	f14,	f14, f9			/* stage 7, result 14 */
	fadds	f9,	f9, f13			/* stage 7, result  9 */
	fadds	f13,	f13, f11		/* stage 7, result 13 */
	fadds	f11,	f11, f15		/* stage 7, result 11 */
/* touchload for outputs */
#ifndef NO_TOUCHLOADS
	dcbt	touchOutOffset,	outputSamp	/* do touch load */
#endif
	blr

Stage8_odd:	
/** final sums **/
	fadds	f0,	f0, f8			/* stage 8, result  0 */
	fadds	f8,	f8, f4			/* stage 8, result  8 */
	fadds	f4,	f4, f12			/* stage 8, result  4 */
	fadds	f12,	f12, f2			/* stage 8, result 12 */
	fadds	f2,	f2, f10			/* stage 8, result  2 */
	fadds	f10,	f10, f6			/* stage 8, result 10 */
	fadds	f6,	f6, f14			/* stage 8, result  6 */
	fadds	f14,	f14, f1			/* stage 8, result 14 */
	fadds	f1,	f1, f9			/* stage 8, result  1 */
	fadds	f9,	f9, f5			/* stage 8, result  9 */
	fadds	f5,	f5, f13			/* stage 8, result  5 */
	fadds	f13,	f13, f3			/* stage 8, result 13 */
	fadds	f3,	f3, f11			/* stage 8, result  3 */
	fadds	f11,	f11, f7			/* stage 8, result 11 */
	fadds	f7,	f7, f15			/* stage 8, result  7 */

/** scale by bit precision **/
	lfs	f16,	0(scale)		/* load scale factor */
	fmuls	f0, 	f0, f16			/* multiply by scale factor */
	fmuls	f1, 	f1, f16			/* multiply by scale factor */
	fmuls	f2, 	f2, f16			/* multiply by scale factor */
	fmuls	f3, 	f3, f16			/* multiply by scale factor */
	fmuls	f4, 	f4, f16			/* multiply by scale factor */
	fmuls	f5, 	f5, f16			/* multiply by scale factor */
	fmuls	f6, 	f6, f16			/* multiply by scale factor */
	fmuls	f7, 	f7, f16			/* multiply by scale factor */
	fmuls	f8, 	f8, f16			/* multiply by scale factor */
	fmuls	f9, 	f9, f16			/* multiply by scale factor */
	fmuls	f10, 	f10, f16		/* multiply by scale factor */
	fmuls	f11, 	f11, f16		/* multiply by scale factor */
	fmuls	f12, 	f12, f16		/* multiply by scale factor */
	fmuls	f13, 	f13, f16		/* multiply by scale factor */
	fmuls	f14, 	f14, f16		/* multiply by scale factor */
	fmuls	f15, 	f15, f16		/* multiply by scale factor */

/** store odd sample outputs **/
	stfs	f1,	72*AUDIO_FLOAT_SIZE(outputSamp)	  /*  sample  1 */
	stfs	f14,	73*AUDIO_FLOAT_SIZE(outputSamp)	  /* -sample 33 */
	stfs	f9,	216*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  3 */
	stfs	f6,	217*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 35 */
	stfs	f5,	360*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  5 */
	stfs	f10,	361*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 37 */
	stfs	f13,	504*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  7 */
	stfs	f2,	505*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 39 */
	stfs	f3,	648*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  9 */
	stfs	f12,	649*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 41 */
	stfs	f11,	792*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample 11 */
	stfs	f4,	793*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 43 */
	stfs	f7,	936*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample 13 */
	stfs	f8,	937*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 45 */
	stfs	f15,	1080*AUDIO_FLOAT_SIZE(outputSamp) /*  sample 15 */
	stfs	f0,	1081*AUDIO_FLOAT_SIZE(outputSamp) /* -sample 47 */

	blr

Stage8_even:	
/** scale by bit precision **/
	lfs	f16,	0(scale)		/* load scale factor */
	fmuls	f0, 	f0, f16			/* multiply by scale factor */
	fmuls	f1, 	f1, f16			/* multiply by scale factor */
	fmuls	f2, 	f2, f16			/* multiply by scale factor */
	fmuls	f3, 	f3, f16			/* multiply by scale factor */
	fmuls	f4, 	f4, f16			/* multiply by scale factor */
	fmuls	f5, 	f5, f16			/* multiply by scale factor */
	fmuls	f6, 	f6, f16			/* multiply by scale factor */
	fmuls	f7, 	f7, f16			/* multiply by scale factor */
	fmuls	f8, 	f8, f16			/* multiply by scale factor */
	fmuls	f9, 	f9, f16			/* multiply by scale factor */
	fmuls	f10, 	f10, f16		/* multiply by scale factor */
	fmuls	f11, 	f11, f16		/* multiply by scale factor */
	fmuls	f12, 	f12, f16		/* multiply by scale factor */
	fmuls	f13, 	f13, f16		/* multiply by scale factor */
	fmuls	f14, 	f14, f16		/* multiply by scale factor */
	fmuls	f15, 	f15, f16		/* multiply by scale factor */

/** storing even sample outputs **/
	stfs	f1,	0*AUDIO_FLOAT_SIZE(outputSamp)	  /*  sample  0 */
	stfs	f0,	1*AUDIO_FLOAT_SIZE(outputSamp)	  /* -sample 48 */
	stfs	f9,	144*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  2 */
	stfs	f14,	145*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 34 */
	stfs	f5,	288*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  4 */
	stfs	f6,	289*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 36 */
	stfs	f13,	432*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  6 */
	stfs	f10,	433*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 38 */
	stfs	f3,	576*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample  8 */
	stfs	f2,	577*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 40 */
	stfs	f11,	720*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample 10 */
	stfs	f12,	721*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 42 */
	stfs	f7,	864*AUDIO_FLOAT_SIZE(outputSamp)  /*  sample 12 */
	stfs	f4,	865*AUDIO_FLOAT_SIZE(outputSamp)  /* -sample 44 */
	stfs	f15,	1008*AUDIO_FLOAT_SIZE(outputSamp) /*  sample 14 */
	stfs	f8,	1009*AUDIO_FLOAT_SIZE(outputSamp) /* -sample 46 */

	blr

EndMatrix:	
/*	restore registers */
	mtlr	SaveLR
/* return */
	blr	




