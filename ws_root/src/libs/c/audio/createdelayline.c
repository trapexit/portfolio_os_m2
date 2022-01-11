/* @(#) createdelayline.c 96/07/29 1.6 */

#include <audio/audio.h>
#include <kernel/item.h>

/**
|||	AUTODOC -public -class audio -group Sample -name CreateDelayLine
|||	Creates a Delay Line Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Item CreateDelayLine (int32 numBytes, int32 numChannels, bool ifLoop)
|||
|||	  Description
|||
|||	    This procedure creates a delay line Sample(@) Item for echo, reverberation,
|||	    and oscilloscope applications. The delay line Item can be attached to any
|||	    instrument that provides an output FIFO (e.g., delay_f1.dsp(@)). It can be
|||	    tapped by a matching 16-bit sampler instrument (e.g., sampler_16_f1.dsp(@))).
|||
|||	    When finished with the delay line, delete it using DeleteDelayLine().
|||
|||	    If you intend to read the contents of a delay line with the CPU (including
|||	    writing it to a file, or doing any other non-DSP operation on it), you
|||	    should flush any data cache lines (using FlushDCache()) containing previous
|||	    delay line contents prior to reading from the delay line. See the Sample(@)
|||	    Item page for more details on this.
|||
|||	  Arguments
|||
|||	    numBytes
|||	        The number of bytes in the delay line's sample buffer. Note that this is
|||	        in bytes, not frames.
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
|||	    The procedure returns the item number of the delay line (non-negative), or
|||	    an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in libc.a V29.
|||
|||	  Examples
|||
|||	    {
|||	        delay = CreateDelayLine (44100*2, 1, TRUE);     // 1 second
|||
|||	        send = LoadInstrument ("delay_f1.dsp", 0, 100);
|||
|||	        tap = LoadInstrument ("sampler_16_f1.dsp", 0, 100);
|||
|||	        sendatt = CreateAttachmentVA (send, delay, TAG_END);
|||
|||	        tapatt = CreateAttachmentVA (tap, delay,
|||	            AF_TAG_START_AT, 50,    // 44050 samples away from send;
|||	                                    // practically 1 second
|||	            TAG_END);
|||
|||	        StartInstrument (tap, NULL);
|||	        StartInstrument (send, NULL);
|||
|||	        // Now, just connect send's Input to thing to be delayed.
|||	        // The delayed signal is available at tap's Ouptut.
|||	    }
|||
|||	    Note: Because of the non-atomic start of the two instruments, the actual
|||	    delay time is not predictable. If precise delay times are critical, create
|||	    a patch out of the delay instrument template, sampler(s), and signal routing
|||	    instruments.
|||
|||	  Notes
|||
|||	    This function is equivalent to:
|||
|||	  -preformatted
|||
|||	        CreateItemVA (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE),
|||	            AF_TAG_DELAY_LINE,   numBytes,
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
|||	    Sample(@), CreateDelayLineTemplate(), DeleteDelayLine(), CreateAttachment(),
|||	    delay_f1.dsp(@), delay_f2.dsp(@), sampler_16_f1.dsp(@), sampler_16_f2.dsp(@)
**/

Item CreateDelayLine (int32 numBytes, int32 numChannels, bool ifLoop)
{
    return CreateItemVA (MKNODEID(AUDIONODE,AUDIO_SAMPLE_NODE),
        AF_TAG_DELAY_LINE,   numBytes,
        AF_TAG_CHANNELS,     numChannels,
        AF_TAG_RELEASEBEGIN, ifLoop ? 0 : -1,
        AF_TAG_RELEASEEND,   ifLoop ? numBytes / (numChannels * 2) : -1,    /* convert numBytes to frames */
        TAG_END);
}


/**
|||	AUTODOC -public -class audio -group Sample -name DeleteDelayLine
|||	Deletes a Delay Line Sample(@) Item.
|||
|||	  Synopsis
|||
|||	    Err DeleteDelayLine (Item delayLine)
|||
|||	  Description
|||
|||	    This procedure deletes a delay line item and frees its resources. It also
|||	    deletes any Attachment(@)s to the delay line.
|||
|||	  Arguments
|||
|||	    delayLine
|||	        The item number of the delay line to delete.
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
|||	    Sample(@), CreateDelayLine(), DeleteAttachment()
**/
