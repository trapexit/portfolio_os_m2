#ifndef __GRAPHICS_FONT_H
#define __GRAPHICS_FONT_H


/******************************************************************************
**
**  @(#) font.h 95/12/20 1.15
**
**  Font folio interface definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __GRAPHICS_CLT_GSTATE_H
#include <graphics/clt/gstate.h>
#endif

#ifndef __GRAPHICS_FRAME2D_SPRITEOBJ_H
#include <graphics/frame2d/spriteobj.h>
#endif

#ifndef _GPBOX
#include <graphics/pipe/box.h>
#endif

#ifndef __STDARG_H
#include <stdarg.h>
#endif


/****************************************************************************/


/* kernel interface definitions */
#define FONT_FOLIONAME  "font"


/****************************************************************************/


/* Font folio item types */
#define FONT_FONT_NODE  1


/*****************************************************************************/


/* Error codes */

#define MakeFontErr(svr,class,err) MakeErr(ER_FOLI,ER_FONT,svr,ER_E_SSTM,class,err)

/* Unknown tag supplied */
#define FONT_ERR_BADTAG       MakeFontErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* No memory */
#define FONT_ERR_NOMEM        MakeFontErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* Object not found */
#define FONT_ERR_NOTFOUND     MakeFontErr(ER_SEVERE,ER_C_STND,ER_NotFound)

/* Bad item number supplied */
#define FONT_ERR_BADITEM      MakeFontErr(ER_SEVERE,ER_C_STND,ER_BadItem)

/* Bad subsystem type for item */
#define FONT_ERR_BADSUBTYPE   MakeFontErr(ER_SEVERE,ER_C_STND,ER_BadSubType)

/* Bad privileged for operation */
#define FONT_ERR_BADPRIV      MakeFontErr(ER_SEVERE,ER_C_STND,ER_NotPrivileged)

/* Bad name for item */
#define FONT_ERR_BADNAME      MakeFontErr(ER_SEVERE,ER_C_STND,ER_BadName)

/* The named file wasn't a valid font file */
#define FONT_ERR_BADFONTFILE  MakeFontErr(ER_SEVERE,ER_C_NSTND,1)

/* The font file isn't of a supported version */
#define FONT_ERR_BADVERSION   MakeFontErr(ER_SEVERE,ER_C_NSTND,2)

/* The font has an unsupported bits-per-pixel value */
#define FONT_ERR_BADBPP       MakeFontErr(ER_SEVERE,ER_C_NSTND,3)

/* Bad TextState */
#define FONT_ERR_BADTS MakeFontErr(ER_SEVERE,ER_C_NSTND,4)

/* Character is out of range */
#define FONT_ERR_BADCHAR MakeFontErr(ER_SEVERE,ER_C_NSTND,5)

/* Range of characters is bad for a stencil */
#define FONT_ERR_BADSTENCILRANGE MakeFontErr(ER_SEVERE,ER_C_NSTND,6)

/*****************************************************************************/
/* TextStencilInfo - used to create a TextStencil */
typedef struct TextStencilInfo
{
    Item tsi_Font;                /* The font to use */
    uint32 tsi_MinChar;       /* min character in the range */
    uint32 tsi_MaxChar;      /* max character in the range */
    uint32 tsi_NumChars;    /* max number of characters in the range */
    uint32 tsi_FgColor;       /* Foreground color */
    uint32 tsi_BgColor;       /* Background color */
    uint32 tsi_reserved;      /* Set to 0 */
} TextStencilInfo;

#ifndef EXTERNAL_RELEASE

#include <kernel/item.h>

