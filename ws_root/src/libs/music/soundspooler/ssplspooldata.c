/* @(#) ssplspooldata.c 96/02/23 1.2 */

#include <audio/soundspooler.h> /* self */

/**
 |||	AUTODOC -public -class libmusic -group SoundSpooler -name ssplSpoolData
 |||	Sends a block full of data. (convenience function)
 |||
 |||	  Synopsis
 |||
 |||	    int32 ssplSpoolData ( SoundSpooler *sspl, char *Data,
 |||	                          int32 NumBytes, void *UserData )
 |||
 |||	  Description
 |||
 |||	    ssplSpoolData() is a convenience function that does the following
 |||	    operations:
 |||
 |||	    - Requests an available SoundBufferNode, returns 0 if none are available.
 |||
 |||	    - Attaches sample data and optional user data pointer to the
 |||	    SoundBufferNode.
 |||
 |||	    - Submits the SoundBufferNode to the active queue.
 |||
 |||	  Arguments
 |||
 |||	    sspl
 |||	        Pointer to a SoundSpooler structure.
 |||
 |||	    Data
 |||	        Pointer to the buffer of data.
 |||
 |||	    NumBytes
 |||	        Number of bytes of data to send.
 |||
 |||	    UserData
 |||	        Pointer to user data, or NULL.
 |||
 |||	  Return Value
 |||
 |||	    Positive value on success indicating the signal to be sent when the buffer
 |||	    finishes playing, zero if no SoundBufferNodes are available, or negative
 |||	    3DO error code for others kinds of failure.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in libmusic.a V21.
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
 |||	    ssplPlayData(), ssplRequestBuffer(), ssplSendBuffer(),
 |||	    ssplSetBufferAddressLength(), ssplSetUserData()
**/

int32 ssplSpoolData (SoundSpooler *sspl, char *Data, int32 NumBytes, void *UserData)
{
    SoundBufferNode *sbn;
    int32 result = 0;

        /* Request a buffer, return if one isn't available */
    if ((sbn = ssplRequestBuffer (sspl)) == NULL) goto clean;

        /*
            Set address and length of buffer.
            Return buffer to spooler on failure.
        */
    if ((result = ssplSetBufferAddressLength (sspl, sbn, Data, NumBytes)) < 0) {
        ssplUnrequestBuffer (sspl, sbn);
        goto clean;
    }

        /* Set User Data (can't fail). */
    ssplSetUserData( sspl, sbn, UserData );

        /* Send that buffer off to be played. */
    return ssplSendBuffer (sspl, sbn);

clean:
    return result;
}
