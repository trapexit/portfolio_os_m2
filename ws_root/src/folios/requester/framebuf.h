/* @(#) framebuf.h 96/09/12 1.3 */

#ifndef __FRAMEBUF_H
#define __FRAMEBUF_H


/*****************************************************************************/


#ifndef __REQ_H
#include "req.h"
#endif


/*****************************************************************************/


Err CreateFrameBuffers(StorageReq *req);
void DeleteFrameBuffers(StorageReq *req);
void DoNextFrame(StorageReq *req, AnimList *animList);
void SnapshotFrameBuf(StorageReq *req, const char *name);


/*****************************************************************************/


#endif /* __FRAMEBUF_H */
