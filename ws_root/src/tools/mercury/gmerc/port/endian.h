
/*
	File:		endian.h

	Written by:	eric carlson

	Simple C preprocessor macros to handle big/little endian byte swapping
	
	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

*/

#ifndef	__ENDIAN_H__
#define	__ENDIAN_H__

/*
 * possible values for the compile time symbol "__MACHINE_ENDIANNESS" are: 
 *	__BIG_ENDIAN
 *	__LITTLE_ENDIAN
 */
#define	__BIG_ENDIAN		0
#define	__LITTLE_ENDIAN		1

/*
 * one of the following compile time symbols must be defined so that the byte swapping
 *  macros "know" the endianness of the machine
 */  
#if (defined(__MACINTOSH__) || defined(__MWERKS__) || defined(macintosh) )
	#define	__MACHINE_ENDIANNESS	__BIG_ENDIAN
#endif
#ifdef __3DO__
	#define	__MACHINE_ENDIANNESS	__BIG_ENDIAN
#endif
#ifdef __WINTEL__
	#define	__MACHINE_ENDIANNESS	__LITTLE_ENDIAN
#endif

#ifdef _SGI_
	#define	__MACHINE_ENDIANNESS	__BIG_ENDIAN
#endif

#ifdef __Sun__
	#define	__MACHINE_ENDIANNESS	__BIG_ENDIAN
#endif

/* sanity check the current setting.  first make sure it has a value... */
#ifndef	__MACHINE_ENDIANNESS
#	error __MACHINE_ENDIANNESS undefined
#endif

/* and that we understand the value... */
#if ((__MACHINE_ENDIANNESS != __BIG_ENDIAN) && (__MACHINE_ENDIANNESS != __LITTLE_ENDIAN))
#	error __MACHINE_ENDIANNESS invalid
#endif


#define Swap32Bits(num)	( (((num) & 0x000000FF) << 24)	\
						| (((num) & 0x0000FF00) <<  8)	\
						| (((num) & 0x00FF0000) >>  8)	\
						| (((num) & 0xFF000000) >> 24))

#define Swap16Bits(num)	( (((num) << 8) & 0xFF00)		\
						| (((num) >> 8) & 0x00FF))

/* Big Endian */
#if	(__MACHINE_ENDIANNESS == __BIG_ENDIAN)
	/* do nothing to values which should be big endian */
	#define BEUInt8(num)	(num)
	#define BEInt8(num)		(num)
	#define BEUInt16(num)	(num)
	#define BEInt16(num)	(num)
	#define BEUInt32(num)	(num)
	#define BEInt32(num)	(num)

	/* swap bytes on values which should be little endian */
	#define LEUInt8(num)	(num)
	#define LEUInt16(num)	Swap16Bits((num))
	#define LEUInt32(num)	Swap32Bits((num))
	#define LEInt8(num)		(num)
	#define LEInt16(num)	Swap16Bits((num))
	#define LEInt32(num)	Swap32Bits((num))
#endif	/* big endian */

/* Little Endian */
#if	(__MACHINE_ENDIANNESS == __LITTLE_ENDIAN)
	/* swap bytes on values which should be big endian */
	#define BEUInt8(num)	(num)
	#define BEUInt16(num)	Swap16Bits((num))
	#define BEUInt32(num)	Swap32Bits((num))
	#define BEInt8(num)		(num)
	#define BEInt16(num)	Swap16Bits((num))
	#define BEInt32(num)	Swap32Bits((num))

	/* do nothing to values which should be little endian */
	#define LEUInt8(num)	(num)
	#define LEUInt16(num)	(num)
	#define LEUInt32(num)	(num)
	#define LEInt8(num)		(num)
	#define LEInt16(num)	(num)
	#define LEInt32(num)	(num)
#endif	/* little endian */


#endif		/* __ENDIAN_H__ */

