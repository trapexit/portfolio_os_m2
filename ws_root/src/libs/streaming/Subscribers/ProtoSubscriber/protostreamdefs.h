#ifndef __PROTOSTREAMDEFS_H
#define __PROTOSTREAMDEFS_H

/******************************************************************************
 ** @(#) protostreamdefs.h 95/09/13 1.1
 ** Filed/stream data types and constants for the ProtoSubscriber.
 ******************************************************************************/

#include <stddef.h>					/* for offsetof() macro */
#include <kernel/types.h>
#include <streaming/dsstreamdefs.h>	/* SUBS_CHUNK_COMMON */


/**************************/
/* PRTO subscriber chunks */
/**************************/

#ifndef PRTO_CHUNK_TYPE
	#define	PRTO_CHUNK_TYPE		MAKE_ID('P','R','T','O')	
#endif

/* Header chunks contain one time initalization and
 * configuration stuff for a given channel. */
#ifndef PHDR_CHUNK_SUBTYPE
	#define	PHDR_CHUNK_SUBTYPE	MAKE_ID('P','H','D','R')	
#endif

/* The actual data that will be consumed. */
#ifndef PDAT_CHUNK_SUBTYPE
	#define	PDAT_CHUNK_SUBTYPE	MAKE_ID('P','D','A','T')	
#endif

/* Proto subscriber's HALT chunk subtype. Used to halt data flow in the
 * streamer for time-consuming subscriber initalization tasks. */
#ifndef PHLT_CHUNK_SUBTYPE
	#define	PHLT_CHUNK_SUBTYPE	MAKE_ID('P','H','L','T')	
#endif


typedef	struct ProtoHeaderChunk {
	SUBS_CHUNK_COMMON;
	int32		version;		/* format version number */
	} ProtoHeaderChunk, *ProtoHeaderChunkPtr;

typedef struct ProtoDataChunk {
	SUBS_CHUNK_COMMON;
	int32		actualDataSize;	/* Number of actual data bytes in chunk,
								   i.e. not including the chunk header */
	uint32 		data[1];		/* start of test data */
	} ProtoDataChunk, *ProtoDataChunkPtr;

/* The size of the header information that preceeds every proto data chunk.
 * Use this instead of sizeof(ProtoDataChunk). */
#define PROTO_DATA_CHUNK_HDR_SIZE	(offsetof(ProtoDataChunk, data))


typedef struct ProtoHaltSubsChunk {
	SUBS_CHUNK_COMMON;
	int32		haltDuration;	/* How long to wait before replying */
	} ProtoHaltSubsChunk, *ProtoHaltSubsChunkPtr;


#endif	/* __PROTOSTREAMDEFS_H */
