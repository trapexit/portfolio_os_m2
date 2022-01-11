/* @(#) text_loop.c 96/10/30 1.7 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/list.h>
#include <kernel/time.h>
#include <kernel/task.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <file/fsutils.h>
#include <ui/icon.h>
#include <ui/requester.h>
#include <graphics/font.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <graphics/clt/gstate.h>
#include <stdio.h>
#include <string.h>
#include "req.h"
#include "eventmgr.h"
#include "dirscan.h"
#include "framebuf.h"
#include "msgstrings.h"
#include "utils.h"
#include "listviews.h"
#include "highlight.h"
#include "bg.h"
#include "animlists.h"
#include "overlays.h"
#include "controls.h"
#include "ioserver.h"
#include "hierarchy.h"
#include "boxes.h"
#include "sound.h"
#include "eventloops.h"


/*****************************************************************************/


static const char * const charSets[] =
{
    "12345+-abcdefghijklmnopqrstuvwxyz",
    "67890!?ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "1234567890!?-@#%&*_=\\[]{}<>,.\"':;",
    NULL
};

#define KEY_NOP       -1
#define KEY_SHIFT     -2
#define KEY_CLEAR     -3
#define KEY_BACKSPACE -4
#define KEY_OK        -5
#define KEY_CANCEL    -6

typedef struct
{
    uint16 ki_X;
    uint16 ki_Y;
    int16  ki_Key;
    bool   ki_Large;

    int16  ki_UpKey;
    int16  ki_DownKey;
    int16  ki_LeftKey;
    int16  ki_RightKey;
} KeyInfo;

static const KeyInfo row0[] =
{
    {75,  80, 0,  FALSE, KEY_NOP, 11, KEY_NOP, 1},
    {119, 80, 1,  FALSE, KEY_NOP, 12, 0,  2},
    {163, 80, 2,  FALSE, KEY_NOP, 13, 1,  3},
    {207, 80, 3,  FALSE, KEY_NOP, 14, 2,  4},
    {251, 80, 4,  FALSE, KEY_NOP, 15, 3,  5},
    {295, 80, 5,  FALSE, KEY_NOP, 16, 4,  6},
    {339, 80, 6,  FALSE, KEY_NOP, 17, 5,  7},
    {383, 80, 7,  FALSE, KEY_NOP, 18, 6,  8},
    {427, 80, 8,  FALSE, KEY_NOP, 19, 7,  9},
    {471, 80, 9,  FALSE, KEY_NOP, 20, 8,  10},
    {515, 80, 10, FALSE, KEY_NOP, 21, 9,  KEY_NOP},
    {0}
};

static const KeyInfo row1[] =
{
    {85,  102, 11, FALSE, 0,  22, KEY_NOP, 12},
    {129, 102, 12, FALSE, 1,  23, 11, 13},
    {173, 102, 13, FALSE, 2,  24, 12, 14},
    {217, 102, 14, FALSE, 3,  25, 13, 15},
    {261, 102, 15, FALSE, 4,  26, 14, 16},
    {305, 102, 16, FALSE, 5,  27, 15, 17},
    {349, 102, 17, FALSE, 6,  28, 16, 18},
    {393, 102, 18, FALSE, 7,  29, 17, 19},
    {437, 102, 19, FALSE, 8,  30, 18, 20},
    {481, 102, 20, FALSE, 9,  31, 19, 21},
    {525, 102, 21, FALSE, 10, 32, 20, KEY_NOP},
    {0,   0}
};

static const KeyInfo row2[] =
{
    {75,  124, 22, FALSE, 11, KEY_SHIFT,     KEY_NOP, 23},
    {119, 124, 23, FALSE, 12, KEY_SHIFT,     22, 24},
    {163, 124, 24, FALSE, 13, KEY_SHIFT,     23, 25},
    {207, 124, 25, FALSE, 14, KEY_SHIFT,     24, 26},
    {251, 124, 26, FALSE, 15, KEY_CLEAR,     25, 27},
    {295, 124, 27, FALSE, 16, KEY_CLEAR,     26, 28},
    {339, 124, 28, FALSE, 17, KEY_CLEAR,     27, 29},
    {383, 124, 29, FALSE, 18, KEY_BACKSPACE, 28, 30},
    {427, 124, 30, FALSE, 19, KEY_BACKSPACE, 29, 31},
    {471, 124, 31, FALSE, 20, KEY_BACKSPACE, 30, 32},
    {515, 124, 32, FALSE, 21, KEY_BACKSPACE, 31, KEY_NOP},
    {0,   0}
};

