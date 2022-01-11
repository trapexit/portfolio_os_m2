/******************************************************************************
**
**  @(#) dspp_newloader.c 96/07/31 1.8
**
**  DSPP Template loader (IFF folio client)
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  960202 WJB  Created
**  960222 WJB  Now using FindFileAndIdentify() to locate .dsp files.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/handy_macros.h> /* MIN() */
#include <audio/music_iff.h>    /* RegisterDataChunks() */
#include <file/filefunctions.h> /* FindFileAndIdentify() */
#include <file/filesystem.h>
#include <misc/iff.h>
#include <string.h>

#include "audio_internal.h"
#include "dspp.h"               /* self */


/* -------------------- debug */

#define DEBUG_Find         0    /* debug FindDotDSPFile() */
#define DEBUG_DumpTemplate 0    /* enable to dump template after loading */


/* -------------------- dsppLoadInsTemplate () */

static Err FindDotDSPFile (char *pathBuf, int32 pathBufSize, const char *insName);
static Err ParseDotDSPFile (IFFParser **resultIFFParser, const char *insName);
static Err BuildDSPPTemplate (DSPPTemplate **resultDTmp, IFFParser *);
static int32 ParrotCB (IFFParser *, int32 result);

/*
    Load a .dsp file. Return user-mode DSPPTemplate.

    Arguments
        resultUserDTmp
            Location to write pointer to resulting User mode DSPPTemplate.

        insName
            .dsp instrument name (e.g., sawtooth.dsp) to load

    Results
        0 on success, Err code on failure
        Writes pointer to allocated DSPPTemplate to *resultUserDTmp on success.
        Writes NULL to *resultUserDTmp on failure.

    @@@ opens and closes the IFF folio
*/
Err dsppLoadInsTemplate (DSPPTemplate **resultUserDTmp, const char *insName)
{
    IFFParser *iff = NULL;
    Err errcode;

        /* init result */
    *resultUserDTmp = NULL;

#if 0
    /* this call is not needed because the IFF folio is currently
       permanently bound to the audio folio. */

        /* open IFF folio */
        /* note: separate failure path to avoid calling unopened IFF folio */
    if ((errcode = OpenIFFFolio()) < 0) return errcode;
#endif

        /* parse .dsp file; returns IFFParser */
    if ((errcode = ParseDotDSPFile (&iff, insName)) < 0) goto clean;

        /* build resulting DSPPTemplate */
    if ((errcode = BuildDSPPTemplate (resultUserDTmp, iff)) < 0) goto clean;

  #if DEBUG_DumpTemplate
    #define DFID_NANOKERNEL 26
    if ((*resultUserDTmp)->dtmp_Header.dhdr_FunctionID != DFID_NANOKERNEL) {
        char b[80];

        sprintf (b, "dsppLoadInsTemplate: '%s' dtmp=0x%x", insName, *resultUserDTmp);
        dsppDumpTemplate (*resultUserDTmp, b);
    }
  #endif

clean:
    DeleteIFFParser (iff);
#if 0
    CloseIFFFolio();
#endif
    return errcode;
}


