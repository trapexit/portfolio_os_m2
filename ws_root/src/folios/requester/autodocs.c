/* @(#) autodocs.c 96/09/07 1.4 */

/**
|||	AUTODOC -class Requester -name CreateStorageReq
|||	Create a storage requester object.
|||
|||	  Synopsis
|||
|||	    Err CreateStorageReq(StorageReq **req, const TagArg *tags);
|||
|||	    Err CreateStorageReqVA(StorageReq **req, uint32 tags, ...);
|||
|||	  Description
|||
|||	    This function creates a storage requester object. This object
|||	    can be used to display a user-interface to allow the user
|||	    to interact with the file system.
|||
|||	  Arguments
|||
|||	    req
|||	        A pointer to a StorageReq variable, where a handle to the
|||	        requester object can be stored.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing data for this
|||	        function. See below for a description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    STORREQ_TAG_VIEW_LIST (Item)
|||	        This tag lets you specify the view list into which the
|||	        requester being created should be displayed. If this tag is not
|||	        supplied, the requester will be displayed in the primary view
|||	        list.
|||
|||	    STORREQ_TAG_FONT (Item)
|||	        This specifies the font to use for textual displays. If this is
|||	        not supplied, a default font is used. Supplied fonts should
|||	        have a 2:1 aspect ratio, since the display used is 640x240
|||	        pixels.
|||
|||	    STORREQ_TAG_LOCALE (Item)
|||	        This specifies the locale to use for language and for number
|||	        formatting. If this tag is not supplied, the current system
|||	        locale is used.
|||
|||	    STORREQ_TAG_OPTIONS (uint32)
|||	        This tag supplies a bitmask of options that control whether
|||	        various features are available to the user. See the many
|||	        STORREQ_OPTION_XXX constants in <ui/requester.h> for details.
|||
|||	    STORREQ_TAG_PROMPT (SpriteObj *)
|||	        This specifies the sprite to display as a prompt to the user.
|||	        If this tag is not supplied, no prompt is displayed.
|||
|||	    STORREQ_TAG_FILTERFUNC (FileFilterFunc)
|||	        This lets you specify a function pointer that is used to
|||	        determine whether or not individual files or directories
|||	        should be displayed to the user. The function gets called
|||	        for every file that is scanned, and is supplied the directory
|||	        name and a DirectoryEntry structure describing the file of
|||	        interest. If the function returns TRUE, then the file is
|||	        displayed to the user. If this tag is not supplied, the default
|||	        is to display all files.
|||
|||	    STORREQ_TAG_DIRECTORY (const char *)
|||	        This lets you specify the initial directory to display when
|||	        bringing up the storage requester. This can be an absolute path
|||	        (something starting with /) or a path relative to the current
|||	        directory. If this tag is not supplied, or if the supplied
|||	        directory can't be found, then the current directory will be
|||	        used. If the current directory is not available, then another
|||	        suitable directory will be displayed instead.
|||
|||	    STORREQ_TAG_FILE (const char *)
|||	        This lets you specify the initial file to select within the
|||	        displayed directory. If this is not supplied, or if that file
|||	        doesn't exist within the directory, a suitable default will
|||	        be selected.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code upon failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in requester folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/requester.h>, System.m2/Modules/requester
|||
|||	  See Also
|||
|||	    DisplayStorageReq(), DeleteStorageReq(), ModifyStorageReq(),
|||	    QueryStorageReq()
|||
**/

/**
|||	AUTODOC -class Requester -name DeleteStorageReq
|||	Delete a storage requester object.
|||
|||	  Synopsis
|||
|||	    Err DeleteStorageReq(StorageReq *req);
|||
|||	  Description
|||
|||	    This function deletes a storage requester object. This frees
|||	    up any resources allocated when the object was created. Once
|||	    the object is deleted, it can no longer be used.
|||
|||	  Arguments
|||
|||	    req
|||	        The storage requester object to delete. This may be NULL in
|||	        which case this function does nothing.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code upon failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in requester folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/requester.h>, System.m2/Modules/requester
|||
|||	  See Also
|||
|||	    DisplayStorageReq(), CreateStorageReq(), ModifyStorageReq(),
|||	    QueryStorageReq()
|||
**/

/**
|||	AUTODOC -class Requester -name DisplayStorageReq
|||	Display a storage requester object to the user.
|||
|||	  Synopsis
|||
|||	    Err DisplayStorageReq(StorageReq *req);
|||
|||	  Description
|||
|||	    This function displays a storage requester to the user.
|||	    The user will be able to navigate the file system and make
|||	    selections. The function only returns when the user is done
|||	    interacting with the object.
|||
|||	  Warning
|||
|||	    Once this calls returnes, the state of the graphics hardware is
|||	    indeterminate. You should not assume anything about register and
|||	    TRAM contents.
|||
|||	  Arguments
|||
|||	    req
|||	        The storage requester object to display to the user.
|||
|||	  Return Value
|||
|||	    STORREQ_CANCEL
|||	        The user chose to cancel the operation. The app should return
|||	        to the same state it was before the storage requester was
|||	        displayed.
|||
|||	    STORREQ_OK
|||	        The user made a choice and wishes the app to proceed with the
|||	        operation.
|||
|||	    A negative error code is returned upon failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in requester folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/requester.h>, System.m2/Modules/requester
|||
|||	  See Also
|||
|||	    CreateStorageReq(), DeleteStorageReq(), ModifyStorageReq(),
|||	    QueryStorageReq()
|||
**/

