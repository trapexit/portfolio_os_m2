#ifndef __GRAPHICS_PIPE_GFX_H
#define __GRAPHICS_PIPE_GFX_H


/******************************************************************************
**
**  @(#) gfx.h 96/02/20 1.32
**
**  Graphics Utility Routines
**
******************************************************************************/


/*
 * Clip results
 */

#define GFX_ClipOut			0     	/* surface entirely outside view vol */
#define GFX_ClipPartial		1 		/* surface partially inside view vol */
#define GFX_ClipIn			2		/* surface entirely within view vol  */

/*
 * Cylinder parts
 */
#define	GFX_Base		1
#define	GFX_Sides		2
#define	GFX_Top			4
#define	GFX_All			7

/*
 * Print Options
 */
#define	GFX_PrintNone		0		/* printing disabled */
#define	GFX_PrintAll		0xF		/* print everything */

#define	GFX_PrintHierarchy	1		/* print children in hierarchy */
#define	GFX_PrintAttributes	2		/* print object attributes */
#define	GFX_PrintSurfaces	4		/* print surface contents */
#define	GFX_PrintVertices	8		/* print vertex data */
#define	GFX_PrintInternal	0x100	/* print internal stuff */

/*
 * Renderer options
 */

#define GFX_RenderWireframe   		0
#define GFX_RenderCommandList 		2
#define GFX_RenderShaded    		3

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

/*
 * PUBLIC API (same for C and C++)
 */
Err		Gfx_Init(void);
int32 	Gfx_GetFrame(void);
void	Gfx_NextFrame(void);
gfloat 	Gfx_GetTime(void);
int32	Gfx_GetPrintLevel(void);
void	Gfx_SetPrintLevel(int32);

#if defined(__cplusplus) && !defined(GFX_C_Bind)
extern "C" {
#endif

void 	Gfx_SetRenderer(int32 renderer);
int32 	Gfx_GetRenderer(void);

#if defined(__cplusplus) && !defined(GFX_C_Bind)
}
#endif

#ifdef _DEBUG
/*
 * Debug Options
 */
#define	GFX_DebugNone	0
#define	GFX_DebugAll	(-1)
#define	GFX_DebugMemory	1

int32	Gfx_GetDebug(void);
void	Gfx_SetDebug(int32);
#endif

Err Gfx_PipeInit(void);

Err Gfx_DrawTorus(Surface* s, gfloat irad, gfloat orad, int32 res, TexCoord*);
Err Gfx_DrawBlock(Surface* s, Point3* size, TexCoord*);
Err Gfx_DrawEllipsoid(Surface *s, Point3* size, int32 res, TexCoord*);
Err Gfx_DrawCylinder(Surface *s, int32 parts, gfloat tradius,
					gfloat bradius, gfloat height, int32 res, TexCoord*);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#endif	/* __GRAPHICS_PIPE_GFX_H */
