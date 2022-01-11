/* @(#) req.c 96/09/12 1.5 */

#include <kernel/cache.h>
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/tags.h>
#include <kernel/list.h>
#include <file/fsutils.h>
#include <file/filefunctions.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <graphics/font.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <audio/audio.h>
#include <ui/icon.h>
#include <file/filesystem.h>
#include <international/intl.h>
#include <misc/savegame.h>
#include <string.h>
#include <stdio.h>
#include "req.h"
#include "utils.h"
#include "eventmgr.h"
#include "msgstrings.h"
#include "framebuf.h"
#include "eventloops.h"
#include "ioserver.h"
#include "art.h"
#include "sound.h"


/*****************************************************************************/


#define IsStorageReq(req) (req && (req->sr_Cookie == req))


/*****************************************************************************/


static const TagArg searchForFile[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS, (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

Err CreateStorageReq(StorageReq **req, const TagArg *tags)
{
Err  result;
char path[90];

#ifdef MEMDEBUG
    CreateMemDebug(NULL);
    ControlMemDebug(MEMDEBUGF_ALLOC_PATTERNS |
                    MEMDEBUGF_FREE_PATTERNS |
                    MEMDEBUGF_PAD_COOKIES |
                    MEMDEBUGF_CHECK_ALLOC_FAILURES |
                    MEMDEBUGF_KEEP_TASK_DATA);
#endif

    *req = AllocMem(sizeof(StorageReq), MEMTYPE_FILL);
    if (*req)
    {
        result = OpenGraphicsFolio();
        if (result >= 0)
        {
            result = OpenFontFolio();
            if (result >= 0)
            {
                result = intlOpenFolio();
                if (result >= 0)
                {
                    result = OpenDateFolio();
                    if (result >= 0)
                    {
                        result = OpenSaveGameFolio();
                        if (result >= 0)
                        {
                            result = OpenFSUtilsFolio();
                            if (result >= 0)
                            {
                                result = OpenAudioFolio();
                                if (result >= 0)
                                {
#if 0
                                    result = OpenIconFolio();
                                    if (result >= 0)
#endif
                                    {
                                        (*req)->sr_Cookie             = *req;
                                        (*req)->sr_ViewList           = 0;
                                        (*req)->sr_Font               = -1;
                                        (*req)->sr_Locale             = -1;
                                        (*req)->sr_DirectoryBuffer[0] = '/';
                                        (*req)->sr_Options            = 0;

                                        result = ModifyStorageReq(*req,tags);
                                        if (result >= 0)
                                        {
                                            if ((*req)->sr_Font < 0)
                                            {
                                                FindFileAndIdentify(path, sizeof(path), "System.m2/Requester/StorMgr_9.font", searchForFile);

                                                (*req)->sr_Font = result = OpenFont(path);
                                                if ((*req)->sr_Font >= 0)
                                                {
                                                    (*req)->sr_CloseFont = TRUE;
                                                }
                                            }

                                            if (result >= 0)
                                            {
                                                if ((*req)->sr_Locale < 0)
                                                {
                                                    (*req)->sr_Locale = result = intlOpenLocale(NULL);
                                                    if ((*req)->sr_Locale >= 0)
                                                    {
                                                        (*req)->sr_CloseLocale = TRUE;
                                                    }
                                                }
                                            }

                                            if (result >= 0)
                                            {
                                                result = LoadArt((*req));
                                                if (result >= 0)
                                                {
                                                    result = LoadSounds((*req));
                                                    if (result >= 0)
                                                    {
                                                        LoadStrings(*req);
                                                        return 0;
                                                    }
                                                    UnloadArt((*req));
                                                }
                                            }

                                            if ((*req)->sr_CloseLocale)
                                                intlCloseLocale((*req)->sr_Locale);

                                            if ((*req)->sr_CloseFont)
                                                CloseFont((*req)->sr_Font);
                                        }
                                        CloseIconFolio();
                                    }
                                    CloseAudioFolio();
                                }
                                CloseFSUtilsFolio();
                            }
                            CloseSaveGameFolio();
                        }
                        CloseDateFolio();
                    }
                    intlCloseFolio();
                }
                CloseFontFolio();
            }
            CloseGraphicsFolio();
        }
        FreeMem(*req, sizeof(StorageReq));
    }
    else
    {
        result = REQ_ERR_NOMEM;
    }

    return result;
}


/*****************************************************************************/


Err DeleteStorageReq(StorageReq *req)
{
    if (!IsStorageReq(req))
        return REQ_ERR_BADREQ;

    req->sr_Cookie = NULL;

    UnloadStrings(req);
    UnloadArt(req);
    UnloadSounds(req);

    if (req->sr_CloseLocale)
        intlCloseLocale(req->sr_Locale);

    if (req->sr_CloseFont)
        CloseFont(req->sr_Font);

    FreeMem(req,sizeof(StorageReq));

    CloseAudioFolio();
    CloseIconFolio();
    CloseFSUtilsFolio();
    CloseFontFolio();
    CloseGraphicsFolio();
    intlCloseFolio();

#ifdef MEMDEBUG
    DumpMemDebug(NULL);
    DeleteMemDebug();
#endif

    return 0;
}


/*****************************************************************************/


Err QueryStorageReq(StorageReq *req, const TagArg *tags)
{
TagArg *tag;
uint32 *data;

    if (!IsStorageReq(req))
        return REQ_ERR_BADREQ;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        data = (uint32 *)tag->ta_Arg;
        if (!IsMemWritable(data,4))
            return REQ_ERR_BADPTR;

        switch (tag->ta_Tag)
        {
            case STORREQ_TAG_VIEW_LIST : *data = req->sr_ViewList;
                                         break;

            case STORREQ_TAG_FONT      : *data = req->sr_Font;
                                         break;

            case STORREQ_TAG_LOCALE    : *data = req->sr_Locale;
                                         break;

            case STORREQ_TAG_OPTIONS   : *data = req->sr_Options;
                                         break;

            case STORREQ_TAG_PROMPT    : *data = (uint32)req->sr_Prompt;
                                         break;

            case STORREQ_TAG_FILTERFUNC: *data = (uint32)req->sr_FilterFunc;
                                         break;

            case STORREQ_TAG_DIRECTORY : strcpy((char *)data, req->sr_DirectoryBuffer);
                                         break;

            case STORREQ_TAG_FILE      : strcpy((char *)data, req->sr_FileBuffer);
                                         break;

            default                    : return REQ_ERR_BADTAG;
        }
    }

    return 0;
}


/*****************************************************************************/


Err ModifyStorageReq(StorageReq *req, const TagArg *tags)
{
TagArg *tag;
void   *data;

    if (!IsStorageReq(req))
        return REQ_ERR_BADREQ;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        data = tag->ta_Arg;
        switch (tag->ta_Tag)
        {
            case STORREQ_TAG_VIEW_LIST : req->sr_ViewList = (Item)data;
                                         break;

            case STORREQ_TAG_FONT      : req->sr_Font = (Item)data;
                                         break;

            case STORREQ_TAG_LOCALE    : req->sr_Locale = (Item)data;
                                         break;

            case STORREQ_TAG_OPTIONS   : if ((uint32)data & ~(STORREQ_OPTION_OK |
                                                              STORREQ_OPTION_LOAD |
                                                              STORREQ_OPTION_SAVE |
                                                              STORREQ_OPTION_CANCEL |
                                                              STORREQ_OPTION_EXIT |
                                                              STORREQ_OPTION_QUIT |
                                                              STORREQ_OPTION_COPY |
                                                              STORREQ_OPTION_MOVE |
                                                              STORREQ_OPTION_DELETE |
                                                              STORREQ_OPTION_RENAME |
                                                              STORREQ_OPTION_CREATEDIR |
                                                              STORREQ_OPTION_CHANGEDIR))
                                         {
                                             return REQ_ERR_BADOPTION;
                                         }
                                         req->sr_Options = (uint32)data;
                                         break;

            case STORREQ_TAG_PROMPT    : req->sr_Prompt = (SpriteObj *)data;
                                         break;

            case STORREQ_TAG_FILTERFUNC: req->sr_FilterFunc = (FileFilterFunc)data;
                                         break;

            case STORREQ_TAG_DIRECTORY : stccpy(req->sr_DirectoryBuffer, (char *)data, sizeof(req->sr_DirectoryBuffer));
                                         break;

            case STORREQ_TAG_FILE      : stccpy(req->sr_FileBuffer, (char *)data, sizeof(req->sr_FileBuffer));
                                         break;

            default                    : return REQ_ERR_BADTAG;
        }
    }

    return 0;
}


