/* @(#) scanaiff.c 96/02/23 1.3 */

#include <audio/aiff_format.h>      /* self */
#include <audio/musicerror.h>
#include <misc/iff.h>

#include "music_internal.h"         /* version string */

MUSICLIB_PACKAGE_ID(aiff)


static int32 ParrotCB (IFFParser *iff, int32 result);

/**
|||	AUTODOC -private -class libmusic -group AIFF -name ScanAIFF
|||	Registers AIFF/AIFC chunks and parses file.
|||
|||	  Synopsis
|||
|||	    Err ScanAIFF (IFFParser **resultIFF, const char *fileName, bool loadData)
|||
|||	  Description
|||
|||	    This function creates an IFFParser, registers AIFF/AIFC chunks, and calls
|||	    ParseIFF(). When successful, the resulting IFFParser is positioned at the
|||	    end of the FORM AIFF/AIFC. At this point it contains the following
|||	    information:
|||
|||	    ID_COMM (required)
|||	        PropChunk containing an AIFFPackedCommon structure.
|||
|||	    ID_SSND (required)
|||	        SSNDInfo which can be found with FindSSNDInfo().
|||
|||	        Also a DataChunk containing a buffered copy of the sound data (minus the
|||	        AIFFSoundDataHeader and padding) if loadData is set. Extract this with
|||	        ExtractDataChunk().
|||
|||	    ID_INST (optional)
|||	        PropChunk containing an AIFFInstrument structure.
|||
|||	    ID_MARK (normally optional, but required if INST chunk has loops)
|||	        PropChunk containing an AIFFMarkerChunk structure.
|||
|||	    To determine what kind of FORM this is (AIFF or AIFC), look at the cn_Type
|||	    field of the current context (as returned by GetCurrentContext()).
|||
|||	    Call DeleteIFFParser() to dispose of resulting IFFParser when done with it.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser positioned at the entry of an SSND chunk. This is assumed to
|||	        be the case.
|||
|||	    storeData
|||	        TRUE to cause SSND data to be read and stored. FALSE to skip it.
|||
|||	  Return Value
|||
|||	    0 on success, Err code on failure. If this isn't a FORM AIFF/AIFC,
|||	    ML_ERR_INVALID_FILE_TYPE is returned.
|||
|||	    On success, writes an IFFParser pointer to *resultIFF. On failure,
|||	    *resultIFF is set to NULL.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
|||
|||	  Module Open Requirements
|||
|||	    OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>, libmusic.a, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    FindSSNDInfo(), ExtractDataChunk()
**/
Err ScanAIFF (IFFParser **resultIFF, const char *fileName, bool loadData)
{
    IFFParser *iff = NULL;
    Err errcode;

        /* init result */
    *resultIFF = NULL;

        /* Create IFF parser */
    if ((errcode = CreateIFFParserVA (&iff, FALSE, IFF_TAG_FILE, fileName, TAG_END)) < 0) goto clean;

        /* Register properties and handers for both AIFF and AIFC FORM types */
    {
        #define MAKE_AIFF_TYPE_ID(id) { ID_AIFF, (id) }, { ID_AIFC, (id) }
        static const IFFTypeID propChunks[] = {
            MAKE_AIFF_TYPE_ID (ID_COMM),    /* required */
            MAKE_AIFF_TYPE_ID (ID_MARK),    /* optional */
            MAKE_AIFF_TYPE_ID (ID_INST),    /* optional */
            0
        };
        #undef MAKE_AIFF_TYPE_ID

            /* Register FORM AIFF/AIFC chunks */
        if ((errcode = RegisterPropChunks (iff, propChunks)) < 0) goto clean;

            /* Install SSND entry handler (required chunk) - stores data size and offset in IFF context */
        if ((errcode = InstallEntryHandler (iff, ID_AIFF, ID_SSND, IFF_CIL_TOP, (IFFCallBack)HandleSSND, (void *)loadData)) < 0) goto clean;
        if ((errcode = InstallEntryHandler (iff, ID_AIFC, ID_SSND, IFF_CIL_TOP, (IFFCallBack)HandleSSND, (void *)loadData)) < 0) goto clean;

            /* Cause ParseIFF() to return 0 on exit of FORM AIFF/AIFC */
        if ((errcode = InstallExitHandler (iff, ID_AIFF, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ParrotCB, (void *)0)) < 0) goto clean;
        if ((errcode = InstallExitHandler (iff, ID_AIFC, ID_FORM, IFF_CIL_TOP, (IFFCallBack)ParrotCB, (void *)0)) < 0) goto clean;
    }

        /* Scan file, stop on exit from FORM AIFF or AIFC */
    if ((errcode = ParseIFF (iff, IFF_PARSE_SCAN)) < 0) {

            /* translate error codes */
        switch (errcode) {
            case IFF_PARSE_EOF:
                    /* ParseIFF() returns this if it gets to the logical end of
                    ** file without finding a FORM AIFF/AIFC
                    */
                errcode = ML_ERR_INVALID_FILE_TYPE;
                break;
        }
        goto clean;
    }

        /* success: set result */
    *resultIFF = iff;
    return 0;

clean:
    DeleteIFFParser (iff);
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
