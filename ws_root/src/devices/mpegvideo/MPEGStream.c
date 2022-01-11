/* @(#) MPEGStream.c 96/12/11 1.18 */
/* file: MPEGStream.c */
/* mpeg stream io routines */
/* 9/28/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include <kernel/debug.h>

#ifndef MPEGSTREAM_HEADER
#include "MPEGStream.h"
#endif

#ifdef TRACE_PTS
#define TRACEPTS_SIZE 1024
uint32 gTracePTSLog[ TRACEPTS_SIZE ];
int32 gTracePTSIndex = 0;
#define mark(a) \
{ gTracePTSLog[ gTracePTSIndex++ ] = (uint32) a;\
  gTracePTSIndex %= TRACEPTS_SIZE;\
  gTracePTSLog [ gTracePTSIndex ] = 0xffffffffL; }
#else
#define mark(a)
#endif

/*	this code is segmented into two sections by the following #ifdef,
	the first section is the 'real' M2 driver code and the second
	section is used for standalone testing of the parser */

#ifndef STANDALONE
#include "mpVideoDriver.h"

static int32 GetNextBuffer(tVideoDeviceContext* theUnit, streamContext *s);

int32 NextStartCode(tVideoDeviceContext* theUnit, streamContext *streamID)
{
	int32 code, status;
	uint32 index, i;

	/* copy first three bytes of the lookahead buffer */
	code = *((int32 *) streamID->lookAhead) >> 8;

	/* look through the lookahead buffer one byte at a time */
	for( index = 3; index < LOOKAHEADSIZE; index++ )
	{
		/* shift in the next byte */
		code = (code << 8) | streamID->lookAhead[ index ];

		/* start code in the lookahead? */
		if( (code & 0xffffff00L) == START_CODE_PREFIX )
		{
			/* if start code is at the front, we're done */
			if( index == 3 )
				return( code );

			/* move the start code to the front */
			for( i = 0, index -= 3; index < LOOKAHEADSIZE; i++, index++ )
				streamID->lookAhead[ i ] = streamID->lookAhead[ index ];

			/* fill the rest of the lookahead buffer */
			for( ; i < LOOKAHEADSIZE; i++ )
			{
				while( streamID->length <= 0L )
				{
					if( (status = GetNextBuffer( theUnit, streamID )) != 0L )
						return(status);
				}
				streamID->lookAhead[ i ] = *(streamID->buffer++);
				streamID->length--;
			}
			return( code );
		}
	}

	/* didn't find start code in the lookahead buffer, try current buffer */
    while(1)
	{
		/* if buffer is empty try the next buffer */
		while( streamID->length <= 0L )
		{
			if( (status = GetNextBuffer( theUnit, streamID )) != 0L )
				return(status);
		}
		/* shift in the next byte */
		code = (code << 8) | *(streamID->buffer++);
		streamID->length--;

		/* start code in buffer? */
		if( (code & 0xffffff00L) == START_CODE_PREFIX )
		{
			/* copy start code to front of the lookahead buffer */
			*((int32 *) streamID->lookAhead) = code;

			/* refill the lookahead buffer */
			for( i = 4; i < LOOKAHEADSIZE; i++ )
			{
				while( streamID->length <= 0L )
				{
					if( (status = GetNextBuffer( theUnit, streamID )) != 0L )
						return(status);
				}
				streamID->lookAhead[ i ] = *streamID->buffer++;
				streamID->length--;
			}
			return( code );
		}
	}
	/* no path to here */
}

int32
FlushBytes(tVideoDeviceContext* theUnit, streamContext *streamID, uint8 count)
{
	/* NOTE: Flushes upto 256 bytes. */
	uint8 tempBuffer[256];

	return MPRead(theUnit, streamID, tempBuffer, count);
}

static int32 GetNextBuffer(tVideoDeviceContext *theUnit, streamContext *s)
{
	int32 readResult;

	MPVCompleteRead(theUnit, 0);
	s->lastFlags = s->flags;
	s->lastPTS = s->PTS;
	s->lastUserData = s->userData;
	readResult = MPVRead(theUnit, &s->bufferStart, &s->length,
						 &s->PTS, &s->userData);

	mark(2); mark( (int32) s->bufferStart ); mark( s->length ); mark( s->PTS );

	s->buffer = s->bufferStart;
	s->bufferEnd = s->buffer + s->length;
	switch( readResult )
	{
		case 0:							/* no pts */
			s->flags &= ~SC_FLAG_PTS_VALID;
			return( 0L );
		case MPVPTSVALID:				/* pts valid */
			s->flags |= SC_FLAG_PTS_VALID;
			return( 0L );
		case MPVFLUSHWRITE:				/* complete current write */
		case MPVFLUSHDRIVER:
		case ABORTED:
			return readResult;
	}
	return( 0L );
}

