/* @(#) autodocs.c 96/12/11 1.11 */

/**
|||	AUTODOC -class fsutils -name AppendPath
|||	Appends a path to an existing path specification.
|||
|||	  Synopsis
|||
|||	    Err AppendPath(char *path, const char *append,
|||	                   uint32 numBytes);
|||
|||	  Description
|||
|||	    This function appends a file, directory, or subpath name
|||	    to an existing path specification, dealing with slashes
|||	    and absolute vs relative directory paths.
|||
|||	  Arguments
|||
|||	    path
|||	        The path to append to.
|||
|||	    append
|||	        The file name, directory name, or full path to append.
|||
|||	    numBytes
|||	        The number of bytes available in the resulting path
|||	        buffer.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes in the resulting path, or -1
|||	    if the path buffer wasn't big enough to receive the final
|||	    path.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    FindFinalComponent()
|||
**/

/**
|||	AUTODOC -class fsutils -name FindFinalComponent
|||	Returns the last component of a path.
|||
|||	  Synopsis
|||
|||	    char *FindFinalComponent(const char *path);
|||
|||	  Description
|||
|||	    This function returns a pointer to the last component of a path
|||	    specification. If there is only one component in the
|||	    path, it returns a pointer to the beginning of the path string.
|||
|||	  Arguments
|||
|||	    path
|||	        The path specification.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the last component of the path.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    AppendPath()
|||
**/

/**
|||	AUTODOC -class fsutils -name GetPath
|||	Obtain the absolute directory path leading to an opened file.
|||
|||	  Synopsis
|||
|||	    Err GetPath(Item file, char *path, uint32 numBytes);
|||
|||	  Description
|||
|||	    This function does the needed legwork to obtain the full
|||	    path leading to an already opened file item. You can achieve
|||	    the same thing on your own by sending a FILECMD_GETPATH IOReq
|||	    to the file.
|||
|||	  Arguments
|||
|||	    file
|||	        A file item as obtained from the File folio's OpenFile()
|||	        function.
|||
|||	    path
|||	        A pointer to a memory buffer to hold the resulting
|||	        path name.
|||
|||	    numBytes
|||	        The number of bytes available in the path buffer.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes in the resulting path, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    AppendPath(), FindFinalComponent()
|||
**/

