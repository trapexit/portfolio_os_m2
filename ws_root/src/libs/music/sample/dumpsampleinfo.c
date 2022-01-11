/* @(#) dumpsampleinfo.c 96/02/13 1.2 */

#include <audio/parse_aiff.h>       /* self */
#include <stdio.h>

/**
|||	AUTODOC -public -class libmusic -group Sample -name DumpSampleInfo
|||	Dumps SampleInfo structure returned by GetAIFFSampleInfo().
|||
|||	  Synopsis
|||
|||	    void DumpSampleInfo (const SampleInfo *smpi, const char *banner)
|||
|||	  Description
|||
|||	    Dumps SampleInfo structure returned by GetAIFFSampleInfo() to debugging
|||	    terminal.
|||
|||	  Arguments
|||
|||	    smpi
|||	        SampleInfo to dump.
|||
|||	    banner
|||	        Optional banner to print. Can be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a
|||
|||	  See Also
|||
|||	    GetAIFFSampleInfo()
**/
void DumpSampleInfo (const SampleInfo *smpi, const char *banner)
{
    if (banner) printf ("%s ", banner);
    printf ("SampleInfo:\n");

    #define PRTMEMBER(fmt,member) printf ("  %-21s " fmt "\n", #member, smpi->member)
    #define PRTID(member)         printf ("  %-21s '%.4s'\n", #member, &smpi->member)

        /* Data */
    PRTMEMBER ("0x%x",      smpi_Data);
    PRTMEMBER ("%d frames", smpi_NumFrames);
    PRTMEMBER ("%d bytes",  smpi_DataSize);
    PRTMEMBER ("0x%x",      smpi_DataOffset);
    PRTMEMBER ("0x%02x",    smpi_Flags);

        /* Format */
    PRTMEMBER ("%u", smpi_Channels);
    PRTMEMBER ("%u", smpi_Bits);
    PRTMEMBER ("%u", smpi_CompressionRatio);
    PRTID (smpi_CompressionType);

        /* Loops */
    PRTMEMBER ("%d", smpi_SustainBegin);
    PRTMEMBER ("%d", smpi_SustainEnd);
    PRTMEMBER ("%d", smpi_ReleaseBegin);
    PRTMEMBER ("%d", smpi_ReleaseEnd);

        /* Tuning */
    PRTMEMBER ("%g Hz", smpi_SampleRate);
    PRTMEMBER ("%u",    smpi_BaseNote);
    PRTMEMBER ("%d",    smpi_Detune);

        /* Multisample */
    PRTMEMBER ("%u", smpi_LowNote);
    PRTMEMBER ("%u", smpi_HighNote);
    PRTMEMBER ("%u", smpi_LowVelocity);
    PRTMEMBER ("%u", smpi_HighVelocity);
}
