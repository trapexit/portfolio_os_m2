/* @(#) textstate.c 96/07/09 1.37 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include "fontfile.h"
#include "fonttable.h"
#include "font_folio.h"
#include <stdio.h>
#include <string.h>

#define DPRT(x) /*printf x*/
#define SHOWERR(s, x) /*{if ((x) < 0) {printf("%s\n", s); PrintfSysErr(x);}}*/

#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

#define LINE_HEIGHT(fd) (fd->fd_charHeight + fd->fd_leading)

static renderTableHeader *BuildRenderTable(FontTextArray *fta, uint32 arrayCount,
                                           FontDescriptor *fd, charInfo *info, uint32 first,
                                           uint32 last, uint32 *realCount, TextState *ts);
int32 CharWidth(char theChar, FontDescriptor *fd);
static void *CalcTexelAddress(char theChar, FontDescriptor *fd, uint32 *offset);
static uint32 CalcTexelOffset(char theChar, FontDescriptor *fd, uint32 firstOffset);
Err CreateTELists(TextState *ts, renderTable *table, uint32 rangeCount,
                  uint32 totalChars, uint32 arrayCount, FontTextArray *fta);
static renderTableHeader *CalculateRectangles(renderTableHeader *rth, FontTextArray *fta,
                                              uint32 arrayCount, uint32 rectCount, FontDescriptor *fd);

/**
|||	AUTODOC -public -class Font -name CreateTextState
|||	Creates a TextState for rendering groups of strings.
|||
|||	  Synopsis
|||
|||	    Err CreateTextState(TextState **ts, Item font,
|||	                        const FontTextArray *fta,
|||	                        uint32 arrayCount);
|||
|||	  Description
|||
|||	    Creates a new TextState. A TextState contains all the
|||	    information needed to render the set of strings in the
|||	    FontTextArray on every frame. The Font Folio assumes that,
|||	    generally, the application programmer will be rendering
|||	    the same strings at the same locations on screen in the
|||	    same colors on every new frame rendered. As such, the
|||	    TextState is created to make optimum use of the Texture
|||	    RAM and the Triangle Engine to render all the strings
|||	    specified.
|||
|||	    In fact, CreateTextState() creates a Triangle Engine List
|||	    that loads the Texture RAM with the textures for the
|||	    fonts, and sets up the vertices to render the strings. The
|||	    DrawText() function simply links this pre-computed
|||	    TEList into the application's GState, thereby reducing the
|||	    CPU requirements to actually render the strings each frame
|||	    to the bare minimum.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to a TextState pointer which will be filled by
|||	        the function with the new TextState.
|||
|||	    font
|||	        A Font Item, the result of OpenFont().
|||
|||	    fta
|||	        A pointer to a FontTextArray, which has been pre-filled
|||	        with the requirements of the strings to render.
|||
|||	    arrayCount
|||	        The number of elements in the FontTextArray.
|||
|||	  Return Value
|||	      The new TextState is stored in *ts, or a negative error
|||	      value is returned.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    DeleteTextState(), DrawText()
|||
**/

Err CreateTextState(TextState **ts, Item font, const FontTextArray *fta, uint32 arrayCount)
{
    return(doCreateTextState(ts, font, fta, arrayCount, TRUE));
}