/**
|||	AUTODOC -class fsutils -name CreateCopyObj
|||	Creates a copier object to perform hierarchical copy operations.
|||
|||	  Synopsis
|||
|||	    Err CreateCopyObj(CopyObj **co, const char *sourceDir,
|||	                      const char *objName, const TagArg *tags);
|||
|||	    Err CreateCopyObjVA(CopyObj **co, const char *sourceDir,
|||	                        const char *objName, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function creates a file copier object. The object lets you
|||	    perform file copy operations of full directory hierarchies between
|||	    any two directories or devices available.
|||
|||	    The copier object involves a set of state machines and algorithms
|||	    to minimize the number of media swaps the user has to perform
|||	    when doing copies from one card to the other on a system with a
|||	    single card slot.
|||
|||	    The copier object provides no user-interface. Whenever user
|||	    interaction is required, callback hooks are invoked. The
|||	    user of the copy object is then free to bring up any UI
|||	    component needed to effect the desired interaction.
|||
|||	    For all callback functions, a return value of 0 indicates
|||	    success, while all other values are considered requests to stop
|||	    the copy process. In such a case, the value returned by the callback
|||	    function is used as return value from the copier function itself.
|||	    For most callbacks, a return value of 0 tells the copier to retry
|||	    the operation.
|||
|||	    The copier operates in a non-recursive manner, which prevents
|||	    running out of stack space when copying a deep directory structure.
|||
|||	    This function begins the input phase of the copy operation.  It
|||	    loads as much input data into buffers as can be accomodated by
|||	    the memory threshold (or default) specified.  Even when all input
|||	    data fits in memory, the output phase of the copy does not begin
|||	    until PerformCopy() is called.  As this function executes, any
|||	    any callbacks hooks you supplied may be invoked. The QueryCopyObj()
|||	    function may be used to determine whether all input data was loaded
|||	    by this function.
|||
|||	  Arguments
|||
|||	    co
|||	        A pointer to a variable where a handle to the copier object
|||	        will be stored. The value is set to NULL if the object can't be
|||	        opened.
|||
|||	    sourceDir
|||	        A path specification denoting the location in the file
|||	        system where the object to copy is located. This path can
|||	        be absolute or relative.
|||
|||	    objName
|||	        The name of the file system object to copy. This is either
|||	        the name of a file or a directory within the directory
|||	        specified by the sourceDir argument.
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
|||	    COPIER_TAG_USERDATA (void *)
|||	        A value that the copier object will pass to all callback
|||	        functions that it calls. You can use this to supply a pointer
|||	        to your context data. If you don't supply this tag, NULL will
|||	        be passed to the callbacks.
|||
|||	    COPIER_TAG_MEMORYTHRESHOLD (uint32)
|||	        This tag lets you specify the maximum amount of memory that
|||	        the copier object will allocate. In general, this amount will
|||	        be exceeded slightly. If this tag is not provided, the default
|||	        is set to 1M.
|||
|||	    COPIER_TAG_ASYNCHRONOUS (bool)
|||	        Specifies whether the reading part of the copier should
|||	        operate asynchronously. This allows read and write operations
|||	        to overlap, which may effectively cut the time to do the
|||	        data transfer in half. More memory is needed when running
|||	        asynchronously, mainly used for a thread's stack. The default
|||	        is to run the copy synchronously.
|||
|||	    COPIER_TAG_PROGRESSFUNC (CopyProgressFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process to let you update a UI display. The
|||	        callback is supplied the current file name being worked on.
|||
|||	    COPIER_TAG_NEEDSOURCEFUNC (CopyNeedSourceFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever the copier needs to see the
|||	        source media. It lets you bring up a UI to prompt the user to
|||	        insert the media. The callback is provided the name of the
|||	        file system which is sought. If this tag is not provided and
|||	        the source media can't be found, the copy operation will stop
|||	        with a FILE_ERR_OFFLINE error.
|||
|||	    COPIER_TAG_NEEDDESTINATIONFUNC (CopyNeedDestinationFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever the copier needs to see the
|||	        destination media. It lets you bring up a UI to prompt the user
|||	        to insert the media. The callback is provided the name of the
|||	        file system which is sought. If this tag is not provided and
|||	        the destination media can't be found, the copy operation will
|||	        stop with a FILE_ERR_OFFLINE error.
|||
|||	    COPIER_TAG_DUPLICATEFUNC (CopyDuplicateFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever an object of the same name
|||	        as objName already exists in the destination directory.
|||	        The callback function can delete this duplicate object,
|||	        or rename it, or do nothing at all. If the callback returns
|||	        0, the copy operation will continue. If the duplicate not
|||	        renamed or deleted, the new object will overwrite the existing
|||	        object. In the case of directories, a merge operation will
|||	        effectively be done.
|||
|||	    COPIER_TAG_DESTINATIONFULLFUNC (CopyDestinationFullFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever the copier determines that
|||	        there isn't enough room on the destination media.
|||	        It lets you bring up a UI to inform the user of this fact.
|||	        The callback could proceed to make more room on the destination
|||	        by deleting some files, or it may just return FILE_ERR_NOSPACE
|||	        to stop the copy operation.
|||
|||	    COPIER_TAG_DESTINATIONPROTECTEDFUNC (CopyDestinationProtectedFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever the copier determines that
|||	        the destination media is write-protected. It lets you bring up
|||	        a UI to inform the user of this fact. The callback can return
|||	        0 to have the operation retried, or return FILE_ERR_READONLY
|||	        to stop the copy operation.
|||
|||	    COPIER_TAG_READERRORFUNC (CopyReadErrorFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever an unexpected error occurs
|||	        while reading the source media. The callback is supplied the
|||	        error code. If the callback can do anything to rectify the
|||	        problem, it may return 0 to have the operation retried, or
|||	        it may simply return the supplied error code to stop the copy
|||	        operation.
|||
|||	    COPIER_TAG_WRITEERRORFUN (CopyWriteErrorFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the copy process whenever an unexpected error occurs
|||	        while writing to the destination media. The callback is
|||	        supplied the error code. If the callback can do anything to
|||	        rectify the problem, it may return 0 to have the operation
|||	        retried, or  it may simply return the supplied error code to
|||	        stop the copy operation.
|||
|||	  Return Value
|||
|||	    Returns 0 if the object was created successfully, or a negative
|||	    error code for failure, or a non-zero value (positive or negative)
|||	    returned from a callback hook invoked during processing.  Any
|||	    non-zero return value implies internal cleanup has ocurred and
|||	    results in *co being set to NULL.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    DeleteCopyObj(), PerformCopy(), QueryCopyObj()
|||
**/

