#include "mm.h"

#define R 			0.6
#define THETAMIN	(0.0)
#define THETAMAX	(PI)
#define PHIMIN		(-PI / 2.0)
#define PHIMAX		((PI * 2.0) - (PI / 2.0))

Err
createTube(ObjDescr *obj, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth)
{
	Point3		*vptr, *vptr0;
	TexCoord	*tptr;
	Vector3		*nptr, *nptr0;

	gfloat		phi;
	gfloat 		x0, y0, x1, y1;
	gfloat		hsegsize, vsegsize;
	uint32		i, x, y;
	Err			err;
	uint32      tx_per_word;

	tx_per_word = 32 / tx_depth;

	obj->num_blocks = num_blocks;
	obj->tx_xsize = tx_xsize;
	obj->tx_ysize = tx_ysize;
	obj->vsegments = vsegments;
	obj->hsegments = hsegments;
	obj->num_verts = ((hsegments + 1) * 2);
	obj->num_txwords = (tx_xsize * (tx_ysize + 3) / tx_per_word);
	obj->hiddenSurf = GP_ZBuffer;
	obj->cullFaces = GP_None;
	hsegsize = ((gfloat) tx_xsize) / ((gfloat) hsegments);
	vsegsize = ((gfloat) tx_ysize) / ((gfloat) vsegments);

	err = objCreateAlloc(obj);
	if (err < 0) return (err);

	vptr = obj->vtxData;
	nptr = obj->normData;
	
	for (i = 0; i < num_blocks; i++) {
		for (y = 0; y < vsegments; y++) {
			vptr0 = vptr;
			nptr0 = nptr;
			y0 = -0.8 + ((gfloat) (i * vsegments + y)) * 1.6 / ((gfloat) (num_blocks * vsegments));
			y1 = -0.8 + ((gfloat) (i * vsegments + y + 1)) * 1.6 / ((gfloat) (num_blocks * vsegments));

			for (x = 0; x < hsegments; x++) {
				phi = PHIMIN +
					(((gfloat) x) * (PHIMAX - PHIMIN) / ((gfloat) hsegments));

				vptr->x = R * cosf(phi);
				vptr->y = y0;
				vptr->z = R * sinf(phi);
				nptr->x = vptr->x;
				nptr->y = 0.0;
				nptr->z = vptr->z;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;

				vptr->x = R * cosf(phi);
				vptr->y = y1;
				vptr->z = R * sinf(phi);
				nptr->x = vptr->x;
				nptr->y = 0.0;
				nptr->z = vptr->z;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;
			}

			vptr->x = vptr0->x;
			vptr->y = vptr0->y;
			vptr->z = vptr0->z;
			nptr->x = nptr0->x;
			nptr->y = nptr0->y;
			nptr->z = nptr0->z;
			vptr++;
			nptr++;
			vptr0++;
			nptr0++;
			vptr->x = vptr0->x;
			vptr->y = vptr0->y;
			vptr->z = vptr0->z;
			nptr->x = nptr0->x;
			nptr->y = nptr0->y;
			nptr->z = nptr0->z;
			vptr++;
			nptr++;
		}
	}

	y0 = 1;
	tptr = obj->texData;
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

	err = objCreateAddSurface(obj);
	if (err < 0) return (err);

	obj->material.ShadeEnable = /*MAT_Specular |*/ MAT_Diffuse | MAT_Ambient;
	Col_Set(&obj->material.Ambient,  0.01, 0.01, 0.01, 1.0);
	Col_Set(&obj->material.Diffuse,  0.9, 0.9, 0.9, 1.0);
	Col_Set(&obj->material.Specular, 0.5, 0.5, 0.5, 1.0);
	obj->material.Shine = 0.05;

	return(APP_OK);

}

