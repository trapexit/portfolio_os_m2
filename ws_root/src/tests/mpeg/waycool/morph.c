#include "mm.h"

Err
initMorph(ObjDescr *morph, uint32 num_blocks, uint32 hsegments, uint32 vsegments, uint32 tx_xsize, uint32 tx_ysize, uint32 tx_depth)
{
	Point3		*vptr;
	Vector3		*nptr;
	TexCoord	*tptr;

	gfloat 		x0, y0, x1, y1, yoff0, yoff1;
	gfloat		hsegsize, vsegsize;
	uint32		x, y, i;
	Err			err;
	uint32      tx_per_word;

	tx_per_word = 32 / tx_depth;

/*	printf("...tx_depth = %d tx_per_word = %d\n", tx_depth, tx_per_word); */

	morph->num_blocks = num_blocks;
	morph->tx_xsize = tx_xsize;
	morph->tx_ysize = tx_ysize;
	morph->vsegments = vsegments;
	morph->hsegments = hsegments;
	morph->num_verts = ((hsegments + 1) * 2);
	morph->num_txwords = (tx_xsize * (tx_ysize + 3)) / tx_per_word;
	morph->hiddenSurf = GP_ZBuffer;
	morph->cullFaces = GP_None;

	hsegsize = ((gfloat) tx_xsize) / ((gfloat) hsegments);
	vsegsize = ((gfloat) tx_ysize) / ((gfloat) vsegments);

/*	printf("...allocating data structures\n"); */
	err = objCreateAlloc(morph);
	if (err < 0) return (err);

/*	printf("...creating geometry\n");
	Geo_CreateData(&morph_ts, GEO_TriStrip, GEO_Normals | GEO_TexCoords, morph->num_verts);
*/
/*	printf("...creating plane structure\n"); */

	vptr = morph->vtxData;
	nptr = morph->normData;
	
	for (i = 0; i < num_blocks; i++) {
		for (y = 0; y < vsegments; y++) {
			yoff0 = -0.6 + ((gfloat) i) * 1.2 / ((gfloat) num_blocks);
			yoff1 = -0.6 + ((gfloat) (i + 1)) * 1.2 / ((gfloat) num_blocks);
			for (x = 0; x <= hsegments; x++) {
				vptr->x = 0.8 - ((gfloat) x) * 1.6 / ((gfloat) hsegments);
				vptr->y = yoff0;
				vptr->z = 0.0;
				nptr->x = 0.0;
				nptr->y = 0.0;
				nptr->z = -1.0;
				Vec3_Normalize(nptr);
				vptr++;
				nptr++;
				
				vptr->x = 0.8 - ((gfloat) x) * 1.6 / ((gfloat) hsegments);
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

/*	printf("...associating texture coordinates\n"); */
	y0 = 1;
/*	vnum = 0; */
	tptr = morph->texData;
	for (y = 0; y < vsegments; y++) {
		y1 = y0 + vsegsize;
		x0 = 1;
		for (x = 0; x <= hsegments; x++) {
			x1 = x0 + hsegsize;
			tptr->u = x0;
			tptr->v = y0;
			tptr++;
/*			morph_ts.TexCoords[vnum++] = *tptr++; */
			tptr->u = x0;
			tptr->v = y1;
			tptr++;
/*			morph_ts.TexCoords[vnum++] = *tptr++; */
			x0 = x1;
		}
		y0 = y1;
	}

/*	printf("...creating surface\n"); */

	err = objCreateAddSurface(morph);
	if (err < 0) return (err);

	morph->material.ShadeEnable = /*MAT_Specular |*/ MAT_Diffuse | MAT_Ambient;
	Col_Set(&morph->material.Ambient,  0.01, 0.01, 0.01, 1.0);
	Col_Set(&morph->material.Diffuse,  0.9, 0.9, 0.9, 1.0);
	Col_Set(&morph->material.Specular, 0.5, 0.5, 0.5, 1.0);
	morph->material.Shine = 0.05;

	return(APP_OK);

}

#define interp(s, e, p) ( s + ((e - s) * p) )

Err
morphObject(ObjDescr *from, ObjDescr *to, ObjDescr *morph, uint32 nsteps, uint32 step)
{
	Point3		*f_vptr, *t_vptr;
	Vector3		*f_nptr, *t_nptr;
	int32		i, j, k;
	Geometry    geo;
	gfloat      progress;

	f_vptr = from->vtxData;
	f_nptr = from->normData;
	t_vptr = to->vtxData;
	t_nptr = to->normData;

	progress = ((gfloat) step) / ((gfloat) nsteps);
	for (i = 0; i < morph->num_blocks; i++) {
		for (k = 0; k < morph->vsegments; k++) {
			Surf_FindGeometry(morph->surf, i * morph->vsegments + k, &geo);
			for (j = 0; j < morph->num_verts; j++) {

				geo.Locations[j].x = interp( f_vptr->x, t_vptr->x, progress );
				geo.Locations[j].y = interp( f_vptr->y, t_vptr->y, progress );
				geo.Locations[j].z = interp( f_vptr->z, t_vptr->z, progress );
				
				geo.Normals[j].x = interp( f_nptr->x, t_nptr->x, progress );
				geo.Normals[j].y = interp( f_nptr->y, t_nptr->y, progress );
				geo.Normals[j].z = interp( f_nptr->z, t_nptr->z, progress );

				/* this slows us down and it looks like we can skip it, so... */
				/*		Vec3_Normalize(&geo.Normals[j]);  */
				
				f_vptr++;
				f_nptr++;
				t_vptr++;
				t_nptr++;
			}
		}
	}
/*	Surf_Touch(morph->surf);  - don't seem to need this, either...*/

	return(APP_OK);
}














