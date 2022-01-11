/* @(#) req.h 96/10/30 1.5 */

#ifndef __REQ_H
#define __REQ_H


/*****************************************************************************/


#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif

#ifndef __GRAPHICS_FRAME2D_SPRITEOBJ_H
#include <graphics/frame2d/spriteobj.h>
#endif

#ifndef __GRAPHICS_FONT_H
#include <graphics/font.h>
#endif

#ifndef __UI_REQUESTER_H
#include <ui/requester.h>
#endif

#ifndef __EVENTMGR_H
#include "eventmgr.h"
#endif

#ifndef __ANIMLISTS_H
#include "animlists.h"
#endif

#ifndef __HIGHLIGHT_H
#include "highlight.h"
#endif

#ifndef __BG_H
#include "bg.h"
#endif

#ifndef __LISTVIEWS_H
#include "listviews.h"
#endif

#ifndef __CONTROLS_H
#include "controls.h"
#endif

#ifndef __MOVINGTEXT_H
#include "movingtext.h"
#endif

#ifndef __BOXES_H
#include "boxes.h"
#endif

#ifndef __IOSERVER_H
#include "ioserver.h"
#endif

#ifndef __DIRSCAN_H
#include "dirscan.h"
#endif

#ifndef __HIERARCHY_H
#include "hierarchy.h"
#endif

#ifndef __MSGSTRINGS_H
#include "msgstrings.h"
#endif


/****************************************************************************/


#define TEXT_COLOR_VIEWLIST_NORMAL			0x00B0B0B0
#define TEXT_COLOR_VIEWLIST_COPIED			0x00B0B030

#define TEXT_COLOR_HIERARCHY_NORMAL			0x00B0B0B0

#define TEXT_COLOR_CONTROL_LABEL_NORMAL		0x00B0B0B0
#define TEXT_COLOR_CONTROL_LABEL_DISABLED	0x00805010
#define TEXT_COLOR_CONTROL_BUTTON			0x00D0D0D0

#define TEXT_COLOR_OVERLAY_TITLE			0x00702020
#define TEXT_COLOR_OVERLAY_TEXT				0x00202060

#define TEXT_COLOR_INFO_TEMPLATE			0x00702020
#define TEXT_COLOR_INFO_DATA				0x00202060

#define TEXT_COLOR_KEYBOARD					0x00C0C0C0


/****************************************************************************/


#define NUM_BG_SLICES                    (10)
#define NUM_BOX_IMAGES                   (11)
#define NUM_SUITCASE_IMAGES              (7)

#define NUM_OK_IMAGES                    (7)
#define NUM_LOAD_IMAGES                  (7)
#define NUM_SAVE_IMAGES                  (7)
#define NUM_CANCEL_IMAGES                (7)
#define NUM_COPY_IMAGES                  (7)
#define NUM_MOVE_IMAGES                  (8)
#define NUM_DELETE_IMAGES                (7)
#define NUM_CREATEDIR_IMAGES             (7)
#define NUM_RENAME_IMAGES                (8)
#define NUM_BUTTON_IMAGES                (2)
#define NUM_LOCK_IMAGES                  (7)
#define NUM_QUESTION_IMAGES              (7)
#define NUM_FOLDER_IMAGES                (7)
#define NUM_WORKING_IMAGES               (6)
#define NUM_HANDINSERT_IMAGES            (7)
#define NUM_STOP_IMAGES                  (6)

#define NUM_TEXTENTRY_BG_SLICES          (10)
#define NUM_TEXTENTRY_BIGBUTTON_IMAGES   (2)
#define NUM_TEXTENTRY_SMALLBUTTON_IMAGES (2)


typedef enum
{
    SOUND_UNAVAILABLE,
    SOUND_ERROR,
    SOUND_QUESTION,
    SOUND_OPENSUITCASE,
    SOUND_CLOSESUITCASE,
    SOUND_DELETED,
    SOUND_TOSUITCASE,
    SOUND_FROMSUITCASE,
    SOUND_SHOWOVERLAY,
    SOUND_HIDEOVERLAY
} Sounds;



