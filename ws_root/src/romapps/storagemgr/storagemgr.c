/* @(#) storagemgr.c 96/02/23 1.6 */

#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/frame2d/spriteobj.h>
#include <ui/requester.h>
#include <stdio.h>


/*****************************************************************************/


Err main(void)
{
Err         result;
StorageReq *req;
SpriteObj  *title;
Point2      corner;

    title = Spr_Create(NULL);
    if (title)
    {
        result = Spr_LoadUTF(title, "StorMgr-Title.utf");
        if (result >= 0)
        {
            Spr_SetTextureAttr(title, TXA_TextureEnable,   1);
            Spr_SetDBlendAttr (title, DBLA_SrcInputEnable, 0);
            Spr_SetDBlendAttr (title, DBLA_BlendEnable,    0);
            Spr_SetDBlendAttr (title, DBLA_Discard,        0);
            Spr_SetTextureAttr(title, TXA_ColorOut,        TX_BlendOutSelectTex);
            Spr_ResetCorners  (title, SPR_TOPLEFT);

            corner.x = 106;
            corner.y = 12;
            Spr_SetPosition(title, &corner);

            result = OpenRequesterFolio();
            if (result >= 0)
            {
                result = CreateStorageReqVA(&req,
                              STORREQ_TAG_OPTIONS, (STORREQ_OPTION_EXIT |
                                                    STORREQ_OPTION_COPY |
                                                    STORREQ_OPTION_MOVE |
                                                    STORREQ_OPTION_DELETE |
                                                    STORREQ_OPTION_RENAME |
                                                    STORREQ_OPTION_CHANGEDIR |
                                                    STORREQ_OPTION_CREATEDIR),
                              STORREQ_TAG_PROMPT, title,
                              TAG_END);
                if (result >= 0)
                {
                    result = DisplayStorageReq(req);
                    DeleteStorageReq(req);
                }
                CloseRequesterFolio();
            }

#ifdef BUILD_STRINGS
            if (result < 0)
            {
                printf("StorageManager failed: ");
                PrintfSysErr(result);
            }
#endif
        }
        Spr_Delete(title);
    }
    else
    {
        result = NOMEM;
    }

    return result;
}