#define MAX_CHAR 256
Err doCreateTextState(TextState **ts, Item font, const FontTextArray *fta, uint32 arrayCount, bool checkRange)
{
    renderTable *table = NULL;
    renderTableHeader *header;
    renderTableHeader *tmpH;
    charInfo *ci;
    uint32 charCnt[MAX_CHAR];
    uint32 totalChars;
    uint32 differentChars;
    TextState *myts = NULL;
    FontDescriptor *fd;
    const FontTextArray *src;
    Err result;
    uint32 i, j;
    char *string;
    uint32 tableSize = 0;
    uint32 rangeCount = 0;
    int32 first, last, next;
    uint32 charCount;
    uint32 rectCount;
    uint32 tsSize;
    uint32 foo;
    bool useCache = FALSE;

    DPRT(("CreateTextState(0x%lx, 0x%lx, 0x%lx, %ld)\n", ts, font, fta, arrayCount));

        /* Do we have a font item? */
    fd = CheckItem(font, NST_FONT, FONT_FONT_NODE);
    if (fd == NULL)
    {
        return(FONT_ERR_BADITEM);
    }

    memset(charCnt, 0, (MAX_CHAR * sizeof(uint32)));
    totalChars = 0;
    differentChars = 0;
    rectCount = 0;

        /* First loop - count the different characters, total characters,
         * and the number of solid-colour rectangles to render with the
         * background pen colour.
         */
    src = fta;
    for (i = 0; i < arrayCount; i++)
    {
        char letter = '\0';

        /* Sanity check - make sure that the Reserved field is 0. If not, there
         * may be problems with future versions.
         */
        if (src->fta_StructSize != sizeof(FontTextArray))
        {
            result = FONT_ERR_BADVERSION;
            goto cts_error;
        }

        /* Skip over leading \ns */
        string = src->fta_String;
        totalChars += src->fta_NumChars;
        j = 0;
        while (*string == '\n')
        {
            j++;
            string++;
        }
        if (*string == '\0')
        {
            /* String was all \ns! */
            continue;
        }

        if (src->fta_Pen.pen_BgColor != 0)
        {
            rectCount++;
        }

        for (; j < src->fta_NumChars; j++)
        {
            letter = *string++;
            if ((letter == '\n') && (*string != '\n') && (src->fta_Pen.pen_BgColor != 0))
            {
                rectCount++;
            }

            if (charCnt[letter] == 0)
            {
                /* This is the first occurence of this character */
                differentChars++;
            }
            charCnt[letter]++; /* Keep track of the total number of this character */
        }
        if ((letter == '\n') && (src->fta_Pen.pen_BgColor != 0))
        {
            /* Last character was a \n, so we have counted one rectangle too many */
            rectCount--;
        }
        src++; /* On to the next array */
    }
    DPRT(("total characters = %ld, different characters = %ld\n", totalChars, differentChars));

    if (totalChars == 0)
    {
        /* Get the fuck outta here! */
        result = 0;
        goto cts_error;
    }

    /* Now allocate the TextState. This includes an array at the end of
     * pointers to all the vertices to speed up on-the-fly vertex
     * alterations.
     */
    tsSize = (sizeof(TextState) + ((totalChars + rectCount) * sizeof(vertexInfo)));
    myts = (TextState *)AllocMem(tsSize, 0);
    if (myts == NULL)
    {
        return(FONT_ERR_NOMEM);
    }
    myts->ts_Font = fd;
    myts->ts_TEList = NULL;
    myts->ts_TESize = 0;
    myts->ts_TERet = NULL;
    myts->ts_Cookie = myts;
    myts->ts_Size = tsSize;
    myts->ts_Vertex = NULL;
    myts->ts_TopLeft.x = fta[0].fta_Pen.pen_X;
    myts->ts_TopLeft.y = (fta[0].fta_Pen.pen_Y - (gfloat)fd->fd_ascent);
    myts->ts_BottomRight.x = (myts->ts_TopLeft.x + 1.0); /* Seed the max.x value */
    myts->ts_BottomRight.y = (fta[0].fta_Pen.pen_Y + (gfloat)fd->fd_descent);
    myts->ts_Track = FALSE;
    myts->ts_Clip = FALSE;
    myts->ts_bgCount = rectCount;
    myts->ts_colorChangeCnt = 0;
    myts->ts_dBlend = NULL;
    PrepList(&myts->ts_ClipList);
    myts->ts_NextClip = FirstNode(&myts->ts_ClipList);

    *ts = myts;

    /* Save time and assume worst case scenario for the table memory allocation. */
    tableSize = ((differentChars * sizeof(renderTableHeader)) +
                 (totalChars * sizeof(charInfo)) +
                 (rectCount * sizeof(bgRectInfo)) +
                 ((rectCount == 0) ? 0 : sizeof(renderTableHeader)));
    if (useCache = (tableSize <= CACHE_SIZE))
    {
        LockSemaphore(FontBase->ff_CacheLock, SEM_WAIT);
        table = FontBase->ff_Cache;
        memset(table, 0, tableSize);
    }
    else
    {
        table = (renderTable *)AllocMem(tableSize, (MEMTYPE_FILL | 0));
        if (table == NULL)
        {
            result = FONT_ERR_NOMEM;
            goto cts_error;
        }
    }


        /* We are going to look for the optimum way to load the TRAM, based on the
         * max number of characters that we can fit in TRAM in one go, and how
         * many contiguous characters we will load into the TRAM before wasting time
         * loading characters that will not be rendered.
         */
    header = (renderTableHeader *)table;
    if (rectCount)
    {
        header = CalculateRectangles(header, fta, arrayCount, rectCount, fd);
    }

    first = -1;
    last = MAX_CHAR;
    charCount = 0;
        /* find the first character to render */
    for (i = fd->fd_firstChar; i <= fd->fd_lastChar; i++)
    {
        if (charCnt[i] > 0)
        {
            last = first = i;
            charCount = charCnt[i];
            DPRT(("first character to print is %c (0x%lx)\n", i, i));
            break;
        }
    }
    /* Now build the range of characters */
    for (i = (first + 1); i <= fd->fd_lastChar; i++)
    {
        if (charCnt[i] > 0)
        {
            next = i;
            if (checkRange &&
                (((next - last) > fd->fd_rangeGap) ||
                 ((next - first) >= fd->fd_maxCharsLoad)))
            {
                /* Gone too far. The range first to last is the next range to load */
                DPRT(("Set the header of the range %c - %c (0x%lx - 0x%lx)\n", first, last, first, last));
                DPRT(("There are %ld characters to render in this range\n", charCount));

                header->texel = CalcTexelAddress(first, fd, &header->offset);
                header->height = fd->fd_charHeight;
                header->bpp = fd->fd_bitsPerPixel;
                header->minChar = first;
                header->maxChar = last;
                header->bytesToLoad = (fd->fd_bytesPerRow * (last - first + 1) *
                                       fd->fd_charHeight);
                header->charCnt = charCount;

                    /* Point the renderTable address for this character to the
                     * start of the charInfo.
                     */
                foo = (uint32)header;
                foo += sizeof(renderTableHeader);
                ci = (charInfo *)foo; /* This is what we are really looking at */
                /* Build the list of characters in this range */
                tmpH = BuildRenderTable(fta, arrayCount, fd, ci, first, last, &foo, myts);
                header->charCnt = foo;
                header = tmpH;

                    /* Start the next range */
                first = last = i;
                charCount = charCnt[i];
                rangeCount++;
            }
            else
            {
                last = next;
                charCount += charCnt[i];
            }
        }
    }

    /* Now add the last range to the list */
    DPRT(("Set the header of the last range %c - %c (%ld - %ld)\n", first, last, first, last));
    DPRT(("There are %ld characters to render in this range\n", charCount));    header->texel = CalcTexelAddress(first, fd, &header->offset);
    header->height = fd->fd_charHeight;
    header->bpp = fd->fd_bitsPerPixel;
    header->minChar = first;
    header->maxChar = last;
    header->bytesToLoad = (fd->fd_bytesPerRow * (last - first + 1) *
                           fd->fd_charHeight);
    header->charCnt = charCount;

    foo = (uint32)header;
    foo += sizeof(renderTableHeader);
    ci = (charInfo *)foo; /* This is what we are really looking at */
        /* There's one more to render */
    BuildRenderTable(fta, arrayCount, fd, ci, first, last, &foo, myts);
    header->charCnt = foo;
    rangeCount++;

    /* Now build the actual TE lists */
    result = CreateTELists(myts, table, rangeCount, totalChars, arrayCount, fta);
    if (result < 0)
    {
        goto cts_error;
    }

    /* Complete the TextState's bounding box */
    myts->ts_TopRight.x = myts->ts_BottomRight.x;
    myts->ts_TopRight.y = myts->ts_TopLeft.y;
    myts->ts_BottomLeft.x = myts->ts_TopLeft.x;
    myts->ts_BottomLeft.y = myts->ts_BottomRight.y;
    myts->ts_BaselineLeft.x = myts->ts_TopLeft.x;
    myts->ts_BaselineLeft.y = (myts->ts_TopLeft.y + fd->fd_ascent);
    myts->ts_BaselineRight.x = myts->ts_TopRight.x;
    myts->ts_BaselineRight.y = myts->ts_BaselineLeft.y;
    myts->ts_BaselineOrigLen.x = (myts->ts_TopRight.x - myts->ts_TopLeft.x);
    myts->ts_BaselineOrigLen.y = (myts->ts_BaselineLeft.y - myts->ts_TopLeft.y);

    if (useCache)
    {
        UnlockSemaphore(FontBase->ff_CacheLock);
    }
    else
    {
        FreeMem(table, tableSize);
    }
    DPRT(("CreateTextState() returning 0x%lx\n", result));
    return(result);

  cts_error:
    DPRT(("CreateTextState() error = 0x%lx\n", result));
    if (useCache)
    {
        UnlockSemaphore(FontBase->ff_CacheLock);
    }
    else
    {
        FreeMem(table, tableSize);
    }
    FreeMem(myts, myts->ts_Size);
    *ts = NULL;
    return(result);
}


