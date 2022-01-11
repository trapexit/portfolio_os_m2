/* @(#) art.h 96/09/07 1.2 */

#ifndef __ART_H
#define __ART_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __REQ_H
#include "req.h"
#endif


/*****************************************************************************/


Err LoadArt(StorageReq *req);
void UnloadArt(StorageReq *req);


/*****************************************************************************/


#endif /* __ART_H */
