/*
 *
 *
 */
#include "pod.h"

/*
 *
 */
void Pod_ReplaceSetup( Pod *self, uint32 nPods, void *replace, void *with )
{
	uint32		i;

	for ( i=0; i < nPods; i++ ) {
		if ( self->pcase == replace ) {
			self->pcase = with;
		}
		self = mPod_GetNext( self );
		if ( self == NULL ) {
			return;
		}
	}
}

/*
 *
 */
void Pod_SetLight( Pod *self, uint32 nPods, uint32* lights )
{
    uint32        	i;
	Pod				*old = self;

	if ( nPods <= 0 )
		return;

	for ( i=0; i < nPods; i++ ) {

		self->plights = lights;

		self = mPod_GetNext( self );
		if ( self == NULL ) {
			return;
		}
	}
}

/*
 *
 */
void Pod_SetCase( Pod *self, uint32 nPods, void* pcase )
{
    uint32        	i;

	if ( nPods <= 0 )
		return;

	for ( i=0; i < nPods; i++ ) {

		self->pcase = pcase;

		self = self->pnext;
		if ( self == NULL ) {
			return;
		}
	}
}

/*
 *
 */
void Pod_SetFlags( Pod *self, uint32 nPods, uint32 flags )
{
    uint32        	i;

	if ( nPods <= 0 )
		return;

	for ( i=0; i < nPods; i++ ) {

		self->flags |= flags;

		self= mPod_GetNext( self );
		if ( self == NULL ) {
			return;
		}
	}
}

/*
 *
 */
void Pod_ClearFlags( Pod *pod, uint32 nPods, uint32 flags )
{
    uint32        	i;

	if ( nPods <= 0 )
		return;

	for ( i=0; i < nPods; i++ ) {

		pod->flags &= ~flags;

		pod = mPod_GetNext( pod );
		if ( pod == NULL ) {
			return;
		}
	}
}


/*
 *
 */
void Pod_CalcBoundingBox( Pod *pod, uint32 num_pods, BBox *bbox )
{
	uint32		i;
	Pod			*tmp;

	bbox->fxmin = 0.0f;
	bbox->fymin = 0.0f;
	bbox->fzmin = 0.0f;
	bbox->fxextent = 0.0f;
	bbox->fyextent = 0.0f;
	bbox->fzextent = 0.0f;

	if ( num_pods <= 0 )
		return;

	tmp = pod;

	bbox->fxmin = tmp->pgeometry->fxmin;
	bbox->fymin = tmp->pgeometry->fymin;
	bbox->fzmin = tmp->pgeometry->fzmin;

	bbox->fxextent = tmp->pgeometry->fxextent;
	bbox->fyextent = tmp->pgeometry->fyextent;
	bbox->fzextent = tmp->pgeometry->fzextent;

	tmp = tmp->pnext;
	if ( tmp == NULL ) {
		return;
	}
	for( i = 1; i < num_pods; i++ ) {
		if( bbox->fxmin > tmp->pgeometry->fxmin )
			bbox->fxmin = tmp->pgeometry->fxmin;
		if( bbox->fymin > tmp->pgeometry->fymin )
			bbox->fymin = tmp->pgeometry->fymin;
		if( bbox->fzmin > tmp->pgeometry->fzmin )
			bbox->fzmin = tmp->pgeometry->fzmin;

		if( bbox->fxextent < tmp->pgeometry->fxextent )
			bbox->fxextent = tmp->pgeometry->fxextent;
		if( bbox->fyextent < tmp->pgeometry->fyextent )
			bbox->fyextent = tmp->pgeometry->fyextent;
		if( bbox->fzextent < tmp->pgeometry->fzextent )
			bbox->fzextent = tmp->pgeometry->fzextent;

		tmp = tmp->pnext;
		if ( tmp == NULL ) {
			return;
		}
	}
}

#if 0
/*
 *
 */
