/**********************************************************************
 *
 *	window.c
 *
 *	MPEG audio windowing function - uses redundancy in datasets 
 *      to reduce number of operations.  In addition to current frame's
 *      matrix output samples and window coefficients, this function 
 *      needs the previous frame's partial sums array, and an output
 *      buffer space of 1152 int32's.  After execution, the output
 *      buffer will contain packed, clipped, 16-bit output samples 
 *	packed into stereo format (2 audio values per 32-bit word)
 *	ready to forward to the DSP.  The partial sums array
 *	will contain data used to compute the next frame's output data.
 *
 *      This procedure calls four subroutines which roughly correlate
 *      to:
 *           A) process group 0 samples (pixels 0, 32, ..., 1120)
 *           B) process group 16 samples (pixels 16, 48, ..., 1136)
 *           C) process remaining group pairs (pixels i, 32-i, i+32,
 *				64-i, i+64, ...., 1120+i, 1152-i)
 *           D) convert non-ordered array of int32 audio samples into 
 *              aligned array of clipped int16 audio samples ready 
 *		for the dsp.
 *
 *      The coversion of float samples to int32 samples is performed in
 *      steps A, B, and C.
 *
 *********************************************************************/

#include "mpaWindow.i"

/**********************************************************************
 *
 *	mpaWinGrp0 entry point
 *
 *      This assembly routine does the windowing for group 0 samples
 *	and places the data into an intermediate output format
 *	Produces 36 samples, keeps 16 partial sums.
 *	Uses f24-31 for window coefficients, f23 for matrix 
 *	output samples.
 *
 *      NOTE: Clipping is performed in mpgWinConM or mpgWinConS
 *	(depending on whether it's mono or stereo).
 *
 *********************************************************************/

	DECFN	mpaWinGrp0
	
/* preserve registers   */
	mflr	SaveLR

/*** load in window coefficients ***/
	lfs	f24,	0(windowCoef)			/* load window coefficent  32 */
	lfs	f25,	AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent  64 */
	lfs	f26,	2*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent  96 */
	lfs	f27,	3*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 128 */
	lfs	f28,	4*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 160 */
	lfs	f29,	5*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 192 */
	lfs	f30,	6*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 224 */
	lfs	f31,	7*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 256 */

/*** load in previous partial sums ***/
	lfs	f0,	0(partialSums)			 /* sum of  1 term  */
	lfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  2 terms */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  3 terms */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  4 terms */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  5 terms */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  6 terms */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  7 terms */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  8 terms */
	lfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  9 terms */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of 10 terms */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	lfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	lfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	lfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*** begin main loop ***/
/* prepare for loop */
	dcbt	r0,	matrixSamp			/* do a touch load for first cache line input samples */
	li	fLineStep,	8*AUDIO_FLOAT_SIZE
	li	sampleCount,	36			/* load count */
	addi	matrixSamp,	matrixSamp, -2*AUDIO_FLOAT_SIZE	/* just before next input sample address */

/* now loop until processed 22 samples */
Loop0:		
	lfsu	f23,	2*AUDIO_FLOAT_SIZE(matrixSamp)	/* load next sample and update pointer */
	dcbt	r0,	outputSamp			/* do a touch load for current output sample */
	fctiwz	f15,	f14				/* convert total sum to int32 */
	fnmsubs	f14,	f23, f24, f13			/* total sum */
	fmadds	f13,	f23, f25, f12			/* sum of 14 terms */
	fnmsubs	f12,	f23, f26, f11			/* sum of 13 terms */
	fmadds	f11,	f23, f27, f10			/* sum of 12 terms */
	fnmsubs	f10,	f23, f28, f9			/* sum of 11 terms */
	fmadds	f9,	f23, f29, f8			/* sum of 10 terms */
	fnmsubs	f8,	f23, f30, f7			/* sum of  9 terms */
	fmadds	f7,	f23, f31, f6			/* sum of  8 terms */
	fmadds	f6,	f23, f30, f5			/* sum of  7 terms */
	fmadds	f5,	f23, f29, f4			/* sum of  6 terms */
	stfiwx	f15,	r0, outputSamp			/* store total sum as int32 */
	dcbt	matrixSamp, fLineStep			/* do a touch load for next cache line input samples */
	fmadds	f4,	f23, f28, f3			/* sum of  5 terms */
	fmadds	f3,	f23, f27, f2			/* sum of  4 terms */
	fmadds	f2,	f23, f26, f1			/* sum of  3 terms */
	fmadds	f1,	f23, f25, f0			/* sum of  2 terms */
	fmuls	f0,	f23, f24			/* sum of  1 term  */
	addi	outputSamp, outputSamp, 32*AUDIO_INT_SIZE	/* next output sample offset */
	addic.	sampleCount, sampleCount, -1			/* decrement loop count */
	bgt	Loop0					/* repeat if loop count > 0 */
/*** end main loop ***/

/*** store away new partial sums ***/
	stfs	f0,	0(partialSums)			/* sum of  1 term  */
	stfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	/* sum of  2 terms */
	stfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  3 terms */
	stfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  4 terms */
	stfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  5 terms */
	stfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  6 terms */
	stfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  7 terms */
	stfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  8 terms */
	stfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  9 terms */
	stfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	/* sum of 10 terms */
	stfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	stfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	stfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	stfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	stfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*	restore registers */
	mtlr	SaveLR
