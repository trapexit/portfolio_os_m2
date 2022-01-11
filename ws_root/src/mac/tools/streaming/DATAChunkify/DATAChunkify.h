/****************************************************************************
**
**  @(#) DATAChunkify.h 96/11/20 1.3
**
**  MPW tool to chunkify files for the DATA subscriber
**
*****************************************************************************/
 
 
#ifndef __STREAMING_DATACHUNKIFY_H
#define __STREAMING_DATACHUNKIFY_H

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <streaming/dsstreamdefs.h>
#endif

#ifndef __STREAMING_DATASUBSCRIBER_H__
	#include <streaming/datasubscriber.h>
#endif

/*
 * Command line variable structure
 */
typedef struct _ChunkParams
{
	uint32	userData[2];

	int32	channelNum;
	int32	chunkSize;
	
	int32	firstChunkTime;
	int32	timeDelta;
	uint32	compressor;
	uint32	memTypeBits;

	char	*inputFileName;
	char	*outputFileName;
	
	/* don't muck with fields below here!! */
	FILE			*outputFile;
	uint32			dataBytesWritten;		/* NOT including header size */
	uint32			uncompressedSize;		/* size of uncompressed data */
	uint32			dataChunksWritten;
	uint32			blockSignature;
	int32			headerChunkLoc;
	DataChunkFirst	data1stHeader;
} ChunkParams, *ChunkParamsPtr;


Err		ChunkDataFile(ChunkParamsPtr params);
uint32	Write1stDataChunk(ChunkParamsPtr params, uint32 dataBytesToWrite, uint32 *dataPtr, uint32 fileCheckSum);
Err		WriteNthDataChunk(ChunkParamsPtr params, uint32 streamTime, uint32 dataBytesToWrite, uint32 *dataPtr);
uint32	FinalizeDataChunk(ChunkParamsPtr params);

#endif	/* __STREAMING_DATACHUNKIFY_H */
