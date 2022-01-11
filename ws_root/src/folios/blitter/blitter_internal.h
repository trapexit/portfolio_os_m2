/* @(#) blitter_internal.h 96/10/18 1.12
 *
 * Internal header file for blitter folio.
 */

/* Private housekeeping stuff pointed to by
 * bo_blitStuff.
 */
typedef struct BlitStuff
{
    uint32 flags;
    gfloat *slicedVertices;
    uint32 slicedVerticesSize;
    uint32 slicedBytesUsed;
    uint32 ySlicesUsed;     /* Number of y slices actually used */
    uint32 totalStrips;        /* Number of triangle strips in the slicedVertices */
    /* Clip stuff */
    Point2 tl;
    Point2 br;
    uint32 *clippedVertices;    /* Execute these vertices when clipped */
    uint32 clippedVerticesSize;
    uint32 clippedBytesUsed;
        /* Use this for copying the rectangle source into the TxData buffer. */
    Item bmI;
} BlitStuff;
#define BLITSTUFF(b) ((BlitStuff *)((b)->bo_blitStuff))
#define VERTICES_SLICED 0x1   /* The vertices have been sliced */
#define CLIP_ENABLED 0x2          /* Clipping is enabled */
#define CLIP_CALCULATED 0x4       /* Vertices have been through the clip code */
#define VERTICES_CLIPPED 0x8      /* Vertices are clipped */
#define CLIP_OUT 0x10              /* Set if CLIPOUT used */
/* Use this to invalidate the clipped vertices */
#define INVALIDATE_CLIP (CLIP_CALCULATED | VERTICES_CLIPPED)

/* Use these flags in the snippet flags field */
#define SYSTEM_SNIPPET 0x1   /* Created by the system */
#define CBO_SNIPPET 0x2         /* Created inside Blt_CreateBlitObject() */
#define SYSTEM_TEXEL 0x4      /* texelData AllocMem()ed by the system */
#define DIMENSIONS_VALID 0x8      /* BltTxData flag:
                                   *BlitStuff width height and bpp are valid
                                   */
#define PERFECT_RECT 0x10       /* Set if the vertices are a perfect rectangle. */

typedef struct BoundingBox
{
    Point2 topLeft;
    Point2 topRight;
    Point2 bottomLeft;
    Point2 bottomRight;
    gfloat leftMost;
    gfloat rightMost;
    gfloat topMost;
    gfloat bottomMost;
    uint32 *instructions;       /* instructions this bbox is relevant to */
} BoundingBox;
    
typedef struct TextureSlice
{
    uint32 ySlice;              /* Number of y slices the texture is split into */
    uint32 ySliceHeight;        /* Height of each y slice */ 
    uint32 ySliceLastHeight;  /* Remainder for the last slice */
    uint32 bitsPerRow;       /* Width of each slice in bits */
    TxLoadSnippet *txlOrig; /* Pointer to the original TxLoadSnippet used */
    BoundingBox *bbox;          /* Pointer to an array of BoundingBoxes */
        /* This next entry is not really for sliced textures,
         * but this is a good place to hide this information.
         * We shadow this value if the system allocated the
         * texelData, because the CltTxLod copy may be changed
         * by Blt_SetTexture().
         */
    void *texelData;
} TextureSlice;
#define TEXTURESLICE(t) ((TextureSlice *)((t)->btd_slice))

    
/* Each VerticesSnippet has a pointer to an array of
 * BlitObjects that use the VerticesSnippet, used to know
 * which vertices to modify in MoveVertices() and RotateVertices().
 */
#define BO_ARRAY_SIZE 16
typedef struct BOArray
{
    BlitObject *bo[BO_ARRAY_SIZE];
    struct BOArray *next;
} BOArray;
#define BOARRAY(v) ((BOArray *)((v)->vtx_private))

#define VTX_FLOATS 9
