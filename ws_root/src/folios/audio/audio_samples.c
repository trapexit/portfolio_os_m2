/* @(#) audio_samples.c 96/08/29 1.67 */
/* $Id: audio_samples.c,v 1.97 1995/03/02 07:40:14 phil Exp phil $ */
/****************************************************************
**
** Audio Internals to support Samples
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/*
** 921116 PLB Do not error out on unrecognized AIFF chunks.
** 921118 PLB Move AIFF sample loading to User Level
**              Load entire sample then pass to CreateItem for blessing.
** 921203 PLB Fixed error trapping and reporting.
** 921207 PLB Free sample memory in User Mode.
** 921214 PLB Use Block I/O for reading sound file.
** 930127 PLB Return actual error in LoadSample.
** 930308 PLB Use new IFF parser to load AIFF
** 930415 PLB Track connections between Items
** 930504 PLB Add Sample Rate, and AF_TAG_SAMPLE_RATE
** 930506 PLB Allow sample Address changes.
** 930510 PLB Added AF_TAG_COMPRESSIONRATIO, TYPE and NUMBITS
** 930524 PLB Add LoadSampleHere
** 930527 PLB internalSetSampleInfo was returning a 1
** 930628 PLB Fixed reading of ReleaseEnd from Sample
** 930715 PLB Added asmp_SuperFlags, DelayLine
** 930824 PLB Changed debug and trace text.
** 930825 PLB Allow attached samples to use Custom Allocate and Free functions.
** 930830 PLB Disallow negative number of channels, free mem if delay line creation fails.
** 930907 PLB Don't attempt to free mem for delay lines in user mode.  Causes fence violation.
** 931129 PLB Use Detune in CalcSampleBaseFreq
** 931215 PLB Added CreateSample()
** 940304 PLB Set NumBytes before calling internalSetSampleInfo so Lemmings doesn't crash.
** 940429 PLB Allow illegal call to MakeSample() so MadDog McCree doesn't lose soundtrack.
** 940506 PLB Added support for changeable DAC sample rates.
** 940513 PLB DebugSample() now uses decimal for reporting most numbers.
** 940606 PLB Added \n to an error message.
** 940614 PLB Added miserable kludge for Mad Dog McCree's bug.  If they call MakeSample()
**            and request both an allocation and a specific address, then we do both
**            and then only use the allocated memory if they call GetAudioItemInfo()
**            and ask for the address back.  Otherwise we can assume the caller really
**            wanted the specified address.  Note the allocated memory does not get freed
**            unless they use it.  This is as before.  This change was made for ROM over CD
**            so that Mad Dog won't break under Anvil OS.
** 940727 WJB Added a quickie fix to permit ScanSample() to parse the header of an
*             AIFF with markers. [REMOVED]
** 940809 PLB Reject a CompressionRatio<1 in SetSampleInfo, avoid zero divide
**            Removed quickie fix from 940727 because it allowed markers outside legal memory.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 940812 PLB Implement LEAVEINPLACE, Make LEAVEINPLACE illegal for files.
** 940817 PLB Shift Frequency by Shift Rate to compensate for execution rate.
** 940907 PLB Frame to byte conversions moved from dspp_touch.c
** 940912 PLB Use Read16 to read uint16 values for NuPup
** 940921 PLB Handle AIFF files where MARK is after INST
** 941121 PLB In MakeSample(), set number of frames to match number of
**            bytes and default format.
** 941128 PLB Set UpdateSize if CompressionRatio set.
** 941128 PLB Recompute NumBits if width set.
** 941212 WJB Tweaked printed messages for AF_TAG_LEAVE_IN_PLACE.
** 941213 WJB Added debug line in UnloadSample()
** 941216 PLB Fix double freemem for AF_TAG_LEAVE_IN_PLACE
** 950322 PLB Merged Saumitra's EAS changes for compatible callback.
** 950419 WJB Added prototypes for CustomAlloc/FreeMem() functions pointers.
** 950627 PLB Converted to float32.
** 950628 PLB Removed MakeSample() and Mad Dog hacks.
** 950818 PLB Moved all file parsing to music library.
** 951003 WJB Rewrote Item management functions.
** 951109 WJB Changed AF_TAG_AUTO_FREE_DATA to use MEMTYPE_TRACKSIZE.
*****************************************************************/

#include <kernel/cache.h>           /* WriteBackDCache() */

#include "audio_internal.h"

/* Macros for debugging. */

#define DEBUG_Item           0      /* debug create/delete */
#define DEBUG_ClearDelayLine 0      /* debug delay line clearing (initial and triggered by AF_SAMPF_CLEARONSTOP) */

#if DEBUG_Item
#define DBUGITEM(x) PRT(x)
#else
#define DBUGITEM(x)
#endif

#if DEBUG_ClearDelayLine
#include <kernel/time.h>
#define DBUGCLR(x) PRT(x)
#else
#define DBUGCLR(x)
#endif


/* -------------------- Local defines */

    /* legal AF_SAMPF_ flags for ordinary samples */
#define AF_SAMPF_ORDINARY_LEGALFLAGS    (AF_SAMPF_LEGALFLAGS & ~(AF_SAMPF_CLEARONSTOP))


/* -------------------- Local functions */

static void ClearDelayLine (AudioSample *);

static Err CalcSampleBaseFreq (AudioSample *asmp);


