
/******************************************************************************
**
**  @(#) opthf.c 96/05/01 1.18
**
**  This file contains the C version of the function that takes
**  a height field object and produces a Triangle Engine command
**  list
**
******************************************************************************/

#include <graphics/gp.h>
#include "hf.h"
#include <stdio.h>

#define MAXU 1024.
#define MAXV 1024.

/* There are 9 floats in a command list vertex */
#define VERTEX_SIZE 36

/* We have just run out of buffer space
   so we need to call the list full function
   to send what's in the command list off to
   be rendered
   */

#if PRINT
void
PrintCommandList(void *buffer, uint32 nbytes);
static int printBuf = 0;
#endif

extern void gps_update_total_matrix(GPData *gp);

static const gfloat i255 = 1./255.;

typedef struct ClipVertex {
    gfloat 	x,y,z,w;
    gfloat 	r,g,b,a;
    gfloat	u,v;
    uint16	mat1, mat2;
    int16   	clip_code;
} ClipVertex;

typedef struct TempVertex {
    gfloat 	x,y,w;
    gfloat 	r,g,b,a;
    uint16	mat1, mat2;
} TempVertex;

#define outputVert(p) \
	*TE_out++ = p->x; \
	*TE_out++ = p->y; \
	*TE_out++ = p->r; \
	*TE_out++ = p->g; \
	*TE_out++ = p->b; \
	*TE_out++ = p->a; \
	*TE_out++ = p->w; \
 if (u<0 || v<0) printf("u = %f v = %f\n",u,v);\
	*TE_out++ = u*p->w;\
	*TE_out++ = v*p->w;


static void
GetMoreTEList(float** pTE_out,
	      uint32 *pbytes_left,
	      GPData *gp)
{
	gp->m_GState->gs_ListPtr = (CmdListP)*pTE_out;
	GS_SendList( gp->m_GState );
	*pTE_out = (float*)gp->m_GState->gs_ListPtr;
	*pbytes_left = ((uint32)gp->m_GState->gs_EndList) -
				   ((uint32)gp->m_GState->gs_ListPtr);
}

