/* @(#) console.c 96/07/18 1.20 */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/semaphore.h>
#include <kernel/tags.h>
#include <kernel/cache.h>
#include <graphics/font.h>
#include <graphics/view.h>
#include <misc/debugconsole.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


/* general attributes of the view we're gonna be creating */
#define BITMAP_TYPE    BMTYPE_16
#define DISPLAY_TYPE   VIEWTYPE_16_640_LACE
#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 200
#define DISPLAY_DEPTH  16
#define DISPLAY_TOP    0
#define DISPLAY_FBCNT  2
#define DISPLAY_USEZ   1

#define FOREGROUND_RED   0x55
#define FOREGROUND_GREEN 0xff
#define FOREGROUND_BLUE  0x55
#define FOREGROUND_ALPHA 0

#define BACKGROUND_RED   0
#define BACKGROUND_GREEN 0
#define BACKGROUND_BLUE  0
#define BACKGROUND_ALPHA 0

#define BYTES_PER_PIXEL  2


/*****************************************************************************/


static Item        lock = -1;
static GState     *gs;
static Item        bitmaps[1];
static Bitmap     *bm;
static uint32      lineHeight;
static uint32      descentHeight;
static Item        view;
static Item        font;
static char        buffer[128];
static uint32      index;
static uint32      penX;
static uint32      penY;

#if 0
used for F2_FillRect(), which we can't use right now since it doesn't
appear functional.

static const Color4 backgroundColor = {BACKGROUND_RED / 256.,
                                       BACKGROUND_GREEN / 256.,
                                       BACKGROUND_BLUE / 256.,
                                       BACKGROUND_ALPHA / 256.};
#endif


/*****************************************************************************/


Err CreateDebugConsole(const TagArg *tags)
{
Err       err;
TagArg   *tag;
void     *arg;
uint32    top;
uint32    height;
uint32    type;
CharacterData cd;

    top    = DISPLAY_TOP;
    height = DISPLAY_HEIGHT;
    type   = DISPLAY_TYPE;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        arg = tag->ta_Arg;
        switch (tag->ta_Tag)
        {
            case DEBUGCONSOLE_TAG_TOP   : top = (uint32)arg;
                                          break;

            case DEBUGCONSOLE_TAG_HEIGHT: height = (uint32)arg;
                                          break;

            case DEBUGCONSOLE_TAG_TYPE  : type = (uint32)arg;
                                          break;

            default                     : return BADTAG;
        }
    }

    err = OpenGraphicsFolio();
    if (err >= 0)
    {
        err = OpenFontFolio();
        if (err >= 0)
        {
            font = err = OpenFont("default_14");
            if (font >= 0)
            {
                GetCharacterData(&cd, font, 'A');
                lineHeight    = cd.cd_CharHeight + cd.cd_Leading + 1;
                descentHeight = cd.cd_Descent;

                lock = err = CreateSemaphore(NULL,0);
                if (lock >= 0)
                {
                    gs = GS_Create();
                    if (gs)
                    {
                        err = GS_AllocLists(gs, 2, 1024);
                        if (err >= 0)
                        {
                            err = GS_AllocBitmaps(bitmaps, DISPLAY_WIDTH, height, BITMAP_TYPE, 1, FALSE);
                            if (err >= 0)
                            {
                                GS_SetDestBuffer(gs, bitmaps[0]);

                                view = err = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                                                VIEWTAG_VIEWTYPE, type,
                                                VIEWTAG_TOPEDGE,  top,
                                                VIEWTAG_BITMAP,   bitmaps[0],
                                                TAG_END);
                                if (view >= 0)
                                {
                                    penX = 0;
                                    penY = cd.cd_Ascent + 1;

                                    CLT_SetSrcToCurrentDest(gs);
                                    CLT_ClearFrameBuffer(gs, BACKGROUND_RED / 256.,
                                                             BACKGROUND_GREEN / 256.,
                                                             BACKGROUND_BLUE / 256.,
                                                             BACKGROUND_ALPHA / 256.,
                                                             TRUE, FALSE);
                                    GS_SendList(gs);
                                    GS_WaitIO(gs);

                                    bm = (Bitmap *)LookupItem(bitmaps[0]);

                                    err = AddViewToViewList(view, 0);
                                    if (err >= 0)
                                    {
                                        return 0;
                                    }
                                    DeleteItem(view);
                                }
                                GS_FreeBitmaps(bitmaps, 1);
                            }
                            GS_FreeLists(gs);
                        }
                    }
                    else
                    {
                        err = NOMEM;
                    }
                    DeleteSemaphore(lock);
                }
                CloseFont(font);
            }
            CloseFontFolio();
        }
        CloseGraphicsFolio();
    }

    return err;
}