static const KeyInfo row3[] =
{
    {135, 150, KEY_SHIFT,     TRUE, 23, KEY_OK,     KEY_NOP,   KEY_CLEAR},
    {259, 150, KEY_CLEAR,     TRUE, 27, KEY_OK,     KEY_SHIFT, KEY_BACKSPACE},
    {383, 150, KEY_BACKSPACE, TRUE, 31, KEY_CANCEL, KEY_CLEAR, KEY_NOP},
    {0,   0}
};

static const KeyInfo row4[] =
{
    {135, 190, KEY_OK,     TRUE, KEY_SHIFT,     KEY_NOP, KEY_NOP, KEY_CANCEL},
    {383, 190, KEY_CANCEL, TRUE, KEY_BACKSPACE, KEY_NOP, KEY_OK,  KEY_NOP},
    {0,   0}
};

static const KeyInfo * const rows[] =
{
    row0,
    row1,
    row2,
    row3,
    row4,
    NULL
};


/*****************************************************************************/


typedef struct
{
    AnimNode kbd;
    char     kbd_String[80];
    uint32   kbd_CharSet;
    int16    kbd_DepressedKey;
} Keyboard;


/*****************************************************************************/


static void DrawKeyboard(StorageReq *req, Keyboard *kbd, GState *gs);

static const AnimMethods kbdMethods =
{
    (UnprepFunc)UnprepAnimNode,
    (StepFunc)StepAnimNode,
    (DrawFunc)DrawKeyboard,
    (DisableFunc)DisableAnimNode,
    (EnableFunc)EnableAnimNode
};


/*****************************************************************************/


static void DrawKeyboard(StorageReq *req, Keyboard *kbd, GState *gs)
{
char           buffer[40];
PenInfo        pi;
Point2         corner;
uint32         row;
uint32         column;
SpriteObj     *spr;
StringExtent   se;
const KeyInfo *key;

    pi.pen_FgColor  = TEXT_COLOR_KEYBOARD;
    pi.pen_BgColor  = 0;
    pi.pen_XScale   = 1.0;
    pi.pen_YScale   = 1.0;
    pi.pen_Flags    = 0;
    pi.pen_reserved = 0;

    /* draw the string being entered */
    pi.pen_X = 100;
    pi.pen_Y = 50 + req->sr_SampleChar.cd_Ascent;
    DrawString(gs, req->sr_Font, &pi, kbd->kbd_String, strlen(kbd->kbd_String));

    row = 0;
    while (rows[row])
    {
        column = 0;
        while (rows[row][column].ki_X)
        {
            key = &rows[row][column];

            corner.x = key->ki_X;
            corner.y = key->ki_Y;

            if (key->ki_Large)
            {
                if (key->ki_Key == kbd->kbd_DepressedKey)
                    spr = req->sr_TextEntryBigButtonImages[1];
                else
                    spr = req->sr_TextEntryBigButtonImages[0];
            }
            else
            {
                if (key->ki_Key == kbd->kbd_DepressedKey)
                    spr = req->sr_TextEntrySmallButtonImages[1];
                else
                    spr = req->sr_TextEntrySmallButtonImages[0];
            }

            switch (key->ki_Key)
            {
                case KEY_SHIFT    : sprintf(buffer, "%s", MSG_TEXT_SHIFT); break;
                case KEY_CLEAR    : sprintf(buffer, "%s", MSG_TEXT_CLEAR); break;
                case KEY_BACKSPACE: sprintf(buffer, "%s", MSG_TEXT_BACKSPACE); break;
                case KEY_OK       : sprintf(buffer, "%s", MSG_OK); break;
                case KEY_CANCEL   : sprintf(buffer, "%s", MSG_CANCEL); break;
                default           : sprintf(buffer, "%c", charSets[kbd->kbd_CharSet][key->ki_Key]); break;
            }

            Spr_SetPosition(spr, &corner);
            F2_Draw(gs, spr);

            GetStringExtent(&se, req->sr_Font, &pi, buffer, strlen(buffer));

            pi.pen_X = corner.x + (Spr_GetWidth(spr) - TEXTWIDTH(&se)) / 2;
            pi.pen_Y = corner.y + (Spr_GetHeight(spr) - TEXTHEIGHT(&se)) / 2 + TO_BASELINE(&se);
            DrawString(gs, req->sr_Font, &pi, buffer, strlen(buffer));

            column++;
        }
        row++;
    }
}