/**
|||	AUTODOC -class fsutils -name DeleteCopyObj
|||	Releases any resources consumed by CreateCopyObj()
|||
|||	  Synopsis
|||
|||	    Err DeleteCopyObj(CopyObj *co);
|||
|||	  Description
|||
|||	    This function releases any resources consumed by CreateCopyObj().
|||	    This might involved freeing memory, flushing I/O buffers, closing
|||	    files and more.
|||
|||	    Callbacks hooks supplied when the object was created may be called
|||	    as this function executes.
|||
|||	  Arguments
|||
|||	    co
|||	        The copy object obtained from CreateCopyObj(), or NULL in which
|||	        case this function does nothing.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    CreateCopyObj(), PerformCopy()
|||
**/

/**
|||	AUTODOC -class fsutils -name PerformCopy
|||	Enter the copier engine and start copying files.
|||
|||	  Synopsis
|||
|||	    Err PerformCopy(CopyObj *co, const char *destinationDir);
|||
|||	  Description
|||
|||	    This function invokes the output phase of the copier engine. It
|||	    tells the copy object where its data should be put. As this
|||	    function executes, any callbacks hooks you supplied when
|||	    creating the copy object may be invoked.
|||
|||	    If the CreateCopyObj() function was able to load all input data
|||	    into buffers, this function performs only output.  If all
|||	    the input data could not be loaded at once due to memory
|||	    constraints, this function internally loops through input
|||	    and output phases as needed to complete the copy operation.
|||
|||	  Arguments
|||
|||	    co
|||	       The copy object, as obtained from CreateCopyObj().
|||
|||	    destinationDir
|||	       The destination directory where the object being copied should
|||	       be put. This can be an absolute or relative path which may or
|||	       may not be on the same physical media as the object being
|||	       copied.
|||
|||	  Return Value
|||
|||	    Returns 0 if the object was created successfully, or a negative
|||	    error code for failure, or a non-zero value (positive or negative)
|||	    returned from a callback hook invoked during processing.
|||	    Upon failure, it is possible that some files or directories might
|||	    have already been copied to the destination.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    CreateCopyObj(), DeleteCopyObj()
|||
**/

/**
|||	AUTODOC -class fsutils -name QueryCopyObj
|||	Return information about the state of a copier engine.
|||
|||	  Synopsis
|||
|||	    Err QueryCopyObj(CopyObj *co, const TagArg *tags);
|||
|||	    Err QueryCopyObjVA(CopyObj *co, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function returns information about the internal state of
|||	    a copier engine.  When used after CreateCopyObj() it can
|||	    indicate whether all input data was loaded into buffers by
|||	    the CreateCopyObj() function, and provide information on
|||	    on how much data was loaded.  When used after PerformCopy()
|||	    it can indicate final statistics on the data processed.
|||
|||	    When the ISREADDONE value is zero following CreateCopyObj(),
|||	    the statistical information indicates the input progress made
|||	    up to the point where the memory threshold was reached. In this
|||	    case, PerformCopy() will have to loop through both input and output
|||	    phases to complete the copy.
|||
|||	    When the ISREADDONE value is non-zero following CreateCopyObj(),
|||	    PerformCopy() will perform only the output phase.  In this case,
|||	    the CopyNeedSourceFunc callback hook will not be invoked only during
|||	    the PerformCopy() operation.  (I.E. at most, only one card swap, from
|||	    source to dest, will be needed to complete the copy.)
|||
|||	    The ISREADDONE value will always be non-zero after PerformCopy(),
|||	    and the statistical information reflects the results of the entire
|||	    copy operation.
|||
|||	  Arguments
|||
|||	    co
|||	       The copy object, as obtained from CreateCopyObj().
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function. The array must be terminated with TAG_END.
|||
|||	    COPIER_TAG_ISREADDONE (uint32 *)
|||	        This returns zero if all input data has been loaded into
|||	        memory, or non-zero if more data remains to be read.
|||
|||	    COPIER_TAG_DIRCOUNT (uint32 *)
|||	        This returns the number of directories processed.
|||
|||	    COPIER_TAG_FILECOUNT (uint32 *)
|||	        This returns the number of files processed.
|||
|||	    COPIER_TAG_FILEBYTES (uint32 *)
|||	        This returns the accumulated size in bytes of all the files
|||	        processed.  It does not include overhead for the file entries
|||	        in the directories, only the size of the files' contents.  (I.E.,
|||	        it cannot be used as a 100% accurate indicator of whether all the
|||	        input data will fit on an output medium with a given free space.)
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    CreateCopyObj(), PerformCopy(), DeleteCopyObj()
|||
**/

