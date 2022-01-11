#ifndef __KERNEL_SPINLOCK_H
#define __KERNEL_SPINLOCK_H


/******************************************************************************
**
**  @(#) spinlock.h 96/08/23 1.1
**
**  Spin locks to synchronize the two processors.
**
**  WARNING: Only use spin locks to synchronize code running on different
**           processors, and not to synchronize two threads on the same
**           processor. Use semaphores for this purpose.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* a handle to a spin lock */
typedef struct SpinLock SpinLock;


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


Err  CreateSpinLock(SpinLock **lock);
Err  DeleteSpinLock(SpinLock *lock);
bool ObtainSpinLock(SpinLock *lock);
void ReleaseSpinLock(SpinLock *lock);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* __KERNEL_SPINLOCK_H */
