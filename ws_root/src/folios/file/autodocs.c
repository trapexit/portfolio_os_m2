/* @(#) autodocs.c 96/09/05 1.38 */

/**
|||	AUTODOC -class File -group Filesystem -name FormatFileSystem
|||	Formats media as a filesystem.
|||
|||	  Synopsis
|||
|||	    Err FormatFileSystem(Item device, const char *name,
|||	                         const TagArg *tags);
|||
|||	    Err FormatFileSystemVA(Item device, const char *name,
|||	                           uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function does the work needed to prepare new media for use
|||	    as a filesystem. Any data existing on the media is erased and
|||	    replaced by the new empty filesystem structure.
|||
|||	  Arguments
|||
|||	    device
|||	        A device item on the media to be formatted.
|||
|||	    name
|||	        The name the file system should be given. This is will be
|||	        used later on when referencing this file system.
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
|||	    FORMATFS_TAG_FSNAME (const char *)
|||	        This specifies the name of the type of filesystem to use to
|||	        format the media. Valid names are "acrobat.fs", "opera.fs",
|||	        or "host.fs". If this tag is not supplied, the preferred
|||	        filesystem for the target device is used.
|||
|||	    FORMATFS_TAG_OFFSET (uint32)
|||	        Lets you specify the block offset within the media where the
|||	        filesystem is to start. This is not normally needed and
|||	        defaults to 0.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V33.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <file/filefunctions.h>
|||
**/

/**
|||	AUTODOC -class File -group Directory -name CloseDirectory
|||	Closes a directory.
|||
|||	  Synopsis
|||
|||	    void CloseDirectory(Directory *dir);
|||
|||	  Description
|||
|||	    This function closes a directory that was previously opened using
|||	    OpenDirectoryItem() or OpenDirectoryPath(). All resources get
|||	    released.
|||
|||	  Arguments
|||
|||	    dir
|||	        A pointer to the directory structure for the directory to close.
|||	        This may be NULL in which case this function does nothing.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/directoryfunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenDirectoryItem(), OpenDirectoryPath(), ReadDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name OpenDirectoryItem
|||	Opens a directory specified by an item.
|||
|||	  Synopsis
|||
|||	    Directory *OpenDirectoryItem(Item openFileItem);
|||
|||	  Description
|||
|||	    This function opens a directory. It allocates a new directory
|||	    structure, opens the directory, and prepares for a traversal of the
|||	    contents of the directory. Unlike OpenDirectoryPath(), you specify
|||	    the file for the directory by its item number rather than by its
|||	    pathname.
|||
|||	  Arguments
|||
|||	    openFileItem
|||	        The item number of the open file to use for the directory.
|||	        When you later call CloseDirectory(), this file item will
|||	        automatically be freed for you, you do not need to call
|||	        CloseFile().
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the directory structure that is
|||	    created or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Caveats
|||
|||	    When you are done scanning the directory and call CloseDirectory(),
|||	    the item you gave to OpenDirectoryItem() will automatically be
|||	    closed for you. In essence, when you call OpenDirectoryItem(), you
|||	    are giving away the File item to the folio, which will dispose of it
|||	    when the directory is closed.
|||
|||	  Associated Files
|||
|||	    <file/directoryfunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenFile(), OpenFileInDir(), OpenDirectoryPath(),
|||	    CloseDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name OpenDirectoryPath
|||	Opens a directory specified by a pathname.
|||
|||	  Synopsis
|||
|||	    Directory *OpenDirectoryPath( const char *path )
|||
|||	  Description
|||
|||	    This function opens a directory. It allocates a new directory
|||	    structure, opens the directory, and prepares for a traversal of the
|||	    contents of the directory. Unlike OpenDirectoryItem(), you specify
|||	    the file for the directory by its pathname rather than by its item
|||	    number.
|||
|||	  Arguments
|||
|||	    path
|||	        An absolute or relative pathname for the file to scan.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the directory structure that is
|||	    created or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/directoryfunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenFile(), OpenFileInDir(), OpenDirectoryItem(),
|||	    CloseDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name ReadDirectory
|||	Reads the next entry from a directory.
|||
|||	  Synopsis
|||
|||	    Err ReadDirectory(Directory *dir, DirectoryEntry *de);
|||
|||	  Description -enumerated
|||
|||	    This routine reads the next entry from the specified directory. It
|||	    stores the information from the directory entry into the supplied
|||	    DirectoryEntry structure. You can then examine the DirectoryEntry
|||	    structure for information about the entry.
|||
|||	    The most interesting fields in the DirectoryEntry structure are:
|||
|||	    de_FileName
|||	        The name of the entry.
|||
|||	    de_Flags
|||	        This contains a series of bit flags that describe
|||	        characteristics of the entry. Flags of interest are
|||	        FILE_IS_DIRECTORY, which indicates the entry is a nested
|||	        directory and FILE_IS_READONLY, which tells you the file cannot
|||	        be written to.
|||
|||	    de_Type
|||	        This is currently one of FILE_TYPE_DIRECTORY, FILE_TYPE_LABEL,
|||	        or FILE_TYPE_CATAPULT.
|||
|||	    de_BlockSize
|||	        This is the size in bytes of the blocks when reading this entry.
|||
|||	    de_ByteCount
|||	        The logical count of the number of useful bytes within the
|||	        blocks allocated for this file.
|||
|||	    de_BlockCount
|||	        The number of blocks allocated for this file.
|||
|||	    You can use OpenDirectoryPath() and ReadDirectory() to scan the list
|||	    of mounted file systems. This is done by supplying a path of "/" to
|||	    OpenDirectoryPath(). The entries that ReadDirectory() returns will
|||	    correspond to all of the mounted file systems. You can then look at
|||	    the de_Flags field to determine if a file system is readable or not.
|||
|||	  Arguments
|||
|||	    dir
|||	        A pointer to the directory structure for the directory.
|||
|||	    de
|||	        A pointer to a DirectoryEntry structure in which to receive
|||	        the information about the next directory entry.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    You will get a negative result when you reach the end of he
|||	    directory.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/directoryfunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenDirectoryItem(), OpenDirectoryPath()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name ChangeDirectory
|||	Changes the current directory.
|||
|||	  Synopsis
|||
|||	    Item ChangeDirectory(const char *path);
|||
|||	  Description
|||
|||	    Changes the current task's working directory to the absolute or
|||	    relative location specified by the path, and returns the item
|||	    number for the directory.
|||
|||	  Arguments
|||
|||	    path
|||	        An absolute or relative pathname for the new current directory.
|||
|||	  Return Value
|||
|||	    Returns the item number of the new directory or a negative
|||	    error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    GetDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name ChangeDirectoryInDir
|||	Changes the current directory relative to another directory.
|||
|||	  Synopsis
|||
|||	    Item ChangeDirectoryInDir(Item dirItem, const char *path);
|||
|||	  Description
|||
|||	    Changes the current task's working directory to the absolute or
|||	    relative location specified by the path, and returns the item
|||	    number for the directory.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The directory relative to which the pathname should be
|||	        interpreted.
|||
|||	    path
|||	        An absolute or relative pathname for the new current directory.
|||
|||	  Return Value
|||
|||	    Returns the item number of the new directory or a negative
|||	    error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    GetDirectory()
|||
**/

