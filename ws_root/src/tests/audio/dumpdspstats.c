/*
**  @(#) dumpdspstats.c 96/05/16 1.2
**
**  List resource usage for all DSP instruments
*/

#include <audio/audio.h>
#include <file/directory.h>
#include <file/directoryfunctions.h>
#include <file/filefunctions.h>     /* FindFileAndIdentify() */
#include <file/filesystem.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <string.h>

static bool ConsiderDirEntry (const DirectoryEntry *);
static Err OpenSystemDirectory (Directory **resultDir, const char *sysPath);

int main (void)
{
    Err errcode;
    Directory *dir = NULL;

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
#ifndef BUILD_STRINGS
		TOUCH(errcode);
#endif
        goto clean;
    }

        /* open dsp directory */
    if ((errcode = OpenSystemDirectory (&dir, "System.m2/Audio/dsp")) < 0) {
        PrintError (NULL, "open DSP directory", NULL, errcode);
#ifndef BUILD_STRINGS
		TOUCH(errcode);
#endif
        goto clean;
    }

        /* scan directory, dump results */
    {
        DirectoryEntry dirEntry;
        InstrumentResourceInfo rinfo;
        int32 i;

        printf (
            "\n"
            "                                   Code       Data\n"
            "Instrument Name           Ticks  Each Ovhd  Each Ovhd  FIFOs  Trig.\n"
            "------------------------  -----  ---- ----  ---- ----  -----  -----\n"
        );

        while ((errcode = ReadDirectory (dir, &dirEntry)) >= 0) if (ConsiderDirEntry (&dirEntry)) {
            Item insTemplate;

            if ((insTemplate = LoadInsTemplate (dirEntry.de_FileName, NULL)) >= 0) {
                printf ("%-24s", dirEntry.de_FileName);

                for (i=0; i<AF_RESOURCE_TYPE_MANY; i++) {
                    memset (&rinfo, 0, sizeof rinfo);
                    GetInstrumentResourceInfo (&rinfo, sizeof rinfo, insTemplate, i);

                    switch (i) {
                        case AF_RESOURCE_TYPE_CODE_MEM:
                        case AF_RESOURCE_TYPE_DATA_MEM:
                            if (rinfo.rinfo_PerInstrument)
                                printf ("  %4d", rinfo.rinfo_PerInstrument);
                            else
                                printf ("      ");
                            if (rinfo.rinfo_MaxOverhead)
                                printf (" %4d", rinfo.rinfo_MaxOverhead);
                            else
                                printf ("     ");
                            break;

                        default:
                            if (rinfo.rinfo_PerInstrument)
                                printf ("  %4d ", rinfo.rinfo_PerInstrument);
                            else
                                printf ("       ");
                            break;
                    }
                }
                printf ("\n");

                UnloadInsTemplate (insTemplate);
            }
        }
        printf ("\n");

        if (errcode != FILE_ERR_NOFILE) PrintError (NULL, NULL, NULL, errcode);
    }

clean:
    CloseDirectory (dir);
    CloseAudioFolio();
    return 0;
}

static bool ConsiderDirEntry (const DirectoryEntry *dirEntry)
{
    const char * const name = dirEntry->de_FileName;
    const size_t nameLen = strlen (name);
    static const char dotDSPSuffix[] = ".dsp";
    static const char subPrefix[] = "sub_";

    return
            /* file? */
        !(dirEntry->de_Flags & FILE_IS_DIRECTORY) &&

            /* .dsp? */
        nameLen >= sizeof dotDSPSuffix-1 && !strcasecmp (&name[nameLen-(sizeof dotDSPSuffix-1)], dotDSPSuffix) &&

            /* not sub_? */
        strncasecmp (name, subPrefix, sizeof subPrefix-1) &&

            /* not nanokernel? */
        strcasecmp (name, "nanokernel.dsp");
}


static Err OpenSystemDirectory (Directory **resultDir, const char *sysPath)
{
    Err errcode;
    Item dirItem;
    Directory *dir = NULL;

        /* init result */
    *resultDir = NULL;

        /* open directory */
    if ((errcode = dirItem = FindFileAndOpenVA (sysPath,
        FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData)DONT_SEARCH_UNBLESSED,
        TAG_END)) < 0) goto clean;

    if (!(dir = OpenDirectoryItem (dirItem))) {
        errcode = FILE_ERR_NOTADIRECTORY;
        goto clean;
    }

        /* success */
    *resultDir = dir;
    return 0;

clean:
    CloseDirectory (dir);
    CloseFile (dirItem);
    return errcode;
}
