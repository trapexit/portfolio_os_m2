/****************************************************************************
**
**  @(#) DATAChunkify.c 96/11/20 1.5
**
**  MPW tool to chunkify files for the DATA subscriber
**
*****************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <types.h>

#include <OSUtils.h>
#include <cursorctl.h>

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <streaming/dsstreamdefs.h>
#endif

#include "getconfig.h"
#include "DATAChunkify.h"

/* if building MPW tool with metrowerks, turn on MPW style newlines */
#if ( defined(__MWERKS__) && defined(macintosh) )
	#pragma mpwc_newline on
#endif

/* private function prototypes */
static	bool	IsValidCompressor(uint32 compressor);
static	void	PrintValidCompressors(void);
static	void 	Usage(char *progName);
static	void	ReportArgValues(ChunkParamsPtr params);
static	void	SetDefaultValues(ChunkParamsPtr params);
static	uint32	WriteData(ChunkParamsPtr params, int32 dataBytesToWrite, uint32 *dataPtr);
static uint32	ChecksumBuffer(uint32 *buffPtr, uint32 buffSize);

/* ---------------- Command line switch strings ----------------------- */
/* required params */
#define	kOutFilenameStr		"-o"			// output file name
#define	kInFileNameStr		"-i"			// text file name

// optional params
#define	kChunkSizeStr		"-cs"			// chunk size
#define	kChannelNumStr		"-chan"			// switch for specifying channel number
#define	kChunkTimeStr		"-t"			// stream time for first chunk
#define	kTimeDeltaStr		"-to"			// offset time for each chunk
#define	kMemTypeStr			"-m"			// mem type bits
#define	kUserDataStr		"-u"			// user data 1
#define	kCompressorStr		"-comp"			// compressor

#define	kVerboseStr			"-v"			// verbose output

#define kMinArgCount		(2 * 2L)		// require in and out names only
#define kMaxArgCount		(9 * 2L)

#define	kToolVersionNum		"1.0"

#define	kDfltChunkSize		(16 * 1024L)
#define	kDfltChannel		0L
#define	kDfltTime			0L
#define	kDfltDeltaTime		0L
#define	kDfltMemTypeBits	0L
#define	kDfltCompressor		DATA_NO_COMPRESSOR

/* ---------------- typedefs/globals ----------------------- */


bool			gVerbose = false;
ChunkParams		gChunkParams;
OptConvertRule	gArgConvRules[] =
{
	{kInFileNameStr,	STRING_TYPE,		0,	&gChunkParams.inputFileName },
	{kOutFilenameStr, 	STRING_TYPE,		0,	&gChunkParams.outputFileName },

	{kChunkSizeStr,		INT32_TYPE,			0,	&gChunkParams.chunkSize },
	{kChannelNumStr,	INT32_TYPE,			0,	&gChunkParams.channelNum },

	{kChunkTimeStr,		INT32_TYPE,			0,	&gChunkParams.firstChunkTime },
	{kTimeDeltaStr,		INT32_TYPE,			0,	&gChunkParams.timeDelta },
	{kMemTypeStr,		UINT32_TYPE,		0,	&gChunkParams.memTypeBits },
	{kUserDataStr,		COPY_T0_BUFFER,		8,	&gChunkParams.userData },
	{kCompressorStr,	COPY_T0_BUFFER,		4,	&gChunkParams.compressor },

	{kVerboseStr,		BOOL_TOGGLE_VAL_TYPE,0,	&gVerbose },

	{NULL,				0,					0,	NULL },
};


// struct which holds a compressor description
typedef struct _CompessorDescr	
{
	uint32		compessorTag;		/* compessor identifyer */
	char		*description;		/* description to print with help */
} CompessorDescr, *CompessorDescrPtr;

// table of all supported compressors
// !!!!! LAST COMPRESSOR ENTRY MUST BE 0 !!!!!
CompessorDescr		gKnownCompressors[] =
{
	{DATA_NO_COMPRESSOR,	"No compression."},
	{DATA_3DO_COMPRESSOR,	"Portfolio compression folio."},
	{0,						NULL}
};



/* handy macros */
#define	QUADBYTE_ALIGN(size) 			(((uint32)size + 3) & ~0x03)	// round chunk size up to long word boundary
#define	IS_QUADBYTE_ALIGNED(size)		((bool)(0 == (((uint32) size) & 0x03L)))

