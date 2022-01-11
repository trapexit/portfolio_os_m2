#ifndef __AUDIO_DSPP_TEMPLATE_H
#define __AUDIO_DSPP_TEMPLATE_H


/****************************************************************************
**
**  @(#) dspp_template.h 96/09/19 1.5
**
**  Audio Folio DSPPTemplate private structures and services. These structures
**  relate directly to the .dsp (FORM DSPP) template files.
**
**  @@@ Notes:
**
**  1.  This group of structures matches the contents of the .dsp file
**      so they cannot be changed without changing the file format.
**
**  2.  The patch compiler also has very intimate knowledge of these
**      structures and their characteristics with different types (e.g.
**      application of drsc_SubType and drsc_Default depending on drsc_Type).
**      Be sure to consider the patch compiler, and update it if necessary
**      when making changes to these structures.
**
**  3.  Both the audio and audoipatch folios communicate with one
**      another using these structures, and the two folios are not written
**      in such a way as to cope with size changes of most of these
**      structures. Non-compatible structure changes (e.g., size change,
**      replacement of pad field with something requiring a non-zero default,
**      etc) require that both the audio and audiopatch folios be upgraded
**      together. However, there is currently no way to ensure that the audio
**      and audiopatch folios are of the same version.
**
**      Because only DSPPTemplate is allocated exclusively by the audio folio, it
**      is the only structure which can grow in size and still retain backwards
**      compatibility (as long as the new fields can safely be ignored by the
**      patch compiler). The others must remain the same size.
**
**      !!! we should fix this for MX.
**
****************************************************************************/


#ifdef EXTERNAL_RELEASE
#error This file may not be used in externally released source code.
#endif


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* -------------------- Misc defines */

    /* Limits on part count (16-bit) (part num range is therefore 0..0xfffe) */
#define AF_PART_COUNT_MIN   1
#define AF_PART_COUNT_MAX   0xffff


/* -------------------- DSPPHeader */

    /* Template Header */
typedef struct DSPPHeader  /* DHDR chunk. @@@ must match FORM DSPP file format. */
{
    int32   dhdr_FunctionID;        /* used to identify functionality if name changed */
    int32   dhdr_SiliconVersion;    /* ASIC version instrument was assembled for (DSPP_SILICON_) */
    int32   dhdr_FormatVersion;     /* Instrument format version, for compatibility. */
    int32   dhdr_Flags;
} DSPPHeader;

    /* Special DSP function IDs defined in src/folios/audio/dsp/function_ids.j */
#define DFID_TEST   (1) /* test instrument, only allowed in development mode */
#define DFID_PATCH  (2) /* patch constructed by ARIA or patch builder */
#define DFID_MIXER  (3) /* mixer constructed by CreateMixerTemplate() */
#define DFID_FIRST_LOADABLE  (10) /* first ID of regular loadable instrument */

    /* DSPP silicon versions (@@@ must match dspp_asm.fth) */
#define DSPP_SILICON_BLUE    1
#define DSPP_SILICON_RED     2
#define DSPP_SILICON_GREEN   3
#define DSPP_SILICON_ANVIL   4
#define DSPP_SILICON_BULLDOG 5

    /* dhdr_FormatVersion */
#define DHDR_SUPPORTED_FORMAT_VERSION   (4)

    /* Flags for DSPPHeader.dhdr_Flags */
#define DHDR_F_PRIVILEGED     (0x0001)
#define DHDR_F_SHARED         (0x0002)
#define DHDR_F_PATCHES_ONLY   (0x0004)  /* Cannot be made into an instrument. */


/* -------------------- DSPPResource */

