/* @(#) autodocs.c 96/11/26 1.7 */

/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name MPAGetCompressedBfrFn
|||	MPEG Audio Decoder callback function: Called to get a compressed MPEG audio buffer.
|||
|||	  Synopsis
|||
|||	     typedef Err (*MPAGetCompressedBfrFn)(const void *theUnit,
|||	         const uint8 **buf, int32 *len, uint32 *pts,
|||	         uint32 *ptsIsValidFlag)
|||
|||	  Description
|||
|||	    To use the MPEG Audio Decoder you need to provide two callback
|||	    functions: MPAGetCompressedBfrFn() and MPACompressedBfrReadFn().
|||	    
|||	    The decoder will call MPAGetCompressedBfrFn() to get each compressed
|||	    data buffer. It passes in a "unit" pointer--a client data value from
|||	    MPAudioDecode(). The other arguments are output parameters used to
|||	    return a description of the next compressed audio buffer to the
|||	    decoder: its address, length, presentation timestamp (PTS) value, and
|||	    a boolean flag indicating if it has a PTS.
|||	    
|||	    See CreateMPAudioDecoder() for documentation on how to feed
|||	    presentation timestamps (PTSs) through the decoder to use to
|||	    synchronize the audio presentation with a video presentation.
|||
|||	  Arguments
|||
|||	    theUnit
|||	        This "unit" pointer is an arbitrary client data value passed
|||	        through the decoder from the client call to MPAudioDecode().
|||
|||	    buf
|||	        Output parameter. The callback function should return the next
|||	        compressed audio buffer's address by storing it into *buf.
|||
|||	    len
|||	        Output parameter. The callback function should return the next
|||	        compressed audio buffer's length, in bytes, by storing it into
|||	        *len.
|||
|||	    pts
|||	        Output parameter. The callback function should return the next
|||	        compressed audio buffer's presentation time stamp, if any, by
|||	        storing it into *pts.
|||
|||	    ptsIsValidFlag
|||	        Output parameter. The callback function should set *ptsIsValidFlag
|||	        to TRUE or FALSE to indicate whether the next compressed audio
|||	        buffer has a presentation time stamp.
|||
|||	  Return Value
|||	
|||	    0 for success (indicates a fresh buffer full of compressed data is ready
|||	    for processing), or any non-zero status which will be returned unchanged
|||	    to the caller of MPAudioDecode().  
|||	
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
|||	  See Also
|||
|||	    MPACompressedBfrReadFn(), MPAudioDecode()
|||
******************************************************************************/
/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name MPACompressedBfrReadFn
|||	MPEG Audio Decoder callback function: Called when done reading a compressed MPEG audio buffer.
|||
|||	  Synopsis
|||
|||	     typedef Err (*MPACompressedBfrReadFn)(const void *theUnit,
|||	         uint8 *buf)
|||
|||	  Description
|||
|||	    To use the MPEG Audio Decoder you need to provide two callback
|||	    functions: MPAGetCompressedBfrFn() and MPACompressedBfrReadFn().
|||	    
|||	    The decoder will call MPACompressedBfrReadFn() when it's finished
|||	    reading each compressed data buffer. It passes in a "unit" pointer
|||	    (client data value) and the address of the compressed data buffer.
|||
|||	  Arguments
|||
|||	    theUnit
|||	        This "unit" pointer is an arbitrary client data value passed
|||	        through the decoder from the client call to MPAudioDecode().
|||
|||	    buf
|||	        The address of the compressed data buffer that the decoder is
|||	        finished reading. (This could be NULL if a buffer was flushed.)
|||
|||	  Return Value
|||	
|||	    0 for success (indicates a fresh buffer full of compressed data is ready
|||	    for processing), or any non-zero status which will be returned unchanged
|||	    to the caller of MPAudioDecode().  
|||
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
|||	  See Also
|||
|||	    MPAGetCompressedBfrFn()
|||
******************************************************************************/
/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name CreateMPAudioDecoder
|||	Instantiate a new MPEG Audio decoder.
|||
|||	  Synopsis
|||
|||	    Err CreateMPAudioDecoder(MPADecoderContext **ctx,
|||	        MPACallbackFns CallbackFns)
|||
|||	  Description
|||
|||	    Allocate and initialize an MPEG audio decoder context structure.
|||	    
|||	    Using the returned context structure, you can decode MPEG audio data.
|||	    Your callback functions MPAGetCompressedBfrFn() and
|||	    MPACompressedBfrReadFn() feed compressed buffers into the decoder. You
|||	    call MPAudioDecode() to feed each output buffer into the decoder; it
|||	    returns with the buffer full of decompressed data.
|||	    
|||	    Each compressed input buffer can have an optional presentation
|||	    timestamp (PTS), which says when to present that data. Since the
|||	    compressed data can be sliced into packets (buffers) at any byte
|||	    boundary (the packets don't have to be frames), the client needs the
|||	    decoder to percolate PTSs through the pipeline and match them up to
|||	    the correct output frames. If a compressed input buffer has a PTS,
|||	    that gives the timestamp for the FIRST FRAME THAT BEGINS IN THAT
|||	    BUFFER. Each decompressed output buffer contains one frame, and will
|||	    have a PTS iff it was decompressed from a frame that had a PTS. The
|||	    decoder passes PTS values as opaque values through the pipeline, so
|||	    the units can be anything you want.
|||	    
|||	    Each instance of the decoder (each decoder context) assumes it's
|||	    called from a single thread. It's not reentrant.
|||	    
|||	  MPEG Audio Parameters -preformatted
|||
|||	    The decoder supports MPEG audio with these parameters:
|||	      Layer                 only Layer II.
|||	      Sampling frequency    only 44.1 KHz.
|||	      Emphasis              ignored by the decoder (50/15 microsecond deemphasis
|||	                            can be implemented in the MPEGAudioSubscriber by
|||	                            switching in the appropriate DSP instrument).
|||	      Bit rate              any Layer II bit rate.
|||	      Channel mode          stereo, intensity_stereo, dual channel, or mono.
|||	
|||	    Along with each decoded frame (presentation unit), the decoder returns
|||	    the frame's AudioHeader (a 32-bit struct containing the above parameters
|||	    and more). The client can look at these parameters to detect emphasis and
|||	    channel mode and then react accordingly.
|||	
|||	  Arguments
|||
|||	    ctx
|||	        Output parameter. CreateMPAudioDecoder() will store the newly
|||	        created MPADecoderContext in *ctx.
|||
|||	    CallbackFns
|||	        A struct containing two callback function pointers for the decoder
|||	        to use. The decoder will call the MPAGetCompressedBfrFn() function
|||	        to get each compressed MPEG audio buffer to decompress, and
|||	        MPACompressedBfrReadFn() when its done decoding each compressed
|||	        data buffer.
|||
|||	  Return Value
|||	
|||	    0 for success, or a negative error code such as MPANoMemErr (couldn't
|||	    allocate memory).
|||
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
|||	  See Also
|||
|||	    DeleteMPAudioDecoder(), MPAudioDecode(), MPAGetCompressedBfrFn(),
|||	    MPACompressedBfrReadFn()
|||
******************************************************************************/
/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name DeleteMPAudioDecoder
|||	Delete an MPEG Audio decoder context structure.
|||
|||	  Synopsis
|||
|||	    Err DeleteMPAudioDecoder(MPADecoderContext *ctx)
|||
|||	  Description
|||
|||	    Delete an MPEG audio decoder context structure. This function should
|||	    be called before your application exits and should not be called from
|||	    the callback functions. It's a good idea to call MPAFlush() before
|||	    calling DeleteMPAudioDecoder().
|||	    
|||	    The decoder assumes that it's called from a single thread.
|||
|||	  Arguments
|||
|||	    ctx
|||	        Pointer to the MPEG audio decoder context structure to delete.
|||
|||	  Return Value
|||	
|||	    0 for success. 
|||
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
|||	  See Also
|||
|||	    CreateMPAudioDecoder(), MPAFlush()
|||
******************************************************************************/
/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name MPAFlush
|||	Flush the decoder's input and output buffers.
|||
|||	  Synopsis
|||
|||	    Err MPAFlush(MPADecoderContext *ctx)
|||
|||	  Description
|||
|||	    Flush the contents of the compressed and decompressed buffers and 
|||	    re-initialize the compressed data buffer struct. You need to call
|||	    this at every branch in the input data, and it's a good idea to call
|||	    this before calling DeleteMPAudioDecoder().
|||
|||	    Do NOT call this from within the MPAGetCompressedBfrFn() or
|||	    MPACompressedBfrReadFn() callback functions.
|||
|||	  Arguments
|||
|||	    ctx
|||	        Pointer to the MPEG audio decoder context structure. 
|||
|||	  Return Value
|||	
|||	    0 for success or a negative error code for failure, e.g. from
|||	    MPACompressedBfrReadFn().
|||
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
******************************************************************************/
/******************************************************************************
|||	AUTODOC -public -class Streaming -group MPEG_Audio_Decoder -name MPAudioDecode
|||	Decode one frame of MPEG audio.
|||
|||	  Synopsis
|||
|||	    Err MPAudioDecode(void *theUnit, MPADecoderContextPtr ctx,
|||	        uint32 *pts, int32 *ptsIsValidFlag, uint32 *decompressedBufr,
|||	        AudioHeader *header)
|||
|||	  Description
|||
|||	    Decode one frame of MPEG audio and store the decoded data into
|||	    *decompressedBufr.
|||
|||	  Arguments
|||
|||	    theUnit
|||	        This "unit" pointer is an arbitrary client data value. The
|||	        decoder will just pass it through to the callback functions.
|||
|||	    ctx
|||	        Pointer to the MPEG audio decoder context structure.
|||
|||	    pts
|||	        Output parameter. MPAudioDecode() will store the decompressed
|||	        frame's presentation time, if any, in *pts. This indicates the
|||	        intended time to present the decompressed frame. The value comes
|||	        from the MPAGetCompressedBfrFn() callback function.
|||
|||	    ptsIsValidFlag
|||	        Output parameter. MPAudioDecode() will store a boolean value in
|||	        *ptsIsValidFlag to indicate whether the decompressed frame has a
|||	        presentation time. The value comes from the MPAGetCompressedBfrFn()
|||	        callback function.
|||
|||	    decompressedBufr
|||	        Pointer to a buffer. MPAudioDecode() will store one frame (1152
|||	        stereo samples) of decompressed audio data into *decompressedBufr.
|||	        
|||	        The buffer must be BYTES_PER_MPEG_AUDIO_FRAME bytes long. (See
|||	        <misc/mpaudiodecode.h>.)
|||
|||	    header
|||	        Output parameter. MPAudioDecode() will store an MPEG-standard
|||	        AudioHeader structure into *header. This AudioHeader structure
|||	        describes the decoded audio frame. (See <misc/mpeg.h>.)
|||	        
|||	        The most useful fields in this header are mode (mono/stereo/
|||	        dual channel) and emphasis (none, 50/15 microseconds, CCITT J.17).
|||
|||	  Return Value
|||	
|||	    0 for success, with the decompressed data stored in decompressedBufr,
|||	    or a negative error code indicating decoding errors, or any non-zero
|||	    (positive or negative) value returned by the MPAGetCompressedBfrFn()
|||	    or MPACompressedBfrReadFn() callback functions.  Any non-zero return 
|||	    value implies that the decompressedBufr does NOT contain valid data.
|||
|||	  Implementation
|||
|||	    Folio call implemented in mpegaudiodecoder folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/mpaudiodecode.h>, System.m2/Modules/mpegaudiodecoder
|||
|||	  See Also
|||
|||	    CreateMPAudioDecoder(), DeleteMPAudioDecoder(), MPAGetCompressedBfrFn()
|||	    MPACompressedBfrReadFn(), MPAFlush()
|||
******************************************************************************/
/* keep the compiler happy... */
extern int foo;
