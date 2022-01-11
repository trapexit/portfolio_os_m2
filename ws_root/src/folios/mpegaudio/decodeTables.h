/* @(#) decodeTables.h 96/03/01 1.2 */

#ifndef DECODE_TABLES
#define DECODE_TABLES

#ifndef _MPEGAUDIOTYPES_H
#include "mpegaudiotypes.h"
#endif

extern const uint16	nlevels[4][32][16] ;
extern const uint8	codebits[4][32][16] ;
extern const uint8	quant[4][32][16] ;
extern const uint8	msbit[4][32][16] ;
extern const uint8	sblimit_table[4] ;
extern const uint8	bound_table[3][4] ;
extern const int16	bitrate[3][16] ;
/* extern const int8	s_freq[4] ; */
extern const uint16	degrouper[10] ;
extern const AUDIO_FLOAT c_table[17] ;
extern const AUDIO_FLOAT d_table[17] ;
extern const AUDIO_FLOAT scalefactor[64] ;
extern const AUDIO_FLOAT scaler[17] ;
#endif /* end of #ifndef DECODE_TABLES */
