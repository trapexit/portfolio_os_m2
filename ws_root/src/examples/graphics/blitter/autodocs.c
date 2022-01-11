/* @(#) autodocs.c 96/08/20 1.1 */
/**
|||	AUTODOC -class Examples -group Blitter -name Blur
|||	Shows how to blur an image using DBlending and scrolling
|||
|||	  Synopsis
|||
|||	    blur <backdrop utf file>
|||
|||	  Description
|||
|||	    Displays the utf file on screen, then scrolls the screen
|||	    by one pixel in all eight directions with destination
|||	    blending enabled. This creates a slightly blurred image.
|||
|||	    You can use Examples/Graphics/Blitter/oxford.utf as the
|||	    backdrop utf file.
|||
|||	  Location
|||
|||	    Examples/Graphics/Blitter/Blur
|||
**/
/**
|||	AUTODOC -class Examples -group Blitter -name Scroll
|||	Shows how to scroll a region of the screen.
|||
|||	  Synopsis
|||
|||	    scroll <backdrop utf file>
|||
|||	  Description
|||
|||	    Displays the utf file on screen. The user can then scroll
|||	    the marked area of the screen in any direction. 
|||
|||	    You can use Examples/Graphics/Blitter/oxford.utf as the
|||	    backdrop utf file.
|||
|||	  Location
|||
|||	    Examples/Graphics/Blitter/Scroll
|||
**/
/**
|||	AUTODOC -class Examples -group Blitter -name Pan
|||	Shows how to change the source of a texture in a BlitObject
|||
|||	  Synopsis
|||
|||	    pan <640x480 backdrop utf file>
|||
|||	  Description
|||
|||	    Displays the top left corner of a 640x480 utf file in a
|||	    320x240 screen. The user can then pan around the bitmap
|||	    using the control pad.
|||
|||	    You can use Examples/Graphics/Blitter/oxford-big.utf as the
|||	    backdrop utf file.
|||
|||	  Location
|||
|||	    Examples/Graphics/Blitter/Pan
|||
**/
/**
|||	AUTODOC -class Examples -group Blitter -name Testblit
|||	Runs through a series of tests on the Blitter folio
|||
|||	  Synopsis
|||
|||	    testblit <backdrop utf file>
|||
|||	  Description
|||
|||	    The first set of tests runs through usage of the
|||	    Blt_...Snippet() functions.
|||
|||	    The second test takes a rectangular region of the screen,
|||	    and warps it into a circular array of vertices. The user
|||	    can move the circle around, turn on or off double
|||	    buffering for some fractal feedback effects, change the
|||	    destination blending ratio, and enable or disable internal
|||	    or external clipping.
|||
|||	    The third test runs through all the possible maskling
|||	    operations.
|||
|||	    The fourth test does some simple timings.
|||
|||	    You can use Examples/Graphics/Blitter/oxford.utf as the
|||	    backdrop utf file.
|||
|||	  Location
|||
|||	    Examples/Graphics/Blitter/Testblit
|||
**/


