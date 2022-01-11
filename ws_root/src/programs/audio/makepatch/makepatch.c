/******************************************************************************
**
**  @(#) makepatch.c 96/07/01 1.49
**
******************************************************************************/

/**
|||	AUTODOC -public -class Shell_Commands -group Audio -name makepatch
|||	Reads patch script language, writes binary patch file.
|||
|||	  Format
|||
|||	    makepatch [-Check|-NoCheck] <source script> <output file>
|||
|||	  Description
|||
|||	    Reads a patch script language to construct a binary patch file of the same
|||	    format as written by ARIA. Patches created in this way can be loaded with
|||	    LoadPatchTemplate(), added to a PIMap(@), etc.
|||
|||	    You can test your newly created patch with dspfaders(@).
|||
|||	  Arguments
|||
|||	    <source script>
|||	        File name of patch script to read.
|||
|||	    <output file>
|||	        File name of output patch file to write. If an error occurs during
|||	        parsing or writing of the file, the output file is deleted. Append
|||	        the suffix .patch to your output patch name if you want
|||	        LoadScoreTemplate() to be able to load it (e.g., foo.patch).
|||
|||	    -Check
|||	        Loads the output patch file to test its validity. Note that the patch
|||	        file is not deleted when an error is detected while loading. This option
|||	        is on by default.
|||
|||	    -NoCheck
|||	        Prevents loading the output patch file to test its validity.
|||
|||	  Patch Script Language
|||
|||	    The patch script language is line-oriented (i.e., one command per line; one
|||	    line per command). Each line consists of white space separated words, of
|||	    which the first word is the command. Leading white space is ignored. All
|||	    commands, keywords, and switches are matched case-insensitively. All text
|||	    following a semicolon (;) or pound sign (#) to the end of the line is
|||	    ignored. Blank lines are ignored.
|||
|||	    With the noted exceptions, commands may appear in any order.
|||
|||	    Many commands accept numerical arguments. Integer arguments may be specified
|||	    in decimal, hexadecimal, or octal, following the C language conventions for
|||	    integer constants (e.g., 35, 0x23, and 043 all represent the same value in
|||	    decimal, hexadecimal, and octal).
|||
|||	    Floating point arguments may contain a decimal point and exponent (e.g.,
|||	    44100, 44100.0, and 44.1E3 all represent the same value).
|||
|||	    Instrument Blocks:
|||
|||	    The following commands each add an instrument block with the specified
|||	    block name to the patch. Block names are matched case-insensitively, and
|||	    must be unique. Multiple blocks from the same template may be added to the
|||	    patch. Each of these commands results in a PATCH_CMD_ADD_TEMPLATE(@).
|||
|||	    Instrument <block name> <template file name>
|||	        Adds an instrument template block with the name <block name> to the
|||	        patch by loading the named standard DSP Instrument Template (e.g.,
|||	        sawtooth.dsp(@)).
|||
|||	    Mixer <block name> <num inputs> <num outputs> [-LineOut] [-Amplitude]
|||	        Adds an instrument template block with the name <block name> to the
|||	        patch by creating a Mixer of the specified configuration. -LineOut
|||	        and -Amplitude correspond to the Mixer flags AF_F_MIXER_WITH_LINE_OUT
|||	        and AF_F_MIXER_WITH_AMPLITUDE, respectively. See CreateMixerTemplate()
|||	        for more detail.
|||
|||	    Ports:
|||
|||	    The commands in this group create patch ports (e.g., knobs, inputs, and
|||	    outputs) or define internal connections between blocks and ports.
|||
|||	    Connect <from block name> <from port name> <from part num> <to block name> <to port name> <to part num>
|||	        Creates an internal connection between a pair of constituent template
|||	        (block) ports, patch inputs, patch outputs, or patch knobs. Use a period
|||	        (.) for the block name when referring to one of the patch's inputs,
|||	        knobs, or outputs (see the example below). This results in a
|||	        PATCH_CMD_CONNECT(@).
|||
|||	    Constant <block name> <port name> <part num> <value>
|||	        Assigns a constant value to a constituent template (block) input or knob.
|||	        This results in a PATCH_CMD_SET_CONSTANT(@).
|||
|||	    Expose <exposed port name> <source block name> <source port name>
|||	        Exposes a FIFO, Envelope Hook, or Trigger of one of the patch's
|||	        constituent templates (blocks) to make it accessible to clients of the
|||	        patch. This results in a PATCH_CMD_EXPOSE(@).
|||
|||	    Input <port name> <num parts> <signal type>
|||	        Define an input port for the patch. This results in a
|||	        PATCH_CMD_DEFINE_PORT(@) of AF_PORT_TYPE_INPUT.
|||
|||	    Knob <knob name> <num parts> <signal type> <default>
|||	        Define a knob for the patch. This results in a PATCH_CMD_DEFINE_KNOB(@).
|||
|||	    Output <port name> <num parts> <signal type>
|||	        Define an output port for the patch. This results in a
|||	        PATCH_CMD_DEFINE_PORT(@) of AF_PORT_TYPE_OUTPUT.
|||
|||	    Attachments:
|||
|||	    The commands in this group create slave Items (e.g., samples and envelopes)
|||	    and their attachments to the patch. Each slave item is given a
|||	    case-insensitively matched, unique item name.
|||
|||	    Attach <item name> <exposed port name> [-StartAt <offset>] [-FatLadySings]
|||	        Attaches a previously defined slave item (sample, delay line, delay line
|||	        template, or envelope) identified by <item name> to the exposed FIFO or
|||	        Envelope port named <exposed port name>.
|||
|||	        -StartAt <offset> permits setting a start offset (sample frame number or
|||	        envelope segment index) for the attachment. This is particularly useful
|||	        for setting delay times for delay line attachments. This corresponds to
|||	        the Attachment(@) tag AF_TAG_START_AT.
|||
|||	        -FatLadySings indicates that the instrument should stop when this
|||	        attachment completes. This is handy for envelope attachments. This
|||	        corresponds to the Attachment(@) flag AF_ATTF_FATLADYSINGS.
|||
|||	        Note: This command must appear after the definition of the item to be
|||	        attached.
|||
|||	        Note: The Attach command can only attach to exposed ports. See the
|||	        Expose command.
|||
|||	    DelayLine <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop]
|||	        Creates a delay line. This causes all instruments created from this
|||	        patch to share a common delay line. This command isn't as useful as
|||	        DelayLineTemplate, and is retained mainly for backwards compatibility.
|||	        Its arguments are identical to those of DelayLineTemplate.
|||
|||	    DelayLineTemplate <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop] (M2 Portfolio V32)
|||	        Creates a delay line template. This causes each instrument created from
|||	        this patch to have its own delay line allocated for it.
|||
|||	        <num bytes> indicates the length of the delay line in bytes (not frames!)
|||	        for each instrument allocated from this patch, <num channels> indicates
|||	        the number of audio channels the delay line has (e.g., 1 for mono, 2 for
|||	        stereo). By default delay lines are played in a loop, that is when the
|||	        delay line writer reaches the end of the delay line memory, it starts
|||	        writing again from the beginning. This is typically the case for delay
|||	        and reverberation applications.
|||
|||	        -NoLoop causes the delay line to be written to just once each time the
|||	        instrument is started.
|||
|||	        -ClearOnStop causes the delay line memory to be cleared when the
|||	        instrument is stopped. Otherwise the delay line is cleared only when it
|||	        is first created, and it retains whatever was written to it from that
|||	        time on.
|||
|||	        See Sample(@) for a more detailed discussion of the differences between
|||	        Delay Lines and Delay Line Templates.
|||
|||	    Envelope <item name> <signal type> [-LockTimeScale] [-PitchTimeScaling <base note> <notes/octave>] <value 0> <duration 0> ... <value N-1> <duration N-1>
|||	        Creates an Envelope(@) with N EnvelopeSegments. Values are expressed in
|||	        the appropriate units for the specified signal type. Duration is
|||	        expressed in seconds.
|||
|||	        -LockTimeScale causes the AF_ENVF_LOCKTIMESCALE flag to be set for the
|||	        Envelope(@). This prevents the StartInstrument() and ReleaseInstrument()
|||	        tag AF_TAG_TIME_SCALE_FP from having any effect on this envelope.
|||
|||	        -PitchTimeScaling sets pitch-based time scaling parameters for the
|||	        envelope. <base note> is the MIDI note number at which time scaling
|||	        factor is 1.0. This defaults to 60 (middle C). <notes/octave> is the
|||	        number of semitones at which pitch time scale doubles. A positive
|||	        value makes the envelope shorter as pitch increases; a negative value
|||	        makes the envelope longer as pitch increases. Zero (the default)
|||	        disables pitch-based time scaling. This corresponds to the Envelope(@)
|||	        tags AF_TAG_BASENOTE and AF_TAG_NOTESPEROCTAVE.
|||
|||	        Envelopes are created without loop points. To set these, apply the
|||	        EnvSustainLoop, EnvReleaseLoop, and EnvReleaseJump commands after
|||	        creating the envelope.
|||
|||	    EnvReleaseJump <item name> <to segment>
|||	        Sets the release jump point for a previously defined envelope. This
|||	        is the segment in the envelope to jump to when the instrument is
|||	        released. This corresponds to the Envelope(@) tag AF_TAG_RELEASEJUMP.
|||
|||	    EnvReleaseLoop <item name> <begin segment> <end segment> <loop time>
|||	        Sets the release loop for a previously defined envelope. <begin segment>
|||	        and <end segment> are indices in the envelope's EnvelopeSegment array.
|||	        <loop time> is the duration in seconds for the segment described by
|||	        looping from <end segment> back to <begin segment>.
|||
|||	        See the documentation for the Envelope(@) tags AF_TAG_RELEASEBEGIN,
|||	        AF_TAG_RELEASEEND, and AF_TAG_RELEASETIME_FP for more detail.
|||
|||	    EnvSustainLoop <item name> <begin segment> <end segment> <loop time>
|||	        Sets the sustain loop for a previously defined envelope. <begin segment>
|||	        and <end segment> are indices in the envelope's EnvelopeSegment array.
|||	        <loop time> is the duration in seconds for the segment described by
|||	        looping from <end segment> back to <begin segment>.
|||
|||	        See the documentation for the Envelope(@) tags AF_TAG_SUSTAINBEGIN,
|||	        AF_TAG_SUSTAINEND, and AF_TAG_SUSTAINTIME_FP for more detail.
|||
|||	    Sample <item name> <sample file name> [-NoteRange <low note> <high note>] [-VelocityRange <low velocity> <high velocity>] [-Tune <base note> <detune cents>] [-SampleRate <sample rate>]
|||	        Loads an AIFF sample file to embed in patch file. The options override the
|||	        associated settings from the AIFF file:
|||
|||	        -NoteRange sets the MIDI note range in which to play the sample in a
|||	        multi-sample patch. <low note> and <high note> are MIDI note numbers in
|||	        the range of 0 to 127 (60 is middle C).
|||
|||	        -VelocityRange sets the MIDI velocity range in which to play the sample
|||	        in a multi-sample patch. <low velocity> and <high velocity> are MIDI
|||	        velocity values in the range of 0 to 127.
|||
|||	        -Tune sets the sample tuning. <base note> is the MIDI base note of the
|||	        sample when played back at its original sample rate. <detune> is the number
|||	        of cents to detune playback in order to reach the <base note> pitch.
|||
|||	        -SampleRate sets the original sample rate of the sample. <sample rate> is
|||	        specified as a floating point value in samples per second (e.g., 44100.0).
|||
|||	    Compiler Options:
|||
|||	    These commands affect the compilation process.
|||
|||	    Coherence [On|Off]
|||	        When set to On (the default state), the patch is to be built in such a
|||	        way as to guarantee signal phase coherence along all internal
|||	        connections. When set to Off, signals output from one constituent
|||	        instrument may not propagate into the destination constituent instrument
|||	        until the next audio frame. The resulting patch Template(@) is slightly
|||	        smaller when Coherence is Off.
|||
|||	        See PATCH_CMD_SET_COHERENCE(@) for more details.
|||
|||	    Signal Types:
|||
|||	    The following names are recognized for the <signal type> argument of the
|||	    commands described above. They are named similarly to the AF_SIGNAL_TYPE_
|||	    constants defined in <audio/audio.h>.
|||
|||	    Signed
|||	        Signed signal values in the range of 1.0..1.0.
|||
|||	    Unsigned
|||	        Unsigned signal values in the range of 0.0..2.0.
|||
|||	    OscFreq
|||	        Oscillator frequency values in the range of -22050.0..22050.0 Hz.
|||
|||	    LFOFreq
|||	        LFO frequency values in the range of -86.1..86.1 Hz.
|||
|||	    SampRate
|||	        Sample rate values in the range of 0.0..88100.0 Hz.
|||
|||	    Whole
|||	        Integer values in the range of -32768..32767.
|||
|||	  Implementation
|||
|||	    Command in V29.
|||
|||	  Location
|||
|||	    System.m2/Programs/makepatch
|||
|||	  Caveats
|||
|||	    The patch script parser only enforces syntax, not content. If the -NoCheck
|||	    option is specified, makepatch will happily write a bogus patch file.
|||
|||	    No support for custom tunings or nested patches yet.
|||
|||	  Examples
|||
|||	    ; sawtooth oscillator with amplitude envelope
|||
|||	    ; instruments
|||	    Instrument env envelope.dsp
|||	    Instrument osc sawtooth.dsp
|||
|||	    ; ports and knobs
|||	    Knob Frequency 1 OscFreq 261.63 ; default to middle C
|||	    Knob Amplitude 1 Signed 1.0     ; default to full amplitude
|||	    Output Output 1 Signed
|||
|||	    ; expose so we can attach envelope below
|||	    Expose AmpEnv env Env
|||
|||	    ; route patch's Frequency and Amplitude knobs
|||	    Connect . Frequency 0 osc Frequency 0
|||	    Connect . Amplitude 0 env Amplitude 0
|||
|||	    ; connect envelope output to oscillator amplitude
|||	    Connect env Output 0 osc Amplitude 0
|||
|||	    ; connect oscillator output to patch's output
|||	    Connect osc Output 0 . Output 0
|||
|||	    ; envelope
|||	    ;                       A        D    S   R
|||	    Envelope env Signed  0 1.0  1.0 0.5  0.5 0.2  0 0
|||	    EnvSustainLoop env 2 2 0.0      ; sustain at single point.
|||	    EnvReleaseJump env 2            ; jump immediately to release, even if not
|||	                                    ; at sustain yet.
|||	    Attach env AmpEnv -FatLadySings ; -FatLadySings causes instrument to stop
|||	                                    ; when envelope completes.
|||
|||	  See Also
|||
|||	    PatchCmd(@), --Patch-File-Overview--(@), LoadPatchTemplate(), dspfaders(@)
**/

