#ifndef __GRAPHICS_PIPE_STACK_INL
#define __GRAPHICS_PIPE_STACK_INL


/******************************************************************************
**
**  @(#) stack.inl 96/02/20 1.11
**
**  Inlines for Stack class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline Stack::Stack()				{ }
inline int	Stack::GetSize() const	{ return m_Size; }
inline void* Stack::Top() const		{ return (void*) (m_Top + 1); }
inline int	Stack::IsEmpty() const	{ return m_Top == NULL; }
inline void* Stack::Push(void* p)	{ return Stk_Push(this, p); }
inline void* Stack::Push()			{ return Stk_Push(this, NULL); }
inline void	Stack::Pop()			{ Stk_Pop(this); }
inline void	Stack::Empty()			{ Stk_Empty(this); }

inline Stack::Stack(int size)
{
	m_Top = NULL;
	m_Free = NULL;
	m_Size = size;
}

#endif

#define Stk_IsEmpty(s)  ((s)->m_Top == NULL)
#define Stk_GetSize(s)  ((s)->m_Size)
#define Stk_Top(s)      ((void*) ((s)->m_Top + 1))


#endif /* __GRAPHICS_PIPE_STACK_INL */
