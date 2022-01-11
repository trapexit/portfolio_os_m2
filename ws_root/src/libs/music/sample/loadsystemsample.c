/* @(#) loadsystemsample.c 96/02/23 1.2 */

#include <audio/musicerror.h>
#include <audio/parse_aiff.h>       /* self */
#include <file/filefunctions.h>     /* FindFileAndIdentify() */
#include <file/filesystem.h>
#include <kernel/mem.h>
#include <string.h>


#define DEBUG_Find  0   /* debug FindDotDSPFile() */

#if DEBUG_Find
#include <stdio.h>
#endif


static Err FindSystemSample (char *pathBuf, int32 pathBufSize, const char *fileName);

/**
|||	AUTODOC -public -class libmusic -group Sample -name LoadSystemSample
|||	Loads a system Sample(@) file.
|||
|||	  Synopsis
|||
|||	    Item LoadSystemSample (const char *fileName)
|||
|||	  Description
|||
|||	    This procedure locates and loads a standard system sample file (e.g.,
|||	    sinewave.aiff). These are samples that are part of the operating system
|||	    to provide simple 'beep' type sounds. Since they are located in System.m2
|||	    they are accessed using FindFileAndIdentify().
|||
|||	    Use UnloadSample() or DeleteItem() to delete the sample item when you are
|||	    finished with it.
|||
|||	  Arguments
|||
|||	    fileName
|||	        File name of standard system sample file (e.g., sinewave.aiff).
|||
|||	  Return Value
|||
|||	    The procedure returns an item number of the Sample if successful (a
|||	    non-negative value) or an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V30.
|||
|||	  Notes -preformatted
|||
|||	    This function is equivalent to:
|||
|||	    Item LoadSystemSample (const char *fileName)
|||	    {
|||	        char pathName [FILESYSTEM_MAX_PATH_LEN];
|||	        Item result;
|||
|||	            // find file
|||	        if ((result = FindSystemSample (pathName, sizeof pathName, fileName)) < 0) return result;
|||
|||	            // attempt to load it
|||	        return LoadSample (pathName);
|||	    }
|||
|||	    static Err FindSystemSample (char *pathBuf, int32 pathBufSize, const char *fileName)
|||	    {
|||	        static const TagArg fileSearchTags[] = {
|||	            { FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData)DONT_SEARCH_UNBLESSED },
|||	            TAG_END
|||	        };
|||	        static const char sampleDir[] = "System.m2/Audio/aiff/";
|||	        char *tempPath;
|||	        Err errcode;
|||
|||	            // allocate temp path name
|||	        if (!(tempPath = AllocMem (sizeof sampleDir - 1 + strlen (fileName) + 1, MEMTYPE_TRACKSIZE))) {
|||	            errcode = ML_ERR_NOMEM;
|||	            goto clean;
|||	        }
|||	        strcpy (tempPath, sampleDir);
|||	        strcat (tempPath, fileName);
|||
|||	            // find file
|||	        errcode = FindFileAndIdentify (pathBuf, pathBufSize, tempPath, fileSearchTags);
|||
|||	    clean:
|||	        FreeMem (tempPath, TRACKED_SIZE);
|||	        return errcode;
|||	    }
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, libspmath.a, System.m2/Modules/audio,
|||	    System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    LoadSample(), UnloadSample(), Sample(@), CreateAttachment()
**/
Item LoadSystemSample (const char *fileName)
{
    char pathName [FILESYSTEM_MAX_PATH_LEN];
    Item result;

        /* find file */
    if ((result = FindSystemSample (pathName, sizeof pathName, fileName)) < 0) return result;

        /* attempt to load it */
    return LoadSample (pathName);
}

/*
    Find newest version of a sample file in System.m2/Audio/aiff using
    FindFileAndIdentify().

    Arguments
        pathBuf
            A buffer into which the call may return the complete (absolute)
            path name of the file which was found.

        pathBufSize
            The size of the pathBuf buffer.

        fileName
            Simple file name of .aiff file to find (e.g., sinewave.aiff)

    Results
        Non-negative value on success, Err code on failure.
        Writes to *pathBuf on success. *pathBuf unchanged on failure.
*/
static Err FindSystemSample (char *pathBuf, int32 pathBufSize, const char *fileName)
{
    static const TagArg fileSearchTags[] = {
        { FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData)DONT_SEARCH_UNBLESSED },
        TAG_END
    };
    static const char sampleDir[] = "System.m2/Audio/aiff/";
    char *tempPath;
    Err errcode;

        /* allocate temp path name */
    if (!(tempPath = AllocMem (sizeof sampleDir - 1 + strlen (fileName) + 1, MEMTYPE_TRACKSIZE))) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    strcpy (tempPath, sampleDir);
    strcat (tempPath, fileName);

        /* find file */
    errcode = FindFileAndIdentify (pathBuf, pathBufSize, tempPath, fileSearchTags);

  #if DEBUG_Find
    if (errcode >= 0) {
        printf ("LoadSystemSample: search for '%s'\n"
                "                       found '%s'\n", tempPath, pathBuf);
    }
  #endif

clean:
    FreeMem (tempPath, TRACKED_SIZE);
    return errcode;
}
