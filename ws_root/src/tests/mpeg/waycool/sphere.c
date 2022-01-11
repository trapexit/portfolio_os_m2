#include "mm.h"

#define R 			1.0
#define THETAMIN	(0.0)
#define THETAMAX	(PI)
#define PHIMIN		(-PI / 2.0)
#define PHIMAX		((PI * 2.0) - (PI / 2.0))

Err
createSphere(ObjDescr *sph, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth)
{
	Point3		*vptr, *vptr0;
	TexCoord	*tptr;
	Vector3		*nptr, *nptr0;

	gfloat		phi, theta0, theta1;
	gfloat 		rsintheta0, rcostheta0;
	gfloat 		rsintheta1, rcostheta1;
	gfloat 		x0, y0, x1, y1;
	gfloat		hsegsize, vsegsize;
	uint32		i, x, y;
	Err			err;
	uint32      tx_per_word;

	tx_per_word = 32 / tx_depth;

/*	printf("tx_depth = %d tx_per_word = %d\n", tx_depth, tx_per_word); */

	sph->num_blocks = num_blocks;
	sph->tx_xsize = tx_xsize;
	sph->tx_ysize = tx_ysize;
	sph->vsegments = vsegments;
	sph->hsegments = hsegments;
	sph->num_verts = ((hsegments + 1) * 2);
	sph->num_txwords = (tx_xsize * (tx_ysize + 3) / tx_per_word);
	sph->hiddenSurf = GP_None;
	sph->cullFaces = GP_Back;
	hsegsize = ((gfloat) tx_xsize) / ((gfloat) hsegments);
	vsegsize = ((gfloat) tx_ysize) / ((gfloat) vsegments);

	err = objCreateAlloc(sph);
	if (err < 0) return (err);

	vptr = sph->vtxData;
	nptr = sph->normData;
	
	for (i = 0; i < num_blocks; i++) {
		for (y = 0; y < vsegments; y++) {
			theta0 = THETAMAX - (
				(gfloat) (i * vsegments + y)) *
				((THETAMAX - THETAMIN) / ((gfloat)(num_blocks * vsegments))
			);
			theta1 = THETAMAX - (
				(gfloat) (i * vsegments + y + 1)) *
				((THETAMAX - THETAMIN) / ((gfloat)(num_blocks * vsegments))
			);

			rsintheta0 = R * sinf(theta0);
			rcostheta0 = R * cosf(theta0);
			rsintheta1 = R * sinf(theta1);
			rcostheta1 = R * cosf(theta1);
			
			vptr0 = vptr;
			nptr0 = nptr;

			for (x = 0; x < hsegments; x++) {
				phi = PHIMIN +
					(((gfloat) x) * (PHIMAX - PHIMIN) / ((gfloat) hsegments));

				vptr->x = rsintheta0 * cosf(phi);
				vptr->y = rcostheta0;
				vptr->z = rsintheta0 * sinf(phi);
				nptr->x = vptr->x;
				nptr->y = vptr->y;
				nptr->z = vptr->z;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;

				vptr->x = rsintheta1 * cosf(phi);
				vptr->y = rcostheta1;
				vptr->z = rsintheta1 * sinf(phi);
				nptr->x = vptr->x;
				nptr->y = vptr->y;
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
	tptr = sph->texData;
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

	err = objCreateAddSurface(sph);
	if (err < 0) return (err);

	sph->material.ShadeEnable = /* MAT_Specular | */ MAT_Diffuse | MAT_Ambient;
	Col_Set(&sph->material.Ambient,  0.01, 0.01, 0.01, 1.0);
	Col_Set(&sph->material.Diffuse,  0.9, 0.9, 0.9, 1.0);
	Col_Set(&sph->material.Specular, 0.5, 0.5, 0.5, 1.0);
	sph->material.Shine = 0.05;

	return(APP_OK);
}