/*
    Open and parse .dsp file. Results in an IFFParser positioned to the first
    end-of-context of a FORM DSPP. This IFFParser may then be scanned for stored
    data chunks using ExtractDataChunk() and FindPropChunk().

    It is the caller's responsibility to clean up the resulting IFFParser,
    regardless of degree of success of this function, by calling
    DeleteIFFParser(). Because of the use of RegisterDataChunks(), any
    un-extracted data chunks will automatically be freed by DeleteIFFParser().

    This parser takes advantage of the fact that RegisterDataChunk() does
    MEMTYPE_TRACKSIZE allocations. This permits simply moving the buffered
    chunks to the user-mode DSPPTemplate under construction w/o a special
    handler or an alloc/copy of each buffered chunk.

    Arguments
        resultIFFParser
            Pointer to variable to write allocated IFFParser. Assumed to be initialized
            to NULL by caller.

        insName
            Name of .dsp instrument to parse (e.g., sawtooth.dsp).

    Results
        0 on success, Err code on failure.

        On successf, *resultIFFParser is left positioned at end of FORM DSPP
        context, with its property and data chunks awaiting extraction.

        On failure, the *resultIFFParser may be created, or may be left NULL.
*/
static Err ParseDotDSPFile (IFFParser **resultIFFParser, const char *insName)
{
    static const IFFTypeID propChunks[] = {
        { ID_DSPP, ID_DHDR },
        0
    };
    static const IFFTypeID dataChunks[] = {
        { ID_DSPP, ID_DCOD },
        { ID_DSPP, ID_DINI },
        { ID_DSPP, ID_DLNK },
        { ID_DSPP, ID_DNMS },
        { ID_DSPP, ID_DRLC },
        { ID_DSPP, ID_MRSC },
        0
    };
    char insPathName [FILESYSTEM_MAX_PATH_LEN];
    IFFParser *iff;
    Err errcode;

        /* Find .dsp file, returns path name in insPathName */
        /* (FindDotDSPFile() prints ERR() messages) */
    if ((errcode = FindDotDSPFile (insPathName, sizeof insPathName, insName)) < 0) goto clean;

        /* Create IFF parser */
    if ((errcode = CreateIFFParserVA (&iff, FALSE, IFF_TAG_FILE, insPathName, TAG_END)) < 0) goto clean;
    *resultIFFParser = iff;

        /* Register FORM DSPP chunks */
    if ((errcode = RegisterPropChunks (iff, propChunks)) < 0) goto clean;
    if ((errcode = RegisterDataChunks (iff, dataChunks)) < 0) goto clean;

        /* Cause ParseIFF() to return 0 on exit of FORM DSPP */
    if ((errcode = InstallExitHandler (iff, ID_DSPP, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ParrotCB, (void *)0)) < 0) goto clean;

        /* Parse file. Returns >=0 only after our exit handler gets called. */
    if ((errcode = ParseIFF (iff, IFF_PARSE_SCAN)) < 0) {

            /* translate error codes */
        switch (errcode) {
            case IFF_PARSE_EOF:
                    /* ParseIFF() returns this if it gets to the logical end of
                    ** file without finding a FORM DSPP.
                    */
                ERR(("dsppLoadInsTemplate: file '%s' not a DSP instrument\n", insName));
                errcode = AF_ERR_BADFILETYPE;
                break;
        }
        goto clean;
    }

        /* success */
    return 0;

clean:
    return errcode;
}


/*
    Finds newest available .dsp file in System.m2/Audio/dsp/ using
    FindFileAndIdentify().

    Arguments
        pathBuf
            A buffer into which the call may return the complete (absolute)
            pathname of the file which was found.

        pathBufSize
            The size of the pathBuf buffer.

        insName
            Simple file name of .dsp file to find (e.g., sawtooth.dsp)

    Results
        0 on success, Err code on failure.
        Writes to *pathBuf on success. *pathBuf unchanged on failure.
*/
static Err FindDotDSPFile (char *pathBuf, int32 pathBufSize, const char *insName)
{
    static const TagArg fileSearchTags[] = {
        { FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData)DONT_SEARCH_UNBLESSED },
        /* @@@ could search for matching version instead of newest if we need to:
        **     { FILESEARCH_TAG_VERSION_EQ, PORTFOLIO_OS_VERSION } */
        TAG_END
    };
    static const char insDir[] = "System.m2/Audio/dsp/";
    char insTempPath [sizeof(insDir)-1 + AF_MAX_NAME_LENGTH + 1];
    Err errcode;

        /* Build temp path name for .dsp file */
    if (strlen(insName) > AF_MAX_NAME_LENGTH) {
        ERR(("dsppLoadInsTemplate: instrument name '%s' too long\n", insName));
        return AF_ERR_NAME_TOO_LONG;
    }
    strcpy (insTempPath, insDir);
    strcat (insTempPath, insName);

        /* Find file */
    if ((errcode = FindFileAndIdentify (pathBuf, pathBufSize, insTempPath, fileSearchTags)) < 0) {
        ERR(("dsppLoadInsTemplate: instrument '%s' not found\n", insName));
        return errcode;
    }

  #if DEBUG_Find
    printf ("dsppLoadInsTemplate: search for '%s'\n"
            "                          found '%s'\n", insTempPath, pathBuf);
  #endif

    return 0;
}