void Pod_CalcBoundingBox( Pod* firstPod, int32 podCount, BBox* bbox )
{
    BBox     curBBox;
    Vector3D curMax, bboxMax;
    Pod*     curPod;
    uint32   i;

    curPod = (Pod*)firstPod;
    BBox_LocalToWorld( bbox, curPod->pmatrix, (BBox*)&(curPod->pgeometry->fxmin) );
    bboxMax.x = bbox->fxmin + bbox->fxextent;
    bboxMax.y = bbox->fymin + bbox->fyextent;
    bboxMax.z = bbox->fzmin + bbox->fzextent;

    for (i=1; i<podCount; i++) {
		curPod = curPod->pnext;
    	BBox_LocalToWorld( &curBBox, curPod->pmatrix, (BBox*)&(curPod->pgeometry->fxmin) );
		curMax.x = curBBox.fxmin + curBBox.fxextent;
		curMax.y = curBBox.fymin + curBBox.fyextent;
		curMax.z = curBBox.fzmin + curBBox.fzextent;

		if (curBBox.fxmin < bbox->fxmin) {
	    	bbox->fxmin = curBBox.fxmin;
		}
		if (curMax.x > bboxMax.x) {
	    	bboxMax.x = curMax.x;
		}
		if (curBBox.fymin < bbox->fymin) {
	    	bbox->fymin = curBBox.fymin;
		}
		if (curMax.y > bboxMax.y) {
	    	bboxMax.y = curMax.y;
		}
		if (curBBox.fzmin < bbox->fzmin) {
	    	bbox->fzmin = curBBox.fzmin;
		}
		if (curMax.z > bboxMax.z) {
	    	bboxMax.z = curMax.z;
		}
    }
    bbox->fxextent = bboxMax.x - bbox->fxmin;
    bbox->fyextent = bboxMax.y - bbox->fymin;
    bbox->fzextent = bboxMax.z - bbox->fzmin;
}
#endif

/*
 *
 */
void Pod_Scale( Pod* pod, uint32 num_pods, float scale )
{
    uint32        	i, j;
    PodGeometry*  	pgeo;
    float*        	pvtx;

	if ( num_pods <= 0 )
		return;

	for ( j=0; j<num_pods; j++ ) {

    	pgeo = pod->pgeometry;
    	pvtx = pgeo->pvertex;

    	for (i=0; i<pgeo->vertexcount; i++) {
			pvtx[i*6 + 0] *= scale;      /* X */
			pvtx[i*6 + 1] *= scale;      /* Y */
			pvtx[i*6 + 2] *= scale;      /* Z */
    	}

    	/*
	 	 * Scale the bounding box
	 	 */
    	pgeo->fxmin *= scale;
    	pgeo->fymin *= scale;
    	pgeo->fzmin *= scale;

    	pgeo->fxextent *= scale;
    	pgeo->fyextent *= scale;
    	pgeo->fzextent *= scale;

		pod = mPod_GetNext( pod );
		if ( pod == NULL ) {
			return;
		}
	}
}


/*
 *
 */
void Pod_Print(Pod *pod)
{
	printf( "Pod:\n" );
	printf( "Flags      %d\n", pod->flags );
	printf( "pnext      %d\n", pod->pnext );
	printf( "pcase      %d\n", pod->pcase );
	printf( "ptexture   %d\n", pod->ptexture );
	printf( "pgeometry  ->\n" );
	printf( "\n" );
	PodGeometry_Print( pod->pgeometry );
	printf( "\n" );
	printf( "pmatrix    ->\n" );
	printf( "\n" );
	Matrix_Print( pod->pmatrix );
	printf( "\n" );
	printf( "plights    ->\n" );
	printf( "\n" );
	Lights_Print( (Lights*)pod->plights );
	printf( "\n" );
	printf( "userdata   %d\n", pod->puserdata );
	printf( "pmaterial  ->\n" );
	printf( "\n" );
	Material_Print( pod->pmaterial );
	printf( "\n" );
}

/* End of File */
