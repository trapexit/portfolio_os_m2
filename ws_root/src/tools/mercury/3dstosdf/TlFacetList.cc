/*
**	File:		TlFacetList.cp
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
**		<1+>	 12/6/94	RRR		Include primitive type
**
**	To Do:
*/

#include "TlFacetList.h"

TlFacetList::TlFacetList()
{
#ifdef DEBUG
	FacetList_numObjs++;
#endif
	firstFacet = NULL;
	lastFacet = NULL;
	numFacets = 0;
	nextFacetList = NULL;
	vtxList = NULL;
}

TlFacetList::TlFacetList( const TlVtxList *vlist )
{
#ifdef DEBUG
	FacetList_numObjs++;
#endif
	firstFacet = NULL;
	lastFacet = NULL;
	numFacets = 0;
	nextFacetList = NULL;
	vtxList = new TlVtxList( *vlist );
	PRINT_ERROR( (vtxList==NULL), "Out of memory" );
}

void
TlFacetList::SetVtxList( TlVtxList *vlist )
{
	if ( vtxList != NULL ) delete vtxList;
	vtxList = vlist;
}

void
TlFacetList::ReplaceVtxList( TlVtxList *vlist )
{
	*vtxList = *vlist;
}

TlFacetList::~TlFacetList()
{
#ifdef DEBUG
	FacetList_numObjs--;
#endif
	TlFacet *cur = firstFacet;
	TlFacet *next;

	while ( cur )
	{
 		next = cur->nextFacet;
		delete cur;
		cur = next;
	}
	if ( vtxList != NULL ) delete vtxList;
}

void 
TlFacetList::AppendFacet( TlFacet* facet )
{
	if ( this && facet )
	{
		if ( lastFacet ) lastFacet->nextFacet = facet;
		else firstFacet = facet;
		lastFacet = facet;
		numFacets++;
	}
}

Boolean
TlFacetList::HasNullData()
{
	if ( ( vtxList->HasNullData() == TRUE ) || ( numFacets == 0 ) )
		return TRUE;
	else return FALSE;
}

/*
** Go through all the facets and change vertices reference
** from "pointer" to "index"
*/
void 
TlFacetList::ToVertexIndices()
{
	TlFacet *cur;
	TlVertex *vcur;
	unsigned int indx;

	for ( cur = firstFacet; cur; ) 
	{
		vcur = cur->GetFirstVertex();
		for( ; vcur; ) 
		{
			indx = (unsigned int)vcur->GetVertexIndex();
			vcur->vertexPtr.indx = indx;
			
			vcur = vcur->GetNextVertex();
		}
		cur = cur->GetNextFacet();
	}
}

/*
** Go through all the facets and change vertices reference
** from "index" to "pointer"
*/
void 
TlFacetList::ToVertexPointers()
{
	TlFacet *cur;
	TlVertex *vcur;
	unsigned int indx;

	for ( cur = firstFacet; cur; ) 
	{
		vcur = cur->GetFirstVertex();
		for( ; vcur; ) 
		{
			indx = (unsigned int)vcur->vertexPtr.indx;
			vcur->vertexPtr.ptr = (void *) (*vtxList)[ indx ];
			
			vcur = vcur->GetNextVertex();
		}
		cur = cur->GetNextFacet();
	}
}

