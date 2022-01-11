/******************************************************************************
**
**  @(#) weaver.c 96/12/11 1.30
**
**	File:			Weaver.c
**	Contains:		data stream multiplexer tool
**
******************************************************************************/

/**
|||	AUTODOC -class Streaming_Tools -group Weaver -name Weaver
|||	Multiplexes chunkified streams into one playable stream.
|||
|||	  Format -preformatted
|||
|||	    Weaver [options] -o output_file < script_file
|||
|||	  Description
|||
|||	    The Weaver takes as input a script file that references one or
|||	    more chunked files and writes one stream that consists of
|||	    interleaved chunks from all input files. The Weaver script
|||	    specifies how the weaving is done. (See the chapter or on-line
|||	    help on the topic "Weaver Script Commands" which begins with
|||	    audioclockchan(@).)
|||
|||	    Weaver scripts consist of commands and comments. Commands start
|||	    with a Weaver command followed by command arguments. Comments
|||	    are lines starting with a #.
|||
|||	  Arguments
|||
|||	    -iobs <size>
|||	        Mac buffer size in bytes used by Weaver. Default: 32768
|||
|||	    -b <time>
|||	        Output stream start time (see settimebase(@) Weaver-script command).
|||	        May also be specified by the streamstarttime(@) Weaver-script
|||	        command.  Default: 0
|||
|||	    -s <size>
|||	        Output stream block size. May also be specified by the
|||	        streamblocksize(@) Weaver-script command.
|||
|||	    -m <size>
|||	        Output stream media block size. May also be specified by
|||	        the mediablocksize(@) Weaver-script command.
|||
|||	    -o <file>
|||	        Output stream file name.
|||
|||	    -mm <num>
|||	        Max markers to allow.
|||
|||	    -mct <time>
|||	        Maximum amount of time (see settimebase(@) Weaver-script command)
|||	        to pull a chunk back in time to use up block "FILL" space. May
|||	        also be specified by the maxearlychunktime(@) Weaver-script command.
|||	        Default: time required to load one stream buffer (see
|||	        streamblocksize(@) Weaver-script) from a quad speed CD.
|||
|||	  Caveats
|||
|||	    Make sure to quote file or folder names that contain spaces.
|||	    Use writestreamheader(@) in every Weaver script.
|||
|||	  Example -preformatted
|||
|||	    set WeaveScript ¶
|||	        'writestreamheader                          ¶n¶
|||	         maxearlychunktime  960                     ¶n¶
|||	         streamblocksize    32768                   ¶n¶
|||	         streambuffers      4                       ¶n¶
|||	         streamerdeltapri   -10                     ¶n¶
|||	         dataacqdeltapri    -9                      ¶n¶
|||	         subscriber         MPVD -2                 ¶n¶
|||	         subscriber         SNDS 11                 ¶n¶
|||	         preloadinstrument  SA_44K_16B_S_CBD2       ¶n¶
|||	         file SF.MPVD       1 0                     ¶n¶
|||	         file raggae.SNDS   0 24                    ¶n'
|||	    echo "{WeaveScript}" | Weaver -o SF.stream
**/


#define	PROGRAM_VERSION_STRING		"3.2"

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cursorctl.h>

#include "weaver.h"
#include "weavestream.h"

#ifndef __STREAMING_SATEMPLATEDEFS_H
	#include <streaming/satemplatedefs.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <streaming/dsstreamdefs.h>
#endif

#include "ParseScriptFile.h"

/*********************************************************************/
/* Parameters set by the ParseCommandLine() and/or ParseDirectives() */
/*********************************************************************/
#define		kStreamBlockSize		(32 * 1024L)
#define		kMediaBlockSize			(2 * 1024L)
#define		kOutputBaseTime			0
#define		kOutputStreamName		NULL
#define		kIOBufferSize			(32 * 1024L)

#define	kDfltMaxTimeBlockPulledBack		(0xDEADBEEF)	/* default to an unlikely value so we can tell the user doesn't set it */

bool		gVerboseFlag			= false;
long		gMaxMarkers				= 1024;

/* Stream header parameters set only by ParseDirectives() */
#define		kDfltTimeBase			AUDIO_TICKS_PER_SEC
#define		kWriteStreamHeader		false
#define		kWriteMarkerTable		false
#define		kWriteGotoChunk			false
#define		kWriteStopChunk			false
#define		kWriteHaltChunk			false
#define		kStreamBuffers			4		/* suggested number of stream buffers to use */
#define		kAudioClockChan			0
#define		kEnableAudioChan		1		/* enable audio channel zero */
#define		kStreamerDeltaPri		-10		/* streamer thread delta priority */
#define		kDataAcqDeltaPri		-9		/* data acquisition thread delta priority */
#define		kNumSubsMsgs			256

long		gNextPreloadInstIndex	= 0;
long		gNextSubscriberIndex	= 0;

/*******************************************/
/* Other globals, local routine prototypes */
/*******************************************/

WeaveParams	gWSParamBlk;			/* Param block used by WeaveStreams() */


static	void		CorrectTimesForTimebase(WeaveParams *pb);