/*
    @@@ Notes:

    . Returns a number of AudioPatch folio and libmusic error codes for lack of
      defining its own custom error codes (Look for PATCH_ERR_ and ML_ERR_).
      Returns MAKEERR() results for standard error codes (e.g., ER_NoMem).
*/

#include <audio/atag.h>
#include <audio/audio.h>
#include <audio/handy_macros.h>     /* PROCESSLIST(), packed strings */
#include <audio/musicerror.h>       /* ML_ERR_ */
#include <audio/musicscript.h>
#include <audio/music_iff.h>        /* WriteLeafChunk() */
#include <audio/parse_aiff.h>
#include <audio/patch.h>
#include <audio/patchfile.h>
#include <file/filefunctions.h>     /* DeleteFile() */
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <misc/iff.h>
#include <stdio.h>
#include <stdlib.h>                 /* strtol(), strtof() */
#include <string.h>


/* -------------------- Debug control */

#define DEBUG_Dump      0   /* dump intermediate contents */
#define DEBUG_Envelope  0   /* envelope related stuff */
#define DEBUG_Sample    0   /* sample related stuff */
#define DEBUG_Parser    0   /* parser innards */
#define DEBUG_Writer    0


/* -------------------- Patch parser and writer */

typedef struct PatchInfo PatchInfo;

    /* reader/writer */
static Err ParsePatch (PatchInfo **resultPatchInfo, const char *fileName);
static Err WritePatch (const char *fileName, const PatchInfo *);
static void DeletePatchInfo (PatchInfo *);
#if DEBUG_Dump
static void DumpPatchInfo (const PatchInfo *, const char *banner);
#endif


/* -------------------- PatchInfo innards */

typedef struct PatchNameNode {
    Node    pnn;
        /* n_Name - Case-sensitive name */
    int32   pnn_Key;                /* Offset in PNAM chunk + 1 for this name */
} PatchNameNode;

typedef struct PatchInsTemplateNode {
    Node    pitn;
        /* n_Type == PTMP_TYPE_INS */
        /* n_Name - name of .dsp file to load. Case-insensitive. */
} PatchInsTemplateNode;

typedef struct PatchMixerTemplateNode {
    Node    pmtn;
        /* n_Type == PTMP_TYPE_MIXER */
    MixerSpec pmtn_MixerSpec;       /* Mixer spec */
} PatchMixerTemplateNode;

enum {
    PTMP_TYPE_INS,
    PTMP_TYPE_MIXER
};

    /* node for tracking an attachment (variable-sized) */
typedef struct PatchAttachmentNode {
    MinNode pan;
    PatchAttachment pan_Att;        /* (variable-sized) */
} PatchAttachmentNode;

    /* node for tracking an attached (slave) item and all of its attachments */
typedef struct PatchSlaveNode {
    Node    psn;
        /* n_Name is local name of slave item (separate name space from blocks and ports) */
    AudioTagHeader *psn_Header;             /* Pointer to ATAG header. Size available from AudioTagHeaderSize() */
    void   *psn_Body;                       /* Pointer to MEMTYPE_TRACKSIZE allocated body data block,
                                               or NULL if no body data. */
    List    psn_AttachmentList;             /* List of PatchAttachmentNodes */
} PatchSlaveNode;

struct PatchInfo {
        /* FORM PCMD stuff */
    List    pi_TemplateList;                /* mixed list of PatchMixerTemplateNodes and PatchInsTemplateNodes */
    List    pi_NameList;                    /* list of PatchNameNodes */
    int32   pi_TotalNameSize;               /* running total size of PNAM chunk (used to generate pnn_Key */
    PatchCmdBuilder *pi_PatchCmdBuilder;    /* PatchCmdBuilder containing PatchCmd list for patch */
                                            /* @@@ note that all the string pointers have been file-conditioned
                                               prior to adding each PatchCmd. This requires that the
                                               PatchCmdBuilder not actually interpret any of the PatchCmds that
                                               it adds. */

        /* FORM PATT */
    List    pi_SlaveList;                   /* list of PatchSlaveNodes */
};

static Err CreatePatchInfo (PatchInfo **resultPatchInfo);
static int32 AddPatchName (PatchInfo *, const char *name);
static int32 AddPatchInsTemplate (PatchInfo *, const char *insTemplateName);
static int32 AddPatchMixerTemplate (PatchInfo *, MixerSpec mixerSpec);

Err AddPatchSlave (const MusicScriptParser *, PatchInfo *, uint8 nodeType, const char *itemName, const TagArg *);
Err AddPatchSlaveVA (const MusicScriptParser *, PatchInfo *, uint8 nodeType, const char *itemName, uint32 tag1, ...);
static PatchSlaveNode *FindPatchSlave (PatchInfo *, const char *itemName);
static PatchSlaveNode *FindPatchSlaveType (PatchInfo *, const char *slaveName, uint8 nodeType);
Err AddPatchAttachment (PatchSlaveNode *, const char *hookName, const TagArg *);
Err AddPatchAttachmentVA (PatchSlaveNode *, const char *hookName, uint32 tag1, ...);


/* -------------------- misc internal functions */

    /* utility */
static int32 GetTagCount (const TagArg *);
static void CopyTagList (TagArg *dstTagArray, const TagArg *srcTagList);


/* -------------------- main() */

static Err MakePatch (const char *srcFileName, const char *outputFileName, bool checkPatch);
static Err CheckPatch (const char *fileName);

