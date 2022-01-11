/* @(#) MPEGAudioChunkifier.c 96/05/13 1.5 */
/* file: MPEGAudioChunkifier.c */
/* DataStreamer chunkifier for MPEG audio streams */
/* 3/16/96 George Mitsuoka */
/* The 3DO Company Copyright 1996 */

/**
|||	AUTODOC -public -class Streaming_Tools -group MPEG -name MPEGAudioChunkifier
|||	Chunkifies an MPEG-1 Audio bitstream file.
|||	
|||	  Format
|||	
|||	    MPEGAudioChunkifier [ -i <inputFile> -o <outputFile> [ -s <startTime> ] ]
|||	  Description
|||	
|||	    This MPW tool chunkifies an MPEG-1 audio bitstream file. The MPEG-1
|||	    audio file MUST be encoded using 44.1khz sampling rate, layer 2
|||	    encoding, fixed bitrate (not free format), and must in all other
|||	    ways conform to the MPEG-1 audio bitstream syntax.
|||	
|||	  Arguments
|||	
|||	    If no arguments are given, the MPEGAudioChunkifier will prompt for
|||	    all paramters.
|||	
|||	    -i <inputFile>
|||	        Specifies the input MPEG-1 audio bitstream file to be chunkified.
|||	
|||	    -o <outputFile>
|||	        Specifies the output file to be created or overwritten. No
|||	        warning is given if the file exists.
|||	
|||	    -s <startTime>
|||	        Specifies the chunkified stream's start time in audio ticks.
|||	        If not specified, defaults to 0.
|||	
|||	  Examples
|||	
|||	    MPEGAudioChunkifier -i music.mpa -o music.mpa.chunk -s 240
|||	
|||	    Chunkifies the file "music.mpa" and places the result in the file
|||	    "music.mpa.chunk". The start time of the chunkified file is 240
|||	    audio ticks (approximately 1 second).
|||	
|||	    MPEGAudioChunkifier -i voice.mpa -o voice.mpa.chunk
|||	
|||	    Chunkifies the file "voice.mpa" and places the result in the file
|||	    "voice.mpa.chunk". The start time of the chunkified file is 0
|||	    audio ticks.
|||	
|||	    MPEGAudioChunkifier
|||	
|||	    With no arguments, the MPEGAudioChunkifier will prompt for the
|||	    input file, output file, and start time.
|||	
**/

#ifdef THINK_C
#include "mpeg.h"
#include "dsstreamdefs.h"
#else
#include <misc/mpeg.h>
#include <streaming/dsstreamdefs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 45678911234567892123456789312345678941234567895123456789612345678971234567 */

int32 ProcessArgs( int argc, char *argv[],char *inName, char *outName, uint32 *startTime );
int32 Chunkify( FILE *inFile, FILE *outFile, uint32 startTime );

#define FILEBUFFERSIZE (8*1024)

char inBuffer[ FILEBUFFERSIZE ];
char outBuffer[ FILEBUFFERSIZE ];

/* chunkify an MPEG audio stream */

main( int argc, char *argv[] )
{
	char inFileName[80],outFileName[80];
	FILE *inFile, *outFile;
	uint32 startTime;

	if( ProcessArgs( argc, argv,inFileName, outFileName, &startTime ) )
		goto bail;

	inFile = fopen( inFileName, "rb" );
	if( inFile == (FILE *) NULL )
	{
		fprintf( stderr, "couldn't open %s\n", inFileName );
		goto bail;
	}
	outFile = fopen( outFileName, "wb" );
	if( outFile == (FILE *) NULL )
	{
		fprintf( stderr, "couldn't open %s\n", outFileName );
		goto bailCloseInFile;
	}
	/* increase file buffering for performance */
	setvbuf( inFile, inBuffer, _IOFBF, FILEBUFFERSIZE );
	setvbuf( outFile, outBuffer, _IOFBF, FILEBUFFERSIZE );
	
	printf( "Chunkifying %s -> %s\n", inFileName, outFileName );
	
	if( Chunkify( inFile, outFile, startTime ) )
		goto bailCloseOutFile;

bailCloseOutFile:
	fclose( outFile );	
bailCloseInFile:
	fclose( inFile );
bail:
	return;
}

void prusage( char *commandName );

