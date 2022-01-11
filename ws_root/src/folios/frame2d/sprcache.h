/******************************************************************************
**
**  @(#) sprcache.h 96/03/01 1.2
**
**  Definitions for sprite TEList caching optimisations.
**  PRIVATE!
**
******************************************************************************/

#include <kernel/types.h>

typedef struct
{
    uint32 tlc_TxLoadSize; /* # of instructions in each TxLoad */
    uint32 *tlc_TxLoad;  /* Pointer to Array of TxLoad instructions */
} TxLoadCache;

typedef struct
{
  /* MinNode spc_Clone; */
    MinNode spc_Node;
    GState *spc_GState;
  /* uint32 spc_LastCount; /*  /* Last GState->gs_Count */
    uint32 spc_ByteCount;  /* # of bytes in memory block */
    uint32 spc_Flags;
    uint32 spc_VertexWords; /* # of words actually used */
  /* uint32 *spc_Return; */    /* Stick the return address here */
    uint32 *spc_Vertices;
    TxLoadCache *spc_TxLoad;
} SpriteCache;

#define SPC_FLAG_VALID 1

#if 0
typedef struct
{
    uint32 vtc_Slices;     /* # of slices. 0 if no slicing */
    uint32 vtc_VtxCnt;     /* # of vertices */
    uint32 vtc_Style;      /* Style of vertex instructions */
    uint32 *vtc_Vertices;  /* Pointer to the vertices */
    uint32 *vtc_TxLoad;    /* Pointer to the TxLoad instructions
                            * if y slicing.
                            */
    uint32 vtc_TxLoadSize; /* Size of the TxLoad list */
} VertexCache;

typedef struct
{
    MinNode spc_Node;
    GState *spc_GState;
  /* uint32 spc_LastCount; */  /* Last GState->gs_Count */
    uint32 *spc_TEBuffer;  /* Points to the first instruction allocated */
    uint32 spc_TEBufferSize;
    uint32 *spc_TxLoad;    /* Pointer to the TxLoad instructions */
    uint32 spc_TxLoadSize; /* Size of the TxLoad list */
    VertexCache spc_VCache[1]; /* First VertexCache in an array */
} SpriteCache;
#endif

#if 0
typedef struct
{
    MinNode spc_Node;
    GState *spc_GState;
  /* uint32 spc_LastCount; */  /* Last GState->gs_Count */
    uint32 *spc_TEBuffer;  /* Points to the first instruction allocated */
    uint32 spc_TEBufferSize;
    uint32 *spc_PIP;       /* Pointer to the PIP instructions */
    uint32 *spc_TxBlend;   /* Pointer to the TxBlend instructions */
    uint32 *spc_DBlend;    /* Pointer to the DBlend instructions */
    uint32 *spc_TxLoad;    /* Pointer to the TxLoad instructions */
    uint32 spc_PIPSize;
    uint32 spc_TxBSize;
    uint32 spc_DBSize;
    uint32 spc_TxLoadSize; /* Size of the TxLoad list */
    VertexCache spc_VCache[1]; /* First VertexCache in an array */
} SpriteCache;
#endif    
    
    
    
    