#ifndef SetFlag
#define SetFlag(val, flag)		((val)|=(flag))
#define ClearFlag(val, flag)	((val)&=~(flag))
#define FlagIsSet(val, flag)	((bool)(((val)&(flag))!=0))
#define FlagIsClear(val, flag)	((bool)(((val)&(flag))==0))
#endif

/******************************************************************************
|||	AUTODOC -public -class Streaming_Tools -group DATA_Subscriber -name DATAChunkify
|||	Chunkifies a data file for the DATA subscriber.
|||
|||	  Format -preformatted
|||
|||	    DATAChunkify <arg>*
|||	        -? or -help              print usage info.
|||	        -i       <file>          input file name [REQUIRED].
|||	        -o       <file>          output file name [REQUIRED].
|||	        -chan    <num>           channel number (default = 0).
|||	        -t       <ticks>         first chunk's time in audio folio ticks (0).
|||	        -to      <ticks>         time offset for subsequent chunks (0).
|||	        -cs      <size>          size of each data chunk in bytes (16384).
|||	        -m       <num>           memtype bits (0).
|||	        -u       <type><type>    user data word (0000000000000000).
|||	        -comp    <type>          compressor (NONE).
|||	    Valid compressors are:
|||	        NONE   No compression.
|||	        3DOC   3DO compession folio compression.
|||
|||	  Description
|||
|||	    This tool breaks files into chunks suitable for the DATA subscriber. 
|||	    The files output by this tool must be woven into a stream file with 
|||	    the Weaver tool in order to be used by the DataStreamer.
|||	    
******************************************************************************/

// +++++++++++++++++++++++++++++++++++++ here we go... +++++++++++++++++++++++++++++++++++++

/*
 * return a compressor description from the table of known compressors, or NULL if no
 *  match is found 
 */
static CompessorDescrPtr
GetCompressorDescr(uint32 compressor)
{
	CompessorDescr	*compDescr;
	
	for ( compDescr = gKnownCompressors; 0 != compDescr->compessorTag; compDescr++ )
		if ( compressor == compDescr->compessorTag )
			return compDescr;
	return NULL;
}

/*
 * see if a compressor is one of the ones we understand 
 */
static bool
IsValidCompressor(uint32 compressor)
{
	if ( NULL != GetCompressorDescr(compressor) )
		return true;
	return false;
}

 
/*
 * print the list of compressors
 */
static void
PrintValidCompressors(void)
{
	CompessorDescr	*compDescr;
	
	printf("Valid compressors are:\n");
	for ( compDescr = gKnownCompressors; 0 != compDescr->compessorTag; compDescr++ )
	{
		printf("   %.4s   %s\n", &compDescr->compessorTag, compDescr->description);
	}
	printf("\n");
}



/*
 * ChunkDataFile
 *  chunkify a mac file and write it out to the stream
 */