typedef struct DSPPResource /* DRSC chunk. @@@ must match FORM DSPP file format. */
{
    uint8   drsc_Reserved0;
    uint8   drsc_SubType;           /* Resource type-specific subtype (see below) */
    uint8   drsc_Flags;             /* DRSC_F_ flags */
    uint8   drsc_Type;              /* DRSC_TYPE_ resource type */

    int32   drsc_Many;              /* Amount of resource to request. Valid for allocations
                                     * and bindings. It is ignored for imported resources
                                     * except to make sure that is a legal value (i.e. > 0)
                                     * !!! should become uint16
                                     */

    int32   drsc_Allocated;         /* Allocated resource (e.g. IMEM location, FIFO channel, etc)
                                     *
                                     * In DSPPTemplate's DSPPResource array, this is treated
                                     * as a part to bind to when DRSC_F_BIND is set. It is unused
                                     * otherwise.
                                     * !!! will eventually become uint16 drsc_BindToPart.
                                     *
                                     * In DSPPInstrument's DSPPResource array, this is the
                                     * allocated resource. It is set to DRSC_ALLOC_INVALID
                                     * when first initialized (a value < 0). When a resource
                                     * is allocated, the resulting value is stored here (a
                                     * value >= 0). This value is reset to DRSC_ALLOC_INVALID
                                     * when the resource is freed. (@@@ this should remain int32
                                     * in order to store DRSC_ALLOC_INVALID).
                                     */

    int32   drsc_BindToRsrcIndex;   /* Index of resource in this template's resource array to
                                     * which this one is bound when DRSC_F_BIND is set. Unused
                                     * otherwise. Note that the resource to which this one is
                                     * bound must precede this one in the array and must have
                                     * DRSC_F_BOUND_TO set. !!! Should become uint16.
                                     */

    int32  drsc_Default;            /* Default value to write to all allocated words of this resource.
                                     * at times specified by DRSC_F_INIT_AT_ flags. Unused otherwise.
                                     * int32 because it can hold both signed or unsigned 16-bit quantities.
                                     */
/* @@@ if you add to this structure, you must add it to copy code in dsppOpenResource()
       (!!! this rule will be relaxed when we move allocated resources to a new structure) */
} DSPPResource;

    /* DSPPResource.drsc_Flags */
#define DRSC_F_IMPORT           0x80    /* resource is to be imported instead of allocated */
#define DRSC_F_EXPORT           0x40    /* resource is to be made public after allocating */
#define DRSC_F_BIND             0x20    /* resource is bound to another allocated resource pointed to by
                                           rsrc[drsc_BindToRsrcIndex].drsc_Allocated + drsc_Allocated. */
#define DRSC_F_BOUND_TO         0x10    /* some other resource is bound to this one. */
#define DRSC_F_INIT_AT_START    0x02    /* Initialize all parts with drsc_Default when started. Matches DINI_F_AT_START */
#define DRSC_F_INIT_AT_ALLOC    0x01    /* Initialize all parts with drsc_Default when allocated. Matches DINI_F_AT_ALLOC */

    /* DSPPResource.drsc_Types */
enum DSPPResourceTypes      /* @@@ These must match dspp_asm.fth */
{
    DRSC_TYPE_CODE,
    DRSC_TYPE_KNOB,
    DRSC_TYPE_VARIABLE,
    DRSC_TYPE_INPUT,
    DRSC_TYPE_OUTPUT,
    DRSC_TYPE_IN_FIFO,
    DRSC_TYPE_OUT_FIFO,
    DRSC_TYPE_TICKS,
    DRSC_TYPE_RBASE,
    DRSC_TYPE_TRIGGER,
    DRSC_TYPE_HW_DAC_OUTPUT,
    DRSC_TYPE_HW_OUTPUT_STATUS,
    DRSC_TYPE_HW_OUTPUT_CONTROL,
    DRSC_TYPE_HW_ADC_INPUT,
    DRSC_TYPE_HW_INPUT_STATUS,
    DRSC_TYPE_HW_INPUT_CONTROL,
    DRSC_TYPE_HW_NOISE,
    DRSC_TYPE_HW_CPU_INT,
    DRSC_TYPE_HW_CLOCK,             /* Down counter for benchmarking instruments. */
    DSPP_NUM_RSRC_TYPES             /* number of resources */
};

/*
    drsc_SubType (type-specific subtype) usage

    DRSC_TYPE_KNOB
    DRSC_TYPE_INPUT
    DRSC_TYPE_OUTPUT
        AF_SIGNAL_TYPE_ signal type.

    DRSC_TYPE_IN_FIFO
        DRSC_INFIFO_SUBTYPE_ FIFO hardware decompression type.

    Others
        Unused. Should be 0.
*/