/* return */
	blr	


/**********************************************************************
 *
 *	mpaWinGrp16 entry point
 *
 *      The assembly routine does the windowing for group 16 samples
 *	and places the data into an intermediate output format
 *	Produces 36 samples, keep 16 partial sums. 
 *	Uses f24-31 for window coefficients, f22-f23 for matrix 
 *	output samples.  No cache block touches are needed because 
 *	they have been loaded with the previous call to mpgWinGrp0.
 *
 *      NOTE: Clipping is performed in mpgWinConM or mpgWinConS
 *	(depending on whether it's mono or stereo).
 *
 *********************************************************************/
	DECFN	mpaWinGrp16
	
/* preserve registers   */
	mflr	SaveLR

/*** load in window coefficients ***/
	lfs	f24,	8*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent  16 */
	lfs	f25,	9*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent  48 */
	lfs	f26,	10*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent  80 */
	lfs	f27,	11*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 112 */
	lfs	f28,	12*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 144 */
	lfs	f29,	13*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 176 */
	lfs	f30,	14*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 208 */
	lfs	f31,	15*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 240 */

/*** load in previous partial sums ***/
	lfs	f0,	0(partialSums)			 /* sum of  1 term  */
	lfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  2 terms */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  3 terms */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  4 terms */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  5 terms */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  6 terms */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  7 terms */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  8 terms */
	lfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  9 terms */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of 10 terms */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	lfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	lfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	lfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*** begin main loop ***/
/* prepare for loop */
	li	sampleCount,	18			/* load count */
	addi	matrixSamp, matrixSamp, -AUDIO_FLOAT_SIZE  /* just before next input sample offset */

/* now loop until processed 36 samples, 2 per iteration */
Loop16:	
	lfsu	f22,	2*AUDIO_FLOAT_SIZE(matrixSamp)	/* load next sample and update pointer */
	lfsu	f23,	2*AUDIO_FLOAT_SIZE(matrixSamp)	/* load next sample and update pointer */
	fnmsubs	f15,	f22, f25, f13			/* total sum */
	fctiwz	f14,	f14				/* convert total sum to int32 */
	fctiwz	f15,	f15				/* convert total sum to int32 */
	stfiwx	f14,	r0, outputSamp			/* store total sum as int32 */
	addi	outputSamp, outputSamp, 32*AUDIO_INT_SIZE /* next output sample offset */
	stfiwx	f15,	r0, outputSamp			/* store total sum as int32 */
	addi	outputSamp, outputSamp, 32*AUDIO_INT_SIZE /* next output sample offset */
	fnmsubs	f14,	f23, f25, f12			/* sum of 15 terms */
	fnmsubs	f13,	f22, f27, f11			/* sum of 14 terms */
	fnmsubs	f12,	f23, f27, f10			/* sum of 13 terms */
	fnmsubs	f11,	f22, f29, f9			/* sum of 12 terms */
	fnmsubs	f10,	f23, f29, f8			/* sum of 11 terms */
	fnmsubs	f9,	f22, f31, f7			/* sum of 10 terms */
	fnmsubs	f8,	f23, f31, f6			/* sum of  9 terms */
	fmadds	f7,	f22, f30, f5			/* sum of  8 terms */
	fmadds	f6,	f23, f30, f4			/* sum of  7 terms */
	fmadds	f5,	f22, f28, f3			/* sum of  6 terms */
	fmadds	f4,	f23, f28, f2			/* sum of  5 terms */
	fmadds	f3,	f22, f26, f1			/* sum of  4 terms */
	fmadds	f2,	f23, f26, f0			/* sum of  3 terms */
	fmuls	f1,	f22, f24			/* sum of  2 terms */
	fmuls	f0,	f23, f24			/* sum of  1 term  */
	addic.	sampleCount,	sampleCount, -1		/* decrement loop count */
	bgt	Loop16					/* repeat if loop count > 0 */
/*** end main loop ***/

/*** store away new partial sums ***/
	stfs	f0,	0(partialSums)			/* sum of  1 term  */
	stfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	/* sum of  2 terms */
	stfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  3 terms */
	stfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  4 terms */
	stfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  5 terms */
	stfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  6 terms */
	stfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  7 terms */
	stfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  8 terms */
	stfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  9 terms */
	stfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	/* sum of 10 terms */
	stfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	stfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	stfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	stfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	stfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*	restore registers */
	mtlr	SaveLR
/* return */
	blr	


/**********************************************************************
 *
 *	mpaWinGrp entry point
 *
 *      The assembly routine does the windowing for group samples
 *	other than groups 0 and 16 and places the data into an 
 *	intermediate output format.  
 *	Produces 36 samples, keep 16 partial sums. 
 *	Uses f16-31 for window coefficients, some of those for matrix 
 *	output samples.
 *
 *      NOTE: Clipping is performed in mpgWinConM or mpgWinConS
 *	(depending on whether it's mono or stereo).
 *
 *********************************************************************/
	DECFN	mpaWinGrp
	
/* preserve registers   */
	mflr	SaveLR

