/* @(#) createdelaylinetemplate.c 96/07/01 1.3 */

#include <audio/audio.h>
#include <kernel/item.h>

/**
|||	AUTODOC -public -class audio -group Sample -name CreateDelayLineTemplate
|||	Creates a Delay Line Template Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Item CreateDelayLineTemplate (int32 numBytes, int32 numChannels, bool ifLoop)
|||
|||	  Description
|||
|||	    Delay Line Templates are simply descriptions of Delay Lines, which carry all
|||	    of the parameters of a Delay Line (e.g., number of channels, length, etc)
|||	    but have no delay line memory of their own. They may be attached to any
|||	    Instrument Template(@) to which Delay Lines may be attached (e.g.,
|||	    delay_f1.dsp(@), sampler_raw_f1.dsp(@)). Whenever an Instrument(@) is
|||	    created from an Instrument Template attached to a Delay Line Template, a new
|||	    Delay Line is automatically created from the Delay Line Template parameters,
|||	    and is then automatically attached to the new Instrument.
|||
|||	    Delay Line Templates are necessary to avoid the case where multiple
|||	    Instruments are created from a Patch Template attached to a Delay
|||	    Line. All such Instruments write to and read from the same Delay Line
|||	    memory, as if they each owned the Delay Line, which results in each
|||	    Instrument overwriting the signal written by the other Instruments.
|||	    However, with a Delay Line Template attached to the Patch Template,
|||	    each Instrument created from the Patch Template get its own independent
|||	    Delay Line.
|||
|||	    Delay Line Templates may not be attached to Instruments, just Instrument
|||	    Templates.
|||
|||	    When finished with the delay line template, delete it using
|||	    DeleteDelayLineTemplate(). Deleting a delay line template has no effect on
|||	    any delay lines which were created from it.
|||
|||	  Arguments
|||
|||	    numBytes
|||	        The number of bytes to be allocated for each delay line created from this
|||	        delay line template. No memory is allocated for the delay line template
|||	        itself. Note that this is in bytes, not frames.
|||
|||	    numChannels
|||	        The number of channels stored in each sample frame.
|||
|||	    ifLoop
|||	        TRUE if delay line should be treated as a circular buffer. This is
|||	        typically the case for delay and reverberation applications.
|||
|||	        FALSE for cases in which the delay line should be not be overwritten
|||	        (e.g., dumping DSP results to memory).
|||
|||	        This merely controls whether the delay line has a release loop. A
|||	        release loop is used to make all of the instruments operating on the
|||	        delay line immune to ReleaseInstrument(). Using a sustain loop instead
|||	        would complicate the use of delay lines in patches.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the delay line template
|||	    (non-negative), or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in libc.a V32.
|||
|||	  Example
|||
|||	    // Create a delay patch template with 1-second delay
|||	    // line template attached to it.
|||	    Item CreateDelayPatch (void)
|||	    {
|||	        Item patchTemplate;
|||	        Item delayLineTemplate = -1;
|||	        Err errcode;
|||
|||	            // create patch template
|||	        if ((errcode = patchTemplate = CreateEmptyDelayPatchTemplate( )) < 0)
|||	            goto clean;
|||
|||	            // create delay line template (1 second, mono)
|||	        if ((errcode = delayLineTemplate =
|||	            CreateDelayLineTemplate (44100*2, 1, TRUE)) < 0) goto clean;
|||
|||	            // attach send to delay line template
|||	        if ((errcode = CreateAttachmentVA (patchTemplate, delayLineTemplate,
|||	            AF_TAG_AUTO_DELETE_SLAVE, TRUE,
|||	            AF_TAG_NAME,              "SendFIFO",
|||	            TAG_END)) < 0) goto clean;
|||
|||	            // attach tap to delay line template 44050 samples away from send;
|||	            // practically 1 second
|||	        if ((errcode = CreateAttachmentVA (patchTemplate, delayLineTemplate,
|||	            AF_TAG_AUTO_DELETE_SLAVE, TRUE,
|||	            AF_TAG_NAME,              "TapFIFO",
|||	            AF_TAG_START_AT,          50,
|||	            TAG_END)) < 0) goto clean;
|||
|||	            // success
|||	        return patchTemplate;
|||
|||	    clean:
|||	            // clean up on failure
|||	        DeletePatchTemplate (patchTemplate);
|||	        DeleteDelayLineTemplate (delayLineTemplate);
|||	        return errcode;
|||	    }
|||
|||	    // Create simple delay patch template (no delay line;
|||	    // just the patch template).
|||	    static Item CreateEmptyDelayPatchTemplate (void)
|||	    {
|||	        const Item send = LoadInsTemplate ("delay_f1.dsp", NULL);
|||	        const Item tap = LoadInsTemplate ("sampler_raw_f1.dsp", NULL);
|||	        PatchCmdBuilder *pb = NULL;
|||	        Item result;
|||
|||	            // Open audiopatch folio, which we only need open long enough to
|||	            // build the patch. Do this first, and return rather than go the
|||	            // normal cleanup if it failed.
|||	        if ((result = OpenAudioPatchFolio()) < 0) return result;
|||
|||	            // create PatchCmdBuilder
|||	        if ((result = CreatePatchCmdBuilder (&pb)) < 0) goto clean;
|||
|||	            // Define ports
|||	        DefinePatchPort (pb, "Input", 1, AF_PORT_TYPE_INPUT,
|||	            AUDIO_SIGNAL_TYPE_GENERIC_SIGNED);
|||	        DefinePatchPort (pb, "Output", 1, AF_PORT_TYPE_OUTPUT,
|||	            AUDIO_SIGNAL_TYPE_GENERIC_SIGNED);
|||
|||	            // Add constituent templates to patch
|||	        AddTemplateToPatch (pb, "send", send);
|||	        AddTemplateToPatch (pb, "tap", tap);
|||
|||	            // Expose FIFOs
|||	        ExposePatchPort (pb, "SendFIFO", "send", "OutFIFO");
|||	        ExposePatchPort (pb, "TapFIFO", "tap", "InFIFO");
|||
|||	            // Connect signal path
|||	        ConnectPatchPorts (pb, NULL,  "Input",  0, "send", "Input",  0);
|||	        ConnectPatchPorts (pb, "tap", "Output", 0, NULL,   "Output", 0);
|||
|||	            // Did all of the PatchCmd constructors work?
|||	        if ((result = GetPatchCmdBuilderError (pb)) < 0) goto clean;
|||
|||	            // Create Patch Template from PatchCmd list returned from
|||	            // PatchCmdBuilder.
|||	        result = CreatePatchTemplateVA (GetPatchCmdList (pb),
|||	            TAG_ITEM_NAME, "delay.patch",   // optional, but sometimes helpful
|||	            TAG_END);
|||
|||	    clean:
|||	            // we don't need this stuff after we've built the patch
|||	        DeletePatchCmdBuilder (pb);
|||	        UnloadInsTemplate (send);
|||	        UnloadInsTemplate (tap);
|||	        CloseAudioPatchFolio();
|||
|||	            // return patch template item number or error code
|||	        return result;
|||	    }
|||
|||	  Notes
|||
|||	    This function is equivalent to:
|||
|||	  -preformatted
|||
|||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE),
|||	            AF_TAG_DELAY_LINE_TEMPLATE, numBytes,
|||	            AF_TAG_CHANNELS,     numChannels,
|||	            AF_TAG_RELEASEBEGIN, ifLoop ? 0 : -1,
|||	                                          // convert numBytes to frames
|||	            AF_TAG_RELEASEEND,   ifLoop ? numBytes / (numChannels * 2) : -1,
|||	            TAG_END);
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, libc.a
|||
|||	  See Also
|||
|||	    Sample(@), CreateDelayLine(), DeleteDelayLineTemplate(), CreateAttachment(),
|||	    CreateInstrument()
**/

Item CreateDelayLineTemplate (int32 numBytes, int32 numChannels, bool ifLoop)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE),
        AF_TAG_DELAY_LINE_TEMPLATE, numBytes,
        AF_TAG_CHANNELS,            numChannels,
        AF_TAG_RELEASEBEGIN,        ifLoop ? 0 : -1,
        AF_TAG_RELEASEEND,          ifLoop ? numBytes / (numChannels * 2) : -1,    /* convert numBytes to frames */
        TAG_END);
}


/**
|||	AUTODOC -public -class audio -group Sample -name DeleteDelayLineTemplate
|||	Deletes a Delay Line Template Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Err DeleteDelayLineTemplate (Item delayLineTemplate)
|||
|||	  Description
|||
|||	    This procedure deletes a delay line template item. It also deletes any
|||	    Attachment(@)s to the delay line template. This function has no effect
|||	    on delay lines created from this delay line template.
|||
|||	  Arguments
|||
|||	    delayLineTemplate
|||	        The item number of the delay line template to delete.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value if successful or an error code
|||	    (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro in <audio/audio.h> V32.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    Sample(@), CreateDelayLineTemplate(), DeleteAttachment()
**/
