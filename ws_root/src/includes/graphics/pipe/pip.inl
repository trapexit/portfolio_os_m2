#ifndef __GRAPHICS_PIPE_PIP_INL
#define __GRAPHICS_PIPE_PIP_INL


/******************************************************************************
**
**  @(#) pip.inl 96/02/20 1.6
**
**  Inlines for PIP class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline PipColor::PipColor()					{ }
inline PipColor::PipColor(uint32 c)			{ v = c; }
inline PipColor::PipColor(PipColor& src)	{ v = src.v; }
inline PipColor::operator uint32()			{ return (uint32) v; }

inline uchar PipColor::GetRed()				{ return (v >> 16) & 0xFF; }
inline uchar PipColor::GetGreen()			{ return (v >> 8) & 0xFF; }
inline uchar PipColor::GetBlue()			{ return v & 0xFF; }
inline uchar PipColor::GetSSB()				{ return (v >> 31) & 1; }
inline uint32 PipColor::GetInt()			{ return v; }
inline void PipColor::SetInt(uchar n)		{ v = n; }

inline PipColor* PipTable::GetData()
	{ return m_PCB.pipData; }

inline int32 PipTable::GetIndex()
	{ return m_PCB.pipIndex; }

inline int32 PipTable::GetSize()
	{ return m_PCB.pipNumEntries; }

#else

#define	Pip_Delete		Obj_Delete
#define	Pip_GetData(t)	(((PipData*) (t))->m_PCB.pipData)
#define	Pip_GetSize(t)	(((PipData*) (t))->m_PCB.pipNumEntries)
#define	Pip_GetIndex(t)	(((PipData*) (t))->m_PCB.pipIndex)

#define	PC_SetInt(c, n)		(*(c) = (n))
#define	PC_GetRed(c)		((*(c) >> 16) & 0xFF)
#define	PC_GetGreen(c)		((*(c) >> 8) & 0xFF)
#define	PC_GetBlue(c)		(*(c) & 0xFF)
#define	PC_GetSSB(c)		((*(c) >> 31) & 1)
#define	PC_GetInt(c)		((uint32) *(c))

#endif


#endif /* __GRAPHICS_PIPE_PIP_INL */
