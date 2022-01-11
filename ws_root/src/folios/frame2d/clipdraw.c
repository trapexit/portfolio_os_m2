/*
 *	@(#) clipdraw.c 96/03/15 1.12
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D Clipping routines
 */


#include "frame2.i"


#define DBUG(x) /* printf x */
#define DBUGX(x) /* printf x */
#define DBUGA(x) /* printf x */

SpriteCache *SetUpCache(SpriteCache *spc, uint32 vertices, uint32 wordsPerVertex)
{
    uint32 byteCount;

    if (spc == NULL) {
        return(NULL);
    }
    
    byteCount = (spc->spc_VertexWords = vertices * wordsPerVertex + 1) * 4;
		/* include 1 instruction word */
    if (byteCount > spc->spc_ByteCount) {
        FreeMem(spc->spc_Vertices, spc->spc_ByteCount);
        spc->spc_ByteCount = 0;
	DBUGA (("Alloc: SetupCache\n"));
        spc->spc_Vertices = (uint32 *)AllocMem(byteCount, MEMTYPE_NORMAL);
        if (spc->spc_Vertices) {
            spc->spc_ByteCount = byteCount;
        }
    }
    return(spc);
}

SpriteCache *
_clip_fan2d (GState *gs, SpriteCache *spc, int32 style, int32 count,
             _geouv *geo, bool dontClip)
{
  gfloat w, h, ratio;
  _geouv geo2[8], geo3[8], *g1, *g2;
  int32 c1, c2, i, iold;
  Bitmap* bm;
  CmdListP *cl;
  CmdListP cachecl;
  
  bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
  w = bm->bm_Width;
  h = bm->bm_Height;

  c1 = count; g1 = geo;
  g2 = geo2;

  if (dontClip) {
      goto calc_vertices;
  }
  
  /* Clip against left edge of display */
  for (i=0; i<c1; i++) {
    if (g1[i].x<0.) break;
  }
  if (i<c1) {
    c2 = 0;
    iold = c1-1;
    for (i=0; i<c1; i++) {
      if ((g1[iold].x<0. && g1[i].x>0.) || (g1[iold].x>0. && g1[i].x<0.)) {
	ratio = (-g1[iold].x)/(g1[i].x-g1[iold].x);
	g2[c2].x = 0.;
	g2[c2].y = g1[iold].y+ratio*(g1[i].y-g1[iold].y);
	g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
	g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
	g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
	g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
	g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
	g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
	g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
	c2++;
      }
      if (g1[i].x>=0.) {
	g2[c2].x = g1[i].x;
	g2[c2].y = g1[i].y;
	g2[c2].u = g1[i].u;
	g2[c2].v = g1[i].v;
	g2[c2].w = g1[i].w;
	g2[c2].r = g1[i].r;
	g2[c2].g = g1[i].g;
	g2[c2].b = g1[i].b;
	g2[c2].a = g1[i].a;
	c2++;
      }
      iold = i;
    }
    g1 = g2;
    c1 = c2;
    g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
  }

  /* Clip against top edge of display */
  for (i=0; i<c1; i++) {
    if (g1[i].y<0.) break;
  }
  if (i<c1) {
    c2 = 0;
    iold = c1-1;
    for (i=0; i<c1; i++) {
      if ((g1[iold].y<0. && g1[i].y>0.) || (g1[iold].y>0. && g1[i].y<0.)) {
	ratio = (-g1[iold].y)/(g1[i].y-g1[iold].y);
	g2[c2].x = g1[iold].x+ratio*(g1[i].x-g1[iold].x);
	g2[c2].y = 0.;
	g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
	g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
	g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
	g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
	g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
	g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
	g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
	c2++;
      }
      if (g1[i].y>=0.) {
	g2[c2].x = g1[i].x;
	g2[c2].y = g1[i].y;
	g2[c2].u = g1[i].u;
	g2[c2].v = g1[i].v;
	g2[c2].w = g1[i].w;
	g2[c2].r = g1[i].r;
	g2[c2].g = g1[i].g;
	g2[c2].b = g1[i].b;
	g2[c2].a = g1[i].a;
	c2++;
      }
      iold = i;
    }
    g1 = g2;
    c1 = c2;
    g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
  }

  /* clip against right edge of display */
  for (i=0; i<c1; i++) {
    if (g1[i].x>w) break;
  }
  if (i<c1) {
    c2 = 0;
    iold = c1-1;
    for (i=0; i<c1; i++) {
      if ((g1[iold].x<w && g1[i].x>w) || (g1[iold].x>w && g1[i].x<w)) {
	ratio = (w-g1[iold].x)/(g1[i].x-g1[iold].x);
	g2[c2].x = w;
	g2[c2].y = g1[iold].y+ratio*(g1[i].y-g1[iold].y);
	g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
	g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
	g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
	g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
	g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
	g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
	g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
	c2++;
      }
      if (g1[i].x<=w) {
	g2[c2].x = g1[i].x;
	g2[c2].y = g1[i].y;
	g2[c2].u = g1[i].u;
	g2[c2].v = g1[i].v;
	g2[c2].w = g1[i].w;
	g2[c2].r = g1[i].r;
	g2[c2].g = g1[i].g;
	g2[c2].b = g1[i].b;
	g2[c2].a = g1[i].a;
	c2++;
      }
      iold = i;
    }
    g1 = g2;
    c1 = c2;
    g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
  }

  /* clip against bottom edge of display */
  for (i=0; i<c1; i++) {
    if (g1[i].y>h) break;
  }
  if (i<c1) {
    c2 = 0;
    iold = c1-1;
    for (i=0; i<c1; i++) {
      if ((g1[iold].y<h && g1[i].y>h) || (g1[iold].y>h && g1[i].y<h)) {
	ratio = (h-g1[iold].y)/(g1[i].y-g1[iold].y);
	g2[c2].x = g1[iold].x+ratio*(g1[i].x-g1[iold].x);
	g2[c2].y = h;
	g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
	g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
	g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
	g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
	g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
	g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
	g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
	c2++;
      }
      if (g1[i].y<=h) {
	g2[c2].x = g1[i].x;
	g2[c2].y = g1[i].y;
	g2[c2].u = g1[i].u;
	g2[c2].v = g1[i].v;
	g2[c2].w = g1[i].w;
	g2[c2].r = g1[i].r;
	g2[c2].g = g1[i].g;
	g2[c2].b = g1[i].b;
	g2[c2].a = g1[i].a;
	c2++;
      }
      iold = i;
    }
    g1 = g2;
    c1 = c2;
    /* g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3); */
  }

  calc_vertices:
  if (c1>2) {
    switch (style) {
    case 0:
      spc = SetUpCache(spc, c1, 3);
      if (spc && spc->spc_Vertices) {
          cachecl = (CmdListP)spc->spc_Vertices;
          cl = &cachecl;   /* build the vertex list into here instead of the gstate */
          spc->spc_Flags |= SPC_FLAG_VALID;
      }
      else {
          GS_Reserve (gs, c1*3+1); /* Build the vertex list into the GState */
	  cl = GS_Ptr(gs);
      }
      CLT_TRIANGLE (cl, 1, RC_FAN, 1, 0, 0, c1);
      DBUGX (("Fan %d\n", c1));
      for (i=0; i<c1; i++) {
	DBUGX (("%g %g %g\n", g1[i].x, g1[i].y, g1[i].w));
	CLT_VertexW (cl, g1[i].x, g1[i].y, g1[i].w);
      }
      break;
    case GEOF2_COLORS:
      spc = SetUpCache(spc, c1, 7);
      if (spc && spc->spc_Vertices) {
          cachecl = (CmdListP)spc->spc_Vertices;
          cl = &cachecl;   /* build the vertex list into here instead of the gstate */
          spc->spc_Flags |= SPC_FLAG_VALID;
      }
      else {
          GS_Reserve (gs, c1*7+1); /* Build the vertex list into the GState */
	  cl = GS_Ptr(gs);
      }
      CLT_TRIANGLE (cl, 1, RC_FAN, 1, 0, 1, c1);
      DBUGX (("Fan %d\n", c1));
      for (i=0; i<c1; i++) {
	DBUGX (("%g %g %g %g %g %g %g\n", g1[i].x, g1[i].y, 
		g1[i].r, g1[i].g, g1[i].b, g1[i].a, g1[i].w));
	CLT_VertexRgbaW (cl, g1[i].x, g1[i].y,
			 g1[i].r, g1[i].g, g1[i].b, g1[i].a, g1[i].w);
      }
      break;
    case GEOF2_TEXCOORDS:
      spc = SetUpCache(spc, c1, 5);
      if (spc && spc->spc_Vertices) {
          cachecl = (CmdListP)spc->spc_Vertices;
          cl = &cachecl;   /* build the vertex list into here instead of the gstate */
          spc->spc_Flags |= SPC_FLAG_VALID;
      }
      else {
          GS_Reserve (gs, c1*5+1); /* Build the vertex list into the GState */
	  cl = GS_Ptr(gs);
      }
      CLT_TRIANGLE (cl, 1, RC_FAN, 1, 1, 0, c1);
      DBUGX (("Fan %d\n", c1));
      for (i=0; i<c1; i++) {
	DBUGX (("%g %g %g %g %g\n", 
		g1[i].x, g1[i].y, g1[i].u, g1[i].v, g1[i].w));
	CLT_VertexUvW (cl, g1[i].x, g1[i].y,
		       g1[i].u, g1[i].v, g1[i].w);
      }
      break;
    case (GEOF2_COLORS + GEOF2_TEXCOORDS):
      spc = SetUpCache(spc, c1, 9);
      if (spc && spc->spc_Vertices) {
          cachecl = (CmdListP)spc->spc_Vertices;
          cl = &cachecl;   /* build the vertex list into here instead of the gstate */
          spc->spc_Flags |= SPC_FLAG_VALID;
      }
      else {
          GS_Reserve (gs, c1*9+1); /* Build the vertex list into the GState */
	  cl = GS_Ptr(gs);
      }
      CLT_TRIANGLE (cl, 1, RC_FAN, 1, 1, 1, c1);
      DBUGX (("Fan %d\n", c1));
      for (i=0; i<c1; i++) {
	DBUGX (("%g %g %g %g\n", g1[i].x, g1[i].y, 
		g1[i].r, g1[i].g, g1[i].b, g1[i].a, 
		g1[i].u, g1[i].v, g1[i].w));
	CLT_VertexRgbaUvW (cl, g1[i].x, g1[i].y,
			   g1[i].r, g1[i].g, g1[i].b, g1[i].a,
			   g1[i].u, g1[i].v, g1[i].w);
      }
      break;
    default:
      DBUGX (("Internal error - invalid geometry style being clipped\n"));
      break;
    }
  }
  else {
      if (spc && spc->spc_Node.n_Next) {
          spc = (SpriteCache *)spc->spc_Node.n_Next;
      }
  }
  
  return(spc);
}