/*****************************************************************************/


static KeyInfo *FindKey(int16 keyNum)
{
const KeyInfo *key;
uint32         row;
uint32         column;

    row = 0;
    while (rows[row])
    {
        column = 0;
        while (rows[row][column].ki_X)
        {
            key = &rows[row][column];

            if (key->ki_Key == keyNum)
                return key;

            column++;
        }
        row++;
    }

#ifdef BUILD_PARANOIA
    printf("FindKey is returning NULL for key %d\n", keyNum);
#endif

    return NULL;
}


/*****************************************************************************/


static void GetKeyCorners(StorageReq *req, Keyboard *kbd, int16 keyNum, Corner *corners)
{
const KeyInfo *key;
SpriteObj     *spr;
int16          x, y, w, h;

    key = FindKey(keyNum);

    x = key->ki_X;
    y = key->ki_Y;

    if (key->ki_Large)
    {
        if (key->ki_Key == kbd->kbd_DepressedKey)
            spr = req->sr_TextEntryBigButtonImages[1];
        else
            spr = req->sr_TextEntryBigButtonImages[0];
    }
    else
    {
        if (key->ki_Key == kbd->kbd_DepressedKey)
            spr = req->sr_TextEntrySmallButtonImages[1];
        else
            spr = req->sr_TextEntrySmallButtonImages[0];
    }

    w = Spr_GetWidth(spr);
    h = Spr_GetHeight(spr);

    corners[0].cr_X = x;
    corners[0].cr_Y = y - 1;

    corners[1].cr_X = x + w - 1;
    corners[1].cr_Y = y - 1;

    corners[2].cr_X = x + w - 1;
    corners[2].cr_Y = y + h;

    corners[3].cr_X = x;
    corners[3].cr_Y = y + h;
}


/*****************************************************************************/