typedef struct FontDescriptor
{
    OpeningItemNode fd;
    uint32          fd_OpenCount;
    uint32          fd_fontFlags;     /* Flags describing the font             */
    uint32          fd_charHeight;    /* Height of character (ascent+descent)  */
    uint32          fd_charWidth;     /* Max width of character (pixels)       */
    uint32          fd_bitsPerPixel;  /* Pixel depth of each character, as stored in file */
    uint32          fd_firstChar;     /* First char defined in character set   */
    uint32          fd_lastChar;      /* Last char defined in character set    */
    uint32          fd_charExtra;     /* Spacing between characters            */
    uint32          fd_ascent;        /* Distance from baseline to ascentline  */
    uint32          fd_descent;       /* Distance from baseline to descentline */
    uint32          fd_leading;       /* Distance from descent line to next ascent line */
    uint32          fd_bytesPerRow;   /* Bytes per character data row          */
    void           *fd_fontHeader;    /* Font header information               */
    void           *fd_charInfo;      /* Per-character data table              */
    void           *fd_charData;      /* The character data                    */
    uint32          fd_maxCharsLoad;  /* Maximum characters to load into TRAM  */
    uint32          fd_rangeGap;      /* Max contiguous unprintable characters */
} FontDescriptor;

/* FontDescriptor.fd_fontFlags */
#define FFLAG_MONOSPACED FONT_FLAG_MONOSPACED
#define FFLAG_ITALIC     FONT_FLAG_ITALIC
#define FFLAG_BOLD       FONT_FLAG_BOLD
#define FFLAG_OUTLINED   FONT_FLAG_OUTLINED
#define FFLAG_SHADOWED   FONT_FLAG_SHADOWED
#define FFLAG_UNDERLINED FONT_FLAG_UNDERLINED

/* FontDescriptor.fd_loadTexRange */
#define FFTR_ALL 0      /* Whole font can be loaded into TRAM */
#define FFTR_SPLIT 1    /* Load Upper case, lower case, then remainder */
#define FFTR_CHAR 2     /* Load each character individually */

#define TS_POINTS 6
typedef struct TextState
{
    FontDescriptor *ts_Font;
    void *ts_TEList;
    uint32 ts_TESize;
    uint32 *ts_TERet;
    void *ts_Cookie;
    uint32 ts_Size;
    Point2 ts_TopLeft;
    Point2 ts_TopRight;
    Point2 ts_BottomLeft;
    Point2 ts_BottomRight;
    Point2 ts_BaselineLeft; /* This is the position of the TextState */
    Point2 ts_BaselineRight;
    Point2 ts_BaselineOrigLen;  /* Store this to calculate scale factor */
    bool ts_Track;
    bool ts_Clip;
    /* These next four define the clipbox of this TextState */
    gfloat ts_Leftmost;
    gfloat ts_Rightmost;
    gfloat ts_Topmost;
    gfloat ts_Bottommost;
    /* And these four define the clip window */
    gfloat ts_LeftEdge;
    gfloat ts_RightEdge;
    gfloat ts_TopEdge;
    gfloat ts_BottomEdge;
    
    uint32 ts_bgCount;        /* Number of background rectangles */
    uint32 ts_colorChangeCnt; /* Number of colour changes */
    List ts_ClipList;         /* List of pointers to clipping vertices */
    Node *ts_NextClip;
    
    uint32 *ts_dBlend;    /*  Pointer to the DBlend instructions */
    uint32 *ts_Vertex;     /* This must be last */
} TextState;

typedef struct TextStencil
{
    TextStencilInfo info;
    TextState *textState;
} TextStencil;

#else /* EXTERNAL_RELEASE */
typedef struct TextState TextState;
typedef struct TextStencil TextStencil;
#endif /* EXTERNAL_RELEASE */

/****************************************************************************/

/* PenInfo - used in DrawString() */
typedef struct PenInfo
{
    gfloat pen_X;
    gfloat pen_Y;
    uint32 pen_FgColor;
    uint32 pen_BgColor;
    gfloat pen_XScale;
    gfloat pen_YScale;
    uint32 pen_Flags;
    gfloat pen_FgW;
    gfloat pen_BgW;
    uint32 pen_reserved;   /* Must be set to 0 */
} PenInfo;
    /* Set this in pen_flags if pen_fgW and pen_bgW are valid */
#define FLAG_PI_W_VALID 1

/* FontTextArray - used to render multiple strings in the same font */
typedef struct FontTextArray
{
    uint32 fta_StructSize;  /* set to the size of the structure */
    PenInfo fta_Pen;
    Box2 fta_Clip;
    char *fta_String;
    uint32 fta_NumChars;    /* Number of characters to render */
} FontTextArray;

