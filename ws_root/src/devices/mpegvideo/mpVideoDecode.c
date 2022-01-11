/* @(#) mpVideoDecode.c 96/12/11 1.17 */
/*	file: mpVideoDecode.c
	M2 MPEG Video Decoder entry point
	3/23/95 George Mitsuoka
	The 3DO Company Copyright 1995 */

#include <string.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include "mpVideoDecode.h"
#include "mpVideoDriver.h"
#include "MPEGVideoParse.h"

#ifdef DEBUG_PRINT_DECODE
  #define DBGDECODE(a)		PRNT(a)
#else
  #define DBGDECODE(a)		(void)0
#endif

void mpVideoDecode(tVideoDeviceContext *theUnit)
{
	int32 parseResult, flags, pts, userData, result;
	MPEGStreamInfo streamInfo;

	flags = 0L; pts = 0L; userData = 0L;

	/* initialize streamInfo */
	memset( &streamInfo, 0, sizeof( MPEGStreamInfo ) );
	
	do 
	{
		result = MPOpen(theUnit, &(streamInfo.streamID));
	} while (result == MPVFLUSHDRIVER);
	
	if( result != 0 )
	{
		MPVCompleteRead(theUnit, result); /* tell the client about the error */
		PERR(("mpVideoDecode: MPOpen returns %lx!\n",result));
		goto abort;
	}
	
	while( 1 )
	{
		parseResult = MPVideoParse(theUnit, &streamInfo);

		/* check return value */
		switch( parseResult )
		{
			case MPVFLUSHWRITE:			/* complete the current write buffer */
				DBGDECODE(("mpVideoDecode: MPVideoParse returns MPVFLUSHWRITE\n"));
				MPVCompleteWrite(theUnit, 0, flags, pts, userData);

				do
				{
					result = MPOpen(theUnit, &(streamInfo.streamID));
					if( result == MPVFLUSHWRITE )
					{
						DBGDECODE(("mpVideoDecode: MPOpen returns MPVFLUSHWRITE\n"));
						MPVCompleteWrite(theUnit, 0, flags, pts, userData);
					}
					else if (result == MPVFLUSHDRIVER)
					{
						DBGDECODE(("mpvVideoDecode: MPOpen returns MPVFLUSHDRIVER\n"));
						theUnit->branchState = JUST_BRANCHED;
						ResetReferenceFrames(theUnit);
				    }
					else if (result == ABORTED)
					{
						DBGDECODE(("mpVideoDecode: MPOpen returns ABORTED\n"));
						goto abort;
					}
					else if( result )
						PERR(("mpVideoDecode: MPOpen returns %lx!\n",result));
				}
				while(result);
				break;

			default:					/* something weird happened */
				MPVCompleteRead(theUnit, parseResult); /* tell the client about the error */
				DBGDECODE(("mpVideoDecode: unexpected %ld result MPVideoParse; treating as FLUSHDRIVER\n", parseResult));
				/* fall into FLUSHDRIVER case */
				
			case MPVFLUSHDRIVER:
				DBGDECODE(("mpVideoDecode: GOT MPVFLUSHDRIVER.\n"));
				theUnit->branchState = JUST_BRANCHED;
				ResetReferenceFrames(theUnit);
				/* fall into FLUSHREAD case */
				
			case MPVFLUSHREAD:			/* complete current read buffer */
				DBGDECODE(("mpVideoDecode: MPVideoParse returns MPVFLUSHREAD\n"));

				do {
					result = MPOpen(theUnit, &(streamInfo.streamID));
					if( result == MPVFLUSHWRITE  )
					{
						DBGDECODE(("mpVideoDecode: MPOpen returns MPVFLUSHWRITE\n"));
						MPVCompleteWrite(theUnit, 0, flags, pts, userData);
					}
					else if (result == MPVFLUSHDRIVER) {
					   DBGDECODE(("mpvVideoDecode: MPOpen returns MPVFLUSHDRIVER\n"));
					   theUnit->branchState = JUST_BRANCHED;
					   ResetReferenceFrames(theUnit);
					}
					else if (result == ABORTED) 
					{
						DBGDECODE(("mpVideoDecode: MPOpen returns ABORTED\n"));
						goto abort;
					}
					else if( result )
						PERR(("mpVideoDecode: MPOpen returns %lx!\n",result));
				} while (result);
				break;
			case ABORTED:
				DBGDECODE(("mpVideoDecode: MPVideoParse returns ABORTED\n"));
				goto abort;
				break;
		}
	}
abort:
	DestroyVCDOptimize(streamInfo.vcdInfo);
	return;
}

void
ResetReferenceFrames(tVideoDeviceContext *theUnit)
{
	theUnit->refPicFlags[0] = REFPICEMPTY;
	theUnit->refPicFlags[1] = REFPICEMPTY;
	theUnit->nextRef = 0;
}

