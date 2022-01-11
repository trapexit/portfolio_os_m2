/*
 *	@(#) f2clt.c 96/04/20 1.9
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * Auxiliary CLT routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * f2clt.c Stephen H. Landrum, 1995
 */


/*
 * This routine sets up for display of textureless primitives
 */


static const uint32 _prllist[] = {
  CLT_SetRegistersHeader(DBUSERCONTROL,1),
  CLA_DBUSERCONTROL (0, 0, 0, 0, 0, 0, 0, DBL_RGBDestOut|DBL_AlphaDestOut),

  CLT_WriteRegistersHeader(DBAMULTCNTL,1),
  CLA_DBAMULTCNTL (DBL_ASelectTexColor, DBL_MASelectTexAlpha,
		   DBL_MAConstByTexSSB, 0),

  CLT_WriteRegistersHeader(DBBMULTCNTL,1),
  CLA_DBBMULTCNTL (DBL_BSelectSrcColor, DBL_MBSelectTexAlphaComplement,
		   DBL_MBConstByTexSSB, 0),

  CLT_WriteRegistersHeader(DBDESTALPHACNTL,1),
  CLA_DBDESTALPHACNTL(CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,TEXALPHA)),

  CLT_WriteRegistersHeader(DBALUCNTL,1),
  CLA_DBALUCNTL(DBL_AddClamp, 0),
};

static const uint32 _alphaen[] = {
  CLT_SetRegistersHeader(DBUSERCONTROL,1),
  CLA_DBUSERCONTROL (0, 0, 0, 0, 1, 1, 0, 0),
};


static const uint32 _alphadis[] = {
  CLT_ClearRegistersHeader(DBUSERCONTROL,1),
  CLA_DBUSERCONTROL (0, 0, 0, 0, 1, 1, 0, 0),
};



static const CltSnippet _prlsnippet = {
  &_prllist[0],
  sizeof(_prllist)/sizeof(uint32),
};

static const CltSnippet _alphaensnippet = {
  &_alphaen[0],
  sizeof(_alphaen)/sizeof(uint32),
};

static const CltSnippet _alphadissnippet = {
  &_alphadis[0],
  sizeof(_alphadis)/sizeof(uint32),
};



void
_set_notexture (GState *gs)
{
  COPYRESERVE (gs, &CltNoTextureSnippet);  /* Contains Sync instruction */

  COPYRESERVE (gs, &_prlsnippet);
}


void
_enable_alpha (GState *gs)
{
  COPYRESERVE (gs, &_alphaensnippet);
}

void
_disable_alpha (GState *gs)
{
  COPYRESERVE (gs, &_alphadissnippet);
}



static const uint32 _passthrulist[] = {
  CLT_SetRegistersHeader(DBUSERCONTROL,1),
  CLA_DBUSERCONTROL (0, 0, 0, 0, 1, 1, 0, DBL_RGBDestOut|DBL_AlphaDestOut),

  CLT_WriteRegistersHeader(DBBMULTCNTL,1),
  CLA_DBBMULTCNTL(DBL_BSelectSrcColor,DBL_MBSelectConst,DBL_MBConstBySrcSSB,0),

  CLT_WriteRegistersHeader(DBBMULTCONSTSSB0, 2),
  CLA_DBBMULTCONSTSSB0(0xff, 0xff, 0xff),
  CLA_DBBMULTCONSTSSB1(0xff, 0xff, 0xff),

  CLT_WriteRegistersHeader(DBCONSTIN, 1),
  CLA_DBCONSTIN(0xff, 0xff, 0xff),

  CLT_WriteRegistersHeader(DBDESTALPHACNTL,1),
  CLA_DBDESTALPHACNTL(DBL_DestAlphaSelectSrcAlpha),

  CLT_ClearRegistersHeader(DBDISCARDCONTROL,1),
  CLA_DBDISCARDCONTROL(0, 1, 1, 1),

  CLT_WriteRegistersHeader(DBALUCNTL,1),
  CLA_DBALUCNTL(DBL_B, 0),
};


static const CltSnippet _passthrusnippet = {
  &_passthrulist[0],
  sizeof(_passthrulist)/sizeof(uint32),
};



void
_set_srcpassthrough (GState *gs)
{
  COPYRESERVE (gs, &CltNoTextureSnippet);	/* Contains Sync instruction */

  COPYRESERVE (gs, &_passthrusnippet);
}


