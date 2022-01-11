/* @(#) rsqrtff.S 96/01/25 1.1 */

	.global	rsqrtff
	
	.text
	

	/* float rsqrtff( float x ) */
	/* This is a fast approximation of 1/sqrt(x) */
	/* The error is approximately 0.04% */

rsqrtff:
	lis     r3,.L3@ha	
	frsqrte	f4, f1		# let y be our approximation of 1.0/sqrt(x)
				# Apply 1 reciproot iteration
				# y = 0.5 * y( 3.0 - x*y*y )
	lfs	f6,.L3@l(r3)	
	fmuls	f5, f4, f4		
	lis     r3,.L0.5@ha	
	fnmsubs	f5, f1, f5, f6	# f5 = 3.0 - x*y*y
	lfs	f7,.L0.5@l(r3)	
	fmuls	f4, f4, f5	# f4 = y( 3.0 - x*y*y )
	fmuls	f1, f4, f7	# y = 0.5 * y( 3.0 - x*y*y )
	blr

					
        .align  2
.L3:	.float	3.0
.L0.5:	.float	0.5
