/* @(#) sqrtfff.S 96/01/25 1.1 */

	.global sqrtfff
	
	.text
	
	/* float sqrtfff( float x ) */
	/* This is an extremely fast approximation to the square root. */
	/* The measured maximum error is about 1.7% */

sqrtfff:
	# if x is 0, return now
	lis     r3,.L0@ha	
	lfs	f0,.L0@l(r3)
	fcmpu	0,f0,f1
	beqlr
	frsqrte	f4, f1
	fmuls	f1,f1,f4
	blr
					
        .align  2
.L0:	.float	0.0
