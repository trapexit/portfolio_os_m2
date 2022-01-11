/* @(#) closegraphicsfolio.c 96/04/23 1.8 */

#include <loader/loader3do.h>


/**
|||	AUTODOC -class graphicsfolio -name CloseGraphicsFolio
|||	Terminate session with the graphics folio.
|||
|||	  Synopsis
|||
|||	    Err CloseGraphicsFolio (void)
|||
|||	  Description
|||
|||	    This function closes the graphics folio, terminating the
|||	    client's session with the folio.  The global variable
|||	    GraphicsBase is not cleared, permitting concurrent threads
|||	    sharing the global variable space to continue using the folio.
|||
|||	    This function may be safely called if the graphics folio was
|||	    never opened; it will do nothing.
|||
|||	  Return Value
|||
|||	    Returns zero on success, or a (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>
|||
|||	  See Also
|||
|||	    OpenGraphicsFolio()
|||
**/

Err CloseGraphicsFolio(void)
{
    return UnimportByName(FindCurrentModule(), "graphics");
}
