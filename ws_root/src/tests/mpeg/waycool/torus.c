#include "mm.h"

#define THETAMAX    (2.0 * PI)
#define THETAMIN    (0.0)
#define PHIMAX      (PI)
#define PHIMIN      (- PI)

#define R1 			(1.0)
#define R2 			(0.5)

Err
createTorus(ObjDescr *tor, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth)
{
	Point3		*vptr;
	TexCoord	*tptr;
	Vector3		*nptr;

	Point3		*vptr0;
	Vector3		*nptr0;

	gfloat		irad, orad, crad, rad, theta0, theta1, phi;
	gfloat 		x0, y0, x1, y1;
	gfloat		hsegsize, vsegsize;
	uint32		i, x, y;
	Err			err;
	uint32      tx_per_word;

	tx_per_word = 32 / tx_depth;

	tor->num_blocks = num_blocks;
	tor->tx_xsize = tx_xsize;
	tor->tx_ysize = tx_ysize;
	tor->vsegments = vsegments;
	tor->hsegments = hsegments;
	tor->num_verts = ((hsegments + 1) * 2);
	tor->num_txwords = (tx_xsize * (tx_ysize + 3) / tx_per_word);
	tor->hiddenSurf = GP_ZBuffer;
	tor->cullFaces = GP_Back;

	hsegsize = ((gfloat) tx_xsize) / ((gfloat) hsegments);
	vsegsize = ((gfloat) tx_ysize) / ((gfloat) vsegments);

	err = objCreateAlloc(tor);
	if (err < 0) return (err);

	vptr = tor->vtxData;
	nptr = tor->normData;

	orad = R1;
	irad = R2;
	rad = (orad - irad) / 2.0;
	crad = irad + rad;

	for (i = 0; i < num_blocks; i++) {
		for (y = 0; y < vsegments; y++) {
			theta0 = THETAMAX -
				( (gfloat) (i * vsegments + y)) *
					((THETAMAX - THETAMIN) / ((gfloat)(num_blocks * vsegments))
			);
			theta1 = THETAMAX - 
				( (gfloat) (i * vsegments + y + 1)) *
					((THETAMAX - THETAMIN) / ((gfloat)(num_blocks * vsegments))
			);
			
			y0 = rad * sinf(theta0);
			y1 = rad * sinf(theta1);
		   
			/* remember where first two vertices are */
			vptr0 = vptr;
			nptr0 = nptr;

			for (x = 0; x < hsegments; x++) {
				phi = PHIMAX - (((gfloat) x) * (PHIMAX - PHIMIN) / ((gfloat) hsegments));

				vptr->x = (crad - rad * cosf(theta0)) * sinf(phi);
				vptr->y = y0;
				vptr->z = (crad - rad * cosf(theta0)) * cosf(phi);
				nptr->x = vptr->x - crad * sinf(phi);
				nptr->y = vptr->y;
				nptr->z = vptr->z - crad * cosf(phi);
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;

				vptr->x = (crad - rad * cosf(theta1)) * sinf(phi);
				vptr->y = y1;
				vptr->z = (crad - rad * cosf(theta1)) * cosf(phi);
				nptr->x = vptr->x - crad * sinf(phi);
				nptr->y = vptr->y;
				nptr->z = vptr->z - crad * cosf(phi);
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
	tptr = tor->texData;
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

	err = objCreateAddSurface(tor);
	if (err < 0) return (err);

	tor->material.ShadeEnable = /* MAT_Specular | */ MAT_Diffuse | MAT_Ambient;
	Col_Set(&tor->material.Ambient,  0.01, 0.01, 0.01, 1.0);
	Col_Set(&tor->material.Diffuse,  0.9, 0.9, 0.9, 1.0);
	Col_Set(&tor->material.Specular, 0.5, 0.5, 0.5, 1.0);
	tor->material.Shine = 0.05;

	return(APP_OK);

}