void
optDrawHF (GPData *gp, HFData *hf)
{
    uint32	bytes_left;			/* Number of bytes left in buffer*/
    uint32	bytes_needed;		/* Number of bytes in one command*/
    gfloat	wscale;				/* scale vaule to apply to 1/w*/
    gfloat	*TE_out;			/* Pointer to next output */
    HFPacket	*pdata;			/* Points to the current input */
    HFPacket	*pdata0;		/* Start of row */

    uint32	header;				/* The TE vertex command word */
    uint32	nrow, ncol;			/* Register versions of data structs*/
    uint32	icol, irow;			/* indices */
    int32	ix0, ix, iy;		/* Actual coords of current block */
    int32	mat=0;				/* Current material */
    int32	lastMat;			/* Last material */

    TempVertex	*thisRow, *lastRow; /* pointers to 2 rows */
    TempVertex	*ctemp0, *ctemp1; /* 2 temp pointers */
    TempVertex	*p0, *p1, *p2;

    gfloat	x, y, w;			/* Current vertex */
    gfloat	h;					/* Current height */
    gfloat	w1;					/* 1/w temp */
    gfloat	rx, ry, rw;			/* Start of row */
    gfloat	Xx, Xy, Xw;			/* Delta for transform across row */
    gfloat	Yx, Yy, Yw;			/* Delta for transform down columns */
    gfloat	Zx, Zy, Zw;			/* Delta for transform up height */

    gfloat	u,v;				/* U,V at current vertex */
    int32	uscale=0, vscale=0;	/* scale of current texture */
    gfloat	ru=0,rv=0;				/* U,V at start of current row */

    uint32	lastNverts;			/* Number of verts produced in
								   last command */
    uint32	*lastHeader;		/* Address of where last heade
								   was poked */

    /* Non-clipped command list assumes that all the vertices are
       visible and no culling is performed. This makes it possible
       to place a vertex directly into the command list without
       any conditionals.

       */
	gps_update_total_matrix(gp);

    {
		gfloat x30, x31, x33;
		gfloat x0, y0;			/* Starting coords */

		gfloat *f;

		f = &gp->m_TotalMatrix.data[0][0];

		Xx = *f++ * hf->deltax;
		Xy = *f++;
		f++;					/* x02 not used */
		Xw = *f++;
		Yx = *f++;
		Yy = *f++ * hf->deltay;
		f++;					/*x12 not used*/
		Yw = *f++;
		Zx = *f++;
		Zy = *f++;
		f++;					/* X22 not used */
		Zw = *f++;
		x30 = *f++;
		x31 = *f++;
		f++;					/* X32 not used */
		x33 = *f;

		x0 = hf->xstart;
		y0 = hf->ystart;

		rx = x0*Xx + y0*Yx + x30;
		ry = x0*Xy + y0*Yy + x31;
		rw = x0*Xw + y0*Yw + x33;
    }

    /* Cache in a register */
    wscale = gp->m_WScale;

    /* Place in local variable/register */
    bytes_left = ((uint32)gp->m_GState->gs_EndList) -
				 ((uint32)gp->m_GState->gs_ListPtr);
	TE_out = (float*) gp->m_GState->gs_ListPtr;

    ncol = hf->rx;
    nrow = hf->ry;

    header = CLA_TRIANGLE(1, RC_STRIP, 1, 1, 1, 1);

    {
		uint32 tempSize;

		/* Make sure we have enough temp buffer allocated to transform
		   all the vertices. We may as well reserve enough space
		   for clipping always */
		tempSize = 2 * ncol * sizeof(ClipVertex);
		if (gp->m_TempBufferSize < tempSize) {
			(*gp->m_TempReallocateFunction)(gp->m_TempBuffer, tempSize,
											&gp->m_TempBuffer, &gp->m_TempBufferSize);
		}
    }

    thisRow = (TempVertex*)gp->m_TempBuffer;
    lastRow = thisRow + ncol;

    ix0 = hf->xstart % hf->nx;
    if (ix0 < 0) ix0 += hf->nx;

    iy = hf->ystart % hf->ny;
    if (iy < 0) iy += hf->ny;

    pdata0 = hf->fdata + iy*hf->nx + ix0;

    /* Do triangles between 2 rows up to 2nd to last row */
    for (irow = 1; irow < nrow; irow++) {

		/* Start out without any texture */
		lastMat = -1;

		x = rx; y = ry; w = rw;

		ix = ix0;
		pdata = pdata0;

		ctemp0 = thisRow;
		thisRow = lastRow;
		lastRow = ctemp0;

		/* Transform one row of vertices into thisRow */
		ctemp0 = thisRow;

		icol = ncol;
		while(icol-->0) {

			h = pdata->h;
			w1 = 1./(w+h*Zw);
			ctemp0->x = w1*(x+h*Zx);
			ctemp0->y = w1*(y+h*Zy);

			ctemp0->r = pdata->r*i255;
			ctemp0->g = pdata->g*i255;
			ctemp0->b = pdata->b*i255;
			ctemp0->a = pdata->a*i255;

			ctemp0->w = w1 * wscale;

			ctemp0->mat1 = pdata->mat1;
			ctemp0->mat2 = pdata->mat2;

			/* Move on to next column */
			x += Xx;
			y += Xy;
			w += Xw;

			ctemp0++;
			ix++;
			if (ix >= hf->nx) {
				ix -= hf->nx;
				pdata = pdata0;
			} else {
				pdata++;
			}
		}

		/* First time through we don't have enough data to actually
		   put out a strip */
		if (irow==1)
		  goto advance;


		/*
		   output all vertices by using ctemp0 and ctemp1

		   There are ncol-1 quads, each quad has 2 triangles:

		   ctemp0, ctemp0+1, ctemp0+2 . .
		   ctemp1, ctemp1+1, ctemp1+2 . .

		   Put out strips breaking them when materials change.
		   p1 = ctemp1,  holds the material index for both triangles

		   */

		ctemp0 = lastRow;
		ctemp1 = thisRow;


		/* No strips in progress */
		lastNverts = 0;
		lastHeader = 0;
		/* icol and irow are used to index the texture scales */

		for (icol = 1; icol < ncol; icol++)  {

			uint32 twice = 2;


			/* first triangle */
			p0 = ctemp0;
			p1 = ctemp1;
			p2 = ctemp0+1;

			mat = p0->mat1;

			while (twice-->0) {

				if (mat != lastMat) {
					HFMaterial *m;
					/* Don't accumulate any more */
					if (lastNverts != 0) {
						*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
						lastNverts = 0;
					}
					m = &hf->mattable[mat];

					/* start off the new material */

					uscale = m->uscale;
					vscale = m->vscale;
					ru = uscale*icol;
					if (ru >= MAXU) ru -= MAXU;
					if (ru < uscale) {
						ru += hf->mattable[mat].usize;
					}
					rv = vscale*irow;
					if (rv >= MAXV) rv -= MAXV;
					if (rv < vscale)
					  rv += hf->mattable[mat].vsize;

					/* Output TE commands to setup texture registers for current texture
					   include 8 bytes for sync */
					bytes_needed = 4*(CLT_GetSize(&m->tabSnip)+CLT_GetSize(&m->useSnip))+8;
					if (bytes_left < bytes_needed) {
						GetMoreTEList(&TE_out, &bytes_left, gp);
					}
					CLT_Sync(((uint32**)&TE_out));
					CLT_CopySnippetData((uint32**)&TE_out, &m->useSnip);
					CLT_CopySnippetData((uint32**)&TE_out, &m->tabSnip);
					bytes_left -= bytes_needed;
				}

				/* Starting a new strip? or continuing an old one? */
				if (lastNverts == 0) {
					bytes_needed = 3*VERTEX_SIZE + 4;
				} else {
					bytes_needed = VERTEX_SIZE;
				}

				if (bytes_left < bytes_needed) {
					/* Patch in progress command */
					if (lastNverts != 0) {
						*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
						lastNverts = 0;
						bytes_needed = 3*VERTEX_SIZE + 4;
					}
					GetMoreTEList(&TE_out, &bytes_left, gp);
					/* Debug check */
					if (bytes_left < bytes_needed) {
						return; /* give up */
					}
				}

				/* commit these new bytes */

				if (lastNverts == 0) {
					lastHeader = (uint32*)TE_out++;
					lastNverts = 2;
					/* Texture coords are:
					   first time:
					   p0 ru-uscale, rv-vscale,
					   p1 ru-uscale, rv
					   p2 ru, rv-vscale
					   second time:
					   p0 ru, rv-vscale,
					   p1 ru-uscale, rv
					   p2 ru, rv
					   */

					/* output p0 and p1 */
					v = rv-vscale;
					if (twice==1) {
						u = ru-uscale;
					} else {
						u = ru;
					}
					outputVert(p0);
					/* p1 is same for both triangles */
					u = ru-uscale;
					v = rv;
					outputVert(p1);
				}
				bytes_left -= bytes_needed;
				lastNverts++;
				u = ru;
				if (twice == 1) {
					v = rv-vscale;
				} else {
					v = rv;
				}
				outputVert(p2);

				/* Now do second triangle in quad */
				lastMat = mat;
				mat = p0->mat2;
				p0 = p2;
				p2 = ctemp1+1;
			}

			/* Do remaining vertices in row */

			ru += uscale;
			if (ru >= MAXU) {
				ru -= MAXU;
				if (ru < uscale) {
					ru += hf->mattable[mat].usize;
				}
				/* End current strip because we wrapped */
				if (lastNverts != 0) {
					*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
					lastNverts = 0;
				}
			}
			ctemp0++;
			ctemp1++;
		}

		if (lastNverts != 0) {
			*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
		}
		/* Advance to next row */

		ru = uscale;
		rv += vscale;
		if (rv >= MAXV) {
			rv -= MAXV;
			if (rv < vscale)
			  rv += hf->mattable[mat].vsize;
		}

	advance:
		rx += Yx;
		ry += Yy;
		rw += Yw;

		iy++;
		if (iy >= hf->ny) {
			iy -= hf->ny;
			pdata0 = hf->fdata + ix0;
		} else {
			pdata0 += hf->nx;
		}
    }
	gp->m_GState->gs_ListPtr = (CmdListP)TE_out;

#if PRINT
    if (printBuf) {
		uint32 idx = gp->m_GState->gs_WhichList;
		PrintCommandList(gp->m_GState->gs_TeRenderInfo[idx].ri_CmdList,
						 gp->m_GState->gs_EndList);
    }
#endif
}
/*----------------------------------------------------------------------

  Clipped version

*/

