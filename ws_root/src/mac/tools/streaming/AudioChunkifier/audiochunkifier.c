/******************************************************************************
**
**  @(#) audiochunkifier.c 96/05/23 1.14
**  MPW tool to turn an AIFF or AIFC soundfile into a stream file.
**
******************************************************************************/

/*********************************************************************************
|||	AUTODOC -public -class Streaming_Tools -group AIFFaudio -name AudioChunkifier
|||	Chunkifies an AIFF or AIFC sampled audio file.
|||	
|||	  Format
|||	
|||	    AudioChunkifier [options] -i <infile> -o <outfile>
|||	
|||	  Description
|||	
|||	    Converts a standard AIFF or compressed-AIFF ("AIFC") format sampled
|||	    audio file to a chunkified file of chunk type 'SNDS'.
|||	
|||	  Supported audio types -normal_format
|||	
|||	    The following table lists the various AIFF and AIFC sampled
|||	    audio data formats supported by the AudioChunkifier tool, as
|||	    well as each format's tag ID, DSP instrument name, and data 
|||	    bandwidth.
|||	
|||	  -preformatted
|||	
|||	    Data Type Tag ID   DSP Instrument Name   Data Type Description   Data BW
|||	    ----------------   -------------------   ---------------------   -------
|||	    SA_22K_8B_M        sampler_8_v1.dsp      22.05 kHz 8-bit mono    22 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_22K_8B_S        sampler_8_v2.dsp      22.05 kHz 8-bit stereo  44 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_22K_16B_M       sampler_16_v1.dsp     22.05 kHz 16-bit mono   44 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_22K_16B_S       sampler_16_v2.dsp     22.05 kHz 16-bit stereo 88 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_44K_8B_M        sampler_8_f1.dsp      44.1 kHz 8-bit mono     44 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_44K_8B_S        sampler_8_f2.dsp      44.1 kHz 8-bit stereo   88 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_44K_16B_M       sampler_16_f1.dsp     44.1 kHz 16-bit mono    88 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_44K_16B_S       sampler_16_f2.dsp     44.1 kHz 16-bit stereo  176 KB/s
|||	                                                uncompressed
|||	    
|||	    SA_22K_16B_M_SQS2  sampler_sqs2_v1.dsp   22.05 kHz 16-bit mono   22 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_44K_16B_M_SQS2  sampler_sqs2_f1.dsp   44.1 kHz 16-bit mono    44 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_22K_16B_M_CBD2  sampler_cbd2_v1.dsp   22.05 kHz 16-bit mono   22 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_22K_16B_S_CBD2  sampler_cbd2_v2.dsp   22.05 kHz 16-bit stereo 44 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_44K_16B_M_CBD2  sampler_cbd2_f1.dsp   44.1 kHz 16-bit mono    44 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_44K_16B_S_CBD2  sampler_cbd2_v2.dsp   44.1 kHz 16-bit stereo  88 KB/s
|||	                                                compressed 2:1
|||	    
|||	    SA_22K_16B_M_ADP4  sampler_adp4_v1.dsp   22.05 kHz 16-bit mono   11 KB/s
|||	                                                compressed 4:1
|||	    
|||	    SA_44K_16B_S_ADP4  sampler_adp4_v1.dsp   44.1 kHz 16-bit mono    22 KB/s
|||	                                                compressed 4:1
|||	    
|||	  Arguments
|||	
|||	    -cs <chunksize>
|||	        Size in bytes of output SNDS data chunks. Only numeric characters
|||	        are allowed. (Good: 4096.  Bad: 4K.  Bad: 4,096.)  Default: 16384
|||	
|||	    -i <infile>
|||	        Input AIFF or AIFC file name.
|||	
|||	    -ia <amp>
|||	        Initial amplitude. Can be changed with a control message while 
|||	        the stream is playing.  Default: 0x7500
|||	
|||	    -ip <pan>
|||	        Initial pan. Can be changed with a control message while the 
|||	        stream is playing.  Default: 0x4000
|||	
|||	    -lc <chan>
|||	        Specifies the logical channel number of the chunkified output
|||	        file. The Audio Subscriber supports up to eight logical channels
|||	        per stream. Audio streams on different logical channels can be
|||	        of different AIFF or AIFC audio file formats.
|||	        Each channel maintains its own amplitude and pan values.
|||	        Generally one of these channels is considered the clock
|||	        channel and the timestamps in its data chunks drive the stream
|||	        clock. 0 is the default clock channel. The clock channel can be
|||	        set while the stream is playing with an SAudioSubscriber
|||	        control message.  Default: 0
|||	
|||	    -nb <num>
|||	        Maximum number of SNDS chunks that can be submitted to the audio
|||	        folio at any one time. Any additional SNDS chunks are queued up 
|||	        separately and submitted to the folio as previously submitted
|||	        buffers complete.  Default: 8
|||	
|||	    -o <outfile>
|||	        Output chunkified audio file name.
|||	
|||	  Caveats
|||	
|||	    In order to play an audio-only stream correctly, the chunkified
|||	    audio file MUST be processed by the Weaver.
|||	
|||	  Example
|||	
|||	    AudioChunkifier -cs 4096 -i soundfile.aiff -o soundfile.snds
|||	
 *********************************************************************************/


