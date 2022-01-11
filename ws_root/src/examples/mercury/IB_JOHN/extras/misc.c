/*
 *
 *
 */
#include "misc.h"

/*
 *
 */
void GState_SetZBufferOff( GState *gs )
{
	gs->gs_SendList(gs);
	/*
	 * Turn OFF Z-Buffering
	 */
	GS_WaitIO(gs);

	GS_Reserve( gs, 4 );
	CLT_Sync(GS_Ptr(gs));
	CLT_ClearRegister( gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZBUFFEN,1) );
	CLT_ClearRegister( gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZOUTEN,1) );
}

/*
 *
 */
void GState_SetZBufferOn( GState *gs )
{
	/*
	 * Turn ON Z-Buffering
	 */
	GS_Reserve( gs, 4 );
	CLT_Sync(GS_Ptr(gs));
	CLT_SetRegister( gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZBUFFEN,1) );
	CLT_SetRegister( gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,ZOUTEN,1) );
}

#if 0
/*
 *
 */
void BBox_LocalToWorld( BBox* world, Matrix* mtx, BBox* local )
{
    Vector3D fmax;

    world->fxmin = (local->fxmin * mtx->mat[0][0] +
		    local->fymin * mtx->mat[1][0] +
		    local->fzmin * mtx->mat[2][0] +
		    mtx->mat[3][0]);
    world->fymin = (local->fxmin * mtx->mat[0][1] +
		    local->fymin * mtx->mat[1][1] +
		    local->fzmin * mtx->mat[2][1] +
		    mtx->mat[3][1]);
    world->fzmin = (local->fxmin * mtx->mat[0][2] +
		    local->fymin * mtx->mat[1][2] +
		    local->fzmin * mtx->mat[2][2] +
		    mtx->mat[3][2]);
    fmax.x = ((local->fxmin + local->fxextent) * mtx->mat[0][0] +
	      (local->fymin + local->fyextent) * mtx->mat[1][0] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][0] +
	      mtx->mat[3][0]);
    fmax.y = ((local->fxmin + local->fxextent) * mtx->mat[0][1] +
	      (local->fymin + local->fyextent) * mtx->mat[1][1] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][1] +
	      mtx->mat[3][1]);
    fmax.z = ((local->fxmin + local->fxextent) * mtx->mat[0][2] +
	      (local->fymin + local->fyextent) * mtx->mat[1][2] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][2] +
	      mtx->mat[3][2]);
    world->fxextent = fmax.x - world->fxmin;
    world->fyextent = fmax.y - world->fymin;
    world->fzextent = fmax.z - world->fzmin;
}
#endif

/*
 *
 */
void BBox_Print( BBox *bbox )
{
	printf( "BBox:\n" );
	printf( "Min( %3.3f, %3.3f, %3.3f ) - Size( W:%3.3f, H:%3.3f, D:%3.3f )\n",
				bbox->fxmin, bbox->fymin, bbox->fzmin,
				bbox->fxextent, bbox->fyextent, bbox->fzextent );
}

/*
 *
 */
void PodGeometry_Print(PodGeometry *geo)
{
	printf( "PodGeometry:\n" );
	printf( "Bound Min     ->\n" );
	Vector3D_Print( (Vector3D*)&geo->fxmin );
	printf( "Bound Extent  ->\n" );
	Vector3D_Print( (Vector3D*)&geo->fxextent );
	printf( "pvertex       ->\n" );
	printf( "\n" );
	Vertices_Print( geo->pvertex, geo->vertexcount );
	printf( "\n" );
	printf( "pshared       %d\n", geo->pshared );
 	printf( "vectexcount   %d\n", geo->vertexcount );
 	printf( "sharedcount   %d\n", geo->sharedcount );
	printf( "pindex        ->\n" );
	printf( "\n" );
	printf( "indicescount  %d\n", Indices_GetCount(geo->pindex));
	Indices_Print(geo->pindex, Indices_GetCount(geo->pindex));
	printf( "\n" );
	printf( "puv           ->\n" );
	printf( "\n" );
	UV_Print(geo->puv, Indices_GetCount(geo->pindex));
	printf( "\n" );
}

/*
 *
 */
void Color4_Print(Color4 *color4)
{
	printf( "Color4: " );
	printf( "( Red: %3.3f, Green: %3.3f, Blue: %3.3f, Alpha: %3.3f )\n",
				color4->r, color4->g, color4->b, color4->a );
}

/*
 *
 */
void Color3_Print(Color3 *color3)
{
	printf( "Color3: " );
	printf( "( Red: %3.3f, Green: %3.3f, Blue: %3.3f )\n",
				color3->r, color3->g, color3->b );
}

/*
 *
 */
void Indices_Print(short *indices, uint32 count)
{
	uint32		i;

	printf( "Indices:\n" );
	for ( i=0; i<count; i++ ) {
		if ( (i & 15)==0 ) {
			printf( "\nIndex: " );
		}
		printf("%x, ",indices[i]);
	}
	printf( "\n" );
}

/*
 *
 */
uint32 Indices_GetCount(short *indices)
{
	uint32		i;

	i = 0;
	while ( indices[i] != -1 ) {
		i++;
	}
	return i;
}

/*
 *
 */
void UV_Print(float *uvs, uint32 count)
{
	uint32		i;

	if ( uvs == NULL ) {
		printf( "NULL\n" );
	} else {
		printf( "UVs:\n" );
		for ( i=0; i<(count*2); i+=2 ) {
			printf( "UV[ %3.3f, %3.3f ]\n", uvs[i+0], uvs[i+1] );
		}
		printf( "\n" );
	}
}

/*
 *
 */
void Vertices_Print(float *vertices, uint32 count)
{
	uint32		i;

	printf( "Vertices:\n" );
	for ( i=0; i<(count*6); i+=6 ) {
		printf("[%d] v[ %3.3f, %3.3f, %3.3f ] c[ %3.3f, %3.3f, %3.3f ]\n", i/6,
			vertices[i+0], vertices[i+1], vertices[i+2],
			vertices[i+3], vertices[i+4], vertices[i+5] );
	}
}



/* End of File */
