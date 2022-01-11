/******************************************************************************
**
**  @(#) subscribertraceutils.c 96/08/01 1.2
**
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/debug.h>
#include <audio/audio.h>
#include <file/fileio.h>
#include <file/filesystem.h>		/* FILE_ERR_NOFILE */
#include <streaming/subscribertraceutils.h>


/*****************************************************************************
 * Local constants and structures
 *****************************************************************************/

#define STRBUF_SIZE			128


/*****************************************************************************
 * Variables
 *****************************************************************************/

	/* This switch controls whether the trace buffer will wrap-around
	 * or stop when it reaches the end. You can set it via the debugger. */
bool	traceWrap = TRUE;


/*****************************************************************************
 * Trace Logging
 *****************************************************************************/

void	AddTrace(TraceBufferPtr traceBuffer, int32 event,
					int32 channel, int32 value, void* ptr)
	{
	TraceEntryPtr entryPtr;		/* pointer to current trace entry

	/* Sanity check on index into trace buffer, so we don't stomp memory */
	if ( traceBuffer->index < MAX_TRACE )
		{
		/* Point to the current trace entry */
		entryPtr = traceBuffer->evList + traceBuffer->index;

		/* Make the entry */
		entryPtr->when = (int32)GetAudioTime();
		entryPtr->what = event;
		entryPtr->channel = channel;
		entryPtr->aValue = value;
		entryPtr->aPtr = ptr;
		}

	/* Increment search and wrap around circular buffer if appropriate */
	if ( ++traceBuffer->index >= MAX_TRACE && traceWrap )
		traceBuffer->index = 0;
	}


/*****************************************************************************
 * Trace dump file I/O routines
 *****************************************************************************/

#define USE_FOPEN			0		/* no dice */
#define USE_RAW_FILE		1

#if USE_FOPEN

	typedef FILE		DumpFile;

	/* Open a file to dump the trace log into.
	 * RETURNS: an error code; also returns the DumpFile* in *file. */
	static Err OpenDumpFile(DumpFile **file, const char *filename)
		{
		*file = fopen(filename, "w");
		if ( *file == NULL )
			{
			APERR(("Can't fopen a trace log file \"%s\"\n", filename));
			return FILE_ERR_NOFILE;
			}

		APRNT(("Writing trace log to the file \"%s\"\n", filename));
		return 0;
		}

	/* Dump a string to the dump file.
	 * NOTE: This ignores any error code from WriteRawFile. But if there is one,
	 *    CloseDumpFile will return it. */
	static void DumpString(const char *string, DumpFile *dumpFile)
		{
		fputs(string, dumpFile);
		}

	/* Close a dump file. */
	static Err CloseDumpFile(DumpFile *dumpFile)
		{
		return fclose(dumpFile);
		}

#elif USE_RAW_FILE

	typedef RawFile		DumpFile;

	/* Open a file to dump the trace log into.
	 * RETURNS: an error code; also returns the DumpFile* in *file. */
	static Err OpenDumpFile(DumpFile **file, const char *filename)
		{
		Err				result;
		const char*		directoryName = "/remote/";
		char			str[STRBUF_SIZE];

		str[0] = '\0';
		strcat(str, directoryName);
		strncat(str, filename, STRBUF_SIZE - 1 - strlen(str));

		result = OpenRawFile(file, str, FILEOPEN_WRITE_NEW);

		if ( result < 0 )
			{
			APERR(("Can't OpenRawFile a trace log file \"%s\":", filename));
			PrintfSysErr(result);
			}
		else
			APRNT(("Writing trace log to the file \"%s\"\n", str));
		return result;
		}

	/* Dump a string to the dump file.
	 * NOTE: This ignores any error code from WriteRawFile. But if there is one,
	 *    CloseDumpFile will return it. */
	static void DumpString(const char *string, DumpFile *dumpFile)
		{
		WriteRawFile(dumpFile, string, strlen(string));
		}

	/* Close a dump file. */
	static Err CloseDumpFile(DumpFile *dumpFile)
		{
		return CloseRawFile(dumpFile);
		}

