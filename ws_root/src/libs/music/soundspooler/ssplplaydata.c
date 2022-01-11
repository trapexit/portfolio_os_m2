/* @(#) ssplplaydata.c 96/02/23 1.2 */

#include <audio/soundspooler.h> /* self */
#include <kernel/task.h>

#define	PRT(x)	{ printf x; }
#define	DBUG(x)	/* PRT(x) */

/**
 |||	AUTODOC -public -class libmusic -group SoundSpooler -name ssplPlayData
 |||	Waits for next available buffer then sends a block full of data to the spooler.
 |||	(convenience function)
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplPlayData ( SoundSpooler *sspl, char *Data, int32 NumBytes )
 |||
 |||	  Description
 |||
 |||	    ssplPlayData() is a convenience function that does the following
 |||	    operations:
 |||
 |||	    - Requests an available SoundBufferNode. If none are available, waits for
 |||	    one to become available.
 |||
 |||	    - Attaches sample data to the SoundBufferNode.
 |||
 |||	    - Submits the SoundBufferNode to the active queue.
 |||
 |||	  Arguments
 |||
 |||	    sspl
 |||	        Pointer to a SoundSpooler structure.
 |||
 |||	    Data
 |||	        Pointer to data buffer.
 |||
 |||	    NumBytes
 |||	        Number of bytes to send.
 |||
 |||	  Return Value
 |||
 |||	    Positive value on success indicating the signal to be sent when the buffer
 |||	    finishes playing, or negative 3DO error code on failure. Never returns
 |||	    zero.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in libmusic.a V21.
 |||
 |||	  Notes
 |||
 |||	    This function calls ssplProcessSignals() when forced to wait for a
 |||	    SoundBufferNode, but doesn't have a mechanism for the client to supply a
 |||	    UserBufferProcessor callback function. If you use this function and
 |||	    require completion notification, use a SoundBufferFunc(@) instead of a
 |||	    UserBufferProcessor.
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
 |||	    ssplSpoolData(), ssplRequestBuffer(), ssplSendBuffer(),
 |||	    ssplSetBufferAddressLength(), ssplSetUserData(), ssplProcessSignals(),
 |||	    SoundBufferFunc(@)
**/

int32 ssplPlayData (SoundSpooler *sspl, char *Data, int32 NumBytes)
{
    SoundBufferNode *sbn;
    int32 result;

        /*
            Request a buffer. If no buffers are available,
            wait for some buffer signals to arrive and process
            them to free up a buffer.
        */
    while ((sbn = ssplRequestBuffer (sspl)) == NULL) {
        const int32 sigs = WaitSignal (sspl->sspl_SignalMask);

        DBUG(("ssplPlayBuffer: WaitSignal() got 0x%x\n", sigs));
        if ((result = ssplProcessSignals (sspl, sigs, NULL)) < 0) goto clean;
    }

        /*
            Set address and length of buffer.
            Return buffer to spooler on failure.
        */
    if ((result = ssplSetBufferAddressLength (sspl, sbn, Data, NumBytes)) < 0) {
        ssplUnrequestBuffer (sspl, sbn);
        goto clean;
    }

        /* Send that buffer off to be played. */
    return ssplSendBuffer (sspl, sbn);

clean:
    return result;
}