/* Each input sample represents two matrix output samples		*
 *	M[j][i] = -M[j][32-i] for 0 < i < 16;				*
 *	M[j][i] =  M[j][96-i] for 32 < i < 48;				*
 *	Part 1 calculates using the lower valued symmetric components.	*
 *	Part 2 calculates using the higher valued symmetric components. */
/*** load in window coefficients, probably will need to reload at some point ***/
	lfs	f16,	0(windowCoef)			/* load window coefficent i	*/
	lfs	f17,	AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+32	*/
	lfs	f18,	2*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+64	*/
	lfs	f19,	3*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+96	*/
	lfs	f20,	4*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+128 */
	lfs	f21,	5*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+160 */
	lfs	f22,	6*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+192 */
	lfs	f23,	7*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+224 */
	lfs	f24,	8*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 32-i  */
	lfs	f25,	9*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 64-i  */
	lfs	f26,	10*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 96-i  */
	lfs	f27,	11*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 128-i */
	lfs	f28,	12*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 160-i */
	lfs	f29,	13*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 192-i */
	lfs	f30,	14*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 224-i */
	lfs	f31,	15*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 256-i */

/****** Part 1 ******/
/*** load in previous partial sums ***/
	lfs	f0,	0(partialSums)			 /* sum of  1 term  */
	lfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  2 terms */
	lfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  3 terms */
	lfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  4 terms */
	lfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  5 terms */
	lfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  6 terms */
	lfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  7 terms */
	lfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  8 terms */
	lfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of  9 terms */
	lfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	 /* sum of 10 terms */
	lfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	lfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	lfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	lfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	lfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*** begin loop for first half of symmetric values ***/
/* prepare for loop */
	dcbt	r0,	matrixSamp			/* do a touch load for first cache line input samples */
	li	fLineStep,	8*AUDIO_FLOAT_SIZE
	li	sampleCount,	36			/* load count */
	addi	inPtr,	matrixSamp, -AUDIO_FLOAT_SIZE	/* just before next input sample offset */
	mr	outPtr,	outputSamp			/* next output sample offset */
	
/* now loop until processed 36 samples */	 
Loopi1:	
	lfsu	f30,	AUDIO_FLOAT_SIZE(inPtr)		/* load next sample and update pointer, low half */
	lfsu	f31,	AUDIO_FLOAT_SIZE(inPtr)		/* load next sample and update pointer, upper half */
	fmadds	f15,	f30, f16, f14			/* total sum */
	fnmsubs	f14,	f31, f17, f13			/* sum of 15 terms */
	fmr	f16,	f30				/* move the data samples into unneeded window coefficients */
	fctiwz	f15,	f15				/* convert total sum to int32 */
	fmr	f17,	f31				/* move the data samples into unneeded window coefficients */
	lfs	f30,	14*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 224-i */
	stfiwx	f15,	r0, outPtr			/* store total sum as int32 */
	lfs	f31,	15*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 256-i */
	addi	outPtr,	outPtr, 32*AUDIO_FLOAT_SIZE 	/* next output sample offset */
	dcbt	r0,	outPtr				/* do a touch load for next output sample cache line */
	fmadds	f13,	f16, f18, f12			/* sum of 14 terms */
	fnmsubs	f12,	f17, f19, f11			/* sum of 13 terms */
	fmadds	f11,	f16, f20, f10			/* sum of 12 terms */
	fnmsubs	f10,	f17, f21, f9			/* sum of 11 terms */
	fmadds	f9,	f16, f22, f8			/* sum of 10 terms */
	fnmsubs	f8,	f17, f23, f7			/* sum of  9 terms */
	fnmsubs	f7,	f16, f31, f6			/* sum of  8 terms */
	fmadds	f6,	f17, f30, f5			/* sum of  7 terms */
	dcbt	inPtr,	fLineStep			/* do a touch load for next cache line input samples */
	fnmsubs	f5,	f16, f29, f4			/* sum of  6 terms */
	fmadds	f4,	f17, f28, f3			/* sum of  5 terms */
	fnmsubs	f3,	f16, f27, f2			/* sum of  4 terms */
	fmadds	f2,	f17, f26, f1			/* sum of  3 terms */
	fnmsubs	f1,	f16, f25, f0			/* sum of  2 terms */
	lfs	f16,	0(windowCoef)			/* load window coefficent i	*/
	fmuls	f0,	f17, f24			/* sum of  1 term  */
	lfs	f17,	AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+32	*/
	addic.	sampleCount,	sampleCount, -1		/* decrement loop count */
	bgt	Loopi1					/* repeat if loop count > 0 */
/*** end main loop ***/

/*** store away new partial sums ***/
	stfs	f0,	0(partialSums)			/* sum of  1 term  */
	stfs	f1,	AUDIO_FLOAT_SIZE(partialSums)	/* sum of  2 terms */
	stfs	f2,	2*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  3 terms */
	stfs	f3,	3*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  4 terms */
	stfs	f4,	4*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  5 terms */
	stfs	f5,	5*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  6 terms */
	stfs	f6,	6*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  7 terms */
	stfs	f7,	7*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  8 terms */
	stfs	f8,	8*AUDIO_FLOAT_SIZE(partialSums)	/* sum of  9 terms */
	stfs	f9,	9*AUDIO_FLOAT_SIZE(partialSums)	/* sum of 10 terms */
	stfs	f10,	10*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	stfs	f11,	11*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	stfs	f12,	12*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	stfs	f13,	13*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	stfs	f14,	14*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */
 