/*****************************************************************************/


void DeleteDebugConsole(void)
{
    if (lock >= 0)
    {
        RemoveView(view);
        DeleteItem(view);
        GS_FreeBitmaps(bitmaps, 1);
        GS_FreeLists(gs);
        GS_Delete(gs);
        DeleteSemaphore(lock);
        CloseFont(font);
        CloseFontFolio();
        CloseGraphicsFolio();

        lock = -1;
    }
}


/*****************************************************************************/


static void PrintString(const char *text, uint32 len)
{
uint32  nuke;
PenInfo pen;

    if (penY + descentHeight >= bm->bm_Height)
    {
        nuke = lineHeight * bm->bm_Width * BYTES_PER_PIXEL;

        GS_WaitIO(gs);

        /* would really be nice to be able to use the TE for this... */
        memcpy(bm->bm_Buffer,
               (void *)((uint32)bm->bm_Buffer + nuke),
               bm->bm_BufferSize - nuke);
        memset((void *)((uint32)bm->bm_Buffer + bm->bm_BufferSize - nuke), 0, nuke);

        FlushDCacheAll(0);

        penY -= lineHeight;
    }

    memset(&pen, 0, sizeof(PenInfo));
    pen.pen_XScale  = 1.0;
    pen.pen_YScale  = 1.0;
    pen.pen_X       = penX;
    pen.pen_Y       = penY;
    pen.pen_FgColor = ((FOREGROUND_RED << 16) |
                       (FOREGROUND_GREEN << 8) |
                       FOREGROUND_BLUE);
    pen.pen_BgColor = ((BACKGROUND_RED << 16) |
                       (BACKGROUND_GREEN << 8) |
                       BACKGROUND_BLUE);

    DrawString(gs, font, &pen, text, len);
    GS_SendList(gs);

    penX = pen.pen_X;
}


/*****************************************************************************/


static int32 ProcessChar(char ch)
{
    buffer[index++] = ch;

    if ((index >= sizeof(buffer) - 1) || (ch == '\n'))
    {
        PrintString(buffer, index);
        index = 0;

        if (ch == '\n')
        {
            penX = 0;
            penY += lineHeight;
        }
    }

    return 0;
}


/*****************************************************************************/


void DebugConsolePrintf(const char *text, ...)
{
va_list args;

    if (LockSemaphore(lock,SEM_WAIT) < 0)
        return;

    va_start(args,text);
    vcprintf(text, (OutputFunc)ProcessChar, NULL, args);
    va_end(args);

    if (index)
    {
        PrintString(buffer, index);
        index = 0;
    }

    UnlockSemaphore(lock);
}


/*****************************************************************************/


void DebugConsoleClear(void)
{
    if (LockSemaphore(lock,SEM_WAIT) < 0)
        return;

    CLT_ClearFrameBuffer(gs, BACKGROUND_RED / 256.,
                             BACKGROUND_GREEN / 256.,
                             BACKGROUND_BLUE / 256.,
                             BACKGROUND_ALPHA / 256.,
                             TRUE, FALSE);
    GS_SendList(gs);

    UnlockSemaphore(lock);
}


/*****************************************************************************/


void DebugConsoleMove(uint32 x, uint32 y)
{
    if (LockSemaphore(lock,SEM_WAIT) < 0)
        return;

    penX = x;
    penY = y;

    UnlockSemaphore(lock);
}
