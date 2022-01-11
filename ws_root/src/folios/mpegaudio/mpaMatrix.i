/* ******************************************************************************* */
/* 	General registers */
/* ******************************************************************************* */


/* This macro is the prefered way of declaring a public function in asm.
 * Use this for public functions.
 */

	.macro
	DECFN	&fnname
	.type	&fnname,@function
	.globl	&fnname
	.text
&fnname:
	/* Code starts here */
	.endm


/* placeholder for input registers */
#define inputSamp	3
#define outputSamp	4
#define matrixCoef	5
#define scale		6
#define sblimit		7
#define nch		8

/* scratch registers */
#define groupCount	9
#define touchOutOffset	10
#define touchInOffset	11
#define SaveLR		12

#define AUDIO_FLOAT_SIZE 4

#define COS1_64   0*AUDIO_FLOAT_SIZE
#define COS3_64   1*AUDIO_FLOAT_SIZE
#define COS5_64   2*AUDIO_FLOAT_SIZE
#define COS7_64   3*AUDIO_FLOAT_SIZE
#define COS9_64   4*AUDIO_FLOAT_SIZE
#define COS11_64  5*AUDIO_FLOAT_SIZE
#define COS13_64  6*AUDIO_FLOAT_SIZE
#define COS15_64  7*AUDIO_FLOAT_SIZE
#define COS17_64  8*AUDIO_FLOAT_SIZE
#define COS19_64  9*AUDIO_FLOAT_SIZE
#define COS21_64 10*AUDIO_FLOAT_SIZE
#define COS23_64 11*AUDIO_FLOAT_SIZE
#define COS25_64 12*AUDIO_FLOAT_SIZE
#define COS27_64 13*AUDIO_FLOAT_SIZE
#define COS29_64 14*AUDIO_FLOAT_SIZE
#define COS31_64 15*AUDIO_FLOAT_SIZE
#define COS1_32  16*AUDIO_FLOAT_SIZE
#define COS3_32  17*AUDIO_FLOAT_SIZE
#define COS5_32  18*AUDIO_FLOAT_SIZE
#define COS7_32  19*AUDIO_FLOAT_SIZE
#define COS9_32  20*AUDIO_FLOAT_SIZE
#define COS11_32 21*AUDIO_FLOAT_SIZE
#define COS13_32 22*AUDIO_FLOAT_SIZE
#define COS15_32 23*AUDIO_FLOAT_SIZE
#define COS1_16  24*AUDIO_FLOAT_SIZE
#define COS3_16  25*AUDIO_FLOAT_SIZE
#define COS5_16  26*AUDIO_FLOAT_SIZE
#define COS7_16  27*AUDIO_FLOAT_SIZE
#define COS1_8   28*AUDIO_FLOAT_SIZE
#define COS3_8   29*AUDIO_FLOAT_SIZE
#define COS1_4   30*AUDIO_FLOAT_SIZE