/****** Part 2 ******/ 
/*** load in previous partial sums ***/	
	lfs	f0,	16*AUDIO_FLOAT_SIZE(partialSums) /* sum of  1 term  */
	lfs	f1,	17*AUDIO_FLOAT_SIZE(partialSums) /* sum of  2 terms */
	lfs	f2,	18*AUDIO_FLOAT_SIZE(partialSums) /* sum of  3 terms */
	lfs	f3,	19*AUDIO_FLOAT_SIZE(partialSums) /* sum of  4 terms */
	lfs	f4,	20*AUDIO_FLOAT_SIZE(partialSums) /* sum of  5 terms */
	lfs	f5,	21*AUDIO_FLOAT_SIZE(partialSums) /* sum of  6 terms */
	lfs	f6,	22*AUDIO_FLOAT_SIZE(partialSums) /* sum of  7 terms */
	lfs	f7,	23*AUDIO_FLOAT_SIZE(partialSums) /* sum of  8 terms */
	lfs	f8,	24*AUDIO_FLOAT_SIZE(partialSums) /* sum of  9 terms */
	lfs	f9,	25*AUDIO_FLOAT_SIZE(partialSums) /* sum of 10 terms */
	lfs	f10,	26*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	lfs	f11,	27*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	lfs	f12,	28*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	lfs	f13,	29*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	lfs	f14,	30*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*** begin loop for second half symmetric values ***/
/* prepare for loop */
	li	sampleCount,	36			/* load count */
	addi	inPtr,	matrixSamp, -AUDIO_FLOAT_SIZE	/* just before next input sample offset */
	addi	outPtr,	outputSamp, AUDIO_INT_SIZE	/* next output sample offset */
	
/* now loop until processed 36 samples */	 
Loopi2:	
	lfsu	f22,	AUDIO_FLOAT_SIZE(inPtr)		/* load next sample and update pointer, low half */
	lfsu	f23,	AUDIO_FLOAT_SIZE(inPtr)		/* load next sample and update pointer, upper half */
	fnmsubs	f15,	f22, f24, f14			/* total sum */
	fnmsubs	f14,	f23, f25, f13			/* sum of 15 terms */
	fmr	f24,	f22				/* move the data samples into unneeded window coefficients */
	fctiwz	f15,	f15				/* convert total sum to int32 */
	fmr	f25,	f23				/* move the data samples into unneeded window coefficients */
	lfs	f22,	6*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+192 */
	stfiwx	f15,	r0, outPtr			/* store total sum as int32 */
	lfs	f23,	7*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent i+224 */
	addi	outPtr,	outPtr, 32*AUDIO_FLOAT_SIZE	/* next output sample offset */
	fnmsubs	f13,	f24, f26, f12			/* sum of 14 terms */
	fnmsubs	f12,	f25, f27, f11			/* sum of 13 terms */
	fnmsubs	f11,	f24, f28, f10			/* sum of 12 terms */
	fnmsubs	f10,	f25, f29, f9			/* sum of 11 terms */
	fnmsubs	f9,	f24, f30, f8			/* sum of 10 terms */
	fnmsubs	f8,	f25, f31, f7			/* sum of  9 terms */
	fmadds	f7,	f24, f23, f6			/* sum of  8 terms */
	fmadds	f6,	f25, f22, f5			/* sum of  7 terms */
	fmadds	f5,	f24, f21, f4			/* sum of  6 terms */
	fmadds	f4,	f25, f20, f3			/* sum of  5 terms */
	fmadds	f3,	f24, f19, f2			/* sum of  4 terms */
	fmadds	f2,	f25, f18, f1			/* sum of  3 terms */
	fmadds	f1,	f24, f17, f0			/* sum of  2 terms */
	lfs	f24,	8*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 32-i	*/
	fmuls	f0,	f25, f16			/* sum of  1 term  */
	lfs	f25,	9*AUDIO_FLOAT_SIZE(windowCoef)	/* load window coefficent 64-i	*/
	addic.	sampleCount,	sampleCount, -1		/* decrement loop count */
	bgt	Loopi2					/* repeat if loop count > 0 */
/*** end main loop ***/

/*** store away new partial sums ***/
	stfs	f0,	16*AUDIO_FLOAT_SIZE(partialSums) /* sum of  1 term  */
	stfs	f1,	17*AUDIO_FLOAT_SIZE(partialSums) /* sum of  2 terms */
	stfs	f2,	18*AUDIO_FLOAT_SIZE(partialSums) /* sum of  3 terms */
	stfs	f3,	19*AUDIO_FLOAT_SIZE(partialSums) /* sum of  4 terms */
	stfs	f4,	20*AUDIO_FLOAT_SIZE(partialSums) /* sum of  5 terms */
	stfs	f5,	21*AUDIO_FLOAT_SIZE(partialSums) /* sum of  6 terms */
	stfs	f6,	22*AUDIO_FLOAT_SIZE(partialSums) /* sum of  7 terms */
	stfs	f7,	23*AUDIO_FLOAT_SIZE(partialSums) /* sum of  8 terms */
	stfs	f8,	24*AUDIO_FLOAT_SIZE(partialSums) /* sum of  9 terms */
	stfs	f9,	25*AUDIO_FLOAT_SIZE(partialSums) /* sum of 10 terms */
	stfs	f10,	26*AUDIO_FLOAT_SIZE(partialSums) /* sum of 11 terms */
	stfs	f11,	27*AUDIO_FLOAT_SIZE(partialSums) /* sum of 12 terms */
	stfs	f12,	28*AUDIO_FLOAT_SIZE(partialSums) /* sum of 13 terms */
	stfs	f13,	29*AUDIO_FLOAT_SIZE(partialSums) /* sum of 14 terms */
	stfs	f14,	30*AUDIO_FLOAT_SIZE(partialSums) /* sum of 15 terms */

