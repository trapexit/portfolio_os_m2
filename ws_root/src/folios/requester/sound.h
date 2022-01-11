/* @(#) sound.h 96/09/07 1.2 */

#ifndef __SOUND_H
#define __SOUND_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __REQ_H
#include "req.h"
#endif


/*****************************************************************************/


Err LoadSounds(StorageReq *req);
void UnloadSounds(StorageReq *req);
void PlaySound(StorageReq *req, Sounds sound);
void StopSound(StorageReq *req);


/*****************************************************************************/


#endif /* __SOUND_H */
