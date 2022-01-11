#ifndef PUTILS_H
#define PUTILS_H

/*
 * Interface code to separate out from the main body of the test programs
 * the code needed to create a GP (including setting up bitmaps), and
 * displaying the rendered results. This not only simplifies the resulting
 * test programs but also allows us to be platform independent.
 */


/* Types */

typedef struct {
	uint32		numScreens;
	uint32		screenWidth;
	uint32		screenHeight;
	uint32		screenDepth;
	uint32		numCmdListBufs;
	uint32		cmdListBufSize;			/* in bytes */
	bool		autoTransparency;
	bool		enableDither;
	bool		enableVidInterp;
} AppInitStruct;




/*
 * Create GP and set draw mode based on command line arguments.
 * This serves to abstract some development options processing,
 * using GL renderer and renderer from the body of the examples 
 * code
 */

GP* InitGP(int* argc, char **argv);

/*
 * These are utility routines which InitGP calls.  By calling them
 * explicitly, rather than InitGP, a program can force certain attrs
 * of the AppInitStruct, defined above, to be set to certain values.
 * For example, to force the display to always be 32-bits per pixel,
 * call GfxUtil_ParseArgs(), set appData.screenDepth = 32, then call
 * GfxUtil_SetupGraphics().
 */

Err GfxUtil_ParseArgs(int* argc, char *argv[], AppInitStruct* appData);
GP* GfxUtil_SetupGraphics(const AppInitStruct* appData);

/* do whatever is needed to get the rendered results on ths screen */

void SwapBuffers(void);
void NextViewBuffer(void);
void NextRendBuffer(void);
void WaitTE(void);
void IncCurBuffer(void);


/* return the current gp in use, on non unix systems this just returns the
 * gp created by InitGP
 */

GP* GetCurrentGP(void);

/* return the gp specified by the render mode, GP_GLRender or GP_SoftRender
 * this call currently only makes sense on unix. In other situtations it's
 * the same as GetCurrentGP
 */

GP* GetGP(int32 rm);

/*
 * specify which gp to use for rendering
 */

void SetGP(int32 rm);

/* ===============================================
 * Screen-fading functions (similar to CLUT fades)
 * =============================================== */

void BeginFadeToColor( GP* gp, bool useDither );

/*
 *	Call this once before the fade is supposed to begin
 */

void SetFadeColor( Color4* fadeColor );

/*
 *	After calling BeginFadeToColor(), call SetFadeColor() to tell the fader
 *	which color to fade to / from.  This can be changed on the fly, if
 *	that's the desired result.
 */
 
void FadeToColor( GP* gp, uint32 pctDone );

/*
 *	This is the call that actually uses the Pipeline, and in particular,
 *	the Destination Blender, to create the effect of mixing the frame
 *	buffer with a constant color.  pctDone is an integer between 0 and
 *	255.  0 means the frame buffer is at 100% intensity (no fade), and
 *	255 means the screen should be completely filled with the solid color.
 */

void EndFadeToColor( GP* gp );

/*
 *	Call this once after the fade has completed
 */

/* ===============================================
 * Misc. viewing functions
 * =============================================== */

void ViewGroup(Character*, GP*);

void ViewScene(Scene*, GP*);

void Block(GP *gp, gfloat xsize, gfloat ysize, gfloat zsize);

#endif
