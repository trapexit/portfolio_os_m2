/******************************************************************************
**
**  @(#) bitstream.c 96/11/25 1.7
******************************************************************************/

#ifndef __MISC_MPAUDIODECODE_H
#include <misc/mpaudiodecode.h>
#endif

#ifndef __KERNEL_DEBUG_H
#include <kernel/debug.h>
#endif

#include "bitstream.h"
#include "decode.h"
#include "mpegaudiotypes.h"
#include "mpegdebug.h"


/* NOTE: The PTS-handling code could be redesigned like this: When receiving
 * a new compressed buffer, set instance variables PTS_received and
 * PTS_received_is_valid_flag. When starting to decode a frame, copy this pair
 * to variables PTS_decoded and PTS_decoded_is_valid_flag, and set
 * PTS_received_is_valid_flag to FALSE. When returning a decoded frame,
 * return PTS_decoded and PTS_decoded_is_valid_flag, and set
 * PTS_decoded_is_valid_flag to FALSE. */

/**********************************************************************
 * resetReadStatus
 **********************************************************************/
 
 void resetReadStatus(BufferInfo *bi)
 {
 	bi->readStatus = 0;
 }
 
/**********************************************************************
 * flushRead
 **********************************************************************/
Err flushRead( BufferInfo *bi )
{
	Err		status = 0;

	/* send the current buffer (if any) back for refilling */
	if (bi->readBuf != NULL) {
		status = bi->CallbackFns.CompressedBfrReadFn(bi->theUnit, bi->readBuf );
	}
	
	/* Flush compressed data buffer info */
	bi->readBuf			= NULL;
	bi->curBytePtr		= NULL;
	bi->readWord		= 0;
	bi->readLen			= 0;
	bi->readBits		= 0;
	bi->PTSIsValid		= 0;
	bi->timeStamp		= 0;

	return (status);

} /* flushRead() */

/**********************************************************************
 * getBits 
 * read N bits from the bit stream 
 **********************************************************************/
uint32 getBits(uint32 N, BufferInfo *bi, Err *readStatus)
{
	int32 	i, j;
	uint32 	result;
	Err		status = 0;
	
	result = 0;
	
	if (bi->readStatus != 0) {	/* if persistant status indicates error on last */
		goto EXIT;				/* read attempt, just return zero result and    */
	}							/* persistant status again.						*/
	
	i = bi->readBits - N;
	j = 24 - bi->readBits;

	while (i < 0) {
		if ( bi->readLen <= 0L ) {
			/* the read buffer is empty, send current buffer (if any) back, then get the next one */
			if (bi->readBuf != NULL) {
				status = bi->CallbackFns.CompressedBfrReadFn(bi->theUnit, bi->readBuf);
				bi->readBuf = NULL;
			}
			
			if (status == 0) {
				status = bi->CallbackFns.GetCompressedBfrFn(bi->theUnit,
					(uint8 **)	&bi->readBuf,
					(int32 *) 	&bi->readLen,
					(uint32 *)	&bi->timeStamp, 
								&bi->PTSIsValid);
			}

			if (status != 0) {
				bi->readStatus = status;
				goto EXIT;
			}
			
			/* Set the current byte pointer to bitstream buffer */
			
			bi->curBytePtr	= bi->readBuf;

#ifdef DEBUG_AUDIO
			PRNT(("getBits(): Read new input bitstream buffer.\n"));
			PRNT(("readBits: %d \n", bi->readBits));
			PRNT(("PTSIsValid: %d \n", bi->PTSIsValid));
#endif

		}

		/* get 8 more bits */
		bi->readWord |= *(bi->curBytePtr++) << j;
		i += 8L;
		j -= 8L;
		bi->readLen--;
	}

	/* got enough bits, return N bits */
	result = bi->readWord >> (32 - N);
	bi->readBits = i;
	bi->readWord <<= N;

EXIT:

	*readStatus = bi->readStatus;

	return( result );
} /* getBits() */

/**********************************************************************
 * findSync
 *
 * This function seeks for a byte aligned sync word in the bit stream and
 * places the bit stream pointer after the sync word.  The search is
 * performed for 255 words and the last 12 bits tested are returned.
 * Therefore if a synch was detected the value MPEG_AUDIO_SYNCWORD
 * will be returned.
 **********************************************************************/
Err findSync(BufferInfo *bi)
{
	int32	oddBits;
	Err		status = 0;
	
	if (bi->readStatus != 0) {	/* if persistant status indicates error on last */
		goto EXIT;				/* read attempt, just return status again.	    */
	}
	
	bi->wholeFrame = true;

	/* seek to byte-aligned sync word */
	oddBits = bi->readBits & 0x7;	/* same as % 8 */
	bi->readWord <<= oddBits;
	bi->readBits -= oddBits;
	bi->readWord >>= (32 - bi->readBits);
	
	while( (bi->readWord & SYNCWORD_SHIFTLEFT_FOUR) != (SYNCWORD_SHIFTLEFT_FOUR ) ) {
		if ( bi->readLen <= 0L ) {
			/* the read buffer is empty, send current buffer (if any) back, then get the next one */
			if (bi->readBuf != NULL) {
				status = bi->CallbackFns.CompressedBfrReadFn(bi->theUnit, bi->readBuf);
				bi->readBuf = NULL;
			}
			if (status == 0) {
				status = bi->CallbackFns.GetCompressedBfrFn(bi->theUnit,
					(uint8 **)	&bi->readBuf,
					(int32 *) 	&bi->readLen,
					(uint32 *)	&bi->timeStamp, 
								&bi->PTSIsValid);
			}

			if (status != 0) {
				bi->readStatus = status;
				goto EXIT;
			}
			
			/* Set the current byte pointer to bitstream buffer */
			bi->curBytePtr	= bi->readBuf;

			if( bi->readBits ) {
				bi->wholeFrame = false;
			}
			
#ifdef DEBUG_AUDIO
			PRNT(("findSync: Read new buffer, wholeFrame->%d\n", bi->wholeFrame));
#endif

    	}

		/* bi->PTSIsValid = MPAPTSVALID; */ /* <-- Why was this here? It can
		 * make the previous PTS get replicated in the output if an input
		 * buffer contains multiple frames! */

		/* get 8 more bits */
		bi->readWord <<= 8L;
		bi->readWord |= *(bi->curBytePtr++);
		bi->readLen--;
	}
	
	/* got a sync word, clean up and return */
	/* point to the beginning of a frame */
	
	bi->readBits = 4L;
	bi->readWord <<= (32 - bi->readBits);
	
EXIT:

	return bi->readStatus;
}