#else	/* use DebugPutStr */

	typedef void		DumpFile;

	static Err OpenDumpFile(DumpFile **file, const char *filename)
		{
		TOUCH(filename);
		*file = NULL;
		return 0;
		}

	static void DumpString(const char *string, DumpFile *dumpFile)
		{
		TOUCH(dumpFile);
		DebugPutStr(string);
		}

	static Err CloseDumpFile(DumpFile *dumpFile)
		{
		TOUCH(dumpFile);
		return 0;
		}

#endif


/*****************************************************************************
 * Trace log dumping
 *****************************************************************************/

/* Dump a trace log entry with fixed-width columns. */
static void	DumpRawTraceEntry(TraceBufferPtr traceBuffer, DumpFile *dumpFile,
		int32 index)
	{
	TraceEntryPtr	curTraceEntry = traceBuffer->evList + index;
	char			str[127];		/* string buffer */

	/*  INDEX  TIME  CHANNEL#  AVALUE  BUFPTR  EVENT */
	sprintf( str,
	 /*	"%5ld\t%10ld\t%10ld\t%10ld\t%#10lx\ttc%ld\r", */
		"%5ld\t%10ld\t%10ld\t%10ld\t%10ld\ttc%ld\r",
		index, curTraceEntry->when, curTraceEntry->channel,
		curTraceEntry->aValue, curTraceEntry->aPtr, curTraceEntry->what );
	DumpString( str, dumpFile );
	}


Err	DumpRawTraceBuffer(TraceBufferPtr traceBuffer, const char *filename)
	{
	Err				status;
	DumpFile		*dumpFile;
	int32			index;				/* for traversing the trace buffer */
	int32			nextIndexToTrace;	/* where trace logging would write next */
	char			str[STRBUF_SIZE];	/* string buffer */

	status = OpenDumpFile(&dumpFile, filename);
	if ( status < 0 )
		return status;

	nextIndexToTrace = (int32)traceBuffer->index;
	if ( nextIndexToTrace > MAX_TRACE )
		nextIndexToTrace = MAX_TRACE;

	sprintf( str, "\r--- STREAMER TRACE LOG ---\r"
		"INDEX\t      TIME\t  CHANNEL#\t    AVALUE\t    BUFPTR\tEVENT\r\r" );
	DumpString( str, dumpFile );

	/* first dump the entries after the wraparound point that were filled */
	for ( index = nextIndexToTrace; index < MAX_TRACE; ++index )
		{
		if ( traceBuffer->evList[index].when == 0 )
			break;	/* tracing didn't fill this entry or following entries */
		DumpRawTraceEntry(traceBuffer, dumpFile, index);
		}

	/* then dump the entries before the wraparound point */
	for ( index = 0; index < nextIndexToTrace; ++index )
		DumpRawTraceEntry(traceBuffer, dumpFile, index);

	return CloseDumpFile( dumpFile );
	}


/* Given the index of some kind of start event, search the trace buffer for the
 * the next matching completion event. To match, it must:
 * 		occur later than the startEvent,
 * 		have an event code that matches the completionEventCode,
 * 		and have an aValue field which matches the start event's aValue.
 *
 * The aValue field is used by the Audio and MPEG subscribers to
 * match I/O start and complete events for the same IOReq Item number. */
