#ifndef __KERNEL_TYPES_H
#define __KERNEL_TYPES_H


/******************************************************************************
**
**  @(#) types.h 96/09/11 1.28
**
**  Standard Portfolio types and constants
**
******************************************************************************/


/* use this to tell you're compiling using Portfolio files */
#define PORTFOLIO_OS

/* Use this for compile-time identification of the includes/libs available.
 * This number gets incremented with every OS release, so that you can do
 * some compile-time checking of which feature set is available, to let you
 * use the same source code base with different versions of the OS includes
 * and libraries.
 */
#define PORTFOLIO_OS_VERSION 33


/*****************************************************************************/


#ifndef __STDDEF_H
#include <stddef.h>
#endif


/*****************************************************************************/


/* signed generic integers */
typedef signed char	   int8;
typedef signed short	   int16;
typedef signed long	   int32;

/* unsigned generic integers */
typedef unsigned char	   uint8;
typedef unsigned short	   uint16;
typedef unsigned long	   uint32;

/* 64-bit types, not supported by all compilers */
#ifndef NO_64_BIT_SCALARS
typedef signed long long   int64;
typedef unsigned long long uint64;
#endif

/* floating point */
typedef float  float32;
typedef double float64;

/* special purpose definitions */
typedef uint8   bool;
typedef uint16  unichar;
typedef int32   Item;
typedef int32   Err;
typedef float32 gfloat;
typedef uint32  PackedID;
typedef uint32	HardwareID;

/* for shared variables */
typedef	volatile long	        vint32;
typedef	volatile unsigned long	vuint32;

#define TRUE	((bool)1)
#define FALSE	((bool)0)

/* this batch is here for source compatibility only, don't use in new code */
typedef uint8	uchar;
typedef uint8	ubyte;
typedef uint32	ulong;
typedef	uint8   Boolean;
#define false	FALSE
#define true	TRUE

/* Use this to touch an unused variable, to avert a compiler warning. */
#define TOUCH(argument)	((void)argument)

/* Build a value of type PackedID */
#define MAKE_ID(a,b,c,d) ((PackedID)((uint32)(a)<<24 | (uint32)(b)<<16 | (uint32)(c)<<8 | (uint32)(d)))

#ifndef EXTERNAL_RELEASE
#define Make_Func(x,y) ((x (*)())y)
#define Offset(struct_type,field)	((uint32)&(((struct_type)NULL)->field))
#endif /* EXTERNAL_RELEASE */

/*****************************************************************************/


/* TagArgs - used to pass a list of arguments to functions */
typedef	void *TagData;

typedef struct TagArg
{
    uint32  ta_Tag;
    TagData ta_Arg;
} TagArg, *TagArgP;

#define TAG_END  0
#define TAG_JUMP 0xfffffffe
#define TAG_NOP  0xffffffff


/*****************************************************************************/


#endif /* __KERNEL_TYPES_H */
