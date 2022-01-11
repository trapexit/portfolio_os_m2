#include "mm.h"

#define PLANE_WIDTH 1.6
#define PLANE_HEIGHT 1.2

Err
createPlane(ObjDescr *pln, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth)
{
	Point3		*vptr;
	TexCoord	*tptr;
	Vector3		*nptr;

	gfloat 		x0, y0, x1, y1;
	gfloat		hsegsize, vsegsize;
	gfloat		yoff0, yoff1;
	uint32		i, x, y;
	Err			err;
	uint32      tx_per_word;

	tx_per_word = 32 / tx_depth;

	pln->num_blocks = num_blocks;
	pln->tx_xsize = tx_xsize;
	pln->tx_ysize = tx_ysize;
	pln->vsegments = vsegments;
	pln->hsegments = hsegments;
	pln->num_verts = ((hsegments + 1) * 2);
	pln->num_txwords = (tx_xsize * (tx_ysize + 3) / tx_per_word);
	pln->hiddenSurf = GP_None;
	pln->cullFaces = GP_None;

	hsegsize = ((gfloat) tx_xsize) / ((gfloat) hsegments);
	vsegsize = ((gfloat) tx_ysize) / ((gfloat) vsegments);

	err = objCreateAlloc(pln);
	if (err < 0) return (err);

	vptr = pln->vtxData;
	nptr = pln->normData;
	
	for (i = 0; i < num_blocks; i++) {
		for (y = 0; y < vsegments; y++) {
			yoff0 = -(PLANE_HEIGHT / 2) + ((gfloat) (i * vsegments + y)) * PLANE_HEIGHT / ((gfloat) (num_blocks * vsegments));
			yoff1 = -(PLANE_HEIGHT / 2) + ((gfloat) (i * vsegments + y + 1)) * PLANE_HEIGHT / ((gfloat) (num_blocks * vsegments));
			for (x = 0; x <= hsegments; x++) {
				vptr->x = (PLANE_WIDTH / 2) - ((gfloat) x) * PLANE_WIDTH / ((gfloat) hsegments);
				vptr->y = yoff0;
				vptr->z = 0.0;
				nptr->x = 0.0;
				nptr->y = 0.0;
				nptr->z = -1.0;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;
				
				vptr->x = (PLANE_WIDTH / 2) - ((gfloat) x) * PLANE_WIDTH / ((gfloat) hsegments);
				vptr->y = yoff1;
				vptr->z = 0.0;
				nptr->x = 0.0;
				nptr->y = 0.0;
				nptr->z = -1.0;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;
			}
		}
	}
	y0 = 1;
	tptr = pln->texData;
	for (y = 0; y < vsegments; y++) {
		y1 = y0 + vsegsize;
		x0 = 1;
		for (x = 0; x <= hsegments; x++) {
			x1 = x0 + hsegsize;
			tptr->u = x0;
			tptr->v = y0;
			tptr++;
			tptr->u = x0;
			tptr->v = y1;
			tptr++;
			x0 = x1;
		}
		y0 = y1;
	}

	err = objCreateAddSurface(pln);
	if (err < 0) return (err);

	pln->material.ShadeEnable = /*MAT_Specular |*/ MAT_Diffuse | MAT_Ambient;
	Col_Set(&pln->material.Ambient,  0.01, 0.01, 0.01, 1.0);
	Col_Set(&pln->material.Diffuse,  0.9, 0.9, 0.9, 1.0);
	Col_Set(&pln->material.Specular, 0.5, 0.5, 0.5, 1.0);
	pln->material.Shine = 0.05;

	return(APP_OK);
}



