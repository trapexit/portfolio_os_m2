/******************************************************************************
**
**  @(#) threadhelper.c 96/04/30 1.19
**
******************************************************************************/
#include <kernel/mem.h>
#include <streaming/threadhelper.h>
#include <streaming/dserror.h>


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DSUtils -name NewThread
|||	Create a new thread and pass it two arguments.
|||
|||	  Synopsis
|||
|||	    Item NewThread(void *threadProcPtr,
|||	                   int32 stackSize,
|||	                   int32 threadPriority,
|||	                   char *threadName,
|||	                   int32 argc,
|||	                   void *argp)
|||
|||	  Description
|||
|||	    Create a new thread given the proc pointer, stack size, thread
|||	    priority, and thread name, and two arguments to pass to it
|||	    (known as argc and argv by convention).
|||
|||	    This just calls CreateThreadVA() and supplies tag args to pass two
|||	    arguments to the new thread. NewThread() was much more useful before
|||	    CreateThreadVA() existed.
|||
|||	    To dispose the thread call DisposeThread( ) (which is just a macro for
|||	    DeleteThread()). Or have the thread call exit() or simply return. In
|||	    any case, the OS will automatically deallocate the thread's stack.
|||
|||	  Arguments
|||
|||	    threadProcPtr
|||	        The new thread begins with a call to this procedure. threadProcPtr
|||	        should really be declared (*threadProcPtr)(int32 argc, void *argp),
|||	        but the caller would still end up type casting due to argc/argp.
|||
|||	    stackSize
|||	        Size of the stack space to allocate for the new thread. NewThread()
|||	        will round it up to a mod 8 value and allocate it mod 8 aligned.
|||	        If there isn't enough memory to allocate it, NewThread() will return
|||	        NOMEM.
|||
|||	    threadPriority
|||	        Priority for the new thread.
|||
|||	    threadName
|||	        A name (a C character string) for the new thread.
|||	        If NULL, NewThread() will return BADNAME.
|||
|||	    argc
|||	        The first arg to pass to threadProcPtr( ). It's known as
|||	        argc and declared int32 per the C model main(argc, argv), but it
|||	        can be any 32-bit value that threadProcPtr( ) wants.
|||
|||	    argp
|||	        The second arg to pass to threadProcPtr( ). It's known as
|||	        argp and declared void* per the C model main(argc, argv), but it
|||	        can be any 32-bit value that threadProcPtr( ) wants.
|||
|||	  Return Value
|||
|||	        The item number of the new thread Item, or an error code such as
|||	        NOMEM (couldn't allocate the stack) or BADNAME (the threadName can't
|||	        be NULL).
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/threadhelper.h>, libdsutils.a
|||
|||	  See Also
|||
|||	    DisposeThread( ), DeleteThread(), CreateItem(), CreateThread().
|||
 ******************************************************************************/
typedef void (*ThreadProcPtr)();
/* [TBD] Declare the first arg as a ThreadProcPtr. */
Item	NewThread(void *threadProcPtr, int32 stackSize, int32 threadPriority,
		char *threadName, int32 argc, void *argp)
	{
	return CreateThreadVA((ThreadProcPtr)(int32)threadProcPtr,
		threadName, threadPriority, ALLOC_ROUND(stackSize, 8),
		CREATETASK_TAG_ARGC,		(void *)argc,	/* first arg to thread */
		CREATETASK_TAG_ARGP,		(void *)argp,	/* second arg to thread */
		TAG_END);
	}