/**
|||	AUTODOC -class File -group File -name CloseFile
|||	Closes a file.
|||
|||	  Synopsis
|||
|||	    int32 CloseFile(Item fileItem);
|||
|||	  Description
|||
|||	    Closes a disk file that was opened with a call to OpenFile() or
|||	    OpenFileInDir(). The specified item may not be used after
|||	    successful completion of this call.
|||
|||	  Arguments
|||
|||	    fileItem
|||	        The item number of the disk file to close.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenFile(), OpenFileInDir()
|||
**/


/**
|||	AUTODOC -class File -group File -name Rename
|||	Rename a file, directory, or filesystem.
|||
|||	  Synopsis
|||
|||	    Err Rename(const char *path, const char *newName);
|||
|||	  Description
|||
|||	    Rename changes the name of a file, directory or filesystem.
|||	    If the path is a file, the file name is replaced with newName.
|||	    If the path is a directory, the directory name is replaced with
|||	    newName. If the path is the root directory of the filesystem,
|||	    then the name of the filesystem is replaced with newName.
|||	    Currently, only acrobat filesystem allows this call.
|||
|||	  Arguments
|||
|||	    path
|||	        The path to the file or directory to be renamed.
|||
|||	    newName
|||	        The name of the file or directory to change to. This must
|||	        be a valid file name.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    DeleteFile(), OpenFile(), OpenFileInDir()
|||
**/

/**
|||	AUTODOC -private -class File -group File -name CreateAlias
|||	Creates a file system alias.
|||
|||	  Synopsis
|||
|||	    Item CreateAlias(const char *aliasPath, const char *realPath);
|||
|||	  Description
|||
|||	    This function creates an alias for a file. The alias can be used in
|||	    place of the full pathname for the file for any task in the system.
|||
|||	  Arguments
|||
|||	    aliasPath
|||	        The alias name.
|||
|||	    realPath
|||	        The substitution string for the alias.
|||
|||	  Return Value
|||
|||	    Returns the item number for the file alias that is created, or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenFile(), OpenFileInDir()
|||
**/

