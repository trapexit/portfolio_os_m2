/* @(#) utils.h 96/09/29 1.3 */

#ifndef __UTILS_H
#define __UTILS_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif


/*****************************************************************************/


bool IsFilesystemName(const char *name);
void stccpy(char *to, const char *from, uint32 maxChars);
Err GetFullPath(char *path, uint32 numBytes);

void SetInClipping(GState *gs, uint32 x0, uint32 y0, uint32 x1, uint32 y1);
void SetOutClipping(GState *gs, uint32 x0, uint32 y0, uint32 x1, uint32 y1);
void ClearClipping(GState *gs);
void ShadedRect(GState *gs, int32 x1, int32 y1, int32 x2, int32 y2, const Color4 *c);
void ShadedStrip(GState *gs, uint32 numPoints, struct Point2 *p, Color4 *c);


/*****************************************************************************/


#endif /* __UTILS_H */
