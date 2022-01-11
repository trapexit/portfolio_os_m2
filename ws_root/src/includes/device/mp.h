#ifndef __DEVICE_MP_H
#define __DEVICE_MP_H


/******************************************************************************
**
**  @(#) mp.h 96/09/11 1.6
**
**  Portfolio multi-processor dispatching services.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


typedef Err (* MPFunc)(void *data);

/* this is the packet of information supplied to the MP_CMD_DISPATCH command */
typedef struct
{
    MPFunc  mp_Code;	  /* ptr to function        */
    void   *mp_Data;	  /* ptr to params          */
    uint32 *mp_Stack;     /* ptr to top of stack    */
    uint32  mp_StackSize; /* size of stack in bytes */
    uint32  mp_Result;	  /* result from func       */
} MPPacket;


/*****************************************************************************/


Item CreateMPIOReq(bool simulation);
Err  DeleteMPIOReq(Item mpioreq);
Err  DispatchMPFunc(Item mpioreq, MPFunc func, void *data,
                    uint32 *stack, uint32 stackSize, Err *result);

/* determine which CPU the caller is on */
bool IsMasterCPU(void);
#define IsSlaveCPU() !IsMasterCPU()


/*****************************************************************************/


#endif /* __DEVICE_MP_H */
