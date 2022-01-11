/******************************************************************************
**
**  @(#) DumpStream.c 96/11/20 1.17
**
**  Contains: tool to dump a stream
**
******************************************************************************/

/**
|||	AUTODOC -public -class Streaming_Tools -group Utility -name DumpStream
|||	Writes diagnostic listing of a stream.
|||	
|||	  Format
|||	
|||	    DumpStream [options] -i <inputfile>
|||	
|||	  Description
|||	
|||	    DumpStream writes a diagnostic listing of stream files output by the 
|||	    Weaver and chunkified files output by the various chunkifier. This 
|||	    can help greatly with debugging and complex stream-file creation.
|||	
|||	  Arguments
|||	
|||	    -i <inputfile>
|||	        Input stream file name.
|||	
|||	    -s <time>
|||	        Stream time in audio ticks at which dump begins.
|||	
|||	    -bw <bps>
|||	        Maximum data bandwidth.
|||	
|||	    -v [<length>]
|||	        Include hex dump of <length> bytes of each chunk's data (default = 64).
|||	
|||	    -xf
|||	        Outputs file positions (byte offsets) in hex rather than decimal.
|||	
|||	    -bs
|||	        Stream block size.
|||	
|||	    -stats
|||	        Statistics output only.
|||	
|||	    -fbs
|||	        Filler block sizes output only.
|||	
|||	    -tb <units>
|||	        Express times in <units> units per second (default = 239.674).
|||	
|||	  Example
|||	
|||	    DumpStream -xf -v 0x400 -i video1.stream
**/


#define	PROGRAM_VERSION_STRING	"1.8 a1"

#include <types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <cursorctl.h>

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <streaming/dsstreamdefs.h>	/* found in {3DOINCLUDES}streaming/ folder */
#endif

#ifndef __STREAMING_SATEMPLATEDEFS_H
	#include <streaming/satemplatedefs.h>
#endif


#define	AUDIO_TICKS_PER_SECOND	((float)(44100./184.))
#define	DFLT_TIME_BASE			AUDIO_TICKS_PER_SECOND

#define	MAX_OK_CHUNK_SIZE		(156L * 1024L)

#define	MEDIA_BLOCK_SIZE		(2048L)
#define	MAX_BUFFER_SIZE			(128 * 1024L)
#define	HEX_DUMP_SIZE			(64)				/* hex bytes to dump on verbose output */

#define	VERBOSE_FLAG_STRING		"-v"
#define	STARTTIME_FLAG_STRING	"-s"
#define	INPUTFILE_FLAG_STRING	"-i"
#define	HEX_FPOS_FLAG_STRING	"-xf"
#define	STATS_FLAG_STRING		"-stats"
#define	FILLBLOCK_FLAG_STRING	"-fbs"
#define	BLOCK_SIZE_FLAG			"-bs"
#define	BANDWIDTH_FLAG_STRING	"-bw"
#define	TIME_BASE_FLAG			"-tb"

/**************************/
/* Stream stats structure */
/**************************/
typedef struct DumpParams_tag
{
	char		*fileName;
	int32		totalFillerBytes;
	int32		totalFillerChunks;
	
	int32		dumpStartTime;
	int32		dumpDataLen;
	int32		maxBandwidth;
	int32		streamBlockSize;
	
	float		timebase;
	
	bool		verbose;
	bool		hexFilePos;
	bool		stats;
	bool		fillerBlockSizes;
	bool		checkBandwidth;
	bool		userSetBlockSize;
} DumpParams;


typedef struct StreamBlockXtra 
{
	int32		channel;		/* logical channel number */
	int32		subChunkType;	/* data sub-type */
} StreamBlockXtra, *StreamBlockXtraPtr;


/*************************/
/* Command line switches */
/*************************/
DumpParams	gDumpParams;