static renderTableHeader *BuildRenderTable(FontTextArray *fta, uint32 arrayCount,
                                           FontDescriptor *fd, charInfo *info, uint32 first, uint32 last,
                                           uint32 *realCount, TextState *ts)
{
    int32 i, j;
    FontTextArray *src;
    char *string;
    gfloat xPos, yPos, yBot;
    gfloat w;
    uint32 charWidth;
    uint32 charCnt = 0;
    gfloat sx, sy;

    DPRT(("BuildRenderTable(0x%lx, %c (%ld) - %c (%ld)\n", info, first, first, last, last));

        /* For every character in the arrays, calculate its position */
    src = fta;
    for (i = 0; i < arrayCount; i++)
    {
        string = src->fta_String;
        xPos = src->fta_Pen.pen_X;
        yPos = src->fta_Pen.pen_Y;
        sx = src->fta_Pen.pen_XScale;
        sy = src->fta_Pen.pen_YScale;
        w = ((src->fta_Pen.pen_Flags & FLAG_PI_W_VALID) ? src->fta_Pen.pen_FgW : DEFAULT_FGW);

        /* If this string is wholly outside the clip region, then discard it
         * now.
         */
        yBot = (yPos + (fd->fd_descent * sy));
        if ((yBot > src->fta_Clip.max.y) || /* Off bottom */
            ((yPos - (fd->fd_ascent * sy)) < src->fta_Clip.min.y) ||  /* Off top */
            (src->fta_Pen.pen_X > src->fta_Clip.max.x)                /* Off right side */
            ) /* Cannot check if off left side */
        {
            DPRT(("Clipped whole string\n"));
            src++;
            continue;
        }

        if (yBot < ts->ts_TopLeft.y)
        {
            ts->ts_TopLeft.y = yBot;
        }
        if (yBot > ts->ts_BottomRight.y)
        {
            ts->ts_BottomRight.y = yBot;
        }

        for (j = 0; j < src->fta_NumChars; j++)
        {
            char letter = *string++;

            if (letter == '\n')
            {
                yPos += ((fd->fd_charHeight + fd->fd_leading) * sy);
                xPos = src->fta_Pen.pen_X;

                    /* After moving down one line, are we out of the clip
                     * region now??
                     */
                yBot = (yPos + (fd->fd_descent * sy));
                if (yBot > src->fta_Clip.max.y)
                {
                    break;   /* Nothing more in this string */
                }
                if (yBot < ts->ts_TopLeft.y)
                {
                    ts->ts_TopLeft.y = yBot;
                }
                if (yBot > ts->ts_BottomRight.y)
                {
                    ts->ts_BottomRight.y = yBot;
                }
                continue;
            }

            charWidth = CharWidth(letter, fd);

                /* If we are off the RHS of the clip region, then we can skip
             * the rest of this string.
             */
            if ((xPos + (charWidth * sx)) > src->fta_Clip.max.x)
            {
                DPRT(("Clip away the rest of this string\n"));
                break;
            }

            /* Is this character within the range we are looking for? */
            if ((letter >= first) && (letter <= last))
            {
                if (xPos >= src->fta_Clip.min.x)
                {
                    info->X = xPos;
                    info->Y = (yPos - (fd->fd_ascent * sy));
                    info->W = w;
                    info->BgColor = src->fta_Pen.pen_BgColor;
                    info->FgColor = src->fta_Pen.pen_FgColor;
                    info->offset = CalcTexelOffset(letter, fd, first);
                    info->width = (charWidth * sx);
                    info->height = (fd->fd_charHeight * sy);
                    info->twidth = charWidth;
                    info->entry = letter;
                    charCnt++;
                    DPRT(("%c at (%g, %g)\n", letter, info->X, info->Y));

                    if ((xPos + info->width) < ts->ts_TopLeft.x)
                    {
                        ts->ts_TopLeft.x = (xPos + info->width);
                    }
                    if ((xPos + info->width) > ts->ts_BottomRight.x)
                    {
                        ts->ts_BottomRight.x = (xPos + info->width);
                    }
                    /* Keep track of where we will store the next char */
                    info++;
                }
            }
                /* Calculate the screen position of the next character. */
            xPos += ((charWidth + fd->fd_charExtra) * sx);
        }
        src++; /* On to the next array */
    }

    DPRT(("Found %ld characters in this range\n", charCnt));
    *realCount = charCnt;
    return((renderTableHeader *)info);
}

