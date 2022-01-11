/*
 *	@(#) draw.c 96/05/07 1.46
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D Rendering routines
 */

#include "frame2.i"

#define DBUG(x) /* printf x */
#define DBUGX(x) /* printf x */
#define DBUGZ(x) /* printf x */
#define DBUGV(x) /*printf x*/
#define DBUGA(x) /* printf x */

void BuildVertices(SpriteObj *sp, SpriteCache *spc, GState *gs,
                   _geouv *geo, bool dontClip, uint32 style)
{
  /* uint32 *ret; */
    
    if (sp->spr_Flags & SPR_CALC_VERTICES) {
            /* Need to recalculate the instructions */
        DBUGV(("clip_fan2d with spc = 0x%lx, flags = 0x%lx\n", spc, spc->spc_Flags));
        _clip_fan2d (gs, spc, style, 4, geo, dontClip);
    }
    DBUGV(("spc now 0x%lx, flags 0x%lx. vertices = 0x%lx\n", spc, spc->spc_Flags, spc->spc_Vertices));      
    if (spc->spc_Vertices && (spc->spc_Flags & SPC_FLAG_VALID)) {
	GS_Reserve(gs, spc->spc_VertexWords);
	memcpy (*GS_Ptr(gs), spc->spc_Vertices, spc->spc_VertexWords*4);
	*GS_Ptr(gs) += spc->spc_VertexWords;
    } /* else, the instructions were built into the GState or this segment is clipped */
    return;
}

SpriteCache *CreateSpriteCache(SpriteCache *spc, GState *gs)
{
    SpriteCache *new;
    DBUGA (("Alloc: CreateSpriteCache\n"));
    if (new = (SpriteCache *)AllocMem(sizeof(SpriteCache), (MEMTYPE_NORMAL | MEMTYPE_FILL | 0))) {
        DBUGV(("New sprite cache allocted at 0x%lx\n", new));      
        if (spc) {
            new->spc_Node.n_Prev = (MinNode *)spc;
            spc->spc_Node.n_Next = (MinNode *)new;
        }
        new->spc_GState = gs;
    }
    return(new);
}
    
TxLoadCache *CreateTxLoadCache(SpriteCache *spc, int32 csize)
{
    TxLoadCache *tlc;
    
    if (spc == NULL) {
        return(NULL);
    }
    if (tlc = spc->spc_TxLoad) {
      if (csize == tlc->tlc_TxLoadSize) {
	/* No need to free and reallocate memory */
	return tlc;
      } else {
        FreeMem(tlc->tlc_TxLoad, ((tlc->tlc_TxLoadSize) * 4));
      }
    } else {
        DBUGA (("Alloc: CreateTxLoadCache - tlc\n"));
        tlc = (TxLoadCache *)AllocMem(sizeof(TxLoadCache), MEMTYPE_NORMAL);
        DBUGV(("New TxLoadCache allocated at 0x%lx\n", tlc));      
        if (tlc == NULL) {
            return(NULL);
        }
        spc->spc_TxLoad = tlc;
    }
    
    tlc->tlc_TxLoadSize = csize;
    DBUGA (("Alloc: CreateTxLoadCache - tlc_TxLoad\n"));
    tlc->tlc_TxLoad = (uint32 *)AllocMem((csize * 4), MEMTYPE_NORMAL);
    if (tlc->tlc_TxLoad == NULL) {
        /* bummer. Free up the TxLoad structure as well. */
        FreeMem(tlc, sizeof(TxLoadCache));
        spc->spc_TxLoad = NULL;
    }
    
    return(tlc);
}