/* get the pts at the current stream pointer */

int32 MPCurrentPTS( streamContext *s, uint32 *thePTS, uint32 *userData )
{
	uint8 *sp;
	int32 result;

	/* calculate effective stream position */
	sp = s->buffer - LOOKAHEADSIZE;

	mark( s->bufferStart ); mark(sp); mark( s->buffer ); mark( s->bufferEnd );

	/* is the effective stream position within the current read buffer? */
	if( (s->bufferStart <= sp) && (sp < s->bufferEnd) )
	{
		/* return the current pts */
		*thePTS = s->PTS;
		*userData = s->userData;
		result = s->flags & SC_FLAG_PTS_VALID;

		/* once the pts is used, it is no longer valid */
		s->flags &= ~SC_FLAG_PTS_VALID;

		mark( 0 ); mark( *thePTS ); mark( result ); mark( s->flags );
		return( result );
	}
	else
	{
		/* return the pts from the last buffer */
		*thePTS = s->lastPTS;
		*userData = s->lastUserData;
		result = s->lastFlags & SC_FLAG_PTS_VALID;

		/* once the pts is used, it is no longer valid */
		s->lastFlags &= ~SC_FLAG_PTS_VALID;
		mark( 1 ); mark( *thePTS ); mark( result ); mark( s->lastFlags );
		return( result );
	}
}

/* open the stream */

int32 MPOpen(tVideoDeviceContext* theUnit, streamContext *s )
{
    int32 status, i;

	s->length = 0L;

    /* initialize our state */
	for( i = 0; i < LOOKAHEADSIZE; i++ )
	{
		while( s->length <= 0L )
		{
			if( (status = GetNextBuffer(theUnit, s)) != 0L )
				return( status );
		}
		s->lookAhead[ i ] = *s->buffer++;
		s->length--;
	}
	s->offset = 0L;
	s->DMAState = DMAStateNoDMA;			/* no DMA in progress */

	return( 0L );
}

/* read length bytes into buffer pointed to by dest, advance read pointer */

int32 MPRead(tVideoDeviceContext* theUnit, streamContext *s,
			 char *dest, int32 length)
{
	int32 i, status;
	
	s->offset += length;
	
	/* can we get all the read bytes out of the lookahead buffer? */
	if( length <= LOOKAHEADSIZE )
	{
		/* copy bytes from the lookAhead buffer */
		for( i = 0L; i < length; i++ )
			dest[ i ] = s->lookAhead[ i ];

		/* shift remaining bytes in the lookAhead buffer */
		for( i = 0L; i < LOOKAHEADSIZE - length; i++ )
			s->lookAhead[ i ] = s->lookAhead[ i + length ];

		/* fill empty bytes in the lookAhead buffer */
		for( ; i < LOOKAHEADSIZE; i++ )
		{
			while( s->length <= 0L )
			{
				if( (status = GetNextBuffer(theUnit, s)) != 0L )
					return( status );
			}
			s->lookAhead[ i ] = *s->buffer++;
			s->length--;
		}
		return( 0L );
	}
	/* copy out all of the lookAhead buffer */
	for( i = 0L; i < LOOKAHEADSIZE; i++ )
		dest[ i ] = s->lookAhead[ i ];

	/* get remainder of the requested bytes from the non-lookAhead buffer */
	for( ; i < length; i++ )
	{
		/* if we've drained it */
		while( s->length <= 0L )
		{
			/* get the next one */
			if( (status = GetNextBuffer(theUnit, s)) != 0L )
				return( status );
		}
		dest[ i ] = *s->buffer++;
		s->length--;
	}
	/* refill the lookahead buffer */
	for( i = 0; i < LOOKAHEADSIZE; i++ )
	{
		while( s->length <= 0L )
		{
			if( (status = GetNextBuffer(theUnit, s)) != 0L )
				return( status );
		}
		s->lookAhead[ i ] = *s->buffer++;
		s->length--;
	}
	return( 0L );		
}

/* read length bytes into dest buffer, DO NOT advance read pointer */

int32 MPLook( streamContext *s, char *dest, int32 length )
{
	int32 i;
	
	for( i = 0L; i < length; i++ )
		dest[ i ] = s->lookAhead[ i ];
		
	return( 0L );
}