/*	restore registers */
	mtlr	SaveLR
/* return */
	blr	


/**********************************************************************
 *
 *	mpaWinConM entry point
 *
 *      The assembly routine converts a non-ordered array of int32 
 *	audio samples into an aligned array of int16 audio samples 
 *	ready for the dsp.  The samples are clipped to signed 16-bit
 *	values.
 *
 *********************************************************************/
	DECFN	mpaWinConM
	
/* preserve registers   */
	mflr	SaveLR
	stwu	r1, 	-60(r1)		/* create stack group */
	stw	LdVal6,	16(r1)		/* save off gp registers */
	stw	LdVal7, 20(r1)	
	stw	StrVal16, 24(r1)	
	stw	StrVal24, 28(r1)	
	stw	StrVal25, 32(r1)	
	stw	StrVal26, 36(r1)	
	stw	StrVal27, 40(r1)	
	stw	StrVal28, 44(r1)	
	stw	StrVal29, 48(r1)	
	stw	StrVal30, 52(r1)	
	stw	StrVal31, 56(r1)	
/* initialize pointers, counters, etc. */
	li	sampleCount,	36		/* doing 36 groups */
	mr	outputSamp,	outputSamp	/* first sample address */
DoGroupM:	
/* load and clip samples 0,16,1,31,2,30,3,29 */
/* branch and load link register */
	bl	LdNClpM
/* move or store results */
	mr	StrVal16, LdVal1			/* move 16 */
	mr	StrVal31, LdVal3			/* move 31 */
	mr	StrVal30, LdVal5			/* move 30 */
	mr	StrVal29, LdVal7			/* move 29 */
	stw	LdVal0,	-8*AUDIO_INT_SIZE(outputSamp)	/* store 0 */
	stw	LdVal2,	-7*AUDIO_INT_SIZE(outputSamp)	/* store 1 */
	stw	LdVal4,	-6*AUDIO_INT_SIZE(outputSamp)	/* store 2 */
	stw	LdVal6,	-5*AUDIO_INT_SIZE(outputSamp)	/* store 3 */
/* load and clip samples 4,28,5,27,6,26,7,25 */
/* branch and load link register */
	bl	LdNClpM
/* move or store results */
	mr	StrVal28, LdVal1			/* move 28 */
	mr	StrVal27, LdVal3			/* move 27 */
	mr	StrVal26, LdVal5			/* move 26 */
	mr	StrVal25, LdVal7			/* move 25 */
	stw	LdVal0,	-12*AUDIO_INT_SIZE(outputSamp)	/* store 4 */
	stw	LdVal2,	-11*AUDIO_INT_SIZE(outputSamp)	/* store 5 */
	stw	LdVal4,	-10*AUDIO_INT_SIZE(outputSamp)	/* store 6 */
	stw	LdVal6,	-9*AUDIO_INT_SIZE(outputSamp)	/* store 7 */
/* load and clip samples 8,24,9,23,10,22,11,21 */
/* branch and load link register */
	bl	LdNClpM
/* move or store results */
	mr	StrVal24, LdVal1			/* move 24 */
	stw	LdVal0,	-16*AUDIO_INT_SIZE(outputSamp)	/* store 8 */
	stw	LdVal2,	-15*AUDIO_INT_SIZE(outputSamp)	/* store 9 */
	stw	LdVal4,	-14*AUDIO_INT_SIZE(outputSamp)	/* store 10 */
	stw	LdVal6,	-13*AUDIO_INT_SIZE(outputSamp)	/* store 11 */
	stw	StrVal16, -8*AUDIO_INT_SIZE(outputSamp)	/* store 16 */
	stw	LdVal7,	-3*AUDIO_INT_SIZE(outputSamp)	/* store 21 */
	stw	LdVal5,	-2*AUDIO_INT_SIZE(outputSamp)	/* store 22 */
	stw	LdVal3,	-1*AUDIO_INT_SIZE(outputSamp)	/* store 23 */
/* load and clip samples 12,20,13,19,14,18,15,17 */
/* branch and load link register */
	bl	LdNClpM