Err	
ChunkDataFile(ChunkParamsPtr params)
{
#define	BytesToWrite(chunkSize, bytesInBuff) ((bytesInBuff > chunkSize) ? chunkSize : bytesInBuff)

	FILE				*inFileSpec = NULL;
	uint32				*ioBuffer = NULL;
	char				*readPtr;
	uint32				inFileSize;
	uint32				blockSignature;
	uint32				streamTime;
	int32				bytesWritten;
	int32				bytesRemaining;
	Err					status;

	// first off, open the input file to make sure it exists
	inFileSpec = fopen(params->inputFileName, "rb");
	if ( NULL == inFileSpec )
	{
		printf("## ERROR: could not open the input file = %s\n", params->inputFileName);
		goto ERROR_EXIT;
	}
	
	// spin the cursor, let folks know we are working on it
	InitCursorCtl(NULL);
	SpinCursor(32);
	
	// first, sanity check the actual data size (including the header)
	if ( (QUADBYTE_ALIGN(params->chunkSize) - sizeof(DataChunkFirst)) < 4 )
	{
		printf("## ERROR: chunk size must be >= %ld characters in length (to allow for the"\
				" chunk header)\n", 
					sizeof(DataChunkFirst) + 4);
		goto ERROR_EXIT;
	}

	// make sure the data was compressed with a valid compressor
	if ( false == IsValidCompressor(params->compressor) )
	{
		printf("## ERROR: unknown compressor!\n");
		PrintValidCompressors();
		goto ERROR_EXIT;
	}

	// see how large the file is, allocate a buffer for it
	fseek(inFileSpec, 0, SEEK_END);
	inFileSize = ftell(inFileSpec);
	fseek(inFileSpec, 0, SEEK_SET);
	
	// alloc our IO buffer
	if ( NULL == (ioBuffer = (uint32 *)malloc(inFileSize)) )
	{
		printf("## ERROR: could not allocate io buffer (size = %ld)\n", inFileSize);
		status = -1;
		goto ERROR_EXIT;
	}

	// now read the file into memory
	if ( (fread(ioBuffer, 1, inFileSize, inFileSpec)) != inFileSize )
	{
		printf("## ERROR: fread() failed reading input file!\n");
		goto ERROR_EXIT;
	}

	// if the data is compressed, make sure we remember how much memory must be
	//  allocated at runtime (NOT just the amount of memory required for the 
	//  compressed data)
	if ( DATA_NO_COMPRESSOR != params->compressor )
	{
		// !!!!!!!!!!!!   
		//  NOTE: we will probably need to come up with some other method of
		//  determining the uncompressed data size when we support other 
		//  decompressors, but for now, the "comp3DO" tool writes the uncompressed
		//  size as the first long word of the compressed file.  read the length
		//  and advance the buffer past the length word
		// !!!!!!!!!!!!   
		params->uncompressedSize = *ioBuffer;
		bytesRemaining = inFileSize - sizeof(uint32);
		readPtr = (char *)(ioBuffer + 1);
		
		if ( false == IS_QUADBYTE_ALIGNED(params->chunkSize) )
		{
			params->chunkSize = QUADBYTE_ALIGN(params->chunkSize);
			printf("## WARNING: the %.4s compressor REQUIRES chunks to be quad byte aligned.\n"
					"##         Your chunk sized has been rounded up to %ld bytes.\n", 
					&((GetCompressorDescr(DATA_3DO_COMPRESSOR))->compessorTag), params->chunkSize);
		}
	}
	else
	{
		bytesRemaining = inFileSize;
		readPtr = (char *)ioBuffer;
	}

	// calculate the block's unique signature.  This value is used in two ways:
	//  - to ensure that every chunk recieved by the subscriber is added to the
	//    correct block
	//  - when compiled with the DEBUG on, the subscriber calculates a checksum 
	//    of the entire block as it recieves it so that it can check for data
	//    corruption by comparing the calcualted checksum with the one stored
	//    in the stream file
	blockSignature = ChecksumBuffer((uint32 *)readPtr, bytesRemaining);

	// init and write out the first chunk...
	bytesWritten = Write1stDataChunk(params,
						BytesToWrite(params->chunkSize - sizeof(DataChunkFirst), bytesRemaining) , 
						(uint32 *)readPtr,
						blockSignature);
	if ( bytesWritten < 0 )
	{
		status = (Err)bytesWritten;
		goto ERROR_EXIT;
	}

	readPtr			+= bytesWritten;
	bytesRemaining	-= bytesWritten;
	streamTime		= params->firstChunkTime + params->timeDelta;
	while ( bytesRemaining > 0 )
	{
		bytesWritten = WriteNthDataChunk(params,
							streamTime,
							BytesToWrite(params->chunkSize - sizeof(DataChunkPart), bytesRemaining),
							(uint32 *)readPtr);
		if ( bytesWritten < 0 )
		{
			status = (Err)bytesWritten;
			goto ERROR_EXIT;
		}

		readPtr			+= bytesWritten;
		bytesRemaining	-= bytesWritten;
		streamTime		+= params->timeDelta;
	}

	// all data has been written, fixup the header chunk
	if ( (bytesWritten = FinalizeDataChunk(params)) < 0 )
	{
		status = (Err)bytesWritten;
		goto ERROR_EXIT;
	}
	
	// cool! guess things worked...
	status = 0;
	goto DONE;

ERROR_EXIT:
	if ( 0 == status )
		status = -666;

DONE:
	if ( NULL != inFileSpec )
		fclose(inFileSpec);

	if ( NULL != ioBuffer )
		free(ioBuffer);

	return status;
#undef	BytesToWrite
}


