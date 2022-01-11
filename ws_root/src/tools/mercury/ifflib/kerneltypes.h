#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H


/******************************************************************************
**
**  @(#) types.h 95/06/27 1.20
**  $Id: types.h,v 1.37 1994/10/07 19:55:04 vertex Exp $
**
**  Standard Portfolio types and constants
**
******************************************************************************/


/* use this to tell you're compiling using Portfolio files */
#define PORTFOLIO_OS

/* use this for compile-time identification of the includes/libs available */
#define PORTFOLIO_OS_VERSION 27


/*****************************************************************************/


#ifndef __STDDEF_H
#include <stddef.h>
#endif

/*****************************************************************************/


/* signed generic integers */
typedef signed char	 int8;
typedef signed short	 int16;
typedef signed long	 int32;

/* 64-bit types, not supported by all compilers */
#ifndef NO_64_BIT_SCALARS
typedef signed long int64;
typedef unsigned long long uint64;
#endif

/* unsigned generic integers */
typedef unsigned char	   uint8;
typedef unsigned short	   uint16;
typedef unsigned long	   uint32;

/* floating point */
typedef float  float32;
typedef double float64;

/* special purpose definitions */
typedef uint8   bool;
typedef uint16  unichar;
typedef int32   Item;
typedef int32   Err;
typedef float32 gfloat;
typedef Item	CodeHandle;
typedef uint32  PackedID;

/* for shared variables */
typedef	volatile long	        vint32;
typedef	volatile unsigned long	vuint32;

#ifndef TRUE
#define TRUE	((bool)1)
#endif
#ifndef FALSE
#define FALSE	((bool)0)
#endif

/* this batch is here for source compatibility only, don't use in new code */
typedef uint8	uchar;
typedef uint8	ubyte;
#ifndef _SYS_BSD_TYPES_H
typedef uint32	ulong;
#endif
typedef	uint8   Boolean;
#ifndef false
#define false	FALSE
#endif
#ifndef true
#define true	TRUE
#endif

/* Use this to touch an unused variable, to avert a compiler warning. */
#define TOUCH(argument)	((void)argument)

/* Build a value of type PackedID */
#define MAKE_ID(a,b,c,d) ((PackedID)((uint32)(a)<<24 | (uint32)(b)<<16 | (uint32)(c)<<8 | (uint32)(d)))

#ifndef EXTERNAL_RELEASE
typedef uint32	pd_mask;
#define NPDBITS	(sizeof(pd_mask) * 8)	/* bits per mask  = 32 */
#define Make_Func(x,y) ((x (*)())y)
typedef int32	(*func_t)();
typedef void	*(*vpfunc_t)();
typedef uint32	(*uifunc_t)();
#define Offset(struct_type,field)	((uint32)&(((struct_type)NULL)->field))
#endif /* EXTERNAL_RELEASE */

/*****************************************************************************/


/* TagArgs - used to pass a list of arguments to functions */
typedef	void *TagData;

typedef struct TagArg
{
	uint32  ta_Tag;
	TagData	ta_Arg;
} TagArg, *TagArgP;

#define TAG_END  0
#define TAG_JUMP 254
#define TAG_NOP  255


/*****************************************************************************/


#endif /* __KERNEL_TYPES_H */