/**
|||	AUTODOC -class Requester -name ModifyStorageReq
|||	Modify attributes of a storage requester object.
|||
|||	  Synopsis
|||
|||	    Err ModifyStorageReq(StorageReq *req, const TagArg *tags);
|||
|||	    Err ModifyStorageReqVA(StorageReq *req, uint32 tags, ...);
|||
|||	  Description
|||
|||	    This function lets you modify attributes of an existing storage
|||	    requester object. The object attributes are persistent throughout
|||	    the life of the object. Using this function you can change any
|||	    individual attribute. Any attribute not specified by the tag list
|||	    will not be affected.
|||
|||	  Arguments
|||
|||	    req
|||	        A pointer to the storage requester object to affect.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing data for this
|||	        function. See below for a description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    STORREQ_TAG_VIEW_LIST (Item)
|||	        This tag lets you specify the view list into which the
|||	        requester being created should be displayed.
|||
|||	    STORREQ_TAG_FONT (Item)
|||	        This specifies the font to use for textual displays. Supplied
|||	        fonts should have a 2:1 aspect ratio, since the display used
|||	        is 640x240 pixels.
|||
|||	    STORREQ_TAG_LOCALE (Item)
|||	        This specifies the locale to use for language and for number
|||	        formatting.
|||
|||	    STORREQ_TAG_OPTIONS (uint32)
|||	        This tag supplies a bitmask of options that control whether
|||	        various features are available to the user. See the many
|||	        STORREQ_OPTION_XXX constants in <ui/requester.h> for details.
|||
|||	    STORREQ_TAG_PROMPT (SpriteObj *)
|||	        This specifies the sprite to display as a prompt to the user.
|||
|||	    STORREQ_TAG_FILTERFUNC (FileFilterFunc)
|||	        This lets you specify a function pointer that is used to
|||	        determine whether or not individual files or directories
|||	        should be displayed to the user. The function gets called
|||	        for every file that is scanned, and is supplied the directory
|||	        name and a DirectoryEntry structure describing the file of
|||	        interest. If the function returns TRUE, then the file is
|||	        displayed to the user.
|||
|||	    STORREQ_TAG_DIRECTORY (const char *)
|||	        This lets you specify the initial directory to display when
|||	        bringing up the storage requester. This can be an absolute path
|||	        (something starting with /) or a path relative to the current
|||	        directory.
|||
|||	    STORREQ_TAG_FILE (const char *)
|||	        This lets you specify the initial file to select within the
|||	        displayed directory.
|||
|||	  Return Value
|||
|||	    >= 0 if successful, or a negative error code upon failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in requester folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/requester.h>, System.m2/Modules/requester
|||
|||	  See Also
|||
|||	    DisplayStorageReq(), DeleteStorageReq(), CreateStorageReq(),
|||	    QueryStorageReq()
|||
**/

/**
|||	AUTODOC -class Requester -name QueryStorageReq
|||	Query the current state of certain attributes of a storage requester
|||	object.
|||
|||	  Synopsis
|||
|||	    Err QueryStorageReq(StorageReq *req, const TagArg *tags);
|||
|||	    Err QueryStorageReqVA(StorageReq *req, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you query the current state of certain
|||	    attributes of a storage requester object. You provide a tag
|||	    list of the attributes to query. The parameter for each tag
|||	    points to a memory location where the resulting information
|||	    should be stored.
|||
|||	  Arguments
|||
|||	    req
|||	        A pointer to the requester object to query.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing data for this
|||	        function. See below for a description of the tags supported.
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    STORREQ_TAG_VIEW_LIST (Item *)
|||	        This queries the requester's view list attribute. This will
|||	        return -1 if no explicit view list was given.
|||
|||	    STORREQ_TAG_FONT (Item *)
|||	        This queries the font that the requester is using for textual
|||	        displays.
|||
|||	    STORREQ_TAG_LOCALE (Item *)
|||	        This queries the locale that the requester is using for
|||	        formatting numbers and language selection.
|||
|||	    STORREQ_TAG_OPTIONS (uint32)
|||	        This returns the set of options currently active for this
|||	        requester. This is a bitmask of the STORREQ_OPTION_XXX
|||	        constants from <ui/requester.h>
|||
|||	    STORREQ_TAG_PROMPT (SpriteObj **)
|||	        This returns a pointer to the SpriteObj used to prompt the
|||	        user.
|||
|||	    STORREQ_TAG_FILTERFUNC (FileFilterFunc *)
|||	        This returns a pointer to the current file filtering function,
|||	        or NULL if no filtering function is currently installed.
|||
|||	    STORREQ_TAG_DIRECTORY (char *)
|||	        This tag specifies a pointer to a string buffer.
|||	        QueryStorageReq() will copy the name of the current directory
|||	        into this buffer. The size of the buffer must be at least
|||	        FILESYSTEM_MAX_NAME_LEN bytes.
|||
|||	    STORREQ_TAG_FILE (char *)
|||	        This tag specifies a pointer to a string buffer.
|||	        QueryStorageReq() will copy the name of the currently selected
|||	        file into this buffer. The size of the buffer must be at least
|||	        FILESYSTEM_MAX_NAME_LEN bytes.
|||
|||	  Return Value
|||
|||	    >= 0 if successful, or a negative error code upon failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in requester folio V30.
|||
|||	  Associated Files
|||
|||	    <ui/requester.h>, System.m2/Modules/requester
|||
|||	  See Also
|||
|||	    CreateStorageReq(), DeleteStorageReq(), ModifyStorageReq(),
|||	    DisplayStorageReq()
|||
**/

/* keep the compiler happy... */
extern int foo;

