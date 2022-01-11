/* @(#) createsample.c 96/07/29 1.3 */

#include <audio/audio.h>
#include <kernel/item.h>

/**
|||	AUTODOC -public -class audio -group Sample -name CreateSample
|||	Creates a Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Item CreateSample (const TagArg *tagList)
|||
|||	    Item CreateSampleVA (uint32 tag1, ...)
|||
|||	  Description
|||
|||	    Creates a Sample Item. The Sample can later be attached to an Instrument(@)
|||	    or Template(@) with an FIFO.
|||
|||	    When you are finished with the Sample, call DeleteSample().
|||
|||	    CreateSample() takes care of writing any of the sample data which may be in
|||	    the data cache back to memory. So generally you don't need to be concerned
|||	    with the data cache. If, however, you modify a Sample Item's sample data
|||	    (including reading it from a file) after binding it to the Sample Item,
|||	    then you must call WriteBackDCache() for the modified portion of the sample
|||	    data, or set AF_TAG_ADDRESS, AF_TAG_FRAMES, or AF_TAG_NUMBYTES again with
|||	    SetAudioItemInfo(). Failure to do so can result in intermittent audio noise.
|||	    See the Sample(@) Item page for more details on this.
|||
|||	  Tag Arguments
|||
|||	    See the Sample(@) Item page.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the sample (a non-negative
|||	    value) or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in libc.a V29.
|||
|||	  Caveats
|||
|||	    There are some important length and loop point alignment issues with
|||	    samples. See the Caveats section on the Sample(@) Item page for details.
|||
|||	  Examples
|||
|||	        // Create a Sample Item to be filled out later with SetAudioItemInfo()
|||	    sample = CreateSample (NULL);
|||
|||	        // Make a Sample Item for raw sample data in memory
|||	        // (8-bit monophonic 22KHz sample in this case)
|||	    sample = CreateSampleVA (
|||	        AF_TAG_ADDRESS,        data,
|||	        AF_TAG_FRAMES,         NUMFRAMES,
|||	        AF_TAG_CHANNELS,       1,
|||	        AF_TAG_WIDTH,          1,
|||	        AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData (22050),
|||	        TAG_END);
|||
|||	  Notes
|||
|||	    This function is equivalent to:
|||
|||	  -preformatted
|||
|||	        CreateItem (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), tagList)
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    Sample(@), DeleteSample(), LoadSample(), CreateAttachment()
**/

Item CreateSample (const TagArg *tagList)
{
    return CreateItem (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE), tagList);
}


/**
|||	AUTODOC -public -class audio -group Sample -name DeleteSample
|||	Deletes a Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Err DeleteSample (Item sample)
|||
|||	  Description
|||
|||	    This procedure deletes the specified sapmle, freeing its resources. It also
|||	    deletes any Attachment(@)s to the sample.
|||
|||	    If the Sample was created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then the
|||	    sample data is also be freed when the Sample Item is deleted. Otherwise, the
|||	    sample data is not freed automatically.
|||
|||	  Arguments
|||
|||	    sample
|||	        Item number of the Sample to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro in <audio/audio.h>
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    Sample(@), CreateSample(), DeleteAttachment()
**/
