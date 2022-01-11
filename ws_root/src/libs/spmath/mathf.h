#ifndef __MATHF_H
#define __MATHF_H

/******************************************************************************
**
**  @(#) mathf.h 95/10/31 1.10
**
**  Single-precision floating-point support.
**
******************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

/* Errors */

#define DOMAIN		1
#define SING		2
#define OVERFLOW	3
#define UNDERFLOW	4

/* Some useful constants for float */

extern  float _NaN_f;

#define X_TLOSS_F ((float)(M_PI * FMAXPOWTWO))
#define H_PREC_F  ((float)(FSIGNIF % 2 ? (1L << FSIGNIF/2) * M_SQRT2 : 1L << FSIGNIF/2))
#define X_EPS_F   ((float)(1.0/H_PREC_F))
#define X_PLOSS_F ((float)(long)(M_PI * H_PREC_F))

#define LN_MAXFLOAT     ((float)(M_LN2 * FMAXEXP))
#define LN_MINFLOAT     ((float)(M_LN2 * (FMINEXP - 1)))

#ifdef	__cplusplus
}
#endif

#endif /* __MATHF_H */
