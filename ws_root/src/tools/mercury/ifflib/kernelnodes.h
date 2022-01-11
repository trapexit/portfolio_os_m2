#ifndef __KERNEL_KERNELNODES_H
#define __KERNEL_KERNELNODES_H


/******************************************************************************
**
**  @(#) kernelnodes.h 95/07/01 1.11
**
**  Kernel node and item types
**
******************************************************************************/


/* subsystem */
#define KERNELNODE	1

/* node types */
#define MEMFREENODE	1
#define FIRQLISTNODE	2
#define MEMHDRNODE	3
#define FOLIONODE	4	  /* see folio.h      */
#define TASKNODE	5	  /* see task.h       */
#define FIRQNODE	6	  /* private          */
#define SEMA4NODE	7	  /* see semaphore.h  */
#define SEMAPHORENODE	SEMA4NODE /* see semaphore.h  */
#define SEMA4WAIT	8	  /* private          */
#define MESSAGENODE	9	  /* see msgport.h    */
#define MSGPORTNODE	10	  /* see msgport.h    */
#define MEMLISTNODE	11
#define ROMTAGNODE	12        /* private          */
#define DRIVERNODE	13	  /* see driver.h     */
#define IOREQNODE	14	  /* see io.h         */
#define	DEVICENODE	15	  /* see device.h     */
#define TIMERNODE	16	  /* private          */
#define ERRORTEXTNODE	17	  /* see operror.h    */
#define MEMLOCKNODE     18        /* private          */
#define MODULENODE	19	  /* see loader3do.h  */
#define DDFNODE		20	  /* see ddf.h	      */
#define INTLEECHNODE    21	  /* private          */


/*****************************************************************************/


#endif	/* __KERNEL_KERNELNODES_H */
