
/******************************************************************************
**
**  @(#) texload.h 96/02/22 1.4
**
******************************************************************************/

#include <graphics/clt/clt.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/clt/gstate.h>

/** texel format **/
#define COLOR_DEPTH_MASK        0x000F
#define ALPHA_DEPTH_MASK        0x00F0
#define IS_TRANS_MASK           0x0100
#define HAS_SSB_MASK            0x0200
#define HAS_COLOR_MASK          0x0400
#define HAS_ALPHA_MASK          0x0800
#define IS_LITERAL_MASK         0x1000

#define txGetColorDepth(x)  (int)((x) & COLOR_DEPTH_MASK)
#define txGetAlphaDepth(x)  (int)(((x) & ALPHA_DEPTH_MASK) >> 4)

#define txHasSSB(x)         (((x) & HAS_SSB_MASK) >> 9)
#define txHasColor(x)       (((x) & HAS_COLOR_MASK) >> 10)
#define txHasAlpha(x)       (((x) & HAS_ALPHA_MASK) >> 11)
#define txIsLiteral(x)      (((x) & IS_LITERAL_MASK) >> 12)

#define ID_FORM      	'FORM'
#define ID_TXTR 		'TXTR'
#define ID_M2TX 		'M2TX'
#define ID_M2PI 		'M2PI'
#define ID_M2CI 		'M2CI'
#define ID_M2TD 		'M2TD'
#define ID_M2TA 		'M2TA'
#define ID_M2LR 		'M2LR'
#define ID_M2DB 		'M2DB'

#define GET_FIELD(ptr, v, t) { v = *((t *)ptr); ptr += sizeof(t); }

typedef struct TextureSnippets {
	CltSnippet					dab;
	CltSnippet					tab;
	CltTxLoadControlBlock*		lcb;
	CltPipControlBlock*			pip;
	CltTxData*					txdata;
} TextureSnippets;

typedef struct tag_M2Rect
{
  int16 XWrapMode;       /* Whether to repeat or clamp the texture */
  int16 YWrapMode;       /* Whether to repeat or clamp the texture */
  int16 FirstLOD;        /* The finest LOD to clip from */
  int16 NLOD;            /* The number of LODs to clip from */
  int16 XOff;            /* The clippping X Offset of the coarset LOD */
  int16 YOff;            /* The clippping Y Offset of the coarset LOD */
  int16 XSize;           /* The X Size of the clip rect on the coarsest LOD */
  int16 YSize;           /* The Y Size of the clip rect on the coarsest LOD */
} M2TXRect;

typedef struct {
	char *buff;
	char *hdr_data;
	char *pip_data;
	char *tex_data;
	char *tab_data;
	char *dab_data;

	char *dci_data;
	char *ldr_data;
	uint32 buffsize;
	uint32 hdr_size;
	uint32 pip_size;
	uint32 tex_size;
	uint32 tab_size;
	uint32 dab_size;
	uint32 dci_size;
	uint32 ldr_size;
} tx_chunk_data;

Err LoadTexture(TextureSnippets* snips, char* filename);
void UseTxLoadCmdLists( GState* gs, TextureSnippets* snips );
