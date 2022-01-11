/* @(#) sampleinfototags.c 96/08/07 1.7 */

#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/parse_aiff.h>       /* self */

#ifdef BUILD_STRINGS
	#include <stdio.h>
	#define ERR(x)  printf x
#else
	#define ERR(x)
#endif

/**
|||	AUTODOC -public -class libmusic -group Sample -name SampleInfoToTags
|||	Fills out a tag list to create a Sample(@) Item from SampleInfo.
|||
|||	  Synopsis
|||
|||	    Err SampleInfoToTags (const SampleInfo *smpi, TagArg *tagBuffer,
|||	                          int32 maxTags)
|||
|||	  Description
|||
|||	    Fills out a tag list which can be passed to CreateSample() to create a
|||	    Sample(@) item from the SampleInfo.
|||
|||	    The sample data pointed to by smpi_Data is considered the property of the
|||	    SampleInfo as long as the ML_SAMPLEINFO_F_SAMPLE_ALLOCATED flag is set in
|||	    smpi_Flags. For this reason, this function does not place an
|||	    AF_TAG_AUTO_FREE_DATA tag in the resulting tag list. You may pass this tag
|||	    along with the tags returned by this function to CreateSample(), but be
|||	    sure to clear the ML_SAMPLEINFO_F_SAMPLE_ALLOCATED to avoid doubly freeing
|||	    the sample data. See below for how to transfer sample data memory ownership
|||	    to the sample item.
|||
|||	  Arguments
|||
|||	    smpi
|||	        SampleInfo to process.
|||
|||	    tagBuffer
|||	        TagArg array in which to store resulting tag list.
|||
|||	    maxTags
|||	        Number of TagArgs in tagBuffer (including space for trailing TAG_END).
|||
|||	  Return Value
|||
|||	    On success, the procedure returns a non-negative value. On failure it
|||	    returns an error code (a negative value).
|||
|||	    On success, tagBuffer contains a legal tag list to pass to CreateSample().
|||	    On failure, tagBuffer contents is undefined, and not guaranteed to be a
|||	    legal list.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Notes
|||
|||	    Tag buffer overflow is trapped as an error. To avoid this condition, make
|||	    sure your TagArg buffer has at least ML_SAMPLEINFO_MAX_TAGS elements.
|||
|||	  Examples
|||
|||	    // Code fragment which demonstrates how to create a sample from the tags
|||	    // returned by SampleInfoToTags() and transfer the sample data memory
|||	    // ownership to the sample item. Be sure to avoid creating more than one
|||	    // sample from a given SampleInfo in this manner, or else you will get a
|||	    // double free when the sample items are deleted.
|||	    {
|||	        TagArg sampleTags[ML_SAMPLEINFO_MAX_TAGS];
|||	        Err errcode;
|||	        Item sampleItem;
|||
|||	        .
|||	        .
|||
|||	            // Get Tags from SampleInfo
|||	        if ((errcode = SampleInfoToTags (sampleInfo, sampleTags,
|||	            ML_SAMPLEINFO_MAX_TAGS)) < 0) goto clean;
|||
|||	            // Create Sample, transfer ownership of data memory to Sample
|||	        if ((errcode = sampleItem = CreateSampleVA (
|||	            AF_TAG_AUTO_FREE_DATA, TRUE,
|||	            TAG_JUMP, tags)) < 0) goto clean;
|||
|||	            // Data now belongs to Sample, so clear
|||	            // ML_SAMPLEINFO_F_SAMPLE_ALLOCATED to prevent DeleteSampleInfo()
|||	            // from deleting it.
|||	        sampleInfo->smpi_Flags &= ~ML_SAMPLEINFO_F_SAMPLE_ALLOCATED;
|||
|||	        .
|||	        .
|||	    }
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    GetAIFFSampleInfo(), LoadSample(), Sample(@)
**/
Err SampleInfoToTags (const SampleInfo *smpi, TagArg *tagBuffer, int32 maxTags)
{
	TagArg *tagp = tagBuffer;
	const TagArg * const tagEnd = tagp + maxTags;

#define ADDTAG(tag,val) \
	{ \
		if (tagp >= tagEnd) goto excess_tags; \
		tagp->ta_Tag = (tag); \
		tagp->ta_Arg = (TagData)(val); \
		tagp++; \
	}

/* !!! perhaps shouldn't make assumptions about audio folio's tag defaults -
**     could create a sample Item to sniff for defaults, or could always
**     specify everything */
#define MAYBEADDTAG(tag,val,default) \
	if ((val) != (default)) ADDTAG(tag,val);

/* Data */
/* @@@ using both DataSize and NumFrames to cause the audio folio to do a detect
**     any inconsistency between the SSND data size and the information specified
**     in the COMM chunk. */
	ADDTAG (AF_TAG_ADDRESS,                 smpi->smpi_Data);
	ADDTAG (AF_TAG_FRAMES,                  smpi->smpi_NumFrames);
	ADDTAG (AF_TAG_NUMBYTES,                smpi->smpi_DataSize);

/* Format */
	MAYBEADDTAG (AF_TAG_CHANNELS,           smpi->smpi_Channels,            1);
	MAYBEADDTAG (AF_TAG_NUMBITS,            smpi->smpi_Bits,                16);
	MAYBEADDTAG (AF_TAG_COMPRESSIONRATIO,   smpi->smpi_CompressionRatio,    1);
	MAYBEADDTAG (AF_TAG_COMPRESSIONTYPE,    smpi->smpi_CompressionType,     0);

/* Loops */
	MAYBEADDTAG (AF_TAG_SUSTAINBEGIN,       smpi->smpi_SustainBegin,        -1);
	MAYBEADDTAG (AF_TAG_SUSTAINEND,         smpi->smpi_SustainEnd,          -1);
	MAYBEADDTAG (AF_TAG_RELEASEBEGIN,       smpi->smpi_ReleaseBegin,        -1);
	MAYBEADDTAG (AF_TAG_RELEASEEND,         smpi->smpi_ReleaseEnd,          -1);

/* Tuning */
		/* Always set sample rate to avoid FP compare. */
	ADDTAG (AF_TAG_SAMPLE_RATE_FP,          ConvertFP_TagData(smpi->smpi_SampleRate));
	MAYBEADDTAG (AF_TAG_BASENOTE,           smpi->smpi_BaseNote,            AF_MIDDLE_C_PITCH);
	MAYBEADDTAG (AF_TAG_DETUNE,             smpi->smpi_Detune,              0);

/* Multisample */
	MAYBEADDTAG (AF_TAG_LOWNOTE,            smpi->smpi_LowNote,             0);
	MAYBEADDTAG (AF_TAG_HIGHNOTE,           smpi->smpi_HighNote,            127);
	MAYBEADDTAG (AF_TAG_LOWVELOCITY,        smpi->smpi_LowVelocity,         0);
	MAYBEADDTAG (AF_TAG_HIGHVELOCITY,       smpi->smpi_HighVelocity,        127);

/* Terminate list */
	ADDTAG (TAG_END, 0);
	TOUCH(tagp);    /* silence compiler warning */

/* Success */
	return 0;

excess_tags:
	ERR(("SampleInfoToTags: exceeded maxTags!\n"));
	return ML_ERR_BUFFER_TOO_SMALL;
}
