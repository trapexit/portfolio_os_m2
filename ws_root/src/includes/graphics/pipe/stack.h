#ifndef __GRAPHICS_PIPE_STACK_H
#define __GRAPHICS_PIPE_STACK_H


/******************************************************************************
**
**  @(#) stack.h 96/02/20 1.16
**
**  Generic Stack Class
**
******************************************************************************/


struct StackElem { struct StackElem* m_Next; };

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/****
 *
 * Stack: General push-down stack object
 * Implements a stack of fixed-size blocks that grows dynamically.
 *
 * Operations:
 *	Stack(int n)		Initialize stack for elements of N bytes each
 *	Push(void*)			Copy and push given element
 *	Pop()				Pop top of stack
 *  Top()				Return top of stack
 *	Empty()				Pop all elements
 *	IsEmpty()			Return TRUE if stack is empty
 *	GetSize()			Return number of bytes a stack element occupies
 *
 ****/
class Stack
   {
public:
	Stack(int size = sizeof(void*));
	Stack();
	int		GetSize() const;
	void*	Top() const;
	void*	Push(void*);
	void*	Push();
	void	Pop();
	void	Empty();
	int		IsEmpty() const;

Protected:
	struct StackElem* m_Top;
	struct StackElem* m_Free;
	int		m_Size;
   };

#else
/****
 *
 * Stack: General push-down stack object
 * Implements a stack of fixed-size blocks that grows dynamically.
 *
 * Operations:
 *	Stk_Init			Initialize stack for elements of N bytes each
 *	Stk_Push			Copy and push given element
 *	Stk_Pop				Pop top of stack
 *  Stk_Top				Return top of stack
 *	Stk_Empty			Pop all elements
 *	Stk_IsEmpty			Return TRUE if stack is empty
 *	Stk_GetSize			Return number of bytes a stack element occupies
 *
 ****/
typedef struct Stack
   {
	struct StackElem* m_Top;
	struct StackElem* m_Free;
	int		m_Size;
   } Stack;

#endif

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

/*
 * These routines are used internally by the C++ implementation
 * as well as being the public API for the C binding
 */
void	Stk_Init(Stack* s, int elem_size);
void*	Stk_Push(Stack* s, void* p);
void	Stk_Pop(Stack* s);
void	Stk_Empty(Stack* s);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#include <graphics/pipe/stack.inl>


#endif /* __GRAPHICS_PIPE_STACK_H */