static Err
_draw_sprite_core (GState *gs, SpriteObj *sp, int32 geoflag,
		   gfloat xx0, gfloat xx1, gfloat xx2, gfloat xx3, 
		   gfloat yy0, gfloat yy1, gfloat yy2, gfloat yy3)
{
  CltTxLoadControlBlock	lcb;
  SpriteCache *spc, *prev;
  TxLoadCache *tlc = NULL;
  gfloat	xmax, ymax;
  CltTxData	*txd;
  int32		csize, bpr, ystep;
#ifdef BUILD_BDA1_1
  uint32	mask1, mask2;
#endif
  bool dontClip;
  bool extended=(sp->spr_Node.n_Type==EXTSPRITEOBJNODE);

  DBUG (("draw\n"));

  /* Do some housekeeping. If spc_Cache is NULL, then this is the first time
   * this sprite has been rendered. If it is not NULL, and the GState count is the
   * same as the cache last gstate count, then this sprite is being reused.
   */
  prev = NULL;
  spc = (SpriteCache *)sp->spr_Cache;
  if (spc) {
      if (spc->spc_GState == NULL) {
          spc->spc_GState = gs;
      }
      if (!(sp->spr_Flags&SPR_CALC_TXLOAD)) {  
          tlc = spc->spc_TxLoad;
      }
  }
  else {
      spc = CreateSpriteCache(prev, gs);
      sp->spr_Cache = (void *)spc;
  }
  
  dontClip = ((sp->spr_Flags & SPR_DONT_CLIP) > 0);
  ystep = 0;

  /* Do the rendering setup */
  if (sp->spr_TexBlend) {
    txd = sp->spr_TexBlend->tb_txData;
    DBUG (("Has texblend\n"));
    if (sp->spr_TexBlend->tb_PCB || txd || sp->spr_TexBlend->tb_TAB.data
	|| sp->spr_TexBlend->tb_DAB.data) {
      GS_Reserve (gs, 2);
      CLT_Sync (GS_Ptr(gs));
    }
    if (sp->spr_TexBlend->tb_PCB) {
      DBUG (("Has PIP\n"));
      COPYRESERVE (gs, &sp->spr_TexBlend->tb_PCB->pipCommandList);
    }
    if (sp->spr_TexBlend->tb_txData) {
      DBUG (("Has texture data\n"));
      lcb.textureBlock = (void*)txd;
      lcb.firstLOD = 0;
      lcb.numLOD = txd->maxLOD;
      lcb.XWrap = 0;
      lcb.YWrap = 0;
      lcb.XSize = txd->minX;
      lcb.YSize = txd->minY;
      lcb.XOffset = 0;
      lcb.YOffset = 0;
      lcb.tramOffset = 0;

      bpr = lcb.XSize*txd->bitsPerPixel;
      if (bpr*lcb.YSize > 16384*8) {
	if (geoflag<0) return -1;
	ystep = (16384*8) / bpr;
#ifdef BUILD_BDA1_1
	if (KB_FIELD(kb_Flags) & KB_BDA1_1) {
	  mask1 = 0xffffffff;
	  mask2 = 0x1f;
	  while (bpr&mask2) {
	    mask2 >>= 1;
	    mask1 <<= 1;
	  }
	  ystep = ystep & mask1;
	}
#endif
	if (ystep<1) return -1;	/* ERR cannot slice */
      } else {
          if (tlc == NULL) {
              csize = CLT_ComputeTxLoadCmdListSize (&lcb);
              if (csize<0) return csize;
              if (spc && (tlc = CreateTxLoadCache(spc, csize))) {
		  lcb.lcbCommandList.data = tlc->tlc_TxLoad;
              } else {
                  GS_Reserve (gs, csize);
                  lcb.lcbCommandList.data = *GS_Ptr(gs);
                  *GS_Ptr(gs) += csize;
              }
              lcb.lcbCommandList.size = csize;
              CLT_CreateTxLoadCommandList (&lcb);
          }
          if (tlc) {
	      GS_Reserve(gs, tlc->tlc_TxLoadSize);
	      memcpy (*GS_Ptr(gs), tlc->tlc_TxLoad, tlc->tlc_TxLoadSize*4);
	      *GS_Ptr(gs) += tlc->tlc_TxLoadSize;
          }
      }
    }
    
    if (sp->spr_TexBlend->tb_TAB.data) {
      DBUG (("Has TAB\n"));
      COPYRESERVE (gs, &sp->spr_TexBlend->tb_TAB);
    }
    if (sp->spr_TexBlend->tb_DAB.data) {
      DBUG (("Has DAB\n"));
      COPYRESERVE (gs, &sp->spr_TexBlend->tb_DAB);
    }
  }

  /* Do the geometry */
  if (geoflag) {
    DBUG (("Has geometry\n"));
    if ((sp->spr_Flags & SPR_CALC_VERTICES) && spc) {
            /* Invalidate all the caches */
        SpriteCache *next = spc;
        do {
            next->spc_Flags &= ~SPC_FLAG_VALID;
            next = (SpriteCache *)next->spc_Node.n_Next;
        } while (next);
    }
    
    if ((sp->spr_TexBlend) && (sp->spr_TexBlend->tb_txData)) {
      xmax = sp->spr_TexBlend->tb_txData->minX<<
	(sp->spr_TexBlend->tb_txData->maxLOD-1);
      ymax = sp->spr_TexBlend->tb_txData->minY<<
	(sp->spr_TexBlend->tb_txData->maxLOD-1);
    } else {
      xmax = ymax = 1.;
    }
    if ((ystep==0) &&
        ((geoflag<0) 
         || ((sp->spr_HSlice==1) && (sp->spr_VSlice==1)))) {
      _geouv geo[4];
      uint32 style = GEOF2_TEXCOORDS;
      
      if (sp->spr_Flags & SPR_CALC_VERTICES) {
          memset ((void*)geo,0,sizeof(geo));
          geo[0].x = xx0;
          geo[0].y = yy0;
          geo[0].u = 0.;
          geo[0].v = 0.;
          geo[1].x = xx1;
          geo[1].y = yy1;
          geo[1].u = xmax;
          geo[1].v = 0.;
          geo[2].x = xx2;
          geo[2].y = yy2;
          geo[2].u = xmax;
          geo[2].v = ymax;
          geo[3].x = xx3;
          geo[3].y = yy3;
          geo[3].u = 0.;
          geo[3].v = ymax;
          if (extended) {
              geo[0].w = ((ExtSpriteObj*)sp)->spr_ZValues[0];
              geo[0].r = ((ExtSpriteObj*)sp)->spr_Colors[0].r;
              geo[0].g = ((ExtSpriteObj*)sp)->spr_Colors[0].g;
              geo[0].b = ((ExtSpriteObj*)sp)->spr_Colors[0].b;
              geo[0].a = ((ExtSpriteObj*)sp)->spr_Colors[0].a;
              geo[1].w = ((ExtSpriteObj*)sp)->spr_ZValues[1];
              geo[1].r = ((ExtSpriteObj*)sp)->spr_Colors[1].r;
              geo[1].g = ((ExtSpriteObj*)sp)->spr_Colors[1].g;
              geo[1].b = ((ExtSpriteObj*)sp)->spr_Colors[1].b;
              geo[1].a = ((ExtSpriteObj*)sp)->spr_Colors[1].a;
              geo[2].w = ((ExtSpriteObj*)sp)->spr_ZValues[2];
              geo[2].r = ((ExtSpriteObj*)sp)->spr_Colors[2].r;
              geo[2].g = ((ExtSpriteObj*)sp)->spr_Colors[2].g;
              geo[2].b = ((ExtSpriteObj*)sp)->spr_Colors[2].b;
              geo[2].a = ((ExtSpriteObj*)sp)->spr_Colors[2].a;
              geo[3].w = ((ExtSpriteObj*)sp)->spr_ZValues[3];
              geo[3].r = ((ExtSpriteObj*)sp)->spr_Colors[3].r;
              geo[3].g = ((ExtSpriteObj*)sp)->spr_Colors[3].g;
              geo[3].b = ((ExtSpriteObj*)sp)->spr_Colors[3].b;
              geo[3].a = ((ExtSpriteObj*)sp)->spr_Colors[3].a;
              style = (GEOF2_TEXCOORDS|GEOF2_COLORS);
          } else {
	    geo[0].w = .999998;
	    geo[1].w = .999998;
	    geo[2].w = .999998;
	    geo[3].w = .999998;
	  }
      }
      BuildVertices(sp, spc, gs, geo, dontClip, style);
    } else {
      if (ystep) {
	int32 i, ys;
	gfloat dx03, dx12, dy03, dy12;
	gfloat dr03, dr12, dg03, dg12, db03, db12, da03, da12;
	gfloat dz03, dz12;
	gfloat r0, r1, r2, r3, g0, g1, g2, g3, b0, b1, b2, b3, a0, a1, a2, a3;
	gfloat z0, z1, z2, z3;
	gfloat yss;
	_geouv geo[4];

	dr03 = dr12 = dg03 = dg12 = db03 = db12 = da03 = da12 = 
	dz03 = dz12 = 
	z0 = z1 = z2 = z3 = 
	r0 = r1 = r2 = r3 = b0 = b1 = b2 = b3 = 
	g0 = g1 = g2 = g3 = a0 = a1 = a2 = a3 = 
	b0 = b1 = b2 = b3 = 0;

	  
#ifdef BUILD_BDA1_1
	if (KB_FIELD(kb_Flags) & KB_BDA1_1) {
	  memcpy (&txd2, txd, sizeof(txd2));
	  memcpy (&txl, txd->texelData, sizeof(txl));
	  txl.texelData = txd2.texelData->texelData;
	  txd2.texelData = &txl;
	  lcb.textureBlock = (void*)&txd2;
	}
#endif

	ys = lcb.YSize;
	yss = (gfloat)ystep/(gfloat)ys;

	if (extended) {
	  r0 = ((ExtSpriteObj*)sp)->spr_Colors[0].r;
	  g0 = ((ExtSpriteObj*)sp)->spr_Colors[0].g;
	  b0 = ((ExtSpriteObj*)sp)->spr_Colors[0].b;
	  a0 = ((ExtSpriteObj*)sp)->spr_Colors[0].a;
	  r1 = ((ExtSpriteObj*)sp)->spr_Colors[1].r;
	  g1 = ((ExtSpriteObj*)sp)->spr_Colors[1].g;
	  b1 = ((ExtSpriteObj*)sp)->spr_Colors[1].b;
	  a1 = ((ExtSpriteObj*)sp)->spr_Colors[1].a;
	  r2 = ((ExtSpriteObj*)sp)->spr_Colors[2].r;
	  g2 = ((ExtSpriteObj*)sp)->spr_Colors[2].g;
	  b2 = ((ExtSpriteObj*)sp)->spr_Colors[2].b;
	  a2 = ((ExtSpriteObj*)sp)->spr_Colors[2].a;
	  r3 = ((ExtSpriteObj*)sp)->spr_Colors[3].r;
	  g3 = ((ExtSpriteObj*)sp)->spr_Colors[3].g;
	  b3 = ((ExtSpriteObj*)sp)->spr_Colors[3].b;
	  a3 = ((ExtSpriteObj*)sp)->spr_Colors[3].a;
	  z0 = ((ExtSpriteObj*)sp)->spr_ZValues[0];
	  z1 = ((ExtSpriteObj*)sp)->spr_ZValues[1];
	  z2 = ((ExtSpriteObj*)sp)->spr_ZValues[2];
	  z3 = ((ExtSpriteObj*)sp)->spr_ZValues[3];
	  dr03 = (r3-r0)*yss;
	  dg03 = (g3-g0)*yss;
	  db03 = (b3-b0)*yss;
	  da03 = (a3-a0)*yss;
	  dz03 = (z3-z0)*yss;
	  dr12 = (r2-r1)*yss;
	  dg12 = (g2-g1)*yss;
	  db12 = (b2-b1)*yss;
	  da12 = (a2-a1)*yss;
	  dz12 = (z2-z1)*yss;
	}

	dx03 = (xx3-xx0)*yss;
	dx12 = (xx2-xx1)*yss;
	dy03 = (yy3-yy0)*yss;
	dy12 = (yy2-yy1)*yss;
	DBUGZ (("x0-3 = %f, %f, %f, %f\n", xx0, xx1, xx2, xx3));
	DBUGZ (("y0-3 = %f, %f, %f, %f\n", yy0, yy1, yy2, yy3));
	DBUGZ (("dx03 = %f, dx12 = %f\n", dx03, dx12));
	DBUGZ (("dy03 = %f, dy12 = %f\n", dy03, dy12));
	DBUGZ (("ystep = %d\n", ystep));
	lcb.YSize = ystep;
        
	for (i=0; i < ys; i+=ystep) {
	  /* If we know that this segment is going to be clipped, then
	   * don't load the TRAM.
	   */
	  tlc = (spc ? spc->spc_TxLoad : NULL);
	  if ((sp->spr_Flags & (SPR_CALC_VERTICES | SPR_CALC_TXLOAD)) ||
	      (tlc == NULL)) {
              
	    if (i+ystep>ys) {
	      lcb.YSize = ys-i;
	      dx03 = xx3-xx0;
	      dx12 = xx2-xx1;
	      dy03 = yy3-yy0;
	      dy12 = yy2-yy1;
	      if (extended) {
		dr03 = r3-r0;
		dg03 = g3-g0;
		db03 = b3-b0;
		da03 = a3-a0;
		dz03 = z3-z0;
		dr12 = r2-r1;
		dg12 = g2-g1;
		db12 = b2-b1;
		da12 = a2-a1;
		dz12 = z2-z1;
	      }
	    }
	    
	    DBUGZ (("width = %d, height = %d\n", lcb.XSize, lcb.YSize));
	    if ((sp->spr_Flags & SPR_CALC_TXLOAD) || (tlc == NULL)) {
#ifdef BUILD_BDA1_1
	      if (KB_FIELD(kb_Flags) & KB_BDA1_1) {
		GS_Reserve (gs, 2);
		CLT_Sync (GS_Ptr(gs));
		txd2.minY = lcb.YSize;
		txl.texelDataSize = (bpr*lcb.YSize+7)/8;
		csize = CLT_ComputeTxLoadCmdListSize (&lcb);
		if (csize<0) return csize;
		GS_Reserve (gs, csize);
		lcb.lcbCommandList.size = csize;
		lcb.lcbCommandList.data = *GS_Ptr(gs);
		CLT_CreateTxLoadCommandList (&lcb);
		*GS_Ptr(gs) += csize;
		txl.texelData = (void*)((uint32)txl.texelData + bpr*ystep/8);
		DBUGZ (("Base address = %x\n", txl.texelData));
		DBUGZ (("Old size %d, new size %d\n", 
			txd->texelData->texelDataSize, txl.texelDataSize));
		DBUGZ (("Old Ysize %d, new YSize %d\n", txd->minY, txd2.minY));
	      } else 
#endif
		{
		  lcb.XOffset = 0;
		  lcb.YOffset = i;
		  
		  csize = CLT_ComputeTxLoadCmdListSize (&lcb);
		  if (csize<0) return csize;
		  if (tlc = CreateTxLoadCache(spc, csize)) {
		    lcb.lcbCommandList.data = tlc->tlc_TxLoad;
		  } else {
		    GS_Reserve (gs, 2);
		    CLT_Sync (GS_Ptr(gs));
		    GS_Reserve (gs, csize);
		    lcb.lcbCommandList.data = *GS_Ptr(gs);
		    *GS_Ptr(gs) += csize;
		  }
		  lcb.lcbCommandList.size = csize;
		  CLT_CreateTxLoadCommandList (&lcb);
		}
	    }
	    
	    if (sp->spr_Flags & SPR_CALC_VERTICES) {
	      memset ((void*)geo,0,sizeof(geo));
	      geo[0].x = xx0;
	      geo[0].y = yy0;
	      /* geo[0].u = 0.; */
	      /* geo[0].v = 0.; */
	      geo[1].x = xx1;
	      geo[1].y = yy1;
	      geo[1].u = xmax;
	      /* geo[1].v = 0.; */
	      geo[2].x = xx1+=dx12;
	      geo[2].y = yy1+=dy12;
	      geo[2].u = xmax;
	      geo[2].v = lcb.YSize;
	      geo[3].x = xx0+=dx03;
	      geo[3].y = yy0+=dy03;
	      /* geo[3].u = 0.; */
	      geo[3].v = lcb.YSize;
	      if (extended) {
		geo[0].r = r0;
		geo[0].g = g0;
		geo[0].b = b0;
		geo[0].a = a0;
		geo[0].w = z0;
		geo[1].r = r1;
		geo[1].g = g1;
		geo[1].b = b1;
		geo[1].a = a1;
		geo[1].w = z1;
		geo[2].r = r1+=dr12;
		geo[2].g = g1+=dg12;
		geo[2].b = b1+=db12;
		geo[2].a = a1+=da12;
		geo[2].w = z1+=dz12;
		geo[3].r = r0+=dr03;
		geo[3].g = g0+=dg03;
		geo[3].b = b0+=db03;
		geo[3].a = a0+=da03;
		geo[3].w = z0+=dz03;
	      } else {
		geo[0].w = geo[1].w = geo[2].w = geo[3].w = .999998;
	      }
	      DBUGZ (("lcb.YSize = %d\n", lcb.YSize));
	    }
	  }
	  if (tlc &&
	      ((spc && (spc->spc_Flags & SPC_FLAG_VALID)) ||
	       (sp->spr_Flags & SPR_CALC_VERTICES))) {
	    /* JUMP to these instructions.
	     * Minor inefficiency: Always do the TxLoad if recalculating
	     * the vertices because we don't know yet if these vertices
	     * are going to be clipped.
	     */
	    GS_Reserve(gs, tlc->tlc_TxLoadSize+2);
	    CLT_Sync (GS_Ptr(gs));
	    memcpy (*GS_Ptr(gs), tlc->tlc_TxLoad, tlc->tlc_TxLoadSize*4);
	    *GS_Ptr(gs) += tlc->tlc_TxLoadSize;
	  }
	  if (extended) {
	    BuildVertices(sp, spc, gs, geo, dontClip, 
			  GEOF2_TEXCOORDS+GEOF2_COLORS);
	  } else {
	    BuildVertices(sp, spc, gs, geo, dontClip, GEOF2_TEXCOORDS);
	  }
	  
	  prev = spc;
	  spc = (SpriteCache *)spc->spc_Node.n_Next;
	  if (spc == NULL) {
	    spc = CreateSpriteCache(prev, gs);
	  }
	}
      } else {
	Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
	uint32 i, j, ip1, jp1;
	gfloat hs=sp->spr_HSlice, vs=sp->spr_VSlice, hsvs=hs*vs;
	gfloat xinc=xmax/vs, yinc=ymax/hs;
	gfloat hdx=(xx1-xx0)/vs;
	gfloat hdy=(yy1-yy0)/vs;
	gfloat vdx=(xx3-xx0)/hs;
	gfloat vdy=(yy3-yy0)/hs;
	gfloat ddx=(xx0+xx2-xx1-xx3)/hsvs;
	gfloat ddy=(yy0+yy2-yy1-yy3)/hsvs;
	gfloat w = (gfloat)bm->bm_Width;
	gfloat h = (gfloat)bm->bm_Height;
	gfloat c0x = xx0;
	gfloat c0y = yy0;

	if (xx0>=0. && xx1>=0. && xx2>=0. && xx3>=0. &&
	    xx0<=w && xx1<=w && xx2<=w && xx3<=w &&
	    yy0>=0. && yy1>=0. && yy2>=0. && yy3>=0. &&
	    yy0<=h && yy1<=h && yy2<=h && yy3<=h ) {
            /* Cache this stuff!! SAS */
	  DBUGX (("Unclipped\n"));
	  for (j=0; j<sp->spr_HSlice; j++) {
	    DBUGZ (("Reserve\n"));
	    GS_Reserve (gs, 5*(2*sp->spr_VSlice+1) + 1);
	    DBUGZ (("Triangle\n"));
	    CLT_TRIANGLE (GS_Ptr(gs), 
			  1, RC_STRIP, 1, 1, 0, 2*(sp->spr_VSlice+1));
	    jp1 = j+1;
	    for (i=0; i<=sp->spr_VSlice; i++) {
	      DBUGZ (("Vertex\n"));
	      CLT_VertexUvW (GS_Ptr(gs), c0x+vdx*j+hdx*i+ddx*i*j,
			     c0y+vdy*j+hdy*i+ddy*i*j,
			     xinc*i, yinc*j, .999998);
	      CLT_VertexUvW (GS_Ptr(gs), c0x+vdx*jp1+hdx*i+ddx*i*jp1,
			     c0y+vdy*jp1+hdy*i+ddy*i*jp1,
			     xinc*i, yinc*jp1, .999998);
	    }
	  }
	} else {
	  _geouv geo[4];
	  memset ((void*)geo,0,sizeof(geo));
	  geo[0].w = .999998;
	  geo[1].w = .999998;
	  geo[2].w = .999998;
	  geo[3].w = .999998;
	  DBUGX (("Clip\n"));
	  for (j=0; j<sp->spr_HSlice; j++) {
	    jp1 = j+1;
	    for (i=0; i<sp->spr_VSlice; i++) {
	      ip1 = i+1;
	      geo[0].x = c0x+vdx*j+hdx*i+ddx*i*j;
	      geo[0].y = c0y+vdy*j+hdy*i+ddy*i*j;
	      geo[0].u = xinc*i;
	      geo[0].v = yinc*j;
	      geo[1].x = c0x+vdx*j+hdx*ip1+ddx*ip1*j;
	      geo[1].y = c0y+vdy*j+hdy*ip1+ddy*ip1*j;
	      geo[1].u = xinc*ip1;
	      geo[1].v = yinc*j;
	      geo[2].x = c0x+vdx*jp1+hdx*ip1+ddx*ip1*jp1;
	      geo[2].y = c0y+vdy*jp1+hdy*ip1+ddy*ip1*jp1;
	      geo[2].u = xinc*ip1;
	      geo[2].v = yinc*jp1;
	      geo[3].x = c0x+vdx*jp1+hdx*i+ddx*i*jp1;
	      geo[3].y = c0y+vdy*jp1+hdy*i+ddy*i*jp1;
	      geo[3].u = xinc*i;
	      geo[3].v = yinc*jp1;
              BuildVertices(sp, spc, gs, geo, dontClip, GEOF2_TEXCOORDS);
              prev = spc;
              spc = (SpriteCache *)spc->spc_Node.n_Next;
              if (spc == NULL) {
                  spc = CreateSpriteCache(prev, gs);
              }
	    }
	  }
	}
      }
    }
  }

  DBUG(("Getting outta here\n"));
      /* next time, there is no need to recalc the vertices */
  sp->spr_Flags &= ~(SPR_CALC_VERTICES | SPR_CALC_TXLOAD);

  return 0;
}


