#ifndef __GRAPHICS_PIPE_TEX_H
#define __GRAPHICS_PIPE_TEX_H


/******************************************************************************
**
**  @(#) tex.h 96/02/20 1.57
**
**  Texture class description
**
******************************************************************************/


/*
 * Texture LOD pointers and sizes
 */
typedef struct TexLOD
   {
	uint32   Size;
	void    *Data;
   } TexLOD;

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/****
 *
 * PUBLIC C++ API
 *
 ****/
class Texture : public GfxObj
{
public:
	GFX_CLASS_DECLARE(Texture)

/*
 *	Constructors and Destructors
 */
	Texture();
	Texture(Texture&);
	~Texture();

	Texture*	Load(char*);
	Texture*	Init(char*);
	Err			Copy(GfxObj*);

	Err		SetMinWidth(uint32);
	Err		SetMinHeight(uint32);
	Err		SetDepth(uint32);
	Err		SetFormat(uint32);
	Err		SetTexelData(uint32, TexLOD*);

	uint32	GetMinWidth();
	uint32	GetMinHeight();
	uint32	GetWidth(int32);
	uint32	GetHeight(int32);
	uint32	GetMaxWidth();
	uint32	GetMaxHeight();
	int32	GetNumLOD();
	int32	GetDepth();
	uint32	GetFormat();
	uint32	GetColorDepth();
	uint32	GetAlphaDepth();
	uint32	HasColor();
	uint32	HasAlpha();
	uint32	HasSSB();
	uint32	IsLiteral();
	TexLOD*	GetTexelData();

Protected:
	int32		m_Flags;	/* internal flags */
	Tx_Data		m_Tex;		/* texture data */
};

#else

#ifdef __cplusplus
extern "C" {
#endif

/****
 *
 * PUBLIC C API
 *
 ****/
Texture* Tex_Create(void);
void	Tex_Delete(Texture*);
Err		Tex_Copy(GfxObj*, GfxObj*);
Err		Tex_Load(Texture*, char*);
Err		Tex_Init(Texture*, char*);
Err		Tex_SetMinWidth(Texture*, int32);
Err		Tex_SetMinHeight(Texture*, int32);
Err		Tex_SetTexelData(Texture*, uint32, TexLOD*);
Err		Tex_SetDepth(Texture*, int32);
Err		Tex_SetFormat(Texture*, uint32);
uint32	Tex_GetMinWidth(Texture*);
uint32	Tex_GetMinHeight(Texture*);
int32	Tex_GetWidth(Texture*, int32);
int32	Tex_GetHeight(Texture*, int32);
int32	Tex_GetMaxWidth(Texture*);
int32	Tex_GetMaxHeight(Texture*);
int32	Tex_GetNumLOD(Texture*);
int32	Tex_GetDepth(Texture*);
uint32	Tex_GetFormat(Texture*);
TexLOD*	Tex_GetTexelData(Texture*);
uint32	Tex_GetColorDepth(Texture*);
uint32	Tex_GetAlphaDepth(Texture*);
uint32	Tex_HasColor(Texture*);
uint32	Tex_HasAlpha(Texture*);
uint32	Tex_HasSSB(Texture*);
uint32	Tex_IsLiteral(Texture*);

#ifdef __cplusplus
}
#endif

/*
 * TxbData is a structure that represents the INTERNAL FORMAT of
 * class TexBlend in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GP
 * attributes should only be accessed by the Txb_XXX functions.
 */
typedef struct TexData
   {
    GfxObj		m_Base;     /* base object */
	int32		m_Flags;	/* internal flags */
	CltTxData	m_Tex;		/* texture data */
   } TexData;

#endif

#include "tex.inl"


#endif /* __GRAPHICS_PIPE_TEX_H */
