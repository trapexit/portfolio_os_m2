/* @(#) createinstrument.c 96/08/23 1.2 */

#include <audio/audio.h>
#include <kernel/item.h>


/**
|||	AUTODOC -public -class audio -group Instrument -name CreateInstrument
|||	Allocates an Instrument(@) from an Instrument Template(@).
|||
|||	  Synopsis
|||
|||	    Item CreateInstrument (Item insTemplate, const TagArg *tagList)
|||
|||	    Item CreateInstrumentVA (Item insTemplate, uint32 tag1, ...)
|||
|||	  Description
|||
|||	    This procedure creates an Instrument(@) based on an Instrument Template(@)
|||	    and allocates the DSP resources necessary for the Instrument. See
|||	    Instrument(@) for the details. See also the convenience function
|||	    LoadInstrument(), which combines the two steps of loading a standard DSP
|||	    Instrument Template and creating an Instrument from it.
|||
|||	    Call DeleteInstrument() when you're finished with the Instrument to
|||	    deallocate its resources.
|||
|||	  Arguments
|||
|||	    insTemplate
|||	        Item number of an Instrument Template from which to allocate the
|||	        Instrument.
|||
|||	  Tag Arguments
|||
|||	    See Instrument(@) for the complete list of Instrument Tags.
|||
|||	    AF_TAG_CALCRATE_DIVIDE (uint32)
|||	        Specifies the denominator of the fraction of the total DSP cycles on
|||	        which this instrument is to run. The valid settings for this are:
|||
|||	        1 - Full rate execution (44,100 cycles/sec)
|||
|||	        2 - Half rate (22,050 cycles/sec)
|||
|||	        8 - 1/8 rate (5,512.5 cycles/sec) (new for M2)
|||
|||	        Defaults to 1.
|||
|||	    AF_TAG_PRIORITY (uint8)
|||	        Priority of new instrument in range of 0..200. Defaults to 100.
|||
|||	    AF_TAG_SET_FLAGS (uint32)
|||	        Set of AF_INSF_ flags to set in new instrument. All flags default to
|||	        cleared. See the Instrument(@) item page a for complete description of
|||	        AF_INSF_ flags.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number for the Instrument (a positive value)
|||	    or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in libc.a V29.
|||
|||	  Notes
|||
|||	    This function is equivalent to:
|||
|||	  -preformatted
|||
|||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
|||	                      AF_TAG_TEMPLATE, insTemplate,
|||	                      TAG_JUMP, tagList);
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteInstrument(), LoadInsTemplate(), LoadInstrument(), Instrument(@)
**/

Item CreateInstrument (Item insTemplate, const TagArg *tagList)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
                         AF_TAG_TEMPLATE, insTemplate,
                         TAG_JUMP,        tagList);
}


/**
|||	AUTODOC -public -class audio -group Instrument -name DeleteInstrument
|||	Frees an Instrument(@) allocated by CreateInstrument().
|||
|||	  Synopsis
|||
|||	    Err DeleteInstrument (Item InstrumentItem)
|||
|||	  Description
|||
|||	    This procedure frees an instrument allocated by CreateInstrument(), which
|||	    frees the resources allocated for the instrument. If the instrument is
|||	    running, it is stopped. All of the Knob(@)s and Probe(@)s belonging to this
|||	    Instrument are deleted. All Attachment(@)s made to this Instrument are
|||	    deleted. If any of those attachments were created with
|||	    { AF_TAG_AUTO_DELETE_SLAVE, TRUE }, the associated slave items are also
|||	    deleted. All connections to this Instrument are broken.
|||
|||	    If you delete an instrument Template(@), all Instruments created using that
|||	    Template are freed, so you don't need to use DeleteInstrument() on them.
|||
|||	    Do not confuse this function with UnloadInstrument(). If you create an
|||	    Instrument using LoadInstrument(), you shouldn't use DeleteInstrument() to
|||	    free the instrument because it will leave the instrument's Template loaded
|||	    and without any handle to the Template with which to unload it. Balance
|||	    CreateInstrument() with DeleteInstrument(); LoadInstrument() with
|||	    UnloadInstrument().
|||
|||	  Arguments
|||
|||	    InstrumentItem
|||	        The item number of the instrument to free.
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
|||	    CreateInstrument(), Instrument(@), Attachment(@)
**/
