#ifndef __AUDIO_AIFF_FORMAT_H
#define __AUDIO_AIFF_FORMAT_H


/******************************************************************************
**
**  @(#) aiff_format.h 96/07/31 1.13
**
**  AIFF file format and low-level support.
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960206 WJB  Tidied up slightly.
**  960207 WJB  Removed some extraneous stuff.
**  960208 WJB  Published a bunch of services from sound player.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifdef EXTERNAL_RELEASE
#error This file may not be used in externally released source code.
#endif

#ifndef __MISC_IFF_H
#include <misc/iff.h>
#endif

#ifndef __LIMITS_H
#include <limits.h>         /* UCHAR_MAX */
#endif

#ifndef __STDDEF_H
#include <stddef.h>         /* size_t, offsetof() */
#endif


/* -------------------- PString stuff (used by FORM AIFF) */

/*
    PString: character array w/ length stored in first character:
    {
        uint8   length;
        char    text[length];
    }

    For example: { 0x04, 'T', 'e', 's', 't' } is equal to the C string "Test"
*/

#define AIFF_PSTR_MAX_LENGTH UCHAR_MAX

    /* macros */
#define AIFFPStringAddr(s)    ( (char *) ((s) + 1) )
#define AIFFPStringLength(s)  ( (size_t) *(const uint8 *)(s) )
#define AIFFPStringSize(s)    ( (AIFFPStringLength(s)+1+1) & ~1 )       /* round up to even number of bytes */

    /* functions */
char *DecodeAIFFPString (char *dest, const char *src, size_t destsize);


/* -------------------- AIFF definitions */

    /* AIFF chunk names */
#define ID_AIFF MAKE_ID('A','I','F','F')
#define ID_AIFC MAKE_ID('A','I','F','C')
#define ID_COMM MAKE_ID('C','O','M','M')
#define ID_INST MAKE_ID('I','N','S','T')
#define ID_MARK MAKE_ID('M','A','R','K')
#define ID_SSND MAKE_ID('S','S','N','D')


/* -------------------- Structures */

/*
    AIFF structures (some are packed: uses arrays of words instead of longs
    where they aren't aligned).
*/

    /*
        Packed things used to deal with alignments which the compiler doesn't
        support. Note that this only affects the compiler. The processor can
        dereference any sized field on arbitrary alignments.
    */
typedef uint16 packed_uint32[2];
typedef uint16 packed_int32[2];
#define UNPACK_UINT32(data) (*(uint32 *)(data))
#define UNPACK_INT32(data)  (*(int32 *)(data))
#define PACK_UINT32(packedp,value) ( (*(uint32 *)(packedp)) = (value) )
#define PACK_INT32(packedp,value)  ( (*(int32 *)(packedp)) = (value) )

    /* IEEE 80-bit format packed into a structure suitable for arbitrary 16-bit alignment */
typedef struct AIFFFloat80 {
    uint16  f80_Exponent;
    uint16  f80_Mantissa[4];
} AIFFFloat80;

float32 DecodeAIFFFloat80 (const AIFFFloat80 *);


    /* packed COMM chunk (has non-long-aligned longs) */
typedef struct AIFFPackedCommon {
    uint16          comm_NumChannels;       /* # of audio channels */
    packed_uint32   commx_NumSampleFrames;  /* # of samples per channel */
    uint16          comm_SampleSize;        /* # of bits per sample */
    AIFFFloat80     comm_SampleRate;        /* sample frames per second */
    packed_uint32   commx_CompressionType;  /* Compression type ID */
    /* compression name follows */
} AIFFPackedCommon;

    /* size of AIFF COMM chunk */
#define AIFF_PACKED_COMMON_MIN_SIZE offsetof (AIFFPackedCommon, commx_CompressionType)


    /* INST loop structure */
typedef struct AIFFLoop {
    uint16  alop_PlayMode;          /* AIFFLOOPT_ */
    uint16  alop_BeginMarker;       /* marker IDs of beginning/end of loop, when PlayMode is a value other than AIFFLOOPT_NONE */
    uint16  alop_EndMarker;
} AIFFLoop;

    /* AIFF loop play modes for alop_PlayMode values */
enum {
    AIFFLOOPT_NONE,
    AIFFLOOPT_FORWARD,
    AIFFLOOPT_BACKWARD_FORWARD
};

    /* INST chunk */
typedef struct AIFFInstrument {
    uint8       inst_BaseNote;
    int8        inst_Detune;
    uint8       inst_LowNote;
    uint8       inst_HighNote;
    uint8       inst_LowVelocity;
    uint8       inst_HighVelocity;
    int16       inst_Gain;
    AIFFLoop    inst_SustainLoop;
    AIFFLoop    inst_ReleaseLoop;
} AIFFInstrument;


    /* packed marker structure */
typedef struct AIFFPackedMarker {
    uint16          mark_ID;                /* unique 16-bit ID of marker */
    packed_uint32   markx_Position;         /* frame number of marker */
    char            mark_Name[1];           /* Name of marker (AIFFPString) */
    /* rest of AIFFPString follows */
} AIFFPackedMarker;

#define AIFF_PACKED_MARKER_MIN_SIZE offsetof (AIFFPackedMarker, mark_Name[1])
#define AIFFPackedMarkerSize(markx) (offsetof (AIFFPackedMarker, mark_Name) + AIFFPStringSize ((markx)->mark_Name))
#define NextAIFFPackedMarker(markx) ( (AIFFPackedMarker *)((char *)(markx) + AIFFPackedMarkerSize(markx)) )

    /* MARK chunk */
typedef struct AIFFMarkerChunk {
    uint16          mark_NumMarkers;        /* number of markers */
    AIFFPackedMarker mark_Markers[1];       /* packed array of AIFFPackedMarkers */
} AIFFMarkerChunk;

#define AIFF_MARKER_CHUNK_MIN_SIZE  offsetof (AIFFMarkerChunk, mark_Markers)

Err ValidateAIFFMarkerChunk (const AIFFMarkerChunk *markerChunk, uint32 markerChunkSize);


    /* header of SSND chunk, followed by sound data */
typedef struct AIFFSoundDataHeader {
    uint32  ssnd_Offset;
    uint32  ssnd_BlockSize;
} AIFFSoundDataHeader;

    /* SSNDInfo - used to locate sound data within file */
typedef struct SSNDInfo {
    uint32 ssnd_DataOffset;
    uint32 ssnd_DataSize;
} SSNDInfo;

Err HandleSSND (IFFParser *, bool storeData);
const SSNDInfo *FindSSNDInfo (const IFFParser *, PackedID formType);


/* -------------------- Misc Functions */

Err ScanAIFF (IFFParser **resultIFF, const char *fileName, bool loadData);
uint32 GetAIFFCompressionRatio (PackedID compressionType);


/*****************************************************************************/


#endif /* __AUDIO_AIFF_FORMAT_H */
