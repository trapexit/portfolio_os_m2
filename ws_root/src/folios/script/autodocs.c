/* @(#) autodocs.c 96/08/02 1.13 */

/**
|||	AUTODOC -class Script -name CreateScriptContext
|||	Creates and initializes a ScriptContext structure.
|||
|||	  Synopsis
|||
|||	    Err CreateScriptContext(ScriptContext **sc, const TagArg *tags);
|||
|||	    Err CreateScriptContextVA(ScriptContext **sc, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function allocates and initializes a ScriptContext structure
|||	    which is used to preserve state information across multiple
|||	    calls to ExecuteCmdLine().
|||
|||	  Arguments
|||
|||	    sc
|||	        A pointer to a variable that will receive the ScriptContext
|||	        pointer.
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
|||	    SCRIPT_TAG_BACKGROUND_MODE (bool)
|||	        Sets the state of the background flag in the shell. When TRUE,
|||	        programs being executed will run in the background.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Script folio V27.
|||
|||	  Associated Files
|||
|||	    <misc/script.h>, System.m2/Modules/script
|||
|||	  See Also
|||
|||	    DeleteScriptContext(), ExecuteCmdLine()
|||
**/

/**
|||	AUTODOC -class Script -name DeleteScriptContext
|||	Deletes a ScriptContext structure.
|||
|||	  Synopsis
|||
|||	    Err DeleteScriptContext(ScriptContext *sc);
|||
|||	  Description
|||
|||	    This function releases any resources allocated by
|||	    CreateScriptContext().
|||
|||	  Arguments
|||
|||	    sc
|||	        A pointer to the ScriptContext structure, or NULL.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Script folio V27.
|||
|||	  Associated Files
|||
|||	    <misc/script.h>, System.m2/Modules/script
|||
|||	  See Also
|||
|||	    CreateScriptContext(), ExecuteCmdLine()
|||
**/