/**
|||	AUTODOC -class File -group File -name CreateFile
|||	Creates a file.
|||
|||	  Synopsis
|||
|||	    Err CreateFile(const char *path);
|||
|||	  Description
|||
|||	    This function creates a new file.
|||
|||	  Arguments
|||
|||	    path
|||	        The pathname for the file.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    DeleteFile(), OpenFile(), OpenFileInDir()
|||
**/

/**
|||	AUTODOC -class File -group File -name CreateFileInDir
|||	Creates a file relative to a directory.
|||
|||	  Synopsis
|||
|||	    Err CreateFileInDir(Item dirItem, const char *path);
|||
|||	  Description
|||
|||	    This function creates a new file.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The directory relative to which the pathname should be
|||	        interpreted.
|||
|||	    path
|||	        The absolute or relative pathname for the file.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    DeleteFile(), OpenFile(), OpenFileInDir()
|||
**/

/**
|||	AUTODOC -class File -group File -name DeleteFile
|||	Deletes a file.
|||
|||	  Synopsis
|||
|||	    Err DeleteFile(const char *path);
|||
|||	  Description
|||
|||	    This function deletes a file.
|||
|||	  Arguments
|||
|||	    path
|||	        The pathname for the file to delete.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the file was successfully deleted or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CreateFile(), OpenFile(), OpenFileInDir(), CloseFile()
|||
**/

/**
|||	AUTODOC -class File -group File -name DeleteFileInDir
|||	Deletes a file relative to a directory.
|||
|||	  Synopsis
|||
|||	    Err DeleteFileInDir(Item dirItem, const char *path);
|||
|||	  Description
|||
|||	    This function deletes a file.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The directory relative to which the pathname should be
|||	        interpreted.
|||
|||	    path
|||	        The absolute or relative pathname for the file to delete.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the file was successfully deleted or a
|||	    negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CreateFile(), OpenFile(), OpenFileInDir(), CloseFile()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name CreateDirectory
|||	Creates a directory.
|||
|||	  Synopsis
|||
|||	    Err CreateDirectory(const char *path);
|||
|||	  Description
|||
|||	    This function creates a new directory.
|||
|||	  Arguments
|||
|||	    path
|||	        The pathname for the new directory.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    DeleteDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name CreateDirectoryInDir
|||	Creates a directory relative to another directory.
|||
|||	  Synopsis
|||
|||	    Err CreateDirectoryInDir(Item ditItem, const char *path);
|||
|||	  Description
|||
|||	    This function creates a new directory.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The directory relative to which the pathname should be
|||	        interpreted.
|||
|||	    path
|||	        The absolute or relative pathname for the new directory.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    DeleteDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name DeleteDirectory
|||	Deletes a directory.
|||
|||	  Synopsis
|||
|||	    Err DeleteDirectory(const char *path);
|||
|||	  Description
|||
|||	    This function deletes an existing directory.
|||
|||	  Arguments
|||
|||	    path
|||	        The pathname for the directory to delete.
|||
|||	  Return Value
|||
|||	    Returns 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CreateDirectory()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name DeleteDirectoryInDir
|||	Deletes a directory relative to another directory.
|||
|||	  Synopsis
|||
|||	    Err DeleteDirectoryInDir(Item dirItem, const char *path);
|||
|||	  Description
|||
|||	    This function deletes an existing directory.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The directory relative to which the pathname should be
|||	        interpreted.
|||
|||	    path
|||	        The absolute or relative pathname for the directory to delete.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CreateDirectory()
|||
**/

/**
|||	AUTODOC -class File -group File -name FindFileAndOpen
|||	Searches one or more locations for a file, and opens the file.
|||
|||	  Synopsis
|||
|||	    Item FindFileAndOpen(const char *partialPath, TagArg *tags);
|||
|||	    Item FindFileAndOpenVA(const char *partialPath, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function searches one or more locations for a file.  Tags
|||	    are used to identify the locations to be searched, to require that
|||	    the version and/or revision of the file be screened against a
|||	    set of suitability criteria, and whether the search should scan
|||	    all locations for the "most recent" version/revision of the file
|||	    or should stop upon finding the first match.
|||
|||	  Arguments
|||
|||	    partialPath
|||	        A relative path, which should be interpreted relative to each
|||	        of the search locations identified in the tag list.
|||
|||	    tags
|||	        A tagarg array, where the tags specify version and revision
|||	        matching criteria, how to handle files having no valid
|||	        version and revision numbers, whether to stop on the first
|||	        file found or scan exhaustively for the highest version,
|||	        and the identity of the directories to be searched.
|||	        See the FILESEARCH_TAG tags in <file/filesystem.h>
|||
|||	  Return Value
|||
|||	    Returns the item number of the opened file (which can be used
|||	    later to refer to the file), or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V29.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    FindFileAndIdentify()
|||
**/

