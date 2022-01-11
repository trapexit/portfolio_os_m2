/******************************************************************************
**
**  @(#) testmpegaudio.h 96/03/11 1.2
**
******************************************************************************/

#ifndef TESTMPEGAUDIO_HEADER
#define TESTMPEGAUDIO_HEADER

void  mpCleanup( void );
void  mpBreakPoint( void );
/* #define audio_printf printf */

/* cleanup stages */
#define CLNUP_BSFILE    0x1
#define CLNUP_BSBUF     0x2
#define CLNUP_EVENT     0x4
#define CLNUP_AUDFOLIO  0x8
#define CLNUP_OUTINS    0x10
#define CLNUP_SAMPINS   0x20
#define CLNUP_SPOOL     0x40
#define CLNUP_AUDBUF    0x80
#define CLNUP_DEVICE    0x100
#define CLNUP_STOPSPOOL 0x400
#define CLNUP_AOUTFILE  0x800
#define CLNUP_ACMPFILE  0x1000
#define CLNUP_CMPBUF    0x2000

#endif /* TESTMPEGAUDIO_HEADER */
