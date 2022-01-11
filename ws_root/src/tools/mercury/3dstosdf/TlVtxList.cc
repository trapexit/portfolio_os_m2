/*
**	File:		TlVtxList.cp
**
**	Contains:	 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**
**		<2+>	 12/5/94	RRR		Enhance this class
**
**	To Do:
*/

#include "TlVtxList.h"

// Static variables out of pipeline code for fast access
static int  vtxSize[] =
{
	1 + 3,              // VERTEX INDEX + LOCATIONS
	1 + 3 + 4,          // COLORS
	1 + 3 + 3,          // NORMALS
	1 + 3 + 4 + 3,      // COLORS | NORMALS 
	1 + 3 + 2,          // TEXTURES
	1 + 3 + 4 + 2,      // COLORS | TEXTURES
	1 + 3 + 3 + 2,      // NORMALS | TEXTURES
	1 + 3 + 4 + 3 + 2,  // COLORS | NORMALS | TEXTURES
};

// Index for normal data
static int  vtxNormIdx[] = { 1, 1, 4, 8, 1, 1, 4, 8, };

// Index for texture data
static int  vtxTexIdx[] = { 1, 1, 1, 1, 4, 8, 7, 11, };

TlVtxList::TlVtxList( TlVtxList::Format format, 
                  Int32 n, 
                  double* data )
{
#ifdef DEBUG
	VtxList_numObjs++;
#endif
	v = new VtxEntry;
	PRINT_ERROR( (v==NULL), "Out of memory" );
	v->vtxFormat = format;
	v->vtxCount = n;
	v->vtxData = data;

}

TlVtxList::TlVtxList( TlVtxList::Format format, 
                  Int32 n )
{
#ifdef DEBUG
	VtxList_numObjs++;
#endif
	v = new VtxEntry;
	PRINT_ERROR( (v==NULL), "Out of memory" );
	v->vtxFormat = format;
	v->vtxCount = n;
	if (n > 0) 
	{
		v->vtxData = new double[ n * TlVtxList::VertexSize( format ) ];
		PRINT_ERROR( (v->vtxData==NULL), "Out of memory" );
	} else v->vtxData = NULL;
}

TlVtxList::TlVtxList( const TlVtxList& vl )
{
#ifdef DEBUG
	VtxList_numObjs++;
#endif

	vl.v->refCount++;
	v = vl.v;
}

TlVtxList::~TlVtxList()
{
#ifdef DEBUG
	VtxList_numObjs--;
#endif
	
	if ( --v->refCount == 0 ) 
	{
		if ( v->vtxData != NULL ) delete[] v->vtxData;
		delete v;
	}
}

/* 
** Expand the size of the TlVertex List to the newSize.
** Note : make sure the facet list calling this has all
** the facet vertices initialised to "vertex index" mode
** rather than "vertex pointer" mode at the time of 
** calling this function
*/
void 
TlVtxList::Expand( unsigned int newSize )
{
	if ( newSize > v->vtxCount )
	{
		unsigned int memSize, oldMemSize;
		double *newData;
		
		oldMemSize = (unsigned int)(v->vtxCount * TlVtxList::VertexSize( v->vtxFormat ) );
		memSize = (unsigned int)( newSize * TlVtxList::VertexSize( v->vtxFormat ) );
		newData = new double[ memSize ];
		PRINT_ERROR( (newData==NULL), "Out of memory" );
		memcpy( newData, v->vtxData, oldMemSize * sizeof(float) );
		
		if ( v->vtxData != NULL ) delete [] v->vtxData;
		
		v->vtxCount = newSize;
		v->vtxData = newData;
	}
}

/*
** Set the array index value for all the vertices
*/
void 
TlVtxList::IndexVertices()
{
	double *vdata = v->vtxData;
	unsigned int vsize = (unsigned int)GetVertexSize();
	unsigned int vcount = (unsigned int)GetVtxCount();
	unsigned int i;

	for ( i = 0; i < vcount; i++, vdata += vsize )	
		((VertexPtr *) vdata)->indx = i;
}

TlVtxList::Format 
TlVtxList::GetVertexFormat() const
{
	return ( v->vtxFormat );
}

char*	
TlVtxList::GetFormatString() const
{
	static char fstr[50];
	
	fstr[0] = '\0';
	if ( v->vtxFormat & TlVtxList::LOCATIONS ) {
		strcat( fstr, "LOCATIONS" );
	}
	if ( v->vtxFormat & TlVtxList::COLORS ) {
		strcat( fstr, " | COLORS" );
	}
	if ( v->vtxFormat & TlVtxList::NORMALS ) {
		strcat( fstr, " | NORMALS" );
	}
	if ( v->vtxFormat & TlVtxList::TEXTURES ) {
		strcat( fstr, " | TEXCOORDS" );
	}
	
	return ( fstr );	
}

