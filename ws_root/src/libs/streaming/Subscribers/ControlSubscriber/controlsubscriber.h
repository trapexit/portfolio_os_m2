/******************************************************************************
**
**  @(#) controlsubscriber.h 96/03/04 1.9
**
******************************************************************************/
#ifndef __STREAMING_CTRLSUBSCRIBER_H
#define __STREAMING_CTRLSUBSCRIBER_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
#include <streaming/datastream.h>
#endif

/*****************************/
/* Public routine prototypes */
/*****************************/
#ifdef __cplusplus 
extern "C" {
#endif


Item NewCtrlSubscriber(DSStreamCBPtr streamCBPtr, int32 priority, Item msgItem);


#ifdef __cplusplus
}
#endif

#endif	/* __STREAMING_CTRLSUBSCRIBER_H */

