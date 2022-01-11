#ifndef __GRAPHICS_PIPE_TEXBLEND_H
#define __GRAPHICS_PIPE_TEXBLEND_H


/******************************************************************************
**
**  @(#) texblend.h 96/02/20 1.22
**
**  Texture and Destination Blender attributes and API.
**
******************************************************************************/


#define	TXB_None	(-1)

#define TXB_LoadChanged 1

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/****
 *
 * PUBLIC C++ API
 *
 ****/
class TexBlend : public GfxObj
{
public:
	GFX_CLASS_DECLARE(TexBlend)

/*
 *	Constructors and Destructors
 */
	TexBlend(void);
	TexBlend(TexBlend&);
	~TexBlend(void);

	TexBlend*	Load(char*);
	TexBlend*	Init(char*);
	Err			Copy(GfxObj*);

	Err			SetTexture(Texture*);
	Texture*	GetTexture(void);
	Err			SetPip(PipTable*);
	Texture*	GetPip(void);
/*
 * Texture load attributes
 */
	Err		SetLoadRect(int32 x, int32 y, int32 w, int32 h);
	Err		GetLoadRect(int32 x, int32 y, int32 w, int32 h);
	Err		SetFirstLOD(int32);
	int32	GetFirstLOD(void);
	Err		SetNumLOD(int32);
	int32	GetNumLOD(void);
	Err		SetXWrap(int32);
	int32	GetXWrap(void);
	Err		SetYWrap(int32);
	int32	GetYWrap(void);
	Err		SetWrap(int32);
	Err		SetTramOffset(int32);
	int32	GetTramOffset(void);
	int32	GetTramSize(void);
	Err		SetXOffset(int32);
	int32	GetXOffset(void);
	Err		SetYOffset(int32);
	int32	GetYOffset(void);
	Err		SetXSize(int32);
	int32	GetXSize(void);
	Err		SetYSize(int32);
	int32	GetYSize(void);

/*
 * Texture attribute setting
 */
	Err	SetTxAttr(int32 id, int32 val);
	Err SetTxMinFilter(int32);
	Err SetTxMagFilter(int32);
	Err SetTxInterFilter(int32);
	Err SetTxPipIndexOffset(int32);
	Err SetTxPipColorSel(int32);
	Err SetTxPipAlphaSel(int32);
	Err SetTxPipSSBSel(int32);
	Err SetTxPipConstSSB0(int32);
	Err SetTxPipConstSSB1(int32);
	Err SetTxFirstColor(int32);
	Err SetTxSecondColor(int32);
	Err SetTxThirdColor(int32);
	Err SetTxFirstAlpha(int32);
	Err SetTxSecondAlpha(int32);
	Err SetTxColorOut(int32);
	Err SetTxAlphaOut(int32);
	Err SetTxBlendOp(int32);
	Err SetTxBlendColorSSB0(int32);
	Err SetTxBlendColorSSB1(int32);
/*
 * Destination blender attribute setting
 */
	Err	SetDblAttr(int32 id, int32 val);
	Err SetDblEnableAttrs(int32);
	Err SetDblDiscard(int32);
	Err SetDblXWinClipMin(int32);
	Err SetDblXWinClipMax(int32);
	Err SetDblYWinClipMin(int32);
	Err SetDblYWinClipMax(int32);
	Err SetDblZCompareControl(int32);
	Err SetDblZXOffset(int32);
	Err SetDblZYOffset(int32);
	Err SetDblDSBSelect(int32);
	Err SetDblDSBConst(int32);
	Err SetDblAInputSelect(int32);
	Err SetDblAMultCoefSelect(int32);
	Err SetDblAMultConstControl(int32);
	Err SetDblAMultRtJustify(int32);
	Err SetDblBInputSelect(int32);
	Err SetDblBMultCoefSelect(int32);
	Err SetDblBMultConstControl(int32);
	Err SetDblBMultRtJustify(int32);
	Err SetDblALUOperation(int32);
	Err SetDblFinalDivide(int32);
	Err SetDblAlpha0ClampControl(int32);
	Err SetDblAlpha1ClampControl(int32);
	Err SetDblAlphaFracClampControl(int32);
	Err SetDblDestAlphaSelect(int32);
	Err SetDblDestAlphaConstSSB0(int32);
	Err SetDblDestAlphaConstSSB1(int32);
	Err SetDblDitherMatrixA(int32);
	Err SetDblDitherMatrixB(int32);
	Err SetDblSrcPixels32Bit(int32);
	Err SetDblSrcBaseAddr(int32);
	Err SetDblSrcXStride(int32);
	Err SetDblSrcXOffset(int32);
	Err SetDblSrcYOffset(int32);
	Err SetDblRGBConstIn(PipColor*);
	Err SetDblBMultConstSSB0(PipColor*);
	Err SetDblBMultConstSSB1(PipColor*);
	Err SetDblAMultConstSSB0(PipColor*);
	Err SetDblAMultConstSSB1(PipColor*);
/*
 * Texture attribute getting
 */
	int32 GetTxAttr(int32);
	int32 GetTxMinFilter(void);
	int32 GetTxMagFilter(void);
	int32 GxInterFilter(void);
	int32 GetTxPipIndexOffset(void);
	int32 GetTxPipColorSel(void);
	int32 GetTxPipAlphaSel(void);
	int32 GetTxPipSSBSel(void);
	int32 GetTxPipConstSSB0(void);
	int32 GetTxPipConstSSB1(void);
	int32 GetTxFirstColor(void);
	int32 GetTxSecondColor(void);
	int32 GetTxThirdColor(void);
	int32 GetTxFirstAlpha(void);
	int32 GetTxSecondAlpha(void);
	int32 GetTxColorOut(void);
	int32 GetTxAlphaOut(void);
	int32 GetTxBlendOp(void);
	int32 GetTxBlendColorSSB0(void);
	int32 GetTxBlendColorSSB1(void);
/*
 * Destination blender attribute getting
 */
	int32 GetDblAttr(int32);
	int32 GetDblEnableAttrs(void);
	int32 GetDblDiscard(void);
	int32 GetDblXWinClipMin(void);
	int32 GetDblXWinClipMax(void);
	int32 GetDblYWinClipMin(void);
	int32 GetDblYWinClipMax(void);
	int32 GetDblZCompareControl(void);
	int32 GetDblZXOffset(void);
	int32 GetDblZYOffset(void);
	int32 GetDblDSBSelect(void);
	int32 GetDblDSBConst(void);
	int32 GetDblAInputSelect(void);
	int32 GetDblAMultCoefSelect(void);
	int32 GetDblAMultConstControl(void);
	int32 GetDblAMultRtJustify(void);
	int32 GetDblBInputSelect(void);
	int32 GetDblBMultCoefSelect(void);
	int32 GetDblBMultConstControl(void);
	int32 GetDblBMultRtJustify(void);
	int32 GetDblALUOperation(void);
	int32 GetDblFinalDivide(void);
	int32 GetDblAlpha0ClampControl(void);
	int32 GetDblAlpha1ClampControl(void);
	int32 GetDblAlphaFracClampControl(void);
	int32 GetDblDestAlphaSelect(void);
	int32 GetDblDestAlphaConstSSB0(void);
	int32 GetDblDestAlphaConstSSB1(void);
	int32 GetDblDitherMatrixA(void);
	int32 GetDblDitherMatrixB(void);
	int32 GetDblSrcPixels32Bit(void);
	int32 GetDblSrcBaseAddr(void);
	int32 GetDblSrcXStride(void);
	int32 GetDblSrcXOffset(void);
	int32 GetDblSrcYOffset(void);
	Err	  GetDblRGBConstIn(PipColor*);
	Err	  GetDblAMultConstSSB0(PipColor*);
	Err   GetDblAMultConstSSB1(PipColor*);
	Err	  GetDblBMultConstSSB0(PipColor*);
	Err	  GetDblBMultConstSSB1(PipColor*);

Protected:
/*
 * Internal functions
 */
	CltSnippet*	GetTxCommands(void);
	CltSnippet*	GetDblCommands(void);
	CltSnippet*	GetLoadCommands(void);
/*
 * Data areas
 */
	int32		m_Flags;	/* status flags */
	Texture*	m_Tex;		/* texture we are using */
	void*		m_LCB;		/* texture load information */
	PipTable*	m_PIP;		/* pip table */
	uint32*		m_TAB;		/* texture attributes */
	uint32*		m_DAB;		/* destination blender attributes */
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
TexBlend*	Txb_Create(void);
void		Txb_Delete(TexBlend*);
Err			Txb_Copy(GfxObj*, GfxObj*);
Err			Txb_Load(TexBlend*, char*);
Err			Txb_Init(TexBlend*, char*);

/*
 * Texture and PIP loading
 */
Texture* Txb_GetTexture(TexBlend*);
Err		Txb_SetTexture(TexBlend*, Texture*);
PipTable* Txb_GetPip(TexBlend*);
Err		Txb_SetPip(TexBlend*, PipTable*);
Err		Txb_SetLoadRect(TexBlend*, int32, int32, int32, int32);
Err		Txb_GetLoadRect(TexBlend*, int32, int32, int32, int32);
Err		Txb_SetFirstLOD(TexBlend*, int32);
int32	Txb_GetFirstLOD(TexBlend*);
Err		Txb_SetNumLOD(TexBlend*, int32);
int32	Txb_GetNumLOD(TexBlend*);
Err		Txb_SetXWrap(TexBlend*, int32);
int32	Txb_GetXWrap(TexBlend*);
Err		Txb_SetYWrap(TexBlend*, int32);
int32	Txb_GetYWrap(TexBlend*);
Err     Txb_SetWrap(TexBlend*, int32);
Err		Txb_SetTramOffset(TexBlend*, int32);
int32	Txb_GetTramOffset(TexBlend*);
int32	Txb_GetTramSize(TexBlend*);
Err		Txb_SetXOffset(TexBlend*, int32);
int32	Txb_GetXOffset(TexBlend*);
Err		Txb_SetYOffset(TexBlend*, int32);
int32	Txb_GetYOffset(TexBlend*);
Err		Txb_SetXSize(TexBlend*, int32);
int32	Txb_GetXSize(TexBlend*);
Err		Txb_SetYSize(TexBlend*, int32);
int32	Txb_GetYSize(TexBlend*);

/*
 * Texture attribute setting
 */
Err	Txb_SetTxAttr(TexBlend*, int32, int32);
Err Txb_SetTxMinFilter(TexBlend*, int32);
Err Txb_SetTxMagFilter(TexBlend*, int32);
Err Txb_SetTxInterFilter(TexBlend*, int32);
Err Txb_SetTxPipIndexOffset(TexBlend*, int32);
Err Txb_SetTxPipColorSel(TexBlend*, int32);
Err Txb_SetTxPipAlphaSel(TexBlend*, int32);
Err Txb_SetTxPipSSBSel(TexBlend*, int32);
Err Txb_SetTxPipConstSSB0(TexBlend*, int32);
Err Txb_SetTxPipConstSSB1(TexBlend*, int32);
Err Txb_SetTxFirstColor(TexBlend*, int32);
Err Txb_SetTxSecondColor(TexBlend*, int32);
Err Txb_SetTxThirdColor(TexBlend*, int32);
Err Txb_SetTxFirstAlpha(TexBlend*, int32);
Err Txb_SetTxSecondAlpha(TexBlend*, int32);
Err Txb_SetTxColorOut(TexBlend*, int32);
Err Txb_SetTxAlphaOut(TexBlend*, int32);
Err Txb_SetTxBlendOp(TexBlend*, int32);
Err Txb_SetTxBlendColorSSB0(TexBlend*, int32);
Err Txb_SetTxBlendColorSSB1(TexBlend*, int32);

/*
 * Destination blender attribute setting
 */
Err	Txb_SetDblAttr(TexBlend*, int32, int32);
Err Txb_SetDblEnableAttrs(TexBlend*, int32);
Err Txb_SetDblDiscard(TexBlend*, int32);
Err Txb_SetDblXWinClipMin(TexBlend*, int32);
Err Txb_SetDblXWinClipMax(TexBlend*, int32);
Err Txb_SetDblYWinClipMin(TexBlend*, int32);
Err Txb_SetDblYWinClipMax(TexBlend*, int32);
Err Txb_SetDblZCompareControl(TexBlend*, int32);
Err Txb_SetDblZXOffset(TexBlend*, int32);
Err Txb_SetDblZYOffset(TexBlend*, int32);
Err Txb_SetDblDSBSelect(TexBlend*, int32);
Err Txb_SetDblDSBConst(TexBlend*, int32);
Err Txb_SetDblAInputSelect(TexBlend*, int32);
Err Txb_SetDblAMultCoefSelect(TexBlend*, int32);
Err Txb_SetDblAMultConstControl(TexBlend*, int32);
Err Txb_SetDblAMultRtJustify(TexBlend*, int32);
Err Txb_SetDblBInputSelect(TexBlend*, int32);
Err Txb_SetDblBMultCoefSelect(TexBlend*, int32);
Err Txb_SetDblBMultConstControl(TexBlend*, int32);
Err Txb_SetDblBMultRtJustify(TexBlend*, int32);
Err Txb_SetDblALUOperation(TexBlend*, int32);
Err Txb_SetDblFinalDivide(TexBlend*, int32);
Err Txb_SetDblAlpha0ClampControl(TexBlend*, int32);
Err Txb_SetDblAlpha1ClampControl(TexBlend*, int32);
Err Txb_SetDblAlphaFracClampControl(TexBlend*, int32);
Err Txb_SetDblDestAlphaSelect(TexBlend*, int32);
Err Txb_SetDblDestAlphaConstSSB0(TexBlend*, int32);
Err Txb_SetDblDestAlphaConstSSB1(TexBlend*, int32);
Err Txb_SetDblDitherMatrixA(TexBlend*, int32);
Err Txb_SetDblDitherMatrixB(TexBlend*, int32);
Err Txb_SetDblSrcPixels32Bit(TexBlend*, int32);
Err Txb_SetDblSrcBaseAddr(TexBlend*, int32);
Err Txb_SetDblSrcXStride(TexBlend*, int32);
Err Txb_SetDblSrcXOffset(TexBlend*, int32);
Err Txb_SetDblSrcYOffset(TexBlend*, int32);
Err Txb_SetDblRGBConstIn(TexBlend*, PipColor*);
Err Txb_SetDblBMultConstSSB0(TexBlend*, PipColor*);
Err Txb_SetDblBMultConstSSB1(TexBlend*, PipColor*);
Err Txb_SetDblAMultConstSSB0(TexBlend*, PipColor*);
Err Txb_SetDblAMultConstSSB1(TexBlend*, PipColor*);

/*
 * Texture attribute getting
 */
int32 Txb_GetTxAttr(TexBlend*, int32);
int32 Txb_GetTxMinFilter(TexBlend*);
int32 Txb_GetTxMagFilter(TexBlend*);
int32 Txb_GetTxInterFilter(TexBlend*);
int32 Txb_GetTxPipIndexOffset(TexBlend*);
int32 Txb_GetTxPipColorSel(TexBlend*);
int32 Txb_GetTxPipAlphaSel(TexBlend*);
int32 Txb_GetTxPipSSBSel(TexBlend*);
int32 Txb_GetTxPipConstSSB0(TexBlend*);
int32 Txb_GetTxPipConstSSB1(TexBlend*);
int32 Txb_GetTxFirstColor(TexBlend*);
int32 Txb_GetTxSecondColor(TexBlend*);
int32 Txb_GetTxThirdColor(TexBlend*);
int32 Txb_GetTxFirstAlpha(TexBlend*);
int32 Txb_GetTxSecondAlpha(TexBlend*);
int32 Txb_GetTxColorOut(TexBlend*);
int32 Txb_GetTxAlphaOut(TexBlend*);
int32 Txb_GetTxBlendOp(TexBlend*);
int32 Txb_GetTxBlendColorSSB0(TexBlend*);
int32 Txb_GetTxBlendColorSSB1(TexBlend*);

/*
 * Destination blender attribute getting
 */
int32 Txb_GetDblAttr(TexBlend*, int32);
int32 Txb_GetDblEnableAttrs(TexBlend*);
int32 Txb_GetDblDiscard(TexBlend*);
int32 Txb_GetDblXWinClipMin(TexBlend*);
int32 Txb_GetDblXWinClipMax(TexBlend*);
int32 Txb_GetDblYWinClipMin(TexBlend*);
int32 Txb_GetDblYWinClipMax(TexBlend*);
int32 Txb_GetDblZCompareControl(TexBlend*);
int32 Txb_GetDblZXOffset(TexBlend*);
int32 Txb_GetDblZYOffset(TexBlend*);
int32 Txb_GetDblDSBSelect(TexBlend*);
int32 Txb_GetDblDSBConst(TexBlend*);
int32 Txb_GetDblAInputSelect(TexBlend*);
int32 Txb_GetDblAMultCoefSelect(TexBlend*);
int32 Txb_GetDblAMultConstControl(TexBlend*);
int32 Txb_GetDblAMultRtJustify(TexBlend*);
int32 Txb_GetDblBInputSelect(TexBlend*);
int32 Txb_GetDblBMultCoefSelect(TexBlend*);
int32 Txb_GetDblBMultConstControl(TexBlend*);
int32 Txb_GetDblBMultRtJustify(TexBlend*);
int32 Txb_GetDblALUOperation(TexBlend*);
int32 Txb_GetDblFinalDivide(TexBlend*);
int32 Txb_GetDblAlpha0ClampControl(TexBlend*);
int32 Txb_GetDblAlpha1ClampControl(TexBlend*);
int32 Txb_GetDblAlphaFracClampControl(TexBlend*);
int32 Txb_GetDblDestAlphaSelect(TexBlend*);
int32 Txb_GetDblDestAlphaConstSSB0(TexBlend*);
int32 Txb_GetDblDestAlphaConstSSB1(TexBlend*);
int32 Txb_GetDblDitherMatrixA(TexBlend*);
int32 Txb_GetDblDitherMatrixB(TexBlend*);
int32 Txb_GetDblSrcPixels32Bit(TexBlend*);
int32 Txb_GetDblSrcBaseAddr(TexBlend*);
int32 Txb_GetDblSrcXStride(TexBlend*);
int32 Txb_GetDblSrcXOffset(TexBlend*);
int32 Txb_GetDblSrcYOffset(TexBlend*);
Err	  Txb_GetDblRGBConstIn(TexBlend*, PipColor*);
Err	  Txb_GetDblAMultConstSSB0(TexBlend*, PipColor*);
Err   Txb_GetDblAMultConstSSB1(TexBlend*, PipColor*);
Err	  Txb_GetDblBMultConstSSB0(TexBlend*, PipColor*);
Err	  Txb_GetDblBMultConstSSB1(TexBlend*, PipColor*);

/*
 * Internal functions
 */
CltSnippet*	Txb_GetTxCommands(TexBlend*);
CltSnippet*	Txb_GetDblCommands(TexBlend*);
CltSnippet*	Txb_GetLoadCommands(TexBlend*);
CltSnippet*	Txb_GetUseCommands(TexBlend*);

#define Txb_Delete  Obj_Delete
#define Txb_Copy    Obj_Copy
#define Txb_Print   Obj_Print

#ifdef __cplusplus
}
#endif

/*
 * TxbData is a structure that represents the INTERNAL FORMAT of
 * class TexBlend in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GP
 * attributes should only be accessed by the Txb_XXX functions.
 */
typedef struct TxbData
   {
    GfxObj		m_Base;     /* base object */
	Texture*	m_Tex;		/* texture we are using */
	int32		m_Flags;	/* status flags */
	void*		m_LCB;		/* texture load information */
	PipTable*	m_PIP;		/* pip table */
	CltSnippet	m_TAB;		/* texture attributes */
	CltSnippet	m_DAB;		/* destination blender attributes */
   } TxbData;

#endif


#endif /* __GRAPHICS_PIPE_TEXBLEND_H */
