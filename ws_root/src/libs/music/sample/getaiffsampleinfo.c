/* @(#) getaiffsampleinfo.c 96/02/23 1.28 */

#include <audio/aiff_format.h>
#include <audio/audio.h>
#include <audio/music_iff.h>
#include <audio/musicerror.h>
#include <audio/parse_aiff.h>       /* self */
#include <kernel/mem.h>             /* AllocMem() */


#define DEBUG_AIFF  0

#if DEBUG_AIFF
    #include <stdio.h>
#endif


static Err CreateAIFFSampleInfo (SampleInfo **resultSampleInfo, const IFFParser *, uint32 flags);
static int32 FindMarker (AIFFMarkerChunk *markerChunk, uint16 markerID);

/**
|||	AUTODOC -public -class libmusic -group Sample -name GetAIFFSampleInfo
|||	Parses an AIFF sample file and return a SampleInfo data structure.
|||
|||	  Synopsis
|||
|||	    Err GetAIFFSampleInfo (SampleInfo **resultSampleInfo, const char *fileName, uint32 flags)
|||
|||	  Description
|||
|||	    Parse an AIFF sample file on disk; allocate and fill out a SampleInfo
|||	    structure with the relevant information.
|||
|||	    The currently supported sample file formats are:
|||
|||	    - AIFF
|||
|||	    - AIFC
|||
|||	    The SampleInfo contains a fairly complete description of the contents of the
|||	    sample file. See <audio/parse_aiff.h> for details.
|||
|||	    You may interrogate the contents of the SampleInfo for your own uses. You
|||	    may also pass it to SampleInfoToTags() to prepare a tag list to pass to
|||	    CreateSample().
|||
|||	    When done with the SampleInfo, free it with DeleteSampleInfo().
|||
|||	  Arguments
|||
|||	    resultSampleInfo
|||	        Pointer to a pointer to a SampleInfo structure that is allocated and
|||	        filled out.
|||
|||	    fileName
|||	        Name of an AIFF file to parse.
|||
|||	    flags
|||	        Set of flags to control parsing.
|||
|||	    The legal flags are:
|||
|||	    ML_GETSAMPLEINFO_F_SKIP_DATA
|||	        When set causes GetAIFFSampleInfo() to get format information only.
|||	        When not set, the sample data is loaded and a pointer to the loaded
|||	        data is stored in smpi_Data.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, libspmath.a, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    SampleInfoToTags(), DeleteSampleInfo(), DumpSampleInfo(), LoadSample(), Sample(@)
**/
Err GetAIFFSampleInfo (SampleInfo **resultSampleInfo, const char *fileName, uint32 flags)
{
    IFFParser *iff = NULL;
    Err errcode;

  #if DEBUG_AIFF
    printf ("GetAIFFSampleInfo: '%s' flags=0x%02x\n", fileName, flags);
  #endif

        /* init result */
    *resultSampleInfo = NULL;

        /* open IFF folio */
        /* note: separate failure path to avoid calling unopened IFF folio */
    if ((errcode = OpenIFFFolio()) < 0) return errcode;

        /* scan AIFF, returns IFFParser if successfully parsed FORM AIFF or AIFC */
    if ((errcode = ScanAIFF (&iff, fileName, !(flags & ML_GETSAMPLEINFO_F_SKIP_DATA))) < 0) goto clean;

        /* create SampleInfo from IFFParser */
    if ((errcode = CreateAIFFSampleInfo (resultSampleInfo, iff, flags)) < 0) goto clean;

  #if DEBUG_AIFF
    DumpSampleInfo (*resultSampleInfo, fileName);
  #endif

        /* success */
    errcode = 0;

clean:
    DeleteIFFParser (iff);
    CloseIFFFolio();
    return errcode;
}

