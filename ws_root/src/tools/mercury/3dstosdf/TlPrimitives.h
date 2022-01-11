#include "TlFacetList.h"

#ifndef PRIMITIVES
#define PRIMITIVES

class TlTriStrip : public TlFacetList
{
  public:
	TlTriStrip( TlVtxList *vlist, int *vts = NULL, int size = 0 );
	TlTriStrip( TlVtxList *vlist, TlFacet *fct );
	~TlTriStrip();

	int GetType( ) { return TRISTRIP; }
	int GetNumVertices() 
		{ return ( GetNumFacets() ? (GetNumFacets() + 2) : 0 ); }
	int GetVertexList( TlVertex *verts[] );

  	ostream& WriteSDF( ostream& os, TlMaterial *mat = NULL );
};

class TlTriFan : public TlFacetList
{
  public:
	TlTriFan( TlVtxList *vlist, int *vts = NULL, int size = 0 );
	~TlTriFan();

	int GetType( ) { return TRIFAN; }
	int GetNumVertices() 
		{ return ( GetNumFacets() ? (GetNumFacets() + 2) : 0 ); }
	int GetVertexList( TlVertex *verts[] );

  	ostream& WriteSDF( ostream& os, TlMaterial *mat = NULL );
};

/* 
** This class contains a list of "TlFacetList" with superclass
** having a "TlVtxList" and no facets. The "primitives" variable
** shares the "TlVtxList" and contains "TlTriStrip" or "TlTriFan" primitive
*/

class TlTriMesh : public TlFacetList
{
  public:
	TlTriMesh();
	TlTriMesh( TlVtxList *vlist );
	~TlTriMesh();

	int GetType( ) { return TRIMESH; }
	void AppendPrimitive( TlFacetList *prim );
	inline int GetNumPrimitives()
		{ return ( numPrimitives ); };

	void ToVertexIndices();
	void ToVertexPointers();

	Boolean HasNullData();
  	ostream& WriteSDF( ostream& os, TlMaterial *mat = NULL );
  	
  public:
  	TlFacetList *primitives;
  private:
  	TlFacetList *lastPrimitive;
  	unsigned int numPrimitives;
};

#endif // PRIMITIVES