static void ClipRectangle(bgRectInfo *ri, FontTextArray *fta);
static renderTableHeader *CalculateRectangles(renderTableHeader *rth, FontTextArray *fta,
                                              uint32 arrayCount, uint32 rectCount, FontDescriptor *fd)
{
    bgRectInfo *ri;
    uint32 foo;
    uint32 i, j;
    gfloat xPos, yPos;
    gfloat w;
    char *string;
    char letter = 0;
    gfloat sx, sy;

    foo = (uint32)rth;
    foo += sizeof(renderTableHeader);
    ri = (bgRectInfo *)foo;
    for (i = 0; i < arrayCount; i++)
    {
        if (fta->fta_Pen.pen_BgColor != 0)
        {
            xPos = fta->fta_Pen.pen_X;
            yPos = fta->fta_Pen.pen_Y;
            sx = fta->fta_Pen.pen_XScale;
            sy = fta->fta_Pen.pen_YScale;
            w = ((fta->fta_Pen.pen_Flags & FLAG_PI_W_VALID) ? fta->fta_Pen.pen_BgW : DEFAULT_BGW);
            string = fta->fta_String;
            rth->texel = NULL; /* shows there is a bgRectInfo, not a charInfo */
            rth->charCnt = rectCount;

            /* Skip over leading \ns */
            j = 0;
            while (*string == '\n')
            {
                string++;
                j++;
                yPos += (LINE_HEIGHT(fd) * sy);
            }

            for (; j < fta->fta_NumChars; j++)
            {
                letter = *string++;
                if (letter == '\n')
                {
                    if (*(string - 2) != '\n')  /* Don`t make rectangles for contiguous \n */
                    {
                            /* Fill out this bgRectInfo */
                        ri->box.min.x = fta->fta_Pen.pen_X;
                        ri->box.min.y = (yPos - (fd->fd_ascent * sy));
                        ri->box.max.x = (xPos - fd->fd_charExtra);
                        ri->box.max.y = (yPos + (fd->fd_descent * sy));
                        ri->rgb = fta->fta_Pen.pen_BgColor;
                        ri->w = w;
                        ClipRectangle(ri, fta);
                        ri++;
                    }
                    /* Move down one line */
                    yPos += (LINE_HEIGHT(fd) * sy);
                    xPos = fta->fta_Pen.pen_X;
                }
                else if ((letter >= fd->fd_firstChar) && (letter <= fd->fd_lastChar))
                {
                    xPos += ((CharWidth(letter, fd) + fd->fd_charExtra) * sx);
                }
            }
            if (letter != '\n')
            {
                /* ie the last character of the string is not \n */
                ri->box.min.x = fta->fta_Pen.pen_X;
                ri->box.min.y = (yPos - (fd->fd_ascent * sy));
                ri->box.max.x = (xPos - fd->fd_charExtra);
                ri->box.max.y = (yPos + (fd->fd_descent * sy));
                ri->rgb = fta->fta_Pen.pen_BgColor;
                ri->w = w;
                ClipRectangle(ri, fta);
                ri++;
            }
        }
        fta++;
    }

    return((renderTableHeader *)ri);
}

