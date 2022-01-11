/******************************************************************************
**
**  @(#) mpabufferqueue.h 96/03/29 1.1
**
******************************************************************************/

#ifndef _MPABUFFERQUEUES_H
#define _MPABUFFERQUEUES_H

#ifndef __MISC_MPAUDIODECODE_H
#include <misc/mpaudiodecode.h>
#endif

#include "mpadecoderinterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The following two functions are the callback functions needed by
 * the MPEG audio decoder to request for new compressed data buffers and
 * to notify the subscriber when done processing the buffers. */
Err GetCompressedBfr( void *ctx, const uint8 **buf, int32 *len,
	uint32 *pts, uint32 *ptsIsValidFlag );
Err CompressedBfrRead( void *ctx, uint8 *buf );

/* Recycle the decompressed buffer back to the subscriber. */
Err DecompressedBfrComplete( DecoderMsgPtr msgPtr, Err status );

/* Process incoming messages from the decoder's message port. */
Err ProcessNewMsgs( DecoderThreadCtxPtr ctx );

#ifdef __cplusplus
}
#endif

#endif /* _MPABUFFERQUEUES_H */