/**
|||	AUTODOC -class Script -name ExecuteCmdLine
|||	Executes a shell command-line.
|||
|||	  Synopsis
|||
|||	    Err ExecuteCmdLine(const char *cmdLine, int32 *pStatus, const TagArg *tags);
|||
|||	    Err ExecuteCmdLineVA(const char *cmdLine, int32 *pStatus, uint32 tag, ...);
|||
|||	  Description
|||
|||	    This function lets you execute a command-line as if it were
|||	    typed in at a shell prompt.
|||
|||	  Arguments
|||
|||	    cmdLine
|||	        The command-line to execute. This can be anything you would
|||	        type at a 3DO shell prompt.
|||
|||	    pStatus
|||	        A pointer to an int32 into which is stored the exit status
|||	        of the command-line when it terminates.  This value is
|||	        meaningful only if ExecuteCmdLine returns a value >= 0,
|||	        and if the command-line was executed in foreground mode.
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
|||	    SCRIPT_TAG_CONTEXT (ScriptContext *)
|||	        Specifies a script context structure. This is used to hold
|||	        state information accross multiple calls to this function.
|||	        If this tag is not supplied, then default values will be
|||	        used.
|||
|||	    SCRIPT_TAG_BACKGROUND_MODE (bool)
|||	        Sets the state of the background flag in the shell. When TRUE,
|||	        programs being executed will run in the background.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the cmd-line was executed, or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Script folio V27.
|||
|||	  Associated Files
|||
|||	    <misc/script.h>, System.m2/Modules/script
|||
|||	  See Also
|||
|||	    CreateScriptContext(), DeleteScriptContext(), system()
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowAvailMem
|||	Display information about the amount of memory currently available in
|||	the system.
|||
|||	  Format
|||
|||	    ShowAvailMem
|||
|||	  Description
|||
|||	    This command displays information about the amount of memory
|||	    installed in the system, and the amount of memory currently
|||	    free.
|||
|||	    This command also displays the amount of memory currently consumed
|||	    by supervisor-mode code.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    AvailMem
|||
|||	  See Also
|||
|||	    ShowMemMap(@), ShowFreeBlocks(@), ShowMemUsage(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowFreeBlocks
|||	Show the contents of a memory list.
|||
|||	  Format
|||
|||	    ShowFreeBlocks [item|name]
|||
|||	  Description
|||
|||	    This command displays all the chunks of memory currently
|||	    available in the list of pages of a task, or of the whole system.
|||	    The list of blocks displayed for the system includes all
|||	    currently unallocated blocks of memory.
|||
|||	  Arguments
|||
|||	    [task name | task item number]
|||	        This specifies the name or item number of the task to display
|||	        the data for. If this argument is not supplied, then the
|||	        data for the system free list is displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V27.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  See Also
|||
|||	    ShowMemMap(@), ShowAvailMem(@), ShowTask(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowMemMap
|||	Display a page map showing which pages of memory are used and free in
|||	the system, and which task owns which pages.
|||
|||	  Format
|||
|||	    ShowMemMap [task name | task item number]
|||
|||	  Description
|||
|||	    This command displays a page map on the debugging terminal showing
|||	    all memory pages in the system, and which task owns each
|||	    page.
|||
|||	  Arguments
|||
|||	    [task name | task item number]
|||	        This specifies the name or item number of the task to pay
|||	        special attention to. The item number can be in decimal,
|||	        or in hexadecimal starting with 0x or When a task is specified,
|||	        any pages owned by that task will be marked with a '*'. This
|||	        makes it quicker to find pages used by a specific task.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    MemMap
|||
|||	  See Also
|||
|||	    ShowAvailMem(@), ShowFreeBlocks(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowMemUsage
|||	Display an exhaustive list of the various blocks of memory
|||	in the system and what they are being used for.
|||
|||	  Format
|||
|||	    ShowMemUsage [address]
|||
|||	  Description
|||
|||	    This command displays a list of all the distinct blocks of
|||	    memory and what these blocks are currently being used for.
|||	    Following the list of blocks is a tally of the various
|||	    types of memory blocks.
|||
|||	    The command examines each area of memory and attempts to
|||	    determine what the memory is being used for currently. The
|||	    blocks are classified as:
|||
|||	    Code
|||	        PowerPC executable code. These blocks are shown with the name
|||	        of the loader module the code is associated with.
|||
|||	    Data
|||	        PowerPC executable data. These blocks hold preinitialized
|||	        variables for loaded code. These blocks are shown with the name
|||	        of the loader module the data is associated with.
|||
|||	    BSS
|||	        PowerPC executable BSS. These blocks hold uninitialized
|||	        variables for loaded code. These blocks are shown with the name
|||	        of the loader module the data is associated with.
|||
|||	    Wasted Data
|||	        Unused executable data space. The loader allocates memory to
|||	        hold a module's data and BSS section in full memory page
|||	        increments. That means that if the module only needs 20 bytes
|||	        of data space, the loader will still allocate a 4K page for it.
|||	        This number indicates the extra padding space that is currently
|||	        going unused.
|||
|||	    User Stack
|||	        Stack memory for tasks or threads. This is shown with the
|||	        name of the task or thread using the stack.
|||
|||	    Super Stack
|||	        Supervisor stack memory for tasks or threads. This is shown
|||	        with the name of the task or thread using the stack.
|||
|||	    Task Resource Table
|||	        The amount of memory used to keep track of all the items
|||	        owned or opened by tasks or threads. This is shown with the
|||	        name of the task of thread to which the table is associated.
|||
|||	    Item
|||	        Memory holding Item structures.
|||
|||	    Item Names
|||	        Memory holding name strings for items.
|||
|||	    Item Table
|||	        Memory holding a part of the item database maintained by the
|||	        kernel.
|||
|||	    Export Table
|||	        This holds information about the various symbols exported
|||	        by a modules.
|||
|||	    Import Table
|||	        This holds information about the various modules a given
|||	        module is importing.
|||
|||	    Relocation Table
|||	        This holds information needed to perform dynamic relocation
|||	        of a module.
|||
|||	    Free Block
|||	        A block of unused memory contained within the pages of
|||	        memory already assigned to a task.
|||
|||	    Unallocated Pages
|||	        Memory that is not currently in use.
|||
|||	    Dynamic Allocation
|||	        Memory block that has been allocated for some unknown
|||	        purpose.
|||
|||	    Special
|||	        Memory block dedicated to some particular system purpose.
|||	        This includes the CD-ROM I/O buffer, debugger communication
|||	        buffers, boot data space, etc.
|||
|||	  Arguments
|||
|||	    address
|||	        This optional argument lets you give a particular address in
|||	        memory, and the command will attempt to determine precisely
|||	        what is located at that address only.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V30.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    MemUsage
|||
|||	  See Also
|||
|||	    ShowMemMap(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowErr
|||	Display an error string associated with a system error code.
|||
|||	  Format
|||
|||	    ShowErr <error number>
|||
|||	  Description
|||
|||	    This command displays the error string associated with a
|||	    numerical system error code.
|||
|||	  Arguments
|||
|||	    <error number>
|||	        The error code to display the string of. This number can be in
|||	        decimal, or in hexadecimal starting with 0x or $.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V21.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    err
**/