#define PROGRAM_VERSION_STRING	"2.5"

#include <StdLib.h>
#include <StdIO.h>
#include <Memory.h>
#include <String.h>
#include <Types.h>
#include <CursorCtl.h>
#include <AIFF.h>

#include <streaming/dsstreamdefs.h> /* for SUBS_CHUNK_COMMON */

/**********************************************/
/* Macro to even up AIF chunk sizes 		  */
/**********************************************/
#define EVENUP(n) if (n & 1) n++


#define 	SAUDIO_STREAM_VERSION_0 	0			/* stream version */

#define 	SHDR_CHUNK_HDR_SIZE 		(sizeof(SAudioHeaderChunk))

#define 	COMPRESSION_TYPE_NONE		'NONE'
#define 	COMPRESSION_TYPE_SQD2		'SQD2'
#define 	COMPRESSION_TYPE_CBD2		'CBD2'	/* M2 only: Cuberoot delta exact 2:1 compression */
#define 	COMPRESSION_TYPE_SQS2		'SQS2'	/* M2 only: Squareroot delta exact shifted 2:1 compression */

#define 	COMPRESSION_TYPE_ADP4		'ADP4'

#define 	AUDIO_TICK_RATE 			44100.0/184.0
#define		STARTING_TICK_OFFSET		1

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
long	gPropChunkSize			= (16 * 1024L); 		/* stream chunk size */
long	gLogicalChanNum 		= 0;					/* Which logical channel to use for this data */
long	gNumBuffers 			= 8;					/* max # of buffers that can be queued to the AudioFolio	*/
long	gInitialAmplitude		= 0x7500;				/* initial volume of the stream */
long	gInitialPan 			= 0x4000;				/* initial pan of the stream */
char*	gOutputFileName 		= NULL; 				/* no default output file name */
char*	gInputFileName			= NULL; 				/* no default input file name */
long	gIOBufferSize			= (32 * 1024L); 		/* bigger I/O buffers for MPW.	Speeds up processing */

char*	gProgramNameStringPtr	= NULL; 				/* program name string ptr for usage() */

/****************************/
/* Local routine prototypes */
/****************************/
static void 	usage( void );
static char*	GetIOBuffer( long bufferSize );
static Boolean	ParseCommandLine ( int argc, char **argv );


/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
void usage( void )
	{
	printf("Summary: Reads AIFF or AIFF-C soundfiles and writes audio streams\n");
	printf("Usage: %s -i <filename> -o <filename>\n", gProgramNameStringPtr );
	printf("Version %s\n",PROGRAM_VERSION_STRING);
	printf( "\t\t-cs\t\tsize of a stream data chunk in BYTES (16384)\n" );
	printf( "\t\t-lc\t\twhich logical channel to use for this stream (0)\n" );
	printf( "\t\t-nb\t\tnumber of AudioFolio buffers to use (8)\n" );
	printf( "\t\t-ia\t\tinitial amplitude (0x7500)\n" );
	printf( "\t\t-ip\t\tinitial pan (0x4000)\n" );
	printf( "\t\t-i\t\t\tinput file name\n" );
	printf( "\t\t-o\t\t\toutput file name\n" );
	}

/*********************************************************************************
 * Main code to parse AIFF/AIFC chunks and write stream data.
 *********************************************************************************/
