/* @(#) sspldumpsoundspooler.c 96/02/23 1.2 */

#include <audio/soundspooler.h>
#include <stdio.h>

static void DumpSoundBufferList (const List *sbnlist);

/**
 |||	AUTODOC -public -class libmusic -group SoundSpooler -name ssplDumpSoundSpooler
 |||	Print debug info for SoundSpooler.
 |||
 |||	  Synopsis
 |||
 |||	    void ssplDumpSoundSpooler ( const SoundSpooler *sspl )
 |||
 |||	  Description
 |||
 |||	    Prints debugging information for SoundSpooler including the contents of
 |||	    both the active and free queues (calls ssplDumpSoundBufferNode() for each
 |||	    SoundBufferNode).
 |||
 |||	  Arguments
 |||
 |||	    sspl
 |||	        SoundSpooler to print.
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
 |||	    ssplDumpSoundBufferNode()
**/

void ssplDumpSoundSpooler( const SoundSpooler *sspl )
{
	printf("SoundSpooler 0x%08lx: nbufs=%ld sigs=0x%08lx\n", sspl, sspl->sspl_NumBuffers, sspl->sspl_SignalMask);

	DumpSoundBufferList (&sspl->sspl_ActiveBuffers);
	DumpSoundBufferList (&sspl->sspl_RequestedBuffers);
	DumpSoundBufferList (&sspl->sspl_FreeBuffers);
}

/* dump a list of SBN's */
static void DumpSoundBufferList (const List *sbnlist)
{
	const SoundBufferNode *sbn;

	SCANLIST (sbnlist, sbn, SoundBufferNode)
	{
		ssplDumpSoundBufferNode( sbn );
	}
}