/* enter/continue DMA mode and get next DMA address and length */

int32 MPNextDMA(tVideoDeviceContext* theUnit, streamContext *s,
				uint8 **addr, int32 *length)
{
	int32 status;

	switch( s->DMAState )
	{
		case DMAStateNoDMA:				/* no DMA completed */
			*addr = s->lookAhead;		/* so DMA the lookAhead buffer */
			*length = LOOKAHEADSIZE;
			s->DMAState = DMAStateLookAheadDMA;
			break;
		case DMAStateBufferDMA:			/* DMA from buffer completed */
			s->length = 0L;
		case DMAStateLookAheadDMA:		/* DMA from lookAhead completed */
			while( s->length <= 0L )	/* DMA from next buffer */
			{
				if( (status = GetNextBuffer(theUnit, s)) != 0L )
					return( status );
			}
			*addr = s->buffer;
			*length = s->length;
			s->DMAState = DMAStateBufferDMA;
			break;
	}
	return( 0L );
}

/*	stop DMA mode, nextBytes and len points to a buffer containing any
	hardware buffered bytes, nextAddr is the address that would be DMA'd next.
	len must be less than or equal to LOOKAHEADSIZE	*/


int32 MPEndDMA(tVideoDeviceContext* theUnit, streamContext *s,
			   uint8 *nextBytes, int32 len, uint8 *nextAddr)
{
	int32 count,i;
	uint8 temp[ LOOKAHEADSIZE ];

	switch( s->DMAState )
	{
		case DMAStateNoDMA:
			/* no DMA in progress */
			PERR(("ACK: MPEndDMA no DMA in progress!\n"));
			return( 0L );
		case DMAStateLookAheadDMA:
			/* DMA from lookAhead was in progress */
			s->DMAState = DMAStateNoDMA;

			/* read out bytes already used */
			count = (int32) (nextAddr - len - s->lookAhead);
			return MPRead(theUnit, s, temp, count);
		case DMAStateBufferDMA:
			/* DMA from buffer was in progress */
			s->DMAState = DMAStateNoDMA;

			/* special case if we've just exhausted the buffer */
			/* try not to wait for the next buffer to become */
			/* available as this may delay the calling application */
			if( nextAddr == 0L )
			{
				for( i = 0L; i < LOOKAHEADSIZE - len; i++ )
					s->lookAhead[ i ] = 0;
				for( ; i < LOOKAHEADSIZE; i++ )
					s->lookAhead[ i ] = *nextBytes++;
				s->length = 0L;
				s->buffer = s->bufferEnd;
				return( 0L );
			}
			if( len > LOOKAHEADSIZE )
			{
				PERR(("ACK!: MPAEndDMA len = %ld > LOOKAHEADSIZE\n",len));
				return(MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError));
			}
			/* copy hw buffered bytes to end of the lookAhead buffer */
			for( i = 0L; i < len; i++ )
			{
				s->lookAhead[ LOOKAHEADSIZE - len + i ] = nextBytes[ i ];
			}
			/* point next buffer at the right place */
			s->buffer = nextAddr;
			s->length = s->bufferEnd - nextAddr;

			/* read out the garbage at the front of the lookAhead buffer */
			count = LOOKAHEADSIZE - len;
			return MPRead(theUnit, s, temp, count);
	}
	return 0L;
}

/* dump data until we find a non-slice start code */

int32 MPDumpSlices(tVideoDeviceContext* theUnit, streamContext *s)
{
	int32 nextCode, readResult;

	while( 1 )
	{
		MPLook( s, (char *) &nextCode, 4L );
		if( (nextCode & 0xffffff00L) == START_CODE_PREFIX )
		{
			if( (nextCode <= SLICE_START_CODE_MIN) ||
				(nextCode >= SLICE_START_CODE_MAX) )
				break;
		}
		readResult = MPRead(theUnit, s, (char *) &nextCode, 1L);
		switch( readResult )
		{
            case 0:                     /* no pts */
				s->flags &= ~MPVPTSVALID;
               	break;
            case MPVPTSVALID:           /* pts valid */
               	s->flags |= MPVPTSVALID;
               	break;
            case MPVFLUSHWRITE:         /* complete current write */
				return( readResult );
            default:                    /* something weird happened */
               	continue;
		}
	}
	return( 0L );
}

#else /* test code */

/* the following test code is for debugging the mpeg parsers using
   the standard C libraries */

/* open the stream */
#define ERROR 0xffffffffL