/**
|||	AUTODOC -class File -group File -name FindFileAndIdentify
|||	Searches one or more locations for a file, and returns its pathname
|||
|||	  Synopsis
|||
|||	    Err FindFileAndIdentify(char *pathBuf, int32 pathBufLen,
|||	                            const char *partialPath, TagArg *tags);
|||
|||	    Err FindFileAndIdentifyVA(char *pathBuf, int32 pathBufLen,
|||	                              const char *partialPath, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function searches one or more locations for a file.  Tags
|||	    are used to identify the locations to be searched, to require that
|||	    the version and/or revision of the file be screened against a
|||	    set of suitability criteria, and whether the search should scan
|||	    all locations for the "most recent" version/revision of the file
|||	    or should stop upon finding the first match.
|||
|||	  Arguments
|||
|||	    pathBuf
|||	        A buffer into which the call may return the complete (absolute)
|||	        pathname of the file which was found.
|||
|||	    pathBufLen
|||	        The size of the pathBuf buffer.
|||
|||	    partialPath
|||	        A relative path, which should be interpreted relative to each
|||	        of the search locations identified in the tag list.
|||
|||	    tags
|||	        A tagarg array, where the tags specify version and revision
|||	        matching criteria, how to handle files having no valid
|||	        version and revision numbers, whether to stop on the first
|||	        file found or scan exhaustively for the highest version,
|||	        and the identity of the directories to be searched.
|||	        See the FILESEARCH_TAG tags in <file/filesystem.h>
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the file is found and its name has been returned
|||	    in pathBuf, or returns a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V29.
|||
|||	  Associated Files
|||
|||	    <file/filesystem.h>, <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    FindFileAndOpen()
|||
**/

/**
|||	AUTODOC -class File -group Directory -name GetDirectory
|||	Gets the item number and pathname for the current directory.
|||
|||	  Synopsis
|||
|||	    Item GetDirectory(char *pathBuf, int32 pathBufLen);
|||
|||	  Description
|||
|||	    This function returns the item number of the calling task's current
|||	    directory. If pathBuf is non-NULL, it must point to a buffer of
|||	    writable memory whose length is given in pathBufLen; the absolute
|||	    pathname of the current working directory is stored into this
|||	    buffer.
|||
|||	  Arguments
|||
|||	    pathBuf
|||	        A pointer to a buffer in which to receive the absolute pathname
|||	        for the current directory. If you do not want to get the
|||	        pathname string, use NULL as the value of this argument.
|||
|||	    pathBufLen
|||	        The size of the buffer pointed to by the pathBuf argument, in
|||	        bytes, or zero if you don't provide a buffer.
|||
|||	  Return Value
|||
|||	    Returns the item number of the current directory.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    ChangeDirectory()
|||
**/

/**
|||	AUTODOC -class File -group File -name OpenFile
|||	Opens a disk file.
|||
|||	  Synopsis
|||
|||	    Item OpenFile(const char *path);
|||
|||	  Description
|||
|||	    This function opens a disk file, given an absolute or relative
|||	    pathname, and returns its item number.
|||
|||	  Arguments
|||
|||	    path
|||	        An absolute or relative pathname for the file to
|||	        open, or an alias for the pathname.
|||
|||	  Return Value
|||
|||	    Returns the item number of the opened file (which can be used
|||	    later to refer to the file), or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CloseFile(), OpenFileInDir()
|||
**/

