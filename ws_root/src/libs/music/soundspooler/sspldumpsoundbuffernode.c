/* @(#) sspldumpsoundbuffernode.c 96/07/25 1.3 */

#include <audio/audio.h>
#include <audio/soundspooler.h>
#include <stdio.h>

/**
 |||	AUTODOC -public -class libmusic -group SoundSpooler -name ssplDumpSoundBufferNode
 |||	Print debug info for SoundBufferNode.
 |||
 |||	  Synopsis
 |||
 |||	    void ssplDumpSoundBufferNode ( const SoundBufferNode *sbn )
 |||
 |||	  Description
 |||
 |||	    Prints debugging information for a SoundBufferNode.
 |||
 |||	  Arguments
 |||
 |||	    sbn
 |||	        SoundBufferNode to print.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libmusic.a V21.
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenAudioFolio()
 |||
 |||	  Associated Files
 |||
 |||	    <audio/soundspooler.h>, libmusic.a, System.m2/Modules/audio
 |||
 |||	  See Also
 |||
 |||	    ssplDumpSoundSpooler()
**/

void ssplDumpSoundBufferNode( const SoundBufferNode *sbn )
{
	printf ("  SBN #%ld (0x%08lx): sig=0x%08x addr=0x%08x len=%ld",
		sbn->sbn_SequenceNum, sbn, sbn->sbn_Signal, sbn->sbn_Address, sbn->sbn_NumBytes);

#if 0   /* this isn't supported */
	{
		TagArg tags[] = {
			{ AF_TAG_STATUS, 0 },
			TAG_END
		};

		if (GetAudioItemInfo (sbn->sbn_Attachment, tags) >= 0)      /* this isn't necessarily supported */
			printf (" attstat=%ld", (int32)tags[0].ta_Arg);
	}
#endif

	printf ("\n");
}