static void ClipRectangle(bgRectInfo *ri, FontTextArray *fta)
{
        /* Clip the rectangle to the string's clip box */
    if ((ri->box.max.x < fta->fta_Clip.min.x) ||
        (ri->box.max.y < fta->fta_Clip.min.y) ||
        (ri->box.min.x > fta->fta_Clip.max.x) ||
        (ri->box.min.y > fta->fta_Clip.max.y))
    {
        ri->clipped = TRUE;
    }
    else
    {
        if (ri->box.min.x < fta->fta_Clip.min.x)
        {
            ri->box.min.x = fta->fta_Clip.min.x;
        }
        if (ri->box.min.y < fta->fta_Clip.min.y)
        {
            /* Partially off the top = wholly clipped */
            ri->clipped = TRUE;
        }
        if (ri->box.max.x > fta->fta_Clip.max.x)
        {
            ri->box.max.x = fta->fta_Clip.max.x;
        }
        if (ri->box.max.y > fta->fta_Clip.max.y)
        {
            /* Partially off the bottom = wholly clipped */
            ri->clipped = TRUE;
        }
    }
}

int32 GetFontCharInfo(const FontDescriptor *fd, int32 theChar, void **blitInfo);
int32 CharWidth(char theChar, FontDescriptor *fd)
{
    return(GetFontCharInfo(fd, theChar, NULL));
}