/**
|||	AUTODOC -class File -group File -name OpenFileInDir
|||	Opens a disk file relative to a directory.
|||
|||	  Synopsis
|||
|||	    Item OpenFileInDir(Item dirItem, const char *path);
|||
|||	  Description
|||
|||	    Similar to OpenFile(), this function allows the caller to
|||	    specify the item of a directory that should serve as a starting
|||	    location for the file search.
|||
|||	  Arguments
|||
|||	    dirItem
|||	        The item number of the directory containing the file.
|||
|||	    path
|||	        A pathname for the file that is relative to the directory
|||	        specified by the dirItem argument.
|||
|||	  Return Value
|||
|||	    Returns the item number of the opened file (which can be used
|||	    later to refer to the file), or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V20.
|||
|||	  Associated Files
|||
|||	    <file/filefunctions.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenFile(), OpenDirectoryItem(), OpenDirectoryPath(),
|||	    CloseFile()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name OpenRawFile
|||	Gains access to a file for raw file I/O.
|||
|||	  Synopsis
|||
|||	    Err OpenRawFile(RawFile **file, const char *path, FileOpenModes mode);
|||
|||	  Description
|||
|||	    This function does the necessary work to establish a connection
|||	    to a file in a file system and prepare it for use. Once a file is
|||	    opened, it can be read or written to.
|||
|||	  Arguments
|||
|||	    file
|||	        A pointer to a variable where a handle to the RawFile will
|||	        be stored. The value is set to NULL if the file can't be
|||	        opened.
|||
|||	    path
|||	        The pathname leading to the file to be opened. This may be an
|||	        absolute path, or relative to the current directory.
|||
|||	    mode
|||	        The mode to open the file in. This determines what can be
|||	        done to the file, and allows I/O operations to be optimized for
|||	        the particular type of access mode.
|||
|||	    The modes supported are:
|||
|||	    FILEOPEN_READ
|||	        The file is being opened for reading only. Attempts to write to
|||	        the file will fail. If the file doesn't exist, the open
|||	        call will fail.
|||
|||	    FILEOPEN_WRITE
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, it will be
|||	        created. If the file already exists, the previous contents are
|||	        left intact, and the file cursor is positioned at the beginning
|||	        of the file.
|||
|||	    FILEOPEN_WRITE_NEW
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, it will be
|||	        created. If the file already exists, it is first deleted, and
|||	        then recreated, which erases any previous contents.
|||
|||	    FILEOPEN_WRITE_EXISTING
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, the open
|||	        call will fail. If the file already exists, the previous
|||	        contents are left intact, and the file cursor is positioned at
|||	        the beginning of the file.
|||
|||	    FILEOPEN_READWRITE
|||	        Same as FILEOPEN_WRITE, except that the file can also be read.
|||
|||	    FILEOPEN_READWRITE_NEW
|||	        Same as FILEOPEN_WRITE_NEW, except that the file can also be
|||	        read.
|||
|||	    FILEOPEN_READWRITE_EXISTING
|||	        Same as FILEOPEN_WRITE_EXISTING, except that the file can also
|||	        be read.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_NOFILE
|||	        No file of the specified name could be found.
|||
|||	    FILE_ERR_NOMEM
|||	        There was not enough memory to allocate the resources needed.
|||
|||	    FILE_ERR_BADMODE
|||	        An illegal open mode was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CloseRawFile(), ReadRawFile(), WriteRawFile(), SeekRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize(), OpenRawFileInDir()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name OpenRawFileInDir
|||	Gains access to a file for raw file I/O.
|||
|||	  Synopsis
|||
|||	    Err OpenRawFileInDir(RawFile **file, Item dirItem, const char *path,
|||	                         FileOpenModes mode);
|||
|||	  Description
|||
|||	    This function does the necessary work to establish a connection
|||	    to a file in a file system and prepare it for use. Once a file is
|||	    opened, it can be read or written to.
|||
|||	  Arguments
|||
|||	    file
|||	        A pointer to a variable where a handle to the RawFile will
|||	        be stored. The value is set to NULL if the file can't be
|||	        opened.
|||
|||	    dirItem
|||	        The item number of the directory containing the file.
|||
|||	    path
|||	        A pathname for the file that is relative to the directory
|||	        specified by the dirItem argument.
|||
|||	    mode
|||	        The mode to open the file in. This determines what can be
|||	        done to the file, and allows I/O operations to be optimized for
|||	        the particular type of access mode.
|||
|||	    The modes supported are:
|||
|||	    FILEOPEN_READ
|||	        The file is being opened for reading only. Attempts to write to
|||	        the file will fail. If the file doesn't exist, the open
|||	        call will fail.
|||
|||	    FILEOPEN_WRITE
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, it will be
|||	        created. If the file already exists, the previous contents are
|||	        left intact, and the file cursor is positioned at the beginning
|||	        of the file.
|||
|||	    FILEOPEN_WRITE_NEW
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, it will be
|||	        created. If the file already exists, it is first deleted, and
|||	        then recreated, which erases any previous contents.
|||
|||	    FILEOPEN_WRITE_EXISTING
|||	        The file is being opened for writing only. Attempts to read
|||	        from the file will fail. If the file doesn't exist, the open
|||	        call will fail. If the file already exists, the previous
|||	        contents are left intact, and the file cursor is positioned at
|||	        the beginning of the file.
|||
|||	    FILEOPEN_READWRITE
|||	        Same as FILEOPEN_WRITE, except that the file can also be read.
|||
|||	    FILEOPEN_READWRITE_NEW
|||	        Same as FILEOPEN_WRITE_NEW, except that the file can also be
|||	        read.
|||
|||	    FILEOPEN_READWRITE_EXISTING
|||	        Same as FILEOPEN_WRITE_EXISTING, except that the file can also
|||	        be read.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_NOFILE
|||	        No file of the specified name could be found.
|||
|||	    FILE_ERR_NOMEM
|||	        There was not enough memory to allocate the resources needed.
|||
|||	    FILE_ERR_BADMODE
|||	        An illegal open mode was supplied.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    CloseRawFile(), ReadRawFile(), WriteRawFile(), SeekRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize(), OpenRawFile()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name CloseRawFile
|||	Concludes access to a file.
|||
|||	  Synopsis
|||
|||	    Err CloseRawFile(RawFile *file);
|||
|||	  Description
|||
|||	    This function concludes access to a file. It flushes any
|||	    buffers that are dirty and releases all resources maintained for
|||	    the file.
|||
|||	    If a read, write, or seek operation failed while this file was
|||	    opened, CloseRawFile() will return the error code associated with
|||	    the failure, unless ClearRawFileError() was used to reset the error
|||	    state.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to close. This value may be NULL, in which case this
|||	        function does nothing.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    If a read, write, or seek operation failed while this file
|||	    was opened, the error code associated with this failure will be
|||	    returned by this function, unless ClearRawFileError() is used to
|||	    clear the error state.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), ReadRawFile(), WriteRawFile(), SeekRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name ReadRawFile
|||	Reads data from a file.
|||
|||	  Synopsis
|||
|||	    int32 ReadRawFile(RawFile *file, void *buffer, int32 numBytes);
|||
|||	  Description
|||
|||	    This function reads data from a file starting at the current file
|||	    cursor position. The data is copied into the supplied buffer. Once
|||	    the data is read, the file cursor is advanced by the number of
|||	    bytes read. This causes sequential read operations to progress
|||	    through all the data in the file.
|||
|||	    If there is an error while reading the file, an error code will
|||	    be returned, and the contents of the supplied buffer will be
|||	    undefined. Once an error occurs for a file, all subsequent
|||	    operations to that file are blocked and will all return errors.
|||	    When you finally close the file, CloseRawFile() will return the
|||	    error code which describes the failure. You can use
|||	    ClearRawFileError() to reset the error state and once again allow
|||	    I/O operations to be performed to the file.
|||
|||	    When reading a file, it is slightly more efficient to supply
|||	    a read buffer which is an integral multiple in size of the file's
|||	    block size. You can obtain the file's block size using the
|||	    GetRawFileInfo() function.
|||
|||	  Note
|||
|||	    The data returned by ReadRawFile() may not be present in physical
|||	    memory and may only appear in the CPU cache. Therefore, before
|||	    processing data read from a file using a DMA device, you must
|||	    first write back the data to main memory using WriteBackDCache()
|||	    or FlushDCacheAll().
|||
|||	  Arguments
|||
|||	    file
|||	        The file to read from.
|||
|||	    buffer
|||	        A memory buffer where the data is to be copied.
|||
|||	    numBytes
|||	        The number of bytes to read from the file. If there aren't
|||	        that many bytes left, the maximum number of bytes possible
|||	        will be read.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes read into the buffer, or a negative
|||	    error code for failure. When a failure occurs, the contents of
|||	    the supplied buffer are undefined.
|||
|||	    Upon failure, the file cursor will be left in its original
|||	    position, so that if the error condition is corrected, it is
|||	    simply a matter of calling ReadRawFile() again with the same
|||	    data. For example, if the failure occured because the media
|||	    was removed, when the user reinserts the media, it is possible to
|||	    resume the reading operations right where it left off.
|||
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADFILE
|||	        A bad file pointer was passed in.
|||
|||	    FILE_ERR_BADCOUNT
|||	        A negative value was supplied for the numBytes argument.
|||
|||	    FILE_ERR_OFFLINE
|||	        The file system is no longer mounted (the user likely removed
|||	        the media).
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), WriteRawFile(), SeekRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name WriteRawFile
|||	Writes data to a file.
|||
|||	  Synopsis
|||
|||	    int32 WriteRawFile(RawFile *file, const void *buffer, int32 numBytes);
|||
|||	  Description
|||
|||	    This function writes data to a file starting at the current file
|||	    cursor position. The data is copied from the supplied buffer. Once
|||	    the data is written, the file cursor is advanced by the number of
|||	    bytes written. This causes sequential write operations to progress
|||	    forward and extend the file as new data is written.
|||
|||	    If there is an error while writing the file, an error code will
|||	    be returned, and whether any part of the data has made it to the
|||	    file is undefined. Once an error occurs for a file, all subsequent
|||	    operations to that file are blocked and will all return errors.
|||	    When you finally close the file, CloseRawFile() will return the
|||	    error code which describes the failure. You can use
|||	    ClearRawFileError() to reset the error state and once again allow
|||	    I/O operations to be performed to the file.
|||
|||	    As data is written at the end of a file, the size of the file is
|||	    automatically increased to hold the new data. If you know the
|||	    amount of data you will be writting to a file ahead of time, it
|||	    is more efficient to use the SetRawFileSize() function to
|||	    preallocate room on the media for the file. This helps reduce
|||	    media fragmentation and improve overall performance.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to write to.
|||
|||	    buffer
|||	        A memory buffer containing the data to write.
|||
|||	    numBytes
|||	        The number of bytes to write to the file.
|||
|||	  Return Value
|||
|||	    Returns the number of bytes written to the file, or a negative
|||	    error code for failure. When a failure occurs, parts of the
|||	    data may have already been written to the file.
|||
|||	    Upon failure, the file cursor will be left in its original
|||	    position, so that if the error condition is corrected, it is
|||	    simply a matter of calling WriteRawFile() again with the same
|||	    data. For example, if the failure occured because the media
|||	    was full, it is possible to just delete some files, and resume
|||	    the writing operations right where it left off.
|||
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADFILE
|||	        A bad file pointer was passed in.
|||
|||	    FILE_ERR_BADCOUNT
|||	        A negative value was supplied for the numBytes argument.
|||
|||	    FILE_ERR_NOSPACE
|||	        There's no more room on the media.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), SeekRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name SeekRawFile
|||	Moves the file cursor within a file.
|||
|||	  Synopsis
|||
|||	    int32 SeekRawFile(RawFile *file, int32 position, FileSeekModes mode);
|||
|||	  Description
|||
|||	    This function moves the file cursor within the file. The file cursor
|||	    determines where the next read operation will get its data from,
|||	    and where the next write operation will write its data to.
|||
|||	    The file cursor can be moved to a position which is relative to
|||	    the start or end of file, as well as relative to the current cursor
|||	    position. The file cursor is never allowed to be less than 0 or
|||	    greater than the number of bytes in the file.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to affect.
|||
|||	    position
|||	        The position to move the file cursor to.
|||
|||	    mode
|||	        Describes what the position argument is relative to.
|||
|||	    The possible seek modes are:
|||
|||	    FILESEEK_START
|||	        Indicates that the supplied position is relative to the
|||	        beginning of the file. So a position of 10 would put the
|||	        file cursor at byte #10 (the eleventh byte) within the
|||	        file.
|||
|||	    FILESEEK_CURRENT
|||	        Indicates that the supplied position is relative to the current
|||	        position within the file. A positive position value moves the
|||	        cursor forward in the file by that many bytes, while a negative
|||	        value moves the cursor back by that number of bytes.
|||
|||	    FILESEEK_END
|||	        Indicates that the supplied position is relative to the end
|||	        of the file. The position value should be a negative value.
|||
|||	  Return Value
|||
|||	    Returns the previous position of the file cursor within the file
|||	    or a negative error code for failure. Upon failure, the file cursor
|||	    will not have been moved. Trying to seek beyond the bounds
|||	    of the file is not considered an error and is handled by
|||	    limiting the motion of the file cursor within allowed bounds.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), WriteRawFile(),
|||	    GetRawFileInfo(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name GetRawFileInfo
|||	Gets some information about an opened file.
|||
|||	  Synopsis
|||
|||	    Err GetRawFileInfo(RawFile *file, FileInfo *info, uint32 infoSize);
|||
|||	  Description
|||
|||	    This function gets some information about an opened file. The
|||	    information includes the size of the file in bytes and blocks,
|||	    the underlying item being used for communicating with the
|||	    file system, and more.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to get information on.
|||
|||	    info
|||	        A structure to hold the information.
|||
|||	    infoSize
|||	        The size of the information structure. This should be
|||	        sizeof(FileInfo).
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADSIZE
|||	        An illegal value was given for the infoSize argument.
|||	        The parameter should be equal to sizeof(FileInfo).
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), WriteRawFile(),
|||	    SeekRawFile(), SetRawFileAttrs(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name SetRawFileAttrs
|||	Sets some attributes of an opened file.
|||
|||	  Synopsis
|||
|||	    Err SetRawFileAttrs(RawFile *file, const TagArg *tags);
|||
|||	    Err SetRawFileAttrsVA(RawFile *file, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you set the value of fields associated
|||	    with each file. You can only call this function if the
|||	    file was opened in a writable mode.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to set the attributes of.
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
|||	    FILEATTRS_TAG_FILETYPE (PackedID)
|||	        This tag lets you specify the 4-byte file type that is
|||	        associated with every file.
|||
|||	    FILEATTRS_TAG_VERSION (uint8)
|||	        Lets you specify the version associated with the file.
|||
|||	    FILEATTRS_TAG_REVISION (uint8)
|||	        Lets you specify the revision associated with the file.
|||
|||	    FILEATTRS_TAG_BLOCKSIZE (uint32)
|||	        Lets you specify the logical block size to use for this file.
|||	        This tag can only be used if the file is currently empty.
|||
|||	    FILEATTRS_TAG_DATE (TimeVal *)
|||	        Lets you specify the modification date to assign to this file.
|||	        The supplied TimeVal structure specifies an amount of time
|||	        since 01-Jan-1993.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADFILE
|||	        A bad file pointer was passed in.
|||
|||	    FILE_ERR_BADMODE
|||	        The file was not in a valid mode to call this function.
|||
|||	    FILE_ERR_BADSIZE
|||	        An attempt was made to set the block size while the file
|||	        was not empty.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V27.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), WriteRawFile(),
|||	    SeekRawFile(), GetRawFileInfo(), ClearRawFileError(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name SetRawFileSize
|||	Sets the size of an opened file.
|||
|||	  Synopsis
|||
|||	    Err SetRawFileSize(RawFile *file, uint32 newSize);
|||
|||	  Description
|||
|||	    This function lets you set the size of a file on disk.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to set the size of.
|||
|||	    newSize
|||	        The new size in bytes of the file. If the file is being made
|||	        smaller, the file cursor is automatically moved, if needed, to
|||	        not exceed the new end of the file.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADFILE
|||	        A bad file pointer was passed in.
|||
|||	    FILE_ERR_BADMODE
|||	        The file was not in a valid mode to call this function.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), WriteRawFile(),
|||	    SeekRawFile(), GetRawFileInfo(), ClearRawFileError(),
|||	    SetRawFileAttrs()
|||
**/