/*
    Given an IFFParser at the end of an AIFF/AIFC context, create SampleInfo
    from properties collected in parser.

    Arguments
        resultSampleInfo
            Buffer to store allocated SampleInfo structure. Assumed to be
            cleared to NULL by caller.

        iff
            IFFParser positioned at end of FORM AIFF context.

        flags
            ML_GETSAMPLEINFO_F_SKIP_DATA
                Extract SSND data if not set.
*/
static Err CreateAIFFSampleInfo (SampleInfo **resultSampleInfo, const IFFParser *iff, uint32 flags)
{
    const bool loadData = !(flags & ML_GETSAMPLEINFO_F_SKIP_DATA);
    SampleInfo *smpi = NULL;
    PackedID formType;
    Err errcode;

        /* get FORM type (AIFF or AIFC) */
    {
        const ContextNode * const top = GetCurrentContext(iff);

        if (!top) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }
        formType = top->cn_Type;
    }
  #if DEBUG_AIFF
    printf ("FORM: '%.4s'\n", &formType);
  #endif

        /* allocate SampleInfo and set optional fields to suitable default values */
    if (!(smpi = AllocMem (sizeof(SampleInfo), MEMTYPE_NORMAL | MEMTYPE_FILL))) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    smpi->smpi_BaseNote     = AF_MIDDLE_C_PITCH;
    smpi->smpi_HighNote     = 127;
    smpi->smpi_HighVelocity = 127;
    smpi->smpi_SustainBegin = -1;
    smpi->smpi_ReleaseBegin = -1;
    smpi->smpi_SustainEnd   = -1;
    smpi->smpi_ReleaseEnd   = -1;

        /* COMM (required) */
    {
        AIFFPackedCommon commx;

        if (!GetPropChunk (iff, formType, ID_COMM, &commx, sizeof commx)) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }

        /* !!! ValidateAIFFCommon()? */

        smpi->smpi_NumFrames        = UNPACK_UINT32(commx.commx_NumSampleFrames);
        smpi->smpi_Channels         = commx.comm_NumChannels;
        smpi->smpi_Bits             = commx.comm_SampleSize;
        smpi->smpi_CompressionType  = formType == ID_AIFC ? UNPACK_UINT32 (commx.commx_CompressionType) : 0;
        smpi->smpi_CompressionRatio = GetAIFFCompressionRatio (smpi->smpi_CompressionType);
        smpi->smpi_SampleRate       = DecodeAIFFFloat80 (&commx.comm_SampleRate);
    }

        /* SSND (required) - always get location and size */
    {
        const SSNDInfo *ssndInfo;

        if (!(ssndInfo = FindSSNDInfo (iff, formType))) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }

        smpi->smpi_DataSize   = ssndInfo->ssnd_DataSize;
        smpi->smpi_DataOffset = ssndInfo->ssnd_DataOffset;
    }

        /* SSND (required) - extract data chunk if requested */
    if (loadData) {
        if (!(smpi->smpi_Data = ExtractDataChunk (iff, formType, ID_SSND))) {
            errcode = ML_ERR_BAD_FORMAT;
            goto clean;
        }
        smpi->smpi_Flags |= ML_SAMPLEINFO_F_SAMPLE_ALLOCATED;
    }

        /* INST and MARK (optional) */
    {
        AIFFInstrument inst;

        if (GetPropChunk (iff, formType, ID_INST, &inst, sizeof inst)) {
            const AIFFMarkerChunk *markerChunk = NULL;

                /* find MARK (required if there are loops, but otherwise unnecessary) */
            {
                const PropChunk *pc;

                if (pc = FindPropChunk (iff, formType, ID_MARK)) {
                    markerChunk = pc->pc_Data;
                    if ((errcode = ValidateAIFFMarkerChunk (markerChunk, pc->pc_DataSize)) < 0) goto clean;
                }
            }

                /* tuning and multi-sample info */
            smpi->smpi_BaseNote     = inst.inst_BaseNote;
            smpi->smpi_Detune       = inst.inst_Detune;
            smpi->smpi_LowNote      = inst.inst_LowNote;
            smpi->smpi_HighNote     = inst.inst_HighNote;
            smpi->smpi_LowVelocity  = inst.inst_LowVelocity;
            smpi->smpi_HighVelocity = inst.inst_HighVelocity;

                /* sustain loop */
            if (inst.inst_SustainLoop.alop_PlayMode) {
                if ((errcode = smpi->smpi_SustainBegin = FindMarker (markerChunk, inst.inst_SustainLoop.alop_BeginMarker)) < 0) goto clean;
                if ((errcode = smpi->smpi_SustainEnd   = FindMarker (markerChunk, inst.inst_SustainLoop.alop_EndMarker)) < 0) goto clean;
            }

                /* release loop */
            if (inst.inst_ReleaseLoop.alop_PlayMode) {
                if ((errcode = smpi->smpi_ReleaseBegin = FindMarker (markerChunk, inst.inst_ReleaseLoop.alop_BeginMarker)) < 0) goto clean;
                if ((errcode = smpi->smpi_ReleaseEnd   = FindMarker (markerChunk, inst.inst_ReleaseLoop.alop_EndMarker)) < 0) goto clean;
            }
        }
    }

        /* success: set result */
    *resultSampleInfo = smpi;
    return 0;

clean:
    DeleteSampleInfo (smpi);
    return errcode;
}

/*
    Find AIFFPackedMarker with matching ID. Return that marker's position.

    Arguments
        markerChunk
            Validated AIFFMarkerChunk or NULL.

        markerID
            Marker ID to find.

    Results
        Sample frame position of matched marker (non-negative value), or Err
        code if not found.
*/
static int32 FindMarker (AIFFMarkerChunk *markerChunk, uint16 markerID)
{
    if (markerChunk) {
        uint16 numMarkers = markerChunk->mark_NumMarkers;
        const AIFFPackedMarker *markx = markerChunk->mark_Markers;

        for (; numMarkers--; markx = NextAIFFPackedMarker (markx)) {
            if (markx->mark_ID == markerID) return (int32)UNPACK_UINT32(markx->markx_Position);
        }
    }

    return ML_ERR_BAD_FORMAT;
}