/**************************/
/* Local utility routines */
/**************************/
static bool		ParseCommandLine(int argc, char **argv);
static void		CheckForStreamHeaderChunk(DumpParams *dumpParams, SubsChunkDataPtr wcp);
static int		ReadChunk(FILE *file, uint8 *buffer, size_t bytesToRead);
static void		DumpStreamFile(DumpParams *dumpParams);
static void		DumpStreamHeaderChunk(DSHeaderChunk *hdrp);
static char		*SAudioInstrName(int32 instrID);
static void		DumpStreamHeaderChunk(DSHeaderChunk *hdrp);
static void		DumpMarkerTableChunk(DSMarkerChunk *markerChunk, DumpParams *dumpParams);
static void		AccumulateFillerChunkStats(DumpParams *dumpParams, SubsChunkDataPtr wcp);
static void		DumpStreamChunk(DumpParams *dumpParams, SubsChunkDataPtr wcp, int32 filePosition);
static void		Usage(char *commandNamePtr);
static void		DoCheckForPadding(FILE *fd, SubsChunkDataPtr wcp, int32 *filePosition);

static void		DumpData(uint32 offset, const uint8 *buf, uint32 len, bool hexFileOffsets);
static void		DumpLine(uint32 offset, const uint8 *buf, uint32 len, bool hexFileOffsets);

/*******************************************************************************************
 * Tool application main entrypoint.
 *******************************************************************************************/
void main(int argc, char* argv[])
{
	memset(&gDumpParams, 0, sizeof(gDumpParams));
	gDumpParams.dumpDataLen			= HEX_DUMP_SIZE;
	gDumpParams.streamBlockSize		= (32 * 1024L);
	gDumpParams.timebase			= DFLT_TIME_BASE;

	gDumpParams.hexFilePos			= false;
	gDumpParams.stats				= false;
	gDumpParams.fillerBlockSizes	= false;
	gDumpParams.checkBandwidth		= false;
	gDumpParams.userSetBlockSize	= false;

	if ( !ParseCommandLine(argc, argv) )
	{
		Usage( argv[0] );
		return;
	}

	/* Make sure an input stream name was specified */
	if ( gDumpParams.fileName == NULL )
	{
		printf("ERROR: stream file name must be specified!\n\n");
		Usage( argv[0] );
		return;
	}

	/* We want to spin cursor later */
	InitCursorCtl(NULL);
	
	DumpStreamFile(&gDumpParams);

	exit(0);
}


/*******************************************************************************************
 * Command line parser. Accepts the usual -x unix style switches. Puts parsing
 * results into application global variables which can be pre-initialized to
 * default values. Returns TRUE if command line parsed OK, FALSE if any error.
 *******************************************************************************************/
