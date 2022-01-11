/* @(#) autodocs.c 96/04/24 1.4 */

/**
|||	AUTODOC -public -class Icon -name LoadIcon
|||	Loads icon data into memory and prepares it for rendering.
|||
|||	  Synopsis
|||
|||	    Err LoadIcon(Icon **icon, const TagArg *tags);
|||	    Err LoadIconVA(Icon **icon, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function provides a method for retrieving icons from
|||	    many different parts of the system, including from a file,
|||	    a file system, a type of hardware, or a device.
|||
|||	  Arguments
|||
|||	    icon
|||	        A pointer to a variable where a handle to the Icon will
|||	        be stored. The value is set to NULL if the icon can't be
|||	        loaded.
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
|||	    LOADICON_TAG_FILENAME (char *)
|||	        Defines the file path of an icon file to load.  This tag is
|||	        mutually exclusive with LOADICON_TAG_IFFPARSER,
|||	        LOADICON_TAG_FILESYSTEM, LOADICON_TAG_DRIVER.
|||
|||	    LOADICON_TAG_IFFPARSER (IFFParser *)
|||	        This tag permits the caller to utilize an already existing
|||	        IFFParser structure from the IFF folio.  This is useful for
|||	        embedding icon data in other IFF files.  This tag is
|||	        mutually exclusive with LOADICON_TAG_FILENAME, 
|||	        LOADICON_TAG_FILESYSTEM, LOADICON_TAG_DRIVER.  Note that use 
|||	        of this tag requires the presence of LOADICON_TAG_IFFPARSETYPE 
|||	        to be functional.
|||	
|||	    LOADICON_TAG_IFFPARSETYPE (uint32)
|||	        This tag defines options for IFF parsing, and is only valid
|||	        when present with the LOADICON_TAG_IFFPARSER tag.  The
|||	        following options are available, and can be OR'd together:
|||
|||	        LOADICON_TYPE_AUTOPARSE
|||	        Requests that LoadIcon() parse for a FORM ICON chunk.
|||
|||	        LOADICON_TYPE_PARSED
|||	        Indicates that the caller has already parsed up to a
|||	        FORM ICON, and that the current context of the IFF stream is
|||	        within that chunk.
|||	
|||	    LOADICON_TAG_FILESYSTEM (char *)
|||	        When provided with a filesystem-style path, this tag will
|||	        attempt to locate an icon associated with that filesystem.
|||	        This tag is mutually exclusive with LOADICON_TAG_FILENAME,
|||	        LOADICON_TAG_IFFPARSER, LOADICON_TAG_DRIVER.
|||
|||	    LOADICON_TAG_DRIVER (char *)
|||	        When provided with the name of a device driver, this tag
|||	        will attempt to locate an icon associated with that device
|||	        driver without loading the driver itself into memory.  This
|||	        tag is mutually exclusive with LOADICON_TAG_FILENAME, 
|||	        LOADICON_TAG_IFFPARSER, LOADICON_TAG_FILESYSTEM.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Icon folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/icon.h>
|||
|||	  See Also
|||
|||	    UnloadIcon()
|||
**/

/**
|||	AUTODOC -public -class Icon -name SaveIcon
|||	Creates an IFF Icon file.
|||
|||	  Synopsis
|||
|||	    Err SaveIcon(char *UTFfilename, char *AppName, const TagArg *tags);
|||	    Err SaveIconVA(char *UTFfilename, char *AppName, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function permits the caller to create either a 
|||	    stand-alone Icon file, or a FORM ICON chunk within 
|||	    another IFF file.
|||
|||	  Arguments
|||
|||	    UTFfilename
|||	        Defines the name of an existing UTF file which
|||	        contains the texture data to be used for this icon.
|||	        Either single or multiple-texture UTF files are appropriate.
|||
|||	    AppName
|||	        A string (which is no greater in length than 31 characters)
|||	        that contains a user-readable string that describes the 
|||	        name of the application that created the icon.  (Such as
|||	        "SuperShark III".)
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
|||	    SAVEICON_TAG_FILENAME (char *)
|||	        Defines the file path to use when creating the icon file.  This 
|||	        tag is mutually exclusive with SAVEICON_TAG_IFFPARSER.
|||
|||	    SAVEICON_TAG_IFFPARSER (IFFParser *)
|||	        This tag, which is mutually exclusive with 
|||	        SAVEICON_TAG_FILENAME, permits the caller to write out a
|||	        full IFF Icon data chunk (a FORM ICON) to an existing
|||	        IFFParser *, from the IFF Folio.  This permits Icons to
|||	        be imbedded in other IFF files.
|||
|||	    SAVEICON_TAG_TIMEBETWEENFRAMES (TimeVal *)
|||	        When provided, this tag defines the amount of time
|||	        that should be waited when displaying each frame
|||	        of a multiple-textured Icon.  If not supplied, 
|||	        time of 0 seconds, 0 microseconds, will be saved to
|||	        the icon.  
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Icon folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/icon.h>
|||
|||	  See Also
|||
|||	    LoadIcon(), UnloadIcon()
|||
**/

/**
|||	AUTODOC -public -class Icon -name UnloadIcon
|||	Unloads an icon from memory and frees any resources associated with it.
|||
|||	  Synopsis
|||
|||	    Err UnloadIcon(Icon *icon);
|||
|||	  Description
|||
|||	    This function frees any resources allocated by LoadIcon(). After
|||	    this call is made, the icon pointer becomes invalid.
|||
|||	  Arguments
|||
|||	    icon
|||	        Pointer to an Icon as obtained from LoadIcon(). This pointer
|||	        may be NULL in which case this function does nothing.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Icon folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/icon.h>
|||
|||	  See Also
|||
|||	    LoadIcon()
|||
**/

/* keep the compiler happy... */
extern int foo;