/* move or store results */
	stw	LdVal0,	-20*AUDIO_INT_SIZE(outputSamp)	/* store 12 */
	stw	LdVal2,	-19*AUDIO_INT_SIZE(outputSamp)	/* store 13 */
	stw	LdVal4,	-18*AUDIO_INT_SIZE(outputSamp)	/* store 14 */
	stw	LdVal6,	-17*AUDIO_INT_SIZE(outputSamp)	/* store 15 */
	stw	LdVal7,	-15*AUDIO_INT_SIZE(outputSamp)	/* store 17 */
	stw	LdVal5,	-14*AUDIO_INT_SIZE(outputSamp)	/* store 18 */
	stw	LdVal3,	-13*AUDIO_INT_SIZE(outputSamp)	/* store 19 */
	stw	LdVal1,	-12*AUDIO_INT_SIZE(outputSamp)	/* store 20 */
	stw	StrVal24, -8*AUDIO_INT_SIZE(outputSamp)	/* store 24 */
	stw	StrVal25, -7*AUDIO_INT_SIZE(outputSamp)	/* store 25 */
	stw	StrVal26, -6*AUDIO_INT_SIZE(outputSamp)	/* store 26 */
	stw	StrVal27, -5*AUDIO_INT_SIZE(outputSamp)	/* store 27 */
	stw	StrVal28, -4*AUDIO_INT_SIZE(outputSamp)	/* store 28 */
	stw	StrVal29, -3*AUDIO_INT_SIZE(outputSamp)	/* store 29 */
	stw	StrVal30, -2*AUDIO_INT_SIZE(outputSamp)	/* store 30 */
	stw	StrVal31, -1*AUDIO_INT_SIZE(outputSamp)	/* store 31 */
	addic.	sampleCount, sampleCount, -1		/* decrement group count */
	bgt	DoGroupM
	b	EndConvM

/* subroutine to load a cache-line into memory, 
   touch load the next input cache line, and clip the samples
   outputSamp is input sample offset */
LdNClpM:		
/* loading first cache-line into LdVal0-LdVal7 */
	lwz	LdVal0,	0(outputSamp)
	lwzu	LdVal1,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal2,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal3,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal4,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal5,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal6,	AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal7,	AUDIO_INT_SIZE(outputSamp)
/* do a touch load for next interations */
	addi	outputSamp,	outputSamp, AUDIO_INT_SIZE	/* next touch load */
	dcbt	r0,	outputSamp
/* clip the input samples */
	cmpwi	LdVal0,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal0,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal0,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal0,	-32768	/* else clip to low mark  */
	cmpwi	LdVal1,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal1,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal1,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal1,	-32768	/* else clip to low mark  */
	cmpwi	LdVal2,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal2,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal2,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal2,	-32768	/* else clip to low mark  */
	cmpwi	LdVal3,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal3,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal3,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal3,	-32768	/* else clip to low mark  */
	cmpwi	LdVal4,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal4,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal4,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal4,	-32768	/* else clip to low mark  */
	cmpwi	LdVal5,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal5,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal5,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal5,	-32768	/* else clip to low mark  */
	cmpwi	LdVal6,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal6,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal6,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal6,	-32768	/* else clip to low mark  */
	cmpwi	LdVal7,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal7,	 32767	/* else clip to high mark */
	b	.+16		/* done, branch clear     */
	cmpwi	LdVal7,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal7,	-32768	/* else clip to low mark  */
	/* rotate and insert halfword into pairs */
	rlwimi	LdVal0, LdVal0, 16, 0, 15
	rlwimi	LdVal1,	LdVal1,	16, 0, 15
	rlwimi	LdVal2,	LdVal2,	16, 0, 15
	rlwimi	LdVal3, LdVal3, 16, 0, 15
	rlwimi	LdVal4,	LdVal4,	16, 0, 15
	rlwimi	LdVal5,	LdVal5,	16, 0, 15
	rlwimi	LdVal6, LdVal6, 16, 0, 15
	rlwimi	LdVal7,	LdVal7,	16, 0, 15
	blr

EndConvM:	
/*	restore registers */
	mtlr	SaveLR
/* done, now restore register contents */
	lwz	LdVal6,	16(r1)		/* pop off gp registers */
	lwz	LdVal7, 20(r1)	
	lwz	StrVal16, 24(r1)	
	lwz	StrVal24, 28(r1)	
	lwz	StrVal25, 32(r1)	
	lwz	StrVal26, 36(r1)	
	lwz	StrVal27, 40(r1)	
	lwz	StrVal28, 44(r1)	
	lwz	StrVal29, 48(r1)	
	lwz	StrVal30, 52(r1)	
	lwz	StrVal31, 56(r1)	
	addi	r1, 	r1, 60		/* update stack pointer */
/* return */
	blr	


/**********************************************************************
 *
 *	mpaWinConS entry point
 *
 *      The assembly routine converts a non-ordered array of int32 
 *	audio samples into an aligned array of int16 audio samples 
 *	ready for the dsp.  The samples are clipped to signed 16-bit
 *	values.
 *
 *********************************************************************/
	DECFN	mpaWinConS
	
/* preserve registers   */
	mflr	SaveLR
	stwu	r1, 	-92(r1)	/* create stack group */
	stw	LdVal6,	16(r1)	/* save off gp registers */
	stw	LdVal7, 20(r1)	
	stw	StrVal16, 24(r1)	
	stw	StrVal24, 28(r1)	
	stw	StrVal25, 32(r1)	
	stw	StrVal26, 36(r1)	
	stw	StrVal27, 40(r1)	
	stw	StrVal28, 44(r1)	
	stw	StrVal29, 48(r1)	
	stw	StrVal30, 52(r1)	
	stw	StrVal31, 56(r1)	
	stw	LdVal0B, 60(r1)	
	stw	LdVal1B, 64(r1)	
	stw	LdVal2B, 68(r1)	
	stw	LdVal3B, 72(r1)	
	stw	LdVal4B, 76(r1)	
	stw	LdVal5B, 80(r1)	
	stw	LdVal6B, 84(r1)	
	stw	LdVal7B, 88(r1)	
