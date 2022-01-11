#ifndef __AUDIO_PARSE_AIFF_H
#define __AUDIO_PARSE_AIFF_H

/****************************************************************************
**
**  @(#) parse_aiff.h 96/08/01 1.12
**
**  Includes for parsing AIFF audio sample files.
**
****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>        /* DeleteItem() */
#endif


/* -------------------- SampleInfo */

/* This structure is returned by GetAIFFSampleInfo() */
typedef struct SampleInfo {

        /* Data */
    void   *smpi_Data;              /* Points to first sample if data was loaded
                                    ** (ML_GETSAMPLEINFO_F_SKIP_DATA not set).
                                    ** This memory is allocated with MEMTYPE_TRACKSIZE. */
    int32   smpi_NumFrames;         /* Size of sample data in frames */
    int32   smpi_DataSize;          /* Size of sample data in bytes */
    int32   smpi_DataOffset;        /* Byte position in AIFF file of first sample frame */
    uint8   smpi_Flags;             /* ML_SAMPLEINFO_F_ flags (defined below) */
    uint8   smpi_Reserved1[3];

        /* Format */
    uint8   smpi_Channels;          /* channels per frame, 1 = mono, 2=stereo */
    uint8   smpi_Bits;              /* ORIGINAL bits per sample BEFORE any compression. */
    uint8   smpi_CompressionRatio;  /* 2 = 2:1, 4 = 4:1 */
    uint8   smpi_Reserved2[1];
    PackedID smpi_CompressionType;  /* eg. ID_SQS2. 0 or 'NONE' when uncompressed. */

        /* Loops (Frame number of loop point or -1) */
    int32   smpi_SustainBegin;      /* Set to -1 if no sustain loop */
    int32   smpi_SustainEnd;
    int32   smpi_ReleaseBegin;      /* Set to -1 if no release loop */
    int32   smpi_ReleaseEnd;

        /* Tuning */
    float32 smpi_SampleRate;        /* Sample Rate recorded at */
    uint8   smpi_BaseNote;          /* MIDI Note when played at original sample rate. */
    int8    smpi_Detune;            /* Amount (in cents) to detune smpi_BaseNote to reach
                                    ** pitch when played at original sample rate. */
    uint8   smpi_Reserved3[2];

        /* Multisample */
    uint8   smpi_LowNote;           /* Lowest MIDI note number for this sample */
    uint8   smpi_HighNote;          /* Highest MIDI note number for this sample */
    uint8   smpi_LowVelocity;       /* Lowest MIDI velocity for this sample */
    uint8   smpi_HighVelocity;      /* Highest MIDI velocity for this sample */

} SampleInfo;

    /* smpi_Flags */
#define ML_SAMPLEINFO_F_SAMPLE_ALLOCATED 0x01   /* When set, data pointed to by smpi_Data is
                                                ** automatically freed by DeleteSampleInfo() */


/* -------------------- Misc Defines */

    /* GetAIFFSampleInfo() flags */
#define ML_GETSAMPLEINFO_F_SKIP_DATA    0x1

    /* SampleInfoToTags() defines */
#define ML_SAMPLEINFO_MAX_TAGS          20


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* top-level AIFF loader */
Item LoadSample (const char *fileName);
Item LoadSystemSample (const char *fileName);
#define UnloadSample(sample) DeleteItem(sample)

    /* lower-level AIFF loader */
Err  GetAIFFSampleInfo (SampleInfo **resultSampleInfo, const char *fileName, uint32 flags);
Err  SampleInfoToTags (const SampleInfo *, TagArg *tagBuffer, int32 maxTags);
void DeleteSampleInfo (SampleInfo *);
void DumpSampleInfo (const SampleInfo *, const char *banner);

    /* sample player identification */
Err SampleFormatToInsName (uint32 compressionType, bool ifVariable, uint32 numChannels, char *nameBuffer, int32 nameBufferSize);
Err SampleItemToInsName (Item sample, bool ifVariable, char *nameBuffer, int32 nameBufferSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_PARSE_AIFF_H */