static	bool	 	SetWriteMarkerTable(char *name, char *parms);
static	bool	 	SetWriteStreamHeader(char *name, char *parms);
static	bool		AddSplitType(WeaveParamsPtr pb, TypeSubType *chunkParams);
static	bool		AddPreloadInstrument(char *name, char *parms);
static	bool		AddSubscriber(char *name, char *parms);
static	bool		SetMediaBlockSize(char *name, char *parms);
static	bool		SetStreamBlockSize(char *name, char *parms);
static	bool		SetStreamStartTime(char *name, char *parms);
static	bool	 	AddMarker(char *name, char *parms);
static	bool		AddChunkFile(char *name, char *parms);
static	bool		SetNumStreamBuffers(char *name, char *parms);
static	bool		SetAudioClockChan(char *name, char *parms);
static	bool		SetEnableAudioChan(char *name, char *parms);
static	bool		SetStreamerDeltaPri(char *name, char *parms);
static	bool		SetDataAcqDeltaPri(char *name, char *parms);
static	bool		SetNumSubsMsgs(char *name, char *parms);
static	bool		AddStopChunk(char *name, char *parms);
static	bool		AddGotoChunk(char *name, char *parms);
static	bool		AddHaltChunk(char *name, char *parms);
static	bool		SetTimeBase(char *name, char *parms);
static	bool		SetMaxEarlyChunkTime(char *name, char *parms);


static CmdFunctionTable gCommandTable[] =
{
	"file",					(CommandFunc *)AddChunkFile,
	"mediablocksize",		(CommandFunc *)SetMediaBlockSize,
	"streamblocksize",		(CommandFunc *)SetStreamBlockSize,
	"streamstarttime",		(CommandFunc *)SetStreamStartTime,
	"markertime",			(CommandFunc *)AddMarker,
	"streambuffers",		(CommandFunc *)SetNumStreamBuffers,
	"audioclockchan",		(CommandFunc *)SetAudioClockChan,
	"enableaudiomask",		(CommandFunc *)SetEnableAudioChan,
	"preloadinstrument",	(CommandFunc *)AddPreloadInstrument,
	"subscriber",			(CommandFunc *)AddSubscriber,
	"streamerdeltapri",		(CommandFunc *)SetStreamerDeltaPri,
	"dataacqdeltapri",		(CommandFunc *)SetDataAcqDeltaPri,
	"writestreamheader",	(CommandFunc *)SetWriteStreamHeader,
	"writemarkertable",		(CommandFunc *)SetWriteMarkerTable,
	"numsubsmessages",		(CommandFunc *)SetNumSubsMsgs,
	"writestopchunk",		(CommandFunc *)AddStopChunk,
	"writegotochunk",		(CommandFunc *)AddGotoChunk,
	"writehaltchunk",		(CommandFunc *)AddHaltChunk,
	"settimebase",			(CommandFunc *)SetTimeBase,
	"maxearlychunktime",	(CommandFunc *)SetMaxEarlyChunkTime,

	NULL,					NULL
};

static bool			ParseCommandLine( int argc, char **argv );
static void			Usage( char* commandNamePtr );

static void			InsertStreamFiles( WeaveParamsPtr pb, InStreamPtr isp );
static void			InsertStrmChunks( StrmChunkNode **head, StrmChunkNodePtr nodePtr );


/*******************************************************************************************
 * Main tool entrypoint. This may be called from a THINK C application shell, or directly
 * as an MPW tool (the former is used for debugging purposes only).
 *******************************************************************************************/