/*****************************************************************************/


Err DisplayStorageReq(StorageReq *req)
{
Err result;

    if (!IsStorageReq(req))
        return REQ_ERR_BADREQ;

    GetCharacterData(&req->sr_SampleChar, req->sr_Font, 'M');

    req->sr_EventPort = result = CreateMsgPort(NULL,0,0);
    if (req->sr_EventPort >= 0)
    {
        result = ConnectEventMgr(&req->sr_EventMgr, req->sr_EventPort);
        if (result >= 0)
        {
            result = CreateIOServer(&req->sr_IOServer, req);
            if (result >= 0)
            {
                req->sr_IOStatusPort = result = CreateMsgPort(NULL, 0, 0);
                if (req->sr_IOStatusPort >= 0)
                {
                    result = CreateFrameBuffers(req);
                    if (result >= 0)
                    {
                        req->sr_RenderSig = result = AllocSignal(0);
                        if (req->sr_RenderSig > 0)
                        {
                            req->sr_View = result = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                                                        VIEWTAG_VIEWTYPE,    VIEWTYPE_16_640,
                                                        VIEWTAG_AVGMODE,      AVGMODE_V,
                                                        VIEWTAG_BITMAP,    req->sr_FrameBuffers[req->sr_CurrentFrameBuffer ^ 1],
                                                        VIEWTAG_RENDERSIGNAL, req->sr_RenderSig,
                                                        VIEWTAG_BESILENT,    TRUE,
                                                        TAG_END);
                            if (req->sr_View >= 0)
                            {
                                result = AddViewToViewList(req->sr_View, req->sr_ViewList);
                                if (result >= 0)
                                {
                                    result = MainLoop(req);
                                    RemoveView(req->sr_View);
                                }
                                DeleteItem(req->sr_View);
                            }
                            FreeSignal(req->sr_RenderSig);
                        }
                        else if (req->sr_RenderSig == 0)
                        {
                            result = REQ_ERR_NOSIGNALS;
                        }
                        DeleteFrameBuffers(req);
                    }
                    DeleteMsgPort(req->sr_IOStatusPort);
                }
                DeleteIOServer(req->sr_IOServer);
            }
            DisconnectEventMgr(req->sr_EventMgr);
        }
        DeleteMsgPort(req->sr_EventPort);
    }

    return result;
}