static bool ParseCommandLine(int argc, char **argv)
	{
	argv++;
	argc--;

	while ( argc > 0 )
		{
		/* Check for the flag that specifies the time base (units per second)
		 */
		if ( strcmp( *argv, TIME_BASE_FLAG ) == 0 )
		{
			argv++;
			if ( (argc -= 2) < 0 )
				return false;
			sscanf(*argv++, "%f", &gDumpParams.timebase);

			/* make sure that the value makes sense */
			if ( gDumpParams.timebase <= 0 )
			{
				fprintf(stderr, "illegal time base value (%f).  Value must be greater than 0.", gDumpParams.timebase);
				return false;
			}
		}

		/* Check for the flag that specifies the stream data
		 * block size. Default for this is 32k bytes.
		 */
		if ( strcmp( *argv, BLOCK_SIZE_FLAG ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gDumpParams.streamBlockSize);
			gDumpParams.userSetBlockSize = true;
			}

		/* Check for the flag that specifies the stream data
		 * start time. Default for this is time == 0.
		 */
		if ( strcmp( *argv, STARTTIME_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gDumpParams.dumpStartTime);
			}

		/* Check for the flag that specifies the output data file name
		 */
		else if ( strcmp( *argv, INPUTFILE_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;

			gDumpParams.fileName = *argv++;	/* save pointer to file name */
			}

		/* Check for the flag that specifies hex output of file positions
		 */
		else if ( strcmp( *argv, HEX_FPOS_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gDumpParams.hexFilePos = ~gDumpParams.hexFilePos;
			}

		/* Check for the flag that specifies verbose output
		 */
		else if ( strcmp( *argv, VERBOSE_FLAG_STRING ) == 0 )
		{
			argv++;
			argc--;
			gDumpParams.verbose = ~gDumpParams.verbose;
			
			/* if the next param DOESN'T have a leading dash, assume it is the amount 
			 *  of data to dump for each byte
			 */
			if ( (argc >= 0) && ('-' != *argv[0]) )
			{
				sscanf(*argv++, "%li", &gDumpParams.dumpDataLen);
				argc--;
			}
		}

		/* Check for the flag to output statistics
		 */
		else if ( strcmp( *argv, STATS_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gDumpParams.stats = ~gDumpParams.stats;
			}

		/* Check for the flag to output filler block sizes
		 */
		else if ( strcmp( *argv, FILLBLOCK_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gDumpParams.fillerBlockSizes = ~gDumpParams.fillerBlockSizes;
			}

		/* Check for the flag to check stream data bandwidth
		 */
		else if ( strcmp( *argv, BANDWIDTH_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gDumpParams.maxBandwidth);
			gDumpParams.checkBandwidth = true;
			}

		/* Unknown flag encountered
		 */
		else
			return false;
		}

	return true;
	}


/* 
 * If the stream has a valid Stream Header chunk 
 *  and the user has not specified a block size on the command line 
 *  then get the block size from the stream header
 */
static void CheckForStreamHeaderChunk(DumpParams *dumpParams, SubsChunkDataPtr wcp)
{
	DSHeaderChunkPtr hdrp;
	
	if ( HEADER_CHUNK_TYPE != wcp->chunkType )
	{
		printf("  No stream header found\n");
		printf("\n  Assuming Stream Block size of %d (0x%X)\n\n",
						dumpParams->streamBlockSize, dumpParams->streamBlockSize);
		goto DONE;
	}

	hdrp = (DSHeaderChunkPtr)wcp;
	if( DS_STREAM_VERSION != hdrp->headerVersion )
	{
		printf("  Unknown stream header version (%ld)\n", hdrp->headerVersion);
		printf("\n  Assuming Stream Block size of %d (0x%X)\n\n",
						dumpParams->streamBlockSize, dumpParams->streamBlockSize);
		goto DONE;
	}

	if ( hdrp->streamBlockSize > 0 )
	{
		if ( false == dumpParams->userSetBlockSize )
			dumpParams->streamBlockSize = hdrp->streamBlockSize;
		else if ( dumpParams->streamBlockSize != hdrp->streamBlockSize )
		{	
			/* Command line blocksize doesn't match the blocksize in streamheader. */
			printf("\nCommand line blocksize doesn't match the blocksize in streamheader." );
			printf("\nCommand line blocksize is : %d\nStream blocksize is : %d\n",
						 dumpParams->streamBlockSize, hdrp->streamBlockSize ); 
			dumpParams->streamBlockSize = hdrp->streamBlockSize;
		}
		printf("\n  Assuming Stream Block size of %d (0x%X) as specified in Stream header\n\n",
					dumpParams->streamBlockSize, dumpParams->streamBlockSize);
	}

DONE:
	return;
}


/*******************************************************************************************
 * change a time from audio folio ticks (44100 / 184 units per second) to whatever units
 *  the user requested.
 *******************************************************************************************/
static uint32
CorrectTimeForTimebase(DumpParams *dumpParams, uint32 time)
{
	float	timebaseMultiplier = dumpParams->timebase / AUDIO_TICKS_PER_SECOND;

	return (uint32) ((float)time * timebaseMultiplier + 0.5);
}


/*******************************************************************************************
 * routine to read a chunk
 *******************************************************************************************/
static int ReadChunk(FILE *file, uint8 *buffer, size_t bytesToRead)
{
	int		err;
	/* read the first chunk */
	if ( 1 != (err = fread(buffer, bytesToRead, 1, file)) )
		goto DONE;

DONE:
	return err;
}



/*******************************************************************************************
 * Routine to dump a stream file to standard output
 *******************************************************************************************/
static void		DumpStreamFile(DumpParams *dumpParams)
{
	FILE				*fd;
	char				*buffer;
	SubsChunkDataPtr	wcp;
	int32				filePosition = 0;
	int32				nextBlockStart;
	int32				lastStreamTime;
	int32				fileSize;
	size_t				bytesToRead;
	bool	 			fDumping = false;
	bool				fFillerChunk;

	/* Open the input data stream file */
	fd = fopen(dumpParams->fileName, "rb" );
	if ( NULL == fd )
	{
		fprintf( stderr, "error opening input stream %s\n", dumpParams->fileName );
		return;
	}

	buffer = (char*) malloc( MAX_BUFFER_SIZE );
	wcp = (SubsChunkDataPtr) buffer;
	if ( buffer == NULL )
		{
		fprintf( stderr, "DumpStreamFile() - unable to allocate input buffer\n" );
		return;
		}

	/* print the file header if we're not doing a stats only dump */
	if ( !dumpParams->stats )
	{
		printf( "\n\n  Dump of stream file: %s\n", dumpParams->fileName );
		if ( dumpParams->dumpStartTime != 0 )
			printf( "    dump starting at time: %ld\n", dumpParams->dumpStartTime );

		/* if the user has specified an alternate time base, point it out */
		if ( DFLT_TIME_BASE != dumpParams->timebase )
			printf( "*** NOTE: all times expressed in %.4f units per second ***\n", dumpParams->timebase);
		printf("\n");
	}

	/* Set the "last" time to trigger comparison of
	 * time/data bandwidth if dumpParams->checkBandwidth is set.
	 */
	lastStreamTime = -1;
	
	/* figure out how much data to read each time through */
	bytesToRead = sizeof(DSHeaderChunk);
	if ( (sizeof(SubsChunkData) + dumpParams->dumpDataLen) > bytesToRead )
		bytesToRead = (sizeof(SubsChunkData) + dumpParams->dumpDataLen);
	/* if the file is smaller than a stream header (ie. headerless stream file or a chunked
	 *  file which hasn't been woven yet), just read as much data as the file contains
	 */
	fseek(fd, 0, SEEK_END);
	fileSize = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	if ( bytesToRead > fileSize )
		bytesToRead = fileSize;
	
	/* read the first chunk */
	if ( 1 != ReadChunk(fd, (uint8 *)buffer, bytesToRead) )
		goto DONE;

	/* see if we just read the stream header, if so parse it */
	CheckForStreamHeaderChunk(dumpParams, wcp);

	if ( !dumpParams->stats )
	{
		printf( "  File Pos   Type        Size       Time    Chan    SubType\n" );
		printf( "  --------  ------       ----       ----    ----    -------\n" );
	}

	do
	{
		SpinCursor(32);
		
		/* sanity check the chunk, if the size not reasonable it is be because the
		 *  file is corrupt or it not a stream file
		 */
		if ( wcp->chunkSize > MAX_OK_CHUNK_SIZE )
		{
			printf( "Invalid chunk size (%ld).  Corrupt stream file?\n", wcp->chunkSize );
			return;
		}

		if ( dumpParams->streamBlockSize != 0 )
			{
			/* Calculate the next block starting position. If there isn't enough
			 * room in the current "block" to hold another complete chunk header,
			 * then skip ahead to the next whole stream block like the parser would.
			 * NOTE: this is only useful for woven streams whose data have been 
			 * aligned on fixed blocks.  These files sometimes have an incomplete
			 * 'FILL' block at the end of a stream block.
			 */
			nextBlockStart = ((filePosition / dumpParams->streamBlockSize) * dumpParams->streamBlockSize) 
								+ dumpParams->streamBlockSize;
			if ( (filePosition + sizeof(SubsChunkData)) >= nextBlockStart )
				{
				filePosition = nextBlockStart;
				fseek(fd, filePosition, SEEK_SET);
				continue;
				}
			}

		/* Skip chunks until the specified start time
		 * arrives in the stream. Set a one-shot flag to enable
		 * dumping. We do this because chunk times may be funky
		 * for some chunks.
		 */
		fFillerChunk = (FILL_CHUNK_TYPE == wcp->chunkType);

		/* Try to detect data bandwidth problems in the following way:
		 * Watch for timestamp > previous_timestamp. Check for bandwidth
		 * problems when they change because there may be several chunks 
		 * with the same timestamp and we want the very last one.
		 */
		if ( (! fFillerChunk) && dumpParams->checkBandwidth )
			{
			if ( wcp->time != lastStreamTime )
				{
				/* Reset the "last" time so we detect the next transition */
				lastStreamTime = wcp->time;

				/* Test for data bandwidth exceeded and output a warning if so */
				if ( (wcp->time * (dumpParams->maxBandwidth / AUDIO_TICKS_PER_SECOND) )
						< filePosition )
					{
					printf( "Warning: bandwidth exceeded at time = %ld, filepos = %ld\n", 
							wcp->time, filePosition );
					}
				}
			}

	
		/* Begin dumping/counting filler when we've reached a non-filler chunk whose timestamp 
		 *  meets or exceeds the starting time specified on the command line.
		 */
		if ( (!fDumping) && (!fFillerChunk) 
			&& (CorrectTimeForTimebase(dumpParams, wcp->time) >= dumpParams->dumpStartTime ) )
			fDumping = true;

		if ( true == fDumping )
		{
			if ( false == dumpParams->stats )
				DumpStreamChunk(dumpParams, wcp, filePosition);
			else
				AccumulateFillerChunkStats(dumpParams, wcp);
		}

		/* Position the file to the next chunk in the file */
		filePosition += wcp->chunkSize;

		/* seek to the start of the next chunk, deal with padding if necessary */
		fseek( fd, filePosition, SEEK_SET );
		DoCheckForPadding(fd, wcp, &filePosition);
	} while ( 1 == ReadChunk(fd, (uint8 *)buffer, bytesToRead) );

DONE:

	/* If we're doing a statistics only dump, then now is our chance to display
	 * what we've collected about the stream. For now, we give the percentage of
	 * data to filler (to help determine wasted space and stream data bandwidth),
	 * and a suggested optimum blocksize, which is the average block size less
	 * the filler blocks, rounded to the nearest MEDIA_BLOCK_SIZE.
	 */
	if ( dumpParams->stats )
		{
		/* Check filePosition to prevent divide-by-zero
		 */
		if ( (filePosition > 0) && (! dumpParams->fillerBlockSizes) )
			{
			int32	percentFiller = (dumpParams->totalFillerBytes * 100) / filePosition;

			if ( dumpParams->totalFillerChunks > 0 )
			{
				printf( "\"%s\" contains %ld%% filler\n", 
						dumpParams->fileName, percentFiller );

				printf( "\t\tblock size = %ld bytes\n", dumpParams->streamBlockSize );

				printf( "\t\taverage filler block size = %ld bytes\n\n", 
							dumpParams->totalFillerBytes / dumpParams->totalFillerChunks );
			}
			else
			{
				printf( "\"%s\" contained no filler blocks\n", dumpParams->fileName );
				printf( "stream block size cannot be determined\n" );
			}
		}
		else
			printf( "File %s is empty!\n", dumpParams->fileName );
	}
} /* DumpStreamFile */


/* check for padding bytes at the end of a chunk not accounted for by the chunk size
 *  (ALL chunks must occupy a quad multiple number of bytes, though the size of the
 *  chunk may be smaller).  seek past pad bytes if necessary
 */
static void
DoCheckForPadding(FILE* fd, SubsChunkDataPtr wcp, int32* filePosition)
{
	int32	remainder = wcp->chunkSize % 4;
	
	if ( remainder )
	{
		int32 temp = 0;
		int32 bytes = 4 - remainder;
		
		// If padded, skip the pad bytes.
		fread(&temp, 1, bytes, fd);
		*filePosition += bytes;
		
		// Padded bytes should all zero. Check.
		if ( temp != 0 )
			fprintf( stderr, "DoCheckForPadding() - Padding bytes aren't zeros.\n");
	}
}


/*******************************************************************************************
 * print a stream chunk or accumulate statistics about it
 *******************************************************************************************/
static void DumpStreamChunk(DumpParams *dumpParams, SubsChunkDataPtr wcp, int32 filePosition)
{
	int32		chunkDataSize;
	bool		fillerChunk = (FILL_CHUNK_TYPE == wcp->chunkType);

	/* try to make sure we have a valid chunk (only filler chunks can have less 
	 *  than a full chunkheader)
	 */
	chunkDataSize = wcp->chunkSize - sizeof(SubsChunkData);
	if ( (chunkDataSize <= 0) && (!fillerChunk) )
	{
		printf("### data chunk size (%ld) less than normal minimum size (%ld)", 
				wcp->chunkSize, (sizeof(SubsChunkData) + sizeof(int32)));
		printf("###    fileposition = %ld", filePosition);
		exit(0);
	}

	/* Output file position in decimal or hex as requested plus the rest */
	if ( dumpParams->hexFilePos )
	{
		printf("- %8.8lX: '%.4s' %10lX ", filePosition, &wcp->chunkType, wcp->chunkSize);
	}
	else
	{
		printf("- %8.8ld: '%.4s' %10ld ", filePosition, &wcp->chunkType, wcp->chunkSize);
	}
	printf("%10ld%8ld    '%.4s'\n", CorrectTimeForTimebase(dumpParams, wcp->time), 
				wcp->channel, &wcp->subChunkType);

	if ( dumpParams->verbose )
	{
		switch ( wcp->chunkType )
		{
			case HEADER_CHUNK_TYPE:
				DumpStreamHeaderChunk((DSHeaderChunk *)wcp);
				break;
			case DATAACQ_CHUNK_TYPE:
				if ( MARKER_TABLE_SUBTYPE == wcp->subChunkType )
				{
					DumpMarkerTableChunk((DSMarkerChunk *)wcp, dumpParams);					
					break;
				}
				/* fall through to generic chunk dump if data acq chunk is not marker table */
			default:	/* unknown type, hex/ascii dump the data */
				/* Pin hex dump size to the max as specified by the user */
				if ( chunkDataSize > dumpParams->dumpDataLen )
					chunkDataSize = dumpParams->dumpDataLen;

				if ( (chunkDataSize > 0) && (!fillerChunk) )
				{
					/* dump the chunk's data, if we don't print the whole chunk
					 *  indicate so by printing an elipsis
					 */
					DumpData(0, (uint8 *)wcp + sizeof(SubsChunkData), chunkDataSize, dumpParams->hexFilePos);
					if ( chunkDataSize < (wcp->chunkSize - sizeof(SubsChunkData)) ) 
						printf ("%7 ...\n");
				}
				break;
		}
	}

	/* output a blank line to separate blocks in the stream */
	if ( (filePosition + wcp->chunkSize) % dumpParams->streamBlockSize == 0 )
		printf("\n");
}


/*******************************************************************************************
 * Accumulating statistics, not doing a normal data dump of the stream. We keep a total count
 *  of the filler bytes in the file
 *******************************************************************************************/
void
AccumulateFillerChunkStats(DumpParams *dumpParams, SubsChunkDataPtr wcp)
{
	if ( FILL_CHUNK_TYPE == wcp->chunkType )
	{
		/* Output the filler block size to stdout if the
		 * option specifying this is selected.
		 */
		if ( dumpParams->fillerBlockSizes )
			printf( "%ld\n", wcp->chunkSize);

		/* Tally all filler bytes in the file */
		dumpParams->totalFillerBytes += wcp->chunkSize;
		dumpParams->totalFillerChunks++;
	}
}

/*******************************************************************************************
 * return the name of an audio folio instrument name tag
 *******************************************************************************************/
static char *SAudioInstrName(int32 instrID)
{
/* structure to associate a token name with it's value	*/
typedef struct TokenValue
{
	char 	*name;
	uint32 	value;
} TokenValue, *TokenValuePtr;

	TokenValue	tokenArray[] = 
		{
			{"sa_44k_16b_s",		SA_44K_16B_S }, 
			{"sa_44k_16b_m",		SA_44K_16B_M }, 
			{"sa_44k_8b_s",			SA_44K_8B_S }, 
			{"sa_44k_8b_m",			SA_44K_8B_M },
			{"sa_22k_16b_s",		SA_22K_16B_S }, 
			{"sa_22k_16b_m",		SA_22K_16B_M }, 
			{"sa_22k_8b_s",			SA_22K_8B_S }, 
			{"sa_22k_8b_m",			SA_22K_8B_M },
/* for opera compatibility only. Should not find "_SDX2" when weaving M2 streams. */
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
	int16	ndx;
	
	for ( ndx = 0; tokenArray[ndx].name != NULL; ndx++ ) 
	{
		if ( instrID == tokenArray[ndx].value ) 
			break;
	}
	
	if ( NULL != tokenArray[ndx].name )
		return tokenArray[ndx].name;
	else
		return "<unknown instrument!>";
} 


/*******************************************************************************************
 * print a stream header chunk
 *******************************************************************************************/
static void DumpStreamHeaderChunk(DSHeaderChunk *hdrp)
{
	DSHeaderSubs	*subsPtr;
	uint16			ndx;

	printf("  [ Stream Header ]\n");
	printf("      Header Version:             %5ld\n", hdrp->headerVersion);
	printf("      Stream block size:          %5ld\n", hdrp->streamBlockSize);
	printf("      Number of stream buffers:   %5ld\n", hdrp->dataAcqDeltaPri);
	printf("      Streamer delta priority:    %5ld\n", hdrp->streamBuffers);
	printf("      Data acq. delta priority:   %5ld\n", hdrp->streamerDeltaPri);
	printf("      Number of subscriber msgs:  %5ld\n", hdrp->numSubsMsgs);

	printf("      Audio clock channel:        %5ld\n", hdrp->audioClockChan);
	printf("      Audio channel enabled meak: %5lX\n", hdrp->enableAudioChan);
	
	if ( 0 != hdrp->preloadInstList[0] )
	{
		printf("      Audio peloaded instruments:");
		for ( ndx = 0;  0 != hdrp->preloadInstList[ndx]; ndx++ )
		{
	        if ( (ndx > 0) && !(ndx & 3) )
	        	printf ("\n                                        ");
			printf("%21s", SAudioInstrName(hdrp->preloadInstList[ndx]));
		}
	}

	printf("\n      Subscriber list:   subscriber   priority\n");
	printf(  "                         ----------   --------\n");
	for ( ndx = 0; 
			subsPtr = &hdrp->subscriberList[ndx],
			0 != subsPtr->subscriberType; 
			ndx++ ) 
	{
		printf("                           '%.4s' %11ld\n", 
					&subsPtr->subscriberType, subsPtr->deltaPriority);
	}
	printf("\n");
}


/*******************************************************************************************
 * print a marker table chunk
 *******************************************************************************************/
static void DumpMarkerTableChunk(DSMarkerChunk *markerChunk, DumpParams *dumpParams)
{
	MarkerRec	*markerRec = (MarkerRec *)((uint8 *)markerChunk + sizeof(DSMarkerChunk));
	int16		tblSize = (markerChunk->chunkSize - sizeof(DSMarkerChunk)) / sizeof(MarkerRec);
	int16		ndx;

	printf("  [ Marker Table ]\n");
	printf("      stream time  file offset\n");
	printf("      -----------  -----------\n");
	for ( ndx = 0; ndx < tblSize; ndx++ )
	{
		if ( dumpParams->hexFilePos )
			printf("      %10ld        %6lX\n", 
					CorrectTimeForTimebase(dumpParams, markerRec[ndx].markerTime), markerRec[ndx].markerOffset);
		else
			printf("      %10ld        %6ld\n", 
					CorrectTimeForTimebase(dumpParams, markerRec[ndx].markerTime), markerRec[ndx].markerOffset);
	}
	printf("\n");
}


/*******************************************************************************************
 * print lines of hex/ascii data
 *******************************************************************************************/
#define MAXBYTESPERLINE 16
#ifndef MIN
	#define	MIN(val1, val2) ((val1) <= (val2) ? (val1) : (val2))
#endif
static void DumpData(uint32 offset, const uint8 *buf, uint32 len, bool hexFileOffsets)
{
	uint32	bytesPerLine;

	while ( len > 0 ) 
	{
		DumpLine(offset, buf, bytesPerLine = MIN(MAXBYTESPERLINE, len), hexFileOffsets);
		offset += bytesPerLine;
		buf    += bytesPerLine;
		len    -= bytesPerLine;
		
		/*  spin the cursor if we're dumping a bunch of data... */
		if ( 0 == (len % 10) )
			SpinCursor(32);
	}
}

/*******************************************************************************************
 * prints an offset, 4 long words of data as hex, followed by it's ascii equivalent, ie. 
 *  
 *     0060: 4d505644 fffffffe 534e4453 0000000b MPVD....SNDS....
 *  
 *******************************************************************************************/
static void DumpLine(uint32 offset, const uint8 *buf, uint32 len, bool hexFileOffsets)
{
    uint32	ndx;
    char	asciiBuff[MAXBYTESPERLINE+1];

	/* print the offset within the current chunk */
	if ( hexFileOffsets )
		printf ("%10.04LX:", offset);
	else
		printf ("%10.04Ld:", offset);

    /* accumulate the ascii data into the buffer */
    for ( ndx = 0; ndx < len; ndx++ )
    {
        const uint8 aChar = buf[ndx];
        if ( !(ndx & 3) )
        	printf (" ");
        printf("%02x", aChar);
        asciiBuff[ndx] = (aChar >= 0x20 && aChar <= 0x7e) ? aChar : '.';
    }
    asciiBuff[len] = '\0';
    
    /* and print if */
    printf("%*s%s\n", (MAXBYTESPERLINE - len) * 2 + (MAXBYTESPERLINE - len) / 4 + 1, "", asciiBuff);
}



/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void Usage( char* commandNamePtr )
{
	fprintf (stderr, "%s version %s\n", commandNamePtr, PROGRAM_VERSION_STRING);
	fprintf (stderr, "usage: %s flags\n", commandNamePtr);
	fprintf (stderr, "   %-6s <file>  input stream file name\n", INPUTFILE_FLAG_STRING);
	fprintf (stderr, "   %-6s <time>  start dump at time\n", STARTTIME_FLAG_STRING);
	fprintf (stderr, "   %-6s <bps>   max data bandwidth\n", BANDWIDTH_FLAG_STRING);
	fprintf (stderr, "   %-6s <bytes> hex dump <bytes> bytes of each chunk's data (default = %ld).\n",
					 VERBOSE_FLAG_STRING, HEX_DUMP_SIZE);
	fprintf (stderr, "   %-6s         hex file position output\n", HEX_FPOS_FLAG_STRING);
	fprintf (stderr, "   %-6s         stream block size\n", BLOCK_SIZE_FLAG);
	fprintf (stderr, "   %-6s         statistics output only\n", STATS_FLAG_STRING);
	fprintf (stderr, "   %-6s         filler block sizes output only\n", FILLBLOCK_FLAG_STRING);
	fprintf (stderr, "   %-6s <units> express times in <units> units per second (default = %f\n", 
					TIME_BASE_FLAG, DFLT_TIME_BASE);
}


