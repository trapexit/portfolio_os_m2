#ifndef __JOINSTREAMDEFS_H
#define __JOINSTREAMDEFS_H

/******************************************************************************
 ** @(#) joinstreamdefs.h 95/09/20 1.1
 ** Join Subscriber filed data types and constants for Data Stream streams.
 ******************************************************************************/

#include <streaming/dsstreamdefs.h>


/**************************/
/* JOIN subscriber chunks */
/**************************/

#ifndef JOIN_CHUNK_TYPE
	#define	JOIN_CHUNK_TYPE		MAKE_ID('J','O','I','N')
#endif

#ifndef JOIN_HEADER_SUBTYPE
	#define	JOIN_HEADER_SUBTYPE	MAKE_ID('J','H','D','R')
#endif

/* subtype for continuation blocks */
#ifndef JOIN_DATA_SUBTYPE
	#define	JOIN_DATA_SUBTYPE	MAKE_ID('J','D','A','T')
#endif


typedef struct JoinChunkFirst {
	SUBS_CHUNK_COMMON;
	int32	joinChunkType;	/* 'JHDR' for JoinChunkFirst */
	int32	totalDataSize;	/* the total size of the data in all chunks */
	int32	ramType;		/* AllocMem flags for this type of data */
	int32	compType;		/* type of compression used on this data */
	int32	dataSize;		/* the size of the data in this chunk */
	/* uint32	data[1]; */	/* the data goes here... */
	} JoinChunkFirst, *JoinChunkFirstPtr;

typedef struct JoinChunkData {
	SUBS_CHUNK_COMMON;
	int32	joinChunkType;	/* 'JDAT' for JoinChunkData */
	int32	dataSize;		/* the size of the data in this chunk */
	/* uint32	data[1]; */	/* the data goes here... */
	} JoinChunkData, *JoinChunkDataPtr;


#endif /* __JOINSTREAMDEFS_H */