/* initialize pointers, counters, etc. */
	li	sampleCount,	36		/* doing 36 groups */
DoGroupS:	
/* load and clip samples 0,16,1,31,2,30,3,29 */
/* branch and load link register */
	bl	LdNClpS
/* move or store results */
	mr	StrVal16, LdVal1			/* move 16 */
	mr	StrVal31, LdVal3			/* move 31 */
	mr	StrVal30, LdVal5			/* move 30 */
	mr	StrVal29, LdVal7			/* move 29 */
	stw	LdVal0,	-8*AUDIO_INT_SIZE(outputSamp)	/* store 0 */
	stw	LdVal2,	-7*AUDIO_INT_SIZE(outputSamp)	/* store 1 */
	stw	LdVal4,	-6*AUDIO_INT_SIZE(outputSamp)	/* store 2 */
	stw	LdVal6,	-5*AUDIO_INT_SIZE(outputSamp)	/* store 3 */
/* load and clip samples 4,28,5,27,6,26,7,25 */
/* branch and load link register */
	bl	LdNClpS
/* move or store results */
	mr	StrVal28, LdVal1			/* move 28 */
	mr	StrVal27, LdVal3			/* move 27 */
	mr	StrVal26, LdVal5			/* move 26 */
	mr	StrVal25, LdVal7			/* move 25 */
	stw	LdVal0,	-12*AUDIO_INT_SIZE(outputSamp)	/* store 4 */
	stw	LdVal2,	-11*AUDIO_INT_SIZE(outputSamp)	/* store 5 */
	stw	LdVal4,	-10*AUDIO_INT_SIZE(outputSamp)	/* store 6 */
	stw	LdVal6,	-9*AUDIO_INT_SIZE(outputSamp)	/* store 7 */
/* load and clip samples 8,24,9,23,10,22,11,21 */
/* branch and load link register */
	bl	LdNClpS
/* move or store results */
	mr	StrVal24, LdVal1			/* move 24 */
	stw	LdVal0,	-16*AUDIO_INT_SIZE(outputSamp)	/* store 8 */
	stw	LdVal2,	-15*AUDIO_INT_SIZE(outputSamp)	/* store 9 */
	stw	LdVal4,	-14*AUDIO_INT_SIZE(outputSamp)	/* store 10 */
	stw	LdVal6,	-13*AUDIO_INT_SIZE(outputSamp)	/* store 11 */
	stw	StrVal16, -8*AUDIO_INT_SIZE(outputSamp)	/* store 16 */
	stw	LdVal7,	-3*AUDIO_INT_SIZE(outputSamp)	/* store 21 */
	stw	LdVal5,	-2*AUDIO_INT_SIZE(outputSamp)	/* store 22 */
	stw	LdVal3,	-1*AUDIO_INT_SIZE(outputSamp)	/* store 23 */
/* load and clip samples 12,20,13,19,14,18,15,17 */
/* branch and load link register */
	bl	LdNClpS
/* move or store results */
	stw	LdVal0,	-20*AUDIO_INT_SIZE(outputSamp)	/* store 12 */
	stw	LdVal2,	-19*AUDIO_INT_SIZE(outputSamp)	/* store 13 */
	stw	LdVal4,	-18*AUDIO_INT_SIZE(outputSamp)	/* store 14 */
	stw	LdVal6,	-17*AUDIO_INT_SIZE(outputSamp)	/* store 15 */
	stw	LdVal7,	-15*AUDIO_INT_SIZE(outputSamp)	/* store 17 */
	stw	LdVal5,	-14*AUDIO_INT_SIZE(outputSamp)	/* store 18 */
	stw	LdVal3,	-13*AUDIO_INT_SIZE(outputSamp)	/* store 19 */
	stw	LdVal1,	-12*AUDIO_INT_SIZE(outputSamp)	/* store 20 */
	stw	StrVal24, -8*AUDIO_INT_SIZE(outputSamp)	/* store 24 */
	stw	StrVal25, -7*AUDIO_INT_SIZE(outputSamp)	/* store 25 */
	stw	StrVal26, -6*AUDIO_INT_SIZE(outputSamp)	/* store 26 */
	stw	StrVal27, -5*AUDIO_INT_SIZE(outputSamp)	/* store 27 */
	stw	StrVal28, -4*AUDIO_INT_SIZE(outputSamp)	/* store 28 */
	stw	StrVal29, -3*AUDIO_INT_SIZE(outputSamp)	/* store 29 */
	stw	StrVal30, -2*AUDIO_INT_SIZE(outputSamp)	/* store 30 */
	stw	StrVal31, -1*AUDIO_INT_SIZE(outputSamp)	/* store 31 */
	addic.	sampleCount, sampleCount, -1		/* decrement group count */
	bgt	DoGroupS
	b	EndConvS

