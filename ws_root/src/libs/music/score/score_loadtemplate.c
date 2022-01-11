/******************************************************************************
**
**  @(#) score_loadtemplate.c 96/03/01 1.8
**
**  Automagic Template loader portion of score player.
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
**  951023 WJB  Created.
**  951024 WJB  Removed AIFF support.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/audio.h>            /* LoadInsTemplate() */
#include <audio/patchfile.h>        /* LoadPatchTemplate() */
#include <audio/score.h>            /* self */
#include <string.h>


/* -------------------- Debug */

#define DEBUG_Load  0       /* debug loader */

#if DEBUG_Load
#include <stdio.h>
#endif


/* -------------------- LoadScoreTemplate() */

static Item internalLoadPatchTemplate (const char *fileName);

 /**
 |||	AUTODOC -public -class libmusic -group Score -name UnloadScoreTemplate
 |||	Unloads a Template(@) loaded by LoadScoreTemplate().
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadScoreTemplate (Item insTemplate)
 |||
 |||	  Description
 |||
 |||	    This macro deletes an instrument Template loaded by LoadScoreTemplate().
 |||
 |||	  Arguments
 |||
 |||	    insTemplate
 |||	        Instrument Template Item to unload.
 |||
 |||	  Return Value
 |||
 |||	    Returns a non-negative value if successful, or an error code (negative
 |||	    value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/score.h> V29.
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenAudioFolio()
 |||
 |||	  Associated Files
 |||
 |||	    <audio/score.h>, System.m2/Modules/audio
 |||
 |||	  See Also
 |||
 |||	    LoadScoreTemplate()
 **/

 /**
 |||	AUTODOC -public -class libmusic -group Score -name LoadScoreTemplate
 |||	Loads instrument or patch Template(@) depending on file name extension.
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadScoreTemplate (const char *fileName)
 |||
 |||	  Description
 |||
 |||	    This function examines the fileName extension to determine which of the
 |||	    following ways to load and return an Instrument Template:
 |||
 |||	    .dsp
 |||	        LoadInsTemplate()
 |||
 |||	    .patch
 |||	        LoadPatchTemplate()
 |||
 |||	    The score player uses this, which explains its grouping and name, but it can
 |||	    be used independently from the score player.
 |||
 |||	    You may use UnloadScoreTemplate() to dispose of the Template when done with
 |||	    it.
 |||
 |||	  Arguments
 |||
 |||	    fileName
 |||	        The name of the DSP Instrument or Patch Template to load. In the case of
 |||	        standard DSP Instrument Templates (e.g., sawtooth.dsp), there should be
 |||	        no directory component to the file name. Full path names are supported
 |||	        for patch file names.
 |||
 |||	  Return Value
 |||
 |||	    Returns a Template Item number (non-negative value) if successful, or an
 |||	    error code (negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in libmusic.a V29.
 |||
 |||	  Notes -preformatted
 |||
 |||	    This function is equivalent to:
 |||
 |||	    Item LoadScoreTemplate (const char *fileName)
 |||	    {
 |||	        const char * const suffix = strrchr (fileName, '.');
 |||
 |||	        return (suffix && !strcasecmp(suffix,".patch"))
 |||	            ? internalLoadPatchTemplate (fileName)
 |||	            : LoadInsTemplate (fileName, NULL);
 |||	    }
 |||
 |||	    static Item internalLoadPatchTemplate (const char *fileName)
 |||	    {
 |||	        Item result;
 |||
 |||	        if ((result = OpenAudioPatchFileFolio()) >= 0) {
 |||	            result = LoadPatchTemplate (fileName);
 |||	            CloseAudioPatchFileFolio();
 |||	        }
 |||
 |||	        return result;
 |||	    }
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenAudioFolio()
 |||
 |||	  Associated Files
 |||
 |||	    <audio/score.h>, libmusic.a, System.m2/Modules/audio,
 |||	    System.m2/Modules/audiopatchfile
 |||
 |||	  See Also
 |||
 |||	    LoadInsTemplate(), LoadPatchTemplate(), UnloadScoreTemplate(), Template(@)
 **/
/* !!! add TagArgs? */
Item LoadScoreTemplate (const char *fileName)
{
    const char * const suffix = strrchr (fileName, '.');

  #if DEBUG_Load
    printf ("LoadScoreTemplate: name='%s' suffix='%s'\n", fileName, suffix);
  #endif

    if (suffix) {
            /* is it a patch? */
        if (!strcasecmp (suffix, ".patch")) return internalLoadPatchTemplate (fileName);
    }

        /* default to handing off to audio folio */
    return LoadInsTemplate (fileName, NULL);
}

static Item internalLoadPatchTemplate (const char *fileName)
{
    Item result;

    if ((result = OpenAudioPatchFileFolio()) >= 0) {
        result = LoadPatchTemplate (fileName);
        CloseAudioPatchFileFolio();
    }

    return result;
}


#if 0       /* @@@ not used, but it might be handy someday */

/* -------------------- LoadSampleTemplate() */

/*
    Load Sample, load and attach to suitable sample player

    Arguments
        sampleFileName
            Path name of sample file to load.

    Results
        Item number of Template with sample attached, or error code.
*/
static Item LoadSampleTemplate (const char *sampleName)
{
    Item sample;
    Item sampleInsTemplate = -1;
    char sampleInsName[AF_MAX_NAME_SIZE];
    Err errcode;

        /* load sample */
    if ((errcode = sample = LoadSample (sampleName)) < 0) goto clean;

        /* get suitable instrument name */
    if ((errcode = SampleItemToInsName (sample, TRUE, sampleInsName, sizeof sampleInsName)) < 0) {
        PrintError (NULL, "find player for", sampleName, errcode);
        goto clean;
    }
  #if DEBUG_Load
    printf ("LoadScoreTemplate: Use instrument '%s' for '%s'\n", sampleInsName, sampleName);
  #endif

        /* load instrument */
    if ((errcode = sampleInsTemplate = LoadInsTemplate (sampleInsName, NULL)) < 0) {
        PrintError (NULL, "load instrument", sampleInsName, errcode);
        goto clean;
    }

        /* attach sample */
    if ((errcode = CreateAttachmentVA (sampleInsTemplate, sample,
        AF_TAG_SET_FLAGS,         AF_ATTF_FATLADYSINGS,
        AF_TAG_AUTO_DELETE_SLAVE, TRUE,
        TAG_END)) < 0) goto clean;

    return sampleInsTemplate;

clean:
    UnloadInsTemplate (sampleInsTemplate);
    UnloadSample (sample);
    return errcode;
}

#endif