static int32 FindCompletionEvent(TraceBufferPtr traceBuffer,
		int32 startEventIndex, int32 completionEventCode)
	{
	int32			curIndex;			/* for traversing the buffer */
	TraceEntryPtr 	startEventEntry;	/* ptr to the trace entry where we
										 * start searching */
	TraceEntryPtr 	curTraceEntry;		/* ptr to current trace entry */

	startEventEntry = traceBuffer->evList + startEventIndex;

	/* Wrap-around search, starting just past the start event. */
	curIndex = startEventIndex;
	while ( true )
		{
		if ( ++curIndex >= MAX_TRACE )
			curIndex = 0;
		if ( curIndex == startEventIndex )
			break;

		curTraceEntry = traceBuffer->evList + curIndex;

		/* If the audio time for an event is 0 then it was never filled.
		 * It can't be the completion event, nothing past it in the log can
		 * be the completion event, and the logger didn't wraparound yet.
		 * So there's no point in searching further. */
		if ( curTraceEntry->when == 0 )
		   break;

		if ( (curTraceEntry->what == completionEventCode) &&
			 (curTraceEntry->aValue == startEventEntry->aValue) &&
			 (curTraceEntry->when > startEventEntry->when) )
			return curIndex;	/* Eureka */
		}

	/* We couldn't find it. */
	return -1;
	}


/* If index is the index of the desired start-event in the trace buffer, search
 * for its matching completion-event and dump stats on it. Else noop.
 * [TBD] This should also print the delta between completions. */
static void DumpEntryCompletionStats(TraceBufferPtr traceBuffer,
		DumpFile *dumpFile, int32 index, int32 startEventCode,
		int32 completionEventCode)
	{
	TraceEntryPtr 	curTraceEntry;		/* ptr to the indexed event */
	int32			completionIndex;	/* index of the completion event */
	char			str[STRBUF_SIZE];	/* string buffer */

	curTraceEntry = traceBuffer->evList + index;

	if ( curTraceEntry->what == startEventCode )
		{
		completionIndex = FindCompletionEvent(traceBuffer, index,
							completionEventCode);

		/* Write out the stats for this completion.  Space with
		 * single tabs for cut/paste into Excel or InControl.  */
		/*  INDEX  CHANNEL#  AVALUE  START_TIME  COMPL_TIME  DURATION */
		sprintf( str,
			"%5ld\t%10ld\t%10ld\t%10ld",
			index, curTraceEntry->channel, curTraceEntry->aValue,
			curTraceEntry->when );
		DumpString( str, dumpFile );

		if ( completionIndex < 0 )
			DumpString( "\t ---------\t ---------\r", dumpFile );
		else
			{
			TraceEntryPtr 	completionTraceEntry =
				traceBuffer->evList + completionIndex;

			sprintf( str,
				"\t%10ld\t%10ld\r",
				completionTraceEntry->when,
				completionTraceEntry->when - curTraceEntry->when );
			DumpString( str, dumpFile );
			}
		}
	}


Err DumpTraceCompletionStats(TraceBufferPtr traceBuffer, int32 startEventCode,
		int32 completionEventCode, const char *filename)
	{
	Err				status;
	DumpFile		*dumpFile;
	char			str[127];
	int32			index;				/* for traversing the trace buffer */
	int32			nextIndexToTrace;	/* where trace logging would write next */

	status = OpenDumpFile(&dumpFile, filename);
	if ( status < 0 )
		return status;

	nextIndexToTrace = (int32)traceBuffer->index;
	if ( nextIndexToTrace > MAX_TRACE )
		nextIndexToTrace = MAX_TRACE;

	sprintf( str, "\r\rEventCompletionStats for tc%ld .. tc%ld\r",
		startEventCode, completionEventCode );
	DumpString( str, dumpFile );

	sprintf( str, "\rINDEX\t  CHANNEL#\t    AVALUE\tSTART_TIME\tCOMPL_TIME\t  DURATION\r\r" );
	DumpString( str, dumpFile );

	/* first process filled entries after the wraparound point */
	for ( index = nextIndexToTrace; index < MAX_TRACE; ++index )
		{
		if ( traceBuffer->evList[index].when == 0 )
			break;	/* tracing didn't fill this entry or following entries */
		DumpEntryCompletionStats(traceBuffer, dumpFile, index,
			startEventCode, completionEventCode);
		}

	/* then process the entries before the wraparound point */
	for ( index = 0; index < nextIndexToTrace; ++index )
		DumpEntryCompletionStats(traceBuffer, dumpFile, index,
			startEventCode, completionEventCode);

	return CloseDumpFile( dumpFile );
	}

