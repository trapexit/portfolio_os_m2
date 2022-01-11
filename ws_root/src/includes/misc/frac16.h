#ifndef __MISC_FRAC16_H
#define __MISC_FRAC16_H


/******************************************************************************
**
**  @(#) frac16.h 95/06/17 1.4
**
**  Minimal support for 16.16 fixed-point fractions.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* -------------------- types */

typedef int32 frac16;           /* signed 16.16 fixed point fraction */
typedef uint32 ufrac16;         /* unsigned 16.16 fixed point fraction */


/* -------------------- Conversion macros */

    /* convert 32-bit integer to frac16 or ufrac16. supports both signed and unsigned. */
#define Convert32_F16(x)    ((x)<<16)

    /* convert frac16 or ufrac16 32-bit integer. supports both signed and unsigned. */
#define ConvertF16_32(x)    ((x)>>16)

    /* convert floating point value to frac16 (signed). */
#define ConvertFP_SF16(x)   ((frac16)((x) * 65536.0))

    /* convert floating point value to ufrac16 (unsigned). */
#define ConvertFP_UF16(x)   ((ufrac16)((x) * 65536.0))

    /* convert frac16 or ufrac16 to floating point value. supports both signed and unsigned. */
#define ConvertF16_FP(x)    ((x) / 65536.0)


/* -------------------- Simple math macros */

#define NegF16(x)   (-(x))
#define NotF16(x)   (~(x))
#define AddF16(x,y) ((x)+(y))
#define SubF16(x,y) ((x)-(y))

    /* Multiply two signed 16.16 integers together, and get a 16.16 result. */
    /* Overflows are not detected.  Lower bits are truncated. */
#define MulSF16(m1,m2)      ( (frac16)  ( ((int64) (frac16) (m1) * (int64) (frac16) (m2)) >> 16) )

    /* Multiply two unsigned 16.16 integers together, and get a 16.16 result. */
    /* Overflows are not detected.  Lower bits are truncated. */
#define MulUF16(m1,m2)      ( (ufrac16) ( ((uint64)(ufrac16)(m1) * (uint64)(ufrac16)(m2)) >> 16) )

    /* Divide a signed 16.16 fraction into another an return a signed 16.16 result. */
    /* !!! integer version requires __div16() */
/* #define DivSF16(d1,d2)   ( (frac16)  ( ((int64) (frac16) (d1) << 16) / (frac16) (d2)) ) */
#define DivSF16(d1,d2)      ConvertFP_SF16 ( ConvertF16_FP((frac16)(d1)) / ConvertF16_FP((frac16)(d2)) )

    /* Divide an unsigned 16.16 fraction into another an return an unsigned 16.16 result. */
    /* !!! integer version requires __udiv16() */
/* #define DivUF16(d1,d2)   ( (ufrac16) ( ((uint64)(ufrac16)(d1) << 16) / (ufrac16)(d2)) ) */
#define DivUF16(d1,d2)      ConvertFP_UF16 ( ConvertF16_FP((ufrac16)(d1)) / ConvertF16_FP((ufrac16)(d2)) )


/*****************************************************************************/


#endif /* __MISC_FRAC16_H */