/**
|||	AUTODOC -class fsutils -name DeleteTree
|||	Deletes a complete filesystem directory tree.
|||
|||	  Synopsis
|||
|||	    Err DeleteTree(const char *path, const TagArg *tags);
|||	    Err DeleteTreeVA(const char *path, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you delete an entire directory tree.
|||	    The deletion is done without recursion, which avoids running
|||	    out of stack space on deep directories.
|||
|||	    The deleter engine provides no user-interface. Whenever user
|||	    interaction is required, callback hooks are invoked. The
|||	    user of the deleter engine is then free to bring up any UI
|||	    component needed to effect the desired interaction.
|||
|||	    The callbacks are invoked with different types of information,
|||	    depending on the callback. The functions can return 0 to tell
|||	    the deleter engine to retry the operation, 1 to tell the deleter
|||	    to skip the current file or directory and move on, or any other
|||	    value to cause the deleter to exit.
|||
|||	  Arguments
|||
|||	    path
|||	        The file system component to delete. This can be a file or
|||	        a directory. If it is a directory, an attempt will made to
|||	        delete all nested objects before deleting the directory itself.
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
|||	    DELETER_TAG_USERDATA (void *)
|||	        A value that the deleter engine will pass to all callback
|||	        functions that it calls. You can use this to supply a pointer
|||	        to your context data. If you don't supply this tag, NULL will
|||	        be passed to the callbacks.
|||
|||	    DELETER_TAG_PROGRESSFUNC (DeleteProgressFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the deletion process to let you update a UI display. The
|||	        callback is supplied the current file name being deleted.
|||
|||	    DELETER_TAG_NEEDMEDIAFUNC (DeleteNeedMediaFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the deletion process whenever the deleter needs to see
|||	        the media. It lets you bring up a UI to prompt the user to
|||	        insert the media. The callback is provided the name of the
|||	        file system which is sought. If this tag is not provided and
|||	        the media can't be found, the copy operation will stop
|||	        with a FILE_ERR_OFFLINE error.
|||
|||	    DELETER_TAG_MEDIAPROTECTEDFUNC (DeleteMediaProtectedFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the deletion process whenever the deleter determines that
|||	        the media is write-protected. It lets you bring up
|||	        a UI to inform the user of this fact. The callback can return
|||	        0 to have the operation retried, or return FILE_ERR_READONLY
|||	        to stop the deletion operation.
|||
|||	    DELETER_TAG_ERRORFUNC (DeleteErrorFunc)
|||	        This tag lets you supply a callback function that is called
|||	        during the deletion process whenever an unexpected error occurs
|||	        accessing the media. The callback is supplied the
|||	        error code. If the callback can do anything to rectify the
|||	        problem, it may return 0 to have the operation retried, or
|||	        it may simply return the supplied error code to stop the
|||	        deletion operation.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Upon failure, files might have already been deleted from the
|||	    directory tree.
|||
|||	  Implementation
|||
|||	    Folio call implemented in FSUtils Folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fsutils.h>, System.m2/Modules/fsutils
|||
|||	  See Also
|||
|||	    DeleteFile(), DeleteDirectory()
|||
**/

/* keep the compiler happy... */
extern int foo;
