
/******************************************************************************
**
**  @(#) hf.c 96/05/01 1.17
**
**  Height field class object
**  This implementation is shared by the C and C++ bindings
**
******************************************************************************/

#include <graphics/fw.h>
#include "hf.h"
#include <stdio.h>

#define	This	((HFData*) this)

extern void gps_update_total_matrix(GPData *gp);
extern int32 Obj_NewClass(int32 base_type, uint32 class_size, char* class_name);

/* Requires user to call Obj_NewClass */
int32	GFX_HField;

Err HField_Display(HField* this, GP* gp, Texture** tex, MatProp* mat)
/****
 *
 * Display a height field
 *
 ****/
{
	int i;
	(void)tex;/*unused */
	(void)mat;/*unused*/

	for (i=0; i<This->nmats; i++) {
		GP_SetTexBlend(gp, This->mattable[i].txb);
	}

    if (!((GPData*) gp)->m_TransValid)
      gps_update_total_matrix((GPData *)gp);

    switch (GP_IsInView(gp, &(This->bound)))
      {
	case GFX_ClipIn:
	  optDrawHF((GPData*)gp, This);
	  break;

	case GFX_ClipPartial:
	  optDrawHFClip((GPData*)gp, This);
	  break;
      }

    return GFX_OK;
}

Err HField_CalcBound(HField* this, Box3* bound)
/****
 *
 * Get the bounding box for all the geometric objects in the script.
 *
 * Notes:
 * For height fields, the bounding box is computed once
 * and never changes.
 *
 ****/
{
	*bound = This->bound;
	return GFX_OK;
}

void HField_PrintInfo(const HField* this)
/****
 *
 * Print a description of the surface to standard output
 *
 ****/
{
    printf("height field Nrows %d Ncols %d \n",This->nx, This->ny);
    /* Sanity check Make sure we have all the tokens we need to fill in
       the surface */
	{
		int 		ir, ic;
		HFPacket 	*p = This->fdata;
		for (ir=0; ir<This->ny; ir++) {
			printf("Row %d:\n",ir);
			for (ic=0; ic<This->nx; ic++) {
				printf("%8.4f %d %d %d %d\n",p->h, p->r, p->g, p->b);
			}
		}
    }
}

static void HField_DeleteAttrs(GfxObj* o)
{
	(void)o;/*unused*/
}

void
InitHFieldClass()
{
	extern Err surf_Construct(GfxObj*);

    GFX_HField = Obj_NewClass(GFX_Surface, sizeof (HFData), "HeightField");
	Obj_FuncConstruct(GFX_HField, surf_Construct);
	Obj_FuncDestroy(GFX_HField, HField_DeleteAttrs);
	Obj_FuncPrint(GFX_HField, HField_PrintInfo);
	Surf_FuncDisplay(GFX_HField, HField_Display);
	Surf_FuncCalcBound(GFX_HField, HField_CalcBound);
}

void
HField_ComputeBound(HFData *field)
{
#define INF 10e30
    /* Find the actual min and max of the data */

    float minz;
    float maxz;
    int i,j;
    HFPacket *p, *pRow;
    int x,y;

    minz = INF;
    maxz = -INF;

    x = field->xstart % field->nx;
    y = field->ystart % field->ny;

    j = field->ry;
    while(j-->0) {
		if (y >= field->ny)
		  y %= field->ny;

		if (y < 0)
		  y += field->ny;

		pRow = field->fdata+field->nx*y;

		i = field->rx;
		while(i-->0) {
			if (x >= field->nx)
			  x %= field->nx;

			if (x < 0)
			  x += field->nx;

			p = pRow + x;
			if (p->h < minz) minz = p->h;
			if (p->h > maxz) maxz = p->h;

			x++;
		}
		y++;
    }

    Box3_Set(&field->bound,
			 field->xstart * field->deltax,
			 field->ystart * field->deltay,
			 minz,
			 field->rx * field->deltax,
			 field->ry * field->deltay,
			 maxz);
}

