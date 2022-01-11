/*
	File:		TlSurface.cp

	Contains:	 

	Written by:	Ravindar Reddy	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

**		<2+>	 12/8/94	RRR		
		<1+>	11/17/94	RRR		Write shading type

	To Do:
*/

#include "TlSurface.h"

TlSurface::TlSurface()
{
	facetList = NULL;
	material = NULL;
	texture = NULL;

	nextSurface = NULL;	
}

// Share TlVtxList, TlMaterial, TlTexture if they are not null
TlSurface::TlSurface( 
const TlVtxList* vlist, 
const TlMaterial* mat, 
const TlTexture* tex )
{
	if ( vlist ) 
	{
		facetList = new TlFacetList( vlist );
		PRINT_ERROR( (facetList==NULL), "Out of memory" );
	} else facetList = NULL;
	
	if ( mat ) 
	{
		material = new TlMaterial( *mat );
		PRINT_ERROR( (material==NULL), "Out of memory" );
	} else material = NULL;
	
	if ( tex ) 
	{
		texture = new TlTexture( *tex );
		PRINT_ERROR( (texture==NULL), "Out of memory" );
	} else texture = NULL;

	nextSurface = NULL;	
}	

TlSurface::~TlSurface()
{
	/*
	TlMaterial *mcur = material;
	TlTexture *tcur = texture;
	TlMaterial *mnext;
	TlTexture *tnext;
	*/
	TlFacetList *fcur = facetList;
	TlFacetList *fnext;

	while ( fcur )
	{
 		fnext = fcur->GetNextFacetList();
		delete fcur;
		fcur = fnext;
	}

	delete material;
	delete texture;
	/* TlSurface will have only one TlTexture and TlMaterial
	while ( mcur )
	{
 		mnext = mcur->GetNextMaterial();
		delete mcur;
		mcur = mnext;
	}
	while ( tcur )
	{
 		tnext = tcur->GetNextTexture();
		delete tcur;
		tcur = tnext;
	}
	*/
}

/* TlSurface will have only one TlTexture and TlMaterial
void 
TlSurface::AddMaterial( TlMaterial* mat )
{
	if ( mat )
	{
		mat->SetNextMaterial( material );
		material = mat;
	}
}

void 
TlSurface::AddTexture( TlTexture* tex )
{
	if ( tex )
	{
		tex->SetNextTexture( texture );
		texture = tex;
	}
}
*/

void 
TlSurface::AppendSurface( TlSurface *surf )
{
	TlSurface *cur = this;

	while ( cur->GetNextSurface() )
 			cur = cur->GetNextSurface();
 			
	cur->SetNextSurface( surf );
}

TlSurface* 
TlSurface::GetLastSurface()
{
	TlSurface *cur = this;

	while ( cur->GetNextSurface() )
 			cur = cur->GetNextSurface();
 			
	return( cur );
}

Boolean 
TlSurface::HasNullData() const
{
	if ( facetList && facetList->HasNullData() == FALSE )
		return FALSE;
	else return TRUE;
}

ostream& 
TlSurface::WriteSDF( ostream& os )
{
	char buf[100];
	TlFacetList *fcur = facetList;
	TlVtxList *vlist = fcur->GetVtxList();
	
	
	if ( this->HasNullData() == FALSE )
	{
		//sprintf( buf, "Surface {" );
		//BEGIN_SDF( os, buf );
		
		if ( material && ( material->HasNullData() == FALSE ) )
		{
			sprintf( buf, "MatIndex %d", material->GetRefID() );
			WRITE_SDF( os, buf );
		}
		if ( texture && ( texture->HasNullData() == FALSE ) )
		{
			sprintf( buf, "TexIndex %d", texture->GetRefID() );
			WRITE_SDF( os, buf );
		} else WRITE_SDF( os, "TexIndex -1" );
		
		// Write autoNormal attribute
		/*if ( material ) {
			if ( material->GetShading() == 1 )
					WRITE_SDF( os, "autoNormals OFF\n" );
			else	WRITE_SDF( os, "autoNormals ON\n" );
		} else		WRITE_SDF( os, "autoNormals ON\n" );*/
		
		// Make autoNormals ON no matter what
		if( vlist->GetVertexFormat() & TlVtxList::NORMALS )
			WRITE_SDF( os, "autoNormals OFF\n" );
		else WRITE_SDF( os, "autoNormals ON\n" );
		// Write all the facet lists 
		while ( fcur )
		{
			//printf( "Name : %s, Two-sided = %s\n",
			//       material->GetName(), ( material->GetTwoSided() ? "TRUE" : "FALSE") );
			fcur->WriteSDF( os, material );
			fcur = fcur->GetNextFacetList();;
		}
	
		//facetList->WriteSDF( os );
		//END_SDF( os, "}" );
	}
	return os;
}