/*
 * generate an XOR checksum for a buffer
 */
static uint32
ChecksumBuffer(uint32 *buffPtr, uint32 buffSize)
{
	int32	numLongs = buffSize / sizeof(uint32);
	uint32	checkSum = 0;

	/* Calculate an XOR checksum of LONG words */
	while ( numLongs-- > 0 )
	{
		uint32	tempLong = *buffPtr++;
		checkSum ^= tempLong;
	}

	return checkSum;
}


/*
 * finish the data chunk.  assumes that ALL data has been written. 
 *   returns > 0 if successful, < 0 if an error
 */
uint32
FinalizeDataChunk(ChunkParamsPtr params)
{
	int32		currFileLoc;
	int32		err;

	// remember where we are in the output file since we will rewind to the header chunk
	currFileLoc = ftell(params->outputFile);

	// seek back to where we started in the file, rewrite the header since we now have all
	//  of the data needed
	params->data1stHeader.pieceCount	= params->dataChunksWritten;
	if ( DATA_NO_COMPRESSOR == params->compressor )
		params->data1stHeader.totalDataSize	= params->dataBytesWritten;
	else
		params->data1stHeader.totalDataSize	= params->uncompressedSize;
	fseek(params->outputFile, params->headerChunkLoc, SEEK_SET);
	if ( 1 != (err = fwrite(&params->data1stHeader, sizeof(DataChunkFirst), 1, params->outputFile)) )
	{
		printf("## ERROR: failed rewriting first data chunk %ld!\n");
		err = -1;
		goto ERROR_EXIT;
	}
	
	// reset the file position to the position we found it in
	fseek(params->outputFile, currFileLoc, SEEK_SET);

ERROR_EXIT:

	return err;
}



/*
 * write a buffer plus up to 3 bytes of filler to make the length a quad multiple.  returns < 0 if an 
 *  error, otherwise the number of bytes written
 */
static uint32
WriteData(ChunkParamsPtr params, int32 dataBytesToWrite, uint32 *dataPtr)
{
	int32		err;
	int32		fillerBytes;

	// write the data chunk
	if ( 1 != (err = fwrite(dataPtr, dataBytesToWrite, 1, params->outputFile)) )
	{
		printf("## ERROR: failed writing data chunk!\n");
		err = -1;
		goto ERROR_EXIT;
	}

	// we need to quad align every block of data written to disk.  thus, if the amount of 
	//  data just written is not modulo 4, write out enough bytes to pad it (these bytes
	//  are accounted for in the chunk header, but NOT in the data chunk "dataSize" field
	//  as they aren't actually part of the client's data)
	if ( (fillerBytes = (QUADBYTE_ALIGN(dataBytesToWrite) - dataBytesToWrite)) > 0 )
	{
		uint32		zero = 0;
		
		if ( 1 != (err = fwrite(&zero, fillerBytes, 1, params->outputFile)) )
		{
			printf("## ERROR: failed writing filler bytes!\n");
			err = -1;
		}
	}

	err = 1;

ERROR_EXIT:

	return err;
}


/*
 * write the data chunk header.  returns < 0 if an error, otherwise the number of bytes written
 */
