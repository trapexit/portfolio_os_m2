/*
 *	@(#) values.h 95/04/25 1.5
 */
#ifndef __VALUES_H
#define __VALUES_H

#include <limits.h>
#include <float.h>

#define BITSPERBYTE	CHAR_BIT
#define BITS(t)		((int)(CHAR_BIT * sizeof(t)))

#define _HIBIT(t)	((t)(1L << BITS(t)-1))
#define HIBITS		_HIBIT(short)
#define HIBITI		_HIBIT(int)
#define HIBITL		_HIBIT(long)

#define MAXSHORT	SHRT_MAX
#define MAXINT		INT_MAX
#define MAXLONG		LONG_MAX

#define MAXDOUBLE	DBL_MAX
#define MAXFLOAT	FLT_MAX
#define MINDOUBLE	DBL_MIN
#define MINFLOAT	FLT_MIN

#define DMAXEXP		DBL_MAX_EXP
#define DMINEXP		DBL_MIN_EXP
#define FMAXEXP		FLT_MAX_EXP
#define FMINEXP		FLT_MIN_EXP

#define DSIGNIF		DBL_MANT_DIG
#define FSIGNIF		FLT_MANT_DIG

#define DMAXPOWTWO	((double)(1L << BITS(long)-2) * (1L << DBL_MANT_DIG-BITS(long)+1))
#define FMAXPOWTWO	((float)(1L << FLT_MANT_DIG-1))
#define LN_MAXDOUBLE	(M_LN2*DBL_MAX_EXP)
#define LN_MINDOUBLE	(M_LN2*(DBL_MIN_EXP-1))
#define H_PREC		(DBL_MANT_DIG&1 ? (1L<<DBL_MANT_DIG/2)*M_SQRT2 : 1L<<DBL_MANT_DIG/2)
#define X_EPS		(1.0/H_PREC)
#define X_PLOSS		((double)(long)(M_PI * H_PREC))
#define X_TLOSS		(M_PI * DMAXPOWTWO)

#endif /* __VALUES_H */