static Err
DrawSpriteObj (GState *gs, SpriteObj *sp)
{
  gfloat xx0=0., xx1=0., xx2=0., xx3=0., yy0=0., yy1=0., yy2=0., yy3=0.;
  int32 geoflag;

  geoflag = (sp->spr_Flags&SPR_GEOMETRYENABLE);
  if ((sp->spr_Node.n_Type!=SHORTSPRITEOBJNODE)
      && geoflag) {
    geoflag = 1;
    xx0 = sp->spr_Corners[0].x + sp->spr_Position.x;
    xx1 = sp->spr_Corners[1].x + sp->spr_Position.x;
    xx2 = sp->spr_Corners[2].x + sp->spr_Position.x;
    xx3 = sp->spr_Corners[3].x + sp->spr_Position.x;
    yy0 = sp->spr_Corners[0].y + sp->spr_Position.y;
    yy1 = sp->spr_Corners[1].y + sp->spr_Position.y;
    yy2 = sp->spr_Corners[2].y + sp->spr_Position.y;
    yy3 = sp->spr_Corners[3].y + sp->spr_Position.y;
  }
  return _draw_sprite_core (gs, sp, geoflag, 
			    xx0, xx1, xx2, xx3, 
			    yy0, yy1, yy2, yy3);
}