uint32
Write1stDataChunk(ChunkParamsPtr params, uint32 dataBytesToWrite, uint32 *dataPtr, 
					uint32 fileCheckSum)
{
	int32		err = -1;
	int32		alignedBytesWritten;

	// make sure we quad align the amount of data we will write
	alignedBytesWritten = QUADBYTE_ALIGN(dataBytesToWrite);

	// the data subscribe expects every sequence of data blocks to have a unique signature. 
	//   use the checksum provided
	params->blockSignature = fileCheckSum;

	// set up the first chunk header with all of the information that we have now.  we'll fill
	//  in the rest when "FinalizeDataChunk()" is called
	memset(&params->data1stHeader, '\0', sizeof(DataChunkFirst));
	params->data1stHeader.chunkType			= DATA_SUB_CHUNK_TYPE;
	params->data1stHeader.chunkSize			= alignedBytesWritten + sizeof(DataChunkFirst);
	params->data1stHeader.time				= params->firstChunkTime;
	params->data1stHeader.channel			= params->channelNum;
	params->data1stHeader.subChunkType		= DATA_FIRST_CHUNK_TYPE;
	
	params->data1stHeader.userData[0]		= params->userData[0];
	params->data1stHeader.userData[1]		= params->userData[1];
	params->data1stHeader.signature			= params->blockSignature;
	params->data1stHeader.totalDataSize		= 666;					// we'll fill this in later
	params->data1stHeader.dataSize			= dataBytesToWrite;
	params->data1stHeader.pieceCount		= 666;					// we'll fill this in later
	params->data1stHeader.memTypeBits		= params->memTypeBits;
	params->data1stHeader.compressor		= params->compressor;

	// remember where we're starting in the output file so we can come back and patch up the 
	//  header chunk later
	params->headerChunkLoc = ftell(params->outputFile);

	// first write the header
	SpinCursor(32);
	if ( 1 != (err = fwrite(&params->data1stHeader, sizeof(DataChunkFirst), 1, params->outputFile)) )
	{
		printf("## ERROR: failed writing data chunk header!\n");
		err = -1;
		goto ERROR_EXIT;
	}

	// and now the buffer
	if ( 1 != (err = WriteData(params, dataBytesToWrite, dataPtr)) )
	{
		printf("## ERROR: failed writing first data chunk!\n");
		err = -1;
		goto ERROR_EXIT;
	}

	// internally
	params->dataBytesWritten	+= dataBytesToWrite;
	params->dataChunksWritten	= 1;

	/* return the number of bytes written FROM THE CALLER'S BUFFER, not the actual
	 *  number of bytes written to the file 
	 */
	err							= dataBytesToWrite;

ERROR_EXIT:

	return err;
}


/*
 * write a data chunk and header.  returns < 0 if an error, otherwise the number of bytes written
 */
Err	
WriteNthDataChunk(ChunkParamsPtr params, uint32 streamTime, uint32 dataBytesToWrite, uint32 *dataPtr)
{
	DataChunkPart	dataNthHeader;
	int32			alignedBytesWritten;
	int32			err;

	// make sure the block size is quad aligned 
	alignedBytesWritten = QUADBYTE_ALIGN(dataBytesToWrite);
	memset(&dataNthHeader, '\0', sizeof(DataChunkPart));
	
	dataNthHeader.chunkType			= DATA_SUB_CHUNK_TYPE;
	dataNthHeader.chunkSize			= alignedBytesWritten + sizeof(DataChunkPart);
	dataNthHeader.channel			= params->channelNum;
	dataNthHeader.time				= streamTime;
	dataNthHeader.subChunkType		= DATA_NTH_CHUNK_TYPE;

	dataNthHeader.signature			= params->blockSignature;
	dataNthHeader.dataSize			= dataBytesToWrite;
	dataNthHeader.pieceNum			= params->dataChunksWritten + 1;

	// first write the header
	SpinCursor(32);
	if ( 1 != (err = fwrite(&dataNthHeader, sizeof(DataChunkPart), 1, params->outputFile)) )
	{
		printf("## ERROR: failed writing header for data chunk %ld!\n", params->dataChunksWritten + 1);
		err = -1;
		goto ERROR_EXIT;
	}

	// and now the buffer
	if ( 1 != (err = WriteData(params, dataBytesToWrite, dataPtr)) )
	{
		printf("## ERROR: failed writing data chunk %ld!\n", params->dataChunksWritten + 1);
		err = -1;
		goto ERROR_EXIT;
	}

	params->dataBytesWritten	+= dataBytesToWrite;
	params->dataChunksWritten	+= 1;
	
	/* return the number of bytes written FROM THE CALLER'S BUFFER, not the actual
	 *  number of bytes written to the file 
	 */
	err							= dataBytesToWrite;

ERROR_EXIT:

	return err;
}



/*
 * ReportArgValues
 *	Report the values we're going to use
 */
