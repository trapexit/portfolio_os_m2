/* @(#) autodocs.c 96/06/30 1.4 */

/**
|||	AUTODOC -class SaveGame -name LoadGameData
|||	Loads a saved game file into memory.
|||
|||	  Synopsis
|||
|||	    Err LoadGameData(const TagArg *tags);
|||	    Err LoadGameDataVA(uint32 tag1, ... );
|||
|||	  Description
|||
|||	    This function scans a standard saved-game file, and attempts to
|||	    load the application specific section into the application
|||	    defined buffer.
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function. See below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    LOADGAMEDATA_TAG_FILENAME
|||	        An absolute or relative pathname for the saved game file.
|||	        Mutually exclusive with LOADGAMEDATA_TAG_IFFPARSER.
|||
|||	    LOADGAMEDATA_TAG_IFFPARSER
|||	        An IFFParser * to read game data from.  Mutually exclusive with
|||	        LOADGAMEDATA_TAG_FILENAME.
|||
|||	    LOADGAMEDATA_TAG_BUFFERARRAY
|||	        An array of SGData structures, each containing a pointer
|||	        to buffer memory, and a length value for that particular
|||	        buffer.  An entry with a length value of 0 terminates the
|||	        array.  This array is used to load in individual data chunks
|||	        as they're encountered.  The folio will fill in the Actual
|||	        field of each structure with the amount of data it actually
|||	        stored there.  Mutually exclusive with the tag
|||	        LOADGAMEDATA_TAG_CALLBACK.
|||
|||	    LOADGAMEDATA_TAG_CALLBACK (GSCallBack *)
|||	        A pointer to a callback function that is called  each time
|||	        the folio encounters a new chunk of saved game data.
|||	        When a new chunk of saved game data is encountered, the folio
|||	        fills in the Length field of a SGData structure with the
|||	        size of the data that it would like to load, then passes that
|||	        structure (along with any app-private data defined by
|||	        LOADGAME_DATA_TAG_CALLBACKDATA) to the callback routine.
|||	        At this point, the app has several options.  If it would
|||	        like to load the data, it should provide a buffer pointer in
|||	        the Buffer field of the SGData structure and return.  If
|||	        the app would like to skip the data, it should modify the Length
|||	        field to 0 and return.  If the app ever returns a negative
|||	        number from the callback, the LoadGameData() function will fail,
|||	        interpreting that as an error.  This tag is mutually exclusive
|||	        with the tag LOADGAMEDATA_TAG_BUFFERARRAY.
|||
|||	    LOADGAMEDATA_TAG_CALLBACKDATA (void *)
|||	        This tag, valid only when presented with the tag
|||	        LOADGAMEDATA_TAG_CALLBACK, defines application-private
|||	        data to be provided each time the callback function is
|||	        used.
|||
|||	    LOADGAMEDATA_TAG_ICON (Icon **)
|||	        When provided, this tag requests that any Icon encountered
|||	        in the game file be loaded and a pointer to the Icon
|||	        structure be stored in the given pointer.  If no Icon is
|||	        encountered, the pointer will return as NULL.  Note that
|||	        if an Icon is loaded, it is your responsibility to free it
|||	        (with the UnloadIcon() function in the icon folio).
|||
|||	    LOADGAMEDATA_TAG_IDSTRING (char *)
|||	        When provided, this tag requests that the game file's
|||	        ID string be copied into this location.  Any string buffer
|||	        provided should be a minimum of 32 characters in length.
|||
|||	  Return Value
|||
|||	    If positive, an indication of success.
|||	    If negative, a system-standard error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in SaveGame folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/savegame.h>
|||
|||	  See Also
|||
|||	    SaveGameData(), SaveGameDataVA()
|||
**/

/**
|||	AUTODOC -class SaveGame -name SaveGameData
|||	Stores a saved game file.
|||
|||	  Synopsis
|||
|||	    Err SaveGameData(const char *appname, const TagArg *tags);
|||	    Err SaveGameDataVA(const char *appname, uint32 tag1, ... );
|||
|||	  Description
|||
|||	    This function saves a standard saved-game file.
|||
|||	  Arguments
|||
|||	    appname
|||	        A pointer to a string (maximum of 32 characters in length) that
|||	        uniquely identifies the name of the application that created the
|||	        save game file.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function. See below for a description of the tags
|||	        supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    SAVEGAMEDATA_TAG_FILENAME (const char *)
|||	        An absolute or relative pathname for the saved game file.
|||	        Mutually exclusive with SAVEGAMEDATA_TAG_IFFPARSER.
|||
|||	    SAVEGAMEDATA_TAG_IFFPARSER (IFFParser *)
|||	        An IFFParser * to save game data to.  Mutually exclusive with
|||	        SAVEGAMEDATA_TAG_FILENAME.
|||
|||	    SAVEGAMEDATA_TAG_IDSTRING (char *)
|||	        A pointer to a string (maximum of 32 characters in length) that
|||	        represents what this particular game is.  [IE, 'Bob on Level 3']
|||	        If not provided, the string 'Saved Game' is used.
|||
|||	    SAVEGAMEDATA_TAG_ICON (const char *)
|||	        A pathname to a UTF file (containing one or more textures) to
|||	        be used as the icon imagery for this saved game file.
|||
|||	    SAVEGAMEDATA_TAG_TIMEBETWEENFRAMES (TimeVal *)
|||	        The amount of time that a reader of this icon should be
|||	        instructed to wait between displaying each frame of the icon.
|||	        [Not necessary if only 0 or 1 frame is provided.]
|||
|||	    SAVEGAMEDATA_TAG_BUFFERARRAY (SGData *)
|||	        An array of SGData structures, each containing a pointer
|||	        to buffer memory and a length value for that particular
|||	        buffer.  An entry with a length value of 0 terminates the
|||	        array.  This array is used to save out particular chunks
|||	        of data to the IFF file.  Mutually exclusive with
|||	        SAVEGAMEDATA_TAG_CALLBACK.
|||
|||	    SAVEGAMEDATA_TAG_CALLBACK (GSCallBack *)
|||	        A pointer to a callback function that is called iteratively
|||	        to locate the next chunk of game data to save.  The function
|||	        is called with a pointer to a SGData struct that the
|||	        function is required to fill in, and an application-private
|||	        value defined in SAVEGAMEDATA_TAG_CALLBACKDATA.  When all
|||	        data has been written, the app should fill in the length
|||	        field of the SGData structure with '0' to terminate
|||	        the process.  Mutually exclusive with
|||	        SAVEGAMEDATA_TAG_BUFFERARRAY.
|||
|||	    SAVEGAMEDATA_TAG_CALLBACKDATA (void *)
|||	        This tag, valid only when presented with the tag
|||	        SAVEGAMEDATA_TAG_CALLBACK, defines application-private
|||	        data to be provided each time the callback function is
|||	        used.
|||
|||	  Return Value
|||
|||	    Positive or equal to zero indicates success.  A negative number
|||	    describes a system-standard error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in SaveGame folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/savegame.h>
|||
|||	  See Also
|||
|||	    LoadGameData(), LoadGameDataVA()
|||
**/

/* keep the compiler happy... */
extern int foo;