#define AddToPtr(ptr, val) ((void*)((((char *)(ptr)) + (long)(val))))
static void *CalcTexelAddress(char theChar, FontDescriptor *fd, uint32 *offset)
{
    FontCharInfo *fci;

    GetFontCharInfo(fd, theChar, &fci);
    if (offset)
    {
        *offset = fci->fci_charOffset;
    }
    return((void *)AddToPtr(fd->fd_charData, fci->fci_charOffset));
}

static uint32 CalcTexelOffset(char theChar, FontDescriptor *fd, uint32 firstOffset)
{
    uint32 pixelOffset;

    pixelOffset = ((theChar - firstOffset) * fd->fd_charHeight);
    return(pixelOffset);
}

/**
|||	AUTODOC -public -class Font -name DeleteTextState
|||	Deletes a TextState.
|||
|||	  Synopsis
|||
|||	    Err DeleteTextState(const TextState *ts);
|||
|||	  Description
|||
|||	    Deletes the TextState created with CreateTextState(), and
|||	    deletes all the resources associated with this
|||	    TextState. You should not call DeleteTextState() until
|||	    the last use of the TextState has completed (when
|||	    GS_WaitIO() returns).
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState to delete.
|||
|||	  Return Value
|||	    Negative if error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextState()
|||
**/

Err DeleteTextState(const TextState *ts)
{
    ClippedVertices *cv, *next;

    CheckTextState(ts);
    if (ts == NULL)
    {
        return(FONT_ERR_BADTS);
    }

    cv = (ClippedVertices *)FirstNode(&ts->ts_ClipList);
    while (IsNode(&ts->ts_ClipList, (Node *)cv))
    {
        next = (ClippedVertices *)NextNode((Node *)cv);
        FreeMem(cv, sizeof(ClippedVertices));
        cv = next;
    }

    FreeMem(ts->ts_TEList, ts->ts_TESize);
    FreeMem(ts, ts->ts_Size);
    return 0;
}

/**
|||	AUTODOC -public -class Font -name DrawString
|||	Immediately draws a single string to the bitmap.
|||
|||	  Synopsis
|||
|||	    Err DrawString(GState *gs, Item font, PenInfo *pen,
|||	                   const char *string, uint32 numChars);
|||
|||	  Description
|||
|||	    Will immediately render the string into the GState and
|||	    bitmap, and will call GS_SendList() and GS_WaitIO(). This is
|||	    not the most efficient way to render text (see
|||	    CreateTextState() for that), but is the most convenient
|||	    for quick-and-dirty string rendering where you are not
|||	    trying to optimise the Triangle Engine or CPU usage.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState.
|||
|||	    font
|||	        Font Item to render the text with.
|||
|||	    pen
|||	        Pointer to a PenInfo, containing the coordinates and the
|||	        colors to render the string.
|||
|||	    string
|||	        String to render.
|||
|||	    numChars
|||	        Number of characters in the string to render.
|||
|||	  Return Value
|||	    Negative value returned if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Caveats
|||	    This call will return with the pen->pen_X and pen->pen_Y
|||	    values modified with the coordinate of the character
|||	    immediately following this string.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    DrawTextStencil()
|||
**/

static bgRectInfo *CalcStringBgRect(char *string, Item fontI, PenInfo *pen, bgRectInfo *ri);
#define TESIZE 4096

