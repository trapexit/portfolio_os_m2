#ifndef __GRAPHICS_PIPE_COL_INL
#define __GRAPHICS_PIPE_COL_INL


/******************************************************************************
**
**  @(#) col.inl 96/02/20 1.14
**
**  Inlines for Color and Color4 Classes
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline Color4::Color4(gfloat red, gfloat green, gfloat blue, gfloat alpha = 1.0)
	{ r = red; g = green; b = blue; a = alpha; }

inline Color4::Color4(const Color4& c)
	{ r = c.r; g = c.g; b = c.b; a = c.a; }

inline Color4& Color4::operator=(const Color4& c)
	{ r = c.r; g = c.g; b = c.b; a = c.a; return *this; }

inline void Color4::Print() const
	{ Col_Print(this); }

#endif

#define	Col_SetRGB(c, R, G, B) 						\
	((c)->r = ((gfloat)R), (c)->g = ((gfloat)G),	\
	 (c)->b = ((gfloat)B), (c)->a = 1.0)

#define	Col_Set(c, R, G, B, A) \
	((c)->r = ((gfloat)R), (c)->g = ((gfloat)G), (c)->b = ((gfloat)B), (c)->a = ((gfloat)A))


#endif /* __GRAPHICS_PIPE_COL_INL */
