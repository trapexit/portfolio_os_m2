/* @(#) art.c 96/09/07 1.3 */

#include <kernel/types.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/loadtxtr.h>
#include <file/filefunctions.h>
#include <ui/requester.h>
#include <stdio.h>
#include "art.h"



/*****************************************************************************/


void LoadImage(StorageReq *req, List *SpriteList, SpriteObj **image, bool opaque)
{
SpriteObj *sp;

    if (req->sr_CurrentSprite == NULL)
        req->sr_CurrentSprite = (SpriteObj *)FirstNode(SpriteList);

#ifdef BUILD_PARANOIA
    if (IsNode(SpriteList, req->sr_CurrentSprite) == FALSE)
    {
        printf("ERROR: not enough textures in art file\n");
        *image = NULL;
        return;
    }
#endif

    *image = req->sr_CurrentSprite;
    sp     = *image;
    req->sr_CurrentSprite = (SpriteObj *)NextNode(req->sr_CurrentSprite);

    Spr_SetTextureAttr(sp, TXA_TextureEnable,   1);
    Spr_SetDBlendAttr (sp, DBLA_SrcInputEnable, 0);
    Spr_SetDBlendAttr (sp, DBLA_BlendEnable,    0);
    Spr_SetDBlendAttr (sp, DBLA_Discard,        0);

    if (opaque)
    {
        Spr_SetTextureAttr(sp, TXA_ColorOut, TX_BlendOutSelectTex);
    }
    else
    {
        Spr_SetTextureAttr(sp, TXA_ColorOut,       TX_BlendOutSelectBlend);
        Spr_SetTextureAttr(sp, TXA_FirstColor,   TX_ColorSelectTexColor);
        Spr_SetTextureAttr(sp, TXA_SecondColor, TX_ColorSelectConstColor);
        Spr_SetTextureAttr(sp, TXA_BlendOp,     TX_BlendOpMult);
        Spr_SetTextureAttr(sp, TXA_BlendColorSSB0, 0x00FFFFFF);

        if (Spr_GetPIP(sp))
            Spr_SetTextureAttr(sp, TXA_PipColorSelect, TX_PipSelectColorTable);
        else
            Spr_SetTextureAttr(sp, TXA_PipColorSelect, TX_PipSelectTexture);
    }

    Spr_ResetCorners(sp, SPR_TOPLEFT);
}


/*****************************************************************************/


void LoadBatch(StorageReq *req, List *SpriteList, SpriteObj **array, bool opaque, uint32 numImages)
{
uint32 i;

    for (i = 0; i < numImages; i++)
        LoadImage(req, SpriteList, &array[i], opaque);
}


/*****************************************************************************/


static const TagArg searchForFile[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

Err LoadArt(StorageReq *req)
{
Err        result;
List       sprites;
SpriteObj *sp;
Node      *node;
uint32     i;
char       path[90];

    FindFileAndIdentify(path, sizeof(path), "System.m2/Requester/StorMgr.utf", searchForFile);

    result = Spr_LoadTextureVA(&req->sr_Sprites, LOADTEXTURE_TAG_FILENAME, path, TAG_END);
    if (result >= 0)
    {
        FindFileAndIdentify(path, sizeof(path), "System.m2/Requester/TextEntry.utf", searchForFile);
        PrepList(&sprites);
        result = Spr_LoadTextureVA(&sprites, LOADTEXTURE_TAG_FILENAME, path, TAG_END);

        if (result >= 0)
        {
            while(node = RemHead(&sprites))
                AddTail(&req->sr_Sprites, node);
        }

        LoadBatch(req, &req->sr_Sprites, req->sr_BgSlices,                  TRUE,   NUM_BG_SLICES);
        LoadImage(req, &req->sr_Sprites, &req->sr_BarImage,                 TRUE);
        LoadBatch(req, &req->sr_Sprites, req->sr_BoxImages,                 FALSE,  NUM_BOX_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_ButtonImages,              FALSE,  NUM_BUTTON_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_CancelImages,              FALSE,  NUM_CANCEL_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_CopyImages,                FALSE,  NUM_COPY_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_CreateDirImages,           FALSE,  NUM_CREATEDIR_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_DeleteImages,              FALSE,  NUM_DELETE_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_FolderImages,              FALSE,  NUM_FOLDER_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_HandInsertImages,          FALSE,  NUM_HANDINSERT_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_LoadImages,                FALSE,  NUM_LOAD_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_LockImages,                FALSE,  NUM_LOCK_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_MoveImages,                FALSE,  NUM_MOVE_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_OKImages,                  FALSE,  NUM_OK_IMAGES);
        LoadImage(req, &req->sr_Sprites, &req->sr_OverlayImage,             TRUE);
        LoadBatch(req, &req->sr_Sprites, req->sr_QuestionImages,            FALSE,  NUM_QUESTION_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_RenameImages,              FALSE,  NUM_RENAME_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_SaveImages,                FALSE,  NUM_SAVE_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_StopImages,                FALSE,  NUM_STOP_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_SuitcaseImages,            FALSE,  NUM_SUITCASE_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_WorkingImages,             FALSE,  NUM_WORKING_IMAGES);

        LoadBatch(req, &req->sr_Sprites, req->sr_TextEntryBgSlices,             FALSE,  NUM_TEXTENTRY_BG_SLICES);
        LoadBatch(req, &req->sr_Sprites, req->sr_TextEntryBigButtonImages,      FALSE,  NUM_TEXTENTRY_BIGBUTTON_IMAGES);
        LoadBatch(req, &req->sr_Sprites, req->sr_TextEntrySmallButtonImages,    FALSE,  NUM_TEXTENTRY_SMALLBUTTON_IMAGES);

        Spr_SetTextureAttr(req->sr_OverlayImage, TXA_ColorOut, TX_BlendOutSelectTex);

        for (i = 0; i < NUM_BG_SLICES; i++)
        {
            sp = req->sr_BgSlices[i];
            Spr_SetTextureAttr(sp, TXA_ColorOut,    TX_BlendOutSelectTex);
            Spr_SetDBlendAttr(sp, DBLA_DiscardRGB0, 0);
        }

        for (i = 0; i < NUM_QUESTION_IMAGES; i++)
        {
            sp = req->sr_QuestionImages[i];
            Spr_SetTextureAttr(sp, TXA_ColorOut,    TX_BlendOutSelectTex);
            Spr_SetDBlendAttr(sp, DBLA_DiscardRGB0, 0);
        }

        for (i = 0; i < NUM_TEXTENTRY_BG_SLICES; i++)
        {
            sp = req->sr_TextEntryBgSlices[i];
            Spr_SetTextureAttr(sp, TXA_ColorOut,    TX_BlendOutSelectTex);
            Spr_SetDBlendAttr(sp, DBLA_DiscardRGB0, 0);
        }
    }

    return result;
}


/*****************************************************************************/


void UnloadArt(StorageReq *req)
{
    Spr_UnloadTexture(&req->sr_Sprites);
}