double* 
TlVtxList::operator[]( Int32 indx ) const
{
	return ( v->vtxData + indx * TlVtxList::VertexSize( v->vtxFormat ) );
}

/*
** TlVtxList::VertexSize()
*/

Int32	
TlVtxList::VertexSize( TlVtxList::Format format )
{
	return ( vtxSize[ format & VTX_MASK ] );	
}

Int32	
TlVtxList::GetVertexSize( ) const
{
	return ( TlVtxList::VertexSize( v->vtxFormat ) );	
}			

TlVtxList& 
TlVtxList::operator=( const TlVtxList& vlist )
{
	vlist.v->refCount++;
	if ( --v->refCount == 0 ) 
	{
		if ( v->vtxData != NULL ) delete[] v->vtxData;
		delete v;
	}
	v = vlist.v;
	return *this;
}

Boolean 
TlVtxList::operator==( const TlVtxList& vlist ) const
{
	if ( v == vlist.v ) return TRUE;
	else return FALSE;
}

Point3D*		
TlVtxList::GetPosition( Int32 indx ) const
{
	double *data;
	
	data = v->vtxData + indx * vtxSize[ v->vtxFormat & VTX_MASK ] + 1;
	return ( (Point3D *) data );
}

Point3D*		
TlVtxList::GetNormal( Int32 indx ) const
{
	double *data;
	Int32 style = v->vtxFormat & VTX_MASK;
		
	data = v->vtxData + indx * vtxSize[ style ] + vtxNormIdx[ style ];
	return ( (Point3D *) data );
}

UVPoint*	
TlVtxList::GetTexCoord( Int32 indx ) const
{
	double *data;
	Int32 style = v->vtxFormat & VTX_MASK;
		
	data = v->vtxData + indx * vtxSize[ style ] + vtxTexIdx[ style ];
	return ( (UVPoint *) data );
}

Point3D*		
TlVtxList::GetPosition( void *vdata ) const
{
	double *data = (double *)vdata;
	
	data = data + 1;
	return ( (Point3D *) data );
}

UVPoint*	
TlVtxList::GetTexCoord(  void *vdata ) const
{
	double *data = (double *)vdata;
	Int32 style = v->vtxFormat & VTX_MASK;
		
	data = data + vtxTexIdx[ style ];
	return ( (UVPoint *) data );
}
		
ostream& 
TlVtxList::WriteSDF( ostream& os, Boolean twoSided, int *vrefCount )
{
	int i, j;
	char buf[200], vbuf[200];
	Int32 vsize = GetVertexSize();
	Int32 vcount = GetVtxCount();
	double *vd = GetVtxData();
	int sindx = 0;
		
	BEGIN_SDF( os, "vertexList {" );	
	sprintf( buf, "format ( %s )", GetFormatString() );
	WRITE_SDF( os, buf );
	BEGIN_SDF( os, "vertices {" );
	for ( i = 0; i < vcount; i++, vd += vsize )
	{	
		// By the time it comes here all the vertex 'indx' contains
		// use count, if vertex is referenced then initialise the 
		// sequential index and then write it
		if ( ((VertexPtr *) vd)->indx > 0 ) 
		{
			if ( twoSided != TRUE ) ((VertexPtr *) vd)->indx = sindx;
			
			sprintf( buf, "{ %g %g %g", vd[1], vd[2], vd[3] ); 
			for ( j = 4; j < vsize; j++ )
			{
				sprintf( vbuf, " %g", vd[ j ] );
				strcat( buf, vbuf );
			}
			strcat( buf, " }" );
			WRITE_SDF( os, buf );
			
			sindx ++;
		} 
	}
	
	// for two-sided polygons write another set of vertices
	if ( twoSided == TRUE )
	{
		vd = GetVtxData();
		sindx = 0;
		for ( i = 0; i < vcount; i++, vd += vsize )
		{	
			// By the time it comes here all the vertex 'indx' contains
			// use count, if vertex is referenced then initialise the 
			// sequential index and then write it
			if ( ((VertexPtr *) vd)->indx > 0 ) 
			{
				((VertexPtr *) vd)->indx = sindx;
				
				sprintf( buf, "{ %0.6g %0.6g %0.6g", vd[1], vd[2], vd[3] ); 
				for ( j = 4; j < vsize; j++ )
				{
					sprintf( vbuf, " %0.6g", vd[ j ] );
					strcat( buf, vbuf );
				}
				strcat( buf, " }" );
				WRITE_SDF( os, buf );
				
				sindx ++;
			} 
			*vrefCount = sindx;
		}
	}
	
	END_SDF( os, "}" );
	END_SDF( os, "}" );
	return os;
}