#define outputVertClip(p) \
	w1 = 1.f/p->w; \
	*TE_out++ = p->x*w1; \
	*TE_out++ = p->y*w1; \
	*TE_out++ = p->r; \
	*TE_out++ = p->g; \
	*TE_out++ = p->b; \
	*TE_out++ = p->a; \
	w1 *= wscale;\
	*TE_out++ = w1; \
 if (u<0 || v<0) printf("u = %f v = %f\n",u,v);\
	*TE_out++ = u*w1;\
	*TE_out++ = v*w1;

#define outputVertClip1(p) \
	w1 = 1.f/p->w; \
	*TE_out++ = p->x*w1; \
	*TE_out++ = p->y*w1; \
	*TE_out++ = p->r; \
	*TE_out++ = p->g; \
	*TE_out++ = p->b; \
	*TE_out++ = p->a; \
	w1 *= wscale;\
	*TE_out++ = w1; \
 if (p->u<0 || p->v<0) printf("p->u = %f p->v = %f\n",p->u,p->v);\
	*TE_out++ = p->u*w1;\
	*TE_out++ = p->v*w1;

typedef struct ClipPoly {
    int 	n;					/* number of sides */
    ClipVertex 	*vert;			/* vertices */
} ClipPoly;


#define SWAP(a, b, temp)        {temp = a; a = b; b = temp;}
#define COORD(vert, i) ((gfloat *)(vert))[i]