Err DrawString(GState *gs, Item fontI, PenInfo *pen, const char *string, uint32 numChars)
{
    FontDescriptor *fd;
    renderTable *rt, *next;
    bgRectInfo *ri;
    char letter;
    TextState myts;
    Err err;
    int32 i;
    uint32 height;
    uint32 bpp;
    uint32 bytesToLoad;
    uint32 charWidth;
    uint32 realChars;
    uint32 tableSize;
    uint32 rectCount = 0;
    gfloat xPos;
    bool useCache;
    uint32 tesize;

    DPRT(("DrawString(0x%lx, 0x%lx, %s, %ld)\n", gs, fontI, string, numChars));

        /* Do we have a font item? */
    fd = CheckItem(fontI, NST_FONT, FONT_FONT_NODE);
    if (fd == NULL)
    {
        return(FONT_ERR_BADITEM);
    }

    if (pen->pen_BgColor)
    {
        char *stringt = string;

        i = 0;
            /* Skip over leading \n's */
        while (*stringt == '\n')
        {
            i++;
            stringt++;
        }
        letter = *(stringt - 1); /* If the string consisted only of \ns, this will
                                  * invoke the rectCount-- code below.
                                  */

            /* How many bgRectInfos do we need? */
        rectCount = 1;
        for (; i < numChars; i++)
        {
            letter = *stringt++;
            if ((letter == '\n') && (*stringt != '\n'))
            {
                rectCount++;
            }
        }
        if (letter == '\n')
        {
            /* Last character was \n, so we have counted 1 too many */
            rectCount--;
        }
    }

        /* The TextState we create will never be reused.
         * In this case, the rendering time will be a small fraction of the
         * time taken to build the TELists, so let's speed up the TEList generation
         * rather than worry about optimising the TELists.
         */
    myts.ts_Font = fd;
    myts.ts_Cookie = &myts;
    myts.ts_Size = 0;   /* CreateTEList() won't store pointers to the vertices */
    tableSize = ((sizeof(renderTable) * numChars) +
                 (pen->pen_BgColor ? (sizeof(renderTableHeader) + (sizeof(bgRectInfo) * rectCount)) : 0));
    if (useCache = (tableSize <= CACHE_SIZE))
    {
        LockSemaphore(FontBase->ff_CacheLock, SEM_WAIT);
        rt = FontBase->ff_Cache;
        myts.ts_TEList = (void *)((uint32)rt + tableSize);
        tesize = myts.ts_TESize = (CACHE_SIZE - tableSize);
    }
    else
    {
        rt = (renderTable *)AllocMem(tableSize + TESIZE, 0);
        if (rt == NULL)
        {
            return(FONT_ERR_NOMEM);
        }
        myts.ts_TEList = (void *)((uint32)rt + tableSize);
        tesize = myts.ts_TESize = TESIZE;
    }

    next = rt;
    bpp = fd->fd_bitsPerPixel;
    height = fd->fd_charHeight;

    /* If there is a background colour, calculate the bgRectInfos now */
    if (rectCount)
    {
        gfloat yPos;
        char *_string = string;

        yPos = pen->pen_Y;
        ri = (bgRectInfo *)&next->toRender;
        next->nextChar.texel = NULL;
        next->nextChar.charCnt = rectCount;

        for (i = 0; i < rectCount; i++)
        {
            /* Skip over leading \ns */
            while (*_string == '\n')
            {
                _string++;
                pen->pen_Y += (LINE_HEIGHT(fd) * pen->pen_YScale);
            }

            ri = CalcStringBgRect(_string, fontI, pen, ri);
            /* Increment the Y pen for every consecutive \n character */
            do
            {
                pen->pen_Y += (LINE_HEIGHT(fd) * pen->pen_YScale);
                _string = strchr(_string, '\n');
            }
            while (_string && (*++_string == '\n'));
        }

        next = (renderTable *)ri;
        pen->pen_Y = yPos;
    }

        /* Assume all chars are stored in the same number of bytes */
    bytesToLoad = (fd->fd_bytesPerRow * height);
    xPos = pen->pen_X;
    realChars = 0;
    for (i = 0; i < numChars; i++)
    {
        letter = *string++;
        /* Is this letter in the font? */
        if ((letter >= fd->fd_firstChar) && (letter <= fd->fd_lastChar))
        {
            next->nextChar.texel = CalcTexelAddress(letter, fd, &next->nextChar.offset);
            next->nextChar.charCnt = 1;
            next->nextChar.height = height;
            next->nextChar.bpp = bpp;
            next->nextChar.bytesToLoad = bytesToLoad;
            next->nextChar.minChar = next->nextChar.maxChar = letter;

            charWidth = CharWidth(letter, fd);
            next->toRender.X = xPos;
            next->toRender.Y = (pen->pen_Y - (fd->fd_ascent * pen->pen_YScale));
            next->toRender.W = DEFAULT_FGW;
            next->toRender.BgColor = pen->pen_BgColor;
            next->toRender.FgColor = pen->pen_FgColor;
            next->toRender.offset = 0;
            next->toRender.width = (charWidth * pen->pen_XScale);
            next->toRender.height = (height * pen->pen_YScale);
            next->toRender.twidth = charWidth;
            next->toRender.entry = letter;

            next++;
            xPos += ((charWidth + fd->fd_charExtra) * pen->pen_XScale);
            realChars++;
        }
        else if (letter == '\n')
        {
            /* Move down one line */
            xPos = pen->pen_X;
            pen->pen_Y += (LINE_HEIGHT(fd) * pen->pen_YScale);
        }
    }
    if (realChars == 0)
    {
        return(0);
    }

    DPRT(("Creating TELists with %ld characters\n", realChars));
    err = CreateTELists(&myts, rt, realChars, realChars, 1, NULL);

    if (err >= 0)
    {
        err = DrawText(gs, &myts);
        GS_SendList(gs);
        GS_WaitIO(gs);
        /* Put the pen's XPos at the end of this string */
        pen->pen_X = xPos;
        if (myts.ts_TESize != tesize)
        {
            /* CreateTEList() allocated a larger buffer */
            FreeMem(myts.ts_TEList, myts.ts_TESize);
        }
    }
    else
    {
        DPRT(("CreateTEList returned 0x%lx\n", err));
    }

    if (useCache)
    {
        UnlockSemaphore(FontBase->ff_CacheLock);
    }
    else
    {
        FreeMem(rt, tableSize + TESIZE);
    }

    return(err);
}

