#ifndef __AUDIO_PATCHFILE_H
#define __AUDIO_PATCHFILE_H


/******************************************************************************
**
**  @(#) patchfile.h 96/03/01 1.11
**
**  Patch File Definitions and Loader
**
******************************************************************************/

/*
    FORM 3PCH {


        Patch Compiler Input - This FORM represents the PatchCmd list to be
        given to the patch compiler. There must be exactly 1 FORM PCMD in the
        FORM 3PCH (not counting nesting).

        FORM PCMD {

            Constituent Template list for Patch. Any number and order of these
            chunks/FORMs. The resulting templates are referenced by index by the
            PCMD chunk.

            PTMP {
                PackedStringArray   // packed string array of .dsp template names
            }
            PMIX {
                MixerSpec[]         // array of MixerSpecs
            }
            FORM 3PCH {             // nested patch
                .
                .
                .
            }


            Packed string array of all strings required by PatchCmd list.
            Indexed by keys stored in PCMD chunk. There must be one of these in
            the FORM PCMD.

            PNAM {
                PackedStrings
            }


            Array of file-conditioned PatchCmds terminated by a PATCH_CMD_END:

            - char * are stored as uint32 byte offset into PNAM chunk + 1, NULL is 0.

            - InsTemplate items are stored as index into List of templates defined
              stored in PTMP, PMIX, and nested FORM 3PCHs in order of appearance
              in the FORM PCMD.

            PCMD {
                PatchCmd[]
            }
        }


        Tuning - Defines a custom tuning for the the Patch Template. Either 0
        or 1 of these FORMs (@@@ not yet supported)

        FORM PTUN {
            ATAG {
                AudioTagHeader AUDIO_TUNING_NODE
            }
            BODY {
                float32 TuningData[]
            }
        }


        Attachments - Each FORM PATT defines a slave Item and attachments for
        that Item. There may be any number of these FORMs in a FORM 3PCH. If
        present, they must appear after the FORM PCMD.

        Each PATT chunk within a FORM PATT defines one attachment between the
        slave defined by the FORM PATT and the patch. There must be at least
        one PATT chunk in each FORM PATT.

        FORM PATT {
            PATT {
                PatchAttachment
            }
            PATT {
                PatchAttachment
            }
            .
            .
            .
            ATAG {
                AudioTagHeader
            }
            BODY {
                data
            }
        }

    }
*/

#ifndef __AUDIO_AUDIO_H
#include <audio/audio.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __MISC_IFF_H
#include <misc/iff.h>
#endif

#ifndef __STDDEF_H
#include <stddef.h>     /* offsetof() */
#endif


/* -------------------- IDs and structures */

    /* main FORM ID */
#define ID_3PCH MAKE_ID('3','P','C','H')

    /* FORM PCMD - stored PatchCmd list */
#define ID_PCMD MAKE_ID('P','C','M','D')
#define ID_PMIX MAKE_ID('P','M','I','X')
#define ID_PNAM MAKE_ID('P','N','A','M')
#define ID_PTMP MAKE_ID('P','T','M','P')

    /* FORM PATT - ATAG object (e.g., sample, or envelope) plus attachment descriptions */
#define ID_PATT MAKE_ID('P','A','T','T')

    /* PATT.PATT collection chunk */
typedef struct PatchAttachment {
    char    patt_HookName[AF_MAX_NAME_SIZE];    /* '\0' terminated hook name. */
    uint32  patt_Reserved0;         /* room for future expansion. Must be 0 for now. */
    uint16  patt_Reserved1;
    uint16  patt_NumTags;           /* number of tags in tag array (including TAG_END) */
    TagArg  patt_Tags[1];           /* Attachment options. Really patt_Tags[patt_NumTags].
                                       Audio TagArg array terminated by TAG_END. Not
                                       allowed to contain AF_TAG_MASTER, AF_TAG_SLAVE,
                                       AF_TAG_NAME, TAG_JUMP, or early TAG_END. */
} PatchAttachment;

#define PatchAttachmentSize(numTags) (offsetof (PatchAttachment, patt_Tags) + (numTags) * sizeof (TagArg))

    /* FORM PTUN - ATAG Tuning for patch */
#define ID_PTUN MAKE_ID('P','T','U','N')


/* -------------------- AudioPatchFile folio error codes */

#define MakeAudioPatchFileErr(svr,class,err)    MakeErr(ER_FOLI,ER_AUDIOPATCHFILE,svr,ER_E_SSTM,class,err)

    /* Standard error codes */
#define PATCHFILE_ERR_NOMEM         MakeAudioPatchFileErr(ER_SEVERE,ER_C_STND,ER_NoMem)

    /* Invalid file type (e.g. not FORM 3PCH) */
#define PATCHFILE_ERR_BAD_TYPE      MakeAudioPatchFileErr(ER_SEVERE,ER_C_NSTND,1)

    /* Data in file is corrupt */
#define PATCHFILE_ERR_MANGLED       MakeAudioPatchFileErr(ER_SEVERE,ER_C_NSTND,2)


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* AudioPatchFile Folio */
Err OpenAudioPatchFileFolio (void);
Err CloseAudioPatchFileFolio (void);

    /* simple patch file loader */
Item LoadPatchTemplate (const char *fileName);
#define UnloadPatchTemplate(patchTemplate) DeleteItem(patchTemplate)

    /* FORM 3PCH parser */
Err EnterForm3PCH (IFFParser *, const char *patchName);
Item ExitForm3PCH (IFFParser *, void *dummy);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_PATCHFILE_H */