/*
    Extract stuff from IFFParser to build DSPPTemplate

    Arguments
        resultDTmp
            pointer to location to write resulting DSPPTemplate pointer.
            Assumes already init'd to NULL, so doesn't write anything on failure.

        iff
            IFFParser positioned at exit of FORM DSPP

    Results
        0 on success, Err code on failure.
        Writes to *resultDTmp on success, nothing written on failure.
*/
static Err BuildDSPPTemplate (DSPPTemplate **resultDTmp, IFFParser *iff)
{
    DSPPTemplate *dtmp;
    Err errcode;

        /* Create user mode DSPPTemplate */
    if (!(dtmp = dsppCreateUserTemplate())) {
        errcode = AF_ERR_NOMEM;
        goto clean;
    }

        /* DHDR required prop chunk -> dtmp_Header */
    {
        const PropChunk * const pc = FindPropChunk (iff, ID_DSPP, ID_DHDR);

        if (!pc) {
            errcode = AF_ERR_BADOFX;
            goto clean;
        }
        memcpy (&dtmp->dtmp_Header, pc->pc_Data, MIN(sizeof dtmp->dtmp_Header, pc->pc_DataSize));
    }

        /* extract data chunks */
        /* @@@ dsppValidateTemplate() traps anything that's missing */
    {
        void *data;

            /* MRSC -> dtmp_Resources */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_MRSC)) {
            dtmp->dtmp_Resources    = (DSPPResource *)data;
            dtmp->dtmp_NumResources = GetMemTrackSize (data) / sizeof (DSPPResource);
        }

            /* DNMS -> dtmp_ResourceNames */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_DNMS)) {
            dtmp->dtmp_ResourceNames = (char *)data;
        }

            /* DRLC -> dtmp_Relocations */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_DRLC)) {
            dtmp->dtmp_Relocations    = (DSPPRelocation *)data;
            dtmp->dtmp_NumRelocations = GetMemTrackSize (data) / sizeof (DSPPRelocation);
        }

            /* DCOD -> dtmp_Codes */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_DCOD)) {
            dtmp->dtmp_Codes    = (DSPPCodeHeader *)data;
            dtmp->dtmp_CodeSize = GetMemTrackSize (data);
        }

            /* DINI -> dtmp_DataInitializer */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_DINI)) {
            dtmp->dtmp_DataInitializer     = (DSPPDataInitializer *)data;
            dtmp->dtmp_DataInitializerSize = GetMemTrackSize (data);
        }

            /* DLNK -> dtmp_DynamicLinkNames */
        if (data = ExtractDataChunk (iff, ID_DSPP, ID_DLNK)) {
            dtmp->dtmp_DynamicLinkNames     = (char *)data;
            dtmp->dtmp_DynamicLinkNamesSize = GetMemTrackSize (data);
        }
    }

        /* success: write to *resultDTmp */
    *resultDTmp = dtmp;
    return 0;

clean:
    dsppDeleteUserTemplate (dtmp);
    return errcode;
}


/*
    IFFCallBack which returns as its result whatever it's passed as user data.
*/
static int32 ParrotCB (IFFParser *iff, int32 result)
{
    TOUCH(iff);

    return result;
}
