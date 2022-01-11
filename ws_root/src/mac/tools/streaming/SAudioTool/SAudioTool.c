/******************************************************************************
**
**  @(#) SAudioTool.c 96/04/08 1.7
**  Contains: An MPW Tool to print formatted output for .saudio header data and
**            allow modification of the header and data chunks.
**
******************************************************************************/

/**
|||	AUTODOC -public -class Streaming_Tools -group AIFFaudio -name SAudioTool
|||	Prints or modifies header information of chunkified AIFF files.
|||	
|||	  Format
|||	
|||	    SAudioTool [options] -i <infile>
|||	
|||	  Description
|||	
|||	    SAudioTool can perform either of two functions: (1) print out the
|||	    header information (see "Examples" section below) of chunkified 
|||	    audio files created by AudioChunkifier, or (2) modify the header
|||	    information (and data chunks, if necessary).
|||	
|||	  Arguments
|||	
|||	    To dump out the existing header info without modification, use
|||	    only the -dh and -i arguments. Using any of the other arguments
|||	    will modify the input file.
|||	
|||	    -dh
|||	        Dump header data to screen.
|||	
|||	    -l <chan>
|||	        Specify logical channel.  Modifies input file.
|||	
|||	    -nb <num>
|||	        Specify umber of audio folio buffers to use. Modifies input
|||	        file.
|||	
|||	    -ia <amp>
|||	        Specify initial amplitude (in hex). Modifies input file.
|||	
|||	    -ip <pan>
|||	        Specify initial pan (in hex). Modifies input file.
|||	
|||	    -i <infile>
|||	        Input chunkified audio file name.
|||	
|||	  Examples
|||	
|||	    SAudioTool -dh -i soundfile.snds
|||	
|||	    In the example above, SAudioTool prints out the following info:
|||	        logical channel
|||	        number of audio folio buffers
|||	        initial amplitude
|||	        initial pan
|||	        sample size
|||	        sample rate
|||	        number of channels
|||	        compression type
|||	        compression ratio
|||	        total number of samples
|||	
|||	    SAudioTool -lc 1 -ia 0x4000 -i soundfile.snds
|||	
|||	    In the example above, SAudioTool modifies the logical channel
|||	    number and the input amplitude of the chunkified input file.
**/


#define	PROGRAM_VERSION_STRING	"1.4"
#define	PROGRAM_DATE_STRING		"08/15/95"

#include <StdLib.h>
#include <StdIO.h>
#include <Memory.h>
#include <String.h>
#include <Types.h>
#include <AIFF.h>
#include <cursorctl.h>
#include <streaming/dsstreamdefs.h>

#define	FILE_FLAG_STRING			"-i"
#define	DUMP_HEADER_FLAG_STRING		"-dh"
#define	RECHANNELIZE_FLAG_STRING	"-lc"
#define	NUMBER_BUFFERS_FLAG_STRING	"-nb"
#define	INITIAL_AMP_FLAG_STRING		"-ia"
#define	INITIAL_PAN_FLAG_STRING		"-ip"

/*********************************************************************
 ** 	Globals                              
 *********************************************************************/
static char*	gProgramNameStringPtr	= NULL;		/* program name string ptr for usage() */
static char*	gFileName				= NULL;		/* default input file name */
static long		gNewLogicalChannel		= -1;		/* for rechannelize option */
static long		gNewNumberBuffers		= 0;
static long		gNewInitialAmp			= 0;
static long		gNewInitialPan			= 0;
static Boolean	gPrintHeaderDataFlag	= false;	

/*********************************************************************
 * 		ANSI function prototypes                                
 *********************************************************************/
static void		usage( void );
static Boolean	ParseCommandLine ( int argc, char **argv );

/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
static void 	usage( void )
	{
	printf("Summary: Modifies .saudio files.\n");
	printf("Usage: %s [flags] -i <filename>\n", gProgramNameStringPtr );
	printf("Version %s, %s\n",PROGRAM_VERSION_STRING, PROGRAM_DATE_STRING );
	printf( "\t\t-dh \t\tdump header data to screen\n" );
	printf( "\t\t-lc \t\tnew logical channel\n" );
	printf( "\t\t-nb \t\tnew number of AudioFolio buffers to use\n" );
	printf( "\t\t-ia \t\tnew initial amplitude (in hex)\n" );
	printf( "\t\t-ip \t\tnew initial pan (in hex)\n" );
	}