/* subroutine to load left and right sample cache-lines into memory, 
   touch load the next input cache lines, clip the samples, and
   combine into left-right sample pairs.  Output results are left in
   LdVal0-LdVal7. outputSamp is left input sample offset, windowTempArray is right input sample 
   offset */
LdNClpS:		
/* load cache-line of left samples into LdVal0B-LdVal7B */
	lwz	LdVal0B, 0(outputSamp)
	lwzu	LdVal1B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal2B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal3B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal4B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal5B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal6B, AUDIO_INT_SIZE(outputSamp)
	lwzu	LdVal7B, AUDIO_INT_SIZE(outputSamp)
/* do a touch load for next interations */
	addi	outputSamp, outputSamp, AUDIO_INT_SIZE	/* next touch load */
	dcbt	r0,	outputSamp
/* clip the input samples */
	cmpwi	LdVal0B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal0B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal0B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal0B, -32768	/* else clip to low mark  */
	cmpwi	LdVal1B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal1B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal1B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal1B, -32768	/* else clip to low mark  */
	cmpwi	LdVal2B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal2B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal2B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal2B, -32768	/* else clip to low mark  */
	cmpwi	LdVal3B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal3B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal3B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal3B, -32768	/* else clip to low mark  */
	cmpwi	LdVal4B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal4B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal4B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal4B, -32768	/* else clip to low mark  */
	cmpwi	LdVal5B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal5B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal5B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal5B, -32768	/* else clip to low mark  */
	cmpwi	LdVal6B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal6B, 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal6B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal6B, -32768	/* else clip to low mark  */
	cmpwi	LdVal7B, 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal7B, 32767	/* else clip to high mark */
	b	.+16		/* done, branch clear     */
	cmpwi	LdVal7B, -32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal7B, -32768	/* else clip to low mark  */
/* load cache-line of right samples into LdVal0-LdVal7 */
	lwz	LdVal0,	0(windowTempArray)
	lwzu	LdVal1,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal2,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal3,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal4,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal5,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal6,	AUDIO_INT_SIZE(windowTempArray)
	lwzu	LdVal7,	AUDIO_INT_SIZE(windowTempArray)
/* do a touch load for next interations */
	addi	windowTempArray, windowTempArray, AUDIO_INT_SIZE /* next touch load */
	dcbt	r0,	windowTempArray
/* clip the input samples */
	cmpwi	LdVal0,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal0,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal0,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal0,	-32768	/* else clip to low mark  */
	cmpwi	LdVal1,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal1,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal1,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal1,	-32768	/* else clip to low mark  */
	cmpwi	LdVal2,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal2,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal2,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal2,	-32768	/* else clip to low mark  */
	cmpwi	LdVal3,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal3,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal3,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal3,	-32768	/* else clip to low mark  */
	cmpwi	LdVal4,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal4,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal4,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal4,	-32768	/* else clip to low mark  */
	cmpwi	LdVal5,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal5,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal5,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal5,	-32768	/* else clip to low mark  */
	cmpwi	LdVal6,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal6,	 32767	/* else clip to high mark */
	b	.+16		/* jump to next compare   */
	cmpwi	LdVal6,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal6,	-32768	/* else clip to low mark  */
	cmpwi	LdVal7,	 32767	/* compare to high mark   */
	ble	.+12		/* branch if <= high mark */
	li	LdVal7,	 32767	/* else clip to high mark */
	b	.+16		/* done, branch clear     */
	cmpwi	LdVal7,	-32768	/* compare to low mark    */
	bge	.+8		/* branch if >= high mark */
	li	LdVal7,	-32768	/* else clip to low mark  */
	/* rotate and insert halfword into pairs */
	rlwimi	LdVal0, LdVal0B, 16, 0, 15
	rlwimi	LdVal1,	LdVal1B, 16, 0, 15
	rlwimi	LdVal2,	LdVal2B, 16, 0, 15
	rlwimi	LdVal3, LdVal3B, 16, 0, 15
	rlwimi	LdVal4,	LdVal4B, 16, 0, 15
	rlwimi	LdVal5,	LdVal5B, 16, 0, 15
	rlwimi	LdVal6, LdVal6B, 16, 0, 15
	rlwimi	LdVal7,	LdVal7B, 16, 0, 15
	blr
EndConvS:	
/*	restore registers */
	mtlr	SaveLR
/* done, now restore register contents */
	lwz	LdVal6,	16(r1)		/* pop off gp registers */
	lwz	LdVal7, 20(r1)	
	lwz	StrVal16, 24(r1)	
	lwz	StrVal24, 28(r1)	
	lwz	StrVal25, 32(r1)	
	lwz	StrVal26, 36(r1)	
	lwz	StrVal27, 40(r1)	
	lwz	StrVal28, 44(r1)	
	lwz	StrVal29, 48(r1)	
	lwz	StrVal30, 52(r1)	
	lwz	StrVal31, 56(r1)	
	lwz	LdVal0B, 60(r1)	
	lwz	LdVal1B, 64(r1)	
	lwz	LdVal2B, 68(r1)	
	lwz	LdVal3B, 72(r1)	
	lwz	LdVal4B, 76(r1)	
	lwz	LdVal5B, 80(r1)	
	lwz	LdVal6B, 84(r1)	
	lwz	LdVal7B, 88(r1)	
	addi	r1, 	r1, 92		/* update stack pointer */
/* return */
	blr	





