/* @(#) rsqrtfff.S 96/01/25 1.1 */

	.global	rsqrtfff
	
	.text
	
	/* float rqrtfff( float x ) */
	/* This is an extremely fast approximation of 1/sqrt(x) */
	/* The measured maximum error is about 1.7%*/

	/* This function is also included as an inline assembly function */
	/* in math.h.  This code is here so function pointers work. */

rsqrtfff:
	frsqrte	f1, f1
	blr