/*********************************************************************************
 * Main code to read the SAudio header chunk and print out contents.
 *********************************************************************************/
int	main( int argc, char* argv[] )
	{
	long	result			=	0;
	long	bytesRead		=	0;
	long	bytesWritten	=	0;
	Boolean	hdrChangedFlag	=	false;
	FILE*	theFile			=	NULL;
	long	chunkStartPos	=	0;
	
	SAudioHeaderChunkPtr	hcp;
	SAudioDataChunkPtr		dcp;
	
	/* Copy program name string pointer for the usage() routine */
	gProgramNameStringPtr = argv[0];

	if ( argc < 3 )
		{
		usage();
		goto error;
		}

	/* Parse command line and check results */
	if ( !ParseCommandLine(argc, argv))
		{
		usage();
		goto error;
		}

	if ( gFileName == NULL )
		{
		usage();
		fprintf( stderr, "Error: file name not specified\n" );
		goto error;
		}

	/* We want to spin cursor later */
	InitCursorCtl(NULL);

	/* Open the input file */
	theFile = fopen( gFileName, "r+b" );
	if ( ! theFile )
		{
		fprintf( stderr, "Error: could not open the file = %s\n", gFileName );
		goto error;
		}

	/* Allocate a buffer to use for chunk parsing */
	hcp = malloc( sizeof(SAudioHeaderChunk) );
	if ( hcp == 0 )
		{
		fprintf( stderr, "Error: could not allocate a buffer of size = %ld\n", sizeof(SAudioHeaderChunk) );
		goto error;
		}

	/* Let the world know we are working on it */
	SpinCursor(32);

	/* Read the header from the input file */
	bytesRead = fread ( hcp, 1, sizeof(SAudioHeaderChunk), theFile );
	if ( bytesRead != sizeof(SAudioHeaderChunk) )
		{
		fprintf( stderr, "Error: Couldn't read the header chunk from %s.\n", gFileName );
		goto error;
		}

	if ( (hcp->chunkType != SNDS_CHUNK_TYPE) || (hcp->subChunkType != SHDR_CHUNK_TYPE) )
		{
		fprintf( stderr, "Error: This does not appear to be an .saudio file.\n");
		goto error;
		}
	
	if ( gPrintHeaderDataFlag )
		{
		fprintf( stdout, "\t\tLogical Channel = %ld\n", hcp->channel);
		fprintf( stdout, "\t\tNumber of AudioFolio Buffers = %ld\n", hcp->numBuffers);
		fprintf( stdout, "\t\tInitial Amplitude = 0x%lx\n", hcp->initialAmplitude);
		fprintf( stdout, "\t\tInitial Pan = 0x%lx\n", hcp->initialPan);
		fprintf( stdout, "\t\tSample Size = %ld bits\n", hcp->sampleDesc.sampleSize);
		fprintf( stdout, "\t\tSample Rate = %ld\n", hcp->sampleDesc.sampleRate);
		fprintf( stdout, "\t\tNumber of Channels = %ld\n", hcp->sampleDesc.numChannels);
		fprintf( stdout, "\t\tCompression Type = %.4s\n", &hcp->sampleDesc.compressionType);
		fprintf( stdout, "\t\tCompression Ratio = %ld\n", hcp->sampleDesc.compressionRatio);
		fprintf( stdout, "\t\tTotal Number of Samples = %lu\n", hcp->sampleDesc.sampleCount);
		}
		
	if (gNewNumberBuffers > 0)
		{
		if (gNewNumberBuffers <= 32)
			{
			hcp->numBuffers = gNewNumberBuffers;
			hdrChangedFlag = true;
			}
		else
			{
			fprintf( stderr, "Error: New number of buffers is out of range.\n");
			fprintf( stderr, "       Must be between 1 - 32.\n");
			goto error;
			}
		}
		
	if (gNewInitialAmp > 0) 
		{
		if (gNewInitialAmp < 0x8000)
			{
			hcp->initialAmplitude = gNewInitialAmp;
			hdrChangedFlag = true;
			}
		else
			{
			fprintf( stderr, "Error: New initial amplitude is out of range.\n");
			fprintf( stderr, "       Must be 1 - 0x7FFF.\n");
			goto error;
			}
		}
		
	if (gNewInitialPan > 0) 
		{
		if (gNewInitialPan < 0x8000) 
			{
			hcp->initialPan = gNewInitialPan;		
			hdrChangedFlag = true;
			}
		else
			{
			fprintf( stderr, "Error: New initial pan is out of range.\n");
			fprintf( stderr, "       Must be 1 - 0x7FFF.\n");
			goto error;
			}
		}
		
	/* Let the world know we are working on it */
	SpinCursor(32);

	if (gNewLogicalChannel >= 0)
		{
		if (gNewLogicalChannel <= 16 )
			{
			/* Change the header's channel and write it back to the file.
			 * The header may include changes from above as well.
			 */
			hcp->channel = gNewLogicalChannel;		
			rewind(theFile);
			bytesWritten = fwrite(hcp, 1, sizeof(SAudioHeaderChunk), theFile);
			if ( bytesWritten != sizeof(SAudioHeaderChunk) )
				{
				fprintf( stderr, "Error: Couldn't write the new header chunk to %s.\n", gFileName );
				goto error;
				}

			/* Use a data chunk ptr from now on because we're done with the header forever. */
			dcp = (SAudioDataChunkPtr) hcp;
	
			while(true)
				{
				/* Let the world know we are working on it */
				SpinCursor(32);

				/* Save the start fpos of the data chunk */
				chunkStartPos = ftell(theFile);
				
				/* Read the first data chunk header from the input file */
				bytesRead = fread ( dcp, 1, SAUDIO_DATA_CHUNK_HDR_SIZE, theFile );
				if ( bytesRead != SAUDIO_DATA_CHUNK_HDR_SIZE )
					{
					/* Unsightly hack... if we got here we're out of data chunks so exit
					 * the loop.
					 */
					 break;
					 }
	
				/* Change the channel */
				dcp->channel = gNewLogicalChannel;
				
				/* Seek back to the chunk's beginning */
				fseek(theFile, chunkStartPos, SEEK_SET);
				
				bytesWritten = fwrite(dcp, 1, SAUDIO_DATA_CHUNK_HDR_SIZE, theFile);
				if ( bytesWritten != SAUDIO_DATA_CHUNK_HDR_SIZE )
					{
					fprintf( stderr, "Error: Couldn't write some new data chunk to %s.\n", gFileName );
					goto error;
					}
	
				/* Skip to next data chunk */
				fseek(theFile, (chunkStartPos + dcp->chunkSize), SEEK_SET);
				}
			}
		else
			{
			fprintf( stderr, "Error: New logical channel number out of range.\n");
			fprintf( stderr, "       Must be 1 - 16.\n");
			goto error;
			}
		}
	else
		if (hdrChangedFlag)
			{
			rewind(theFile);
			bytesWritten = fwrite(hcp, 1, sizeof(SAudioHeaderChunk), theFile);
			if ( bytesWritten != sizeof(SAudioHeaderChunk) )
				{
				fprintf( stderr, "Error: Couldn't write the new header chunk to %s.\n", gFileName );
				goto error;
				}
			}

	fflush(theFile);
	fclose(theFile);
	return 0;  /* sucessful completion */

error:
	if (theFile)
		fclose(theFile);
	return 1;
}


