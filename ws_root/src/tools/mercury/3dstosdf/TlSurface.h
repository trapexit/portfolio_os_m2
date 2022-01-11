/*
**	File:		TlSurface.h
**
**	Contains:	Contains surface class - surface is a single entity
**          	containing any number of facet lists sharing 
**          	a single vertex list, texture and material
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
**
**	To Do:
*/

#include "TlFacetList.h"
#include "TlMaterial.h"
#include "TlTexture.h"

#ifndef SURFACE
#define SURFACE

class TlSurface;

class TlSurface
{
  public:
	TlSurface( const TlVtxList* vlist, const TlMaterial* mat, const TlTexture* tex );
	TlSurface(); 
	~TlSurface();

	inline void SetFacetList( TlFacetList* flist)
		{ facetList = flist; };
	inline void SetNextSurface( TlSurface* next )
		{ nextSurface = next; }
	inline void SetMaterial( TlMaterial* mat )
		{ material = mat; };
	inline void SetTexture( TlTexture* tex )
		{ texture = tex; };
		
	/* TlSurface will have only one TlTexture and TlMaterial
	void AddMaterial( TlMaterial* );
	void AddTexture( TlTexture* );
	*/
	void AppendSurface( TlSurface *surf );

	inline TlFacetList* GetFacetList()
		{ return facetList; }
	inline TlSurface* GetNextSurface()
		{ return nextSurface; }
	TlSurface* GetLastSurface();

	inline TlMaterial* GetMaterial()
		{ return material; }
	inline TlTexture* GetTexture()
		{ return texture; }
	
	Boolean HasNullData() const;
	ostream& WriteSDF( ostream& os );

  private:
	TlFacetList	*facetList;
	TlMaterial	*material;
	TlTexture		*texture;

	TlSurface		*nextSurface;
};

#endif // SURFACE

