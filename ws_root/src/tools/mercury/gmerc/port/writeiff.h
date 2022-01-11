
/*
	File:		writeiff.h

	Written by:	eric carlson

	Functions to write data to an iff file in a specified endianness
	
	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.
*/

#ifndef _WRITE_IFF_H_
#define	_WRITE_IFF_H_

#define	DONT_USE_IFF_FOLIO

#ifdef __MWERKS__
	#define	DONT_USE_IFF_FOLIO
	#include "kerneltypes.h"
	#include "iff.h"
#else	/* #ifdef __MWERKS__ */
	#include "kerneltypes.h"
	#ifdef	DONT_USE_IFF_FOLIO
		#include "iff.h"
	#else
		#include <:misc:iff.h>
	#endif	/* DONT_USE_IFF_FOLIO */

	/* #include <:kernel:types.h> */
	/* #include <types.h> */
#endif	/*  #ifdef __MWERKS__ */

#include "endian.h"


#ifdef __cplusplus
extern "C" {
#endif

/* an enum to specify the endianness of a word AS IT SHOULD BE WRITTEN TO DISK (ie. NOT
 *  the current in memory format, though they may well be the same)
 */
typedef enum Endianness_tag
{
	eMotorola		= 0,
	eIntel,
	
	/* synonyms */
	eBigEndian		= eMotorola,
	eLittleEndian	= eIntel
}Endianness;


Err		PutBytes(IFFParser *parser, const void *buff, uint32 byteCount);
Err		PutInt8(IFFParser *parser, int8 aChar, Endianness ndn);
Err		PutUInt8(IFFParser *parser, uint8 aChar, Endianness ndn);
Err		PutInt16(IFFParser *parser, int16 aShort, Endianness ndn);
Err		PutUInt16(IFFParser *parser, uint16 aShort, Endianness ndn);
Err		PutInt32(IFFParser *parser, int32 aLong, Endianness ndn);
Err		PutUInt32(IFFParser *parser, uint32 aLong, Endianness ndn);
Err		PutFloat(IFFParser *parser, float aFloat, Endianness ndn);

Err		PutInt16s(IFFParser *parser, int16 *aShort, int32 count, Endianness ndn);
Err		PutUInt16s(IFFParser *parser, uint16 *aShort, int32 count, Endianness ndn);
Err		PutInt32s(IFFParser *parser, int32 *aLong, int32 count, Endianness ndn);
Err		PutUInt32s(IFFParser *parser, uint32 *aLong, int32 count, Endianness ndn);
Err		PutFloats(IFFParser *parser, float *aFloat, int32 count, Endianness ndn);


#ifdef __cplusplus
}
#endif

#endif	/* _WRITE_IFF_H_ */
