/******************************************************************************
**
**  @(#) autodocs.c 96/06/19 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class libmusic -group ATAG -name --ATAG-File-Format--
|||	Simple audio object file format for Samples, Envelopes, Delay Lines, Delay Line
|||	Templates, and Tunings.
|||
|||	  File Format -preformatted
|||
|||	    {
|||	            // header
|||	        PackedID 'ATAG'
|||	        uint32   size of AudioTagHeader
|||	        AudioTagHeader
|||
|||	            // data
|||	        PackedID 'BODY'
|||	        uint32   size of body data in bytes. Use 0 size when no body data
|||	                 is present
|||	        uint8 data [size]
|||	    }
|||
|||	  Description
|||
|||	    This file format provides a low-overhead method of loading data-oriented
|||	    audio Items from disc. It supports the following types of audio folio Items:
|||
|||	    - Sample
|||
|||	    - Delay Line
|||
|||	    - Delay Line Template
|||
|||	    - Envelope
|||
|||	    - Tuning
|||
|||	    The format is IFF-like in that it has chunks, but is not actually an IFF
|||	    FORM. This is because this format is meant to be as simple to parse as
|||	    possible.
|||
|||	    Because of its IFF-like chunk nature, such a file can be embedded directly
|||	    into an IFF FORM and parsed using the IFF folio. LoadATAG() can't be used
|||	    to deal with extracting this from an IFF FORM. Instead call
|||	    ValidateAudioTagHeader() and CreateItem() after extracting the chunks.
|||
|||	    Support for parsing ATAG files is provided in libmusic.a.
|||
|||	  Associated Files
|||
|||	    <audio/atag.h>, libmusic.a
|||
|||	  Examples
|||
|||	    Here is an example his function which demonstrates how to create an Item
|||	    based on ATAG and BODY chunk contents:
|||
|||	    CreateATAGItem (const AudioTagHeader *atag, void *body)
|||	    {
|||	        return CreateItemVA (MKNODEID (AUDIONODE, atag->athd_NodeType),
|||	                body ? AF_TAG_ADDRESS        : TAG_NOP, body,
|||	                body ? AF_TAG_AUTO_FREE_DATA : TAG_NOP, TRUE,
|||	                TAG_JUMP, atag->athd_Tags);
|||	    }
|||
|||	    Since this uses AF_TAG_AUTO_FREE_DATA to hand off the body data to the
|||	    Item, the body data must be allocated with MEMTYPE_TRACKSIZE. Both
|||	    AF_TAG_ADDRESS and AF_TAG_AUTO_FREE_DATA tags are illegal for delay lines,
|||	    so don't specify these tags when there's no body data (a necessary
|||	    condition for a delay line).
|||
|||	    Note that this function as written expects the caller to have validated the
|||	    chunk contents.
|||
|||	  See Also
|||
|||	    LoadATAG(), ValidateAudioTagHeader(), Sample(@), Envelope(@), Tuning(@)
**/

/**
|||	AUTODOC -public -class libmusic -group ATAG -name AudioTagHeaderSize
|||	Returns size of AudioTagHeader for given number of tags.
|||
|||	  Synopsis
|||
|||	    int32 AudioTagHeaderSize (int32 numTags)
|||
|||	  Description
|||
|||	    This function returns the size of an AudioTagHeader in bytes for the given
|||	    number of tags.
|||
|||	  Arguments
|||
|||	    numTags
|||	        Number of tags.
|||
|||	  Return Value
|||
|||	    Size of the AudioTagHeader in bytes.
|||
|||	  Implementation
|||
|||	    Macro function implemented in <audio/atag.h> V29.
|||
|||	  Associated Files
|||
|||	    <audio/atag.h>
|||
|||	  See Also
|||
|||	    --ATAG-File-Format--(@), ValidateAudioTagHeader()
**/

/* keep the compiler happy... */
extern int foo;
