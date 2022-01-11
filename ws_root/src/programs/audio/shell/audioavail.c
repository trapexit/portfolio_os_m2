/******************************************************************************
**
**  @(#) audioavail.c 96/04/25 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class Shell_Commands -group Audio -name audioavail
|||	Prints audio resource usage to debug terminal.
|||
|||	  Format
|||
|||	    audioavail
|||
|||	  Description
|||
|||	    Prints available audio resources to the debug console.
|||
|||	  Implementation
|||
|||	    Command implemented in V30.
|||
|||	  Location
|||
|||	    System.m2/Programs/audioavail
|||
|||	  See Also
|||
|||	    audiomon(@), GetAudioResourceInfo()
**/

#include <audio/audio.h>
#include <stdio.h>

int main (void)
{
    static const char * const rsrcDesc[AF_RESOURCE_TYPE_MANY] = {   /* @@@ depends on AF_RESOURCE_TYPE_ order */
        "Ticks / Batch",
        "Code Memory Words",
        "Data Memory Words",
        "FIFOs",
        "Triggers",
    };
    Err errcode;
    int32 i;

    if ((errcode = OpenAudioFolio()) < 0) {
        PrintError (NULL, "open audio folio", NULL, errcode);
        goto clean;
    }

    printf ("Resource             Avail   In-Use    Total  Largest\n");
    /*       xxxxxxxxxxxxxxxxx    xxxxx    xxxxx    xxxxx    xxxxx  */
    for (i=0; i<AF_RESOURCE_TYPE_MANY; i++) {
        AudioResourceInfo info;

        if ((errcode = GetAudioResourceInfo (&info, sizeof info, i)) >= 0) {
            printf ("%-17s%9d%9d%9d%9d\n", rsrcDesc[i], info.rinfo_Free, info.rinfo_Total - info.rinfo_Free, info.rinfo_Total, info.rinfo_LargestFreeSpan);
        }
        else {
            printf ("%-17s error 0x%x\n", rsrcDesc[i], errcode);
        }
    }

clean:
    TOUCH(errcode);     /* silence warning for not using errcode */
    CloseAudioFolio();
    return 0;
}