int main (int argc, char *argv[])
{
    Err errcode;

    if (argc < 3) {
        printf ("usage: %s [-Check|-NoCheck] <src script> <output file>\n", argv[0]);
        return -1;
    }

  #ifdef MEMDEBUG
    if ((errcode = CreateMemDebug ( NULL )) < 0) goto clean;

    if ((errcode = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
                                     MEMDEBUGF_FREE_PATTERNS |
                                     MEMDEBUGF_PAD_COOKIES |
                                     MEMDEBUGF_CHECK_ALLOC_FAILURES |
                                     MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto clean;
  #endif

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }
    if ((errcode = OpenIFFFolio()) < 0) {
        PrintError (NULL, "open iff folio", NULL, errcode);
        goto clean;
    }

    {
        bool checkPatch = TRUE;
        const char *srcFileName = NULL;
        const char *outputFileName = NULL;

        {
            int numArgs=0;
            int i;

            for (i=1; i<argc; i++) {
                const char * const arg = argv[i];

                if (arg[0] == '-') {
                    if (!strcasecmp (arg,"-Check")) checkPatch = TRUE;
                    else if (!strcasecmp (arg,"-NoCheck")) checkPatch = FALSE;
                    else {
                        printf ("%s: Unknown switch \"%s\"\n", argv[0], arg);
                        errcode = -1;
                        goto clean;
                    }
                }
                else {
                    switch (numArgs++) {
                        case 0:
                            srcFileName = arg;
                            break;

                        case 1:
                            outputFileName = arg;
                            break;

                        default:
                            printf ("%s: Too many arguments.\n", argv[0]);
                            errcode = -1;
                            goto clean;
                    }
                }
            }

            if (numArgs < 2) {
                printf ("%s: Missing required argument.\n", argv[0]);
                errcode = -1;
                goto clean;
            }
        }

            /* parse args, make patch */
            /* (prints its own error message) */
        errcode = MakePatch (srcFileName, outputFileName, checkPatch);
    }

clean:
    CloseIFFFolio();
    CloseAudioFolio();
  #ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
    printf ("%s: done\n", argv[0]);
  #endif
    return (int)errcode;        /* @@@ potential type truncation */
}

static Err MakePatch (const char *srcFileName, const char *outputFileName, bool checkPatch)
{
    PatchInfo *pi = NULL;
    Err errcode;

        /* parse patch script */
        /* (prints its own error messages) */
    printf ("Reading patch script '%s'...\n", srcFileName);
    if ((errcode = ParsePatch (&pi, srcFileName)) < 0) goto clean;

  #if DEBUG_Dump
    DumpPatchInfo (pi, srcFileName);
  #endif

        /* write binary patch file */
    printf ("Writing binary patch '%s'...\n", outputFileName);
    if ((errcode = WritePatch (outputFileName, pi)) < 0) {
        PrintError (NULL, "write patch", outputFileName, errcode);
        goto clean;
    }

        /* check patch, if requested */
    if (checkPatch) {
        printf ("Checking binary patch '%s'...\n", outputFileName);
        if ((errcode = CheckPatch (outputFileName)) < 0) {
            PrintError (NULL, "check patch", outputFileName, errcode);
            goto clean;
        }
    }

    printf ("done.\n");

clean:
    DeletePatchInfo (pi);
    return errcode;
}

static Err CheckPatch (const char *fileName)
{
    Item patchTemplate;
    Err errcode;

    if ((errcode = patchTemplate = LoadPatchTemplate (fileName)) < 0) goto clean;
    DumpInstrumentResourceInfo (patchTemplate, fileName);

        /* success */
    errcode = 0;

clean:
    UnloadPatchTemplate (patchTemplate);
    return errcode;
}


/* -------------------- Patch Language -> PatchInfo parser */

    /* misc defines */
#define CMD_LineBufSize  1024
#define CMD_ArgArrayElts 64

    /* processing functions */
typedef Err cmdprocproto_t (const MusicScriptParser *, PatchInfo *, int argc, const char * const argv[]);

static cmdprocproto_t cmdproc_Coherence;

static cmdprocproto_t cmdproc_Instrument;
static cmdprocproto_t cmdproc_Mixer;

static cmdprocproto_t cmdproc_Connect;
static cmdprocproto_t cmdproc_Constant;
static cmdprocproto_t cmdproc_Expose;
static cmdprocproto_t cmdproc_Input;
static cmdprocproto_t cmdproc_Knob;
static cmdprocproto_t cmdproc_Output;

static cmdprocproto_t cmdproc_Attach;
static cmdprocproto_t cmdproc_DelayLine;
static cmdprocproto_t cmdproc_DelayLineTemplate;
static cmdprocproto_t cmdproc_Envelope;
static cmdprocproto_t cmdproc_EnvReleaseJump;
static cmdprocproto_t cmdproc_EnvReleaseLoop;
static cmdprocproto_t cmdproc_EnvSustainLoop;
static cmdprocproto_t cmdproc_Sample;

    /* command table */
static const MusicScriptCmdInfo CmdTable[] = {
    { "Coherence",          1, (MusicScriptCmdCallback)cmdproc_Coherence },         /* Coherence [On|Off] */

    { "Instrument",         3, (MusicScriptCmdCallback)cmdproc_Instrument },        /* Instrument <block name> <template file name> */
    { "Mixer",              4, (MusicScriptCmdCallback)cmdproc_Mixer },             /* Mixer <block name> <num inputs> <num outputs> [-LineOut] [-Amplitude] */

    { "Connect",            7, (MusicScriptCmdCallback)cmdproc_Connect },           /* Connect <from block name> <from port name> <from part num> <to block name> <to port name> <to part num> */
    { "Constant",           5, (MusicScriptCmdCallback)cmdproc_Constant },          /* Constant <block name> <port name> <part num> <value> */
    { "Expose",             4, (MusicScriptCmdCallback)cmdproc_Expose },            /* Expose <exposed port name> <source block name> <source port name> */
    { "Input",              4, (MusicScriptCmdCallback)cmdproc_Input },             /* Input <port name> <num parts> <signal type> */
    { "Knob",               5, (MusicScriptCmdCallback)cmdproc_Knob },              /* Knob <knob name> <num parts> <signal type> <default> */
    { "Output",             4, (MusicScriptCmdCallback)cmdproc_Output },            /* Output <port name> <num parts> <signal type> */

    { "Attach",             3, (MusicScriptCmdCallback)cmdproc_Attach },            /* Attach <item name> <exposed port name> [-StartAt <offset>] [-FatLadySings] */
    { "DelayLine",          4, (MusicScriptCmdCallback)cmdproc_DelayLine },         /* DelayLine <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop] */
    { "DelayLineTemplate",  4, (MusicScriptCmdCallback)cmdproc_DelayLineTemplate }, /* DelayLineTemplate <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop] */
    { "Envelope",           5, (MusicScriptCmdCallback)cmdproc_Envelope },          /* Envelope <item name> <signal type>
                                                                                        [-LockTimeScale] [-PitchTimeScaling <base note> <notes/octave>]
                                                                                        <value 0> <duration 0> ... <value N-1> <duration N-1> */
    { "EnvReleaseJump",     3, (MusicScriptCmdCallback)cmdproc_EnvReleaseJump },    /* EnvReleaseJump <item name> <to segment> */
    { "EnvReleaseLoop",     5, (MusicScriptCmdCallback)cmdproc_EnvReleaseLoop },    /* EnvReleaseLoop <item name> <begin segment> <end segment> <loop time> */
    { "EnvSustainLoop",     5, (MusicScriptCmdCallback)cmdproc_EnvSustainLoop },    /* EnvSustainLoop <item name> <begin segment> <end segment> <loop time> */
    { "Sample",             3, (MusicScriptCmdCallback)cmdproc_Sample },            /* Sample <item name> <sample file name>
                                                                                        [-NoteRange <low note> <high note>]
                                                                                        [-VelocityRange <low velocity> <high velocity>]
                                                                                        [-Tune <base note> <detune cents>]
                                                                                        [-SampleRate <sample rate>] */

    NULL
};

typedef struct KeywordEntry {
    const char *kwe_Word;
    int32       kwe_ID;
} KeywordEntry;

typedef struct KeywordTable {
    const char *kwt_Desc;
    const KeywordEntry *kwt_Entries;
    uint32      kwt_NumEntries;
} KeywordTable;

static int32 LookupSignalType (const MusicScriptParser *, const char *keyword);
static int32 LookupBoolean (const MusicScriptParser *, const char *keyword);
static int32 LookupKeyword (const MusicScriptParser *, const KeywordTable *, const char *keyword);
static Err ParseInteger (const MusicScriptParser *, int32 *resultInt, const char *arg);
static Err ParseFloat (const MusicScriptParser *, float32 *resultFloat, const char *arg);

static Err DelayLineCommon (const MusicScriptParser *, PatchInfo *, int argc, const char * const argv[], uint32 delayLineTag);
Err ModifyEnvelopeTags (const MusicScriptParser *, PatchInfo *, const char *itemName, const TagArg *newTagList);
Err ModifyEnvelopeTagsVA (const MusicScriptParser *, PatchInfo *, const char *itemName, uint32 tag1, ...);

/*
    Parses patch script, returns PatchInfo.

    This function and its children print error messages.
*/
static Err ParsePatch (PatchInfo **resultPatchInfo, const char *fileName)
{
    PatchInfo *pi;
    Err errcode;

        /* init result */
    *resultPatchInfo = NULL;

        /* create empty PatchInfo */
    if ((errcode = CreatePatchInfo (&pi)) < 0) {
        PrintError (NULL, "parse patch", fileName, errcode);
        goto clean;
    }

        /* parse file */
        /* (prints its own error messages) */
    if ((errcode = ParseMusicScript (fileName, CMD_LineBufSize, CMD_ArgArrayElts, CmdTable, pi)) < 0) goto clean;

        /* success: set result */
    *resultPatchInfo = pi;
    return 0;

clean:
    DeletePatchInfo (pi);
    return errcode;
}


/* Coherence [On|Off] */
static Err cmdproc_Coherence (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 state;
    Err errcode;

    TOUCH(argc);

    if ((errcode = state = LookupBoolean (msp, argv[1])) < 0) goto error_quiet;
    if ((errcode = SetPatchCoherence (pi->pi_PatchCmdBuilder, state)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* Instrument <block name> <template file name> */
static Err cmdproc_Instrument (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 blockNameKey;
    int32 insTemplateIndex;
    Err errcode;

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */
    TOUCH(argc);

    if ((errcode = blockNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = insTemplateIndex = AddPatchInsTemplate (pi, argv[2])) < 0) goto error_msg;
    if ((errcode = AddTemplateToPatch (pi->pi_PatchCmdBuilder, (char *)blockNameKey, insTemplateIndex)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
/* error_quiet: */
    return errcode;
}

/* Mixer <block name> <num inputs> <num outputs> [-LineOut] [-Amplitude] */
static Err cmdproc_Mixer (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 mixerTemplateIndex;
    Err errcode;

        /* generate mixer spec */
    {
        int32 numIns;
        int32 numOuts;
        uint32 flags = 0;

        if ((errcode = ParseInteger (msp, &numIns, argv[2])) < 0) goto error_quiet;
        if ((errcode = ParseInteger (msp, &numOuts, argv[3])) < 0) goto error_quiet;
        {
            int i;

            for (i=4; i<argc; i++) {
                if (!strcasecmp (argv[i], "-LineOut")) flags |= AF_F_MIXER_WITH_LINE_OUT;
                else if (!strcasecmp (argv[i], "-Amplitude")) flags |= AF_F_MIXER_WITH_AMPLITUDE;
                else {
                    errcode = ML_ERR_BAD_KEYWORD;
                    MSERR((msp, errcode, "Unknown switch \"%s\"", argv[i]));
                    goto error_quiet;
                }
            }
        }
        if ((errcode = mixerTemplateIndex = AddPatchMixerTemplate (pi, MakeMixerSpec(numIns, numOuts, flags))) < 0) goto error_msg;
    }

        /* add template to patch */
    {
        int32 blockNameKey;

        if ((errcode = blockNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
        if ((errcode = AddTemplateToPatch (pi->pi_PatchCmdBuilder, (char *)blockNameKey, mixerTemplateIndex)) < 0) goto error_msg;
    }

    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}


/* Connect <from block name> <from port name> <from part num> <to block name> <to port name> <to part num> */
static Err cmdproc_Connect (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 fromBlockNameKey;
    int32 fromPortNameKey;
    int32 fromPartNum;
    int32 toBlockNameKey;
    int32 toPortNameKey;
    int32 toPartNum;
    Err errcode;

    TOUCH(argc);

    if ((errcode = fromBlockNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = fromPortNameKey  = AddPatchName (pi, argv[2])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &fromPartNum, argv[3])) < 0) goto error_quiet;
    if ((errcode = toBlockNameKey   = AddPatchName (pi, argv[4])) < 0) goto error_msg;
    if ((errcode = toPortNameKey    = AddPatchName (pi, argv[5])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &toPartNum, argv[6])) < 0) goto error_quiet;
    if ((errcode = ConnectPatchPorts (pi->pi_PatchCmdBuilder,
        (char *)fromBlockNameKey, (char *)fromPortNameKey, fromPartNum,
        (char *)toBlockNameKey,   (char *)toPortNameKey,   toPartNum)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* Constant <block name> <port name> <part num> <value> */
static Err cmdproc_Constant (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 blockNameKey;
    int32 portNameKey;
    int32 partNum;
    float32 value;
    Err errcode;

    TOUCH(argc);

    if ((errcode = blockNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = portNameKey  = AddPatchName (pi, argv[2])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &partNum, argv[3])) < 0) goto error_quiet;
    if ((errcode = ParseFloat (msp, &value, argv[4])) < 0) goto error_quiet;
    if ((errcode = SetPatchConstant (pi->pi_PatchCmdBuilder, (char *)blockNameKey, (char *)portNameKey, partNum, value)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* Expose <exposed port name> <source block name> <source port name> */
static Err cmdproc_Expose (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 asPortNameKey;
    int32 srcBlockNameKey;
    int32 srcPortNameKey;
    Err errcode;

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */
    TOUCH(argc);

    if ((errcode = asPortNameKey   = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = srcBlockNameKey = AddPatchName (pi, argv[2])) < 0) goto error_msg;
    if ((errcode = srcPortNameKey  = AddPatchName (pi, argv[3])) < 0) goto error_msg;
    if ((errcode = ExposePatchPort (pi->pi_PatchCmdBuilder, (char *)asPortNameKey, (char *)srcBlockNameKey, (char *)srcPortNameKey)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
/* error_quiet: */
    return errcode;
}

/* Input <port name> <num parts> <signal type> */
static Err cmdproc_Input (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 portNameKey;
    int32 numParts;
    int32 sigType;
    Err errcode;

    TOUCH(argc);

    if ((errcode = portNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &numParts, argv[2])) < 0) goto error_quiet;
    if ((errcode = sigType = LookupSignalType (msp, argv[3])) < 0) goto error_quiet;
    if ((errcode = DefinePatchPort (pi->pi_PatchCmdBuilder, (char *)portNameKey, numParts, AF_PORT_TYPE_INPUT, sigType)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* Knob <knob name> <num parts> <signal type> <default> */
static Err cmdproc_Knob (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 knobNameKey;
    int32 numParts;
    int32 sigType;
    float32 defaultValue;
    Err errcode;

    TOUCH(argc);

    if ((errcode = knobNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &numParts, argv[2])) < 0) goto error_quiet;
    if ((errcode = sigType = LookupSignalType (msp, argv[3])) < 0) goto error_quiet;
    if ((errcode = ParseFloat (msp, &defaultValue, argv[4])) < 0) goto error_quiet;
    if ((errcode = DefinePatchKnob (pi->pi_PatchCmdBuilder, (char *)knobNameKey, numParts, sigType, defaultValue)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* Output <port name> <num parts> <signal type> */
static Err cmdproc_Output (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 portNameKey;
    int32 numParts;
    int32 sigType;
    Err errcode;

    TOUCH(argc);

    if ((errcode = portNameKey = AddPatchName (pi, argv[1])) < 0) goto error_msg;
    if ((errcode = ParseInteger (msp, &numParts, argv[2])) < 0) goto error_quiet;
    if ((errcode = sigType = LookupSignalType (msp, argv[3])) < 0) goto error_quiet;
    if ((errcode = DefinePatchPort (pi->pi_PatchCmdBuilder, (char *)portNameKey, numParts, AF_PORT_TYPE_OUTPUT, sigType)) < 0) goto error_msg;
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}


/* Attach <item name> <exposed port name> [-StartAt <offset>] [-FatLadySings] */
static Err cmdproc_Attach (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    const char *itemName = argv[1];
    const char *portName = argv[2];
    PatchSlaveNode *psn;
    uint8 attFlags = 0;
    int32 startAt = 0;
    Err errcode;

    TOUCH(argc);

        /* find slave */
    if (!(psn = FindPatchSlave (pi, itemName))) {
        errcode = ML_ERR_BAD_ARG;
        MSERR((msp, errcode, "Attached item \"%s\" not found", itemName));
        goto error_quiet;
    }

        /* check port name */
    if (strlen (portName) > AF_MAX_NAME_LENGTH) {
        errcode = PATCH_ERR_NAME_TOO_LONG;
        MSERR((msp, errcode, "Port name \"%s\" too long", portName));
        goto error_quiet;
    }

        /* parse switches - set atts_Flags and atts_StartAt */
    {
        const char * const *argp = &argv[3];
        const char *arg;

        while (arg = *argp++) {
            if (!strcasecmp (arg, "-FatLadySings"))     attFlags |= AF_ATTF_FATLADYSINGS;
        /*  else if (!strcasecmp (arg, "-NoAutoStart")) attFlags |= AF_ATTF_NOAUTOSTART;    @@@ no use at the moment */
            else if (!strcasecmp (arg, "-StartAt")) {
                const char *valarg;

                if (!(valarg = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseInteger (msp, &startAt, valarg)) < 0) goto error_quiet;
            }
            else {
                errcode = ML_ERR_BAD_KEYWORD;
                MSERR((msp, errcode, "Unknown switch \"%s\"", arg));
                goto error_quiet;
            }
        }
    }

        /* add PatchAttachmentNode */
    if ((errcode = AddPatchAttachmentVA (psn, portName,
        attFlags ? AF_TAG_SET_FLAGS : TAG_NOP,  attFlags,
        startAt  ? AF_TAG_START_AT  : TAG_NOP,  startAt,
        TAG_END)) < 0) goto error_msg;

        /* success */
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    return errcode;
}

/* DelayLine <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop] */
static Err cmdproc_DelayLine (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    return DelayLineCommon (msp, pi, argc, argv, AF_TAG_DELAY_LINE);
}

/* DelayLineTemplate <item name> <num bytes> <num channels> [-NoLoop] [-ClearOnStop] */
static Err cmdproc_DelayLineTemplate (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    return DelayLineCommon (msp, pi, argc, argv, AF_TAG_DELAY_LINE_TEMPLATE);
}

/*
    Common command processor for DelayLine and DelayLineTemplate commands.

    Arguments
        msp, pi, argc, argv
            Args passed to cmdproc_DelayLine() or cmdproc_DelayLineTemplate().

        delayLineTag
            Tag ID: AF_TAG_DELAY_LINE to create delay line,
            AF_TAG_DELAY_LINE_TEMPLATE to create delay line template.

    Results
        0 on success, Err code on failure. Prints its own error messages.
*/
static Err DelayLineCommon (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[], uint32 delayLineTag)
{
    const char *itemName = argv[1];
    int32 numBytes;
    int32 numChannels;
    bool loop = TRUE;
    uint8 sampFlags = 0;
    Err errcode;

    TOUCH(argc);

        /* parse numbers (prints its own error messages) */
    if ((errcode = ParseInteger (msp, &numBytes, argv[2])) < 0) goto error_quiet;
    if ((errcode = ParseInteger (msp, &numChannels, argv[3])) < 0) goto error_quiet;

        /* parse switches */
    {
        const char * const *argp = &argv[4];
        const char *arg;

        while (arg = *argp++) {
            if (!strcasecmp (arg, "-NoLoop")) loop = FALSE;
            else if (!strcasecmp (arg, "-ClearOnStop")) sampFlags |= AF_SAMPF_CLEARONSTOP;
            else {
                errcode = ML_ERR_BAD_KEYWORD;
                MSERR((msp, errcode, "Unknown switch \"%s\"", arg));
                goto error_quiet;
            }
        }
    }

        /* add PatchSlaveNode (prints its own error message) */
    if ((errcode = AddPatchSlaveVA (msp, pi, AUDIO_SAMPLE_NODE, itemName,
        delayLineTag,    numBytes,
        AF_TAG_CHANNELS, numChannels,
        sampFlags ? AF_TAG_SET_FLAGS    : TAG_NOP, sampFlags,
        loop      ? AF_TAG_RELEASEBEGIN : TAG_NOP, 0,
        loop      ? AF_TAG_RELEASEEND   : TAG_NOP, numBytes / (numChannels * 2), /* convert numBytes to frames */
        TAG_END)) < 0) goto error_quiet;

        /* success */
    return 0;

/*
error_msg:
    MSERR((msp, errcode, NULL));
*/
error_quiet:
    return errcode;
}

/* Envelope <item name> <signal type> [-LockTimeScale]
   [-PitchTimeScaling <base note> <notes/octave>]
   <value 0> <duration 0> ... <value N-1> <duration N-1> */
static Err cmdproc_Envelope (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    const char * const itemName = argv[1];
    int32 sigType;
    uint32 envFlags = 0;
    int32 ptsBaseNote = AF_MIDDLE_C_PITCH;
    int32 ptsNotesPerOctave = 0;
    const char * const *argp;
    int32 numEnvSegs;
    EnvelopeSegment *envSegs = NULL;
    Err errcode;

        /* parse fixed args */
    if ((errcode = sigType = LookupSignalType (msp, argv[2])) < 0) goto error_quiet;

        /* parse switches
        ** - Set envFlags, pteBaseNote, and pteNotesPerOctave.
        ** - Position argp to first envelope segment arg.
        */
    argp = &argv[3];
    {
        const char *arg;

        while (arg = *argp++) {
            if (!strcasecmp (arg, "-LockTimeScale")) envFlags |= AF_ENVF_LOCKTIMESCALE;
            else if (!strcasecmp (arg, "-PitchTimeScaling")) {
                const char *valarg1, *valarg2;

                if (!(valarg1 = *argp++) || !(valarg2 = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseInteger (msp, &ptsBaseNote, valarg1)) < 0) goto error_quiet;
                if ((errcode = ParseInteger (msp, &ptsNotesPerOctave, valarg2)) < 0) goto error_quiet;
            }
            else break;     /* anything else is an envelope point - break out of loop */
        }
        argp--;     /* back up to arg that caused loop to exit - which is either the first env seg, or NULL */
    }

        /* get EnvelopeSegment array from remaining args - sets numEnvSegs and envSegs */
    {
            /* determine number of envelope segments based on the remaining number of arguments */
        numEnvSegs = (argc-(argp-argv)) / 2;
      #if DEBUG_Envelope
        printf ("cmdproc_Envelope: '%s', env args start at argv[%d] '%s', numEnvSegs=%d\n", itemName, argp-argv, *argp, numEnvSegs);
      #endif
        if (numEnvSegs < 1) {
            errcode = ML_ERR_MISSING_ARG;
            MSERR((msp, errcode, "Missing envelope segments"));
            goto error_quiet;
        }

            /* alloc envSegs */
        if (!(envSegs = AllocMem (numEnvSegs * sizeof (EnvelopeSegment), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
            errcode = MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
            goto error_msg;
        }

            /* fill in envSegs from arguments */
        {
            int32 i;

            for (i=0; i<numEnvSegs; i++) {
                if ((errcode = ParseFloat (msp, &envSegs[i].envs_Value, *argp++)) < 0) goto error_quiet;
                if ((errcode = ParseFloat (msp, &envSegs[i].envs_Duration, *argp++)) < 0) goto error_quiet;
            }
        }
    }

        /* add PatchSlaveNode (prints its own error message) */
        /* @@@ since this is the last thing done, no need to have failure path deal w/
               the fact that envSegs is owned by PatchSlaveNode after success */
    if ((errcode = AddPatchSlaveVA (msp, pi, AUDIO_ENVELOPE_NODE, itemName,
        AF_TAG_ADDRESS,        envSegs,
        AF_TAG_FRAMES,         numEnvSegs,
        AF_TAG_TYPE,           sigType,
        envFlags          ? AF_TAG_SET_FLAGS      : TAG_NOP, envFlags,
        ptsNotesPerOctave ? AF_TAG_BASENOTE       : TAG_NOP, ptsBaseNote,
        ptsNotesPerOctave ? AF_TAG_NOTESPEROCTAVE : TAG_NOP, ptsNotesPerOctave,
            /* place holders for Env loop commands */
        AF_TAG_SUSTAINBEGIN,   -1,
        AF_TAG_SUSTAINEND,     -1,
        AF_TAG_SUSTAINTIME_FP, ConvertFP_TagData(0.0),
        AF_TAG_RELEASEBEGIN,   -1,
        AF_TAG_RELEASEEND,     -1,
        AF_TAG_RELEASETIME_FP, ConvertFP_TagData(0.0),
        AF_TAG_RELEASEJUMP,    -1,
        TAG_END)) < 0) goto error_quiet;

        /* success */
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    FreeMem (envSegs, TRACKED_SIZE);
    return errcode;
}

/* EnvReleaseJump <item name> <to segment> */
static Err cmdproc_EnvReleaseJump (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 releaseJump;
    Err errcode;

    TOUCH(argc);

    if ((errcode = ParseInteger (msp, &releaseJump, argv[2])) < 0) return errcode;

    return ModifyEnvelopeTagsVA (msp, pi, argv[1],
        AF_TAG_RELEASEJUMP, releaseJump,
        TAG_END);
}

/* EnvReleaseLoop <item name> <begin segment> <end segment> <loop time> */
static Err cmdproc_EnvReleaseLoop (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 releaseBegin;
    int32 releaseEnd;
    float32 releaseTime;
    Err errcode;

    TOUCH(argc);

    if ((errcode = ParseInteger (msp, &releaseBegin, argv[2])) < 0) return errcode;
    if ((errcode = ParseInteger (msp, &releaseEnd,   argv[3])) < 0) return errcode;
    if ((errcode = ParseFloat   (msp, &releaseTime,  argv[4])) < 0) return errcode;

    return ModifyEnvelopeTagsVA (msp, pi, argv[1],
        AF_TAG_RELEASEBEGIN,   releaseBegin,
        AF_TAG_RELEASEEND,     releaseEnd,
        AF_TAG_RELEASETIME_FP, ConvertFP_TagData(releaseTime),
        TAG_END);
}

/* EnvSustainLoop <item name> <begin segment> <end segment> <loop time> */
static Err cmdproc_EnvSustainLoop (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    int32 sustainBegin;
    int32 sustainEnd;
    float32 sustainTime;
    Err errcode;

    TOUCH(argc);

    if ((errcode = ParseInteger (msp, &sustainBegin, argv[2])) < 0) return errcode;
    if ((errcode = ParseInteger (msp, &sustainEnd,   argv[3])) < 0) return errcode;
    if ((errcode = ParseFloat   (msp, &sustainTime,  argv[4])) < 0) return errcode;

    return ModifyEnvelopeTagsVA (msp, pi, argv[1],
        AF_TAG_SUSTAINBEGIN,   sustainBegin,
        AF_TAG_SUSTAINEND,     sustainEnd,
        AF_TAG_SUSTAINTIME_FP, ConvertFP_TagData(sustainTime),
        TAG_END);
}

/*
    Modify Envelope PatchSlaveNode's athd_Tag list by overwriting placeholder
    tags with new values.

    Arguments
        msp

        pi
            PatchInfo containing attached envelope to modify.

        itemName
            Attached item symbol name of envelope to modify

        newTags
            tag list containing replacement values for tags which are already
            in athd_Tags.

    Results
        0 on success, Err code on failure. Prints its own error messages.
*/
Err ModifyEnvelopeTags (const MusicScriptParser *msp, PatchInfo *pi, const char *itemName, const TagArg *newTagList)
{
    PatchSlaveNode *psn;
    Err errcode;

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */

        /* find envelope's PatchSlaveNode */
    if (!(psn = FindPatchSlaveType (pi, itemName, AUDIO_ENVELOPE_NODE))) {
        errcode = PATCH_ERR_NAME_NOT_FOUND;
        MSERR((msp, errcode, "Attached envelope \"%s\" not found", itemName));
        goto error_quiet;
    }

        /* replace tag values (this won't fail if we set it up correctly in cmdproc_Envelope()) */
    {
        const TagArg *newTag, *tstate;
        TagArg *const modifyTagList = psn->psn_Header->athd_Tags;
        TagArg *modifyTag;

        for (tstate = newTagList; newTag = NextTagArg (&tstate); ) {
            if (modifyTag = FindTagArg (modifyTagList, newTag->ta_Tag))
                modifyTag->ta_Arg = newTag->ta_Arg;
        }
    }

    return 0;

error_quiet:
    return errcode;
}


/*
    Sample <item name> <sample file name>
        [-NoteRange <low note> <high note>]
        [-VelocityRange <low velocity> <high velocity>]
        [-Tune <base note> <detune cents>]
        [-SampleRate <sample rate>]
*/
static Err cmdproc_Sample (const MusicScriptParser *msp, PatchInfo *pi, int argc, const char * const argv[])
{
    const char *itemName = argv[1];
    const char *fileName = argv[2];
    SampleInfo *sampInfo = NULL;
    TagArg sampTags [ML_SAMPLEINFO_MAX_TAGS];
    Err errcode;

    TOUCH(argc);

        /* get SampleInfo */
    if ((errcode = GetAIFFSampleInfo (&sampInfo, fileName, 0)) < 0) {
        MSERR((msp, errcode, "Unable to load sample \"%s\"", fileName));
        goto error_quiet;
    }

        /* parse switches - apply changes to SampleInfo */
    {
        const char * const *argp = &argv[3];
        const char *arg;

        while (arg = *argp++) {
            if (!strcasecmp (arg, "-NoteRange")) {
                const char *valarg1, *valarg2;
                int32 lowNote, highNote;

                if (!(valarg1 = *argp++) || !(valarg2 = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseInteger (msp, &lowNote,  valarg1)) < 0) goto error_quiet;
                if ((errcode = ParseInteger (msp, &highNote, valarg2)) < 0) goto error_quiet;

                sampInfo->smpi_LowNote  = lowNote;
                sampInfo->smpi_HighNote = highNote;
            }
            else if (!strcasecmp (arg, "-VelocityRange")) {
                const char *valarg1, *valarg2;
                int32 lowVelocity, highVelocity;

                if (!(valarg1 = *argp++) || !(valarg2 = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseInteger (msp, &lowVelocity,  valarg1)) < 0) goto error_quiet;
                if ((errcode = ParseInteger (msp, &highVelocity, valarg2)) < 0) goto error_quiet;

                sampInfo->smpi_LowVelocity  = lowVelocity;
                sampInfo->smpi_HighVelocity = highVelocity;
            }
            else if (!strcasecmp (arg, "-Tune")) {
                const char *valarg1, *valarg2;
                int32 baseNote, detune;

                if (!(valarg1 = *argp++) || !(valarg2 = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseInteger (msp, &baseNote, valarg1)) < 0) goto error_quiet;
                if ((errcode = ParseInteger (msp, &detune,   valarg2)) < 0) goto error_quiet;

                sampInfo->smpi_BaseNote = baseNote;
                sampInfo->smpi_Detune   = detune;
            }
            else if (!strcasecmp (arg, "-SampleRate")) {
                const char *valarg;

                if (!(valarg = *argp++)) {
                    errcode = ML_ERR_MISSING_ARG;
                    MSERR((msp, errcode, "Missing value for %s", arg));
                    goto error_quiet;
                }
                if ((errcode = ParseFloat (msp, &sampInfo->smpi_SampleRate, valarg)) < 0) goto error_quiet;
            }
            else {
                errcode = ML_ERR_BAD_KEYWORD;
                MSERR((msp, errcode, "Unknown switch \"%s\"", arg));
                goto error_quiet;
            }
        }
    }

  #if DEBUG_Sample
    DumpSampleInfo (sampInfo, itemName);
  #endif

        /* generate tag list */
    if ((errcode = SampleInfoToTags (sampInfo, sampTags, sizeof sampTags / sizeof sampTags[0])) < 0) goto error_msg;

        /* add PatchSlaveNode (prints its own error message) */
    if ((errcode = AddPatchSlave (msp, pi, AUDIO_SAMPLE_NODE, itemName, sampTags)) < 0) goto error_quiet;

        /* mark data as belonging to PatchSlaveNode now */
    sampInfo->smpi_Flags &= ~ML_SAMPLEINFO_F_SAMPLE_ALLOCATED;

        /* success */
    errcode = 0;

error_msg:
    if (errcode < 0) MSERR((msp, errcode, NULL));
error_quiet:
    DeleteSampleInfo (sampInfo);
    return errcode;
}


/* parser helpers */

/*
    Lookup signal type. Prints its own error messages.

    Results
        AF_SIGNAL_TYPE on match, or Err code on failure.
*/
static int32 LookupSignalType (const MusicScriptParser *msp, const char *keyword)
{
    static const KeywordEntry entries[] = {
        { "Signed",     AF_SIGNAL_TYPE_GENERIC_SIGNED },
        { "Unsigned",   AF_SIGNAL_TYPE_GENERIC_UNSIGNED },
        { "OscFreq",    AF_SIGNAL_TYPE_OSC_FREQ },
        { "LFOFreq",    AF_SIGNAL_TYPE_LFO_FREQ },
        { "SampRate",   AF_SIGNAL_TYPE_SAMPLE_RATE },
        { "Whole",      AF_SIGNAL_TYPE_WHOLE_NUMBER },
    };
    static const KeywordTable table = {
        "signal type",
        entries,
        sizeof entries / sizeof entries[0]
    };

    return LookupKeyword (msp, &table, keyword);
}


/*
    Lookup boolean. Prints its own error messages.

    Results
        TRUE or FALSE on success, Err code on failure.
*/
static int32 LookupBoolean (const MusicScriptParser *msp, const char *keyword)
{
    static const KeywordEntry entries[] = {
        { "Off", FALSE },
        { "On",  TRUE },
    };
    static const KeywordTable table = {
        "boolean keyword",
        entries,
        sizeof entries / sizeof entries[0]
    };

    return LookupKeyword (msp, &table, keyword);
}


/*
    Lookup keyword. Prints its own error messages.

    Arguments
        msp
            MusicScriptParser containing keyword being looked up.

        keywordTable
            KeywordTable to look up keyword in.

        keyword
            Word to match (case-insensitively)

    Results
        Identifier stored in keyword table on success (non-negative), Err code
        if not found in table.
*/
static int32 LookupKeyword (const MusicScriptParser *msp, const KeywordTable *keywordTable, const char *keyword)
{
    const KeywordEntry *entry = keywordTable->kwt_Entries;
    uint32 numEntries = keywordTable->kwt_NumEntries;

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */

    for (; numEntries--; entry++) {
        if (!strcasecmp (keyword, entry->kwe_Word)) return entry->kwe_ID;
    }

    MSERR((msp, ML_ERR_BAD_KEYWORD, "Unknown %s \"%s\"", keywordTable->kwt_Desc, keyword));
    return ML_ERR_BAD_KEYWORD;
}


/*
    Gets integer from arg. Prints its own error message if number contains
    non-numeric characters.

    Arguments
        msp
            MusicScriptParser with line containing arg being processed.

        resultVal
            Buffer to write resulting int32.

        arg
            Argument string. May be in decimal or hex.

    Results
        0 on success, Err code on failure.
        Writes to resultVal on success; doesn't touch it on failure.
*/
static Err ParseInteger (const MusicScriptParser *msp, int32 *resultVal, const char *arg)
{
    const char *endp;
    const int32 val = strtol (arg, &endp, 0);

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */

    if (*endp) {
        /* @@@ const */ Err errcode = ML_ERR_BAD_NUMBER;

        MSERR((msp, errcode, "Bad integer value \"%s\"", arg));
        return errcode;
    }

        /* success: set result */
    *resultVal = val;
    return 0;
}


/*
    Gets floating point value from arg. Prints its own error message if number
    contains non-numeric characters.

    Arguments
        msp
            MusicScriptParser with line containing arg being processed.

        resultVal
            Buffer to write resulting float32.

        arg
            Argument string. May be in decimal or hex.

    Results
        0 on success, Err code on failure.
        Writes to resultVal on success; doesn't touch it on failure.
*/
static Err ParseFloat (const MusicScriptParser *msp, float32 *resultVal, const char *arg)
{
    const char *endp;
    const float32 val = strtof (arg, &endp);

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */

    if (*endp) {
        /* @@@ const */ Err errcode = ML_ERR_BAD_NUMBER;

        MSERR((msp, errcode, "Bad floating point value \"%s\"", arg));
        return errcode;
    }

        /* success: set result */
    *resultVal = val;
    return 0;
}


/* -------------------- PatchInfo manager */

#define DeletePatchNameNode FreeMemTrack
static PatchNameNode *FindPatchName (const PatchInfo *pi, const char *name);
#define DeletePatchTemplateNode FreeMemTrack
static PatchMixerTemplateNode *FindPatchMixerTemplate (const PatchInfo *pi, MixerSpec mixerSpec);
static void DeletePatchSlaveNode (PatchSlaveNode *);
#define DeletePatchAttachmentNode(pan) FreeMem ((pan), TRACKED_SIZE)

/*
    Create a new PatchInfo

    Results
        0 on success, Err code on failure.
*/
static Err CreatePatchInfo (PatchInfo **resultPatchInfo)
{
    PatchInfo *pi;
    Err errcode;

        /* init result */
    *resultPatchInfo = NULL;

        /* alloc/init PatchInfo */
    if (!(pi = AllocMem (sizeof *pi, MEMTYPE_NORMAL | MEMTYPE_FILL))) {
        errcode = MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
        goto clean;
    }
    PrepList (&pi->pi_TemplateList);
    PrepList (&pi->pi_NameList);
    PrepList (&pi->pi_SlaveList);

        /* create PatchCmdBuilder */
    if ((errcode = CreatePatchCmdBuilder (&pi->pi_PatchCmdBuilder)) < 0) goto clean;

        /* success: set result */
    *resultPatchInfo = pi;
    return 0;

clean:
    DeletePatchInfo (pi);
    return errcode;
}

/*
    Delete PatchInfo

    Arguments
        pi
            PatchInfo to delete. Can be NULL or partially constructed.
*/
static void DeletePatchInfo (PatchInfo *pi)
{
    if (pi) {
        {
            Node *n, *succ;

            PROCESSLIST (&pi->pi_TemplateList, n, succ, Node) {
                DeletePatchTemplateNode(n);
            }
        }
        {
            PatchNameNode *n, *succ;

            PROCESSLIST (&pi->pi_NameList, n, succ, PatchNameNode) {
                DeletePatchNameNode(n);
            }
        }
        DeletePatchCmdBuilder (pi->pi_PatchCmdBuilder);
        {
            PatchSlaveNode *n, *succ;

            PROCESSLIST (&pi->pi_SlaveList, n, succ, PatchSlaveNode) {
                DeletePatchSlaveNode (n);
            }
        }
        FreeMem (pi, sizeof *pi);
    }
}

/*
    Finds an existing PatchNameNode (or adds a new PatchNameNode if a new name)
    and returns its key for storage in PCMD chunk.

    Arguments
        pi
        name
            Name to return key for. Can be the self-referential name, ".", which
            causes 0 to be returned. Names are case-sensitive in order to preserve
            capitalization from source file.

    Results
        >=0
            key

        <0
            error code
*/
static int32 AddPatchName (PatchInfo *pi, const char *name)
{
    PatchNameNode *pnn;
    int32 nameSize;

  #if DEBUG_Parser
    printf ("AddPatchName: '%s'\n", name);
  #endif

        /* if this is the self referential string, return magic key for NULL */
    if (!strcmp (name, ".")) return 0;

        /* if name is already in list, return its key */
    if (pnn = FindPatchName (pi, name)) return pnn->pnn_Key;

        /* otherwise, allocate a new PatchNameNode + string */
    nameSize = strlen(name) + 1;
    if (!(pnn = AllocMem (sizeof *pnn + nameSize, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        return MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
    }
    pnn->pnn.n_Name = (char *)(pnn + 1);
    strcpy (pnn->pnn.n_Name, name);
    pnn->pnn_Key = pi->pi_TotalNameSize + 1;        /* translate offset -> key by adding 1 */

  #if DEBUG_Parser
    printf ("              key=%d\n", pnn->pnn_Key);
  #endif

        /* add to name list */
    AddTail (&pi->pi_NameList, (Node *)pnn);
    pi->pi_TotalNameSize += nameSize;

        /* return key from new PatchNameNode */
    return pnn->pnn_Key;
}

/*
    Finds a PatchNameNode (case-sensitive search)
*/
static PatchNameNode *FindPatchName (const PatchInfo *pi, const char *name)
{
    PatchNameNode *pnn;

    SCANLIST (&pi->pi_NameList, pnn, PatchNameNode) {
        if (!strcmp (name, pnn->pnn.n_Name)) return pnn;
    }
    return NULL;
}

/*
    Finds an existing PatchInsTemplateNode (or adds a new one if a not found)
    and returns its index for storage in PCMD chunk.

    Arguments
        pi
        insTemplateName

    Results
        >=0
            index

        <0
            error code
*/
static int32 AddPatchInsTemplate (PatchInfo *pi, const char *insTemplateName)
{
    PatchInsTemplateNode *pitn;

        /* if ins template is already in list, return its index */
    if (pitn = (PatchInsTemplateNode *)FindNamedNode (&pi->pi_TemplateList, insTemplateName))
        return GetNodePosFromHead (&pi->pi_TemplateList, (Node *)pitn);

        /* otherwise, allocate a new PatchInsTemplateNode + name */
    if (!(pitn = AllocMem (sizeof *pitn + strlen(insTemplateName) + 1, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        return MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
    }
    pitn->pitn.n_Type = PTMP_TYPE_INS;
    pitn->pitn.n_Name = (char *)(pitn + 1);
    strcpy (pitn->pitn.n_Name, insTemplateName);

        /* add to template list */
    AddTail (&pi->pi_TemplateList, (Node *)pitn);

        /* return index of new node */
    return GetNodePosFromHead (&pi->pi_TemplateList, (Node *)pitn);
}


/*
    Finds an existing PatchMixerTemplateNode (or adds a new one if a not found)
    and returns its index for storage in PCMD chunk.

    Arguments
        pi
        mixerSpec

    Results
        >=0
            index

        <0
            error code
*/
static int32 AddPatchMixerTemplate (PatchInfo *pi, MixerSpec mixerSpec)
{
    PatchMixerTemplateNode *pmtn;

        /* if mixer spec is already in list, return its index */
    if (pmtn = FindPatchMixerTemplate (pi, mixerSpec))
        return GetNodePosFromHead (&pi->pi_TemplateList, (Node *)pmtn);

        /* otherwise, allocate a new PatchMixerTemplateNode */
    if (!(pmtn = AllocMem (sizeof *pmtn, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        return MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
    }
    pmtn->pmtn.n_Type = PTMP_TYPE_MIXER;
    pmtn->pmtn_MixerSpec = mixerSpec;

        /* add to template list */
    AddTail (&pi->pi_TemplateList, (Node *)pmtn);

        /* return index of new node */
    return GetNodePosFromHead (&pi->pi_TemplateList, (Node *)pmtn);
}


/*
    Finds a PatchMixerTemplateNode.
*/
static PatchMixerTemplateNode *FindPatchMixerTemplate (const PatchInfo *pi, MixerSpec mixerSpec)
{
    PatchMixerTemplateNode *pmtn;

    SCANLIST (&pi->pi_TemplateList, pmtn, PatchMixerTemplateNode) {
        if (pmtn->pmtn.n_Type == PTMP_TYPE_MIXER && pmtn->pmtn_MixerSpec == mixerSpec) return pmtn;
    }
    return NULL;
}


/*
    Add PatchSlaveNode. If successful, any body data pointer is now owned by
    the PatchSlaveNode. It will be automatically freed when the PatchSlaveNode
    is freed. If this function fails, the data pointer remains the property of
    the caller and must be disposed of manually.

    Arguments
        msp
            MusicScriptParser containing file and line no of line responsible
            for creating this PatchSlaveNode. Used for diagnostics only.

        pi
            PatchInfo containing pi_SlaveList to add to.

        nodeType
            AUDIO_<type>_NODE for item.

        itemName
            Internal symbol name for slave item. Case-insensitive. Checked for
            uniqueness.

        tagList
            AF_TAG_ tag list to write in ATAG. Ignores inappropriate tags:
            AF_TAG_AUTO_FREE_DATA and all control tags. AF_TAG_ADDRESS is removed
            from ATAG list, but value is used to write BODY chunk.

    Results
        0 on success, Err code on failure.

    Notes
        Prints its own error messages.
*/
/* @@@ not static because the VA glue needs to find it */
Err AddPatchSlave (const MusicScriptParser *msp, PatchInfo *pi, uint8 nodeType, const char *itemName, const TagArg *tagList)
{
    PatchSlaveNode *psn = NULL;
    int32 numTags;
    void *body = NULL;
    Err errcode;

    TOUCH(msp);     /* silence warning when MSERR() is a NOP */

  #if DEBUG_Parser
    printf ("AddPatchSlave: '%s' type=%d\n", itemName, nodeType);
    DumpTagList (tagList, NULL);
  #endif

        /* check itemName uniqueness */
    if (FindPatchSlave (pi, itemName)) {
        errcode = PATCH_ERR_NAME_NOT_UNIQUE;
        MSERR((msp, errcode, "Attached item name \"%s\" not unique", itemName));
        goto error_quiet;
    }

        /* count tags */
    {
        const TagArg *srcTag, *tstate;

        numTags = 0;
        for (tstate = tagList; srcTag = NextTagArg (&tstate); ) {
            switch (srcTag->ta_Tag) {
                case AF_TAG_ADDRESS:
                case AF_TAG_AUTO_FREE_DATA:
                    break;

                default:
                    numTags++;
                    break;
            }
        }
        numTags++;      /* space for TAG_END */
    }

        /* alloc/init PatchSlaveNode + AudioTagHeader + Name */
    if (!(psn = AllocMem ( sizeof (PatchSlaveNode) + AudioTagHeaderSize (numTags) + strlen (itemName) + 1,
                           MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL ))) {
        errcode = MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
        goto error_msg;
    }
    psn->psn_Header = (AudioTagHeader *)(psn + 1);
    psn->psn.n_Name = (char *)psn->psn_Header + AudioTagHeaderSize (numTags);
    strcpy (psn->psn.n_Name, itemName);
    PrepList (&psn->psn_AttachmentList);

        /* fill out psn_Header */
    {
        AudioTagHeader * const athd = psn->psn_Header;
        const TagArg *srcTag, *tstate;
        TagArg *dstTag = athd->athd_Tags;

        athd->athd_NodeType = nodeType;
        athd->athd_NumTags  = numTags;

        for (tstate = tagList; srcTag = NextTagArg (&tstate); ) {
            switch (srcTag->ta_Tag) {
                case AF_TAG_ADDRESS:
                    body = (void *)srcTag->ta_Arg;
                    break;

                case AF_TAG_AUTO_FREE_DATA:
                    break;

                default:
                    *dstTag++ = *srcTag;
                    break;
            }
        }
        dstTag->ta_Tag = TAG_END;
    }

        /* success: install psn_Body, add to list */
    psn->psn_Body = body;
    AddTail (&pi->pi_SlaveList, (Node *)psn);
    return 0;

error_msg:
    MSERR((msp, errcode, NULL));
error_quiet:
    DeletePatchSlaveNode (psn);
    return errcode;
}

/*
    Delete PatchSlaveNode
*/
static void DeletePatchSlaveNode (PatchSlaveNode *psn)
{
    if (psn) {
            /* free attachments */
        {
            PatchAttachmentNode *n, *succ;

            PROCESSLIST (&psn->psn_AttachmentList, n, succ, PatchAttachmentNode) {
                DeletePatchAttachmentNode (n);
            }
        }

            /* free body and self */
        FreeMem (psn->psn_Body, TRACKED_SIZE);
        FreeMem (psn, TRACKED_SIZE);
    }
}

/*
    Find PatchSlaveNode by name

    Arguments
        pi
            PatchInfo containing pi_SlaveList to scan.

        slaveName
            Attached item symbol name to match case-insensitively.

    Results
        Returns pointer to PatchSlaveNode on success, or NULL if not found.
*/
static PatchSlaveNode *FindPatchSlave (PatchInfo *pi, const char *slaveName)
{
    return (PatchSlaveNode *)FindNamedNode (&pi->pi_SlaveList, slaveName);
}


/*
    Find PatchSlaveNode by name and type

    Arguments
        pi
            PatchInfo containing pi_SlaveList to scan.

        slaveName
            Attached item symbol name to match case-insensitively.

        nodeType
            AUDIO_<type>_NODE which the found PatchSlaveNode must match for
            success.

    Results
        Returns pointer to PatchSlaveNode on success, or NULL if not found or
        type doesn't match.
*/
static PatchSlaveNode *FindPatchSlaveType (PatchInfo *pi, const char *slaveName, uint8 nodeType)
{
    PatchSlaveNode * const psn = FindPatchSlave (pi, slaveName);

    return psn && psn->psn_Header->athd_NodeType == nodeType ? psn : NULL;
}


/*
    Add a PatchAttachmentNode to the specified PatchSlaveNode

    Arguments
        psn
            PatchSlaveNode containing psn_AttachmentList to add to.

        hookName
            Hook name. Assumed to be short enough to fit.

        tagList
            Optional attachment tags. Ignores all control tags.

    Results
        0 on success, Err code on failure.
*/
/* @@@ not static because the VA glue needs to find it */
Err AddPatchAttachment (PatchSlaveNode *psn, const char *hookName, const TagArg *tagList)
{
    PatchAttachmentNode *pan;
    const int32 numTags = GetTagCount (tagList) + 1;    /* non-trivial tag count + space for TAG_END */
    Err errcode;

        /* alloc/init PatchAttachmentNode w/ variable-sized embedded PatchAttachment */
    if (!(pan = AllocMem (offsetof(PatchAttachmentNode,pan_Att) + PatchAttachmentSize(numTags), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) {
        errcode = MAKEERR (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
        goto clean;
    }
    strcpy (pan->pan_Att.patt_HookName, hookName);
    pan->pan_Att.patt_NumTags = numTags;
    CopyTagList (pan->pan_Att.patt_Tags, tagList);

        /* success */
    AddTail (&psn->psn_AttachmentList, (Node *)pan);
    return 0;

clean:
    DeletePatchAttachmentNode (pan);
    return errcode;
}


/* -------------------- Binary patch file writer */

/*
    Writes FORM 3PCH file

    Arguments
        fileName
            Name of file to write. If an error occurs, and it has successfully
            opened the file, the file is deleted.

        pi
            PatchInfo to write.
*/
static Err WritePatch (const char *fileName, const PatchInfo *pi)
{
    IFFParser *iff = NULL;
    bool created = FALSE;
    Err errcode;

  #if DEBUG_Writer
    printf ("WritePatch: '%s' pi=0x%x\n", fileName, pi);
  #endif

    if ((errcode = CreateIFFParserVA (&iff, TRUE, IFF_TAG_FILE, fileName, TAG_END)) < 0) goto clean;
    created = TRUE;     /* set this to indicate that a failure after here delete file */

        /* FORM 3PCH */
    {
        if ((errcode = PushChunk (iff, ID_3PCH, ID_FORM, IFF_SIZE_UNKNOWN_32)) < 0) goto clean;

            /* FORM PCMD */
        {
            if ((errcode = PushChunk (iff, ID_PCMD, ID_FORM, IFF_SIZE_UNKNOWN_32)) < 0) goto clean;

                /* PTMP and PMIX chunks */
            {
                PackedID curID = 0;
                const Node *n;

                #define SETCONTEXT(newID) \
                    if (curID != (newID)) { \
                        if (curID) if ((errcode = PopChunk (iff)) < 0) goto clean; \
                        if ((errcode = PushChunk (iff, ID_PCMD, newID, IFF_SIZE_UNKNOWN_32)) < 0) goto clean; \
                        curID = (newID); \
                    }

                SCANLIST (&pi->pi_TemplateList, n, Node) {
                    switch (n->n_Type) {
                        case PTMP_TYPE_INS:
                                /* append to PTMP as a Packed String Array element */
                            {
                                const PatchInsTemplateNode * const pitn = (PatchInsTemplateNode *)n;

                                SETCONTEXT (ID_PTMP);
                                if ((errcode = WriteChunk (iff, pitn->pitn.n_Name, PackedStringSize(pitn->pitn.n_Name))) < 0) goto clean;
                            }
                            break;

                        case PTMP_TYPE_MIXER:
                                /* append to MixerSpec to PMIX */
                            {
                                const PatchMixerTemplateNode * const pmtn = (PatchMixerTemplateNode *)n;

                                SETCONTEXT (ID_PMIX);
                                if ((errcode = WriteChunk (iff, &pmtn->pmtn_MixerSpec, sizeof (MixerSpec))) < 0) goto clean;
                            }
                            break;
                    }
                }
                    /* pop PMIX or PTMP chunk under construction, if there is one */
                if (curID) {
                    if ((errcode = PopChunk(iff)) < 0) goto clean;
                }

                #undef SETCONTEXT
            }

                /* PNAM chunk */
            {
                const PatchNameNode *pnn;

                if ((errcode = PushChunk (iff, ID_PCMD, ID_PNAM, pi->pi_TotalNameSize)) < 0) goto clean;
                SCANLIST (&pi->pi_NameList, pnn, PatchNameNode) {
                    if ((errcode = WriteChunk (iff, pnn->pnn.n_Name, PackedStringSize(pnn->pnn.n_Name))) < 0) goto clean;
                }
                if ((errcode = PopChunk (iff)) < 0) goto clean;
            }

                /* PCMD chunk */
            {
                const PatchCmd *pc, *pcstate;

                if ((errcode = PushChunk (iff, ID_PCMD, ID_PCMD, IFF_SIZE_UNKNOWN_32)) < 0) goto clean;

                for (pcstate = GetPatchCmdList (pi->pi_PatchCmdBuilder); pc = NextPatchCmd (&pcstate); ) {
                    if ((errcode = WriteChunk (iff, pc, sizeof *pc)) < 0) goto clean;
                }
                {
                    static const PatchCmdGeneric endpc = { PATCH_CMD_END };

                    if ((errcode = WriteChunk (iff, &endpc, sizeof endpc)) < 0) goto clean;
                }
                if ((errcode = PopChunk (iff)) < 0) goto clean;
            }

            if ((errcode = PopChunk (iff)) < 0) goto clean;
        }

            /* FORM PATTs */
        {
            const PatchSlaveNode *psn;

            SCANLIST (&pi->pi_SlaveList, psn, PatchSlaveNode) {

                if ((errcode = PushChunk (iff, ID_PATT, ID_FORM, IFF_SIZE_UNKNOWN_32)) < 0) goto clean;

                    /* PATT chunks */
                {
                    const PatchAttachmentNode *pan;

                    SCANLIST (&psn->psn_AttachmentList, pan, PatchAttachmentNode) {
                        if ((errcode = WriteLeafChunk (iff, ID_PATT, &pan->pan_Att, PatchAttachmentSize (pan->pan_Att.patt_NumTags))) < 0) goto clean;
                    }
                }

                    /* ATAG */
                if ((errcode = WriteLeafChunk (iff, ID_ATAG, psn->psn_Header, AudioTagHeaderSize(psn->psn_Header->athd_NumTags))) < 0) goto clean;

                    /* BODY */
                if (psn->psn_Body) {
                    if ((errcode = WriteLeafChunk (iff, ID_BODY, psn->psn_Body, GetMemTrackSize(psn->psn_Body))) < 0) goto clean;
                }

                if ((errcode = PopChunk (iff)) < 0) goto clean;
            }
        }

        /* @@@ FORM PTUN writer would go here */

        if ((errcode = PopChunk (iff)) < 0) goto clean;
    }

clean:
        /* close file */
    DeleteIFFParser (iff);

        /* delete file if there was an error while writing it */
    if (errcode < 0) {
        if (created) DeleteFile (fileName);
    }

    return errcode;
}


/* -------------------- debug */

#if DEBUG_Dump

static const char *LookupPatchNameKey (const PatchInfo *, int32 matchKey);

static void DumpPatchInfo (const PatchInfo *pi, const char *banner)
{
    if (banner) printf ("%s\n", banner);

    printf ("\nTemplates:\n");
    {
        const Node *n;
        int32 i = 0;

        SCANLIST (&pi->pi_TemplateList, n, Node) {
            printf ("%4d: ", i++);

            switch (n->n_Type) {
                case PTMP_TYPE_INS:
                    {
                        const PatchInsTemplateNode * const pitn = (PatchInsTemplateNode *)n;

                        printf ("template '%s'\n", pitn->pitn.n_Name);
                    }
                    break;

                case PTMP_TYPE_MIXER:
                    {
                        const PatchMixerTemplateNode * const pmtn = (PatchMixerTemplateNode *)n;

                        printf ("mixer %dx%d flags=0x%04x\n",
                            MixerSpecToNumIn(pmtn->pmtn_MixerSpec),
                            MixerSpecToNumOut(pmtn->pmtn_MixerSpec),
                            MixerSpecToFlags(pmtn->pmtn_MixerSpec));
                    }
                    break;

                default:
                    printf ("type=%d name='%s'\n", n->n_Type, n->n_Name);
                    break;
            }
        }
    }

    printf ("\nNames:\n");
    {
        const PatchNameNode *pnn;

        SCANLIST (&pi->pi_NameList, pnn, PatchNameNode) {
            printf ("%4d: '%s'\n", pnn->pnn_Key, pnn->pnn.n_Name);
        }
    }

    printf ("\nPatchCmds:\n");
    {
        const PatchCmd *pci, *pcstate;

        #define FIXUP_NAME(fld) (fld) = LookupPatchNameKey (pi, (int32)(fld))

        for (pcstate = GetPatchCmdList (pi->pi_PatchCmdBuilder); pci = NextPatchCmd (&pcstate); ) {
            PatchCmd pc = *pci;

            switch (pc.pc_Generic.pc_CmdID) {
                case PATCH_CMD_ADD_TEMPLATE:
                    FIXUP_NAME (pc.pc_AddTemplate.pc_BlockName);
                    break;

                case PATCH_CMD_DEFINE_PORT:
                    FIXUP_NAME (pc.pc_DefinePort.pc_PortName);
                    break;

                case PATCH_CMD_DEFINE_KNOB:
                    FIXUP_NAME (pc.pc_DefineKnob.pc_KnobName);
                    break;

                case PATCH_CMD_EXPOSE:
                    FIXUP_NAME (pc.pc_Expose.pc_PortName);
                    FIXUP_NAME (pc.pc_Expose.pc_SrcBlockName);
                    FIXUP_NAME (pc.pc_Expose.pc_SrcPortName);
                    break;

                case PATCH_CMD_CONNECT:
                    FIXUP_NAME (pc.pc_Connect.pc_FromBlockName);
                    FIXUP_NAME (pc.pc_Connect.pc_FromPortName);
                    FIXUP_NAME (pc.pc_Connect.pc_ToBlockName);
                    FIXUP_NAME (pc.pc_Connect.pc_ToPortName);
                    break;

                case PATCH_CMD_SET_CONSTANT:
                    FIXUP_NAME (pc.pc_SetConstant.pc_BlockName);
                    FIXUP_NAME (pc.pc_SetConstant.pc_PortName);
                    break;
            }

            DumpPatchCmd (&pc, NULL);
        }

        #undef FIXUP_NAME
    }

    printf ("\nAttached Items:\n");
    {
        const PatchSlaveNode *psn;

        SCANLIST (&pi->pi_SlaveList, psn, PatchSlaveNode) {
            const PatchAttachmentNode *pan;

            printf ("\nItem '%s' type=%d  %d tags (including TAG_END) body=0x%x, %d bytes  ",
                psn->psn.n_Name,
                psn->psn_Header->athd_NodeType,
                psn->psn_Header->athd_NumTags,
                psn->psn_Body,
                psn->psn_Body ? GetMemTrackSize(psn->psn_Body) : 0);
            DumpTagList (psn->psn_Header->athd_Tags, NULL);

            SCANLIST (&psn->psn_AttachmentList, pan, PatchAttachmentNode) {
                printf ("Attachment '%s' %d tags (including TAG_END)  ", pan->pan_Att.patt_HookName, pan->pan_Att.patt_NumTags);
                DumpTagList (pan->pan_Att.patt_Tags, NULL);
            }
        }
    }

    /* @@@ dump tuning information here */

    printf ("\n");
}

/*
    Finds a PatchNameNode by key.
*/
static const char *LookupPatchNameKey (const PatchInfo *pi, int32 matchKey)
{
    PatchNameNode *pnn;

    if (!matchKey) return NULL;

    SCANLIST (&pi->pi_NameList, pnn, PatchNameNode) {
        if (pnn->pnn_Key == matchKey) return pnn->pnn.n_Name;
    }

    return "*** bad name key";
}

#endif


/* -------------------- Utility */

/*
    Count non-control tags in a tag list.

    Arguments
        tagList
            Tag list to scan. NULL is equivalent to an empty tag list.

    Results
        Returns the number of non-control tags in tagList (e.g., 0 for an empty
        list, or completely trivial list).
*/
static int32 GetTagCount (const TagArg *tagList)
{
    int32 numTags = 0;

    while (NextTagArg (&tagList)) numTags++;

    return numTags;
}

/*
    Copy tag list skipping over control tags

    Arguments
        dstTag
            Destination TagArg Array. Must be big enough to hold all
            non-control tags in srcTagList plus a terminating TAG_END.

        srcTagList
            Source tag list to copy.
*/
static void CopyTagList (TagArg *dstTag, const TagArg *srcTagList)
{
    const TagArg *srcTag;

    while (srcTag = NextTagArg (&srcTagList)) *dstTag++ = *srcTag;
    dstTag->ta_Tag = TAG_END;
}
