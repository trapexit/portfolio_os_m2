/******************************************************************************
**
**  @(#) threadhelper.h 96/03/04 1.8
**
******************************************************************************/
#ifndef __THREADHELPER_H__
#define __THREADHELPER_H__

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif


/* The following are useful for calling these helper routines and in writing
 * thread code. The current task priority is useful for creating new threads in
 * that you usually want to specify the new thread's priority as a value
 * relative to the creator task (higher or lower than the current task).
 * The thread parent is useful to threads that wish to communicate via with
 * the parent task via: AllocSignal(), Signal(), Wait(), & FreeSignal(). */
#define CURRENT_TASK_PRIORITY	((uint8)(CURRENTTASK->t.n_Priority))
#define THREAD_PARENT			((Item)CURRENTTASK->t_ThreadTask->t.n_Item)

/***********************************/
/* Prototypes for thread functions */
/***********************************/

#ifdef __cplusplus
extern "C" {
#endif

Item	NewThread(void *threadProcPtr, int32 stackSize, int32 threadPriority,
			char *threadName, int32 argc, void *argp);

#define DisposeThread(threadItem)	(DeleteThread(threadItem))

#ifdef __cplusplus
}
#endif

#endif

