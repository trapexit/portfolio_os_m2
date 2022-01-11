/******************************************************************************
**
**  @(#) autodocs.c 96/11/20 1.3
**
**	File:			autodocs.c
**	Contains:		autodocs for Weaver script commands
**
******************************************************************************/

/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name audioclockchan
|||	Specifies which audio subscriber channel will drive the stream clock.
|||	
|||	  Synopsis
|||	
|||	    audioclockchan <audio_clock_channel>
|||	
|||	  Description
|||	
|||	    Specifies which audio subscriber channel will drive the data stream
|||	    presentation clock. The default is audio channel 0.
|||	    
|||	    This information will be useful IF the stream has audio data on the
|||	    specified channel.
|||	
|||	    The audio subscriber will adjust the Data Streamer's presentation
|||	    clock to match the time stamps in data chunks that arrive for this
|||	    channel. If a stream has audio data throughout, using this will help
|||	    synchronize audio with video, esp. when the stream starts up and when
|||	    it branches.
|||	
|||	  Arguments
|||	
|||	    audio_clock_channel
|||	        An integer which specifies which audio channel will drive the Data
|||	        Streamer's clock. Legal channel numbers are from 0 to 31.
|||	
|||	  Caveats
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    audioclockchan 1
|||	
|||	  See Also
|||	
|||	    Weaver(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name dataacqdeltapri
|||	Specifies the relative priority for the Data Acquisition thread.
|||	
|||	  Synopsis
|||	
|||	    dataacqdeltapri <delta_priority>
|||	
|||	  Description
|||	
|||	    Specifies the priority of the Data Acquisition thread relative to
|||	    the priority of the client application of the Data Streaming
|||	    library.
|||	
|||	  Arguments
|||	
|||	    delta_priority
|||	        An integer to add to the application thread's priority to compute
|||	        the Data Acquisition thread's priority. A positive value
|||	        will make the Data Acq thread higher priority than the app,
|||	        while a negative value will make it lower.
|||	
|||	  Caveats
|||	
|||	    The computed priority must be in the range 11 to 199.
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    dataacqdeltapri -9
|||	
|||	  See Also
|||	
|||	    Weaver(@), streamdeltapri(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name enableaudiomask
|||	Specifies which audio channels to pre-enable.
|||	
|||	  Synopsis
|||	
|||	    enableaudiomask <bit_mask>
|||	
|||	  Description
|||	
|||	    Specifies which audio channels to pre-enable before stream playback
|||	    begins. Legal channel numbers are from 0 to 31.
|||	    
|||	    The application can also enable and disable channels by calling
|||	    DSSetChannel().
|||	
|||	  Arguments
|||	
|||	    bit_mask
|||	        A hex bit mask which specifies which audio channels will be enabled,
|||	        as indicated by the position of bits set to 1 in a 32-bit register.
|||	        Thus, value 0x3 enables audio channels 0 and 1.
|||	
|||	  Caveats
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    enableaudiomask 0x3
|||	
|||	  See Also
|||	
|||	    Weaver(@),  writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name file
|||	Weaves an input stream file into the woven stream.
|||	
|||	  Synopsis
|||	
|||	    file <input_file> <priority> <time_offset>
|||	
|||	  Description
|||	
|||	    Specifies an input stream file to "weave" (multiplex) with other input
|||	    stream files. The priority determines the order in which any chunks
|||	    with identical timestamps are written to the output stream. Smaller
|||	    priority numbers are higher priority. The time offset is similar to the
|||	    streamstarttime(@) offset except it only offsets this input file's
|||	    chunks.
|||	
|||	  Arguments
|||	
|||	    input_file
|||	        Name of a chunkified file to weave.
|||	
|||	    priority
|||	        An integer which determines the relative order, compared to other
|||	        input files, to output of chunks into the woven stream when two or
|||	        more chunks from different streams have identical timestamps.
|||	        Value 0 is the highest priority.
|||	
|||	    time_offset
|||	        Time, in audio ticks, to offset this input file's chunks when
|||	        writing them to the woven output stream.
|||	
|||	  Example
|||	
|||	    file raggae.SNDS 0 24
|||	
|||	  See Also
|||	
|||	    Weaver(@), streamstarttime(@)
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name markertime
|||	Adds an entry to the marker table to use as a branch destination.
|||	
|||	  Synopsis
|||	
|||	    markertime <time>
|||	
|||	  Description
|||	
|||	    A stream file can contain a marker table: a table of branch destination
|||	    points in that file. Each marker entry is a <time, offset> pair which
|||	    allows the streamer to branch to the given byte offset within the file
|||	    and set the stream clock to the given presentation time.
|||	    
|||	    A call to DSGoMarker() or a a STRM GOTO chunk (see writegotochunk(@))
|||	    can branch to the nth marker in the marker table. DSGoMarker() can also
|||	    branch to an absolute byte offset within the file (e.g. 0) (which
|||	    doesn't require a marker table), or to the marker with a specified
|||	    time, or other alternatives.
|||	    
|||	    The markertime(@) command asks the Weaver to add an entry to the marker
|||	    table that it's building up. You specify the destination time. The
|||	    Weaver(@) will find the matching byte offset.
|||	    
|||	    The markertime(@) command also asks the Weaver to place all pre-branch
|||	    data (that is, all data chunks with timestamps before the marker time)
|||	    before the marker point, and to place all post-branch data (data chunks
|||	    with timestamps at or beyond the marker time) after the marker point.
|||	    
|||	    The markertime(@) command also asks the Weaver to place the post-branch
|||	    data at the start of a stream block even if that requires inserting a
|||	    FILL chunk to fill out the current stream block. (In the future, there
|||	    could be an "unfilledmarkertime" command. An unfilled marker conserves
|||	    stream space and bandwidth but costs seek covering time since the
|||	    streamer will skip part of the first block read after branching.)
|||	    
|||	    Include a markertime(@) command in your weave script for each location
|||	    you wish to branch to.
|||	
|||	  Arguments
|||	
|||	    time
|||	        An integer which specifies the marker's stream time position in
|||	        audio ticks. Allowed values are integers in the range
|||	        0 to 7FFFFFFF (hex).
|||	
|||	  Caveats
|||	
|||	    This command adds an entry to the marker table. The writemarkertable(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    markertime 800
|||	    markertime 1200
|||	    writemarkertable
|||	
|||	  See Also
|||	
|||	    Weaver(@), writemarkertable(@), writegotochunk(@);
|||	    DSMarkerChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name mediablocksize
|||	Specifies the block size of the media. (Use 2048 for CD-ROM.)
|||	
|||	  Synopsis
|||	
|||	    mediablocksize <blocksize>
|||	
|||	  Description
|||	
|||	    Mediablocksize(@) specifies the blocksize of the media--the run-time
|||	    source of the stream file. This value can also be specified by the "-m"
|||	    Weaver(@) command-line argument.
|||	
|||	    If this command is not present in the weave script, the default
|||	    value is 2048 bytes (the blocksize of most CD-ROM drives).
|||	
|||	    What the Weaver(@) does with the media blocksize info is cross-check
|||	    that it evenly divides the stream block size, as specified by the
|||	    streamblocksize(@) weave script command or the "-s" Weaver(@) command-
|||	    line argument. This is important because the Data Streamer always reads
|||	    the stream file in units of stream blocks via block-oriented file I/O.
|||	    If the stream block isn't an integral number of media blocks, the
|||	    streamer will fail with I/O errors.
|||	
|||	  Arguments
|||	
|||	    blocksize
|||	        An integer no greater than 7FFFFFFF (hex).
|||	
|||	  Example
|||	
|||	    mediablocksize 2048
|||	
|||	  See Also
|||	
|||	    Weaver(@), streamblocksize(@)
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name numsubsmessages
|||	Specifies number of subscriber messages for the Data Streamer to allocate.
|||	
|||	  Synopsis
|||	
|||	    numsubsmessages <number_of_messages>
|||	
|||	  Description
|||	
|||	    Specifies the number of subscriber messages that the Data Streamer
|||	    should allocate to run a given stream.
|||	
|||	    If this command is not present in the weave script, the default
|||	    value is 256.
|||	
|||	  Arguments
|||	
|||	    number_of_messages
|||	        An integer specifying the number of subscriber messages to allocate.
|||	
|||	  Caveats
|||	
|||	    If the Data Streamer doesn't allocate enough subscriber messages, it
|||	    will get stuck and abort playback.
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    numsubsmessages 200
|||	
|||	  See Also
|||	
|||	    Weaver(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name preloadinstrument
|||	Specifies an audio DSP instrument to preload.
|||	
|||	  Synopsis
|||	
|||	    preloadinstrument <audio_data_type>
|||	
|||	  Description
|||	
|||	    Specifies an audio DSP instrument that the SAudioSubscriber should
|||	    preload, expressed as an SAudioSubscriber tag name. You can repeat this
|||	    command to specify up to 16 instruments to preload.
|||	    
|||	    The SAudioSubscriber uses DSP instruments to decode audio samples,
|||	    and it can load these instruments on demand. But it's important to
|||	    preload instruments if you're using the SAudioSubscriber since loading
|||	    them requires seeking the CD which disrupts stream playback.
|||	
|||	  Arguments
|||	
|||	    audio_data_type
|||	        A string which specifies the Data Type Tag ID. There are
|||	        tags for both uncompressed (AIFF) and compressed (AIFC)
|||	        sound types. For example, SA_22K_16B_M refers to uncompressed
|||	        22.05 kHz 16-bit mono. SA_44K_16B_S_CBD2 refers to 44.1 kHz
|||	        16-bit stereo, compressed 2:1. For a complete listing of all
|||	        AIFF and AIFC sound types, see the documentation for the
|||	        AudioChunkifier(@) tool.
|||	
|||	  Caveats
|||	
|||	    This command stores info in the stream header chunk. The
|||	    writestreamheader(@) command must also be in the weave script for this
|||	    to have any effect.
|||	
|||	  Example
|||	
|||	    preloadinstrument SA_44K_16B_S_CBD2
|||	
|||	  See Also
|||	
|||	    Weaver(@), AudioChunkifier(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name streamblocksize
|||	Specifies the stream block size used by the Weaver.
|||	
|||	  Synopsis
|||	
|||	    streamblocksize <block_size>
|||	
|||	  Description
|||	
|||	    Specifies the stream block size which will be used by the Weaver and
|||	    the run-time Data Streamer. This value can also be specified by the
|||	    "-s" Weaver(@) command-line argument. The Weaver will check that this
|||	    value is a multiple of the media block size (see mediablocksize(@)).
|||	
|||	    Typical sizes are 32768, 65536, and 98304. Smaller sizes allow faster
|||	    branching (since it takes less time to read the first post-branch block)
|||	    while larger sizes allow higher stream bandwidth.
|||	
|||	    If this command is not present in the weave script, the default
|||	    streamblocksize(@) is 32768 bytes.
|||	
|||	  Arguments
|||	
|||	    blocksize
|||	        A positive integer no larger than 7FFFFFFF (hex).
|||	
|||	  Caveats
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    streamblocksize 65536
|||	
|||	  See Also
|||	
|||	    Weaver(@), writestreamheader(@), mediablocksize(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name streambuffers
|||	Specifies number of stream buffers the application should allocate.
|||	
|||	  Synopsis
|||	
|||	    streambuffers <number_of_buffers>
|||	
|||	  Description
|||	
|||	    Specifies the number of buffers the application should allocate to
|||	    playback this stream.
|||	
|||	    If this command is not present in the weave script, the default
|||	    number of stream buffers is 4.
|||	
|||	    For most situations, at least 4 stream buffers are needed to ensure
|||	    smooth, robust, synchronized streaming. It may be possible to get by
|||	    with just 3 in some situations.
|||	
|||	  Arguments
|||	
|||	    number_of_buffers
|||	        A positive integer.
|||	
|||	  Caveats
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    streambuffers 6
|||	
|||	  See Also
|||	
|||	    Weaver(@), writestreamheader(@), streamblocksize(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name streamdeltapri
|||	Specifies the relative priority for the Data Stream thread.
|||	
|||	  Synopsis
|||	
|||	    streamdeltapri <delta_priority>
|||	
|||	  Description
|||	
|||	    Specifies the priority of the Data Stream thread relative to
|||	    the priority of the client application of the Data Streaming
|||	    library.
|||	
|||	  Arguments
|||	
|||	    delta_priority
|||	        An integer to add to the application thread's priority to compute
|||	        the Data Stream thread's priority. A positive value
|||	        will make the Data Stream thread higher priority than the app,
|||	        while a negative value will make it lower.
|||	
|||	  Caveats
|||	
|||	    The computed priority must be in the range 11 to 199.
|||	
|||	    This command sets a stream header chunk field. The writestreamheader(@)
|||	    command must also be in the weave script for this to have any effect.
|||	
|||	  Example
|||	
|||	    streamdeltapri -10
|||	
|||	  See Also
|||	
|||	    Weaver(@), dataacqdeltapri(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name streamstarttime
|||	Specifies a time offset for chunks going into the output stream.
|||	
|||	  Synopsis
|||	
|||	    streamstarttime <time>
|||	
|||	  Description
|||	
|||	    Specifies a time offset for chunks going into the output stream. This
|||	    value can also be specified by the "-b" Weaver(@) command line argument.
|||	
|||	    If this command is not present in the weave script, the default
|||	    value is 0.
|||	
|||	  Arguments
|||	
|||	    time
|||	        A positive integer giving the offset in audio ticks.
|||	
|||	  Example
|||	
|||	    streamstarttime 240
|||	    
|||	        Assuming all input streams start at time zero (which they'd
|||	        better!), this will add 240 ticks (approximately 1 second) to
|||	        the time stamp of each chunk when writing it to the woven
|||	        stream.
|||	
|||	  See Also
|||	
|||	    Weaver(@)
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name subscriber
|||	Asks to instantiate a subscribe for a particular data type.
|||	
|||	  Synopsis
|||	
|||	    subscriber <subscriber_ID> <delta_priority>
|||	
|||	  Description
|||	
|||	    Adds a subscriber entry into the stream header which asks the
|||	    application to instantiate a subscriber for a particular data type.
|||	    The arguments specify the subscriber data type and relative thread
|||	    priority (relative to the priority of the client application of the
|||	    Data Streaming library).
|||	
|||	  Arguments
|||	
|||	    subscriber_ID
|||	        A 4-byte ASCII code indicating the subscriber data type, e.g.:
|||	        MPVD for MPEGVideoSubscriber, MPAU for MPEGAudioSubscriber, EZFL
|||	        for EZFlixSubscriber, SNDS for SAudioSubscriber, and DATA for
|||	        DataSubscriber.
|||	
|||	    delta_priority
|||	        An integer to add to the application thread's priority to compute
|||	        the subscriber thread's priority. A positive value will make the
|||	        subscriber thread higher priority than the app, while a negative
|||	        value will make it lower. Generally you want to give the audio
|||	        subscriber the highest priority of all to help audio playback
|||	        without hiccups.
|||	
|||	  Caveats
|||	
|||	    If the run-time program doesn't instantiate a subscriber for some
|||	    data type, the Data Streamer will ignore all chunks of that data
|||	    type.
|||	
|||	    The computed priority must be in the range 11 to 199.
|||	
|||	    This command stores info in the stream header chunk. The
|||	    writestreamheader(@) command must also be in the weave script for this
|||	    to have any effect.
|||	
|||	  Examples
|||	
|||	    subscriber MPVD -2
|||	    subscriber SNDS 11
|||	
|||	  See Also
|||	
|||	    Weaver(@), writestreamheader(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>; NewDataSubscriber(),
|||	    NewEZFlixSubscriber(), NewMPEGAudioSubscriber(),
|||	    NewMPEGVideoSubscriber(), NewSAudioSubscriber()
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name writegotochunk
|||	Writes a STRM GOTO chunk at the specified time in the output stream.
|||	
|||	  Synopsis
|||	
|||	    writegotochunk <stream_time> <options> <destination>
|||	
|||	  Description
|||	
|||	    Writes a STRM GOTO chunk at the specified time in the output stream.
|||	    A GOTO chunk causes stream playback to unconditionally branch when
|||	    playback reaches the given stream_time.
|||	    
|||	    A GOTO chunk can use any of three alternative ways to specify the
|||	    branch destination, and can also use some optional flags. The branch
|||	    alternative and optional flags are encoded in the <options> argument.
|||	    
|||	    The <destination> argument specifies the branch destination point.
|||	    Its units depend on the <options> branch alternative.
|||	
|||	  Arguments
|||	
|||	    stream_time
|||	        Where to place the GOTO chunk, in audio ticks. At runtime, the
|||	        streamer will begin branching after it delivers chunks that
|||	        preceed the GOTO chunk (chunks with times less than this
|||	        stream_time).
|||	
|||	    options
|||	        A GOTO chunk can use any of three alternative ways to specify the
|||	        branch destination, and can also use some optional flags.
|||	        
|||	        branch option 0 = GOTO_OPTIONS_ABSOLUTE, which means branch to an
|||	        absolute (byte position, time). This alternative doesn't require a
|||	        marker.
|||	        NOTE: ABSOLUTE GOTO CHUNKS ARE NOT YET SUPPORTED BY THE WEAVER. A
|||	        future Weaver could create absolute GOTO chunks. You would specify
|||	        the branch destination time and the Weaver would compute the branch
|||	        destination byte position in the same way that it computes marker
|||	        table entries (see markertime(@)).
|||	        
|||	        branch option 1 = GOTO_OPTIONS_MARKER, which means branch to a
|||	        marker by marker number, an index into the marker table (see
|||	        markertime(@)).
|||	        
|||	        branch option 2 = GOTO_OPTIONS_PROGRAMMED, which means do a
|||	        programmed branch number, i.e. the branch destination is
|||	        determined by indexing into the programmed branch table.
|||	        NOTE: PROGRAMMED BRANCHES ARE NOT YET SUPPORTED BY THE WEAVER OR
|||	        THE DATA STREAMER.
|||	        
|||	        You can also BITOR these branch option flags into the <options>
|||	        argument:
|||	        
|||	        The flag 1<<8 = GOTO_OPTIONS_FLUSH means flush subscribers instead
|||	        of doing a butt-joint branch. This would be weird in a GOTO chunk
|||	        since it means not playing out all the data the preceeds the GOTO
|||	        chunk before branching.
|||	        
|||	        The flag 1<<9 = GOTO_OPTIONS_WAIT means branch then wait for
|||	        DSStartStream. This flag IS NOT YET SUPPORTED BY THE DATA STREAMER.
|||	
|||	    destination
|||	        This specifies where to branch to. Its units depend on the
|||	        <options> branch alternative.
|||	        
|||	        For GOTO_OPTIONS_MARKER, the <destination> argument specifies a
|||	        marker number, that is, an index into the marker table. Use the
|||	        markertime(@) command to generate marker table entries.
|||	        
|||	        For GOTO_OPTIONS_ABSOLUTE, the <destination> argument specifies a
|||	        destination stream time, as with markertime(@).
|||	        
|||	        For GOTO_OPTIONS_PROGRAMMED, the <destination> argument specifies a
|||	        programmed branch number, which is not yet supported by the
|||	        Data Streamer.
|||	
|||	  Example
|||	
|||	    writegotochunk 3351 1 1
|||	
|||	  See Also
|||	
|||	    Weaver(@), markertime(@); struct StreamGoToChunk and enum GOTO_Options
|||	    in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name writemarkertable
|||	Writes the marker table to the woven output stream.
|||	
|||	  Synopsis
|||	
|||	    writemarkertable
|||	
|||	  Description
|||	
|||	    Asks the Weaver(@) to write a marker table chunk to the woven output
|||	    stream. You need to use this command in every weave script that uses
|||	    the markertime(@) command.
|||	
|||	  Arguments
|||	
|||	    none.
|||	
|||	  Example
|||	
|||	    markertime 800
|||	    markertime 1200
|||	    writemarkertable
|||	
|||	  See Also
|||	
|||	    Weaver(@) "-mm" command line argument, markertime(@), writegotochunk(@);
|||	    DSMarkerChunk in <streaming/dsstreamdefs.h>
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name writestopchunk
|||	Writes a STRM STOP chunk at the specified time in the output stream.
|||	
|||	  Synopsis
|||	
|||	    writestopchunk <stream_time>
|||	
|||	  Description
|||	
|||	    Writes a STRM STOP chunk at the specified time in the output stream.
|||	    
|||	    A STOP chunk causes stream playback to stop much as if the client
|||	    application called DSStopStream(). If the client application has used
|||	    DSWaitEndOfStream() to register for end-of-stream-playback
|||	    notifications, the streamer will reply to that message with the
|||	    Err code kDSSTOPChunk. The client application can resume playback by
|||	    calling DSStartStream(), presuming after re-registering for
|||	    end-of-stream-playback notifications.
|||	
|||	  Arguments
|||	
|||	    stream_time
|||	        Where to place the STOP chunk, in audio ticks. At runtime, the
|||	        streamer will STOP after it finishes playing chunks that
|||	        preceed this STOP chunk (chunks with times less than this
|||	        stream_time).
|||	
|||	  Example
|||	
|||	    writestopchunk 2400
|||	
|||	  See Also
|||	
|||	    Weaver(@), struct StreamStopChunk in <streaming/dsstreamdefs.h>,
|||	    <streaming/datastreamlib.h>.
**/


/**
|||	AUTODOC -public -class Streaming_Tools -group Weaver_Script_Commands -name writestreamheader
|||	Writes the stream header to the woven output stream.
|||	
|||	  Synopsis
|||	
|||	    writestreamheader
|||	
|||	  Description
|||	
|||	    Asks the Weaver(@) to write a stream header chunk to the woven output
|||	    stream. The values in the stream header chunk are supplied by other
|||	    weave script commands and by Weaver(@) command line arguments.
|||	
|||	  Arguments
|||	
|||	    none.
|||	
|||	  Caveats
|||	
|||	    You should write a stream header in every woven stream.
|||	
|||	  Example
|||	
|||	    writestreamheader
|||	
|||	  See Also
|||	
|||	    Weaver(@), audioclockchan(@), dataacqdeltapri(@), enableaudiomask(@),
|||	    numsubsmessages(@), preloadinstrument(@), streamblocksize(@),
|||	    streambuffers(@), streamdeltapri(@), subscriber(@);
|||	    DSHeaderChunk in <streaming/dsstreamdefs.h>
**/


extern int foo;	/* a dummy declaration to keep the C compiler happy */
