#ifndef __GRAPHICS_PIPE_COL_H
#define __GRAPHICS_PIPE_COL_H


/******************************************************************************
**
**  @(#) col.h 96/08/07 1.22
**
**  Color4 Class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

struct Color4
   {
	gfloat r, g, b, a;

	Color4() { };
	Color4(gfloat red, gfloat green, gfloat blue, gfloat alpha);
	Color4(const Color4&);
	Color4& operator=(const Color4&);
	void Print() const;
   };

void Col_Print(const Color4*);

#else

typedef struct Color4 { gfloat r, g, b, a; } Color4;

#endif

#include <graphics/pipe/col.inl>

#endif /* __GRAPHICS_PIPE_COL_H */