int main( int argc, char* argv[] )
	{
	long				result;

	FILE*				inputFile			= NULL;
	FILE*				outputFile			= NULL;
	char*				bufferPtr;

	long				currFilePos;
	long				endSSNDChunkPos;

	ExtCommonChunk		myCommChunk;
	long				AIFFormHeader[3];
	long				AIFChunkHeader[2];
	long				SSNDChunkSize;

	SAudioDataChunkPtr		dcp;
	SAudioHeaderChunkPtr		hcp;

	long				ChunkSize			=	0;
	long				AudioFrameCount		=	0;
	long				FramesPChunk		=	0;
	long				SampleDataSize		=	0;

	long				BytesRead			=	0;
	long				BytesLeft			=	0;

	long				CompressionType 	=	COMPRESSION_TYPE_NONE;
	long				CompressionRatio	=	1;

	InitCursorCtl(NULL);

	/* Copy program name string pointer for the usage() routine
	 */
	gProgramNameStringPtr = argv[0];

	if ( argc < 3 )
		{
		usage();
		goto ERROR_EXIT;
		}

	/* Parse command line and check results */

	if ( !ParseCommandLine(argc, argv))
		{
		usage();
		goto ERROR_EXIT;
		}

	if ( gInputFileName == NULL )
		{
		usage();
		printf( "error: input file name not specified\n" );
		goto ERROR_EXIT;
		}

	if ( gOutputFileName == NULL )
		{
		usage();
		printf( "error: output file name not specified\n" );
		goto ERROR_EXIT;
		}

	if ( gPropChunkSize%4 != 0 )
		{
		printf( "error: data chunk size must be a multiple of 4\n" );
		goto ERROR_EXIT;
		}

	/* Open the input and output files */

	inputFile = fopen( gInputFileName, "rb" );
	if ( ! inputFile )
		{
		printf( "error: could not open the input file = %s\n", gInputFileName );
		goto ERROR_EXIT;
		}

	if ( setvbuf( inputFile, GetIOBuffer( gIOBufferSize ), _IOFBF, gIOBufferSize ) )
		{
		printf ( "setvbuf() failed for file: %s\n", gInputFileName );
		goto ERROR_EXIT;
		}

	outputFile = fopen( gOutputFileName, "wb" );
	if ( ! outputFile )
		{
		printf( "error: could not open the output file = %s\n", gOutputFileName );
		goto ERROR_EXIT;
		}

	if ( setvbuf( outputFile, GetIOBuffer( gIOBufferSize ), _IOFBF, gIOBufferSize ) )
		{
		printf ( "setvbuf() failed for file: %s\n", gOutputFileName );
		goto ERROR_EXIT;
		}


	/* Check the AIF form header for the input file */
	BytesRead = fread ((char *) &AIFFormHeader, 1, 12, inputFile);
	if((AIFFormHeader[2] != AIFFID) && (AIFFormHeader[2] != AIFCID))
		{
		printf( "error: %s is not an AIFF or AIFC file\n", gInputFileName );
		goto ERROR_EXIT;
		}

	/* Search for the COMM chunk */
	BytesLeft = AIFFormHeader[1] - 4;  /* subtract the length of the formType field */

	while (BytesLeft > 0)
		{
		SpinCursor(32);

		BytesRead = fread ((char *) &AIFChunkHeader, 1, 8, inputFile);
		BytesLeft -= BytesRead;
		if (AIFChunkHeader[0] != CommonID)
			{
			EVENUP(AIFChunkHeader[1]);									/* round up chunk size to and even number per aif spec */
			result = fseek(inputFile, AIFChunkHeader[1] , SEEK_CUR);	/* point to next chunk header */
			BytesLeft -= AIFChunkHeader[1]; 							/* update size */
			}
		else
			{
			/* read in the COMM chunk data and leave the loop */
			BytesRead = fread ((char *) &myCommChunk.numChannels, 1, sizeof(ExtCommonChunk) - 8, inputFile);
			break;
			}
		}

	/* Figure out compression type and ratio from AIF COMM chunk info */
	if ( (AIFFormHeader[2] == AIFCID) 
			&& (myCommChunk.compressionType != COMPRESSION_TYPE_SQD2)
			&& (myCommChunk.compressionType != COMPRESSION_TYPE_CBD2)
			&& (myCommChunk.compressionType != COMPRESSION_TYPE_SQS2)
		    && (myCommChunk.compressionType != COMPRESSION_TYPE_ADP4)
			&& (myCommChunk.compressionType != COMPRESSION_TYPE_NONE) )
		{
		printf( "error: Unrecognized compression type or invalid AIFC header.\n");
		goto ERROR_EXIT;
		}
	else
		/* Figure out compression type and ratio from AIF COMM chunk info */
		switch ( myCommChunk.compressionType )
		{
			case COMPRESSION_TYPE_SQD2:
				CompressionType = COMPRESSION_TYPE_SQD2;
				CompressionRatio = 2;
				break;

			case COMPRESSION_TYPE_CBD2:
				CompressionType = COMPRESSION_TYPE_CBD2;
				CompressionRatio = 2;
				break;

			case COMPRESSION_TYPE_SQS2:
				CompressionType = COMPRESSION_TYPE_SQS2;
				CompressionRatio = 2;
				break;

			case COMPRESSION_TYPE_ADP4:
				CompressionType = COMPRESSION_TYPE_ADP4;
				CompressionRatio = 4;
				break;
		} /* switch */

	if ( (myCommChunk.sampleSize != 8) && (myCommChunk.sampleSize != 16) )
		{
		printf( "error: Sample size must be 8 or 16 bits.\n");
		goto ERROR_EXIT;
		}

	if ( ((long)myCommChunk.sampleRate != 44100) && ((long)myCommChunk.sampleRate != 22050) )
		{
		printf( "error: Sample rate must be 44100 or 22050.\n");
		goto ERROR_EXIT;
		}

	if ( ((long)myCommChunk.numChannels != 1) && ((long)myCommChunk.numChannels != 2) )
		{
		printf( "error: Sample must be mono or stereo.\n");
		goto ERROR_EXIT;
		}

	SampleDataSize = gPropChunkSize - SAUDIO_DATA_CHUNK_HDR_SIZE; /* defined in dsstreamdefs.h */
	ChunkSize = gPropChunkSize;
	FramesPChunk =  (SampleDataSize * CompressionRatio) / (myCommChunk.numChannels * myCommChunk.sampleSize/8);

	/* Now Search for the SSND chunk */
	result = fseek(inputFile, 12, SEEK_SET);	/* point back to first chunk after FORM chunk */
	BytesLeft = AIFFormHeader[1] - 4;			/* reset size since we're pointing back to the beginning */

	while (BytesLeft > 0)
		{
		SpinCursor(32);

		BytesRead = fread ((char *) &AIFChunkHeader, 1, 8, inputFile);
		BytesLeft -= BytesRead;
		if (AIFChunkHeader[0] != SoundDataID)
			{
			EVENUP(AIFChunkHeader[1]);
			result = fseek(inputFile, AIFChunkHeader[1] , SEEK_CUR);
			BytesLeft -= AIFChunkHeader[1];
			}
		else
			{
			result = fseek(inputFile, 8 , SEEK_CUR);	/* skip to the beginning of the sample data */
			SSNDChunkSize = AIFChunkHeader[1] - 8;		/* account for offset and blocksize fields */
			BytesLeft = SSNDChunkSize;					/* set up BytesLeft for reading just the sample data */
			break;
			}
		}


	/* Allocate the I/O buffer */
	bufferPtr = malloc( ChunkSize );
	if ( bufferPtr == 0 )
		{
		printf( "error: could not allocate I/O block memory size = %ld\n", ChunkSize );
		goto ERROR_EXIT;
		}

	/*******************************************************************************************
	 * Write a header chunk into the output stream file using our AIFF/C header data
	 ******************************************************************************************/

	hcp = (SAudioHeaderChunkPtr) bufferPtr;

	hcp->chunkType						= SNDS_CHUNK_TYPE;
	hcp->chunkSize						= SHDR_CHUNK_HDR_SIZE;
	hcp->time							= 0;							/* this is setup data */
	hcp->channel						= gLogicalChanNum;
	hcp->subChunkType					= SHDR_CHUNK_TYPE;
	hcp->version						= SAUDIO_STREAM_VERSION_0;
	hcp->numBuffers 					= gNumBuffers;					/* max # of buffers that can be queued to the AudioFolio	*/
	hcp->initialAmplitude				= gInitialAmplitude;			/* initial volume of the stream */
	hcp->initialPan 					= gInitialPan;					/* initial pan of the stream */
	hcp->sampleDesc.dataFormat			= 0;							/* currently unused */
	hcp->sampleDesc.sampleSize			= myCommChunk.sampleSize;		/* bits per sample, typically 16 */
	hcp->sampleDesc.sampleRate			= (long)myCommChunk.sampleRate; /* 44100 or 22050 */
	hcp->sampleDesc.numChannels 		= myCommChunk.numChannels;		/* channels per frame, 1 = mono, 2=stereo */
	hcp->sampleDesc.compressionType 	= CompressionType;				/* eg. SQD2 */
	hcp->sampleDesc.compressionRatio	= CompressionRatio;

	/* Total number of samples in sound file:
	 * Number of bytes in AIFF/AIFC SSND chunk i.e. (SSNDChunkSize)
	 * divided by number of bytes per sample i.e. ((myCommChunk.sampleSize / CompressionRatio) / 8)
	 */
	hcp->sampleDesc.sampleCount 		= (long)( (float)SSNDChunkSize / (((float)myCommChunk.sampleSize 
														/ (float)CompressionRatio) / 8) ); 
	
	if (1 != fwrite( bufferPtr, SHDR_CHUNK_HDR_SIZE, 1, outputFile ) )
		{
		printf("error: header chunk write failed!\n");
		goto ERROR_EXIT;
		}

	/*******************************************************************************************
	 * Read in samps, format into stream chunks, and write into stream file.
	 ******************************************************************************************/
	currFilePos = ftell(inputFile); 				/* figure out where we are */
	endSSNDChunkPos = currFilePos + BytesLeft;		/* store end of sample data */

	while (currFilePos < endSSNDChunkPos)
		{
		SpinCursor(32);

		dcp = (SAudioDataChunkPtr) bufferPtr;

		/* Init the SSMP chunk header */
		dcp->chunkType			= SNDS_CHUNK_TYPE;
		dcp->chunkSize			= ChunkSize;
		dcp->channel			= gLogicalChanNum;
		dcp->subChunkType		= SSMP_CHUNK_TYPE;

		dcp->time = (((double)AudioFrameCount / (double)myCommChunk.sampleRate) * AUDIO_TICK_RATE) + 0.5;
		dcp->time += STARTING_TICK_OFFSET;		

		BytesRead = fread (&dcp->samples[0], 1, SampleDataSize, inputFile);
		dcp->actualSampleSize = BytesRead;
		currFilePos += BytesRead;

		AudioFrameCount += FramesPChunk;

		if (currFilePos < endSSNDChunkPos)		/* normal chunk  */
			{
			if (1 != fwrite( bufferPtr, ChunkSize, 1, outputFile ) )
				{
				printf("error: data chunk write failed!\n");
				goto ERROR_EXIT;
				}
			}
		else
			{
			/* This is the last chunk we will write and it will almost certainly
			 * be smaller than the "standard" size we computed earlier so we must
			 * round this one chunk size up by hand for quadbyte alignment. 
			 */
			dcp->chunkSize	= (SAUDIO_DATA_CHUNK_HDR_SIZE + BytesRead + 3) & ~3;
			if (1 != fwrite( bufferPtr, dcp->chunkSize, 1, outputFile ) ) /* out of data */
				{
				printf("error: data chunk write failed!\n");
				goto ERROR_EXIT;
				}
			}
		}

		fclose(inputFile);
		fclose(outputFile);

		return 0;	/* successful completion */

ERROR_EXIT:

		if (inputFile) {
			fclose(inputFile);
		}

		if (outputFile) {
			fclose(outputFile);
		}

		remove(gOutputFileName);

		return 1;

	} /* main */


