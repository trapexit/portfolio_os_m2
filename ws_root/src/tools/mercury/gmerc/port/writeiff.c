
/*
	File:		writeiff.c

	Written by:	eric carlson

	Functions to write data to an iff file in a specified endianness
	
	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.
*/

#include <string.h>
#include "bsdf_iff.h"
#include "writeiff.h"

#ifdef __MWERKS__
	#define	__MACINTOSH__
	#include <stdlib.h>

	/* the mac iff code is old, the function name has changed */
	/* 
	extern	int32 WriteChunkBytes(IFFParser	*iff, const void *buf, uint32 numBytes);
	#define	WriteChunk(iff, buf, numBytes)	WriteChunkBytes(iff, buf, numBytes)
	*/
#else
/*
	#include <:kernel:debug.h>
	#include <:kernel:mem.h>
	#include <:kernel:tags.h>
*/
	#include <stdlib.h>
#endif		/* __MWERKS__ not defined */


/* data writing methods */

/*
 * write the specified number of bytes.  returns the number of bytes written if it is able
 *  to write the number of bytes you requested, otherwise it returns an error code
 */
Err
PutBytes(IFFParser *parser, const void *buff, uint32 byteCount)
{
	Err		err = WriteChunk(parser, buff, byteCount);
	if ( (err >= 0) && (err != (int32)byteCount) )
		err = -1;
	return err;			/* @@@@@ FIXME: use a real error code here! @@@@@ */
}


/*
 * write a signed 8 bit word 
 */
Err
PutInt8(IFFParser *parser, int8 aChar, Endianness ndn)
{
	int8	temp = (ndn == eMotorola ) ? BEInt8(aChar) : LEInt8(aChar);
 	return PutBytes(parser, &temp, sizeof(int8));
}


/*
 * write an unsigned 8 bit word 
 */
Err
PutUInt8(IFFParser *parser, uint8 aChar, Endianness ndn)
{
	uint8	temp = (ndn == eMotorola ) ? BEUInt8(aChar) : LEUInt8(aChar);
 	return PutBytes(parser, &temp, sizeof(uint8));
}


/*
 * write a signed 16 bit integer 
 */
Err
PutInt16(IFFParser *parser, int16 aShort, Endianness ndn)
{
	int16	temp = (ndn == eMotorola ) ? BEInt16(aShort) : LEInt16(aShort);
 	return PutBytes(parser, &temp, sizeof(int16));
}



/*
 * write an unsigned 16 bit integer 
 */
Err
PutUInt16(IFFParser *parser, uint16 aShort, Endianness ndn)
{
	uint16	temp = (ndn == eMotorola ) ? BEUInt16(aShort) : LEUInt16(aShort);
 	return PutBytes(parser, &temp, sizeof(uint16));
}

/*
 * write a signed 32 bit integer 
 */
Err
PutInt32(IFFParser *parser, int32 aLong, Endianness ndn)
{
	int32	temp = (ndn == eMotorola ) ? BEInt32(aLong) : LEInt32(aLong);
 	return PutBytes(parser, &temp, sizeof(int32));
}

/*
 * write an unsigned 32 bit integer 
 */
Err
PutUInt32(IFFParser *parser, uint32 aLong, Endianness ndn)
{
	uint32	temp = (ndn == eMotorola ) ? BEUInt32(aLong) : LEUInt32(aLong);
 	return PutBytes(parser, &temp, sizeof(uint32));
}

/*
 * write a float 
 */
Err
PutFloat(IFFParser *parser, float aFloat, Endianness ndn)
{
	uint32	temp;
	memcpy((void *)&temp, (void *)&aFloat, sizeof(uint32));
	temp = (ndn == eMotorola ) ? BEUInt32(temp) : LEUInt32(temp);
 	return PutBytes(parser, &temp, sizeof(uint32));
}

/*
 * write signed 16 bit integers
 */
Err
PutInt16s(IFFParser *parser, int16 *aShort, int32 count, Endianness ndn)
{
	int32	i, err;
	for (i = 0; i < count; i++)
	{
		err = PutInt16(parser, aShort[i], ndn);
		if (err == -1)
			break;
	}
 	return err;
}



/*
 * write unsigned 16 bit integers
 */
Err
PutUInt16s(IFFParser *parser, uint16 *aShort, int32 count, Endianness ndn)
{
	int32	i, err;
	for (i = 0; i < count; i++)
	{
		err = PutUInt16(parser, aShort[i], ndn);
		if (err == -1)
			break;
	}
 	return err;
}

/*
 * write signed 32 bit integers
 */
Err
PutInt32s(IFFParser *parser, int32 *aLong, int32 count, Endianness ndn)
{
	int32	i, err;
	for (i = 0; i < count; i++)
	{
		err = PutInt32(parser, aLong[i], ndn);
		if (err == -1)
			break;
	}
 	return err;
}

/*
 * write unsigned 32 bit integers
 */
Err
PutUInt32s(IFFParser *parser, uint32 *aLong, int32 count, Endianness ndn)
{
	int32	i, err;
	for (i = 0; i < count; i++)
	{
		err = PutUInt32(parser, aLong[i], ndn);
		if (err == -1)
			break;
	}
 	return err;
}

/*
 * write floats 
 */
Err
PutFloats(IFFParser *parser, float *aFloat, int32 count, Endianness ndn)
{
	int32	i, err;
	for (i = 0; i < count; i++)
	{
		err = PutFloat(parser, aFloat[i], ndn);
		if (err == -1)
			break;
	}
 	return err;
}

