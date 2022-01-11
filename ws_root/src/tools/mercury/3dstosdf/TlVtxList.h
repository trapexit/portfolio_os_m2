/*
**	File:		TlVtxList.h
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
**		<1+>	 12/5/94	RRR		Enhance this class
**
**	To Do:
*/

#include "TlBasicTypes.h"

#ifndef VTX_LIST
#define VTX_LIST

#define VTX_MASK 7

class TlVtxList
{
  public:	
  	enum _Format {
  		COLORS = 0x01, 
  		NORMALS = 0x02, 
  		TEXTURES = 0x04,
		LOCATIONS = 0x08
	};
	BITFIELD( _Format, Format );
	
	TlVtxList( Format format, 
	                Int32 n, 
	                double* data );
	TlVtxList( Format format, 
	                Int32 n = 0 );
	TlVtxList( const TlVtxList& vl );
	~TlVtxList();

	void Expand( unsigned int newSize );
	void IndexVertices();
	
	Format	GetVertexFormat() const;
	char*	GetFormatString() const;
	static	Int32 VertexSize( Format format );
	Int32	GetVertexSize() const;
	inline Int32	GetVtxCount()
		{ return v->vtxCount; }
	inline Boolean HasNullData()
	{
			if ( v && (v->vtxData == NULL) ) return TRUE;
			else return FALSE;
	}
	inline double* GetVtxData() const
		{ return v->vtxData; }
	double* operator[]( Int32 ) const;

	TlVtxList& operator=( const TlVtxList& vlist );
	Boolean operator==(const TlVtxList& vlist ) const;
	ostream& WriteSDF( ostream& os, Boolean twoSided = FALSE, int *vcount = NULL );
	
	Point3D*		GetNormal( Int32 indx ) const;
	Point3D*		GetPosition(Int32) const;
	UVPoint*	GetTexCoord(Int32) const;
	Point3D*		GetPosition(void *vdata) const;
	UVPoint*	GetTexCoord(void *vdata) const;

	/* Work on these when needed - extract from pipeline
	Vector3D*	GetNormal(Int32) const;
	TlColor*	GetColor(Int32) const;
	double*	GetVertex(Int32) const;

	void	SetPosition(Int32, const Point3D*);
	void	SetPosition(Int32, const Point3D&);
	void	SetNormal(Int32, const Vector3D*);
	void	SetNormal(Int32, const Vector3D&);
	void	SetColor(Int32, const TlColor*);
	void	SetColor(Int32, const TlColor&);
	void	SetTexCoord(Int32, const float*);
	void	SetVertex(Int32, const int32*);
	*/

	// vertex list is shared only within a single surface
	// primitive. i.e. TlTriMesh
  private:
  	struct VtxEntry {
  		int     refCount;
		Format	vtxFormat;
		Int32	vtxCount;
		double*	vtxData;
		VtxEntry() { refCount = 1; }
	};
	
	VtxEntry *v;
};

#endif	// VTX_LIST
