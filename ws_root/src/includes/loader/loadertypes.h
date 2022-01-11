#ifndef __LOADER_LOADERTYPES_H
#define __LOADER_LOADERTYPES_H


/******************************************************************************
**
**  @(#) loadertypes.h 96/05/11 1.11
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#define LOAD_FOR_PORTFOLIO
#endif

/* {{{ select loading for PORTFOLIO, UNIX, or MAC (default is PORTFOLIO) */

#ifndef	LOAD_FOR_PORTFOLIO
#ifndef	LOAD_FOR_UNIX
#ifndef	LOAD_FOR_MAC
#define	LOAD_FOR_PORTFOLIO
#endif
#endif
#endif

/* }}} */

/* {{{ if loading for PORTFOLIO: get types.h, provide LoaderBool */

#ifdef	LOAD_FOR_PORTFOLIO

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

typedef	Boolean	LoaderBool;

#endif

/* }}} */
/* {{{ if not loading for PORTFOLIO: provide many PORTFOLIO types; maybe get types.h */

#ifndef	LOAD_FOR_PORTFOLIO

#define __KERNEL_TYPES_H		/* Don't load types elsewhere		*/

/* The xlc compiler doesn't like signed chars - so we kill the signed keywrd */

#define signed

/* On the arm, char is unsigned */

/* compatibility typedefs */
typedef signed char	int8;
typedef signed short	int16;		/* Avoid this type - unreliable */
typedef signed long	int32;
typedef unsigned char   bool;

#ifndef _ALL_SOURCE

/* The IBM compiler defines this */

typedef unsigned char	uchar;		/* should be unsigned char */
typedef unsigned short	ushort;		/* Avoid this type - unreliable */
typedef unsigned long	ulong;

#else	/* _ALL_SOURCE */

#ifndef LOAD_FOR_MAC
#include "sys/types.h"
#endif

#ifdef __sun__
typedef u_char uchar;
typedef u_long ulong;
#endif

#endif	/* _ALL_SOURCE */

typedef uchar		ubyte;
typedef uchar		uint8;

typedef ushort		uint16;		/* Avoid this type - unreliable */
typedef ulong		uint32;

typedef volatile long	vlong;
typedef	vlong		vint32;

typedef volatile ulong	vulong;
typedef	vulong		vuint32;

typedef int32		Item;
typedef	int32		Err;

typedef	ubyte		LoaderBool;

#endif	/* LOAD_FOR_PORTFOLIO */

/* }}} */
/* {{{ if loading for UNIX: provide true and false */

#ifdef LOAD_FOR_UNIX

#ifndef true
#define true 1
#define false 0
#endif

#endif	/* LOAD_FOR_UNIX */

/* }}} */
/* {{{ if loading for MAC: provide size_t, get Types.h */

#ifdef	LOAD_FOR_MAC

typedef uint32		size_t;

#include <Types.h>

#endif	/* LOAD_FOR_MAC */

/* }}} */

#endif	/* __LOADER_LOOADERTYPES_H */
