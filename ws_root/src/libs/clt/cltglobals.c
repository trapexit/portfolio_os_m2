
/******************************************************************************
**
**  @(#) cltglobals.c 96/07/09 1.9
**
**  Command lists used externally
**
******************************************************************************/

#include <graphics/clt/clt.h>

static const uint32 NoTextureList[] =
{
	CLT_WriteRegistersHeader(DCNTL, 1),
	CLT_Bits(DCNTL, SYNC, 1),
	CLT_ClearRegistersHeader(TXTADDRCNTL, 1),
	FV_TXTADDRCNTL_TEXTUREENABLE_MASK,	/* Clear texture lookup enable */
	CLT_ClearRegistersHeader(TXTTABCNTL, 1),
	/* Clear just color out and alpha out mask */
	FV_TXTTABCNTL_COLOROUT_MASK | FV_TXTTABCNTL_ALPHAOUT_MASK,
	CLT_SetRegistersHeader(TXTTABCNTL, 1),
	CLT_Const(TXTTABCNTL,COLOROUT,PRIMCOLOR) | CLT_Const(TXTTABCNTL,ALPHAOUT,PRIMALPHA),
};

const CltSnippet CltNoTextureSnippet = {
	(uint32 *)&NoTextureList[0],
	sizeof(NoTextureList)/sizeof(uint32)
};

static const uint32 EnableTextureList[] =
{
	CLT_SetRegistersHeader(TXTADDRCNTL, 1),
	FV_TXTADDRCNTL_TEXTUREENABLE_MASK,	/* Set texture lookup enable */
};

const CltSnippet CltEnableTextureSnippet = {
	(uint32 *)&EnableTextureList[0],
	sizeof(EnableTextureList)/sizeof(uint32)
};