struct StorageReq
{
    Item                sr_ViewList;
    Item                sr_Font;
    Item                sr_Locale;
    CharacterData       sr_SampleChar;
    uint32              sr_Options;
    bool                sr_CloseFont;
    bool                sr_CloseLocale;

    char                sr_FileBuffer[FILESYSTEM_MAX_NAME_LEN+1];
    char                sr_DirectoryBuffer[FILESYSTEM_MAX_PATH_LEN+1];
    FileFilterFunc      sr_FilterFunc;

    EventMgr           *sr_EventMgr;
    Item                sr_EventPort;

    Item                sr_View;
    GState             *sr_GState;
    Item                sr_FrameBuffers[2];
    uint8               sr_CurrentFrameBuffer;
    int32               sr_RenderSig;

    char               *sr_MsgStrings[NUM_STRINGS];
    void               *sr_StringBlock;

    List                sr_Sprites;
    SpriteObj          *sr_CurrentSprite;

    AnimList            sr_AnimList;
    Highlight           sr_Highlight;
    Background          sr_Bg;
    ListView            sr_ListView;
    Control             sr_Suitcase;
    Boxes               sr_Boxes;
    MovingText          sr_DeletedText;
    MovingText          sr_CopiedText;

    TimeVal             sr_IOStartTime;
    DirScanner         *sr_DirScanner;
    List               *sr_DirEntries;
    Hierarchy           sr_Hierarchy;

    bool                sr_DeletedTextAnimation;
    bool                sr_CopiedTextAnimation;

    SpriteObj          *sr_Prompt;
    SpriteObj          *sr_BarImage;
    SpriteObj          *sr_BgSlices[NUM_BG_SLICES];
    SpriteObj          *sr_BoxImages[NUM_BOX_IMAGES];
    SpriteObj          *sr_SuitcaseImages[NUM_SUITCASE_IMAGES];

    SpriteObj          *sr_OKImages[NUM_OK_IMAGES];
    SpriteObj          *sr_LoadImages[NUM_LOAD_IMAGES];
    SpriteObj          *sr_SaveImages[NUM_SAVE_IMAGES];
    SpriteObj          *sr_CancelImages[NUM_CANCEL_IMAGES];
    SpriteObj          *sr_CreateDirImages[NUM_CREATEDIR_IMAGES];
    SpriteObj          *sr_RenameImages[NUM_RENAME_IMAGES];
    SpriteObj          *sr_CopyImages[NUM_COPY_IMAGES];
    SpriteObj          *sr_MoveImages[NUM_MOVE_IMAGES];
    SpriteObj          *sr_DeleteImages[NUM_DELETE_IMAGES];
    SpriteObj          *sr_ButtonImages[NUM_BUTTON_IMAGES];
    SpriteObj          *sr_LockImages[NUM_LOCK_IMAGES];
    SpriteObj          *sr_QuestionImages[NUM_QUESTION_IMAGES];
    SpriteObj          *sr_FolderImages[NUM_FOLDER_IMAGES];
    SpriteObj          *sr_WorkingImages[NUM_WORKING_IMAGES];
    SpriteObj          *sr_HandInsertImages[NUM_HANDINSERT_IMAGES];
    SpriteObj          *sr_StopImages[NUM_STOP_IMAGES];

    SpriteObj          *sr_TextEntryBgSlices[NUM_TEXTENTRY_BG_SLICES];
    SpriteObj          *sr_TextEntryBigButtonImages[NUM_TEXTENTRY_BIGBUTTON_IMAGES];
    SpriteObj          *sr_TextEntrySmallButtonImages[NUM_TEXTENTRY_SMALLBUTTON_IMAGES];

    SpriteObj          *sr_OverlayImage;
    bool                sr_OverlayDisplayed;

	bool				sr_MovePending;
	bool				sr_CopyPending;
    bool                sr_FSChanged;

    IOServer           *sr_IOServer;
    Item                sr_IOStatusPort;

    TimeVal             sr_CurrentTime;

    void               *sr_Cookie;
};


/*****************************************************************************/


#endif /* __REQ_H */