/**
|||	AUTODOC -class File -group RawFile -name ClearRawFileError
|||	Clears the error state of a file.
|||
|||	  Synopsis
|||
|||	    Err ClearRawFileError(RawFile *file);
|||
|||	  Description
|||
|||	    After an error occurs while reading, writing, or seeking in a
|||	    file, further operations to that file are automatically rejected
|||	    and all return error codes. This makes it generally easy to
|||	    deal with errors since it is only necessary to check the
|||	    return status of the last I/O performed instead of all I/O calls.
|||
|||	    In some cases, it is desirable not to have errors always be fatal.
|||	    In particular, when an error condition can be corrected, it is
|||	    useful to resume I/O operations where they left off. For example,
|||	    if the error occured because the user removed the media, it is
|||	    desirable to prompt the user for the media, and resume the I/O
|||	    operation as soon as the media becomes again mounted.
|||
|||	    This function lets you clear the error state of an opened file.
|||	    This enables I/O operations to the file once again.
|||
|||	  Arguments
|||
|||	    file
|||	        The file to clear the error state for.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
|||	  See Also
|||
|||	    OpenRawFile(), CloseRawFile(), ReadRawFile(), WriteRawFile(),
|||	    SeekRawFile(), GetRawFileInfo(), SetRawFileAttrs(),
|||	    SetRawFileSize()
|||
**/