/* -------------------- Item autodoc */

 /**
 |||	AUTODOC -public -class Items -group Audio -name Sample
 |||	A digital recording of a sound.
 |||
 |||	  Description
 |||
 |||	    A Sample Item is a handle to a digital recording of a sound in memory.
 |||	    Samples come in three kinds:
 |||
 |||	    Ordinary Samples
 |||	        Samples that are playable by various instruments (e.g.,
 |||	        sampler_16_v1.dsp(@)). Memory for these samples is provided by the
 |||	        client.
 |||
 |||	    Delay Lines
 |||	        Special samples suitable for receiving the DMA output from delay
 |||	        instruments (e.g., delay_f1.dsp(@)). They may be used for creating
 |||	        special effects such as reverb or echo, and also for capturing DSP
 |||	        output to RAM. The memory for delay lines is allocated from supervisor
 |||	        memory by the audio folio. Delay line memory is cleared when allocated.
 |||	        Once an Attachment to it has been started, it contains whatever was
 |||	        last written to it. The AF_SAMPF_CLEARONSTOP flag can be used to cause
 |||	        the delay line memory to be cleared when the all the Attachments to it
 |||	        have stopped.
 |||
 |||	    Delay Line Templates
 |||	        Descriptions of Delay Lines, which carry all of the parameters of a
 |||	        Delay Line (e.g., number of channels, length, etc) but have no delay
 |||	        line memory of their own. They may be attached to any Instrument
 |||	        Template(@) to which Delay Lines may be attached (e.g., delay_f1.dsp(@),
 |||	        sampler_raw_f1.dsp(@)). Whenever an Instrument(@) is created from an
 |||	        Instrument Template attached to a Delay Line Template, a new Delay Line
 |||	        is automatically created from the Delay Line Template parameters, and
 |||	        is then automatically attached to the new Instrument.
 |||
 |||	        Delay Line Templates are necessary to avoid the case where multiple
 |||	        Instruments are created from a Patch Template attached to a Delay
 |||	        Line. All such Instruments write to and read from the same Delay Line
 |||	        memory, as if they each owned the Delay Line, which results in each
 |||	        Instrument overwriting the signal written by the other Instruments.
 |||	        However, with a Delay Line Template attached to the Patch Template,
 |||	        each Instrument created from the Patch Template get its own independent
 |||	        Delay Line.
 |||
 |||	        Delay Line Templates may not be attached to Instruments, just Instrument
 |||	        Templates.
 |||
 |||	    Since the DSP accesses Sample and Delay Line memory via DMA, and DMA can't
 |||	    access the CPU's data cache, you must be somewhat cache-aware when using
 |||	    Samples and Delay Lines.
 |||
 |||	    Ordinary Samples and the Data Cache
 |||	        Since the DSP can't read from the CPU's data cache, any CPU-made
 |||	        modifications to sample data (including loading sample data from disc or
 |||	        any other device) must be written back to memory before the DSP can play
 |||	        it. CreateSample() and SetAudioItemInfo() automatically take care of
 |||	        this when binding sample data to a Sample Item (by setting
 |||	        AF_TAG_ADDRESS, AF_TAG_FRAMES, or AF_TAG_NUMBYTES). So, if you never
 |||	        modify the sample data after binding it to the Sample Item, then you do
 |||	        not need to be concerned with this issue.
 |||
 |||	        If, however, you modify a Sample Item's sample data after binding it to
 |||	        the Sample Item, then you must call WriteBackDCache() for the modified
 |||	        portion of the sample data, or set AF_TAG_ADDRESS, AF_TAG_FRAMES, or
 |||	        AF_TAG_NUMBYTES again with SetAudioItemInfo(). Failure to do so can
 |||	        result in intermittent audio noise.
 |||
 |||	    Delay Lines and the Data Cache
 |||	        When the DSP writes to a delay line, it writes to main memory without
 |||	        regard for what is in the CPU's data cache. Therefore it is possible for
 |||	        the data cache to contain stale data from a previous read of the delay
 |||	        line. If you intend to read the contents of a delay line with the CPU
 |||	        (including writing it to a file, or doing any other non-DSP operation
 |||	        on it), you should flush any data cache lines (using FlushDCache())
 |||	        containing previous delay line contents prior to reading from the delay
 |||	        line.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_SAMPLE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateDelayLine(), CreateDelayLineTemplate(), CreateItem(), CreateSample()
 |||
 |||	  Delete
 |||
 |||	    DeleteDelayLine(), DeleteDelayLineTemplate(), DeleteItem(), DeleteSample()
 |||
 |||	  Query
 |||
 |||	    GetAudioItemInfo()
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    CreateAttachment(), DebugSample()
 |||
 |||	  Tags
 |||
 |||	    Data:
 |||
 |||	    AF_TAG_ADDRESS (const void *) - Create*, Query, Modify*+
 |||	        Address of sample data. The length of the data may be specified in bytes
 |||	        using AF_TAG_NUMBYTES or in sample frames using AF_TAG_FRAMES. This data
 |||	        must remain valid for the life of the Sample or until another address is
 |||	        set with AF_TAG_ADDRESS.
 |||
 |||	        Can set to NULL for a sample Item without data. This is useful if you
 |||	        intend to hook multiple data pointers to the Sample Item using
 |||	        SetAudioItemInfo() some time after creation. NULL address should be used
 |||	        with 0 length.
 |||
 |||	        If the sample is created with { AF_TAG_AUTO_FREE_DATA, TRUE }, then this
 |||	        data will be freed automatically when the Sample is deleted. Memory used
 |||	        with AF_TAG_AUTO_FREE_DATA must be allocated with MEMTYPE_TRACKSIZE set.
 |||	        NULL address and 0 size are not legal in this case.
 |||
 |||	        Defaults to NULL on creation. When specified, triggers automatic
 |||	        WriteBackDCache() of sample data.
 |||
 |||	    AF_TAG_AUTO_FREE_DATA (bool) - Create*
 |||	        Set to TRUE to cause data pointed to by AF_TAG_ADDRESS to be
 |||	        automatically freed when Sample Item is deleted. If the Item isn't
 |||	        successfully created, the memory isn't freed.
 |||
 |||	        The memory pointed to by AF_TAG_ADDRESS must be freeable by
 |||	        FreeMem (Address, TRACKED_SIZE).
 |||
 |||	    AF_TAG_FRAMES (int32) - Create*, Query, Modify*+
 |||	        Length of sample data expressed in frames. In a stereo sample, this
 |||	        would be the number of stereo pairs. Defaults to 0 on creation. When
 |||	        specified, triggers automatic WriteBackDCache() of sample data.
 |||
 |||	    AF_TAG_NUMBYTES (int32) - Create*, Query, Modify*+
 |||	        Length of sample data expressed in bytes. Defaults to 0 on creation.
 |||	        When specified, triggers automatic WriteBackDCache() of sample data.
 |||
 |||	    AF_TAG_SUPPRESS_WRITE_BACK_D_CACHE (bool) - Create*, Modify*+
 |||	        Suppress the normal WriteBackDCache() of the sample data triggered by
 |||	        AF_TAG_ADDRESS, AF_TAG_FRAMES, or AF_TAG_NUMBYTES. This tag has no
 |||	        lasting effect; it merely controls the behavior of a single call to
 |||	        CreateSample() or SetAudioItemInfo(). Defaults to FALSE.
 |||
 |||	    AF_TAG_DELAY_LINE (int32) - Create
 |||	        Number of bytes to be allocated from supervisor memory for delay line.
 |||	        Causes the Sample to be a Delay Line. Mutually exclusive with
 |||	        AF_TAG_ADDRESS, AF_TAG_FRAMES, AF_TAG_NUMBYTES, AF_TAG_AUTO_FREE_DATA,
 |||	        and AF_TAG_DELAY_LINE_TEMPLATE.
 |||
 |||	    AF_TAG_DELAY_LINE_TEMPLATE (int32) - Create
 |||	        Number of bytes to be allocated from supervisor memory for each delay line
 |||	        created from this delay line template. No memory is allocated for the delay
 |||	        line template itself. Causes the Sample to be a Delay Line Template.
 |||	        Mutually exclusive with AF_TAG_ADDRESS, AF_TAG_FRAMES, AF_TAG_NUMBYTES,
 |||	        AF_TAG_AUTO_FREE_DATA, and AF_TAG_DELAY_LINE.
 |||
 |||	    * These operations are allowed for ordinary samples, but not for delay lines
 |||	    or delay line templates.
 |||
 |||	    + These tags cannot be used to modify a Sample created with
 |||	    { AF_TAG_AUTO_FREE_DATA, TRUE }.
 |||
 |||	    Format:
 |||
 |||	    AF_TAG_CHANNELS (uint8) - Create, Query, Modify
 |||	        Number of channels (or samples per sample frame). For example: 1 for
 |||	        mono, 2 for stereo. Valid range is 1..255. Defaults to 1 on creation.
 |||
 |||	    AF_TAG_WIDTH (uint8) - Create, Query, Modify
 |||	        Number of bytes per sample (uncompressed). Valid range is 1..2. Defaults
 |||	        to 2 on creation.
 |||
 |||	    AF_TAG_NUMBITS (uint8) - Create, Query, Modify
 |||	        Number of bits per sample (uncompressed). Valid range is 1..16. Width
 |||	        is rounded up to the next byte when computed from this tag. Defaults to
 |||	        16 on creation.
 |||
 |||	    AF_TAG_COMPRESSIONTYPE (PackedID) - Create, Query, Modify
 |||	        32-bit ID representing AIFC compression type of sample data (e.g.,
 |||	        ID_SDX2). Use 0 for no compression, which is the default for creation.
 |||
 |||	    AF_TAG_COMPRESSIONRATIO (uint8) - Create, Query, Modify.
 |||	        Compression ratio of sample data. Uncompressed data has a value of 1.
 |||	        Valid range is 1..255. Defaults to 1 on creation.
 |||
 |||	    Note: These tags affect the frame size and therefore also affect the
 |||	    relationship between AF_TAG_FRAMES and AF_TAG_NUMBYTES.
 |||
 |||	    Loops:
 |||
 |||	    AF_TAG_SUSTAINBEGIN (int32) - Create, Query, Modify.
 |||	        Frame index of the first frame of the sustain loop. Valid range is
 |||	        0..NumFrames-1. -1 for no sustain loop. Use in conjunction with
 |||	        AF_TAG_SUSTAINEND.
 |||
 |||	    AF_TAG_SUSTAINEND (int32) - Create, Query, Modify.
 |||	        Frame index of the first frame after the last frame in the sustain
 |||	        loop. Valid range is 1..NumFrames. -1 for no sustain loop. Use in
 |||	        conjunction with AF_TAG_SUSTAINBEGIN.
 |||
 |||	    AF_TAG_RELEASEBEGIN (int32) - Create, Query, Modify.
 |||	        Frame index of the first frame of the release loop. Valid range is
 |||	        0..NumFrames-1. -1 for no release loop. Use in conjunction with
 |||	        AF_TAG_RELEASEEND.
 |||
 |||	    AF_TAG_RELEASEEND (int32) - Create, Query, Modify
 |||	        Frame index of the first frame after the last frame in the release loop.
 |||	        Valid range is 1..NumFrames. -1 for no release loop. Use in conjunction
 |||	        with AF_TAG_RELEASEBEGIN.
 |||
 |||	    Tuning:
 |||
 |||	    AF_TAG_BASENOTE (uint8) - Create, Query, Modify
 |||	        MIDI note number for this sample when played at the original sample
 |||	        rate (as set by AF_TAG_SAMPLE_RATE_FP). This defines the frequency
 |||	        conversion reference note for the StartInstrument() AF_TAG_PITCH tag.
 |||	        Defaults to middle C (60) on creation.
 |||
 |||	    AF_TAG_DETUNE (int8) - Create, Query, Modify
 |||	        Amount in cents to detune playback in order to reach the exact pitch of
 |||	        the MIDI base note. Positive values cause sample to be played back at a
 |||	        higher sample rate; negative values cause sample to be played back at a
 |||	        lower sample rate.
 |||
 |||	        For example, 50 causes sample to be played back at a sample rate 50 cents
 |||	        sharp of the AF_TAG_SAMPLE_RATE_FP sample rate in order to reach the
 |||	        AF_TAG_BASENOTE pitch. This would be used to compensate for a sample that
 |||	        was recorded 50 cents flat of the AF_TAG_BASENOTE pitch at the
 |||	        AF_TAG_SAMPLE_RATE_FP sample rate.
 |||
 |||	        Must be in the range of -100 to 100. Defaults to 0 on creation.
 |||
 |||	    AF_TAG_SAMPLE_RATE_FP (float32) - Create, Query, Modify
 |||	        Original sample rate in Hz. Defaults to 44,100 Hz on creation.
 |||
 |||	    AF_TAG_BASEFREQ_FP (float32) - Query
 |||	        The frequency of the sample, in Hz, when played at the DAC sample
 |||	        rate (as returned by GetAudioFolioInfo() using the
 |||	        AF_TAG_SAMPLE_RATE_FP tag). This value is computed from the other tuning
 |||	        tag values.
 |||
 |||	        Note: this is the frequency value of the sample when played at the DAC
 |||	        sample rate, not the Sample's original sample rate.
 |||
 |||	    Multisample:
 |||
 |||	    AF_TAG_LOWNOTE (uint8) - Create, Query, Modify
 |||	        Lowest MIDI note number at which to play this sample when part of a
 |||	        multisample. StartInstrument() AF_TAG_PITCH tag is used to perform
 |||	        selection. Valid range is 0 to 127. Defaults to 0 on creation.
 |||
 |||	    AF_TAG_HIGHNOTE (uint8) - Create, Query, Modify
 |||	        Highest MIDI note number at which to play this sample when part of a
 |||	        multisample. Valid range is 0 to 127. Defaults to 127 on creation.
 |||
 |||	    AF_TAG_LOWVELOCITY (uint8) - Create, Query, Modify
 |||	        Lowest MIDI attack velocity at which to play this sample when part of a
 |||	        multisample. StartInstrument() velocity tags (e.g., AF_TAG_VELOCITY) are
 |||	        used to perform selection. Range is 0 to 127. Defaults to 0 on creation.
 |||
 |||	    AF_TAG_HIGHVELOCITY (uint8) - Create, Query, Modify
 |||	        Highest MIDI attack velocity at which to play this sample when part of
 |||	        a multisample. Range is 0 to 127. Defaults to 127 on creation.
 |||
 |||	    Misc:
 |||
 |||	    AF_TAG_CLEAR_FLAGS (uint32) - Create, Modify
 |||	        Set of AF_SAMPF_ flags (see below) to clear. Clears every flag for which
 |||	        a 1 is set in ta_Arg.
 |||
 |||	    AF_TAG_SET_FLAGS (uint32) - Create, Query, Modify
 |||	        Set of AF_SAMPF_ flags (see below) to set. Sets every flag for which a 1
 |||	        is set in ta_Arg. Defaults to 0 on creation.
 |||
 |||	  Flags
 |||
 |||	    AF_SAMPF_CLEARONSTOP
 |||	        Causes the delay line memory to be cleared when all Attachments to the
 |||	        delay line stop. This is useful for patches containing delay lines
 |||	        where the delay line must contain all zeroes when the instrument is
 |||	        restarted after being stopped. This flag is valid only for Delay Lines
 |||	        and Delay Line Templates.
 |||
 |||	  Caveats
 |||
 |||	    Be careful when modifying sample data after binding it to a Sample Item and
 |||	    when using AF_TAG_SUPPRESS_WRITE_BACK_D_CACHE. Playing modified sample data
 |||	    which hasn't yet been written back to memory can result in intermittent audio
 |||	    noise (the sort of noise that goes away when you try to find the cause, but
 |||	    returns immediately after you have given up looking for it).
 |||
 |||	    All sample data, loop points, and lengths must be byte aligned. For
 |||	    example, a 1-channel, ADPCM sample (which has 4 bits per frame) is only
 |||	    legal when the lengths and loop points are at multiples of 2 frames.
 |||
 |||	    In the development version of the audio folio, frame- and byte-aligned
 |||	    sample lengths and byte-aligned loop points are enforced. In the production
 |||	    version of the audio folio, these traps are removed with the expectation
 |||	    that you are using valid lengths and loop points. Failure to do so may
 |||	    result in noise at loop points, or slight popping at the ending of sound
 |||	    playback. It is recommended that you pay very close attention to sample
 |||	    lengths and loop points when creating, converting, and compressing samples.
 |||
 |||	    Not all sample parameters are copied by Delay line Template to Delay Line
 |||	    promotion. Besides delay line length, only the following sample attributes
 |||	    are copied: TAG_ITEM_NAME, AF_TAG_CHANNELS, AF_TAG_SUSTAINBEGIN,
 |||	    AF_TAG_SUSTAINEND, AF_TAG_RELEASEBEGIN, AF_TAG_RELEASEEND, and
 |||	    AF_TAG_SET_FLAGS. All others are set to their default values.
 |||
 |||	  See Also
 |||
 |||	    Attachment(@), Instrument(@), Template(@), CreateDelayLine(),
 |||	    CreateDelayLineTemplate(), StartInstrument(), sampler_16_v1.dsp(@),
 |||	    delay_f1.dsp(@), LoadSample()
 **/