#define PLANEX1 0x01
#define PLANEX2 0x02
#define PLANEY1 0x04
#define PLANEY2 0x08
#define PLANEZ1 0x10
#define PLANEZ2 0x20

/*
 * PerPolyHalfPlaneClip: clip convex polygon p against a plane,
 * copying the portion satisfying sign*s[index] < k*w into q,
 * where s is a Poly_vert* cast as a gfloat*.
 * index is an index into the array of gfloat at each vertex, such that
 * s[index] is x, y, or z (screen space x, y, or z).
 * Thus, to clip against xmin, use
 *  poly_clip_to_halfspace(p, q, XINDEX, -1., -xmin);
 * and to clip against xmax, use
 *  poly_clip_to_halfspace(p, q, XINDEX,  1.,  xmax);
 */

static void
PerPolyHalfPlaneClip(ClipPoly * p, ClipPoly * q, int index, gfloat sign, gfloat k)
{
	ClipVertex *u, *v, *w;
	int32 i;
	gfloat t, tu, tv;

	q->n = 0;

	/* start with u=vert[n-1], v=vert[0] */
	u = &p->vert[p->n - 1];
	tu = sign*(COORD(u, index) - u->w * k);
	for (v = &p->vert[0], i = p->n; i > 0; i--, u = v, tu = tv, v++) {
		tv = sign*(COORD(v, index) - v->w * k);
		if (((int) (tu < 0.) && (int) (tv > 0.)) ||
			((int) (tu > 0.) && (int) (tv < 0.))) {
			/*
			 * edge crosses plane; add intersection point to q
			 * if either u or v on the plane, don't calculate intersection.
			 */
			w = & q->vert[q->n];
			if (tu < tv) {
				t = tu / (tu - tv);
				w->x = u->x + t * (v->x - u->x);
				w->y = u->y + t * (v->y - u->y);
				w->w = u->w + t * (v->w - u->w);
				w->r = u->r + t * (v->r - u->r);
				w->g = u->g + t * (v->g - u->g);
				w->b = u->b + t * (v->b - u->b);
				w->a = u->a + t * (v->a - u->a);
				w->z = u->z + t * (v->z - u->z);
				w->u = u->u + t * (v->u - u->u);
				w->v = u->v + t * (v->v - u->v);
			} else {
				t = tv / (tv - tu);
				w->x = v->x + t * (u->x - v->x);
				w->y = v->y + t * (u->y - v->y);
				w->w = v->w + t * (u->w - v->w);
				w->r = v->r + t * (u->r - v->r);
				w->g = v->g + t * (u->g - v->g);
				w->b = v->b + t * (u->b - v->b);
				w->a = v->a + t * (u->a - v->a);
				w->z = v->z + t * (u->z - v->z);
				w->u = v->u + t * (u->u - v->u);
				w->v = v->v + t * (u->v - v->v);
			}
			q->n++;
		}
		if (tv <= 0.)			/* vertex v is in or on the clip plane, copy it to q */
		  q->vert[q->n++] = *v;
	}
}