/**
|||	AUTODOC -private -class Shell_Commands -name Alias
|||	Set a file path alias.
|||
|||	  Format
|||
|||	    Alias <alias> <str>
|||
|||	  Description
|||
|||	    This command lets you create a filesystem path alias. Once
|||	    created, the alias can be referenced from anywhere in the system.
|||
|||	  Arguments
|||
|||	    <alias>
|||	        The name of the alias to create.
|||
|||	    <str>
|||	        The string that should be substituted whenever the alias is
|||	        encountered when parsing directory and file names.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    alias
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name SetMinMem
|||	Set the amount of memory available in the system to the minimum amount
|||	of memory guaranteed to be available in a production environment.
|||
|||	  Format
|||
|||	    SetMinMem
|||
|||	  Description
|||
|||	    This command causes the shell to adjust the amount of
|||	    memory available in the system to match the minimum amount
|||	    a memory a title must be able to run in.
|||
|||	    The setmaxmem command can be used to restore the amount of
|||	    memory to the maximum available in the current system.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    MinMem
|||
|||	  See Also
|||
|||	    SetMaxMem(@)
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name SetMaxMem
|||	Set the amount of memory available in the system to the maximum amount
|||	possible.
|||
|||	  Format
|||
|||	    SetMaxMem
|||
|||	  Description
|||
|||	    This command causes the shell to adjust the amount of
|||	    memory available in the system to be the maximum supported by
|||	    the hardware. This effectively undoes a previous use of the
|||	    setminmem command.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    MaxMem
|||
|||	  See Also
|||
|||	    SetMinMem(@)
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name SetBG
|||	Set the shell's default behavior to background execution mode.
|||
|||	  Format
|||
|||	    SetBG
|||
|||	  Description
|||
|||	    This command sets the shell's default execution mode to
|||	    background. This prevents the shell from waiting for tasks
|||	    to complete when they are executed. The shell returns
|||	    immediately and is ready to accept more commands.
|||
|||	    If the shell is currently in foreground mode and you wish to
|||	    to execute only a single program in background mode, you can
|||	    append a '&' at the end of the command-line.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    BG
|||
|||	  See Also
|||
|||	    SetFG(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name SetFG
|||	Set the shell's default behavior to foreground execution mode.
|||
|||	  Format
|||
|||	    SetFG
|||
|||	  Description
|||
|||	    This command sets the shell's default execution mode to
|||	    foreground. This forces the shell to wait for tasks to
|||	    complete when they are executed. The shell will not accept
|||	    new commands until the current task completes.
|||
|||	    If the shell is currently in background mode and you wish to
|||	    to execute only a single program in foreground mode, you can
|||	    append a '#' at the end of the command-line.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    FG
|||
|||	  See Also
|||
|||	    SetBG(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name SetPri
|||	Set the shell's priority.
|||
|||	  Format
|||
|||	    SetPri [priority]
|||
|||	  Description
|||
|||	    This command sets the priority of the shell's task. It is
|||	    sometimes desirable to boost the shell's priority to a high
|||	    number. This lets commands such as showtask or memmap work
|||	    with more accuracy.
|||
|||	    The current priority of the shell can be displayed by
|||	    entering this command with no argument.
|||
|||	  Arguments
|||
|||	    [priority]
|||	        The new shell priority. This value must be in the range 10..199
|||	        This number can be in decimal, or in hexadecimal starting with
|||	        0x or $. If you don't specify a priority then the current
|||	        priority is simply displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    shellpri
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name SetCD
|||	Set the shell's current directory.
|||
|||	  Format
|||
|||	    SetCD [directory name]
|||
|||	  Description
|||
|||	    The shell maintains the concept of a current directory.
|||	    Files within the current directory can be referenced without
|||	    a full path specification, in a relative manner.
|||
|||	    This command lets you specify the name of a directory that
|||	    should become the current directory.
|||
|||	    The current directory of the shell can be displayed using the
|||	    the showcd command, or by executing setcd with no arguments.
|||
|||	  Arguments
|||
|||	    [directory name]
|||	        The name of the directory that should become the new current
|||	        directory. If this argument is not supplied, then the name of
|||	        the current directory is displayed, and is not changed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    CD
|||
|||	  See Also
|||
|||	    ShowCD(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name Sleep
|||	Cause the shell to pause for a number of seconds.
|||
|||	  Format
|||
|||	    Sleep <number of seconds>
|||
|||	  Description
|||
|||	    This command tells the shell to go to sleep for a given
|||	    number of seconds. The shell will not accept commands while
|||	    it is sleeping.
|||
|||	  Arguments
|||
|||	    <number of seconds>
|||	        The number of seconds to sleep for. This number can be in
|||	        decimal, or in hexadecimal starting with 0x or $.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
**/