/*
** Warning : This function alters the indices in vertex list
** to weed out unreferenced vertices
** two_sided flag writes another set of facets in the reverse
** order so that you get the normal flipped
** Note : this assumes the polygons are triangles, which is true
**        in case of 3DS converter
*/
ostream& 
TlFacetList::WriteFlatShaded( ostream& os, TlMaterial *mat )
{
	TlFacet *cur;
	TlVertex *vcur;
	int j;
	char dump[100], quad[100];
	int max_verts = 0;
	Int32 indx = 0, facet_verts[3];
	 Boolean two_sided = FALSE;

	if( mat != NULL ) two_sided = mat->GetTwoSided();
	
	if ( this->HasNullData() == FALSE ) {
		sprintf( dump, "# Duplicated vertices for flat-shading" );
		WRITE_SDF( os, dump );
	
		if ( two_sided == TRUE ) 
		{
			sprintf( dump, "# Duplicated %d polygons for two-sided attribute\n", numFacets );
			WRITE_SDF( os, dump );
			cerr << "WARNING: " << dump << endl;
		}
		
		BEGIN_SDF( os, "Triangles {" );
		
		// initialise the ref count to zero
		Int32 vsize = vtxList->GetVertexSize();
		Int32 vcount = vtxList->GetVtxCount();	
		double *vd = vtxList->GetVtxData();

		BEGIN_SDF( os, "vertexList {" );	
		sprintf( dump, "format ( %s )", vtxList->GetFormatString() );
		WRITE_SDF( os, dump );
		BEGIN_SDF( os, "vertices {" );
		
		// write triangles
		for ( cur = firstFacet; cur; ) 
		{
			dump[ 0 ] = '\0';
			indx = 0;
			vcur = cur->GetFirstVertex();
			for( ; vcur; ) 
			{
				facet_verts[ indx ++ ] = vcur->GetVertexIndex();
				vcur = vcur->GetNextVertex();
			}
			for( indx = 0; indx < 3; indx++ )
			{	
				// write the vertex data
				vd = (*vtxList)[ facet_verts[ indx ] ];

				sprintf( dump, "{ %0.6g %0.6g %0.6g", vd[1], vd[2], vd[3] ); 
				for ( j = 4; j < vsize; j++ )
				{
					sprintf( quad, " %0.6g", vd[ j ] );
					strcat( dump, quad );
				}
				strcat( dump, " }" );
				WRITE_SDF( os, dump );
			}
				
			if ( two_sided == TRUE )
			{
				for( indx = 2; indx >= 0; indx-- )
				{	
					// write the vertex data
					vd = (*vtxList)[ facet_verts[ indx ] ];
	
					sprintf( dump, "{ %0.6g %0.6g %0.6g", vd[1], vd[2], vd[3] ); 
					for ( j = 4; j < vsize; j++ )
					{
						sprintf( quad, " %0.6g", vd[ j ] );
						strcat( dump, quad );
					}
					strcat( dump, " }" );
					WRITE_SDF( os, dump );
				}
 			}
						
			cur = cur->GetNextFacet();
		}
		END_SDF( os, "}" );
		END_SDF( os, "}" );

		END_SDF( os, "}" );
	}
		
	return os;
}		

/*
** Warning : This function alters the indices in vertex list
** to weed out unreferenced vertices
** two_sided flag writes another set of facets in the reverse
** order so that you get the normal flipped
*/
ostream& 
TlFacetList::WriteSmoothShaded( ostream& os, TlMaterial *mat )
{
	TlFacet *cur;
	TlVertex *vcur;
	int nverts, i;
	char dump[100], quad[100];
	int *dup_facet, indx = 0, max_verts = 0;
	 Boolean two_sided = FALSE;

	if( mat != NULL ) two_sided = mat->GetTwoSided();
	
	if ( this->HasNullData() == FALSE ) {
	
		if ( two_sided == TRUE ) 
		{
			sprintf( dump, "# Duplicated %d polygons for two-sided attribute\n", numFacets );
			WRITE_SDF( os, dump );
			cerr << "WARNING: " << dump << endl;
		}
		
		BEGIN_SDF( os, "TriMesh {" );
		
		// initialise the ref count to zero
		Int32 vsize = vtxList->GetVertexSize();
		Int32 vcount = vtxList->GetVtxCount();	
		double *vd = vtxList->GetVtxData();
		VertexPtr *vtx;
		int vrefCount = 0;
		
		for ( i = 0; i < vcount; i++, vd += vsize )	
			((VertexPtr *) vd)->indx = 0;
		// initialise the use count
		for ( cur = firstFacet; cur; ) 
		{
			vcur = cur->GetFirstVertex();
			for( ; vcur; ) 
			{
				vtx = ( VertexPtr *) vcur->GetVertexData();
				vtx->indx++;
				vcur = vcur->GetNextVertex();
			}
			cur = cur->GetNextFacet();
		}		
			
		// write vertex list
		vtxList->WriteSDF( os, two_sided, &vrefCount );
		
		// write vertex count
		BEGIN_SDF( os, "vertexCount {" );
		for ( cur = firstFacet; cur;  ) 
		{
			dump[ 0 ] = '\0';
			for ( i = 0; ( cur && i < 10 ); i++ ) 
			{
				nverts = (int)cur->GetNumVertices();
				
				// calculate the maximum number of verts per facet in the model
				if ( nverts > max_verts ) max_verts = nverts;
				
				// triangle fan or triangle strip
				if ( nverts > 3 ) sprintf( quad, " -%d", nverts );
				else sprintf( quad, " %d", nverts );
				strcat(dump, quad);
				
				// duplicate the facets if it is double sided
				if ( two_sided == TRUE ) strcat(dump, quad);
				
				cur = cur->GetNextFacet();
			}
			strcat( dump, "" );
			WRITE_SDF( os, dump );
		}
		END_SDF( os, "}" );
		
		// write vertex indices
		BEGIN_SDF( os, "vertexIndices {" );
		
		if ( two_sided == TRUE ) 
		{
			dup_facet = new int[ max_verts ];
			PRINT_ERROR( (dup_facet==NULL), "Out of memory" );
		}
		
		for ( cur = firstFacet; cur; ) 
		{
			dump[ 0 ] = '\0';
			indx = 0;
			vcur = cur->GetFirstVertex();
			for( ; vcur; ) 
			{
				nverts = (int)vcur->GetVertexIndex();
				
				// indexes for two-sided polys are new set of vertices at the end
				if ( two_sided == TRUE ) dup_facet[ indx++ ] = nverts + vrefCount;
				
				sprintf( quad, "%d ", nverts );
				strcat(dump, quad);
				vcur = vcur->GetNextVertex();
			}
			strcat( dump, "" );
			WRITE_SDF( os, dump );
			
			// if the model is two-sided write another set os facets
			if ( two_sided == TRUE )
			{
				dump[ 0 ] = '\0'; indx--;
				for( ; indx >= 0; indx-- ) 
				{
					sprintf( quad, "%d ", dup_facet[ indx ] );
					strcat(dump, quad);
				}
				strcat( dump, "" );
				WRITE_SDF( os, dump );
			}
			
			cur = cur->GetNextFacet();
		}
		END_SDF( os, "}" );		
		END_SDF( os, "}" );
	}
	
	if ( two_sided == TRUE ) delete [] dup_facet;
	
	return os;
}