long main( int argc, char* argv[] )
	{
	long				status;
	long				index;
	long				markerTableSize;
	InStreamPtr			isp;
	StrmChunkNodePtr	CPtr;
	TypeSubType			mpegSplitInfo = {MPVD_CHUNK_TYPE,
										MVDAT_CHUNK_SUBTYPE,
										MVDAT_SPLIT_START_CHUNK_SUBTYPE,
										MVDAT_SPLIT_END_CHUNK_SUBTYPE};


	/* initialize the global param table so we don't get old values when the tool
	 *  is run more than once without being flushed from memory
	 */
	memset(&gWSParamBlk, 0, sizeof(gWSParamBlk));

	/* Set up the parameter block for the weaving routine with default values */
	gWSParamBlk.mediaBlockSize		= kMediaBlockSize;
	gWSParamBlk.streamBlockSize		= kStreamBlockSize;
	gWSParamBlk.outputBaseTime		= kOutputBaseTime;
	gWSParamBlk.outputStreamName	= kOutputStreamName;
	gWSParamBlk.ioBufferSize		= kIOBufferSize;
	gWSParamBlk.streamBuffers		= kStreamBuffers;
	gWSParamBlk.audioClockChan		= kAudioClockChan;
	gWSParamBlk.enableAudioChan		= kEnableAudioChan;
	gWSParamBlk.streamerDeltaPri	= kStreamerDeltaPri;
	gWSParamBlk.dataAcqDeltaPri		= kDataAcqDeltaPri;
	gWSParamBlk.fWriteStreamHeader	= kWriteStreamHeader;
	gWSParamBlk.fWriteMarkerTable	= kWriteMarkerTable;
	gWSParamBlk.numSubsMsgs			= kNumSubsMsgs;
	gWSParamBlk.fWriteGotoChunk		= kWriteGotoChunk;
	gWSParamBlk.fWriteStopChunk		= kWriteStopChunk;
	gWSParamBlk.fWriteHaltChunk		= kWriteHaltChunk;
	gWSParamBlk.earlyChunkMaxTime	= kDfltMaxTimeBlockPulledBack;

	gWSParamBlk.timebase			= kDfltTimeBase;

	/* make a standard MPEG chunk splittable */
	AddSplitType(&gWSParamBlk, &mpegSplitInfo);


/*
//@@@@@  INCLUDE THIS TO TEST MULTIPLE SPLITTABLE CHUNKS.  THIS ALLOWS THE WEAVER TO
//@@@@@   SPLIT 'DATn' CHUNKS (THE STREAm WON'T ACTUALLY WORK WITH THE DATA SUBSCRIBER AS
//@@@@@   WRITTEN, BUT IT WOULDN'T BE HARD TO CHANGE SO IT DOES)
{
	#define DATA_SPLIT_START_CHUNK_SUBTYPE	MAKE_ID('D','A','T','[')
	#define DATA_SPLIT_END_CHUNK_SUBTYPE	MAKE_ID('D','A','T',']')

	TypeSubType			dataSplitInfo = {DATA_SUB_CHUNK_TYPE,
										DATA_NTH_CHUNK_TYPE,
										DATA_SPLIT_START_CHUNK_SUBTYPE,
										DATA_SPLIT_END_CHUNK_SUBTYPE};
	AddSplitType(&gWSParamBlk, &dataSplitInfo);

	// and while we're at it, make 'DAT1' chunks splittable too...
	dataSplitInfo.subType = DATA_FIRST_CHUNK_TYPE;
	AddSplitType(&gWSParamBlk, &dataSplitInfo);
}
//@@@@@
*/

	/* Collect command line switch values. If any error, output some
	 * helpful info and exit.
	 */
	if ( ! ParseCommandLine( argc, argv ) )
		{
		Usage(argv[0]);
		return EXIT_FAILURE;
		}

	/* So we can spin cursor */
	InitCursorCtl(NULL);

	/* Allocate the marker array. We fill this in with times that
	 * the user deems "interesting", and the weaver fills in the
	 * file positions associated with those times and takes care
	 * of "aligning" the data for branching to those times.
	 * We add one more slot to insert an 'infinity' marker so
	 * that marker comparison (in GetEarliestBelowNextMarker() and
	 * GetBestFitBelowNextMarker() ) will work properly. */
	markerTableSize = (gMaxMarkers + 1) * sizeof( MarkerRec );
	gWSParamBlk.marker = (MarkerRecPtr) malloc( markerTableSize );
	if ( NULL == gWSParamBlk.marker )
		{
		fprintf( stderr, "### Error allocating marker table\n" );
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	/* Initialize the marker table to all ones */
	memset( gWSParamBlk.marker, 0xff, markerTableSize );

	/* Initialize a couple of param block fields */
	gWSParamBlk.numInputStreams		= 0;
	gWSParamBlk.numMarkers			= 0;
	gWSParamBlk.maxMarkers			= gMaxMarkers;

	/* Create a marker for the beginning of time */
	gWSParamBlk.marker[ gWSParamBlk.numMarkers++ ].markerTime = 0;

	/* Clear the instrument preload table */
	for ( index = 0; index < DS_HDR_MAX_PRELOADINST; index++ )
		gWSParamBlk.preloadInstList[ index ] = 0;

	/* Clear the subscriber tag table */
	for ( index = 0; index < DS_HDR_MAX_SUBSCRIBER; index++ )
		{
		gWSParamBlk.subscriberList[ index ].subscriberType = 0;
		gWSParamBlk.subscriberList[ index ].deltaPriority = 0;
		}

	/* Read and parse the input directives file. This will set up the
	 * parameter block we'll pass to WeaveStreams() below.  */
	if ( false == ParseScriptFile(stdin, (CmdFunctionTablePtr)&gCommandTable) )
	{
		status = EXIT_FAILURE;
		goto BAILOUT;
	}

	/* make sure that at least one file name was specified */
	if ( NULL == gWSParamBlk.inStreamList )
	{
		fprintf( stderr, "### Error: at least one chunk file must be specified\n" );
		status = EXIT_FAILURE;
		goto BAILOUT;
	}

	/* Verify that the stream block size is a multiple of the media block size.
	 * This is to insure that we can do integral block I/O to the device where
	 * the stream data will ultimately original from. */
	if ( (gWSParamBlk.streamBlockSize % gWSParamBlk.mediaBlockSize) != 0 )
		{
		fprintf(stderr, "### Error: stream block size not exact multiple of media block size\n");
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	SpinCursor(32);

	/* if the user did not specify a max early chunk time, calculate one now equal
	 *  to about 1 block size @ 4x CD rate
	 */
	#define	kCDReadBytesPerSec	(600 * 1024)
	if ( kDfltMaxTimeBlockPulledBack == gWSParamBlk.earlyChunkMaxTime )
	{
		gWSParamBlk.earlyChunkMaxTime = (gWSParamBlk.streamBlockSize * AUDIO_TICKS_PER_SEC)
											/ kCDReadBytesPerSec;
	}
	#undef	kCDReadBytesPerSec

	/* correct marker times for whatever the user wants to use as a timebase */
	CorrectTimesForTimebase(&gWSParamBlk);

	if ( ! OpenDataStreams( &gWSParamBlk ) )
		{
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	/* Call the weaving routine. The result is one output data stream
	 * file, sorted by time order. */
	status = WeaveStreams( &gWSParamBlk );
	if ( status != 0 )
		status = EXIT_FAILURE;

BAILOUT:
	CloseDataStreams( &gWSParamBlk );

	/* Delete the InStream link list */
	for ( ; NULL != gWSParamBlk.inStreamList; )
		{
		isp							= gWSParamBlk.inStreamList;
		gWSParamBlk.inStreamList	= gWSParamBlk.inStreamList->next;
		free ( isp );
		}

	/* Delete any left over Goto chunks in the link list */
	for ( ; NULL != gWSParamBlk.gotoChunkList; )
		{
		CPtr						= gWSParamBlk.gotoChunkList;
		gWSParamBlk.gotoChunkList	= gWSParamBlk.gotoChunkList->next;
		free ( CPtr );
		}

	/* Delete any left over Stop chunks in the link list */
	for ( ; NULL != gWSParamBlk.stopChunkList; )
		{
		CPtr 						= gWSParamBlk.stopChunkList;
		gWSParamBlk.stopChunkList	= gWSParamBlk.stopChunkList->next;
		free ( CPtr );
		}

	/* Delete any left over Halt chunks in the link list */
	for ( ; NULL != gWSParamBlk.haltChunkList; )
		{
		HaltChunksPtr				HPtr;

		HPtr 						= gWSParamBlk.haltChunkList;
		gWSParamBlk.haltChunkList	= gWSParamBlk.haltChunkList->next;
		free ( HPtr );
		}

	/* toss the splittable chunk list */
	if ( NULL != gWSParamBlk.splitTypes )
		free(gWSParamBlk.splitTypes);

	return status;
	}



/*
 * the 3DO audio clock has 239.674 ticks per second, but developers have often
 *  incorrectly used 240 ticks per second as the basis for times in weave scripts.
 *  This works fine for short streams, but causes problems with longer scripts
 *  because of accululated error.  Thus the script file command "settimebase"
 *  allows the user to set the timebase for a script file to whatever they want
 *  and we correct for the actual machine time base before weaving the stream.
 *  this is where we fix things up
 */
static void
CorrectTimesForTimebase(WeaveParams *pb)
{
	float				timebaseMultiplier = AUDIO_TICKS_PER_SEC / gWSParamBlk.timebase;
	InStreamPtr			isp;
	StrmChunkNodePtr	chnkPtr;
	HaltChunksPtr		haltPtr;
	uint16				ndx;

	/* if time is already expressed in the machine's time base, don't bother */
	if ( 1 == timebaseMultiplier )
		goto DONE;

	/* correct all marker times */
	for ( ndx = 0; ndx < pb->numMarkers; ndx++ )
		pb->marker[ndx].markerTime *= timebaseMultiplier;

	/* stream start time and early chunk max time */
	pb->outputBaseTime		*= timebaseMultiplier;
	pb->earlyChunkMaxTime	*= timebaseMultiplier;

	/* the start time of all of the input streams */
	for ( isp = pb->inStreamList; isp != NULL; isp = isp->next )
		isp->startTime *= timebaseMultiplier;

	/* time stamp of all of goto chunks */
	for ( chnkPtr = pb->gotoChunkList; chnkPtr != NULL; chnkPtr = chnkPtr->next )
		chnkPtr->chunk.gotoChk.time *= timebaseMultiplier;

	/* all of stop chunks */
	for ( chnkPtr = pb->stopChunkList; chnkPtr != NULL; chnkPtr = chnkPtr->next )
		chnkPtr->chunk.stopChk.time *= timebaseMultiplier;

	/* and all of halt chunks */
	for ( haltPtr = pb->haltChunkList; haltPtr != NULL; haltPtr = haltPtr->next )
		haltPtr->dataChunk.streamerChunk.time *= timebaseMultiplier;

DONE:
	return;
}


/*
 * Add a new type/subtype pair to the list of splittable chunk types
 */
static bool
AddSplitType(WeaveParamsPtr pb, TypeSubType *chunkParams)
{
	int32			count;
	TypeSubType		*newTypeList;
	bool			success = false;

	/* bump the length by enough to hold the new type/subtype */
	count = ++pb->splitTypeCount;
	if ( NULL == (newTypeList = (TypeSubType *)calloc(count * sizeof(TypeSubType), 1)) )
	{
		fprintf(stderr, "AddSplitType() - out of memory!\n");
		goto DONE;
	}

	/* copy the old list, add the new elements and we're done */
	newTypeList[--count] = *chunkParams;
	while ( --count >= 0 )
		newTypeList[count] = pb->splitTypes[count];
	if ( NULL != pb->splitTypes )
		free((char *)pb->splitTypes);
	pb->splitTypes = newTypeList;
	success = true;

DONE:
	return success;
}


/*
 * SetWriteMarkerTable
 */
bool
SetWriteMarkerTable(char *name, char *parms)
{
	TOUCH(name);
	TOUCH(parms);

	gWSParamBlk.fWriteMarkerTable = true;

	return true;
}

/*
 * SetWriteStreamHeader
 */
bool
SetWriteStreamHeader(char *name, char *parms)
{
	TOUCH(name);
	TOUCH(parms);
	gWSParamBlk.fWriteStreamHeader = true;

	return true;
}

/*
 * AddPreloadInstrument
 *	set the game difficulty level
 */
bool
AddPreloadInstrument(char *name, char *parms)
{
	uint32 	instrumentTag;
	char	*tempStr;
	TokenValue	instrumentTokens[] =
		{
			{"sa_44k_16b_s",		SA_44K_16B_S },
			{"sa_44k_16b_m",		SA_44K_16B_M },
			{"sa_44k_8b_s",			SA_44K_8B_S },
			{"sa_44k_8b_m",			SA_44K_8B_M },
			{"sa_22k_16b_s",		SA_22K_16B_S },
			{"sa_22k_16b_m",		SA_22K_16B_M },
			{"sa_22k_8b_s",			SA_22K_8B_S },
			{"sa_22k_8b_m",			SA_22K_8B_M },
/* for opera compatibility only. Do not use SDX2 when weaving M2 streams. */
			{"sa_44k_16b_s_sdx2",	SA_44K_16B_S_SDX2 },
			{"sa_44k_16b_m_sdx2",	SA_44K_16B_M_SDX2 },
			{"sa_22k_16b_s_sdx2",	SA_22K_16B_S_SDX2 },
			{"sa_22k_16b_m_sdx2",	SA_22K_16B_M_SDX2 },
			{"sa_44k_16b_s_cbd2",	SA_44K_16B_S_CBD2 },
			{"sa_44k_16b_m_cbd2",	SA_44K_16B_M_CBD2 },
			{"sa_22k_16b_s_cbd2",	SA_22K_16B_S_CBD2 },
			{"sa_22k_16b_m_cbd2",	SA_22K_16B_M_CBD2 },
			{"sa_44k_16b_m_sqs2",	SA_44K_16B_M_SQS2 },
			{"sa_22k_16b_m_sqs2",	SA_22K_16B_M_SQS2 },
			{"sa_44k_16b_m_adp4",	SA_44K_16B_M_ADP4 },
			{"sa_22k_16b_m_adp4",	SA_22K_16B_M_ADP4 },
			{NULL,					0}
		};



	(void)name;
	/* worry about overflowing the table */
	if ( gNextPreloadInstIndex >= DS_HDR_MAX_PRELOADINST )
	{
		Parse_ScriptError("maximum number of preload instruments is: %ld", DS_HDR_MAX_PRELOADINST);
		goto ERROR_EXIT;
	}

	if ( false == Parse_EvalString((long*)&instrumentTag, &parms, &tempStr, (TokenValuePtr)&instrumentTokens) )
	{
		Parse_ScriptError("unknown instrument tag: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}

	/* found a valid instrument name string, remember it in the next table slot */
	gWSParamBlk.preloadInstList[gNextPreloadInstIndex] = instrumentTag;
	++gNextPreloadInstIndex;

	gWSParamBlk.fWriteStreamHeader = true;
	return true;

ERROR_EXIT:
	return false;
}


/*
 * AddSubscriber
 *	set the game difficulty level
 */
bool
AddSubscriber(char *name, char *parms)
{
	long 	subscriberTag = 0;
	long	tempInt;
	char	*tempStr;
	char	*tempParms;

	(void)name;
	/* worry about overflowing the table */
	if ( gNextSubscriberIndex >= DS_HDR_MAX_SUBSCRIBER )
	{
		Parse_ScriptError("maximum number of subscribers is: %ld", DS_HDR_MAX_SUBSCRIBER);
		goto ERROR_EXIT;
	}

	/* Get next token */
	tempParms = Parse_NextToken(&parms);

	/* Copy up to 4 characters of subscriber tag string. */
	/* Space fill the tag if there are less than 4 characters specified. */
	for ( tempInt = 0; tempInt < 4; tempInt++ )
		if ( *tempParms != 0 )
			subscriberTag = (subscriberTag << 8) | *tempParms++;
		else
			subscriberTag = (subscriberTag << 8) | ' ';

	/* found a valid subscriber name string, remember it in the next table slot */
	gWSParamBlk.subscriberList[gNextSubscriberIndex].subscriberType = subscriberTag;

	if ( false == Parse_GetInt(&tempInt, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown file priority: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}
	gWSParamBlk.subscriberList[gNextSubscriberIndex].deltaPriority = tempInt;

	++gNextSubscriberIndex;

	gWSParamBlk.fWriteStreamHeader = true;
	return true;
ERROR_EXIT:
	return false;
}



/*
 * SetMediaBlockSize
 */
bool
SetMediaBlockSize(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.mediaBlockSize, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown MediaBlockSize: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetStreamBlockSize
 */
bool
SetStreamBlockSize(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt((int32 *)&gWSParamBlk.streamBlockSize, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown StreamBlockSize: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetStreamStartTime
 */
bool
SetStreamStartTime(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt((int32 *)&gWSParamBlk.outputBaseTime, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown StreamStartTime: \"%s\"", tempStr);
		return false;
	}
	else
		return true;
}

/*
 * AddMarker
 */
bool
AddMarker(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( gWSParamBlk.numMarkers >= gWSParamBlk.maxMarkers )
	{
		Parse_ScriptError("overflowed marker table (max = %ld).  Use the \"%s\" command"
							" line option to increase table size", gWSParamBlk.maxMarkers,
							MAX_MARKERS_FLAG_STRING);
		goto ERROR_EXIT;
	}

	if ( false == Parse_GetInt((long *)&gWSParamBlk.marker[gWSParamBlk.numMarkers].markerTime, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown marker time: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}

	++gWSParamBlk.numMarkers;

	return true;
ERROR_EXIT:
	return false;
}

/*
 * AddChunkFile
 *	Allocate and fill in a new inStreamList object
 */
bool
AddChunkFile(char *name, char *parms)
{
	char		*tempStr;
	InStreamPtr	isp;

	(void)name;
	if ( NULL == (tempStr = Parse_NextToken(&parms)) )
	{
		Parse_ScriptError("unknown file name");
		goto ERROR_EXIT;
	}

	/* Allocate a new inStreamList object */
	if ( NULL == (isp = (InStreamPtr)malloc(sizeof(InStream))) )
	{
		Parse_ScriptError("unable to alloc memory for InStream object");
		goto ERROR_EXIT;
	}

	/* clear out any lingering garbage from the InStream descriptor */
	memset(isp, 0, sizeof (InStream));

	/* fill in the entry, clear the eof flag */
	isp->next	= NULL;
	isp->eof 	= false;

	/* allocate a place to store the file name */
	isp->fileName = (char*) malloc(strlen(tempStr) + 1);
	strcpy(isp->fileName, tempStr);

	if ( false == Parse_GetInt(&isp->priority, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown file priority: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}

	if ( false == Parse_GetInt((long *)&isp->startTime, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown file start time: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}

	/* and finally insert the file name into the global block */
	InsertStreamFiles(&gWSParamBlk, isp);

	return true;

ERROR_EXIT:
	return false;
}


/*
 * SetNumStreamBuffers
 */
bool
SetNumStreamBuffers(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.streamBuffers, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown StreamBuffers: \"%s\"", tempStr);
		return false;
	}
	else
	{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
	}
}

/*
 * SetAudioClockChan
 */
bool
SetAudioClockChan(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.audioClockChan, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown AudioClockChan: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetEnableAudioChan
 */
bool
SetEnableAudioChan(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.enableAudioChan, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown EnableAudioChan: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetStreamerDeltaPri
 */
bool
SetStreamerDeltaPri(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.streamerDeltaPri, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown StreamerDeltaPri: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetDataAcqDeltaPri
 */
bool
SetDataAcqDeltaPri(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.dataAcqDeltaPri, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown DataAcqDeltaPri: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}

/*
 * SetNumSubsMsgs
 */
bool
SetNumSubsMsgs(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.numSubsMsgs, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown NumSubsMsgs: \"%s\"", tempStr);
		return false;
	}
	else
		{
		gWSParamBlk.fWriteStreamHeader = true;
		return true;
		}
}


/*
 * SetMaxEarlyChunkTime
 */
bool
SetMaxEarlyChunkTime(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetInt(&gWSParamBlk.earlyChunkMaxTime, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown MaxEarlyChunkTime: \"%s\"", tempStr);
		return false;
	}

	return true;
}

/*
 * SetTimeBase
 */
bool
SetTimeBase(char *name, char *parms)
{
	char	*tempStr;

	(void)name;
	if ( false == Parse_GetFloat(&gWSParamBlk.timebase, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown time base value: \"%s\"", tempStr);
		return false;
	}
	/* make sure that the value makes sense */
	if ( gWSParamBlk.timebase <= 0 )
	{
		Parse_ScriptError("illegal time base value (%f).  Value must be greater than 0.", gWSParamBlk.timebase);
		return false;
	}

	return true;
}


/*
 * AddStopChunk
 */
bool
AddStopChunk(char *name, char *parms)
{
	char			*tempStr;
	StrmChunkNodePtr	stopChunkNodePtr;

	(void)name;
	if ( NULL == (stopChunkNodePtr = (StrmChunkNodePtr)malloc(sizeof(StrmChunkNode))) )
	{
		Parse_ScriptError("unable to allocate memory for Stop Chunk object");
		goto ERROR_EXIT;
	}

	/* Fill in the entry */
	stopChunkNodePtr->chunk.stopChk.chunkType		= STREAM_CHUNK_TYPE;
	stopChunkNodePtr->chunk.stopChk.subChunkType	= STOP_CHUNK_SUBTYPE;
	stopChunkNodePtr->chunk.stopChk.chunkSize		= sizeof(StreamStopChunk);
	stopChunkNodePtr->chunk.stopChk.channel			= 0;
	stopChunkNodePtr->chunk.stopChk.options			= 0;
	stopChunkNodePtr->next							= NULL;

	if ( false == Parse_GetInt((long *)&stopChunkNodePtr->chunk.stopChk.time, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown StopChunk time: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}
	InsertStrmChunks( &gWSParamBlk.stopChunkList, stopChunkNodePtr );
	gWSParamBlk.fWriteStopChunk	= true;

	return true;
ERROR_EXIT:
	return false;
}

/*
 * AddGotoChunk
 */
bool
AddGotoChunk(char *name, char *parms)
{
	char				*tempStr;
	long				temp;
	StrmChunkNodePtr	gotoChunkNodePtr;

	(void)name;
	if ( NULL == (gotoChunkNodePtr = (StrmChunkNodePtr)malloc(sizeof(StrmChunkNode))) )
	{
		Parse_ScriptError("unable to allocate memory for Goto chunk object");
		goto ERROR_EXIT;
	}

	/* Fill in the entry */
	gotoChunkNodePtr->chunk.gotoChk.chunkType		= STREAM_CHUNK_TYPE;
	gotoChunkNodePtr->chunk.gotoChk.subChunkType	= GOTO_CHUNK_SUBTYPE;
	gotoChunkNodePtr->chunk.gotoChk.chunkSize		= sizeof(StreamGoToChunk);
	gotoChunkNodePtr->chunk.gotoChk.channel			= 0;
	gotoChunkNodePtr->next							= NULL;

	if ( false == Parse_GetInt((long *)&gotoChunkNodePtr->chunk.gotoChk.time, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown writegotochunk time argument: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}
	if ( false == Parse_GetInt((long *)&gotoChunkNodePtr->chunk.gotoChk.options, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown writegotochunk options argument: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}
	if ( false == Parse_GetInt(&temp, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown writegotochunk value argument: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}
	if ( GOTO_OPTIONS_MARKER == gotoChunkNodePtr->chunk.gotoChk.options )
		gotoChunkNodePtr->chunk.gotoChk.dest1 = temp;

	else if ( (GOTO_OPTIONS_ABSOLUTE || GOTO_OPTIONS_PROGRAMMED) ==
				gotoChunkNodePtr->chunk.gotoChk.options )
	{
		Parse_ScriptError("writegotochunk options argument not yet supported: \"%ld\"",
			gotoChunkNodePtr->chunk.gotoChk.options);
		goto ERROR_EXIT;
	}
	else
	{
		Parse_ScriptError("unknown writegotochunk options argument: \"%ld\"",
			gotoChunkNodePtr->chunk.gotoChk.options);
		goto ERROR_EXIT;
	}

	InsertStrmChunks( &gWSParamBlk.gotoChunkList, gotoChunkNodePtr );
	gWSParamBlk.fWriteGotoChunk	= true;

	return true;
ERROR_EXIT:
	return false;
}

/*
 * AddHaltChunk
 *
 * NOTE: This routine presently non-functional.  See #if 0 code.  The routine
 * 		 InsertCtrlChunks must be updated to new routine InsertStrmChunks.
 *
 */
bool
AddHaltChunk(char *name, char *parms)
{
	char			*tempStr;
	long			tempInt;
	uint32		 	subscriberTag = 0;
	HaltChunksPtr	haltChunksPtr;

	(void)name;

	/* let the user know that we don't do HALT chunks at this time */
	Parse_ScriptError("HALT chunks not supported by this version of the weaver");

	if ( NULL == (haltChunksPtr = (HaltChunksPtr)malloc(sizeof(HaltChunk))) )
	{
		Parse_ScriptError("unable to allocate memory for Halt chunk object.");
		goto ERROR_EXIT;
	}

	/* Fill in the entry */
	haltChunksPtr->dataChunk.streamerChunk.chunkType	= STREAM_CHUNK_TYPE;
	haltChunksPtr->dataChunk.streamerChunk.subChunkType	= HALT_CHUNK_SUBTYPE;
	haltChunksPtr->dataChunk.streamerChunk.chunkSize	= sizeof(HaltChunk);
	haltChunksPtr->dataChunk.streamerChunk.channel		= 0;

	if ( false == Parse_GetInt((long *)&haltChunksPtr->dataChunk.streamerChunk.time, &parms, &tempStr) )
	{
		Parse_ScriptError("unknown Halt chunk time: \"%s\"", tempStr);
		goto ERROR_EXIT;
	}

	/* Copy up to 4 characters of subscriber tag string. */
	/* Space fill the tag if there are less than 4 characters specified. */
	if ( NULL == (tempStr = Parse_NextToken(&parms)) )
	{
		Parse_ScriptError("missing subscriber tag");
		goto ERROR_EXIT;
	}
	for ( tempInt = 0; tempInt < 4; tempInt++ )
		if ( *tempStr != 0 )
			subscriberTag = (subscriberTag << 8) | *tempStr++;
		else
			subscriberTag = (subscriberTag << 8) | ' ';

	/* The halt chunk consists of a chunk within a chunk. */
	/* This is the contained chunk, and we now know its type */
	/* This is the actual chunk that gets sent to the subscriber */
	/* right before the datastreamer halts. */
	haltChunksPtr->dataChunk.subscriberChunk.chunkType	= subscriberTag;
	haltChunksPtr->dataChunk.subscriberChunk.subChunkType	= HALT_CHUNK_SUBTYPE;
	haltChunksPtr->dataChunk.subscriberChunk.chunkSize	= sizeof(SimpleChunk);
	haltChunksPtr->dataChunk.subscriberChunk.channel	= 0;
	haltChunksPtr->dataChunk.subscriberChunk.time 		= haltChunksPtr->dataChunk.streamerChunk.time;
	haltChunksPtr->next									= NULL;
#if 0
	InsertCtrlChunks((CtrlChunks**) &gWSParamBlk.haltChunkList,
						(CtrlChunksPtr)haltChunksPtr);
	gWSParamBlk.fWriteHaltChunk	= true;
#endif
	return true;
ERROR_EXIT:
	return false;
}

/*******************************************************************************************
 * Command line parser. Accepts the usual -x unix style switches. Puts parsing
 * results into application global variables which can be pre-initialized to
 * default values. Returns TRUE if command line parsed OK, FALSE if any error.
 *******************************************************************************************/
static bool ParseCommandLine ( int argc, char **argv )
	{
	bool		success = false;

	argv++;
	argc--;

	/* Check if there's any commands to parse. */
	if ( argc == 0 )
		return false;

	while ( argc > 0 )
		{
		/* Check for the flag that specifies the base (start) time
		 * value for the output stream. */
		if ( strcmp( *argv, BASETIME_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;
			sscanf( *argv++, "%ld", &gWSParamBlk.outputBaseTime);
			}

		/* Check for the flag that specifies the physical media
		 * data block size. For CDROM, this will be 2048. */
		else if ( strcmp( *argv, MEDIABLK_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;
			sscanf( *argv++, "%ld", &gWSParamBlk.mediaBlockSize);
			}

		/* Check for the flag that specifies the stream data
		 * block size. Default for this is 32k bytes. */
		else if ( strcmp( *argv, STREAMBLK_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;
			sscanf( *argv++, "%ld", &gWSParamBlk.streamBlockSize);
			}


		/* Check for the flag that specifies the stream
		 * I/O buffer size. Default is 32k bytes. */
		else if ( strcmp( *argv, IOBUFSIZE_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;
			sscanf( *argv++, "%ld", &gWSParamBlk.ioBufferSize);
			}

		/* Check for the flag that specifies the output data file name */
		else if ( strcmp( *argv, OUTDATA_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;

			gWSParamBlk.outputStreamName = *argv++;	/* save pointer to file name */
			}

		/* Check for the flag that specifies the max number of markers to allow */
		else if ( strcmp( *argv, MAX_MARKERS_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;
			sscanf( *argv++, "%ld", &gMaxMarkers);

			/* Verify the max marker number.  Must be greater than 1 and less than 32,766. */
			if ( ( gMaxMarkers < 1 ) || ( gMaxMarkers > 32766 ) )
				{
				fprintf ( stderr, "Invalid maximum number of markers -> %ld\n", gMaxMarkers );
				fprintf ( stderr, "Maximum number of markers must be between 1 and 32,766\n" );
				goto DONE;
				}
			}

		/* Check for the flag that specifies max time to pull a chunk back in time */
		else if ( strcmp( *argv, MAX_EARLY_TIME_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				goto DONE;

			sscanf( *argv++, "%ld", &gWSParamBlk.earlyChunkMaxTime);
			}

		/* Check for the flag that specifies verbose output */
		else if ( strcmp( *argv, VERBOSE_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gVerboseFlag = ~gVerboseFlag;
			}

		/* Unknown flag encountered */
		else
			goto DONE;
		}

		success = true;
DONE:

	return success;
	}


/*******************************************************************************************
 * Routine to insert the input file descriptors into ascending order based
 * upon their user specified priorities.
 *******************************************************************************************/
static void		InsertStreamFiles( WeaveParamsPtr pb, InStreamPtr isp )
	{

	InStreamPtr	Ptr1 = pb->inStreamList;
	InStreamPtr	Ptr2;

	/* Check to see if the list is empty. */
	if ( NULL == Ptr1 )
		/* Insert the first entry. */
		 pb->inStreamList = isp;

	/* Is the entry to be inserted at the front? */
	else if ( Ptr1->priority > isp->priority )
	{
		isp->next			= Ptr1;
		pb->inStreamList	= isp;
	} /* if insert at the front */

	/* Insertion is after the first entry. */
	else
	{
		Ptr2 = Ptr1;
		Ptr1 = Ptr1->next;

		/* Traverse the list and find a place to
		 * insert in ascending priority order. */
		while ( (NULL != Ptr1 ) &&
				( Ptr1->priority < isp->priority ) )
		{
			Ptr2 = Ptr1;
			Ptr1 = Ptr1->next;
		} /* end while */

		if ( NULL == Ptr1 )	/* Insert at the end of the list. */
			Ptr2->next = isp;
		else
		{
			/* Insert between Ptr1 and Ptr2. */
			Ptr2->next	= isp;
			isp->next	= Ptr1;
		}
	}
}

/*******************************************************************************************
 * Routine to insert the stream control chunks into ascending order based
 * upon their user specified streamtime.
 *******************************************************************************************/
static void		InsertStrmChunks( StrmChunkNode **head, StrmChunkNodePtr nodePtr )
	{

	StrmChunkNodePtr	ptr1 = *head;
	StrmChunkNodePtr	ptr2;

	/* Check to see if the list is empty. */
	if ( NULL == ptr1 )
		/* Insert the first entry. */
		 *head = nodePtr;

	/* Is the entry to be inserted at the front? */
	/*   (NB: chunk.stopChk.time works for any stream control   */
	/*   chunk type, due to StrmChunkNodePtr's embedded union.  */

	else if ( ptr1->chunk.stopChk.time > nodePtr->chunk.stopChk.time )
			{
			nodePtr->next	= ptr1;
			*head			= nodePtr;
			} /* if insert at the front */

	/* Insertion is after the first entry. */
	else
		{

		ptr2 = ptr1;
		ptr1 = ptr1->next;

		/* Traverse the list and find a place to
		 * insert in ascending time order. */
		while ( (NULL != ptr1) &&
				( ptr1->chunk.stopChk.time < nodePtr->chunk.stopChk.time ) )
			{
			ptr2 = ptr1;
			ptr1 = ptr1->next;
			} /* end while */

		if ( NULL == ptr1 )
			/* Insert at the end of the list. */
			ptr2->next = nodePtr;

		else
			{
			/* Insert between ptr1 and ptr2. */
			ptr2->next	= nodePtr;
			nodePtr->next	= ptr1;

			} /* end if-else ptr1 = NULL */

		} /* end if-else list empty */

	} /* end InsertStrmChunks */


/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void Usage( char* commandNamePtr )
	{
	fprintf( stderr, "%s version %s\n", commandNamePtr, PROGRAM_VERSION_STRING);
	fprintf( stderr, "usage: %s flags < <scriptfile> \n", commandNamePtr);
	fprintf( stderr, "  %-6s <size>  I/O buffer size (0<size<65536, default = 32,768)\n", IOBUFSIZE_FLAG_STRING );
	fprintf( stderr, "  %-6s <time>  output stream start time\n", BASETIME_FLAG_STRING );
	fprintf( stderr, "  %-6s <size>  output stream block size\n", STREAMBLK_FLAG_STRING );
	fprintf( stderr, "  %-6s <size>  output stream media block size\n", MEDIABLK_FLAG_STRING );
	fprintf( stderr, "  %-6s <file>  output stream file name\n", OUTDATA_FLAG_STRING );
	fprintf( stderr, "  %-6s <num>   max markers to allow(1-32,767)\n", MAX_MARKERS_FLAG_STRING );
	fprintf(stderr,  "  %-6s <num>   max time to pull chunks back in time\n", MAX_EARLY_TIME_FLAG_STRING);
	fprintf(stderr,  "              to use block filler space (default = approx. one stream block)\n");
	fprintf( stderr, "  %-6s         verbose diagnostic output\n", VERBOSE_FLAG_STRING );
	}

