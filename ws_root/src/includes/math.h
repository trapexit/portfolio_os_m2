#ifndef __MATH_H
#define __MATH_H


/******************************************************************************
**
**  @(#) math.h 96/06/25 1.28
**
**  Standard floating-point support.
**
******************************************************************************/


#ifdef	__cplusplus
extern "C" {
#endif

/* this is set to INF */
#ifndef HUGE_VAL_F
extern float _huge_val_f;
#define HUGE_VAL_F	_huge_val_f
#endif

/* some useful constants */
#define M_PI            3.1415926535897932384626
#define M_PI_2          1.57079632679489661923
#define M_PI_4          0.78539816339744830962
#define M_1_PI          0.31830988618379067154
#define M_2_PI          0.63661977236758134308
#define M_2_SQRTPI      1.12837916709551257390
#define M_LN2           0.6931471805599453094172
#define M_LOG2E         1.4426950408889634074
#define M_LOG10E        0.43429448190325182765
#define M_SQRT2         1.4142135623730950488016
#define M_SQRT1_2       0.70710678118654752440

/* prototypes for all the functions in spmath */
float asinf(float x);
float acosf(float x);
float atanf(float x);
float atan2f(float y, float x);
float sinf(float x);
float cosf(float x);
float tanf(float x);
float sinhf(float x);
float coshf(float x);
float tanhf(float x);
float expf(float x);
float log10f(float x);
float logf(float x);
float powf(float x, float y);
float sqrtf(float x);
float sqrtff(float x);
float sqrtfff(float x);
float rsqrtf(float x);
float rsqrtff(float x);
float rsqrtfff(float x);
float floorf(float x);
float ceilf(float x);
float fabsf(float x);
float fmodf(float x, float y);
float frexpf(float x, int* exp);
float ldexpf(float x, int exp);
float modff(float x, float *iptr);


#ifdef __DCC__
/* special stuff for the Diab compiler */
#pragma no_side_effects acosf(errno), asinf(errno), atanf(errno), atan2f(errno)
#pragma no_side_effects tanf(errno), coshf(errno)
#pragma no_side_effects sinhf(errno), tanhf(errno), expf(errno)
#pragma no_side_effects logf(errno), log10f(errno)
#pragma no_side_effects powf(errno)

#pragma pure_function sinf, cosf, fmodf
#pragma pure_function floorf, ceilf, ldexpf, frexpf
#pragma pure_function rsqrtff, rsqrtf, sqrtfff, sqrtff, sqrtf

#pragma pure_function fabsf
#pragma inline fabsf
__asm float fabsf( float num )
{
%   reg		num;
    fmr		f1,num
    fabs	f1,f1
}

#pragma pure_function rsqrtfff
#pragma inline rsqrtfff
__asm float rsqrtfff( float num )
{
%   reg		num;
    fmr		f1,num
    frsqrte	f1,f1
}
#endif

#ifdef	__cplusplus
}
#endif


#endif /* __MATH_H */