/* -------------------- Create/Delete/Modify/Query Sample Item */

typedef struct SampleTagData {
    uint8   std_SampleKind;             /* SAMPLEKIND_ value set by last kind-setting tag (only really valid for Create, not Set) */
    bool    std_AddressChanged;         /* asmp_Address was set */
    bool    std_NumFramesChanged;       /* asmp_NumFrames was set (e.g., AF_TAG_FRAMES) */
    bool    std_NumBytesChanged;        /* asmp_NumBytes was set (e.g., AF_TAG_NUMBYTES) */
    bool    std_FrameSizeChanged;       /* asmp_Width, asmp_NumBits, or asmp_CompressionRatio was set */
    bool    std_TuningChanged;          /* asmp_BaseNote or asmp_SampleRate was set */
    bool    std_SuppressWriteBackDCache; /* bypass WriteBackDCache() in CompleteAndValidateAudioSample() (ignored for delay lines) */
} SampleTagData;

    /* Sample Kinds */
enum {
    SAMPLEKIND_DEFAULT,                 /* no kind-setting tags were specified, becomes an ordinary sample (@@@ assumed to be 0) */
    SAMPLEKIND_ORDINARY,                /* set by AF_TAG_ADDRESS, AF_TAG_NUMBYTES, AF_TAG_FRAMES, AF_TAG_AUTO_FREE_DATA */
    SAMPLEKIND_DELAY_LINE,              /* set by AF_TAG_DELAY_LINE */
    SAMPLEKIND_DELAY_LINE_TEMPLATE      /* set by AF_TAG_DELAY_LINE_TEMPLATE */
};

#if defined(BUILD_STRINGS) || DEBUG_Item
    /* Sample kind descriptions used for debug and diagnostic output */
static const char * const sampleKindDesc[] = {      /* @@@ indexed by sample kinds */
    "Default",
    "Ordinary Sample",
    "Delay Line",
    "Delay Line Template",
};
#endif

static void SetSampleDefaults (AudioSample *);
static Err CreateSampleTagHandler (AudioSample *, SampleTagData *, uint32 tag, TagData arg);
static Err SampleTagHandler (AudioSample *, SampleTagData *, uint32 tag, TagData arg);
static Err SetDelayLineKindAndLength (AudioSample *, SampleTagData *, uint8 sampleKind, int32 numBytes);
static Err SetSampleKind (SampleTagData *, uint8 sampleKind);
static Err SetSampleMIDIParameter (uint8 *resultValue, int32 val);
static Err SetSampleLoopParameter (int32 *resultValue, int32 val);
static Err CompleteAndValidateAudioSample (AudioSample *, const SampleTagData *);