static Err
DrawGridObj (GState *gs, GridObj *gr)
{
  int32 i, j;
  gfloat xx0, xx1, xx2, xx3, yy0, yy1, yy2, yy3;
  Err err;

  for (j=0; j<gr->gro_Height; j++) {
    gfloat jj=j;
    for (i=0; i<gr->gro_Width; i++) {
      gfloat ii=i;
      int32 index=j*gr->gro_Width+i;

      xx0 = gr->gro_Position.x
	+ ii*gr->gro_HDelta.x + jj*gr->gro_VDelta.x;
      yy0 = gr->gro_Position.y
	+ ii*gr->gro_HDelta.y + jj*gr->gro_VDelta.y;
      xx1 = xx0 + gr->gro_HDelta.x;
      yy1 = yy0 + gr->gro_HDelta.y;
      xx2 = xx1 + gr->gro_VDelta.x;
      yy2 = yy1 + gr->gro_VDelta.y;
      xx3 = xx0 + gr->gro_VDelta.x;
      yy3 = yy0 + gr->gro_VDelta.y;

      gr->gro_SpriteArray[index]->spr_Flags |= SPR_CALC_VERTICES;
      err = _draw_sprite_core (gs, gr->gro_SpriteArray[index], -1,
			       xx0, xx1, xx2, xx3,
			       yy0, yy1, yy2, yy3);
      gr->gro_SpriteArray[index]->spr_Flags |= SPR_CALC_VERTICES;
      if (err<0) return err;
    }
  }
  return 0;
}