/* Set decompression types for input FIFOs.  Values for drsc_SubType */
enum DSPPInFIFOTypes    /* @@@ These must match dspp_asm.fth */
{
    DRSC_INFIFO_SUBTYPE_16BIT,
    DRSC_INFIFO_SUBTYPE_8BIT,
    DRSC_INFIFO_SUBTYPE_SQS2
};


/* -------------------- DSPPRelocation */

    /* Relocation record */
typedef struct DSPPRelocation  /* DRLC chunk. @@@ must match FORM DSPP file format. */
{
    uint16  drlc_RsrcIndex;         /* Resource index to apply */
    uint16  drlc_Part;              /* part of resource (including attributes like FIFOSTATUS) */
    uint8   drlc_Reserved;
    uint8   drlc_CodeHunk;          /* Code hunk from DCOD containing fixup list.
                                       @@@ multiple code hunks are currently not supported */
    uint16  drlc_CodeOffset;        /* Offset within code hunk of first fixup in list.
                                       Subsequent fixups are pointed to by relative
                                       offset stored template's code image. 0 terminates. */
} DSPPRelocation;

    /* Special drlc_Part values for FIFOs */
#define DRSC_FIFO_NORMAL 0
#define DRSC_FIFO_STATUS 1      /* fifo status register */
#define DRSC_FIFO_READ   2      /* read fifo data w/o bumping it */
#define DRSC_FIFO_OSC    3      /* base of oscillator register set (M2) */


/* -------------------- DSPPCodeHeader */

    /* Code Hunk header */
typedef struct DSPPCodeHeader  /* DCOD chunk. @@@ Warning - must match FORM DSPP file format. */
{
    int32   dcod_Type;              /* !!! not used */
    int32   dcod_Offset;            /* byte offset in chunk beginning of code image */
    int32   dcod_Size;              /* number of uint16 words in hunk */
} DSPPCodeHeader;

    /* dcod_Type (!!! not actually used) */
#define DCOD_RUN_DSPP  0
#define DCOD_INIT_DSPP 1
#define DCOD_ARGS      2

    /* void *dsppGetCodeHunkImage (DSPPCodeHeader *codeheaders, int32 hunknum) - find code image for hunk */
#define dsppGetCodeHunkImage(codeheaders,hunknum) ((void *)((uint8 *)(codeheaders) + (codeheaders)[hunknum].dcod_Offset))


/* -------------------- DSPPDataInitializer */

    /* Data initializer */
typedef struct DSPPDataInitializer  /* DINI chunk. @@@ must match FORM DSPP file format. */
{
    int32   dini_Many;              /* How many values, <= drsc_Many */
    int32   dini_Flags;             /* DINI_F_ flags */
    int32   dini_RsrcIndex;         /* Index of entry in Resource Chunk */
    int32   dini_Reserved;          /* Set to zero. */
} DSPPDataInitializer;

    /* DSPPDataInitialzier dini_Flags */
#define DINI_F_AT_ALLOC (DRSC_F_INIT_AT_ALLOC)
#define DINI_F_AT_START (DRSC_F_INIT_AT_START)

    /* int32 *dsppGetDataInitializerImage (const DSPPDataInitializer *dini) - return pointer to initializer image */
#define dsppGetDataInitializerImage(dini) ( (int32 *)((dini)+1) )

    /* DSPPDataInitializer *dsppNextDataInitializer (const DSPPDataInitializer *dini)
       - return pointer to next DSPPDataInitializer. returns pointer past end of chunk
         when called with last DSPPDataInitializer in chunk. */
#define dsppNextDataInitializer(dini)  ( (DSPPDataInitializer *) ( dsppGetDataInitializerImage(dini) + (dini)->dini_Many ) )

    /* size_t dsppDataInitializerSize (int32 many)
       - return size of DSPPDataInitializer structure and data (in bytes) for specified
         number of data entries. */
#define dsppDataInitializerSize(many)  ( sizeof (DSPPDataInitializer) + (many) * sizeof (int32) )


/* -------------------- DSPPTemplate */

    /* @@@ Be sure to update dsppCloneTemplate() (dspp_template.c) if the DSPPTemplate structure changes */