/*
** Warning : This function alters the indices in vertex list
** to weed out unreferenced vertices
** two_sided flag writes another set of facets in the reverse
** order so that you get the normal flipped
*/
ostream& 
TlFacetList::WriteSDF( ostream& os, TlMaterial *mat )
{
	int shade_type;
	
	if ( mat != NULL ) shade_type = mat->GetShading();
	
	if ( shade_type == 1 ) WriteFlatShaded( os, mat );
	else WriteSmoothShaded( os, mat );
	
	return os;
}

// TlFacet methods

TlFacet::TlFacet()
{
	firstVertex = NULL;
	lastVertex = NULL;
	numVertices = 0;
	nextFacet = NULL;
}

TlFacet::TlFacet( const TlFacet& fct )
{
	TlVertex *cur = fct.GetFirstVertex();
	TlVertex *nvtx;

	firstVertex = NULL;
	lastVertex = NULL;
	numVertices = 0;
	nextFacet = NULL;

	while ( cur )
	{
		nvtx = new TlVertex( *cur );
		PRINT_ERROR( (nvtx==NULL), "Out of memory" );
		AppendVertex( nvtx );
 		cur = cur->nextVertex;
	}
}

/*
** Create a triangle facet 
*/
TlFacet::TlFacet( TlVertex *v1, TlVertex *v2, TlVertex *v3 )
{
	firstVertex = NULL;
	lastVertex = NULL;
	numVertices = 0;
	nextFacet = NULL;
	
	AppendVertex( new TlVertex(*v1) );
	AppendVertex( new TlVertex(*v2) );
	AppendVertex( new TlVertex(*v3) );
}

TlFacet::~TlFacet()
{
	TlVertex *cur = firstVertex;
	TlVertex *next;

	while ( cur )
	{
 		next = cur->nextVertex;
		delete cur;
		cur = next;
	}
}

void 
TlFacet::AppendVertex( TlVertex* vtx )
{
	if ( this && vtx )
	{
		if ( lastVertex ) lastVertex->nextVertex = vtx;
		else firstVertex = vtx;
		lastVertex = vtx;
		numVertices++;
	}
}

// TlVertex methods

TlVertex::TlVertex()
{
	vertexPtr.ptr = NULL;
	nextVertex = NULL;
}

TlVertex::TlVertex( const TlVertex& vtx )
{
	vertexPtr.ptr = vtx.vertexPtr.ptr;
	nextVertex = NULL;
}

TlVertex::TlVertex( TlVtxList& vlist, Int32 index )
{
	vertexPtr.ptr = (void *) vlist[ index ];
	nextVertex = NULL;
}

Int32 
TlVertex::GetVertexIndex() const
{
	if ( vertexPtr.ptr != NULL )
	{
		return ( ((VertexPtr *)vertexPtr.ptr)->indx );
	} else return -1;
}
	

// Set the data
void 
TlVertex::SetVertexIndex( Int32 index )
{
	if ( vertexPtr.ptr != NULL )
	{
		((VertexPtr *)vertexPtr.ptr)->indx = index;
	}
}