/**
|||	AUTODOC -public -class frame2d -name F2_Draw
|||	Draw a 2D object on the display
|||
|||	  Synopsis
|||
|||	    Err F2_Draw (GState *gs, void* f2obj);
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a 2d object
|||	    on the display.  If the 2D object has no geometry, then just
|||	    enter the commands to modify the state of the triangle engine.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    f2obj
|||	        Pointer to the 2D object to be drawn.
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_DrawList()
|||
**/
Err
F2_Draw (GState *gs, void* f2obj)
{
  if (((Frame2Obj*)f2obj)->Flags&F2_SKIP) return 0;

  switch (((Frame2Obj*)f2obj)->Node.n_Type) {
  case SPRITEOBJNODE:
  case EXTSPRITEOBJNODE:
  case SHORTSPRITEOBJNODE:
    return DrawSpriteObj (gs, (SpriteObj*)f2obj);
  case GRIDOBJNODE:
    return DrawGridObj (gs, (GridObj*)f2obj);
  default:
    return -1;
  }
}


/**
|||	AUTODOC -public -class frame2d -name F2_DrawList
|||	Draw a 2D object on the display
|||
|||	  Synopsis
|||
|||	    Err F2_DrawList (GState *gs, List *l);
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw each 2d object
|||	    in a list.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    l
|||	        Pointer to the list of 2D objects to be drawn.
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_Draw()
|||
**/
Err
F2_DrawList (GState *gs, const List* l)
{
  Node *f2obj;
  Err r;

  f2obj = FIRSTNODE(l);
  while (ISNODE(l,f2obj)) {
    r = F2_Draw (gs, f2obj);
    if (r<0) return r;
    f2obj = NEXTNODE(f2obj);
  }

  return 0;
}