int32 ProcessArgs( int argc, char *argv[],char *inName, char *outName, uint32 *startTime )
{
	int32 arg, gotInName = 0L, gotOutName = 0L;
	
	/* default start time */
	*startTime = 0L;
	
	if( argc == 1L )
	{
		printf( "enter input filename -> \n" );
		scanf( "%s", inName );
		printf( "enter output filename -> \n" );
		scanf( "%s", outName );
		printf( "enter start time (audio ticks) -> \n" );
		scanf( "%lud", startTime );
		
		return( 0L );
	}
	for( arg = 1L; arg < argc; arg++ )
	{
		if( (strcmp( argv[ arg ], "-i" ) == 0L) && (++arg < argc) )
		{
			strcpy( inName, argv[ arg ] );
			gotInName = 1L;
		}
		else if( (strcmp( argv[ arg ], "-o") == 0L) && (++arg < argc) )
		{
			strcpy( outName, argv[ arg ] );
			gotOutName = 1L;
		}
		else if( (strcmp( argv[ arg ], "-s") == 0L) && (++arg < argc) )
		{
			sscanf( argv[ arg ], "%ld", startTime);
		}
		else
		{
			prusage( argv[ 0 ] );
			return( -1L );
		}
	}
	if( !gotOutName || !gotInName )
	{
		prusage( argv[ 0 ] );
		return( -1L );
	}
	return( 0L );
}

void prusage( char *commandName )
{
	printf("\nUsage:");
	printf("\n  %s [ -i <inputFile> -o <outputFile> [ -s <startTime> ] ]", commandName );
	printf("\n\n    if no arguments are given, %s will prompt for parameters.\n", commandName );
}

#define SAMPLESPERSEC 		44100.0
#define MPEGTICKSPERSEC		90000.0
#define SAMPLESPERFRAME		1152.0
#define MPEGTICKSPERFRAME	(MPEGTICKSPERSEC*SAMPLESPERFRAME/SAMPLESPERSEC)
#define AUDIO_TICK_RATE  	(44100.0/184.0)
#define AUDIOTICKSPERFRAME	(AUDIO_TICK_RATE*SAMPLESPERFRAME/SAMPLESPERSEC)

int32 MPEGAudioBitRate[] = {	0L,
								32000L,
								48000L,
								56000L,
								64000L,
								80000L,
								96000L,
								112000L,
								128000L,
								160000L,
								192000L,
								224000L,
								256000L,
								320000L,
								384000L };
									
int32 MPEGAudioSampleRate[] = {	44100L,
								48000L,
								32000L };

#define MAXENCODEDFRAMESIZE (8L*1024L)

uint8 encodedFrameBuf[ MAXENCODEDFRAMESIZE ];
int32 ValidateMPEGAudioHeader( AudioHeader *MPEGAudioHeader );

