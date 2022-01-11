#ifndef __GRAPHICS_PIPE_TEX_INL
#define __GRAPHICS_PIPE_TEX_INL


/******************************************************************************
**
**  @(#) tex.inl 96/02/20 1.5
**
**  Inlines for Texture class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline uint32 Texture::GetMinWidth()
	{ return m_Tex.minX; }

inline uint32 Texture::GetMinHeight();
	{ return m_Tex.minY; }

inline int32 Texture::GetNumLOD();
	{ return m_Tex.maxLOD; }

inline int32 Texture::GetDepth();
	{ return m_Tex.bitsPerPixel; }

inline uint32 Texture::GetFormat();
	{ return m_Tex.expansionFormat; }

inline uint32 Texture::GetAlphaDepth()
	{ return m_Tex.expansionFormat & 0x0F0; }

inline uint32 Texture::HasColor()
	{ return (m_Tex.expansionFormat & 0x0400) >> 10; }

inline uint32 Texture::HasAlpha()
	{ return (m_Tex.expansionFormat & 0x0800) >> 11; }

inline uint32 Texture::HasSSB()
	{ return (m_Tex.expansionFormat & 0x0200) >> 9; }

inline uint32 Texture::IsLiteral()
	{ return (m_Tex.expansionFormat & 0x01000) >> 12; }

#else

/****
 *
 * PUBLIC C API
 *
 ****/
#define	Tex_Delete				Obj_Delete
#define	Tex_Copy				Obj_Copy
#define	Tex_GetMinWidth(t)		(((TexData*) (t))->m_Tex.minX)
#define	Tex_GetMinHeight(t)		(((TexData*) (t))->m_Tex.minY)
#define	Tex_GetNumLOD(t)		(((TexData*) (t))->m_Tex.maxLOD)
#define	Tex_GetDepth(t)			(((TexData*) (t))->m_Tex.bitsPerPixel)
#define	Tex_GetFormat(t)		(((TexData*) (t))->m_Tex.expansionFormat)

#define Tex_GetColorDepth(t) \
		(((TexData*) (t))->m_Tex.expansionFormat & 0x0F)

#define Tex_GetAlphaDepth(t) \
		(((TexData*) (t))->m_Tex.expansionFormat & 0x0F0)

#define Tex_HasColor(t) \
		((((TexData*) (t))->m_Tex.expansionFormat & 0x0400) >> 10)

#define Tex_HasAlpha(t) \
		((((TexData*) (t))->m_Tex.expansionFormat & 0x0800) >> 11)

#define Tex_HasSSB(t) \
		((((TexData*) (t))->m_Tex.expansionFormat & 0x0200) >> 9)

#define Tex_IsLiteral(t) \
		((((TexData*) (t))->m_Tex.expansionFormat & 0x01000) >> 12)

#endif


#endif /* __GRAPHICS_PIPE_TEX_INL */