static bgRectInfo *CalcStringBgRect(char *string, Item fontI, PenInfo *pen, bgRectInfo *ri)
{
    StringExtent se;
    uint32 len;
    char *newline;
    uint32 i;

    /* Calculate the background rect as far as the first \n */
    newline = strchr(string, '\n');
    len = strlen(string);
    if (newline)
    {
        len = ((uint32)newline - (uint32)string);
    }
    GetStringExtent(&se, fontI, pen, string, len);
    ri->box.min = se.se_TopLeft;
    ri->box.max = se.se_BottomRight;
    ri->w = DEFAULT_BGW;
    ri->rgb = pen->pen_BgColor;
    ri->clipped = FALSE;
    i = (uint32)ri;
    i += sizeof(bgRectInfo);
    ri = (bgRectInfo *)i;

    return(ri);
}

/**
|||	AUTODOC -public -class Font -name GetCharacterData
|||	Gets data about a single character
|||
|||	  Synopsis
|||
|||	    Err GetCharacterData(CharacterData *cd, Item fontI,
|||	                         char character);
|||
|||	  Description
|||
|||	    Fills out a CharacterData structure for a single character
|||	    in the font, providing enough information to render
|||	    the character yourself with the Triangle Engine or CPU, or
|||	    use it in a sprite in the 2D Framework library.
|||
|||	  Arguments
|||
|||	    cd
|||	        Pointer to the CharacterData to fill out.
|||
|||	    font
|||	        Font Item for the text.
|||
|||	    character
|||	        Character to get the information about.
|||
|||	  Return Value
|||
|||	    Negative value returned if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
**/

Err GetCharacterData(CharacterData *cd, Item fontI, char character)
{
    FontDescriptor *fd;

        /* Do we have a font item? */
    fd = CheckItem(fontI, NST_FONT, FONT_FONT_NODE);
    if (fd == NULL)
    {
        return(FONT_ERR_BADITEM);
    }

    /* Is the character in the font? */
    if ((character < fd->fd_firstChar) ||
        (character > fd->fd_lastChar))
    {
        return(FONT_ERR_BADCHAR);
    }

    cd->cd_Texel = CalcTexelAddress(character, fd, NULL);
    cd->cd_CharHeight = fd->fd_charHeight;
    cd->cd_CharWidth = CharWidth(character, fd);
    cd->cd_BitsPerPixel = fd->fd_bitsPerPixel;
    cd->cd_Ascent = fd->fd_ascent;
    cd->cd_Descent = fd->fd_descent;
    cd->cd_Leading = fd->fd_leading;
    cd->cd_BytesPerRow = fd->fd_bytesPerRow;

    return(0);
}

/**
|||	AUTODOC -public -class Font -name TrackTextBounds
|||	Enables or disables the tracking of a TextState's bounding box.
|||
|||	  Synopsis
|||
|||	    Err TrackTextBounds(TextState *ts, bool track);
|||
|||	  Description
|||
|||	    If you will ever need to get the TextExtent of the
|||	    TextState with GetTextExtent(), then you must enable the
|||	    bounds tracking. By default, bounds tracking is disabled
|||	    when the TextState is created. Keeping track of the text's
|||	    extent will slow MoveText(), ScaleText() and RotateText()
|||	    by about 5%.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState.
|||
|||	    track
|||	        TRUE to enable bounds tracking.
|||
|||	  Return Value
|||
|||	    Negative value returned if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
**/

Err TrackTextBounds(TextState *ts, bool track)
{
    CheckTextState(ts);

    ts->ts_Track = track;
    return(0);
}
