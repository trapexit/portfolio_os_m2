/******************************************************************************
**
**  @(#) autodocs.c 96/03/25 1.2
**
**  Misc audio folio autodocs
**
******************************************************************************/

/**
|||	AUTODOC -class Audio -name --Audio-Port-Types--
|||	Audio port type descriptions.
|||
|||	  Description
|||
|||	    All Instrument(@) ports have one of the port types defined below. The port
|||	    type of any port may be read with GetInstrumentPortInfoByName() or
|||	    GetInstrumentPortInfoByIndex().
|||
|||	  Port Types
|||
|||	    AF_PORT_TYPE_INPUT
|||	        These ports may be the destination of a connection created with
|||	        ConnectInstrumentParts() and PATCH_CMD_CONNECT(@). They may be created
|||	        within a patch by PATCH_CMD_DEFINE_PORT(@). These ports may be single-
|||	        or multi-part and may have any signal type.
|||
|||	    AF_PORT_TYPE_OUTPUT
|||	        These ports may be the source of a connection created with
|||	        ConnectInstrumentParts() and PATCH_CMD_CONNECT(@), and may also be
|||	        examined with a Probe(@). They may be created within a patch by
|||	        PATCH_CMD_DEFINE_PORT(@). These ports may be single- or multi-part and
|||	        may have any signal type.
|||
|||	    AF_PORT_TYPE_KNOB
|||	        Knobs may be set manually (via CreateKnob() and SetKnobPart()) or be
|||	        the destination of a connection created with ConnectInstrumentParts()
|||	        and PATCH_CMD_CONNECT(@). They may be created within a patch by
|||	        PATCH_CMD_DEFINE_KNOB(@). Knobs may be single- or multi-part and may
|||	        have any signal type.
|||
|||	    AF_PORT_TYPE_IN_FIFO
|||	        These ports are for attaching a Sample(@)s to be played. They are always
|||	        single-part and have no signal type.
|||
|||	    AF_PORT_TYPE_OUT_FIFO
|||	        These ports are for attaching a delay line Sample(@) to be written to
|||	        by a delay instrument (e.g., delay_f1.dsp(@)). They are always
|||	        single-part and have no signal type.
|||
|||	    AF_PORT_TYPE_TRIGGER
|||	        Triggers are the means of signaling the host CPU by the DSP. They are
|||	        always single-part and have no signal type. See schmidt_trigger.dsp(@)
|||	        and ArmTrigger().
|||
|||	    AF_PORT_TYPE_ENVELOPE
|||	        These ports are for attaching Envelope(@)s. They are always single-part.
|||	        Envelope hooks have no signal type, but Envelope(@) items do. Envelope
|||	        hooks are always accompanied by a pair of knobs (with the same name as
|||	        the envelope hook plus the suffixes .request and .incr) which may be
|||	        controlled manually instead of with an Envelope(@). See envelope.dsp(@)
|||	        for details.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>
|||
|||	  See Also
|||
|||	    --Audio-Signal-Types--(@)
**/

/**
|||	AUTODOC -class Audio -name --Audio-Signal-Types--
|||	Audio signal type descriptions.
|||
|||	  Description
|||
|||	    Instrument(@) ports (Knobs, Inputs, and Outputs), Knob(@)s, Probe(@)s, and
|||	    Envelope(@)s all have a signal type, which defines the units and legal
|||	    ranges for their values. The signal type of an instrument port may be read
|||	    with GetInstrumentPortInfoByName() or GetInstrumentPortInfoByIndex().
|||	    Knob(@)s and Probe(@)s inherit the signal type of the port they are attached
|||	    to by default, but may be set to any other signal when created. An
|||	    Envelope(@)'s signal type determines the units of the envs_Value members of
|||	    its EnvelopeSegment array.
|||
|||	    Since the DSP only operates on generic signals, the others are merely
|||	    convenient representations. The conversion formula from generic to each
|||	    non-generic type is listed under the description for that type. In each
|||	    formula, Generic represents the internal generic signal, SystemSampleRate
|||	    represents the DAC sample rate, and CalcRateDivision is the
|||	    AF_TAG_CALCRATE_DIVIDE value for the instrument (either 1, 2, or 8). The
|||	    functions ConvertGenericToAudioSignal() and ConvertAudioSignalToGeneric()
|||	    perform conversions between generic and non-generic signal types.
|||
|||	  Signal Type Descriptions
|||
|||	    AUDIO_SIGNAL_TYPE_GENERIC_SIGNED
|||	        Signed generic signal (e.g., amplitude).
|||
|||	    AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED
|||	        Unsigned generic signal.
|||
|||	    AUDIO_SIGNAL_TYPE_OSC_FREQ
|||	        Oscillator frequency in Hertz.
|||
|||	        Freq (Hz) = Generic * SystemSampleRate / (CalcRateDivision * 2.0)
|||
|||	    AUDIO_SIGNAL_TYPE_LFO_FREQ
|||	        Low-frequency oscillator frequency in Hertz.
|||
|||	        Freq (Hz) = Generic * SystemSampleRate / (CalcRateDivision * 2.0 * 256)
|||
|||	    AUDIO_SIGNAL_TYPE_SAMPLE_RATE
|||	        Sample rate in samples/second.
|||
|||	        SampleRate (Hz) = Generic * SystemSampleRate / CalcRateDivision
|||
|||	    AUDIO_SIGNAL_TYPE_WHOLE_NUMBER
|||	        Whole numbers in the range of -32768 to 32767.
|||
|||	        Whole = Generic * 32768.0
|||
|||	  Signal Ranges And Precision -preformatted
|||
|||	    Signal Type                              Min          Max      Precision    Units
|||	    ----------------------------------  ------------  -----------  -----------  --------
|||	    AUDIO_SIGNAL_TYPE_GENERIC_SIGNED        -1.00000      0.99997  3.05176e-05
|||	    AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED       0.00000      1.99997  3.05176e-05
|||	    AUDIO_SIGNAL_TYPE_OSC_FREQ          -22050.00000  22049.32617  0.67291      Hz *
|||	    AUDIO_SIGNAL_TYPE_LFO_FREQ             -86.13281     86.13018  2.62856e-03  Hz *
|||	    AUDIO_SIGNAL_TYPE_SAMPLE_RATE            0.00000  88198.65625  1.34583      samp/s *
|||	    AUDIO_SIGNAL_TYPE_WHOLE_NUMBER      -32768.00000  32767.00000  1.00000
|||
|||	    * Actual frequency ranges and precision are determined by instrument execution
|||	      rate, which is determined by DAC sample rate (normally 44100) and instrument
|||	      calculation rate division. As presented, the instrument execution rate is
|||	      assumed to be 44100 times/second.
|||
|||	  Associated Files
|||
|||	    <audio/audio_signals.h>, <audio/audio.h>
|||
|||	  See Also
|||
|||	    --Audio-Port-Types--(@), ConvertAudioSignalToGeneric(),
|||	    ConvertGenericToAudioSignal(), GetAudioSignalInfo()
**/


/* keep the compiler happy... */
extern int foo;