typedef struct StringExtent
{
    Point2 se_TopLeft;
    Point2 se_TopRight;
    Point2 se_BottomLeft;
    Point2 se_BottomRight;
    Point2 se_BaselineLeft;
    Point2 se_BaselineRight;
    gfloat se_Angle;
    uint32 se_Leading;       /* Same as the font's leading value */
} StringExtent;
/* These macros work if se_angle == 0 */
#define TEXTHEIGHT(t) (((StringExtent *)(t))->se_BottomLeft.y - \
                       ((StringExtent *)(t))->se_TopLeft.y + \
                       ((StringExtent *)(t))->se_Leading + 1)
#define TO_BASELINE(t) (((StringExtent *)(t))->se_BaselineLeft.y - \
                        ((StringExtent *)(t))->se_TopLeft.y + 1)
#define TEXTWIDTH(t) (((StringExtent *)(t))->se_TopRight.x - \
                      ((StringExtent *)(t))->se_TopLeft.x + 1)
/* Use these instead if the text has been scaled */
#define SCALED_TEXTHEIGHT(t, sy) (TEXTHEIGHT(t) * (sy))
#define SCALED_TO_BASELINE(t, sy) (TO_BASELINE(t) * (sy))
#define SCALED_TEXTWIDTH (t, sx) (TEXTWIDTH(t) * (sx))
typedef StringExtent TextExtent;

typedef struct CharacterData
{
    void *cd_Texel;          /* Actual texture data */
    uint32 cd_CharHeight;    /* Height of character (ascent+descent)  */
    uint32 cd_CharWidth;     /* Width of character (pixels)       */
    uint32 cd_BitsPerPixel;  /* Pixel depth of each character, as stored in file */
    uint32 cd_Ascent;        /* Distance from baseline to ascentline  */
    uint32 cd_Descent;       /* Distance from baseline to descentline */
    uint32 cd_Leading;       /* Distance from descent line to next ascent line */
    uint32 cd_BytesPerRow;   /* Bytes per character data row          */
} CharacterData;

typedef gfloat FontMatrix[3][2];

/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/* folio management */
Err OpenFontFolio(void);
Err CloseFontFolio(void);

/* font file API */
#define OpenFont(fontName)  FindAndOpenNamedItem(MKNODEID(NST_FONT,FONT_FONT_NODE),fontName)
#define CloseFont(it)       CloseItem(it)

/* Text State API */
Err CreateTextState(TextState **ts, Item font, const FontTextArray *fta, uint32 arrayCount);
Err DeleteTextState(const TextState *ts);

/* TextStencil API */
Err CreateTextStencil(TextStencil **ts, const TextStencilInfo *tsi);
Err DrawTextStencil(GState *gs, TextStencil *ts, PenInfo *pen, char *string, uint32 numChars);
TextState *GetTextStateFromStencil(TextStencil *ts);
Err DeleteTextStencil(TextStencil *ts);

/* Text altering APIs */
Err MoveText(TextState *ts, gfloat dx, gfloat dy);
Err GetTextPosition(const TextState *ts, Point2 *pt);
Err ScaleText(TextState *ts, gfloat sx, gfloat sy);
Err GetTextScale(const TextState *ts, gfloat *sx, gfloat *sy);
Err RotateText(TextState *ts, const gfloat angle, const gfloat x, const gfloat y);
Err GetTextAngle(const TextState *ts, gfloat *angle);
Err TrackTextBounds(TextState *ts, bool track);
Err SetTextColor(TextState *ts, uint32 fg, uint32 bg);
Err SetClipBox(TextState *ts, bool set, Point2 *topLeft, Point2 *bottomRight);
Err TransformText(TextState *ts, FontMatrix fm);

/* Text Rendering API */
Err DrawText(GState *gs, TextState *ts);
Err DrawString(GState *gs, Item font, PenInfo *pen, const char *string, uint32 numChars);

Err GetStringExtent(StringExtent *se, Item font, const PenInfo *pen, const char *string, uint32 numChars);
Err GetTextExtent(const TextState *ts, TextExtent *te);
Err GetCharacterData(CharacterData *cd, Item font, char character);

#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#endif  /* __GRAPHICS_FONT_H */
