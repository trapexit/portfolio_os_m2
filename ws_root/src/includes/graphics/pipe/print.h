#ifndef __GRAPHICS_PIPE_PRINT_H
#define __GRAPHICS_PIPE_PRINT_H


/******************************************************************************
**
**  @(#) print.h 96/02/20 1.17
**
**  Printing definitions
**
******************************************************************************/


#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

bool GP_BeginLine(void);

void GP_PrintIndent(void);

void GP_PrintUnindent(void);

void GP_PrintVertexList(const gfloat* vtx, int n, int row, char* label);

void GP_PrintShortsList(int32 nshorts, int16 *s);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#ifndef GP_Printf
#define GP_Printf(s) printf s
#define GP_PrintBegin(s) ( GP_BeginLine() ?  printf s : 0 )
#endif


#endif /* __GRAPHICS_PIPE_PRINT_H */
