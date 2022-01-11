/* @(#) opengraphicsfolio.c 96/04/23 1.7 */

#include <loader/loader3do.h>


/**
|||	AUTODOC -class graphicsfolio -name OpenGraphicsFolio
|||	Initiate a session with the graphics folio.
|||
|||	  Synopsis
|||
|||	    Err OpenGraphicsFolio (void)
|||
|||	  Description
|||
|||	    This function opens the graphics folio (demand-loading it, if
|||	    necessary), making its facilities available to the client.
|||
|||	    This function defines and sets a global variable named
|||	    GraphicsBase, which is set to a pointer to the folio base.
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
|||	    CloseGraphicsFolio()
|||
**/

Err OpenGraphicsFolio(void)
{
    return ImportByName(FindCurrentModule(), "graphics");
}
