/* @(#) spdumpplayer.c 96/02/23 1.1 */

#include <stdio.h>

#include "soundplayer_internal.h"

 /**
 |||	AUTODOC -public -class libmusic -group SoundPlayer -name spDumpPlayer
 |||	Print debug information for sound player
 |||
 |||	  Synopsis
 |||
 |||	    void spDumpPlayer (const SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Prints out a bunch of debug information for an SPPlayer including:
 |||
 |||	    - list of the SPPlayer's SPSounds
 |||
 |||	    - list of each SPSound's SPMarkers
 |||
 |||	    - cross reference of static branches between markers
 |||
 |||	  Arguments
 |||
 |||	    player
 |||	        Pointer to SPPlayer to print.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libmusic.a V24.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/soundplayer.h>, libmusic.a
 |||
 |||	  See Also
 |||
 |||	    spCreatePlayer()
 **/
void spDumpPlayer (const SPPlayer *player)
{
    const SPSound *sound;
    const SPMarker *marker;
    const MinNode *refnode;

    printf ("SPPlayer $%08lx width=%lu channels=%lu compression=%lu alignment=%lu frames %lu bytes. xref:\n", player, player->sp_SampleFrameInfo.spfi_Width, player->sp_SampleFrameInfo.spfi_Channels, player->sp_SampleFrameInfo.spfi_CompressionRatio, player->sp_SampleFrameInfo.spfi_AlignmentFrames, player->sp_SampleFrameInfo.spfi_AlignmentBytes);

    SCANLIST (&player->sp_Sounds, sound, SPSound) {
        printf ("  SPSound $%08lx: numframes=%ld numbytes=%ld class=$%08lx\n", sound, sound->spso_NumFrames, spCvtFrameToByte(&sound->spso_SampleFrameInfo,sound->spso_NumFrames), sound->spso_ClassDefinition);

      #if 0
            /* done this way to avoid debug code being a method, since that would cause the debug code to
               always be linked in */
        if (sound->spso_ClassDefinition == &sp_soundFileClass) {
            printf ("  fileitem=%ld offset=%ld blocksize=%ld\n",
                ((SPSoundFile *)sound)->spsf_File, ((SPSoundFile *)sound)->spsf_DataOffset, ((SPSoundFile *)sound)->spsf_BlockSize );
        }
        else if (sound->spso_ClassDefinition == &sp_soundSampleClass) {
            printf ("  address=$%08lx\n", ((SPSoundSample *)sound)->spss_DataAddress);
        }
      #endif

        SCANLIST (&sound->spso_Markers, marker, SPMarker) {
            printf ("    SPMarker $%08lx: pos=%lu tomarker=$%08lx decision=$%08lx,$%08lx name='%s'\n", marker, marker->spmk_Position, marker->spmk_BranchToMarker, marker->spmk_DecisionFunction, marker->spmk_DecisionData, marker->spmk.n_Name);

            SCANLIST (&marker->spmk_BranchRefList, refnode, MinNode) {
                const SPMarker *refmarker = GetMarkerFromBranchRefNode (refnode);
                printf ("      ref: marker=$%08lx sound=$%08lx\n", refmarker, refmarker->spmk_Sound);
            }
        }
    }
}
