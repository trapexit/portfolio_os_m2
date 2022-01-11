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
#define outputSamp	3
#define partialSums	4
#define windowTempArray 4
#define matrixSamp	5
#define windowCoef	6

/* scratch registers */
#define fLineStep	7
#define sampleCount	8
#define inPtr		9
#define outPtr		10
#define SaveLR		12
#define LdVal0		5
#define LdVal1		6
#define LdVal2		7
#define LdVal3		9
#define LdVal4		10
#define LdVal5		11
#define LdVal6		13
#define LdVal7		14
#define StrVal16	15
#define StrVal24	16
#define StrVal25	17
#define StrVal26	18
#define StrVal27	19
#define StrVal28	20
#define StrVal29	21
#define StrVal30	22
#define StrVal31	23
#define LdVal0B		24
#define LdVal1B		25
#define LdVal2B		26
#define LdVal3B		27
#define LdVal4B		28
#define LdVal5B		29
#define LdVal6B		30
#define LdVal7B		31

#define AUDIO_FLOAT_SIZE 4
#define AUDIO_INT_SIZE 4

