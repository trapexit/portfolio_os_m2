/* @(#) autodocs.c 96/09/17 1.6 */

/**
|||	AUTODOC -class Kernel -group Loader -name ImportByName
|||	Loads and resolves the named exportng module.
|||
|||	  Synopsis
|||
|||	    Item ImportByName( Item module, const char *name )
|||
|||	  Description
|||
|||	    Loads and resolves the named exporting module.
|||
|||	  Arguments
|||
|||	    module
|||	        The item number of the importing module.
|||
|||	    name
|||	        The name of the exporting module.
|||
|||	  Return Value
|||
|||	    Returns the item module which was loaded, or an error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>, libc.a
|||
|||	  See Also
|||
|||	    UnimportByName()
|||
**/

/**
|||	AUTODOC -class Kernel -group Loader -name UnimportByName
|||	Unloads a named module
|||
|||	  Synopsis
|||
|||	    Err UnimportByName( Item module, const char *name )
|||
|||	  Description
|||
|||	    This function unloads the named module.
|||
|||	  Arguments
|||
|||	    module
|||	        The item number of the importing module.
|||
|||	    name
|||	        The name of the exporting module.
|||
|||	  Return Value
|||
|||	    Returns an error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>, libc.a
|||
|||	  See Also
|||
|||	    ImportByName()
|||
**/

/**
|||	AUTODOC -class Kernel -group Loader -name OpenModule
|||	Opens an executable file from disk and prepares it for use.
|||
|||	  Synopsis
|||
|||	    Item OpenModule(const char *path, OpenModuleTypes type,
|||	                    const TagArg *tags);
|||
|||	  Description
|||
|||	    This function loads an executable file from disk into memory. Once
|||	    loaded, the code can be spawned as a thread, or executed as a
|||	    subroutine.
|||
|||	    Give this function the name of the executable file to load, and
|||	    how to load it into memory. It builds a module item that represents
|||	    the loaded code, and returns it to you.
|||
|||	    Once you finish using the loaded code, you can remove it from
|||	    memory by using CloseModule().
|||
|||	    To execute the loaded code, you can call either
|||	    ExecuteModule(), CreateModuleThread(), or CreateTask()
|||
|||	  Arguments
|||
|||	    path
|||	        The file system pathname of the executable to open.
|||
|||	    type
|||	        How the memory should be allocated for the module. If this
|||	        value if OPENMODULE_FOR_THREAD, then the memory is allocated
|||	        within the current task's pages. If this value is
|||	        OPENMODULE_FOR_TASK, then the memory is allocated from new
|||	        pages.
|||
|||	    tags
|||	        This must currently be NULL.
|||
|||	  Return Value
|||
|||	    Returns the item number of the newly opened module, or a negative
|||	    error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>, libc.a
|||
|||	  See Also
|||
|||	    CloseModule(), ExecuteModule(), CreateModuleThread(), system()
|||
**/

/**
|||	AUTODOC -class Kernel -group Loader -name CloseModule
|||	Concludes use of a module item.
|||
|||	  Synopsis
|||
|||	    Err CloseModule(Item module);
|||
|||	  Description
|||
|||	    Closes a module item previously opened with OpenModule(). Once
|||	    the module is closed, it can no longer be used, as the system may
|||	    unload it from memory.
|||
|||	  Arguments
|||
|||	    module
|||	        The module's item number, as obtained from OpenModule().
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BADITEM
|||	        The supplied module argument is not a valid item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>, libc.a
|||
|||	  See Also
|||
|||	    OpenModule(), ExecuteModule(), CreateModuleThread()
|||
**/

/**
|||	AUTODOC -class Kernel -group Loader -name ExecuteModule
|||	Executes code in a module item as a subroutine.
|||
|||	  Synopsis
|||
|||	    int32 ExecuteModule(Item module, uint32 argc, char **argv);
|||
|||	  Description
|||
|||	    This function lets you execute a chunk of code that was previously
|||	    loaded from disk using OpenModule(). The code will run as a
|||	    subroutine of the current task or thread.
|||
|||	    The argc and argv parameters are passed directly to the main()
|||	    entry point of the loaded code. The return value of this
|||	    function is the value returned by main() of the code being run.
|||
|||	    The values you supply for argc and argv are irrelevant to this
|||	    function. They are simply passed through to the loaded code.
|||	    Therefore, their meaning must be agreed upon by the caller of this
|||	    function, and by the loaded code.
|||
|||	  Arguments
|||
|||	    module
|||	        The item for the loaded code as obtained from OpenModule().
|||
|||	    argc
|||	        A value that is passed directly as the argc parameter to the
|||	        loaded code's main() entry point. This function doesn't use the
|||	        value of this argument, it is simply passed through to the
|||	        loaded code.
|||
|||	    argv
|||	        A value that is passed directly as the argv parameter to the
|||	        loaded code's main() entry point. This function doesn't use the
|||	        value of this argument, it is simply passed through to the
|||	        loaded code.
|||
|||	  Return Value
|||
|||	    Returns the value that the loaded code's main() function returns,
|||	    or BADITEM if the supplied module item is invalid.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>, libc.a
|||
|||	  See Also
|||
|||	    OpenModule(), CloseModule(), CreateModuleThread()
|||
**/

/**
|||	AUTODOC -class Kernel -group Loader -name FindCurrentModule
|||	Returns the item number of the module from which the function call
|||	was made.
|||
|||	  Synopsis
|||
|||	    Item FindCurrentModule(void);
|||
|||	  Description
|||
|||	    This function returns the item number of the module that contains
|||	    the call to this function. That is, the function asks the "who am I"
|||	    question and gets in response the caller's module number.
|||
|||	  Return Value
|||
|||	    The item number of the module that contains the call to this
|||	    function.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V32.
|||
|||	  Associated Files
|||
|||	    <loader/loader3do.h>
|||
|||	  See Also
|||
|||	    ImportByName(), UnimportByName()
|||
**/

/* keep the compiler happy... */
extern int foo;
