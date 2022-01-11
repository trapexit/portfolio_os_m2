
/*
 *  @(#) iconshared.c 96/02/28 1.1
 *  Shared Icon code.
 */

#include <kernel/mem.h>
#include <graphics/frame2d/loadtxtr.h>
#include <ui/icon.h>

Err UnloadIcon(Icon *i)
{
    if (i)
    {
        Spr_UnloadTexture(&i->SpriteObjs);
        FreeMem(i, sizeof(Icon));
    }
    else
        return(ICON_ERR_BADPTR);
        
    return(0);
}


