/* @(#) modff.S 96/02/23 1.3 */

/**
|||	AUTODOC -public -class Math -group Conversion -name modff
|||	get integer and fractional parts of a float
|||
|||	  Synopsis
|||
|||	    float modff( float frac, float *iptr )
|||
|||	  Description
|||
|||	    modff splits a float into an integer and a fractional part.
|||	    For example, modff( 5.75, &i ) will return 0.75 and set integer
|||	    i to 5.
|||
|||	  Arguments
|||
|||	    frac
|||	        floating point number
|||
|||	    iptr
|||	        pointer to where you want the integer part stored
|||
|||	  Return Value
|||
|||	     The fractional value is returned as a float.
|||
|||	  Implementation
|||
|||	    Assembly function in libspmath.  
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
**/

	.global	modff
	.type	modff,@function
	.text
modff:
	stfs	f1,0(r3)
	lwz	r5,0(r3)
	
	rlwinm	r4,r5,9,24,31
	subfic	r4,r4,150
	cmpwi	r4,23
	ble+	.LAllInt
	lis	r4,0
	stw	r4,0(r3)
	blr

.LAllInt:			
	cmpwi	r4,0
	bgt+	.LFracInt
	lis	r4,.Lzero@ha
	lfs	f1,.Lzero@l(r4)
	blr

.LFracInt:		# has fractional and integer parts		
	li	r6,-1
	slw	r6,r6,r4
	and	r5,r5,r6
	stw	r5,0(r3)
	lfs	f13,0(r3)
	fsubs	f1,f1,f13
	blr

        .align	2
.Lzero:	.float	0.0

	


