/*
**	File:		TlFacetList.h
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
**		<1+>	 12/6/94	RRR		Include primitive type an clean code
**
**	To Do:
*/

#include "TlVtxList.h"
#include "TlMaterial.h"
#include "TlTexture.h"

#ifndef FACET_LIST
#define FACET_LIST

// Graphics primitive types
#define FACETLIST    0
#define TRIMESH      1
#define TRISTRIP     2
#define TRIFAN       3
#define QUADMESH     4


// Forward Declarations
class TlFacet;
class TlVertex;
class TlFacetList;

// TlFacetList class
class TlFacetList
{
  public :
	TlFacetList();
	TlFacetList( const TlVtxList *vlist );
	virtual ~TlFacetList();

	// Get the data
	virtual int GetType( )
		{ return FACETLIST; }
	
	inline TlFacet* GetFirstFacet() 
		{ return firstFacet; }
	inline TlFacet* GetLastFacet()
		{ return lastFacet; }
	inline Int32 GetNumFacets()
		{ return numFacets; }
	inline TlFacetList* GetNextFacetList()
		{ return nextFacetList; }
	inline TlVtxList* GetVtxList()
		{ return vtxList; }

	// Set the data
	void ReplaceVtxList( TlVtxList *vlist );
	void SetVtxList( TlVtxList* vlist );
	inline void SetNextFacetList( TlFacetList* flist )
		{ nextFacetList = flist; }
	void AppendFacet( TlFacet* facet );

	virtual void ToVertexIndices();
	virtual void ToVertexPointers();
		
	virtual Boolean HasNullData();
	virtual ostream& WriteSDF( ostream& os, TlMaterial *mat = NULL );
	ostream& WriteSmoothShaded( ostream& os, TlMaterial *mat = NULL );
	ostream& WriteFlatShaded( ostream& os, TlMaterial *mat = NULL );
  
  private:
  	TlVtxList *vtxList;
	TlFacet *firstFacet;
	TlFacet *lastFacet;
	Int32 numFacets;

	TlFacetList *nextFacetList;
	
	friend TlFacet;
};

// TlFacet class
class TlFacet
{
  public :
	TlFacet();
	TlFacet( const TlFacet& fct );
	TlFacet( TlVertex *v1, TlVertex *v2, TlVertex *v3 );
	~TlFacet();

	// Get the data
	inline TlVertex* GetFirstVertex() const
		{ return firstVertex; }
	inline TlVertex* GetLastVertex() const
		{ return lastVertex; }
	inline Int32 GetNumVertices() const
		{ return numVertices; }
	inline TlFacet* GetNextFacet() const
		{ return nextFacet; }
	/*
	inline TlMaterial* GetFacetMaterial()
		{ return facetMaterial; }
	inline TlTexture* GetFacetTexture()
		{ return facetTexture; }
	
		
	// Set the data
	inline void SetFacetMaterial( TlMaterial* mat )
		{ facetMaterial = mat; }
	inline void SetFacetTexture( TlTexture *tex )
		{facetTexture = tex; }
	*/
	void AppendVertex( TlVertex* vtx );
  
  private:
	TlVertex *firstVertex;
	TlVertex *lastVertex;
	Int32 numVertices;

	TlFacet *nextFacet;
	
	/* 
	Facets can not have their own attributes. They will
	get these from "surface" class
	TlMaterial *facetMaterial;
	TlTexture  *facetTexture;
	*/
	friend TlVertex;
	friend TlFacetList;
};

// TlVertex class
class TlVertex
{
  public :
	TlVertex();
	TlVertex( const TlVertex& vtx );
	TlVertex( TlVtxList& vlist, Int32 index );

	// Get the data
	inline TlVertex* GetNextVertex()
		{ return nextVertex; }
	Int32 GetVertexIndex() const;
	inline void *GetVertexData() const
		{ return ( vertexPtr.ptr ); }

	// Set the data
	void SetVertexIndex( Int32 index );
	inline void SetVertexData( void *data )
		{ vertexPtr.ptr = data; }
	inline void SetVertexDataIndx( Int32 index )
		{ vertexPtr.indx = index; }
	inline Int32 GetVertexDataIndx()
		{ return (vertexPtr.indx); }

	// Later provide all set/get operations on 
	// vertex - as soon as "TlVtxList" has it

  private:	
	TlVertex *nextVertex;
	IPtr vertexPtr;
	
	friend TlVtxList;
	friend TlFacetList;
	friend TlFacet;
};

#endif // POLY_LIST
