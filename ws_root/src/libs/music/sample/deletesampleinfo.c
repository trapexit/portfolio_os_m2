/* @(#) deletesampleinfo.c 96/02/13 1.2 */

#include <audio/parse_aiff.h>       /* self */
#include <kernel/mem.h>             /* FreeMem() */

/**
|||	AUTODOC -public -class libmusic -group Sample -name DeleteSampleInfo
|||	Frees SampleInfo.
|||
|||	  Synopsis
|||
|||	    void DeleteSampleInfo (SampleInfo *smpi)
|||
|||	  Description
|||
|||	    Frees SampleInfo created by GetAIFFSampleInfo(). Frees data pointed to by
|||	    smpi_Data if smpi_Flags contains the flag ML_SAMPLEINFO_F_SAMPLE_ALLOCATED.
|||
|||	  Arguments
|||
|||	    smpi
|||	        SampleInfo to free. Can be NULL.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a
|||
|||	  See Also
|||
|||	    GetAIFFSampleInfo()
**/
void DeleteSampleInfo (SampleInfo *smpi)
{
    if (smpi) {
        if (smpi->smpi_Flags & ML_SAMPLEINFO_F_SAMPLE_ALLOCATED) {
            FreeMem (smpi->smpi_Data, TRACKED_SIZE);
        }
        FreeMem (smpi, sizeof *smpi);
    }
}