typedef struct DSPPTemplate
{
    int32           dtmp_ShareCount;            /* This can be shared. Last one frees it.
                                                   One less than the number of AudioInsTemplates
                                                   referring to it */
    int32           dtmp_NumResources;
    DSPPResource   *dtmp_Resources;
    char           *dtmp_ResourceNames;
    int32           dtmp_NumRelocations;
    DSPPRelocation *dtmp_Relocations;
    int32           dtmp_CodeSize;
    DSPPCodeHeader *dtmp_Codes;
    int32           dtmp_DataInitializerSize;
    DSPPDataInitializer *dtmp_DataInitializer;
    int32           dtmp_DynamicLinkNamesSize;  /* Size of chunk including NUL terminator. */
    char           *dtmp_DynamicLinkNames;
    DSPPHeader      dtmp_Header;                /* copied from DHDR chunk (950131) */
} DSPPTemplate;


/* -------------------- Envelope resources */

typedef struct DSPPEnvHookRsrcInfo {
        /* resource indeces of envelope resources */
    uint16  deri_RequestRsrcIndex;
    uint16  deri_IncrRsrcIndex;
    uint16  deri_TargetRsrcIndex;
    uint16  deri_CurrentRsrcIndex;
} DSPPEnvHookRsrcInfo;

    /* envelope resource suffixes */
#define ENV_SUFFIX_REQUEST  ".request"
#define ENV_SUFFIX_INCR     ".incr"
#define ENV_SUFFIX_TARGET   ".target"
#define ENV_SUFFIX_CURRENT  ".current"

    /* name length restrictions */
#define ENV_MAX_SUFFIX_LENGTH   (sizeof ENV_SUFFIX_REQUEST - 1)                 /* @@@ assumes ENV_SUFFIX_REQUEST is longest */
#define ENV_MAX_NAME_LENGTH     (AF_MAX_NAME_LENGTH - ENV_MAX_SUFFIX_LENGTH)    /* longest envelope hook name allowed */

    /* envelope resource types */
#define ENV_DRSC_TYPE_REQUEST   DRSC_TYPE_KNOB
#define ENV_DRSC_TYPE_INCR      DRSC_TYPE_KNOB
#define ENV_DRSC_TYPE_TARGET    DRSC_TYPE_VARIABLE
#define ENV_DRSC_TYPE_CURRENT   DRSC_TYPE_VARIABLE


/* -------------------- Signal conversion */
/* (!!! not sure about putting this one here) */

    /* Convert between Floats and DSPP 1.15 numbers. */
#define ConvertFP_SF15(x)   ((int32)((x) * 32768.0))
#define ConvertFP_UF15(x)   ((uint32)((x) * 32768.0))
#define ConvertF15_FP(x)    ((x) / 32768.0)

    /* Clipping */
int32 dsppClipRawValue (int32 signalType, int32 rawValue);


/* -------------------- Misc Functions */

    /* user DSPPTemplate create/delete */
DSPPTemplate *dsppCreateUserTemplate (void);
void dsppDeleteUserTemplate (DSPPTemplate *);

    /* Item lookup */
DSPPTemplate *dsppLookupTemplate (Item insTemplate);

    /* resource lookup */
const char *dsppGetTemplateRsrcName (const DSPPTemplate *, int32 rsrcIndex);
int32 dsppFindResourceIndex (const DSPPTemplate *, const char *rsrcName);
Err dsppFindEnvHookResources (DSPPEnvHookRsrcInfo *info, uint32 infoSize, const DSPPTemplate *, const char *envHookName);

    /* relocator */
typedef void (*DSPPFixupFunction) (void *fixupData, uint16 offset, uint16 value);
void dsppFixupCodeImage (uint16 *codeImage, uint16 offset, uint16 value);
void dsppRelocate (const DSPPTemplate *, const DSPPRelocation *, uint16 value, DSPPFixupFunction fixupFunc, void *fixupData);

    /* debug (libaudioprivate.a) */
void dsppDumpTemplate (const DSPPTemplate *, const char *banner);
void dsppDumpResource (const DSPPTemplate *, int32 rsrcIndex);
void dsppDumpRelocation (const DSPPRelocation *);
void dsppDumpDataInitializer (const DSPPDataInitializer *);


/*****************************************************************************/


#endif /* __AUDIO_DSPP_TEMPLATE_H */
