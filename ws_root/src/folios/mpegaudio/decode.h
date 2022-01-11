/* @(#) decode.h 96/11/25 1.4 */

#ifndef DECODE_H
#define DECODE_H

#include "mpegaudiotypes.h"

/* protection codes */
#define PROTECT_OFF             1
#define PROTECT_ON              0


/* max of 30 subbands needed for layer 2, supported by matrixing */
#define	SBLIMIT		32	
#define SCALE		32768

#define	SYNC_WORD_LNGTH		12

typedef struct tableToUse {
    const uint16	*nlevels_table;
    const uint8		*codebits_table;
    const uint8		*quant_table;
    const uint8		*msbit_table;
} tableToUse, *tableToUsePtr;
 
Err parseHeader(BufferInfo *bi, FrameInfo *fi, tableToUsePtr pickedTbls );
Err parseSubbands(BufferInfo *bi, FrameInfo *fi,
	tableToUsePtr pickedTbls, uint32 *sample_code );
void Requantize(FrameInfo *fi, tableToUsePtr pickedTbls, uint32 *sample_code,
	AUDIO_FLOAT *input_sample, uint32 *alloc_pointer, uint32 *index_pointer);

#endif /* end of #ifndef DECODE_H */