/*****************************************************************/
/* AUDIO_SAMPLE_NODE ir_Create method */
Item internalCreateAudioSample (AudioSample *asmp, const TagArg *tagList)
{
    Err errcode;

    DBUGITEM(("internalCreateAudioSample(0x%x,0x%x)\n", asmp, tagList));

        /* pre-init asmp */
    PrepList (&asmp->asmp_AttachmentRefs);
    SetSampleDefaults (asmp);

        /* process tags, validate results */
    {
        SampleTagData tagdata;

            /* init tagdata */
        memset (&tagdata, 0, sizeof tagdata);
        tagdata.std_TuningChanged = TRUE;       /* force updating base frequency */

            /* process tags */
        if ((errcode = TagProcessor (asmp, tagList, CreateSampleTagHandler, &tagdata))) goto clean;

            /* handle kind-specific stuff */
        DBUGITEM(("internalCreateSample: sample kind: %s (%d)\n", sampleKindDesc[tagdata.std_SampleKind], tagdata.std_SampleKind));
        switch (tagdata.std_SampleKind) {
            case SAMPLEKIND_DELAY_LINE:
                DBUGITEM(("internalCreateAudioSample: alloc delay line %d bytes, rounded to %d bytes (%d byte dcache line)\n",
                    asmp->asmp_NumBytes, ALLOC_ROUND(asmp->asmp_NumBytes,AB_FIELD(af_DCacheLineSize)), AB_FIELD(af_DCacheLineSize)));

                    /* allocate data cache line-aligned, supervisor delay line memory */
                if (!(asmp->asmp_Data = SuperAllocMemAligned (
                    ALLOC_ROUND(asmp->asmp_NumBytes,AB_FIELD(af_DCacheLineSize)),
                    MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE,
                    AB_FIELD(af_DCacheLineSize)))) {

                    errcode = AF_ERR_NOMEM;
                    goto clean;
                }

                    /* clear delay line memory (cache-aware) */
                ClearDelayLine (asmp);
                break;

          #if 0     /* comment these out since they don't do anything and appear to occupy code space if left in */
            case SAMPLEKIND_DELAY_LINE_TEMPLATE:
                /* nothing special to do */
                break;

            case SAMPLEKIND_ORDINARY:
            default:    /* SAMPLEKIND_DEFAULT */
                /* nothing special to do */
                break;
          #endif
        }

            /* complete changes based on additional stuff in tagdata and validate results */
        if ((errcode = CompleteAndValidateAudioSample (asmp, &tagdata)) < 0) goto clean;
    }

        /* add to sample list (permits system wide update asmp_BaseFreq if DAC sample rate changes) */
    AddTail (&AB_FIELD(af_SampleList), (Node *)asmp);

  #if DEBUG_Item
    internalDumpSample (asmp, "internalCreateAudioSample");
  #endif

        /* return Item as success */
    return asmp->asmp_Item.n_Item;

clean:
        /* clean up allocations on error */
    if (IsDelayLine(asmp)) {
        DBUGITEM(("internalCreateSample: delete delay line @ 0x%x\n", asmp->asmp_Data));
        SuperFreeMem (asmp->asmp_Data, TRACKED_SIZE);
    }
    return errcode;
}