/**
|||	AUTODOC -class Shell_Commands -name KillTask
|||	Remove an executing task or thread from the system.
|||
|||	  Format
|||
|||	    KillTask <task name | task item number>
|||
|||	  Description
|||
|||	    This command removes a task or thread from the system. When
|||	    removing a task, all resources used by the task are also
|||	    returned to the system.
|||
|||	  Arguments
|||
|||	    <task name | task item num>
|||	        This specifies the name or item number of the task to remove.
|||	        The item number can be in decimal, or in hexadecimal starting
|||	        with 0x or $.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    Kill
|||
|||	  See Also
|||
|||	    ShowTask(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowTask
|||	Display information about tasks in the system.
|||
|||	  Format
|||
|||	    ShowTask [task name | task item number]
|||
|||	  Description
|||
|||	    This command displays information about task and threads in the
|||	    system. It can also be given a specific task or thread name,
|||	    in which case a more detailled output is produced describing
|||	    the specified task exclusively.
|||
|||	  Arguments
|||
|||	    [task name | task item num]
|||	        This specifies the name or item number of the task to display
|||	        the information about. The item number can be in decimal, or in
|||	        hexadecimal starting with 0x or $. If this argument is not
|||	        supplied, then general information about all tasks in the
|||	        system is displayed.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    ps
**/

/**
|||	AUTODOC -class Shell_Commands -name ShowCD
|||	Show the name of the current directory.
|||
|||	  Format
|||
|||	    ShowCD
|||
|||	  Description
|||
|||	    This command displays the name of the current directory
|||	    to the debugging terminal.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V20.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
|||	  Synonyms
|||
|||	    PCD
|||
|||	  See Also
|||
|||	    SetCD(@)
**/

/**
|||	AUTODOC -class Shell_Commands -name Log
|||	Control event logging.
|||
|||	  Format
|||
|||	    Log {[-]eventType}
|||
|||	  Description
|||
|||	    This command lets you control Lumberjack, the Portfolio logging
|||	    service. Lumberjack is used to log system events to aid during
|||	    debugging. The kernel uses Lumberjack to store a large amount
|||	    of information about what is currently happening in the system.
|||
|||	    To display the contents of the logs to the screen, use the
|||	    dumplogs command.
|||
|||	    Refer to the Kernel documentation for more information on
|||	    Lumberjack.
|||
|||	  Arguments
|||
|||	    {[-]eventType}
|||	        This argument lets you specify the types of event to be
|||	        logged. You can specify any number of events at the same time.
|||	        The possible event types are: user, tasks, interrupts,
|||	        signals, messages, semaphores, pages, items, ioreqs. If
|||	        you put a - in front of an event type, it turns off logging
|||	        for that event. If you do not specify any event type, the
|||	        command simply displays the events currently being logged.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V27.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name DumpLogs
|||	Dump event logs to the debugging terminal.
|||
|||	  Format
|||
|||	    DumpLogs
|||
|||	  Description
|||
|||	    This command lets you display the contents of the Lumberjack
|||	    event logs to the debugging terminal. You use the log command
|||	    to initiate event logging.
|||
|||	    Refer to the Kernel documentation for more information on
|||	    Lumberjack.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V27.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
**/

/**
|||	AUTODOC -class Shell_Commands -name Expunge
|||	Remove unused demand-loaded modules from memory.
|||
|||	  Format
|||
|||	    Expunge
|||
|||	  Description
|||
|||	    This command causes any demand-loaded modules that have a 0
|||	    use count to be removed from memory.
|||
|||	  Implementation
|||
|||	    Command implemented in shell V27.
|||
|||	  Location
|||
|||	    Built-in shell command.
|||
**/

/* keep the compiler happy... */
extern int foo;