Err TextLoop(StorageReq *req, char *string, uint32 maxChars)
{
bool       exit;
Item       msg;
Event      event;
Corner     corners[NUM_HIGHLIGHT_CORNERS];
int32      sigs;
AnimList   animList;
Background bg;
Highlight  highlight;
Keyboard   kbd;
Err        result;
int16      keyNum;
int16      newKeyNum;
KeyInfo   *key;

    PrepAnimList(req, &animList);

    PrepHighlight(req, &highlight);
    InsertAnimNode(&animList, (AnimNode *)&highlight);

    PrepBackground(req, &bg, req->sr_TextEntryBgSlices, 0);
    InsertAnimNode(&animList, (AnimNode *)&bg);

    PrepAnimNode(req, (AnimNode *)&kbd, &kbdMethods, sizeof(Keyboard));
    sprintf(kbd.kbd_String, "%s", string);
    kbd.kbd_CharSet      = 0;
    kbd.kbd_DepressedKey = KEY_NOP;
    InsertAnimNode(&animList, (AnimNode *)&kbd);

    keyNum = KEY_OK;
    GetKeyCorners(req, &kbd, keyNum, corners);
    MoveHighlightNow(req, &highlight, corners);
    DisableHighlightNow(req, &highlight);
    EnableHighlight(req, &highlight);

    result = -1;
    exit   = FALSE;
    while (!exit)
    {
        sigs = WaitSignal(req->sr_RenderSig | MSGPORT(req->sr_EventPort)->mp_Signal);
        if (sigs & req->sr_RenderSig)
            DoNextFrame(req, &animList);

        while (!exit)
        {
            msg = GetMsg(req->sr_EventPort);
            if (msg <= 0)
                break;

            event = *(Event *)(MESSAGE(msg)->msg_DataPtr);
            ReplyMsg(msg,0,NULL,0);

            switch (event.ev_Type)
            {
                case EVENT_TYPE_RELATIVE_MOVE:
                {
                    newKeyNum = keyNum;
                    key = FindKey(keyNum);

                    if (event.ev_X < 0)
                    {
                        newKeyNum = key->ki_LeftKey;
                    }
                    else if (event.ev_X > 0)
                    {
                        newKeyNum = key->ki_RightKey;
                    }

                    if (newKeyNum != KEY_NOP)
                        keyNum = newKeyNum;

                    newKeyNum = keyNum;
                    key = FindKey(keyNum);

                    if (event.ev_Y < 0)
                    {
                        newKeyNum = key->ki_UpKey;
                    }
                    else if (event.ev_Y > 0)
                    {
                        newKeyNum = key->ki_DownKey;
                    }

                    if (newKeyNum != KEY_NOP)
                        keyNum = newKeyNum;

                    GetKeyCorners(req, &kbd, keyNum, corners);
                    MoveHighlight(req, &highlight, corners);
                    break;
                }

                case EVENT_TYPE_BUTTON_DOWN:
                {
					if (event.ev_Button == EVENT_BUTTON_STOP)
					{
                  	    GetKeyCorners(req, &kbd, KEY_CANCEL, corners);
                  	    MoveHighlightFast(req, &highlight, corners);
						/* will set exit TRUE at button-up time */
					}
					else if (event.ev_Button == EVENT_BUTTON_SHIFT)
					{
					   kbd.kbd_CharSet++;
					   if (charSets[kbd.kbd_CharSet] == NULL)
						   kbd.kbd_CharSet = 0;
					}
					else if (event.ev_Button == EVENT_BUTTON_SELECT)
                    {
                        switch (keyNum)
                        {
                           case KEY_SHIFT:
                           {
                               kbd.kbd_CharSet++;
                               if (charSets[kbd.kbd_CharSet] == NULL)
                                   kbd.kbd_CharSet = 0;

                               break;
                           }

                           case KEY_CLEAR:
                           {
                               kbd.kbd_String[0] = 0;
                               break;
                           }

                           case KEY_BACKSPACE:
                           {
                           	int32 len;

                               len = strlen(kbd.kbd_String);
                               if (len > 0)
                                   kbd.kbd_String[len-1] = 0;

                               break;
                           }

                           case KEY_OK:
                           {
                               exit   = TRUE;
							   if (strlen(kbd.kbd_String) != 0)
							   {
	                               result = 1;
    	                           strcpy(string, kbd.kbd_String);
							   }
							   else
							   {
							   	   result = 0; /* no chars entered: treat as CANCEL */
							   }
                               break;
                           }

                           case KEY_CANCEL:
                           {
                               exit   = TRUE;
                               result = 0;
                               break;
                           }

                           default:
                           {
                           uint32 len;

                               len = strlen(kbd.kbd_String);
                               if (len < (maxChars-1) && len < (sizeof(kbd.kbd_String) - 1))
                               {
                                   kbd.kbd_String[len] = charSets[kbd.kbd_CharSet][keyNum];
                                   kbd.kbd_String[len + 1] = 0;
                               }
                               break;
                           }
                        }
                        kbd.kbd_DepressedKey = keyNum;
                    }
                    break;
                }

                case EVENT_TYPE_BUTTON_UP:
                {
					if (event.ev_Button == EVENT_BUTTON_STOP)
					{
						while (!IsMoveHighlightDone(req, &highlight))
						{
							WaitSignal(req->sr_RenderSig);
							DoNextFrame(req, &animList);
						}
						exit = TRUE;
						result = 0;
					}
					else if (event.ev_Button == EVENT_BUTTON_SELECT)
					{
                        kbd.kbd_DepressedKey = KEY_NOP;
					}
                    break;
                }

                case EVENT_TYPE_FSCHANGE:
                {
                    req->sr_FSChanged = TRUE;
                    break;
                }
            }
        }
    }

    UnprepAnimList(req, &animList);

    return result;
}
