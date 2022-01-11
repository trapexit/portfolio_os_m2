/* @(#) fonttable.h 95/09/04 1.2 */

#ifndef __FONTTABLE_H
#define __FONTTABLE_H

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

/* Build this into the table of rendered characters */
#define DEFAULT_BGW 0.999900
#define DEFAULT_FGW 0.999998
typedef struct charInfo
{
    gfloat X;           /* Pen position */
    gfloat Y;
    gfloat W;
    uint32 BgColor;     /* Background Color */
    uint32 FgColor;     /* Foreground Color */
    uint32 offset;      /* Offset from first texel in the TRAM */
    gfloat width;       /* width of the character */
    gfloat height;      /* height of the character */
    gfloat twidth;      /* Actual width of the texture */
    char entry;
} charInfo;

typedef struct bgRectInfo
{
    Box2 box;
    gfloat w;
    uint32 rgb;
    bool clipped;
} bgRectInfo;

typedef struct renderTableHeader
{
    void *texel;
    uint32 charCnt;
    uint32 height;
    uint32 bpp;
    uint32 bytesToLoad;
    uint32 offset;
    char minChar;
    char maxChar;
} renderTableHeader;

typedef struct renderTable
{
    renderTableHeader nextChar;
    charInfo toRender;    /* First entry in the array */
} renderTable;

/* These structs are stored in an array at the end
 * of the TextState.
 */
typedef struct vertexInfo
{
    uint32 *vertex;    /* Pointer to the first vertex */
    gfloat dx;         /* Distance from LHS of first element
                        * in the FontTextArray.
                        */
    gfloat dy;         /* Distance from baseline of first
                        * element in the FontTextArray.
                        */
    uint32 saved[3]; /* Save the instructions here when clipping */
    uint32 flags;
} vertexInfo;

#define SAVED_BGND 1
#define SAVED_CHAR 2

/* CLIPPED_BYTES is the number of bytes needed for worst case
 * clipped character. This is 8 vertices, 7 words per vertex for the
 * background case, 1 instruction, and 3 instructions for the return
 * jump. 4 bytes per word.
 */
#define CLIPPED_BYTES (1 + (7 * 8) + 3)
typedef struct ClippedVertices
{
    MinNode node;
    uint32 *jumpBack;
    uint32 instructions[CLIPPED_BYTES];
} ClippedVertices;

typedef struct _geouv
{
  gfloat x,y,u,v,w,r,g,b,a;
} _geouv;

bool ClipBackground(TextState *ts, vertexInfo *vi, uint32 *v, uint32 *nextv);
bool ClipCharacter(TextState *ts, vertexInfo *vi, uint32 *v, uint32 *nextv);
void RestoreBackground(vertexInfo *vi, uint32 *v);
#define RestoreCharacter RestoreBackground
bool DoClip(TextState *ts, _geouv *geo, ClippedVertices *cv, uint32 type);
void CalcClipBox(TextState *ts, Point2 *pt);
Err ParseVertices(TextState *ts, void (*callback)(gfloat *, void *), void *params);
Err doCreateTextState(TextState **ts, Item font, const FontTextArray *fta, uint32 arrayCount, bool checkRange);

#endif