/**
|||	AUTODOC -class File -group File -name SetFileAttrs
|||	Sets some attributes of a file.
|||
|||	  Synopsis
|||
|||	    Err SetFileAttrs(const char *path, const TagArg *tags);
|||
|||	    Err SetFileAttrsVA(const char *path, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you set the value of fields associated
|||	    with each file.
|||
|||	  Arguments
|||
|||	    path
|||	        The path name leading to the file to set the attributes of.
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
|||	    FILEATTRS_TAG_FILETYPE (PackedID)
|||	        This tag lets you specify the 4-byte file type that is
|||	        associated with every file.
|||
|||	    FILEATTRS_TAG_VERSION (uint8)
|||	        Lets you specify the version associated with the file.
|||
|||	    FILEATTRS_TAG_REVISION (uint8)
|||	        Lets you specify the revision associated with the file.
|||
|||	    FILEATTRS_TAG_BLOCKSIZE (uint32)
|||	        Lets you specify the logical block size to use for this file.
|||	        This tag can only be used if the file is currently empty.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    FILE_ERR_BADFILE
|||	        A bad file pointer was passed in.
|||
|||	    FILE_ERR_BADSIZE
|||	        An attempt was made to set the block size while the file
|||	        was not empty.
|||
|||	  Implementation
|||
|||	    Folio call implemented in File folio V28.
|||
|||	  Associated Files
|||
|||	    <file/fileio.h>, <file/filesystem.h>, System.m2/Boot/filesystem
|||
**/

/* keep the compiler happy... */
extern int foo;