int32 MPOpen( streamContext *streamID )
{
	FILE *streamPtr;
	
	if( (streamPtr = fopen( streamID->fileName, "rb" )) == (FILE *) NULL )
		return( ERROR );
	if( fread( streamID->lookAhead, sizeof( char ), 4, streamPtr ) == 4L )
		streamID->lookAheadValid = 1L;
	else
		streamID->lookAheadValid = 0L;
	
	streamID->offset = 0L;
	streamID->OSStreamID = (int32) streamPtr;
	
	return( 0L );
}

/* read length bytes into buffer pointed to by dest, advance read pointer */

int32 MPRead( streamContext *streamID, char *dest, int32 length )
{
	int32 i, readResult;
	
	if( !streamID->lookAheadValid )
		return( ERROR );
	
	streamID->offset += length;
	
	if( length < 5 )
	{
		for( i = 0L; i < length; i++ )
			dest[ i ] = streamID->lookAhead[ i ];
		for( i = 0L; i < 4L - length; i++ )
			streamID->lookAhead[ i ] = streamID->lookAhead[ i + length ];
		readResult = fread( &(streamID->lookAhead[ 4L - length ]),
							sizeof( char ),
							length,
							(FILE *) streamID->OSStreamID );
		if( readResult == length )
			streamID->lookAheadValid = 1L;
		else
			streamID->lookAheadValid = 0L;
		
		return( 0L );
	}
	for( i = 0L; i < 4L; i++ )
		dest[ i ] = streamID->lookAhead[ i ];
	readResult = fread( &(dest[ 4L ]),
						sizeof( char ),
						length - 4L,
						(FILE *) streamID->OSStreamID );
	if( readResult != length - 4L )
		return( ERROR );
	readResult = fread( streamID->lookAhead,
						sizeof( char ),
						4,
						(FILE *) streamID->OSStreamID );
	if( readResult == 4L )
		streamID->lookAheadValid = 1L;
	else
		streamID->lookAheadValid = 0L;

	return( 0L );		
}

/* read length bytes into dest buffer, DO NOT advance read pointer */

int32 MPLook( streamContext *streamID, char *dest, int32 length )
{
	int32 i;
	
	if( !streamID->lookAheadValid || (length > 4L) )
		return( ERROR );
	
	for( i = 0L; i < length; i++ )
		dest[ i ] = streamID->lookAhead[ i ];
		
	return( 0L );
}

/* dump data until we find a non-slice start code */

#define BUFFERSIZE 4096L
uint8 temp[ BUFFERSIZE ];

int32 MPDumpSlices( streamContext *streamID )
{
	int32 len, i, state;
	uint32 pos;
	
	state = 0L;
	
	/* seek to the slice start code */
	fseek( (FILE *) streamID->OSStreamID, -4, 1L );
		
	do
	{
		/* remember where we are */
		pos = ftell( (FILE *) streamID->OSStreamID );
		
		/* read a chunk */
		len = fread( temp,
					 sizeof( char ),
					 BUFFERSIZE,
					 (FILE *) streamID->OSStreamID );
		if( len < 0L )
			return( len );
			
		/* if the next start code is not a slice start code, */
		/* dump until the start code, then exit */
		for( i = 0; i < len; i++ )
		{
			/* search for start code: 0x0000 01xx */
			switch( state )
			{
				case 0:
				case 1:
					if( temp[ i ] == 0 )
						state++;
					else
						state = 0L;
					continue;
				case 2:
					if( temp[ i ] == 1 )
						state++;
					else if( temp[ i ] != 0 )
						state = 0L;
					continue;
				case 3:
					state = 0;
					if( ((SLICE_START_CODE_MIN & 0xffL) <= temp[ i ]) &&
						(temp[ i ] <= (SLICE_START_CODE_MAX & 0xffL)) )
						continue;
					else
					{
						/* found non-slice start code, dump buffer up */
						/* through the start code */
						fwrite( temp,
								sizeof( char ),
								i+1,
								streamID->slicesFile );
						
						/* seek to the start code in the input file */
						pos += i + 1;
						fseek( (FILE *) streamID->OSStreamID, pos - 4, 0L );
						len = fread( streamID->lookAhead,
									 sizeof( char ),
									 4,
									 (FILE *) streamID->OSStreamID );
						if( len == 4L )
							streamID->lookAheadValid = 1L;
						else
							streamID->lookAheadValid = 0L;
						return( 0L );
					}
					break;
			}
		}
		/* didn't find a non-slice start code, write out entire buffer */
		fwrite( temp, sizeof( char ), len, streamID->slicesFile );
	}
	while( len > 0L );
}

#endif

