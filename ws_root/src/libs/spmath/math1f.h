
/* @(#) math1f.h 95/04/25 1.1 */

/* math1f.h -- float version of math1.h
 * Athour:      aem, 910726
 * History:	aem, 930216: Corrected mc88k __POL? macros
 * COPYRIGHT 1991 - 1993: Diab Data AB, Sweden
 */

#ifndef _MATH1F__H
#define _MATH1F__H

/* Math error: writes a constant string on stderr */
#define MERROR(s)	printf("%s",s)

/* Polynomial expandors. Observe! polynomials should be reversed! */
#if !defined(m88k) && !defined(__m88k)	/* 88k uses subroutines for these */
#define __POL1F(C,X)	((C)[0]*(X) + (C)[1])
#define __POL2F(C,X)	(__POL1F(C,X)*(X) + (C)[2])
#define __POL3F(C,X)	(__POL2F(C,X)*(X) + (C)[3])
#define __POL4F(C,X)	(__POL3F(C,X)*(X) + (C)[4])
#define __POL5F(C,X)	(__POL4F(C,X)*(X) + (C)[5])
#define __POL6F(C,X)	(__POL5F(C,X)*(X) + (C)[6])
#define __POL7F(C,X)	(__POL6F(C,X)*(X) + (C)[7])
#define __POL8F(C,X)	(__POL7F(C,X)*(X) + (C)[8])
#define __POL9F(C,X)	(__POL8F(C,X)*(X) + (C)[9])
#define __POL10F(C,X)	(__POL9F(C,X)*(X) + (C)[10])
#else
extern float __POL1F(float arr[], float x), __POL2F(float arr[], float x);
extern float __POL3F(float arr[], float x), __POL4F(float arr[], float x);
extern float __POL5F(float arr[], float x), __POL6F(float arr[], float x);
extern float __POL7F(float arr[], float x), __POL8F(float arr[], float x);
extern float __POL9F(float arr[], float x), __POL10F(float arr[], float x);
#endif

/* float IEEE format */
union ieee_bits_f {
	struct {
		unsigned sign:1;
		unsigned exp:8;
		unsigned mant:23;
	} b;
	struct {
		unsigned long i;
	} i;
	float f;
};

/* Information about float IEEE */
#define IEEE_BIAS_F	127             /* Exponent bias                */
#define IEEE_MAX_F	255             /* Maximum exponent             */
#define IEEE_MANT_F	23              /* Number of bits in matissa    */
#define IEEE_EBIT_F	(1L<<23)        /* High bit in mantissa         */

#endif