/*******************************************************************************************
 * Routine to allocate alternate I/O buffers for the weaving process. Bigger is better!
 *******************************************************************************************/
static char*	GetIOBuffer( long bufferSize )
	{
	char*	ioBuf;

	ioBuf = (char*) NewPtr( bufferSize );
	if ( ioBuf == NULL )
		{
		fprintf( stderr, "GetIOBuffer() - failed. Use larger MPW memory partition!\n" );
		fprintf( stderr, "                Use Get Info... from the Finder to set it.\n" );
		}

	return ioBuf;
	}

/*********************************************************************************
 * Routine to parse command line arguments
 *********************************************************************************/
#define PARSE_FLAG_ARG( argString, argFormat, argVariable)	\
	if ( strcmp( *argv, argString ) == 0 )				\
		{												\
		argv++; 										\
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
		PARSE_FLAG_ARG( "-cs", "%li", gPropChunkSize );

		PARSE_FLAG_ARG( "-lc", "%li", gLogicalChanNum );

		PARSE_FLAG_ARG( "-nb", "%li", gNumBuffers );

		PARSE_FLAG_ARG( "-ia", "%li", gInitialAmplitude );

		PARSE_FLAG_ARG( "-ip", "%li", gInitialPan );

		PARSE_FLAG_ARG( "-iobs", "%li", gIOBufferSize );

		if ( strcmp( *argv, "-i" ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gInputFileName = *argv++;
			continue;
			}

		if ( strcmp( *argv, "-o" ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gOutputFileName = *argv++;
			continue;
			}

		/* Unknown flag encountered
		 */
		else
			return false;
		}

	return true;
	}