int32 Chunkify( FILE *inFile, FILE *outFile, uint32 time )
{
	MPEGAudioHeaderChunk streamHeader;
	MPEGAudioFrameChunk frameHeader;
	AudioHeader MPEGAudioHeader,nextMPEGAudioHeader;
	int32 result = 0L, length, inLength = 0L, outLength = 0L, frameCount = 0L;
	int32 bytesPerSecond, samplesPerSecond, bytesPerFrame;
	uint32 padBytes = 0L;

	/* read the mpeg audio header */
	result = fread( &MPEGAudioHeader, sizeof( AudioHeader ), 1L, inFile );
	if( result != 1L )
	{
		if( feof( inFile ) )
		{
			printf("done!\n");
			return( 0L );
		}
		fprintf( stderr, "error reading frame %ld header\n", frameCount );
		return( -1L );
	}
	inLength += sizeof( AudioHeader );
	
	/* validate the header */
	result = ValidateMPEGAudioHeader( &MPEGAudioHeader );
	if( result )
	{
		fprintf( stderr, "bad audio header on frame %ld\n", frameCount );
		return( result );
	}
	/* write the stream header chunk */
	streamHeader.chunkType = MPAU_CHUNK_TYPE;
	streamHeader.chunkSize = sizeof( MPEGAudioHeaderChunk );
	streamHeader.time = 0L;
	streamHeader.channel = 0L;
	streamHeader.subChunkType = MAHDR_CHUNK_SUBTYPE;
	streamHeader.version = 1L;
	streamHeader.audioHdr = MPEGAudioHeader;
	result = fwrite( &streamHeader, sizeof( MPEGAudioHeaderChunk ),
					 1L, outFile );
	if( result != 1 )
	{
		fprintf(stderr, "error writing audio header chunk\n");
		return( - 1L );
	}
	
	/* chunkify the rest of the stream */
	frameHeader.chunkType = MPAU_CHUNK_TYPE;
	frameHeader.channel = 0L;
	frameHeader.subChunkType = MA1FRME_CHUNK_SUBTYPE;
	while( 1 )
	{
		/* calculate length of this frame's encoded data */
		bytesPerSecond = MPEGAudioBitRate[ MPEGAudioHeader.bitrate_index ] / 8;
		samplesPerSecond = MPEGAudioSampleRate[ MPEGAudioHeader.sampling_frequency ];
		bytesPerFrame = bytesPerSecond * SAMPLESPERFRAME / samplesPerSecond;
		length = bytesPerFrame - sizeof( MPEGAudioHeader );
		if( MPEGAudioHeader.padding_bit )
			length += 1L;
			
		/* read frame data */
		result = fread( encodedFrameBuf, sizeof( uint8 ), length, inFile );
		if( result != length )
		{
			fprintf( stderr, "error reading frame %ld data\n", frameCount );
			return( result );
		}
		/* write chunk header */
		frameHeader.chunkSize = sizeof( MPEGAudioFrameChunk ) + sizeof( AudioHeader) + length - 4L;
		frameHeader.time = time + frameCount*AUDIOTICKSPERFRAME;

		result = fwrite( &frameHeader, sizeof( MPEGAudioFrameChunk ) - 4L, 1L, outFile );
		if( result != 1 )
		{
			fprintf(stderr, "error writing frame header %ld\n",frameCount);
			return( - 1L );
		}
		/* write the MPEG audio header */
		result = fwrite( &MPEGAudioHeader, sizeof( AudioHeader ), 1L, outFile );
		if( result != 1 )
		{
			fprintf(stderr, "error writing MPEG audio header %ld\n",frameCount);
			return( - 1L );
		}
		/* write data */
		result = fwrite( encodedFrameBuf, 1L, length, outFile );
		if( result != length )
		{
			fprintf(stderr, "error writing chunk %ld data\n",frameCount);
			return( - 1L );
		}
		/* pad the chunk if necessary */
		length = 4L - (frameHeader.chunkSize % 4L);
		if( length != 4L )
		{
			result = fwrite( &padBytes, 1L, length, outFile );
			if( result != length )
			{
				fprintf(stderr, "error writing pad bytes on frame %ld\n",frameCount);
				return( - 1L );
			}
		}
		frameCount++;
		/* print out a progress dot about once every seconds worth of encoded data */
		if( (frameCount % 40) == 0L )
		{
			printf(".");
			fflush( stdout );
		}
		/* put a new line after every 40 dots */
		if( (frameCount % 1600) == 0L )
			printf("\n");
			
		/* read next MPEG audio header */
		result = fread( &MPEGAudioHeader, sizeof( AudioHeader ), 1L, inFile );
		if( result != 1L )
		{
			if( feof( inFile ) )
			{
				printf("\ndone!\n");
				return( 0L );
			}
			fprintf( stderr, "error reading frame %ld header\n", frameCount );
			return( -1L );
		}
		inLength += sizeof( AudioHeader );
		
		/* validate the header */
		result = ValidateMPEGAudioHeader( &MPEGAudioHeader );
		if( result )
		{
			fprintf( stderr, "bad audio header on frame %ld\n", frameCount );
			return( result );
		}
	}
}

int32 ValidateMPEGAudioHeader( AudioHeader *hdr )
{
	if( hdr->syncword != MPEG_AUDIO_SYNCWORD )
	{
		fprintf( stderr, "bad syncword 0x%03x\n", hdr->syncword );
		return( -1L );
	}
	if( hdr->id != 1 )
	{
		fprintf( stderr, "bad id\n" );
		return( -1L );
	}
	if( hdr->layer != audioLayer_II )
	{
		fprintf( stderr, "bad layer %d\n", hdr->layer );
		return( -1L );
	}
	if( (hdr->bitrate_index < 1) || (hdr->bitrate_index > 0xe) )
	{
		fprintf( stderr, "bad bitrate index %d\n", hdr->bitrate_index );
		return( -1L );
	}
	if( hdr->sampling_frequency != aFreq_44100_Hz )
	{
		fprintf( stderr, "bad sampling frequency %d\n", hdr->sampling_frequency );
		return( -1L );
	}
	return( 0L );
}


