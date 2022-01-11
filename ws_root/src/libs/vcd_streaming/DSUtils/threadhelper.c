/******************************************************************************
**
**  @(#) threadhelper.c 96/08/01 1.2
**
******************************************************************************/
#include <kernel/mem.h>
#include <streaming/threadhelper.h>
#include <streaming/dserror.h>


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