static void
ReportArgValues(ChunkParamsPtr params)
{
	printf(" command line args are:\n");
	printf("  input file:     %s\n",	params->inputFileName);
	printf("  output file:    %s\n",	params->outputFileName);
	printf("  channel number: %ld\n",	params->channelNum);
	printf("  chunk size:     %ld\n",	params->chunkSize);
	printf("  chunk time:     %ld\n",	params->channelNum);
	printf("  time offset:    %ld\n",	params->timeDelta);
	printf("  mem type bits:  %ld\n",	params->memTypeBits);
	printf("  compressor:     %.4s\n",	&params->compressor);
	printf("  user data[0]:   %.4s\n",	&params->userData[0]);
	printf("  user data[1]:   %.4s\n",	&params->userData[1]);
}


/*
 * output command line help to stdout
 */
static void 
Usage(char *progName)
{
	ChunkParams	params;
	SetDefaultValues(&params);
	
	printf( "%s version %s\n", progName, kToolVersionNum);
	printf("Usage: %s <flags>\n", progName);
	printf("    %-8s <file>          input file name [REQUIRED]\n", kInFileNameStr);
	printf("    %-8s <file>          output file name [REQUIRED]\n", kOutFilenameStr);
	printf("    %-8s <num>           channel number (%ld)\n", kChannelNumStr, params.channelNum);
	printf("    %-8s <ticks>         first chunk time in audio folio ticks (%ld)\n", kChunkTimeStr, params.firstChunkTime);
	printf("    %-8s <ticks>         time offset for subsequent chunks (%ld)\n", kTimeDeltaStr, params.timeDelta);
	printf("    %-8s <size>          size of a data chunk in BYTES (%ld)\n", kChunkSizeStr, params.chunkSize);
	printf("    %-8s <num>           memtype bits (%ld)\n", kMemTypeStr, params.memTypeBits);
	printf("    %-8s <type><type>    user data word (%.8ld%.8ld)\n", kUserDataStr, params.userData[0], params.userData[1]);
	printf("    %-8s <type>          compressor (%.4s)\n", kCompressorStr, &params.compressor);
	
	PrintValidCompressors();
}


/*
 * set default values for the chunking parameters 
 */
static void
SetDefaultValues(ChunkParamsPtr params)
{
	params->userData[0]		= 0;
	params->userData[1]		= 0;

	params->channelNum		= kDfltChannel;
	params->chunkSize		= kDfltChunkSize;
	params->firstChunkTime	= kDfltTime;
	params->timeDelta		= kDfltDeltaTime;
	params->compressor		= kDfltCompressor;
	params->memTypeBits		= kDfltMemTypeBits;

	params->inputFileName	= NULL;
	params->outputFileName	= NULL;
	
}



int	
main(int argc, char **argv)
{
	int			success;
	char		*progName = argv[0];
	bool		verbose = false;

	// asking for help?
	if ( (NULL != argv[1]) && 
			((0 == strcmp("-help",argv[1])) || (0 == strcmp("-?",argv[1]))) )
	{
		Usage(progName);
		goto ERROR_EXIT;
	}

	// collect up the command line arguments
	SetDefaultValues(&gChunkParams);
	if ( false == GetConfigSettings(&argc, argv, gArgConvRules, &verbose, 
									kMinArgCount, kMaxArgCount) )
	{
		Usage(progName);
		goto ERROR_EXIT;
	}
	
	// and make sure we got at least the in and out names
	if ( (NULL == gChunkParams.outputFileName) || (NULL == gChunkParams.inputFileName) )
	{
		printf("## ERROR: input and output file names required.\n");
		Usage(progName);
		goto ERROR_EXIT;
	}
	
	// now the output file.  ("wb" mode deletes anything with the same name)
	gChunkParams.outputFile = fopen(gChunkParams.outputFileName, "wb");
	if ( NULL == gChunkParams.outputFile )
	{
		printf("## ERROR: could not open the output file \"%s\" (%ld).\n", gChunkParams.outputFileName, 
					(int32)errno);
		goto ERROR_EXIT;
	}

	// OK, we've got what we need to attemp writing the new file - so do it
	if ( 0 != ChunkDataFile(&gChunkParams) )
		goto ERROR_EXIT;

	// all's well, tell em so
	success = 0;
	
DONE:

	if ( NULL != gChunkParams.outputFile )
		fclose(gChunkParams.outputFile);
		
	return success;

ERROR_EXIT:
	// something terrible happened.  clean up after ourselves and bail
	success = 1;

	if ( NULL != gChunkParams.outputFileName )
		remove(gChunkParams.outputFileName);
	
	goto DONE;
}

