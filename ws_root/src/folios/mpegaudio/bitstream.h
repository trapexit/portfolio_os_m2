/******************************************************************************
**
**  @(#) bitstream.h 96/11/25 1.3
**
******************************************************************************/
#ifndef BITSTREAM_H
#define BITSTREAM_H

#include "mpegaudiotypes.h"

#define	MINIMUM         4    /* Minimum size of the buffer in bytes */
#define	MAX_LENGTH      32   /* Maximum length of word written or
                                        read from bit stream */
#define	READ_MODE       0
#define	WRITE_MODE      1
#define	ALIGNING        8
#define	BINARY          0
#define	ASCII           1
#define BS_FORMAT       BINARY /* BINARY or ASCII = 2x bytes */
#define	BUFFER_SIZE     0x10000

#define	MIN(A, B)       ((A) < (B) ? (A) : (B))
#define	MAX(A, B)       ((A) > (B) ? (A) : (B))

#define SYNCWORD_PLUS_A_NIBBLE	0xFFF0
#define	NULL_CHAR		'\0'


uint32 getBits(uint32 N, BufferInfo *bi, Err *readStatus);
Err findSync(BufferInfo *bi);
Err flushRead(BufferInfo *bi);
void resetReadStatus(BufferInfo *bi);

#endif /* end of #ifndef BITSTREAM */
