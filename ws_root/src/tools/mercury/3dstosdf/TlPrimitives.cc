/*
**	File:		TlPrimitives.cp
**
**	Contains:	Create SDF graphics primitives
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
**				 12/21/94	RRR		Initial version
**				 3/27/95	RRR		modified GetVertexList() & TriStrip()
**
**	To Do:
*/

#include "TlPrimitives.h"

/*
** Create a TlTriStrip given a TlVertex list and vertex indices
** Odd polygons are ordered "counter-clockwise" and "even" polygons
** are ordered "clockwise" and they need to be switched
*/
TlTriStrip::TlTriStrip( TlVtxList *vlist, int *vts, int size )
: TlFacetList( vlist )
{
	int i;
	int nThings = size - 2;
	TlFacet *fct;
	TlVertex *vtx;
	
	for ( i = 0; i < nThings; i++ )
	{
		fct = new TlFacet();
		PRINT_ERROR( (fct==NULL), "Out of memory" );
		
		// make all the triangle polygons clockwise
		if ( !( i % 2 ) )	// even numbered triangles - same order
		{			 
			vtx = new TlVertex( *vlist, (Int32)vts[i] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
			vtx = new TlVertex( *vlist, (Int32)vts[i+1] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
			vtx = new TlVertex( *vlist, (Int32)vts[i+2] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
		} else {		// odd numbered triangles - inverted order
			vtx = new TlVertex( *vlist, (Int32)vts[i] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
			vtx = new TlVertex( *vlist, (Int32)vts[i+2] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
			vtx = new TlVertex( *vlist, (Int32)vts[i+1] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
		}
		this->AppendFacet( fct );			
	}	
}

/*
** Create a TlTriStrip out of a single triangle facet
** the polygon should be oriented "counter-clockwise"
** w.r.t its normal
*/
TlTriStrip::TlTriStrip(  TlVtxList *vlist, TlFacet *fct )
: TlFacetList( vlist )
{
	this->AppendFacet( fct );			
}

TlTriStrip::~TlTriStrip()
{
}

/*
** Get the vertex array of the primitives. Size of the array "verts"
** should be equal to "GetNumFacets() + 2" and  pre-allocated
** This function returns number of vertices in the primitive
*/
int 
TlTriStrip::GetVertexList( TlVertex *verts[] )
{
	TlFacet *cur;
	TlVertex *vcur;
		
	if ( HasNullData() == FALSE ) 
	{
		unsigned int vcount;

		// collect all the vertices
		for ( vcount = 0, cur = GetFirstFacet(); cur; ) 
		{
			vcur = cur->GetFirstVertex();
			verts[ vcount++ ] = vcur;
			cur = cur->GetNextFacet();
		}
		// get two vertices from last triangle
		if ( !( (vcount - 1) % 2 ) ) 	// even numbered last triangle
		{
			vcur = vcur->GetNextVertex();
			verts[ vcount++ ] = vcur;
			vcur = vcur->GetNextVertex();
			verts[ vcount ] = vcur;
		} else {					// odd numbered last triangle
			vcur = vcur->GetNextVertex();
			verts[ ++vcount ] = vcur;
			vcur = vcur->GetNextVertex();
			verts[ vcount - 1 ] = vcur;
		}
		return ( (vcount + 1) );
	} else return 0;
}

ostream& 
TlTriStrip::WriteSDF( ostream& os, TlMaterial *mat )
{
	unsigned int i, j;
	char buf[100], vbuf[100];
		
	if ( HasNullData() == FALSE ) 
	{
		BEGIN_SDF( os, "TriStrip {" );

		float *vd;
		TlVtxList *vlist = this->GetVtxList();		
		Int32 vsize = vlist->GetVertexSize();
		unsigned int vcount = (unsigned int)this->GetNumFacets() + 2;
		TlVertex **verts = new TlVertex* [ vcount ]; 

		PRINT_ERROR( (verts==NULL), "Out of memory" );

		(void)&mat;

		BEGIN_SDF( os, "vertexList {" );
		sprintf( buf, "format ( %s )", vlist->GetFormatString() );
		WRITE_SDF( os, buf );
		BEGIN_SDF( os, "vertices {" );

		// collect all the vertices
		vcount = GetVertexList( verts );
			
		// write the vertices out
		for ( i = 0; i < vcount; i++ )
		{	
			//cout << verts[ i ]->GetVertexIndex() << endl;
			vd = (float *)verts[ i ]->GetVertexData();
				
			sprintf( buf, "{ %f %f %f", vd[1], vd[2], vd[3] );
			for ( j = 4; j < vsize; j++ )
			{
				sprintf( vbuf, " %f", vd[ j ] );
				strcat( buf, vbuf );
			}
			strcat( buf, " }" );
			WRITE_SDF( os, buf );
		}
		END_SDF( os, "}" );
		END_SDF( os, "}" );
		END_SDF( os, "}" );
		delete [] verts;
	}
	return os;	
}

/*
** Create a TlTriFan given a TlVertex list and vertex indices
** All the polygons are ordered "counter-clockwise"
*/
TlTriFan::TlTriFan( TlVtxList *vlist, int *vts, int size )
: TlFacetList( vlist )
{
	int i, j;
	int nThings = size - 2;
	TlFacet *fct;
	TlVertex *vtx;
	
	for ( i = 0; i < nThings; i++ )
	{
		fct = new TlFacet();
		PRINT_ERROR( (fct==NULL), "Out of memory" );
		vtx = new TlVertex( *vlist, (Int32)vts[0] );
		PRINT_ERROR( (vtx==NULL), "Out of memory" );
		fct->AppendVertex( vtx );
		
		for ( j = i+1; j < (i + 3); j++ ) 
		{
			vtx = new TlVertex( *vlist, (Int32)vts[j] );
			PRINT_ERROR( (vtx==NULL), "Out of memory" );
			fct->AppendVertex( vtx );
		}
		this->AppendFacet( fct );
	}	
}

TlTriFan::~TlTriFan()
{
}

/*
** Get the vertex indices of the primitives. Size of the array "vts"
** should be equal to "GetNumFacets() + 2" and  pre-allocated
** This function returns number of vertices in the primitive
*/
int 
TlTriFan::GetVertexList( TlVertex *verts[] )
{
	TlFacet *cur;
	TlVertex *vcur;
	char buf[100], vbuf[100];
		
	if ( HasNullData() == FALSE ) 
	{
		unsigned int vcount;
		
		// collect all the vertices
		vcur = GetFirstFacet()->GetFirstVertex();
		verts[ 0 ] = vcur;
		
		for ( vcount = 0, cur = GetFirstFacet(); cur; ) 
		{
			vcur = cur->GetFirstVertex();
			vcur = vcur->GetNextVertex();
			verts[ ++vcount ] = vcur;
			
			if ( !cur->GetNextFacet() )
			{
				vcur = cur->GetFirstVertex();
				vcur = vcur->GetNextVertex();
				vcur = vcur->GetNextVertex();
				verts[ ++vcount ] = vcur;
			}
			
			cur = cur->GetNextFacet();
		}
		return ( (vcount + 1) );
	} else return 0;
}

ostream& 
TlTriFan::WriteSDF( ostream& os, TlMaterial *mat )
{
	unsigned int i, j;
	char buf[100], vbuf[100];
		
	if ( HasNullData() == FALSE ) 
	{
		BEGIN_SDF( os, "TriFan {" );

		float *vd;
		TlVtxList *vlist = this->GetVtxList();		
		Int32 vsize = vlist->GetVertexSize();
		unsigned int vcount = (unsigned int)this->GetNumFacets() + 2;
		TlVertex **verts = new TlVertex* [ vcount ]; 
		PRINT_ERROR( (verts==NULL), "Out of memory" );

		(void)&mat;

		BEGIN_SDF( os, "vertexList {" );
		sprintf( buf, "format ( %s )", vlist->GetFormatString() );
		WRITE_SDF( os, buf );
		BEGIN_SDF( os, "vertices {" );
		
		// collect all the vertices
		vcount = GetVertexList( verts );
		
		// write the vertices out
		for ( i = 0; i < vcount; i++ )
		{	
			//cout << verts[ i ]->GetVertexIndex() << endl;
			vd = (float *)verts[ i ]->GetVertexData();
				
			sprintf( buf, "{ %f %f %f", vd[1], vd[2], vd[3] );
			for ( j = 4; j < vsize; j++ )
			{
				sprintf( vbuf, " %f", vd[ j ] );
				strcat( buf, vbuf );
			}
			strcat( buf, " }" );
			WRITE_SDF( os, buf );
		}
		END_SDF( os, "}" );
		END_SDF( os, "}" );
		END_SDF( os, "}" );
		delete [] verts;
	}
	return os;	
}

/*
** Create an empty TriMesh
*/
TlTriMesh::TlTriMesh()
{
	primitives = NULL;
	lastPrimitive = NULL;
	numPrimitives = 0;
}

/*
** Create a TriMeshn given a TlVertex list shared by other
** primitives in the list. VertexList sharing is done
** only within a TlTriMesh primitives, so duplicate the
** VertexList here
*/
TlTriMesh::TlTriMesh( TlVtxList *vlist ) : TlFacetList( vlist )
{
	/*unsigned int vcount = vlist->GetVtxCount();
	unsigned int vsize = vlist->GetVertexSize();
	float *vdata = vlist->GetVtxData();
	float *vd = new float[ vcount * vsize ];
	
	memcpy( vd, vdata, vcount * vsize * sizeof( float ) );
	
	TlVtxList *vl = new TlVtxList( vlist->GetVertexFormat(),
	                           vcount, vd );
	SetVtxList( vl );*/
	
	primitives = NULL;
	lastPrimitive = NULL;
	numPrimitives = 0;
}

/*
** TlTriMesh contains a list of "TlFacetList"s attached to a 
** empty "TlFacetList" of type "TRIMESH"
*/
TlTriMesh::~TlTriMesh()
{
	TlFacetList *cur = primitives;
	TlFacetList *next;

	while ( cur )
	{
 		next = cur->GetNextFacetList();
		delete cur;
		cur = next;
	}
}

/*
** Go through all the facets and change vertices reference
** from "pointer" to "index"
*/
void 
TlTriMesh::ToVertexIndices()
{
	TlVtxList *vlist = this->GetVtxList();
	TlFacetList *cur = primitives;

	// index the vertices in vertex list
	vlist->IndexVertices();
	
	// convert all the primitive vertices to index reference
	while ( cur )
	{
		cur->ToVertexIndices();
		cur = cur->GetNextFacetList();
	}
}

/*
** Go through all the facets and change vertices reference
** from "index" to "pointer"
*/
void 
TlTriMesh::ToVertexPointers()
{
	TlVtxList *vlist = this->GetVtxList();
	TlFacetList *cur = primitives;

	// convert all the primitive vertices to pointer reference
	while ( cur )
	{
		cur->ToVertexPointers();
		cur = cur->GetNextFacetList();
	}
}
		
Boolean
TlTriMesh::HasNullData()
{
	if ( ( primitives == NULL ) || ( GetVtxList()->HasNullData() == TRUE ) )
		return TRUE;
	else return FALSE;
}

void 
TlTriMesh::AppendPrimitive( TlFacetList *prim )
{
	if ( this && prim )
	{
		if ( lastPrimitive ) 
			lastPrimitive->SetNextFacetList( prim );
		else primitives = prim;
		lastPrimitive = prim;
		numPrimitives++;
	}
}

/*
** Warning : This function alters the indices in vertex list
** to weed out unreferenced vertices
*/
ostream& 
TlTriMesh::WriteSDF( ostream& os, TlMaterial *mat )
{
	TlFacetList *cur;
	unsigned int nverts, i, j;
	char dump[100], quad[100];
	unsigned int ptype;
	TlVtxList *vlist = this->GetVtxList();
	unsigned int vcount = (unsigned int)vlist->GetVtxCount();

	BEGIN_SDF( os, "TriMesh {" );
	
	// initialise the ref count to zero
	Int32 vsize = vlist->GetVertexSize();
	double *vd = vlist->GetVtxData();
	
	(void)&mat;

	// initialise each vertex use count to 1
	for ( i = 0; i < vcount; i++, vd += vsize )	
		((VertexPtr *) vd)->indx = 1;
			
	// write vertex list
	vlist->WriteSDF( os );
	
	// write vertex count and calculate the maximum vertex count
	BEGIN_SDF( os, "vertexCount {" );
	for ( cur = primitives; cur;  ) 
	{
		dump[ 0 ] = '\0';
		for ( i = 0; ( cur && i < 10 ); i++ ) 
		{
			ptype = cur->GetType();
			
			if ( ptype == TRIFAN ) {
				nverts = ((TlTriFan *)cur)->GetNumVertices();
				vcount = ( vcount < nverts ) ? nverts : vcount;
				if ( nverts > 3 ) sprintf( quad, " -%d", nverts );
				else sprintf( quad, " %d", nverts );
			} else if ( ptype == TRISTRIP ) {
				nverts = ((TlTriStrip *)cur)->GetNumVertices();
				vcount = ( vcount < nverts ) ? nverts : vcount;
				sprintf( quad, " %d", nverts );
			} else {
				cerr << "Unknown primitive in TlTriMesh\n";
			}
			strcat(dump, quad);
			cur = cur->GetNextFacetList();
		}
		strcat( dump, "" );
		WRITE_SDF( os, dump );
	}
	END_SDF( os, "}" );
	
	// write vertex indices
	TlVertex **verts = new TlVertex* [ vcount ];
	PRINT_ERROR( (verts==NULL), "Out of memory" );
	BEGIN_SDF( os, "vertexIndices {" );
	for ( cur = primitives; cur; ) 
	{
		ptype = cur->GetType();
		
		if ( ptype == TRIFAN ) {
			vcount = ((TlTriFan *)cur)->GetVertexList( verts );
		} else if ( ptype == TRISTRIP ) {
			vcount = ((TlTriStrip *)cur)->GetVertexList( verts );
		} else {
			vcount = 0;
			cerr << "Unknown primitive in TlTriMesh\n";
		}
		for( i = 0; i < vcount; )
		{
			dump[ 0 ] = '\0';
			for( j = 0; (i < vcount) && ( j < 10 ); j++, i++ )
			{
				nverts = (unsigned int)verts[ i ]->GetVertexIndex();
				sprintf( quad, "%d ", nverts );
				strcat(dump, quad);
			}
			strcat( dump, "" );
			WRITE_SDF( os, dump );
		}
		cur = cur->GetNextFacetList();
	}
	END_SDF( os, "}" );	
		
	END_SDF( os, "}" );

	return os;
}

