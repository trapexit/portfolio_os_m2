#ifndef __GRAPHICS_PIPE_PIP_H
#define __GRAPHICS_PIPE_PIP_H


/******************************************************************************
**
**  @(#) pip.h 96/02/20 1.14
**
**  PIP attributes and API.
**
******************************************************************************/


#define PIP_IsAllocated 1
#define PIP_Changed     2

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/****
 *
 * PUBLIC C++ API
 *
 ****/
/****
 *
 * Defines a 32 bit color representation for use in Pip tables
 *
 ****/
class PipColor
{
public:
	PipColor();
	PipColor(uint32);
	PipColor(PipColor& src);
	PipColor(uchar r, uchar g, uchar b, uchar a = 1, uchar ssb = 0);
	operator uint32();

	uchar	GetRed();
	uchar	GetGreen();
	uchar	GetBlue();
	uchar	GetAlpha();
	uchar	GetSSB();
	uint32	GetInt();
	void	GetRGB(uchar*, uchar*, uchar*);
	void	GetRGB(uchar&, uchar&, uchar&);
	void	Get(uchar*, uchar*, uchar*, uchar*, uchar*);
	void	SetRed(uchar);
	void	SetGreen(uchar);
	void	SetBlue(uchar);
	void	SetAlpha(uchar);
	void	SetSSB(uchar);
	void	SetInt(uint32);
	void	SetRGB(uchar, uchar, uchar);
	void	Set(uchar, uchar, uchar, uchar, uchar);

Protected:
	uint32	v;
};

/****
 *
 * Defines a Pip color table and associated command list
 *
 ****/
class PipTable : public GfxObj
{
public:
	GFX_CLASS_DECLARE(PipTable)

/*
 *	Constructors and Destructors
 */
	PipTable();
	PipTable(int32 size = 0);
	PipTable(PipTable&);
	~PipTable();

	Err			Copy(GfxObj*);
	Err			Load(char*);
	Err			Init(char*);

	Err			SetData(PipColor*);
	PipColor*	GetData();
	Err			SetIndex(int32);
	int32		GetIndex();
	int32		GetSize();
	Err			SetSize(int32);
	CltSnippet*	GetCommands();

Protected:
	int32				m_Flags;
	Tx_PipControlBlock	m_PCB;
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
typedef uint32 PipColor;

PipTable*	Pip_Create(int32 size);
Err			Pip_Load(PipTable*, char* filename);
Err			Pip_Init(PipTable*, char* pipChunk);
void		Pip_Delete(PipTable*);
Err			Pip_Copy(GfxObj*, GfxObj*);
PipColor*	Pip_GetData(PipTable*);
Err			Pip_SetData(PipTable*, PipColor*);
int32		Pip_GetSize(PipTable*);
Err			Pip_SetSize(PipTable*, int32);
int32		Pip_GetIndex(PipTable*);
Err			Pip_SetIndex(PipTable*, int32);
CltSnippet*	Pip_GetCommands(PipTable*);

void		PC_SetRed(PipColor*, uchar);
void		PC_SetGreen(PipColor*, uchar);
void		PC_SetBlue(PipColor*, uchar);
void		PC_SetSSB(PipColor*, uchar);
void		PC_SetInt(PipColor*, uint32);
void		PC_SetRGB(PipColor*, uchar, uchar, uchar);
void		PC_Set(PipColor*, uchar r, uchar g, uchar b, uchar a, uchar ssb);
uchar		PC_GetRed(PipColor*);
uchar		PC_GetGreen(PipColor*);
uchar		PC_GetBlue(PipColor*);
uchar		PC_GetSSB(PipColor*);
uint32		PC_GetInt(PipColor*);
void		PC_GetRGB(PipColor*, uchar*, uchar*, uchar*);


typedef struct PipData
   {
	GfxObj      		m_Base;     /* base object */
	int32				m_Flags;
	CltPipControlBlock	m_PCB;
   } PipData;

#ifdef __cplusplus
}
#endif

#endif

#include "pip.inl"

#endif /* __GRAPHICS_PIPE_PIP_H */