/*****************************************************************/
/* AUDIO_SAMPLE_NODE SetAudioItemInfo() method */
Err internalSetSampleInfo (AudioSample *asmp, const TagArg *tagList)
{
    AudioSample tempasmp = *asmp;
    Err errcode;

        /* process tags and validate results */
        /* (this doesn't allocate anything, so no need to clean anything up) */
        /* @@@ this must prevent any changes to tempasmp that are invalid in asmp
           - does blind structure copy to apply changes */
    {
        SampleTagData tagdata;

        memset (&tagdata, 0, sizeof tagdata);
        /* @@@ doesn't set std_SampleKind */

            /* process tags */
        {
            const TagArg *tag;
            int32 tagResult;

            for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
                DBUGITEM(("internalSetSampleInfo: tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));

                switch (tag->ta_Tag) {
                    case AF_TAG_ADDRESS:
                    case AF_TAG_FRAMES:
                    case AF_TAG_NUMBYTES:
                        if (IsDelayLine(&tempasmp)) {
                            ERR(("internalSetSampleInfo: Can't change data address or size of delay line\n"));
                            return AF_ERR_BADTAG;
                        }
                        if (tempasmp.asmp_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
                            ERR(("internalSetSampleInfo: Created with { AF_TAG_AUTO_FREE_DATA, TRUE }; can't change data address or size\n"));
                            return AF_ERR_BADTAG;
                        }
                        /* fall thru to default */

                    default:
                        if ((errcode = SampleTagHandler (&tempasmp, &tagdata, tag->ta_Tag, tag->ta_Arg)) < 0) return errcode;
                        break;
                }
            }

                /* Catch tag processing errors */
            if ((errcode = tagResult) < 0) {
                ERR(("internalSetSampleInfo: Error processing tag list 0x%x\n", tagList));
                return errcode;
            }
        }

            /* complete changes based on additional stuff in tagdata and validate results */
        if ((errcode = CompleteAndValidateAudioSample (&tempasmp, &tagdata)) < 0) return errcode;
    }

        /* apply results */
    *asmp = tempasmp;

  #if DEBUG_Item
    internalDumpSample (asmp, "internalSetSampleInfo");
  #endif

    return 0;
}

/*
    Set sample defaults. Assumes that caller has precleared AudioSample.
    Doesn't initialize asmp_BaseFreq. Expects caller to do that after processing
    tags.
*/
static void SetSampleDefaults (AudioSample *asmp)
{
        /* format */
    asmp->asmp_Bits             = 16;
    asmp->asmp_Width            = 2;
    asmp->asmp_Channels         = 1;
    asmp->asmp_CompressionRatio = 1;

        /* loops */
    asmp->asmp_SustainBegin     = -1;
    asmp->asmp_SustainEnd       = -1;
    asmp->asmp_ReleaseBegin     = -1;
    asmp->asmp_ReleaseEnd       = -1;

        /* tuning */
    asmp->asmp_BaseNote         = AF_MIDDLE_C_PITCH;
    asmp->asmp_SampleRate       = DEFAULT_SAMPLERATE;

        /* multisample */
    asmp->asmp_HighNote         = 127;
    asmp->asmp_HighVelocity     = 127;
}

/*
    Sample TagProcessor() callback function for internalCreateAudioSample().

    Arguments
        asmp
            AudioSample Item to fill out. Client must initialize it before
            calling TagProcessor().

        tagdata
            SampleTagData to fill out.
*/
static Err CreateSampleTagHandler (AudioSample *asmp, SampleTagData *tagdata, uint32 tag, TagData arg)
{
    Err errcode;

    DBUGITEM(("CreateSampleTagHandler: tag { %d, 0x%x }\n", tag, arg));

    switch (tag) {
        case AF_TAG_AUTO_FREE_DATA:
            if ((errcode = SetSampleKind (tagdata, SAMPLEKIND_ORDINARY)) < 0) return errcode;
            if ((int32)arg) asmp->asmp_Item.n_Flags |= AF_NODE_F_AUTO_FREE_DATA;
            else            asmp->asmp_Item.n_Flags &= ~AF_NODE_F_AUTO_FREE_DATA;
            break;

        case AF_TAG_DELAY_LINE:
            return SetDelayLineKindAndLength (asmp, tagdata, SAMPLEKIND_DELAY_LINE, (int32)arg);

        case AF_TAG_DELAY_LINE_TEMPLATE:
            return SetDelayLineKindAndLength (asmp, tagdata, SAMPLEKIND_DELAY_LINE_TEMPLATE, (int32)arg);

        default:
            return SampleTagHandler (asmp, tagdata, tag, arg);
    }

    return 0;
}


/*
    Common tag processing for CreateSampleTagHandler() and internalSetSampleInfo().

    Arguments
        asmp
            AudioSample Item to fill out.
*/
static Err SampleTagHandler (AudioSample *asmp, SampleTagData *tagdata, uint32 tag, TagData arg)
{
    Err errcode;

    switch (tag) {
    /* Data */
        case AF_TAG_ADDRESS:
                /* set sample kind */
            if ((errcode = SetSampleKind (tagdata, SAMPLEKIND_ORDINARY)) < 0) return errcode;

                /* memory is validated by CompleteAndValidateAudioSample() */
            asmp->asmp_Data = (void *)arg;
            tagdata->std_AddressChanged = TRUE;
            break;

        case AF_TAG_FRAMES:
                /* set sample kind */
            if ((errcode = SetSampleKind (tagdata, SAMPLEKIND_ORDINARY)) < 0) return errcode;

            {
                const int32 numFrames = (int32)arg;

                if (numFrames < 0) {
                    ERR(("SampleTagHandler: NumFrames (%d) out of range\n", numFrames));
                    return AF_ERR_OUTOFRANGE;
                }

                asmp->asmp_NumFrames = numFrames;
                tagdata->std_NumFramesChanged = TRUE;
            }
            break;

        case AF_TAG_NUMBYTES:
                /* set sample kind */
            if ((errcode = SetSampleKind (tagdata, SAMPLEKIND_ORDINARY)) < 0) return errcode;

                /* can rely on ValidateAudioItemData() to catch negative value */
            asmp->asmp_NumBytes = (int32)arg;
            tagdata->std_NumBytesChanged = TRUE;
            break;

        case AF_TAG_SUPPRESS_WRITE_BACK_D_CACHE:
            tagdata->std_SuppressWriteBackDCache = (int32)arg ? TRUE : FALSE;
            break;

    /* Format */
        case AF_TAG_COMPRESSIONTYPE:
            asmp->asmp_CompressionType = (PackedID)arg;
            break;

        case AF_TAG_COMPRESSIONRATIO:
            {
                const int32 compressionRatio = (int32)arg;

                if (compressionRatio < 1 || compressionRatio > 255) {
                    ERR(("SampleTagHandler: Compression ratio (%d) out of range\n", compressionRatio));
                    return AF_ERR_OUTOFRANGE;
                }
                asmp->asmp_CompressionRatio = compressionRatio;
                tagdata->std_FrameSizeChanged = TRUE;
            }
            break;

        case AF_TAG_WIDTH:          /* bytes per sample UNCOMPRESSED */
            {
                const int32 width = (int32)arg;

                if (width < 1 || width > 2) {       /* @@@ keep in sync with trap in AF_TAG_NUMBITS */
                    ERR(("SampleTagHandler: Sample width (%d bytes) out of range\n", width));
                    return AF_ERR_OUTOFRANGE;
                }
                asmp->asmp_Width = (uint8)width;
                asmp->asmp_Bits  = (uint8)(width * 8);
                tagdata->std_FrameSizeChanged = TRUE;
            }
            break;

        case AF_TAG_NUMBITS:        /* bits per sample UNCOMPRESSED */
            {
                const int32 numBits = (int32)arg;

                if (numBits < 1 || numBits > 16) {  /* @@@ keep in sync with trap in AF_TAG_WIDTH */
                    ERR(("SampleTagHandler: Sample width (%d bits) out of range\n", numBits));
                    return AF_ERR_OUTOFRANGE;
                }
                asmp->asmp_Bits  = (uint8)numBits;
                asmp->asmp_Width = (uint8)(((uint32)numBits + 7) / 8);      /* extra (uint32) causes unsigned division */
                tagdata->std_FrameSizeChanged = TRUE;
            }
            break;

        case AF_TAG_CHANNELS:       /* samples per frame */
            {
                const int32 numChannels = (int32)arg;

                if (numChannels < 1 || numChannels > 255) {
                    ERR(("SampleTagHandler: NumChannels (%d) out of range\n", numChannels));
                    return AF_ERR_OUTOFRANGE;
                }
                asmp->asmp_Channels = numChannels;
                tagdata->std_FrameSizeChanged = TRUE;
            }
            break;

    /* Loops */
        case AF_TAG_SUSTAINBEGIN:
            return SetSampleLoopParameter (&asmp->asmp_SustainBegin, (int32)arg);

        case AF_TAG_SUSTAINEND:
            return SetSampleLoopParameter (&asmp->asmp_SustainEnd, (int32)arg);

        case AF_TAG_RELEASEBEGIN:
            return SetSampleLoopParameter (&asmp->asmp_ReleaseBegin, (int32)arg);

        case AF_TAG_RELEASEEND:
            return SetSampleLoopParameter (&asmp->asmp_ReleaseEnd, (int32)arg);

    /* Tuning */
        case AF_TAG_BASENOTE:   /* MIDI note when played at 44.1 Khz */
            tagdata->std_TuningChanged = TRUE;
            return SetSampleMIDIParameter (&asmp->asmp_BaseNote, (int32)arg);

        case AF_TAG_DETUNE:
            {
                const int32 detune = (int32)arg;

              #ifdef BUILD_PARANOIA     /* this is caught in production by the call to Convert12TET_FP() to update asmp_BaseFreq */
                if (detune < -100 || detune > 100) {
                    ERR(("SampleTagHandler: Detune (%d) out of range\n", detune));
                    return AF_ERR_OUTOFRANGE;
                }
              #endif
                asmp->asmp_Detune = (int8)detune;
                tagdata->std_TuningChanged = TRUE;
            }
            break;

        case AF_TAG_SAMPLE_RATE_FP:
            {
                const float32 sampleRate = ConvertTagData_FP(arg);

                    /* Bounds check sample rate. Out of bounds value causes
                    ** various FP exceptions (e.g., div by 0, inexact when converting
                    ** to 1.15 fixed point) */
                    /* !!! these limits are guesses and may not prevent all FP exceptions */
                if (sampleRate < 10.0 || sampleRate > 1.0E6) {
                    ERR(("SampleTagHandler: Sample rate (%g samp/s) out of range\n", sampleRate));
                    return AF_ERR_OUTOFRANGE;
                }
                asmp->asmp_SampleRate = sampleRate;
                tagdata->std_TuningChanged = TRUE;
            }
            break;

    /* Multisample */
        case AF_TAG_LOWNOTE:            /* lowest note to use when multisampling */
            return SetSampleMIDIParameter (&asmp->asmp_LowNote, (int32)arg);

        case AF_TAG_HIGHNOTE:           /* highest note to use when multisampling */
            return SetSampleMIDIParameter (&asmp->asmp_HighNote, (int32)arg);

        case AF_TAG_LOWVELOCITY:        /* lowest velocity to use when multisampling */
            return SetSampleMIDIParameter (&asmp->asmp_LowVelocity, (int32)arg);

        case AF_TAG_HIGHVELOCITY:       /* highest velocity to use when multisampling */
            return SetSampleMIDIParameter (&asmp->asmp_HighVelocity, (int32)arg);

    /* Misc */
        case AF_TAG_SET_FLAGS:
            {
                const uint32 setFlags = (uint32)arg;

              #ifdef BUILD_PARANOIA
                if (setFlags & ~AF_SAMPF_LEGALFLAGS) {
                    ERR(("SampleTagHandler: Illegal sample flags (0x%x)\n", setFlags));
                    return AF_ERR_BADTAGVAL;
                }
              #endif

                asmp->asmp_Flags |= setFlags;
            }
            break;

        case AF_TAG_CLEAR_FLAGS:
            {
                const uint32 clearFlags = (uint32)arg;

              #ifdef BUILD_PARANOIA
                if (clearFlags & ~AF_SAMPF_LEGALFLAGS) {
                    ERR(("SampleTagHandler: Illegal sample flags (0x%x)\n", clearFlags));
                    return AF_ERR_BADTAGVAL;
                }
              #endif

                asmp->asmp_Flags &= ~clearFlags;
            }
            break;

        default:
            ERR(("SampleTagHandler: Unrecognized tag { %d, 0x%x }\n", tag, arg));
            return AF_ERR_BADTAG;
    }

    return 0;
}

/*
    Sets sample to delay line or delay line template kind and sets length (lots
    of common code).

    Arguments
        asmp, tagdata
            Sample and SampleTagData being set.

        sampleKind
            SAMPLEKIND_DELAY_LINE or SAMPLEKIND_DELAY_LINE_TEMPLATE

        numBytes
            Size of the delay line in bytes.
*/
static Err SetDelayLineKindAndLength (AudioSample *asmp, SampleTagData *tagdata, uint8 sampleKind, int32 numBytes)
{
    Err errcode;

        /* set sample kind */
    if ((errcode = SetSampleKind (tagdata, sampleKind)) < 0) return errcode;

        /* mark as delay line */
    asmp->asmp_Item.n_ItemFlags |= ITEMNODE_PRIVILEGED;

        /* set asmp_NumBytes */
  #ifdef BUILD_PARANOIA
        /* trap <= 0 size in paranoid mode to avoid having it generate confusing memdebug output */
    if (numBytes <= 0) {
        ERR(("CreateDelayLineTagHandler: Delay line size (%d) out of range\n", numBytes));
        return AF_ERR_BADTAGVAL;
    }
  #endif
    asmp->asmp_NumBytes = numBytes;
    tagdata->std_NumBytesChanged = TRUE;

    return 0;
}

/*
    Sets kind of sample and detects conflicts with previous setting. Returns an
    error if the old kind is not SAMPLEKIND_DEFAULT and is different from the
    new kind.

    Arguments
        tagdata
            SampleTagData for sample being set.

        sampleKind
            New sample kind to set sample to.

    Results
        Non-negative on success; Err code on failure.
*/
static Err SetSampleKind (SampleTagData *tagdata, uint8 sampleKind)
{
        /* check for conflict */
    if (tagdata->std_SampleKind != SAMPLEKIND_DEFAULT && tagdata->std_SampleKind != sampleKind) {
      #ifdef BUILD_STRINGS
        ERR(("SetSampleKind: sample creation tags are of conflicting kinds: %s and %s.\n",
            sampleKindDesc[tagdata->std_SampleKind],
            sampleKindDesc[sampleKind]));
      #endif
        return AF_ERR_TAGCONFLICT;
    }

        /* update sample kind */
    tagdata->std_SampleKind = sampleKind;
    return 0;
}

/*
    Check and install a 7-bit MIDI value (note number or velocity). Target is a
    uint8, source must be in the range of 0..127
*/
static Err SetSampleMIDIParameter (uint8 *resultValue, int32 val)
{
  #ifdef BUILD_PARANOIA
    if (val < 0 || val > 127) {
        ERR(("SampleTagHandler: MIDI value (%d) out of range\n", val));
        return AF_ERR_OUTOFRANGE;
    }
  #endif

    *resultValue = (uint8)val;
    return 0;
}

/*
    Check and install a loop parameter. Target is a int32.
*/
static Err SetSampleLoopParameter (int32 *resultValue, int32 val)
{
    if (val < -1) {
        ERR(("SampleTagHandler: Loop value (%d) out of range\n", val));
        return AF_ERR_OUTOFRANGE;
    }

    *resultValue = val;
    return 0;
}

/*
    Complete changes to AudioSample and validate results.

    Updates:
        . asmp_NumBytes if std_NumFramesChanged
        . asmp_NumFrames if std_NumBytesChanged or std_FrameSizeChanged
        . asmp_BaseFreq if std_TuningChanged

    Validates:
        . data legality w/ and w/o AF_NODE_F_AUTO_FREE_DATA
        . loop points
        . frame and byte alignments of length and loops (if BUILD_PARANOIA)
        . multisample stuff (if BUILD_PARANOIA)

    Caveats
        std_SampleKind isn't necessarily valid here.
*/
static Err CompleteAndValidateAudioSample (AudioSample *asmp, const SampleTagData *tagdata)
{
  #ifdef BUILD_PARANOIA
        /*
            alignment values to check size and loop alignments: the greater of 1 AudioGrain
            or 1 sample frame

            @@@ This makes the assumption about sample frame size relationship to dma
            granularity:

            If frame size < DMA granularity, then DMA granularity must be a multiple of
            frame size.

            If frame size > DMA granularity, then frame size must be a multiple of
            DMA granularity.

            For all currently used sample frame configurations, this is true. If
            it should become necessary to fix this, compute a least common multiple
            between DMA granularity and frame size for AlignmentBytes (in bits if it
            helps) and then compute AlignmentBytes and AlignmentFrames from this.
        */
    const uint32 alignmentFrames = MAX (CvtByteToFrame (sizeof(AudioGrain), asmp), 1);  /* in frames */
    const uint32 alignmentBytes  = CvtFrameToByte (alignmentFrames, asmp);              /* in bytes */
  #endif
    Err errcode;

  #if DEBUG_Item
    PRT(("CompleteAndValidateAudioSample: changed"));
    if (tagdata->std_AddressChanged) PRT((" Address"));
    if (tagdata->std_NumFramesChanged) PRT((" NumFrames"));
    if (tagdata->std_NumBytesChanged) PRT((" NumBytes"));
    if (tagdata->std_FrameSizeChanged) PRT((" FrameSize"));
    if (tagdata->std_TuningChanged) PRT((" Tuning"));
    PRT(("\n"));
    PRT(("  format: channels=%d width=%d compression=%d\n", asmp->asmp_Channels, asmp->asmp_Width, asmp->asmp_CompressionRatio));
   #ifdef BUILD_PARANOIA
    PRT(("   align: %u frames, %u bytes\n", alignmentFrames, alignmentBytes));
   #endif
  #endif

    /* update/validate data */

        /* update sample length based on requested changes to length and format size */
    if (tagdata->std_NumFramesChanged && tagdata->std_NumBytesChanged) {

            /* length specified 2 ways, make sure they represent the same length */
        if (asmp->asmp_NumFrames != CvtByteToFrame (asmp->asmp_NumBytes, asmp)) {
            ERR(("ValidateAudioSample: NumFrames (%d) and NumBytes (%d) represent different lengths\n", asmp->asmp_NumFrames, asmp->asmp_NumBytes));
            return AF_ERR_BADTAGVAL;
        }

    }
    else if (tagdata->std_NumFramesChanged) {

            /* num frames changed, updated num bytes */
        asmp->asmp_NumBytes = CvtFrameToByte (asmp->asmp_NumFrames, asmp);

    }
    else if (tagdata->std_NumBytesChanged || tagdata->std_FrameSizeChanged) {

            /* num bytes or frame size changed, updated num frames */
        asmp->asmp_NumFrames = CvtByteToFrame (asmp->asmp_NumBytes, asmp);

    }

  #ifdef BUILD_PARANOIA
        /* check length frame and byte alignments */
    if (asmp->asmp_NumFrames % alignmentFrames) {
        ERR(("ValidateAudioSample: NumFrames (%d) not byte-aligned. Must be multiple of %d frames\n", asmp->asmp_NumFrames, alignmentFrames));
        return AF_ERR_BADTAGVAL;
    }
    if (asmp->asmp_NumBytes % alignmentBytes) {
        ERR(("ValidateAudioSample: NumBytes (%d) not frame aligned. Must be multiple of %d bytes\n", asmp->asmp_NumBytes, alignmentBytes));
        return AF_ERR_BADTAGVAL;
    }
  #endif

        /* special processing for ordinary sample data (not necessary for delay lines) */
    if (!IsDelayLine(asmp)) {

            /* Validate sample data */
        if ((errcode = ValidateAudioItemData (asmp->asmp_Data, asmp->asmp_NumBytes,
            (asmp->asmp_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) != 0)) < 0) return errcode;

            /* Prepare sample memory for DMA (write back to physical RAM) whenever address or length has changed. */
            /* (WriteBackDCache() is a NOP when length==0) */
        if (!tagdata->std_SuppressWriteBackDCache && (tagdata->std_AddressChanged || tagdata->std_NumBytesChanged || tagdata->std_NumFramesChanged)) {
            DBUGITEM(("CompleteAndValidateAudioSample: WriteBackDCache(0,0x%x,0x%x)\n", asmp->asmp_Data, asmp->asmp_NumBytes));
            WriteBackDCache (0, asmp->asmp_Data, asmp->asmp_NumBytes);
        }
    }

    /* update/validate loops */

        /* Check sustain loop */
    if (asmp->asmp_SustainBegin != -1) {
        if (asmp->asmp_SustainEnd > asmp->asmp_NumFrames) {
            ERR(("ValidateAudioSample: SustainEnd (%d) <= NumFrames (%d)\n",
                asmp->asmp_SustainEnd, asmp->asmp_NumFrames));
            return AF_ERR_BADTAGVAL;
        }
        if (asmp->asmp_SustainBegin >= asmp->asmp_SustainEnd) {
            ERR(("ValidateAudioSample: SustainBegin (%d) must be < SustainEnd (%d)\n",
                asmp->asmp_SustainBegin, asmp->asmp_SustainEnd));
            return AF_ERR_BADTAGVAL;
        }
      #ifdef BUILD_PARANOIA
        if (asmp->asmp_SustainBegin % alignmentFrames || asmp->asmp_SustainEnd % alignmentFrames) {
            ERR(("ValidateAudioSample: Sustain loop (%d,%d) not byte-aligned. Must be multiple of %d frames\n", asmp->asmp_SustainBegin, asmp->asmp_SustainEnd, alignmentFrames));
            return AF_ERR_BADTAGVAL;
        }
      #endif
    }

        /* Check release loop */
    if (asmp->asmp_ReleaseBegin != -1) {
        if (asmp->asmp_ReleaseEnd > asmp->asmp_NumFrames) {
            ERR(("ValidateAudioSample: ReleaseEnd (%d) <= NumFrames (%d)\n",
                asmp->asmp_ReleaseEnd, asmp->asmp_NumFrames));
            return AF_ERR_BADTAGVAL;
        }
        if (asmp->asmp_ReleaseBegin >= asmp->asmp_ReleaseEnd) {
            ERR(("ValidateAudioSample: ReleaseBegin (%d) must be < ReleaseEnd (%d)\n",
                asmp->asmp_ReleaseBegin, asmp->asmp_ReleaseEnd));
            return AF_ERR_BADTAGVAL;
        }
      #ifdef BUILD_PARANOIA
        if (asmp->asmp_ReleaseBegin % alignmentFrames || asmp->asmp_ReleaseEnd % alignmentFrames) {
            ERR(("ValidateAudioSample: Release loop (%d,%d) not byte-aligned. Must be multiple of %d frames\n", asmp->asmp_ReleaseBegin, asmp->asmp_ReleaseEnd, alignmentFrames));
            return AF_ERR_BADTAGVAL;
        }
      #endif
    }

    /* update/validate tuning */

    if (tagdata->std_TuningChanged) {
        if ((errcode = CalcSampleBaseFreq (asmp)) < 0) {
            ERR(("ValidateAudioSample: Base frequency calculation failed. SampleRate=%g Detune=%d\n", asmp->asmp_SampleRate, asmp->asmp_Detune));
            return errcode;
        }
    }

    /* update/validate multisample */

  #ifdef BUILD_PARANOIA
    if (asmp->asmp_LowNote > asmp->asmp_HighNote) {
        ERR(("ValidateAudioSample: LowNote (%d) must be <= HighNote (%d)\n", asmp->asmp_LowNote, asmp->asmp_HighNote));
        return AF_ERR_BADTAGVAL;
    }
    if (asmp->asmp_LowVelocity > asmp->asmp_HighVelocity) {
        ERR(("ValidateAudioSample: LowVelocity (%d) must be <= HighVelocity (%d)\n", asmp->asmp_LowVelocity, asmp->asmp_HighVelocity));
        return AF_ERR_BADTAGVAL;
    }
  #endif

    /* validate misc */

        /* check more restrictive set of flags for ordinary samples */
    if (!IsDelayLine(asmp) && (asmp->asmp_Flags & ~AF_SAMPF_ORDINARY_LEGALFLAGS)) {
        ERR(("ValidateAudioSample: Illegal sample flags (0x%02x) for ordinary sample\n", asmp->asmp_Flags & ~AF_SAMPF_ORDINARY_LEGALFLAGS));
        return AF_ERR_BADTAGVAL;
    }

    return 0;
}


/**************************************************************/
/* AUDIO_SAMPLE_NODE ir_Delete method */
int32 internalDeleteAudioSample (AudioSample *asmp, Task *ct)
{
    DBUGITEM(("internalDeleteAudioSample(0x%x)\n", asmp->asmp_Item.n_Item));

        /* Stop all attachments to avoid noise or DMA write to free memory. */
    {
        AudioReferenceNode *arnd;

        SCANLIST (&asmp->asmp_AttachmentRefs, arnd, AudioReferenceNode) {
            swiStopAttachment (arnd->arnd_RefItem, NULL);
        }
    }

        /* delete attachments to this item */
    afi_DeleteReferencedItems (&asmp->asmp_AttachmentRefs);

        /* Remove from list of samples in AudioFolio structure. */
    ParanoidRemNode ((Node *)asmp);

        /* delete delay line memory */
    if (IsDelayLine(asmp)) {
        DBUGITEM(("internalDeleteAudioSample: Deleting delay line 0x%08x, 0x%x bytes\n", asmp->asmp_Data, GetMemTrackSize(asmp->asmp_Data)));
        SuperFreeMem (asmp->asmp_Data, TRACKED_SIZE);
    }
        /* delete AF_NODE_F_AUTO_FREE_DATA data */
    else if (asmp->asmp_Item.n_Flags & AF_NODE_F_AUTO_FREE_DATA) {
        DBUGITEM(("internalDeleteAudioSample: Deleting AF_NODE_F_AUTO_FREE_DATA @ 0x%08x, 0x%x bytes\n", asmp->asmp_Data, GetMemTrackSize(asmp->asmp_Data)));
        SuperFreeUserMem (asmp->asmp_Data, TRACKED_SIZE, ct);
    }

    return 0;
}


/*****************************************************************/
/* AUDIO_SAMPLE_NODE GetAudioItemInfo() method */
Err internalGetSampleInfo (const AudioSample *asmp, TagArg *tagList)
{
    TagArg *tag;
    int32 tagResult;

    for (tagResult = SafeFirstTagArg (&tag, tagList); tagResult > 0; tagResult = SafeNextTagArg (&tag)) {
        DBUGITEM(("internalGetSampleInfo: %d\n", tag->ta_Tag));

        switch (tag->ta_Tag) {
        /* Data */
            case AF_TAG_ADDRESS:
                tag->ta_Arg = (TagData)asmp->asmp_Data;
                break;

            case AF_TAG_FRAMES:
                tag->ta_Arg = (TagData)asmp->asmp_NumFrames;
                break;

            case AF_TAG_NUMBYTES:
                tag->ta_Arg = (TagData)asmp->asmp_NumBytes;
                break;

        /* Format */
            case AF_TAG_COMPRESSIONTYPE:
                tag->ta_Arg = (TagData)asmp->asmp_CompressionType;
                break;

            case AF_TAG_COMPRESSIONRATIO:
                tag->ta_Arg = (TagData)asmp->asmp_CompressionRatio;
                break;

            case AF_TAG_WIDTH:          /* bytes per sample */
                tag->ta_Arg = (TagData)asmp->asmp_Width;
                break;

            case AF_TAG_NUMBITS:
                tag->ta_Arg = (TagData)asmp->asmp_Bits;
                break;

            case AF_TAG_CHANNELS:       /* samples per frame */
                tag->ta_Arg = (TagData)asmp->asmp_Channels;
                break;

        /* Loops */
            case AF_TAG_SUSTAINBEGIN:
                tag->ta_Arg = (TagData)asmp->asmp_SustainBegin;
                break;

            case AF_TAG_SUSTAINEND:
                tag->ta_Arg = (TagData)asmp->asmp_SustainEnd;
                break;

            case AF_TAG_RELEASEBEGIN:
                tag->ta_Arg = (TagData)asmp->asmp_ReleaseBegin;
                break;

            case AF_TAG_RELEASEEND:
                tag->ta_Arg = (TagData)asmp->asmp_ReleaseEnd;
                break;


        /* Tuning */
            case AF_TAG_BASENOTE:       /* MIDI note when played at 44.1 Khz */
                tag->ta_Arg = (TagData)asmp->asmp_BaseNote;
                break;

            case AF_TAG_DETUNE:
                tag->ta_Arg = (TagData)asmp->asmp_Detune;
                break;

            case AF_TAG_SAMPLE_RATE_FP:
                tag->ta_Arg = ConvertFP_TagData (asmp->asmp_SampleRate);
                break;

            case AF_TAG_BASEFREQ_FP:    /* base frequency derived from sample rate, note, and detune */
                tag->ta_Arg = ConvertFP_TagData (asmp->asmp_BaseFreq);
                break;

        /* Multisample */
            case AF_TAG_LOWNOTE:        /* lowest note to use when multisampling */
                tag->ta_Arg = (TagData)asmp->asmp_LowNote;
                break;

            case AF_TAG_HIGHNOTE:       /* highest note to use when multisampling */
                tag->ta_Arg = (TagData)asmp->asmp_HighNote;
                break;

            case AF_TAG_LOWVELOCITY:
                tag->ta_Arg = (TagData)asmp->asmp_LowVelocity;
                break;

            case AF_TAG_HIGHVELOCITY:
                tag->ta_Arg = (TagData)asmp->asmp_HighVelocity;
                break;

        /* Misc */
            case AF_TAG_SET_FLAGS:
                    /* filter off AF_SAMPF_LEGALFLAGS in case there are internal flags stored here too. */
                tag->ta_Arg = (TagData)(asmp->asmp_Flags & AF_SAMPF_LEGALFLAGS);
                break;

            default:
                ERR (("internalGetSampleInfo: Unrecognized tag (%d)\n", tag->ta_Tag));
                return AF_ERR_BADTAG;
        }
    }

        /* Catch tag processing errors */
    if (tagResult < 0) {
        ERR(("internalGetSampleInfo: Error processing tag list 0x%x\n", tagList));
        return tagResult;
    }

    return 0;
}


/* -------------------- Item Debug */

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Sample -name DebugSample
 |||	Prints Sample(@) information for debugging.
 |||
 |||	  Synopsis
 |||
 |||	    Err DebugSample (Item sample)
 |||
 |||	  Description
 |||
 |||	    This procedure dumps all available sample information to the 3DO Debugger
 |||	    screen for your delight and edification. This function is merely a stub in
 |||	    the production version of the audio folio.
 |||
 |||	  Arguments
 |||
 |||	    sample
 |||	        Item number of a sample.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>, libc.a
 |||
 |||	  See Also
 |||
 |||	    Sample(@), CreateSample()
 **/
/* !!! move to libc.a instead? */
Err DebugSample (Item sample)
{
#ifdef BUILD_STRINGS
    const AudioSample *const asmp = (AudioSample *)CheckItem (sample, AUDIONODE, AUDIO_SAMPLE_NODE);
    if (asmp == NULL) return AF_ERR_BADITEM;
    internalDumpSample (asmp, NULL);
#else
    TOUCH (sample);
#endif
    return 0;
}

#ifdef BUILD_STRINGS    /* { */

#define REPORT_SAMPLE(format,member)    printf("  %-21s "format"\n", #member, asmp->member)
#define REPORT_SAMPLE_ID(member)        printf("  %-21s '%.4s'\n", #member, &asmp->member)

/* !!! move to audio_debug.h? */
void internalDumpSample (const AudioSample *asmp, const char *banner)
{
    PRT(("--------------------------\n"));
    if (banner) PRT(("%s: ", banner));
    PRT(("Sample 0x%x ('%s') @ 0x%08x\n", asmp->asmp_Item.n_Item, asmp->asmp_Item.n_Name, asmp));

        /* Data */
    REPORT_SAMPLE("0x%02x",asmp_Item.n_Flags);
    REPORT_SAMPLE("0x%02x",asmp_Item.n_ItemFlags);
    REPORT_SAMPLE("0x%08x",asmp_Data);
    REPORT_SAMPLE("%d",asmp_NumFrames);
    REPORT_SAMPLE("%d",asmp_NumBytes);

        /* Format */
    REPORT_SAMPLE("%d",asmp_Channels);
    REPORT_SAMPLE("%d bytes",asmp_Width);
    REPORT_SAMPLE("%d bits",asmp_Bits);
    REPORT_SAMPLE_ID(asmp_CompressionType);
    REPORT_SAMPLE("%d",asmp_CompressionRatio);

        /* Loops */
    REPORT_SAMPLE("%d",asmp_SustainBegin);
    REPORT_SAMPLE("%d",asmp_SustainEnd);
    REPORT_SAMPLE("%d",asmp_ReleaseBegin);
    REPORT_SAMPLE("%d",asmp_ReleaseEnd);

        /* Tuning */
    REPORT_SAMPLE("%d",asmp_BaseNote);
    REPORT_SAMPLE("%d",asmp_Detune);
    REPORT_SAMPLE("%g samp/s",asmp_SampleRate);
    REPORT_SAMPLE("%g Hz",asmp_BaseFreq);

        /* Multisample */
    REPORT_SAMPLE("%d",asmp_LowNote);
    REPORT_SAMPLE("%d",asmp_HighNote);
    REPORT_SAMPLE("%d",asmp_LowVelocity);
    REPORT_SAMPLE("%d",asmp_HighVelocity);

        /* Misc */
    REPORT_SAMPLE("0x%02x",asmp_Flags);
}
#endif  /* } defined(BUILD_STRINGS) */


/* -------------------- Clear delay line */

static bool IsSampleStopped (const AudioSample *);

/*
    Clear delay line memory of delay line with AF_SAMPF_CLEARONSTOP set, if all
    the attachments to that delay line have aatt_ActivityLevel <= AF_STOPPED.

    (supervisor mode function)

    Arguments
        asmp
            Delay line to consider. Nothing happens unless delay line has
            AF_SAMPF_CLEARONSTOP set.

    Notes
        @@@ Does not check the kind of sample.
            - Ordinary samples - OK
                Because AF_SAMPF_CLEARONSTOP cannot be set for ordinary
                samples, they are treated just like delay lines without
                AF_SAMPF_CLEARONSTOP: nothing happens.

            - Delay line template - not allowed
                Caller is assumed to be handling a StopAttachment() event,
                which can only happen for Instrument Attachments. Since Delay
                Line Templates cannot be attached to Instruments, this should
                never happen.
*/
void ClearDelayLineIfStopped (AudioSample *asmp)
{
    DBUGCLR(("ClearDelayLineIfStopped: asmp=0x%x asmp_Flags=0x%02x %s\n", asmp->asmp_Item.n_Item, asmp->asmp_Flags, IsSampleStopped(asmp) ? "stopped" : "running"));

    if (asmp->asmp_Flags & AF_SAMPF_CLEARONSTOP && IsSampleStopped(asmp)) {
        ClearDelayLine (asmp);
    }
}

/*
    Determine whether all attachments to sample have stopped.

    Arguments
        asmp
            Sample to check. Assumed to be valid. Doesn't matter what kind of
            sample it is, or whether attachments are to Instrument or Template.

    Results
        TRUE if all attachments to sample have aatt_ActivityLevel <=
        AF_STOPPED, FALSE otherwise.
*/
static bool IsSampleStopped (const AudioSample *asmp)
{
    const AudioReferenceNode *arnd;

    ScanList (&asmp->asmp_AttachmentRefs, arnd, AudioReferenceNode) {
        const AudioAttachment * const aatt = SlaveRefToAttachment(arnd);

            /* @@@ Attachment can be a template attachment, in which case
            ** ActivityLevel is the initial value AF_ABANDONED. This is OK. */
        if (aatt->aatt_ActivityLevel > AF_STOPPED) return FALSE;
    }

    return TRUE;
}

/*
    Clear a delay line.

    Arguments
        asmp
            Assumed to be a valid pointer to a Delay Line (not a Delay Line
            Template or ordinary sample). Will trash memory otherwise.
*/
static void ClearDelayLine (AudioSample *asmp)
{
  #if DEBUG_ClearDelayLine
    TimeVal base, clear, flush;
  #endif

  #if DEBUG_ClearDelayLine
    DBUGCLR(("ClearDelayLine: asmp=0x%x data=0x%x,%d bytes - ", asmp->asmp_Item.n_Item, asmp->asmp_Data, asmp->asmp_NumBytes));
    SampleSystemTimeTV (&base);
  #endif

        /* clear delay line memory */
    memset (asmp->asmp_Data, 0, asmp->asmp_NumBytes);

  #if DEBUG_ClearDelayLine
    SampleSystemTimeTV (&clear);
  #endif

        /* writeback cache for DMA */
    WriteBackDCache (0, asmp->asmp_Data, asmp->asmp_NumBytes);

  #if DEBUG_ClearDelayLine
    SampleSystemTimeTV (&flush);
    SubTimes (&clear, &flush, &flush);
    SubTimes (&base, &clear, &clear);
    DBUGCLR(("clear %d.%06d sec, flush %d.%06d sec\n", clear.tv_sec, clear.tv_usec, flush.tv_sec, flush.tv_usec));
  #endif
}


/* -------------------- Sample goodies */

/**************************************************************
** Frame to byte conversions
**************************************************************/
uint32 CvtFrameToByte (uint32 frameNum, const AudioSample *asmp)
{
  #if 0     /* !!! optimizations commented out for now */
    const uint32 tmp = frameNum * asmp->asmp_Channels * asmp->asmp_Width;

    switch (asmp->asmp_CompressionRatio) {
        case 1:     return tmp;
        case 2:     return tmp / 2;
        default:    return tmp / asmp->asmp_CompressionRatio;
    }
  #endif

    return frameNum * asmp->asmp_Channels * asmp->asmp_Width / asmp->asmp_CompressionRatio;
}

uint32 CvtByteToFrame (uint32 byteNum, const AudioSample *asmp)
{
  #if 0     /* !!! optimizations commented out for now */
    switch (asmp->asmp_CompressionRatio) {
        case 1:     return byteNum / (asmp->asmp_Channels * asmp->asmp_Width);
        case 2:     return byteNum * 2 / (asmp->asmp_Channels * asmp->asmp_Width);
        default:    return byteNum * asmp->asmp_CompressionRatio / (asmp->asmp_Channels * asmp->asmp_Width);
    }
  #endif

    return byteNum * asmp->asmp_CompressionRatio / (asmp->asmp_Channels * asmp->asmp_Width);
}


/**************************************************************
** Update sample base frequencies. 940506
**************************************************************/
Err UpdateAllSampleBaseFreqs( void )
{
	Node *n;
	Err Result;

	n = FirstNode( &AB_FIELD(af_SampleList) );
	while (ISNODE( &AB_FIELD(af_SampleList), n))
	{
		Result = CalcSampleBaseFreq( (AudioSample *) n );
		if( Result < 0 ) return Result;
		n = NextNode( n );
	}

	return 0;
}

/*****************************************************************/
/*
    Compute new value for asmp_BaseFreq based on asmp_BaseNote,
    asmp_Detune, and asmp_SampleRate.
*/
static Err CalcSampleBaseFreq (AudioSample *asmp)
{
	Err Result;
	float32 PitchFreq, DetuneFrac;

/* Calculate samples Base Frequency. */
	Result = PitchToFrequency( &DefaultTuning, asmp->asmp_BaseNote,
		&PitchFreq );
	if(Result < 0) return Result;
	asmp->asmp_BaseFreq = PitchFreq * ( AF_SAMPLE_RATE / asmp->asmp_SampleRate );

	if( asmp->asmp_Detune != 0 )
	{
		Result = Convert12TET_FP( 0, -(asmp->asmp_Detune), &DetuneFrac );
		if(Result < 0) return Result;
		asmp->asmp_BaseFreq = asmp->asmp_BaseFreq * DetuneFrac;
	}
TRACEB(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE,
	("asmp->asmp_BaseNote = %g, asmp->asmp_BaseFreq = %g\n",
	asmp->asmp_BaseNote, asmp->asmp_BaseFreq));

	return 0;
}

/**************************************************************/
#if 0   /* !!! unused: remove? */
float32 CalcSampleNoteRate ( AudioInstrument *ains, AudioSample *asmp, int32 Note)
{
	int32 Result;
	float32 Fraction, Freq;
	DSPPInstrument *dins;

TRACEE(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE, ("CalcSampleNoteRate(ains=0x%lx, asmp=0x%lx, note=%d)\n",
			ains, asmp, Note));

	Result = PitchToFrequency( GetInsTuning(ains), Note, &Freq);
	if( Result) return 0x8000;

/* Shift Frequency by Shift Rate to compensate for execution rate. 940817 */
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Freq = Freq * ((float32) (1<< dins->dins_RateShift));

	Fraction = Freq / asmp->asmp_BaseFreq ;

TRACEB(TRACE_INT, TRACE_TUNING|TRACE_SAMPLE,
		("CalcSampleNoteRate: Fraction=%g, Freq=%g, BaseFreq=%g\n",
		Fraction, Freq, asmp->asmp_BaseFreq));

	return (uint32) Fraction;
}
#endif