void
optDrawHFClip (GPData *gp, HFData *hf)
{
    /*
       Clipping is done by first transforming all vertices and placing them
       in a temp buffer. Each row is placed contiguously in the temp buffer.
       Two rows at a time are used

       */

    uint32	bytes_left;			/* Number of bytes left in buffer*/
    uint32	bytes_needed;		/* Number of bytes in one command*/
    gfloat	wscale;				/* scale vaule to apply to 1/w*/
    gfloat	*TE_out;			/* Pointer to next output */
    ClipVertex	*thisRow, *lastRow; /* pointers to 2 rows */
    ClipVertex	*ctemp0, *ctemp1; /* 2 temp pointers */
    ClipVertex	*p0, *p1, *p2;
    ClipVertex	*p;

    ClipVertex	cvBuf1[9], cvBuf2[9];

    ClipPoly   	cpBuf1, cpBuf2;
    ClipPoly	*clipBuf1, *clipBuf2;
    ClipPoly	*r;				/* Temp used for swap */

    HFPacket	*pdata;			/* Points to the current input */
    HFPacket	*pdata0;		/* Start of row */

    uint32	header;				/* The TE vertex command word */
    uint32	headerFan;			/* Header used for fan commands */
    uint32	nrow, ncol;			/* Register versions of data structs*/
    uint32	irow;				/* loop index */
    uint32	icol, ivert;		/* indices */
    int32	ix0, ix, iy;		/* Actual coords of current block */
    int32	mat=0;				/* Current material */
    int32	lastMat;			/* Last material */

    gfloat	x, y, z, w;			/* Current vertex */
    gfloat	h;					/* Current height */
    gfloat	w1;					/* 1/w temp */
    gfloat	rx, ry, rz, rw;		/* Start of row */
    gfloat	Xx, Xy, Xw;			/* Delta for transform across row */
    gfloat	Yx, Yy, Yw;			/* Delta for transform down columns */
    gfloat	Zx, Zy, Zz, Zw;		/* Delta for transform up height */

    gfloat	u,v;				/* U,V at current vertex */
    gfloat	ru=0,rv=0;			/* U,V at start of current row */
    gfloat	uscale=0, vscale=0;	/* scale of current texture */

    Box3	*clipVol;			/* Box enclosing viewing frustrum*/
    gfloat  	clipMinX, clipMaxX;
    gfloat  	clipMinY, clipMaxY;
    gfloat  	clipMinZ, clipMaxZ;

    uint32	lastNverts;			/* Number of verts produced in
								   last command */
    uint32	*lastHeader = NULL;	/* Address of where last heade
								   was poked */

    {
		gfloat x30, x31, x32, x33;
		gfloat Xz, Yz;
		gfloat x0, y0;
		gfloat *f;

		f = &gp->m_TotalMatrix.data[0][0];

		Xx = *f++ * hf->deltax;
		Xy = *f++;
		Xz = *f++;
		Xw = *f++;
		Yx = *f++;
		Yy = *f++ * hf->deltay;
		Yz = *f++;
		Yw = *f++;
		Zx = *f++;
		Zy = *f++;
		Zz = *f++;
		Zw = *f++;
		x30 = *f++;
		x31 = *f++;
		x32 = *f++;
		x33 = *f;

		x0 = hf->xstart;
		y0 = hf->ystart;

		rx = x0*Xx + y0*Yx + x30;
		ry = x0*Xy + y0*Yy + x31;
		rz = x0*Xz + y0*Yz + x32;
		rw = x0*Xw + y0*Yw + x33;
    }

    /* Cache in a register */
    wscale = gp->m_WScale;

    /* setup these once */

    cpBuf1.vert = &cvBuf1[0];
    cpBuf2.vert = &cvBuf2[0];

    clipVol = &gp->m_ClipVol;
    clipMinX = clipVol->min.x;
    clipMaxX = clipVol->max.x;
    clipMinY = clipVol->min.y;
    clipMaxY = clipVol->max.y;
    clipMinZ = clipVol->min.z;
    clipMaxZ = clipVol->max.z;

    /* Place in local variable/register remember to update
       structure before leaving function */
	bytes_left = ((uint32)gp->m_GState->gs_EndList) -
				 ((uint32)gp->m_GState->gs_ListPtr);
	TE_out = (float*) gp->m_GState->gs_ListPtr;

    ncol = hf->rx;
    nrow = hf->ry;

    /* Check the temp storage to see if 2 rows of transformed vertices
       can fit */

    {
		uint32 tempSize;

		/* Make sure we have enough temp buffer allocated to transform
		   all the vertices. We may as well reserve enough space
		   for clipping always */
		tempSize = 2 * ncol * sizeof(ClipVertex);
		if (gp->m_TempBufferSize < tempSize) {
			(*gp->m_TempReallocateFunction)(gp->m_TempBuffer, tempSize,
											&gp->m_TempBuffer, &gp->m_TempBufferSize);
		}
    }

    header = CLA_TRIANGLE(1, RC_STRIP, 1, 1, 1, 1);
    headerFan = CLA_TRIANGLE(1, RC_FAN, 1, 1, 1, 1);

    ix0 = hf->xstart % hf->nx;
    if (ix0 < 0) ix0 += hf->nx;

    iy = hf->ystart % hf->ny;
    if (iy < 0) iy += hf->ny;

    pdata0 = hf->fdata + iy*hf->nx + ix0;

    /* First transform the first row into the temp buffer */
    thisRow = (ClipVertex*)gp->m_TempBuffer;
    lastRow = thisRow + ncol;

    /* Do 2 rows at a time up to 2nd to last row */
    for (irow = 1; irow < nrow; irow++) {

		/* Start out without any texture */
		lastMat = -1;

		x = rx; y = ry; z = rz; w = rw;
		ix = ix0;
		pdata = pdata0;

		/* Switch temp buffers */

		ctemp0 = thisRow;
		thisRow = lastRow;
		lastRow = ctemp0;

		/* Transform one row of vertices into thisRow */
		ctemp0 = thisRow;

		icol = ncol;
		while(icol-->0) {
			uint32 clip_code;
			gfloat tx, ty, tz, tw;

			h = pdata->h;
			ctemp0->x = tx = x+h*Zx;
			ctemp0->y = ty = y+h*Zy;
			ctemp0->z = tz = z+h*Zz;
			ctemp0->r = pdata->r*i255;
			ctemp0->g = pdata->g*i255;
			ctemp0->b = pdata->b*i255;
			ctemp0->a = pdata->a*i255;
			ctemp0->w = tw = w+h*Zw;

			ctemp0->mat1 = pdata->mat1;
			ctemp0->mat2 = pdata->mat2;

			clip_code = 0;

			if ((tx - clipMinX * tw) < 0.f)
			  clip_code |= PLANEX1;
			if ((clipMaxX * tw - tx) < 0.f)
			  clip_code |= PLANEX2;
			if ((ty - clipMinY * tw) < 0.f)
			  clip_code |= PLANEY1;
			if ((clipMaxY * tw - ty) < 0.f)
			  clip_code |= PLANEY2;
			if ((tz - clipMinZ * tw) < 0.f)
			  clip_code |= PLANEZ1;
			if ((clipMaxZ * tw - tz) < 0.f)
			  clip_code |= PLANEZ2;
			ctemp0->clip_code = clip_code;

			/* Move on to next column */
			x += Xx;
			y += Xy;
			w += Xw;

			ctemp0++;
			ix++;
			if (ix == ncol) {
				ix = 0;
				pdata = pdata0;
			} else {
				pdata++;
			}
		}

		/* First time through we don't have enough data to actually
		   put out a strip */
		if (irow==1)
			goto advance;

		ctemp0 = lastRow;
		ctemp1 = thisRow;

		/*
		   output all vertices by using ctemp0 and ctemp1

		   There are ncol-1 quads, each quad has 2 triangles:

		   ctemp0, ctemp0+1, ctemp0+2 . .
		   ctemp1, ctemp1+1, ctemp1+2 . .

		   Each triangle is clip tested. If it accumulated to be
		   later placed into the command list.
		   If it needs clipping, it is clipped and the surviving pieces
		   are placed into the command list

		   */
		/* No strips in progress */
		lastNverts = 0;

		/* icol and irow are used to index the texture scales */

		for (icol = 1; icol < ncol; icol++)  {

			uint32 twice = 2;
			/* first triangle */
			p0 = ctemp0;
			p1 = ctemp1;
			p2 = ctemp0+1;

			mat = p0->mat1;

			while (twice-->0) {
				uint32 someClipped;
				uint32 allClipped =  p0->clip_code & p1->clip_code & p2->clip_code;

				if (mat != lastMat) {
					HFMaterial *m;
					/* Don't accumulate any more */
					if (lastNverts != 0) {
						*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
						lastNverts = 0;
					}
					m = &hf->mattable[mat];

					/* start off the new material */

					uscale = m->uscale;
					vscale = m->vscale;
					ru = uscale*icol ;
					if (ru >= MAXU) ru -= MAXU;
					if (ru < uscale) {
						ru += hf->mattable[mat].usize;
					}
					rv = vscale*irow;
					if (rv >= MAXV) rv -= MAXV;
					if (rv < vscale)
					  rv += hf->mattable[mat].vsize;

					/* Output TE commands to setup texture registers for current texture
					   include 8 bytes for sync */
					bytes_needed = 4*(CLT_GetSize(&m->tabSnip)+CLT_GetSize(&m->useSnip))+8;
					if (bytes_left < bytes_needed) {
						GetMoreTEList(&TE_out, &bytes_left, gp);
					}
					CLT_Sync(((uint32**)&TE_out));
					CLT_CopySnippetData((uint32**)&TE_out, &m->tabSnip);
					CLT_CopySnippetData((uint32**)&TE_out, &m->useSnip);
					bytes_left -= bytes_needed;

				}

				if (allClipped) {
					/* Finish any in progress strip because one vertex is completly outside */
					if (lastNverts != 0) {
						*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));;
						lastNverts = 0;
					}
				} else {
					/* Finish any in progress strip because some clipping is going on */
					if (lastNverts != 0) {
						*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));;
						lastNverts = 0;
					}
					someClipped = p0->clip_code | p1->clip_code | p2->clip_code;
					if (someClipped) {
						/* there is a non-zero clip_code so
						   clip against half planes */

						cvBuf1[0] = *p0;
						cvBuf1[1] = *p1;
						cvBuf1[2] = *p2;
						cvBuf1[1].u = ru-uscale;
						cvBuf1[1].v = rv;
						cvBuf1[0].v = rv-vscale;
						cvBuf1[2].u = ru;
						if (twice==1) {
							cvBuf1[0].u = ru-uscale;
							cvBuf1[2].v = rv-vscale;
						} else {
							cvBuf1[0].u = ru;
							cvBuf1[2].v = rv;
						}

						cpBuf1.n = 3;
						clipBuf1 = &cpBuf1;
						clipBuf2 = &cpBuf2;

						/*
						 * clip against each of the planes that might cut the polygon,
						 * at each step toggling between polygons p1 and p2
						 */

						if (someClipped & PLANEX1) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 0, -1., clipMinX);
							SWAP(clipBuf1, clipBuf2, r);
						}
						if (someClipped & PLANEX2) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 0, 1., clipMaxX);
							SWAP(clipBuf1, clipBuf2, r);
						}
						if (someClipped & PLANEY1) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 1, -1., clipMinY);
							SWAP(clipBuf1, clipBuf2, r);
						}
						if (someClipped & PLANEY2) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 1, 1., clipMaxY);
							SWAP(clipBuf1, clipBuf2, r);
						}
						if (someClipped & PLANEZ1) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 2, -1., clipMinZ);
							SWAP(clipBuf1, clipBuf2, r);
						}
						if (someClipped & PLANEZ2) {
							PerPolyHalfPlaneClip(clipBuf1, clipBuf2, 2, 1., clipMaxZ);
							SWAP(clipBuf1, clipBuf2, r);
							TOUCH(clipBuf2);
						}

						if (clipBuf1->n > 2) {
							p = clipBuf1->vert;

							if (lastNverts > 0) {
								*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
								lastNverts  = 0;
							}
							/* Check space for enough space for this polygon */
							bytes_needed = clipBuf1->n * VERTEX_SIZE + 4;
							if (bytes_left < bytes_needed) {
								GetMoreTEList(&TE_out, &bytes_left, gp);
								/* Debug check */
								if (bytes_left < bytes_needed) {
									return; /* give up */
								}
							}
							ivert = clipBuf1->n;
							*((uint32*)TE_out++) = headerFan | CLT_Bits(TRIANGLE, COUNT, (ivert-1));
							while(ivert-->0) {
								outputVertClip1(p);
								p++;
							}
							bytes_left -= bytes_needed;
						}
					} else {
						/* All visible */

						/* Accumulate vertices that aren't clipped and output later */
						if (lastNverts == 0) {
							bytes_needed = 3*VERTEX_SIZE + 4;
						} else {
							bytes_needed = VERTEX_SIZE;
						}

						if (bytes_left < bytes_needed) {
							/* Patch in progress command */
							if (lastNverts != 0) {
								*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));;
								lastNverts = 0;
								bytes_needed = 3*VERTEX_SIZE + 4;
							}
							GetMoreTEList(&TE_out, &bytes_left, gp);
							/* Debug check */
							if (bytes_left < bytes_needed) {
								return; /* give up */
							}
						}

						/* commit these new bytes */

						if (lastNverts == 0) {
							lastHeader = (uint32*)TE_out++;
							lastNverts = 2;
							/* output p0 and p1 */
							v = rv-vscale;
							if (twice==1) {
								u = ru-uscale;
							} else {
								u = ru;
							}
							outputVertClip(p0);
							/* p1 is same for both triangles */
							u = ru-uscale;
							v = rv;
							outputVertClip(p1);
						}
						bytes_left -= bytes_needed;
						lastNverts++;
						u = ru;
						if (twice == 1) {
							v = rv-vscale;
						} else {
							v = rv;
						}
						outputVertClip(p2);
					}
				}

				/* Now do second triangle in quad */
				lastMat = mat;
				mat = p0->mat2;
				p0 = p2;
				p2 = ctemp1+1;
			}
			/* Do remaining vertices in row */

			/* Keep the integer value of our position in order
			   to switch textures */

			ix++;
			if (ix >= hf->nx) {
				ix -= hf->nx;
			}

			ru += uscale;
			if (ru >= MAXU) {
				ru -= MAXU;
				if (ru < uscale) {
					ru += hf->mattable[mat].usize;
				}
				/* End current strip because we wrapped */
				if (lastNverts != 0) {
					*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
					lastNverts = 0;
				}
			}
			ctemp0++;
			ctemp1++;
		}

		if (lastNverts != 0) {
			*lastHeader = header | CLT_Bits(TRIANGLE, COUNT, (lastNverts-1));
			lastNverts = 0;
			TOUCH(lastNverts);
		}
		/* Advance to next row */

		ru = uscale;
		rv += vscale;
		if (rv >= MAXV) {
			rv -= MAXV;
			if (rv < vscale)
			  rv += hf->mattable[mat].vsize;
		}

	advance:
		rx += Yx;
		ry += Yy;
		rw += Yw;

		iy++;
		if (iy == hf->ny) {
			iy = 0;
			pdata0 = hf->fdata + ix0;
		} else {
			pdata0 += hf->nx;
		}
    }
	gp->m_GState->gs_ListPtr = (CmdListP)TE_out;

#if PRINT
    if (printBuf) {
		uint32 idx = gp->m_GState->gs_WhichList;
		PrintCommandList(gp->m_GState->gs_TeRenderInfo[idx].ri_CmdList,
						 gp->m_GState->gs_EndList);
    }
#endif
}
