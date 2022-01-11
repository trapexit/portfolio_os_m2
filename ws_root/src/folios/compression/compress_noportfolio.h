/*  @(#) compress_noportfolio.h 96/04/23 1.2 */
/*
	File:		compress_noportfolio.h

	Written by:	John R. McMullen

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by The 3DO Company.

	Change History (most recent first):

		 <2>	96/04/15	JRM		Got it working.

	To Do:
*/

#ifndef __compress_noportfolio__
#define __compress_noportfolio__

#ifdef macintosh
#include <types.h>
typedef Boolean bool;
#else
typedef bool Boolean;
#endif /* macintosh */

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef char int8;
typedef short int16;
typedef long int32;
typedef long Err;

typedef struct Compressor Compressor;
typedef struct Decompressor Decompressor;

#define TagArg void
#define COMP_ERR_BADPTR       -50
#define COMP_ERR_BADTAG       -51
#define COMP_ERR_NOMEM        -52
#define COMP_ERR_DATAREMAINS  -53
#define COMP_ERR_DATAMISSING  -54
#define COMP_ERR_OVERFLOW     -55

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef void (* CompFunc)(void *userData, uint32 word);
Err CreateCompressor(Compressor **compr, CompFunc cf, const TagArg *tags);
Err DeleteCompressor(Compressor *compr);
Err FeedCompressor(Compressor *compr, const void *data, uint32 numDataWords);
int32 GetCompressorWorkBufferSize(const TagArg *tags);
#ifdef __cplusplus
}
#endif

#endif /* __compress_noportfolio__ */
