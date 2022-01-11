/* @(#) loadsample.c 96/02/23 1.6 */

#include <audio/audio.h>
#include <audio/parse_aiff.h>       /* self */

/**
|||	AUTODOC -public -class libmusic -group Sample -name LoadSample
|||	Loads a Sample(@) from an AIFF or AIFC file.
|||
|||	  Synopsis
|||
|||	    Item LoadSample (const char *fileName)
|||
|||	  Description
|||
|||	    This procedure allocates task memory and creates a sample item there that
|||	    contains the digital-audio recording from the specified file. The file
|||	    must be either an AIFF file or an AIFC file. These can be created using
|||	    almost any sound development tool including AudioMedia, Alchemy, CSound,
|||	    and SoundHack.
|||
|||	    AIFC files contain compressed audio data. These can be created from an
|||	    AIFF file using the SquashSound MPW tool from The 3DO Company.
|||
|||	    Most AIFF attributes are converted to Sample(@) attributes. These include
|||	    sustain and release loops, multisample selection, tuning, compression, and
|||	    data format. Markers other than the sustain and release loops are not stored
|||	    in the Sample.
|||
|||	    The Sample Item is created with { AF_TAG_AUTO_FREE_DATA, TRUE } to facilitate
|||	    simple and complete deletion. The Sample Item is given the same name as the
|||	    AIFF file.
|||
|||	    When you finish with the sample, you should call UnloadSample() to
|||	    deallocate the resources.
|||
|||	  Arguments
|||
|||	    fileName
|||	        Name of AIFF or AIFC file to load sample from.
|||
|||	  Return Value
|||
|||	    The procedure returns an item number of the Sample if successful (a
|||	    non-negative value) or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Notes -preformatted
|||
|||	    This function is equivalent to:
|||
|||	    Item LoadSample (const char *fileName)
|||	    {
|||	        Item result;
|||	        SampleInfo *smpi = NULL;
|||	        TagArg tags[ML_SAMPLEINFO_MAX_TAGS];
|||
|||	            // Get SampleInfo from AIFF file including the sample data
|||	        if ((result = GetAIFFSampleInfo (&smpi, fileName, 0)) < 0) goto clean;
|||
|||	            // Get Tags from SampleInfo
|||	        if ((result = SampleInfoToTags (smpi, tags, ML_SAMPLEINFO_MAX_TAGS)) < 0)
|||	            goto clean;
|||
|||	            // Create Sample, transfer ownership of data memory to Sample using
|||	            // AF_TAG_AUTO_FREE_DATA
|||	        if ((result = CreateSampleVA (
|||	            TAG_ITEM_NAME,          fileName,
|||	            AF_TAG_AUTO_FREE_DATA,  TRUE,
|||	            TAG_JUMP,               tags)) < 0) goto clean;
|||
|||	            // Clear allocation flag after successful CreateSample() call
|||	            // because the Sample is now responsible for freeing it.
|||	        smpi->smpi_Flags &= ~ML_SAMPLEINFO_F_SAMPLE_ALLOCATED;
|||
|||	    clean:
|||	        DeleteSampleInfo (smpi);
|||	        return result;
|||	    }
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, libspmath.a, System.m2/Modules/audio,
|||	    System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    UnloadSample(), Sample(@), CreateAttachment(), GetAIFFSampleInfo(),
|||	    SampleInfoToTags(), LoadSystemSample()
**/
Item LoadSample (const char *fileName)
{
    Item result;
    SampleInfo *smpi = NULL;
    TagArg tags[ML_SAMPLEINFO_MAX_TAGS];

        /* Get SampleInfo from AIFF file including the sample data */
    if ((result = GetAIFFSampleInfo (&smpi, fileName, 0)) < 0) goto clean;

        /* Get Tags from SampleInfo */
    if ((result = SampleInfoToTags (smpi, tags, ML_SAMPLEINFO_MAX_TAGS)) < 0) goto clean;

        /* Create Sample, transfer ownership of data memory to Sample using
        ** AF_TAG_AUTO_FREE_DATA
        */
    if ((result = CreateSampleVA (
        TAG_ITEM_NAME,          fileName,
        AF_TAG_AUTO_FREE_DATA,  TRUE,
        TAG_JUMP,               tags)) < 0) goto clean;

        /* Clear allocation flag after successful CreateSample() call
        ** because the Sample is now responsible for freeing it.
        */
    smpi->smpi_Flags &= ~ML_SAMPLEINFO_F_SAMPLE_ALLOCATED;

clean:
    DeleteSampleInfo (smpi);
    return result;
}

/**
|||	AUTODOC -public -class libmusic -group Sample -name UnloadSample
|||	Unloads a sample loaded by LoadSample().
|||
|||	  Synopsis
|||
|||	    Err UnloadSample (Item sample)
|||
|||	  Description
|||
|||	    This procedure deletes a sample Item and sample memory for a sample loaded
|||	    by LoadSample().
|||
|||	  Arguments
|||
|||	    sample
|||	        Item number of sample to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative value)
|||	    if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/parse_aiff.h> V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    LoadSample()
**/