/*******************************************************************************************
 * Routine to parse command line arguments.
 *******************************************************************************************/
#define	PARSE_FLAG_ARG( argString, argFormat, argVariable)	\
	if ( strcmp( *argv, argString ) == 0 )				\
		{												\
		argv++;											\
		if ((argc -= 2) < 0)							\
			return false;								\
		sscanf( *argv++, argFormat, &argVariable);		\
		continue;										\
		}

static Boolean	ParseCommandLine ( int argc, char **argv )
	{
	/* Skip past the program name */
	argv++;
	argc--;

	while ( argc > 0 )
		{		
		PARSE_FLAG_ARG( NUMBER_BUFFERS_FLAG_STRING, "%li", gNewNumberBuffers );
		PARSE_FLAG_ARG( INITIAL_AMP_FLAG_STRING, "%li", gNewInitialAmp );
		PARSE_FLAG_ARG( INITIAL_PAN_FLAG_STRING, "%li", gNewInitialPan );
		PARSE_FLAG_ARG( RECHANNELIZE_FLAG_STRING, "%li", gNewLogicalChannel );

		if ( strcmp( *argv, DUMP_HEADER_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gPrintHeaderDataFlag = true;
			}

		else if ( strcmp( *argv, FILE_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gFileName = *argv++;
			continue;
			}

			
		/* Unknown flag encountered
		 */
		else 
			return false;
		}
	return true;
	}
