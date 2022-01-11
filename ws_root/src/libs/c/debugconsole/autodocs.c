/* @(#) autodocs.c 95/09/14 1.1 */

/**
|||	AUTODOC -public -class DebugConsole -name CreateDebugConsole
|||	Creates a view for debugging console output.
|||
|||	  Synopsis
|||
|||	    Err CreateDebugConsole(const TagArg *tags);
|||	    Err CreateDebugConsoleVA(uint32 tag, ...);
|||
|||	  Description
|||
|||	    Outputting text to the standard debugging terminal using printf()
|||	    is a fairly slow proposal, involving a fair amount of overhead and
|||	    delays, making it hard to use printf() in time-sensitive code.
|||
|||	    This function creates a view on the front of the 3DO display
|||	    where debugging output can be sent. This is a more efficient
|||	    and less intrusive way to perform debugging output. You use the
|||	    DebugConsolePrintf() call to output the text instead of printf().
|||
|||	    The various routines that affect the debugging console are
|||	    protected to allow multiple threads to write to the debug
|||	    output simultaneously and not interfere with one another.
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
|||	    DEBUGCONSOLE_TAG_HEIGHT (uint32)
|||	        This tag specifies the height in pixels of the debugging view
|||	        console. Defaults to 200.
|||
|||	    DEBUGCONSOLE_TAG_TOP (uint32)
|||	        This tag specifies the position of the top of the debugging
|||	        view console relative to the top of the entire display.
|||	        Defaults to 0.
|||
|||	    DEBUGCONSOLE_TAG_TYPE (enum ViewType)
|||	        This tag specifies the type of view to use for the debugging
|||	        console. See <graphics/view.h> for a definition of the possible
|||	        view types. This defaults to VIEWTYPE_16_640_LACE.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libdebugconsole.a V27.
|||
|||	  Associated Files
|||
|||	    <misc/debugconsole.h>, libdebugconsole.a
|||
|||	  See Also
|||
|||	    DeleteDebugConsole(), DebugConsolePrintf()
|||	    DebugConsoleClear(), DebugConsoleMove()
|||
**/

/**
|||	AUTODOC -public -class DebugConsole -name DeleteDebugConsole
|||	Closes the debugging console view preventing further debugging output.
|||
|||	  Synopsis
|||
|||	    void DeleteDebugConsole(void);
|||
|||	  Description
|||
|||	    This function takes down the debugging console, and releases
|||	    any resources allocated by CreateDebugConsole().
|||
|||	  Implementation
|||
|||	    Link library call implemented in libdebugconsole.a V27.
|||
|||	  Associated Files
|||
|||	    <misc/debugconsole.h>, libdebugconsole.a
|||
|||	  See Also
|||
|||	    CreateDebugConsole(), DebugConsolePrintf()
|||	    DebugConsoleClear(), DebugConsoleMove()
|||
**/

/**
|||	AUTODOC -public -class DebugConsole -name DebugConsolePrintf
|||	Output information to a debugging view console.
|||
|||	  Synopsis
|||
|||	    void DebugConsolePrintf(const char *text, ...);
|||
|||	  Description
|||
|||	    This function works like printf(), with the difference that the
|||	    output is sent to a debugging view console on the 3DO display
|||	    instead of to the debugging terminal.
|||
|||	  Arguments
|||
|||	    Same as printf()
|||
|||	  Implementation
|||
|||	    Link library call implemented in libdebugconsole.a V27.
|||
|||	  Associated Files
|||
|||	    <misc/debugconsole.h>, libdebugconsole.a
|||
|||	  See Also
|||
|||	    CreateDebugConsole(), DeleteDebugConsole(),
|||	    DebugConsoleClear(), DebugConsoleMove()
|||
**/

/**
|||	AUTODOC -public -class DebugConsole -name DebugConsoleClear
|||	Erases everything in a debugging view console.
|||
|||	  Synopsis
|||
|||	    void DebugConsoleClear(void);
|||
|||	  Description
|||
|||	    This function erases everything in the debugging view console and
|||	    resets everything to the background color.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libdebugconsole.a V27.
|||
|||	  Associated Files
|||
|||	    <misc/debugconsole.h>, libdebugconsole.a
|||
|||	  See Also
|||
|||	    CreateDebugConsole(), DeleteDebugConsole(), DebugConsolePrintf()
|||	    DebugConsoleMove()
|||
**/

/**
|||	AUTODOC -public -class DebugConsole -name DebugConsoleMove
|||	Moves the debugging console rendering cursor.
|||
|||	  Synopsis
|||
|||	    void DebugConsoleMove(uint32 x, uint32 y);
|||
|||	  Description
|||
|||	    This function lets you move the cursor which determines
|||	    where the next character will be displayed when printing to
|||	    to debugging view console.
|||
|||	    The coordinates are relative to the top left corner of the
|||	    view console and are specified in pixels.
|||
|||	  Arguments
|||
|||	    x
|||	        The new horizontal pixel position of the cursor.
|||
|||	    y
|||	        The new vertical pixel position of the cursor.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libdebugconsole.a V27.
|||
|||	  Associated Files
|||
|||	    <misc/debugconsole.h>, libdebugconsole.a
|||
|||	  See Also
|||
|||	    CreateDebugConsole(), DeleteDebugConsole(), DebugConsolePrintf()
|||	    DebugConsoleClear(), DebugConsoleMove()
|||
**/

/* keep the compiler happy... */
extern int foo;
