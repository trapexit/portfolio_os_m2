/*
	File:		A3dsfile.cp

	Contains:	3D Studio 3.0 file reader

	Written by:	Ravindar Reddy

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

**		<5+>	 12/8/94	RRR		
**		 <5>	 12/2/94	RRR		Fix 4-byte alignment for UNIX
		<3+>	 11/8/94	RRR		Concatenate rotation from previous key and fix initial frame
		<2+>	 10/4/94	Reddy	To try fixing transformation matrix corresponding to keyframe
									data

	To Do:
*/

#include <string.h>
#include <ctype.h>
#include "TlA3dsfile.h"
#include "math.h"
#include <fstream.h>

#define NAME_ARRAY_INC 8

typedef struct __SmoothFacet
{
	TlSurface *surf;
	UInt32 smoothingGroup;
	UInt32 facetIndex;
} SmoothFacet;

extern "C" int
CompareSmoothFacets(
	const void *mem1,
	const void *mem2
	)
{	
	SmoothFacet *s1 = ( SmoothFacet * )mem1;
	SmoothFacet *s2 = ( SmoothFacet * )mem2;
	
	if( s1->surf == s2->surf )
	{
		if ( s1->smoothingGroup < s2->smoothingGroup )
			return (-1);
		else if ( s1->smoothingGroup > s2->smoothingGroup )
			return (1);
		else return (0);
	} else {
		if ( s1->surf < s2->surf )
			return (-1);
		else if ( s1->surf > s2->surf )
			return (1);
		else return (0);
	}	
}
	
static char
IsFloat
	(
	float val
	)
{
	if (val > 0.0)
		return 1;
	else if (val < 0.0)
		return 1;
	else if (val == 0.0)
		return 1;
	else
		return 0;
}

// number of animation frames
static Int32 maxFrames;

// 3DS filename used to identify texture and material array
static char *fileName3DS = NULL;

void
SetFileName3DS( const char *nm )
{
	if ( fileName3DS ) delete [] fileName3DS;
	fileName3DS = new char[ strlen( nm ) + 1 ];
	PRINT_ERROR( (fileName3DS==NULL), "Out of memory" );
	
	strcpy( fileName3DS, nm );
}

const char *GetFileName3DS()
{
	return fileName3DS;
}

/*
** A3dsfile::ReadData()
** Read binary data from the file
*/

Int32			
A3dsfile::ReadData( void	*outBuffer, 
Int32 numBytes )
{
	int bytesRead;
	
	bytesRead = fread( (char *) outBuffer, 
	                   (int) numBytes, 1,  fp ); 
	return( bytesRead );
}

/*
** A3dsfile::SetMarker(
** Set read marker in binary file
*/

void			
A3dsfile::SetMarker( Int32 offSet, 
int fromWhere)
{
	fseek( fp, offSet, fromWhere ); 
}

/*
** A3dsfile::GetMarker(
** Get read marker from binary file
*/

Int32
A3dsfile::GetMarker()
{
	return ( ftell( fp ) );
}	

/*				
** A3dsfile::A3dsfile()
** A3dsfile constructor
*/

A3dsfile::A3dsfile(
char *fileName )
{
	fp = fopen( fileName, "rb" );
	if ( fp == NULL ) {
		cout << "Can not open the file " << fileName << endl; 
		exit(0);
	}

	// Global material and texture list
	materials = NULL;
	textures = NULL;
	kfObjs = NULL;
	
	mdlNames = new a3dmdlName[ NAME_ARRAY_INC ];
	PRINT_ERROR( (mdlNames==NULL), "Out of memory" );
	mdlNum = 0;
	maxMdlNum = NAME_ARRAY_INC;

	prefix = NULL;
	writeTexRefs = TRUE;
	writeLights = TRUE;
	writeTextureStitiching = TRUE;
	
	/* by default set the smoothing groups off to increase the
	   performance achived by better snakification */
	writeSmoothingGroups = FALSE;
#if 0	
	// set the prefix to be concatenated
	char buf[100];
	sprintf( buf, "%s_", strtok(fileName,"." ) );
	prefix = new char[ strlen( buf ) + 1 ];
	PRINT_ERROR( (prefix==NULL), "Out of memory" );
	strcpy( prefix, buf );
#endif
}

/*				
** A3dsfile::~A3dsfile()
** A3dsfile destructor
*/

A3dsfile::~A3dsfile()
{
	TlMaterial *mcur = materials;
	TlTexture *tcur = textures;
	KFObject *kfcur = kfObjs;
	TlMaterial *mnext;
	TlTexture *tnext;
	KFObject *kfnext;

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
	while ( kfcur )
	{
 		kfnext = kfcur->next;
 		
 		// delete KFObject components
 		if ( kfcur->instName != NULL) 
 			delete[] kfcur->instName;
 			
 		if ( kfcur->mt != NULL) {
 			delete kfcur->mt->morphObjs;
 			delete kfcur->mt;
 		}
 		
 		if ( kfcur->pt != NULL) {
 			delete kfcur->pt->pnts;
 			delete kfcur->pt->spl;
 			delete kfcur->pt;
 		}
 			
  		if ( kfcur->rt != NULL) {
 			delete kfcur->rt->rots;
 			delete kfcur->rt->spl;
 			delete kfcur->rt;
 		}
 			
  		if ( kfcur->st != NULL) {
 			delete kfcur->st->scls;
 			delete kfcur->st->spl;
 			delete kfcur->st;
 		}
 			
		delete kfcur;
		
		kfcur = kfnext;
	}
	
	delete[] mdlNames;
	if ( prefix ) delete[] prefix;
	
	if ( fp != NULL ) fclose( fp ); 
}

/*
** A3dsfile::ReadModel()
** Opens, reads, and loads a model file
*/

ErrCode		
A3dsfile::ReadModel(
TlGroup *prnt )
{
	ErrCode		readErr;

	// Construct the model tree
	readErr = this->LoadModel( prnt );
	
	return readErr;
}

/*
** A3dsfile::ScanString()
** Scans the stream for a terminating null
*/

const char		
*A3dsfile::ScanString()
{
	int				i;
	Int32			bytesRead;
	static char		name[ 80 ];
	
	for ( i = 0; i < 80; i++ )
	{
		bytesRead = ReadData( &name[ i ], 1 );
		// First character cannot be a number '3' -> '_3'
		if( i == 0 && !isalpha( name[ i ] ) && ( name[ i ] != '\0' ) )
		{
			name[ i+1 ] = name[ i ]; ++i;
			name[ 0 ] = '_';			
		// Disallow blank space in a name ' ' -> '_'
		} if ( name[ i ] == ' ' ) name[ i ] = '_';
		// Disallow minus in a name '-' -> '_'
		else if ( name[ i ] == '-' ) name[ i ] = '_';
		// Disallow slash in a name '/' -> '_'
		else if ( name[ i ] == '/' ) name[ i ] = '_';
		
		if ( name[ i ] == '\0' )
			break;
	}
	
	return name;
}

/*
** to generate unique names concatenate the model number
*/
char*
A3dsfile::AddModelName( const char *mname )
{
	a3dmdlName *newData;
	static char mdlname[ NAME_SIZE_3DS ];
	
	if ( maxMdlNum <= mdlNum )
	{
		maxMdlNum += NAME_ARRAY_INC;
		newData = new a3dmdlName[ maxMdlNum ];
		PRINT_ERROR( (newData==NULL), "Out of memory" );
		memcpy( newData, mdlNames, mdlNum*sizeof(a3dmdlName) );
		
		delete[] mdlNames;
		mdlNames = newData;
	}
	strcpy( mdlNames[ mdlNum++ ], mname );
	
	// add prefix if exists
	if ( prefix ) sprintf( mdlname, "%s%s_%d", prefix, mname, mdlNum - 1 );
	else sprintf( mdlname, "%s_%d", mname, mdlNum - 1 );
 
	return mdlname;	
}		

/*
** to generate unique names concatenate the model number
*/
char*
A3dsfile::GetModelName( char *mname )
{
	int i;
	static char mdlname[ NAME_SIZE_3DS ];
	
	for ( i = 0; i < mdlNum; i++ )
	{
		if( !strcmp( mname, mdlNames[ i ] )  )
		{ 
			if ( prefix ) sprintf( mdlname, "%s%s_%d", prefix, mname, i );
			else sprintf( mdlname, "%s_%d", mname, i );
			return mdlname;
		}
	}
	/* you can have rogue models with names like - _$$$DUMMY so ignore them */
	/* printf( "Error : Can not find the name : %s\n", mname ); */
	return NULL;
}		

/*
** A3dsfile::ScanPoint()
** Scan point
*/

void		
A3dsfile::ScanPoint(
Point3D *p )
{
  float pt[3];
	ReadData( pt, 3*sizeof(float) );
	SwapInt32( (Int32 *)&pt[0] );
	SwapInt32( (Int32 *)&pt[1] );
	SwapInt32( (Int32 *)&pt[2] );
	p->x = pt[0];
	p->y = pt[1];
	p->z = pt[2];
	//    cout << p->x<<','<<p->y<<','<<p->z<<endl;
}

void
A3dsfile::ScanSplineKey( SplineProps *spl )
{
	spl->mUseFlag = ScanInt16();
	//cout << hex << spl->mUseFlag << dec << ',' ;
	if ( spl->mUseFlag & 0x01 ) {
		ReadData( &spl->mTension, 4 );
		SwapInt32( (Int32 *)&spl->mTension );
	} else spl->mTension = 0.0;
	//cout << "mTension :" << spl->mTension << endl;
	
	if ( spl->mUseFlag & 0x02 ) {
		ReadData( &spl->mContinuity, 4 );
		SwapInt32( (Int32 *)&spl->mContinuity );
	} else spl->mContinuity = 0.0;
	//cout << "mContinuity :" << spl->mContinuity << endl;
	
	if ( spl->mUseFlag & 0x04 ) {
		ReadData( &spl->mBias, 4 );
		SwapInt32( (Int32 *)&spl->mBias );
	} else spl->mBias = 0.0;
	//cout << "mBias :" << spl->mBias << endl;
	
	if ( spl->mUseFlag & 0x08 ) {
		ReadData( &spl->mEaseIn, 4 );
		SwapInt32( (Int32 *)&spl->mEaseIn );
	} else spl->mEaseIn = 0.0;
	//cout << "mEaseIn :" << spl->mEaseIn << endl;
	
	if ( spl->mUseFlag & 0x10 ) {
		ReadData( &spl->mEaseOut, 4 );
		SwapInt32( (Int32 *)&spl->mEaseOut );
	} else spl->mEaseOut = 0.0;
	//cout << "mEaseOut :" << spl->mEaseOut << endl;
}
	
/*
** A3dsfile::ScanChunk()
** Scans the next chunk in the stream
*/

void		
A3dsfile::ScanChunk(
a3dChunk		*chunk )
{
	Int32			bytesRead;
	
	// Scan the chunk's header
	chunk->offset.start = GetMarker();
	bytesRead = ReadData( &(chunk->header.ID), a3dHeaderSize );
	
	// Byte-swap header info
	SwapInt16( (Int16 *)&(chunk->header.ID) );
	SwapInt32( &chunk->header.size );

	chunk->offset.end = chunk->offset.start + chunk->header.size;
}

/*
** A3dsfile::ScanInt16()
** Scans a 2-byte value from the stream
*/

Int16		
A3dsfile::ScanInt16()
{
	Int32			bytesRead;
	static Int16	value;

	bytesRead = ReadData( &value, sizeof( Int16 ) );
	SwapInt16( &value );
	return value;
}

/*
** A3dsfile::ScanByte()
** Scans a 1-byte value from the stream
*/

Byte		
A3dsfile::ScanByte()
{
	static Byte	value;

	ReadData( &value, 1 );
	return value; 
}

/*
** A3dsfile::ScanInt32()
** Scans a 4-byte value from the stream
*/

void		
A3dsfile::ScanFloat32( float *flt )
{
	Int32			bytesRead;

	bytesRead = ReadData( flt, sizeof( Int32 ) );
	SwapInt32( (Int32 *)flt );
}

/*
** A3dsfile::ScanInt32()
** Scans a 4-byte value from the stream
*/

Int32		
A3dsfile::ScanInt32()
{
	Int32			bytesRead;
	static Int32	value;

	bytesRead = ReadData( &value, sizeof( Int32 ) );
	SwapInt32( &value );
	return value;
}

/*
** A3dsfile::SkipChunk()
** Moves the stream to the end of the chunk passed
*/

void		
A3dsfile::SkipChunk(
a3dChunk *chunk )
{
	SetMarker( chunk->offset.end, 0 );
}			

/*
** ParseTextureVertices();
** Scans the texture coordinates of the TlTriMesh model
** Object containes texture&position coordinates
*/

#define LARGE_TEXCOORDS 1.69474e+37

void	
A3dsfile::ParseTextureVertices( 
a3dChunk *chunk, 
TlSurface *surf )
{
	int				i;
	Int32			bytesRead;
	Int16			nThings = ScanInt16();
	Int32			bytesToRead = (Int32)nThings * sizeof( UVPointDS );
	MemPtr		ap( (unsigned int)bytesToRead );
	 
	UVPointDS	*a3dvs;
	TlFacetList *flist = surf->GetFacetList();
	VData1 *pos = ( VData1 *) flist->GetVtxList()->GetVtxData(); 
	Vdata2 *vd = (VData2 *) new char[ sizeof(VData2)*nThings ];	
	PRINT_ERROR( (vd==NULL), "Out of memory" );
	
	(void)&chunk;

	//	Allocate space for the vertex info and acquire it
	a3dvs = (UVPointDS *)ap.mPtr;
	
	bytesRead = ReadData( a3dvs, bytesToRead );
	//	Put data into TlVtxList data type
	for ( i = 0; i < nThings; i++ )
	{
		SwapInt32( (Int32 *)&a3dvs[ i ].u );
		SwapInt32( (Int32 *)&a3dvs[ i ].v );
		vd[ i ].indx = i;
		vd[ i ].pnt.x = pos[ i ].pnt.x;
		vd[ i ].pnt.y = pos[ i ].pnt.y;
		vd[ i ].pnt.z = pos[ i ].pnt.z;
		vd[ i ].uvp.u = a3dvs[ i ].u;
		vd[ i ].uvp.v = 1.0 - a3dvs[ i ].v;

		/* clamp huge rougue texture coordinates to 0.0 */
		if( fabs(vd[ i ].uvp.u) >= LARGE_TEXCOORDS ) vd[ i ].uvp.u = 0.0;
		if( fabs(vd[ i ].uvp.v) >= LARGE_TEXCOORDS ) vd[ i ].uvp.v = 0.0;
#if 0
		if( ( vd[ i ].uvp.u >= 1.69474e+37 ) || ( vd[ i ].uvp.v >= 1.69474e+37 ) )
		{
		fprintf( stderr, "U = %g, V = %g\n", vd[ i ].uvp.u, vd[ i ].uvp.v );
		fprintf( stderr, "+U = %g, +V = %g\n", 1 - vd[ i ].uvp.u, 1 - vd[ i ].uvp.v );
		fprintf( stderr, "-U = %g, -V = %g\n", 1.0 - HUGE_VAL, HUGE_VAL );
		}
#endif

	}
	
	TlVtxList	*vl = new TlVtxList ( TlVtxList::LOCATIONS | TlVtxList::TEXTURES, 
	                            nThings,
	                            (double *) vd );
	PRINT_ERROR( (vl==NULL), "Out of memory" );
	flist->SetVtxList( vl );
}	

/*
** DeleteTextureVertices();
** Delete the texture coordinates of the TlTriMesh model
** when there is no texture reference
*/

void	
A3dsfile::DeleteTextureVertices( 
	TlSurface *surf 
	)
{
	int				i;
	 
	TlFacetList *flist = surf->GetFacetList();
	TlVtxList *vlist = flist->GetVtxList();
	TlTexture *tex = surf->GetTexture();
	

	if ( !( vlist->GetVertexFormat() & TlVtxList::TEXTURES ) )
		return;

	// delete texture coordinates out of model
	if ( tex == NULL )
	{		
		Int16 nThings =  (Int16)vlist->GetVtxCount(); 
		VData2 *pos = ( VData2 *) vlist->GetVtxData(); 
		Vdata1 *vd = ( VData1 * ) new char[ sizeof(Vdata1) * nThings ];
		PRINT_ERROR( (vd==NULL), "Out of memory" );
	
		flist->ToVertexIndices();
		for ( i = 0; i < nThings; i++ )
		{
			vd[ i ].indx = pos[ i ].indx;
			vd[ i ].pnt.x = pos[ i ].pnt.x;
			vd[ i ].pnt.y = pos[ i ].pnt.y;
			vd[ i ].pnt.z = pos[ i ].pnt.z;
		}
		
		TlVtxList	*vl = new TlVtxList ( TlVtxList::LOCATIONS, 
		                            nThings,
		                            (double *) vd );
		PRINT_ERROR( (vl==NULL), "Out of memory" );
		
		flist->SetVtxList( vl );
		flist->ToVertexPointers();
	}
}	

/*
** Calculate the normals of the facets in the given surface.
** Store these normal information in the new vertex list.
** This vertex list will be used by the input surface later
*/
static void	
CalculateNormals( 
	TlSurface *surf,
	TlVtxList *vlist 
	)
{
	TlFacetList *flist= NULL;
	TlVertex *v1, *v2, *v3;
	TlFacet *cur, *next;
	Int32 iv1, iv2, iv3;
	Point3D *p0, *p1, *p2;
	Point3D *n0, *n1, *n2;
	Point3D vec1, vec2;
	double x, y, len;
	Int32 vcount = vlist->GetVtxCount();
	Int32 i;
	
	flist = surf->GetFacetList();

	// if the texture refernces exist then try stiching
	cur = flist->GetFirstFacet();	
	while ( cur )
	{
 		next = cur->GetNextFacet();
		
		// get the triangle vertices
		v1 = cur->GetFirstVertex();
		v2 = v1->GetNextVertex();
		v3 = v2->GetNextVertex();
		
		iv1 = v1->GetVertexDataIndx();
		iv2 = v2->GetVertexDataIndx();
		iv3 = v3->GetVertexDataIndx();
		
		p0 = vlist->GetPosition( iv1 );
		p1 = vlist->GetPosition( iv2 );
		p2 = vlist->GetPosition( iv3 );
		n0 = vlist->GetNormal( iv1 );
		n1 = vlist->GetNormal( iv2 );
		n2 = vlist->GetNormal( iv3 );

		/* calculate the vectors */
		vec1.x = p1->x - p0->x; vec1.y = p1->y - p0->y; vec1.z = p1->z - p0->z;
		vec2.x = p2->x - p1->x; vec2.y = p2->y - p1->y; vec2.z = p2->z - p1->z;
		/* cross product */
		x = vec1.y * vec2.z - vec1.z * vec2.y;
		y = vec1.z * vec2.x - vec1.x * vec2.z;
		vec1.z = vec1.x * vec2.y - vec1.y * vec2.x;
		vec1.x = x; vec1.y = y;
		/* normalize */
		len = sqrt( vec1.x * vec1.x + vec1.y * vec1.y + vec1.z * vec1.z );

		/* if this is a degenerate triangle then do accumalate it's contribution */
		if( !AEQUAL( len, 0.0 ) )
		{
			vec1.x /= len; vec1.y /= len; vec1.z /= len;
			/* add the normal to all vertices */
			n0->x += vec1.x; n1->x += vec1.x; n2->x += vec1.x;
			n0->y += vec1.y; n1->y += vec1.y; n2->y += vec1.y;
			n0->z += vec1.z; n1->z += vec1.z; n2->z += vec1.z;
		}
		cur = next;
	}

	/* normalize all the normal coordinates */
	for( i = 0; i < vcount; i++ )
	{
		n0 = vlist->GetNormal( i );
		len = sqrt( n0->x * n0->x + n0->y * n0->y + n0->z * n0->z );
		if ( ! AEQUAL( len, 0.0 ) )
		{
			n0->x /= len; n0->y /= len; n0->z /= len;
		}
	}
}

/*
** StichTextureSeams();
** Stitch the texture seams if the geometry has cylindrical or spherical
** texture mapping. The surface passed is the head of the model surface
** list.
** Stitching introduces lonely vertices ( no sharing ) thus causing 
** the facets to be flat shaded ( normal calculation without vertex 
** sharing ). For this reason normals needs to be pre-calculated to 
**  avoid this problem.
*/

static void	
StitchTextureSeams( 
	TlSurface *in_surf,
	a3dTexInfo *tex_info 
	)
{
	int				i;
	unsigned int ulen;
	TlSurface *surf;
	TlVtxList *vlist;
	TlFacetList *flist;
	TlSurface *next_surf;
	TlTexture *tex;
	TlFacet *cur, *next;
	TlVertex *v1, *v2, *v3;
	UVPoint *t1, *t2, *t3;
	int tindex = 0;
	double area;
	Int32 vsize, new_vsize;
	Int32 iv1, iv2, iv3;
	Int32 new_count, vcount;
	TlVtxList::Format new_format;
	TlVtxList *new_vlist = NULL;
	double *new_vdata = NULL, *vdata = NULL, *tmp_vdata = NULL;
	double half_tilex = tex_info->tileX / 2.0;
	
	surf = in_surf;

	if ( ( in_surf == NULL) ||
		( tex_info->type == 0 ) )		// planar mapping inwhich case
										// this problem will not exist
		return;

	while ( surf )
	{
		next_surf = surf->GetNextSurface();
		flist = surf->GetFacetList();
		vlist = flist->GetVtxList();
		vlist->IndexVertices();
		tex = surf->GetTexture();
		tindex = 0;

		// if the texture refernces exist then try stiching
		if ( ( tex != NULL ) && ( vlist->GetVertexFormat() & TlVtxList::TEXTURES ) )
		{	
			cur = flist->GetFirstFacet();	
			while ( cur )
			{
		 		next = cur->GetNextFacet();
				
				// get the triangle vertices
				v1 = cur->GetFirstVertex();
				v2 = v1->GetNextVertex();
				v3 = v2->GetNextVertex();
				
				t1 = vlist->GetTexCoord( v1->GetVertexData());
				t2 = vlist->GetTexCoord( v2->GetVertexData() );
				t3 = vlist->GetTexCoord( v3->GetVertexData() );

				area = (t2->u - t1->u) * (t3->v - t1->v) - (t2->v - t1->v) * (t3->u - t1->u);
				
				if( !( AEQUAL(area, 0.0 ) ||
						( AEQUAL( t1->v, t2->v )  &&
						  AEQUAL( t1->v, t3->v )  &&
						  AEQUAL( t2->v, t3->v ) ) ) )
				{
					if ((fabs(t1->u-t2->u) > half_tilex) || 
					    (fabs(t2->u-t3->u) > half_tilex) || 
					    (fabs(t1->u-t3->u) > half_tilex))
					{
						if ((t1->u <= t2->u) && (t1->u <= t3->u))
						{
							tindex++;
						} else if ((t2->u <= t1->u) && (t2->u <= t3->u)) {
							tindex++;
						} else if ((t3->u <= t1->u) && (t3->u <= t2->u)) {
							tindex++;
						}
					}
					if ((fabs(t1->u-t2->u) > half_tilex) || 
					    (fabs(t2->u-t3->u) > half_tilex) || 
					    (fabs(t1->u-t3->u) > half_tilex))
					{
						if ((t1->u <= t2->u) && (t1->u <= t3->u))
						{
							tindex++;
						} else if ((t2->u <= t1->u) && (t2->u <= t3->u)) {
							tindex++;
						} else if ((t3->u <= t1->u) && (t3->u <= t2->u)) {
							tindex++;
						}
					}
				}
				
				cur = next;
			}

			if ( tindex != 0 )
			{
				fprintf( stderr, "WARNING: Duplicated %d vertices to stitch texture seam\n", tindex );
				// convert to have vertex indices
				flist->ToVertexIndices();
	
				/* if the surface has a seam then create a new vertex list */
				vcount = vlist->GetVtxCount();
				vsize = vlist->GetVertexSize();
				new_format = vlist->GetVertexFormat() | TlVtxList::NORMALS;
				new_vsize = TlVtxList::VertexSize( new_format );	
				new_count = vcount + tindex;
				new_vdata = new double[new_count * new_vsize];
				PRINT_ERROR( (new_vdata==NULL), "Out of memory" );
				
				/* Warning: by the time you come here you both texture coordinates and 
				** positions. In the new vertex list we create enough space for normals
				*/
				tmp_vdata = new_vdata;
				vdata = vlist->GetVtxData();
				for( i = 0; i < vcount; i++ )
				{
					/* copy vertex index + position */
					memcpy( tmp_vdata, vdata, 4 * sizeof(double) );
					/* clear the normal data to zero */
					memset( tmp_vdata+4, 0, 3 * sizeof(double) );
					/* copy texture coordinates */
					memcpy( tmp_vdata+7, vdata+4, 2 * sizeof(double) );
					
					tmp_vdata += new_vsize;
					vdata     += vsize;
				}
				new_vlist  = new TlVtxList( new_format, new_count, new_vdata );
	
				/* calculate the normals for all TriMeshes */
				CalculateNormals( surf,  new_vlist );
				
				cur = flist->GetFirstFacet();	
				while ( cur )
				{
			 		next = cur->GetNextFacet();
					
					// get the triangle vertices
					v1 = cur->GetFirstVertex();
					v2 = v1->GetNextVertex();
					v3 = v2->GetNextVertex();
					
					iv1 = v1->GetVertexDataIndx();
					iv2 = v2->GetVertexDataIndx();
					iv3 = v3->GetVertexDataIndx();
					
					
					t1 = vlist->GetTexCoord( iv1);
					t2 = vlist->GetTexCoord( iv2 );
					t3 = vlist->GetTexCoord( iv3 );
	
					area = (t2->u - t1->u) * (t3->v - t1->v) - (t2->v - t1->v) * (t3->u - t1->u);
					
					ulen = ( unsigned int)new_vsize * sizeof(double);
					if( !( AEQUAL(area, 0.0 ) ||
							( AEQUAL( t1->v, t2->v )  &&
							  AEQUAL( t1->v, t3->v )  &&
							  AEQUAL( t2->v, t3->v ) ) ) )
					if ((fabs(t1->u-t2->u) > half_tilex) || 
					    (fabs(t2->u-t3->u) > half_tilex) || 
					    (fabs(t1->u-t3->u) > half_tilex))
					{							
						if ((fabs(t1->u-t2->u) > half_tilex) || 
						    (fabs(t2->u-t3->u) > half_tilex) || 
						    (fabs(t1->u-t3->u) > half_tilex))
						{
							if ((t1->u <= t2->u) && (t1->u <= t3->u))
							{
								vdata = &new_vdata[vcount*new_vsize];
								memcpy( vdata, (*new_vlist)[iv1], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v1->SetVertexDataIndx( vcount );
								t1 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t1->u += tex_info->tileX;
							} else if ((t2->u <= t1->u) && (t2->u <= t3->u)) {
								vdata = &new_vdata[(vcount)*new_vsize];
								memcpy( vdata, (*new_vlist)[iv2], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v2->SetVertexDataIndx( vcount );
								t2 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t2->u += tex_info->tileX;
							} else if ((t3->u <= t1->u) && (t3->u <= t2->u)) {
								vdata = &new_vdata[(vcount)*new_vsize];
								memcpy( vdata, (*new_vlist)[iv3], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v3->SetVertexDataIndx( vcount );
								t3 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t3->u += tex_info->tileX;
							}
						}
						if ((fabs(t1->u-t2->u) > half_tilex) || 
						    (fabs(t2->u-t3->u) > half_tilex) || 
						    (fabs(t1->u-t3->u) > half_tilex))
						{
							if ((t1->u <= t2->u) && (t1->u <= t3->u))
							{
								vdata = &new_vdata[vcount*new_vsize];
								memcpy( vdata, (*new_vlist)[iv1], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v1->SetVertexDataIndx( vcount );
								t1 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t1->u += tex_info->tileX;
							} else if ((t2->u <= t1->u) && (t2->u <= t3->u)) {
								vdata = &new_vdata[(vcount)*new_vsize];
								memcpy( vdata, (*new_vlist)[iv2], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v2->SetVertexDataIndx( vcount );
								t2 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t2->u += tex_info->tileX;
							} else if ((t3->u <= t1->u) && (t3->u <= t2->u)) {
								vdata = &new_vdata[(vcount)*new_vsize];
								memcpy( vdata, (*new_vlist)[iv3], ulen );
								((VertexPtr *) vdata)->indx = vcount;
								v3->SetVertexDataIndx( vcount );
								t3 = new_vlist->GetTexCoord( vcount );
								vcount++;
								t3->u += tex_info->tileX;
							}
						}
					}
					cur = next;
				}
		
				// convert to have vertex indices of the new vertex list
				flist->SetVtxList( new_vlist );
					
				flist->ToVertexPointers();
			}
		}
		surf = next_surf;
	}
}	
				
/*
**  Procedure     : ParseMeshTextureInfo();
**	Description   : Scans the texture map information
**  Notes         :	
*/
		 
void		
A3dsfile::ParseMeshTextureInfo( 
a3dChunk *chunk,
a3dTexInfo *info )
{
	int			i;
	Int32			bytesRead;
	Int32			bytesToRead = 21 * sizeof(float);
	float 			*data;

	(void)&chunk;

	info->type = ScanInt16();
	
	data = &info->tileX;
	bytesRead = ReadData( data, bytesToRead );
	for ( i = 0; i < 21; i++ )
	{
		SwapInt32( (Int32 *)&data[i] );
	}
}

/*
**  Procedure     : ParseTextureMap();
**	Description   : Scans the texture map data
**  Notes         :	
*/

void
A3dsfile::ParseTextureMap(  
a3dChunk *headmaster, 
const char *name )
{
	a3dChunk chunk;
	TlTexture *tex = new TlTexture( name );
	Int16 strength;
	const char *tex_file = NULL;

	PRINT_ERROR( (tex==NULL), "Out of memory" );
		
	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_IntPercentage: // TlTexture map strength
				strength = ScanInt16();
				break;
			case chunkid_MatMapName: // TlTexture map name
				tex_file = ScanString();
				if( strlen( tex_file ) != 0 )
					tex->SetFileName( tex_file );					
				else {
					delete tex;
					return;
				}	
				break;
			case chunkid_MatTexTiling:
				strength = ScanInt16();
				if (strength & 0x10) {
					tex->SetxWrapMode( TRUE );
					tex->SetyWrapMode( TRUE );
				}
#if 0
				if (strength & 0x1) cerr << "decal, " << endl;
				if (strength & 0x2) cerr << "mirror, " << endl;
				if (strength & 0x8) cerr << "negation, " << endl;
				if (strength & 0x10) cerr << "deactivate tiling, " << endl;
				if (strength & 0x20) cerr << "area sampling, " << endl;
				if (strength & 0x40) cerr << "activate alpha, " << endl;
				if (strength & 0x80) cerr << "tinting, " << endl;
				if (strength & 0x100) cerr << "ignore alpha, " << endl;
				if (strength & 0x200) cerr << "RGB tinting, " << endl;
#endif
				break;
			default:
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
	// Add the texture map info to the global list
	AddTexture( tex );
}

/*
** A3dsfile::LoadModel()
** Loads 3DS data into scene object
*/

ErrCode		
A3dsfile::LoadModel(
TlGroup *prnt )
{
	a3dChunk		chunk;
	ErrCode			readErr = noError;
	
	// Rewind to the start of the raw chunk stream
	this->SetMarker( 0, 0 );
	
	// Identify chunk types and assign them to the model
	ScanChunk( &chunk );
	
	// If the object is extracted out of 3DS file then there
	// will not be any MAGIC file header, starts with MDATA chunk
	if ( chunk.header.ID == chunkid_StartModel )
		this->SetMarker( 0, 0 );
		
	if ( ( chunk.header.ID == chunkid_3DSHeader ) || 
	     ( chunk.header.ID == chunkid_StartModel ) )
	{
		// Parse the 3DS file
		Parse3DS( &chunk, prnt );
		return noError;
	}
	else return ioError;		
}

/*
** A3dsfile::Parse3DS()
** Parses chunks subordinate to a 3DS chunk
*/

void
A3dsfile::Parse3DS(
a3dChunk *headmaster,
TlGroup *prnt )
{
	a3dChunk		chunk;
	
	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_StartModel:
				ParseModel( &chunk, prnt );
				break;
			
			case chunkid_KFData:
				ParseKFData( &chunk );
				break;
			
			case chunkid_3DSProlog:
				break;
				
			default:
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
	// Build KeyFrame object hierarchy
	BuildKFObjHierarchy( prnt );
}

/*
** A3dsfile::ParseMeshMatrix()
** Parse Mesh Object matrix
*/
void			
A3dsfile::ParseMeshMatrix( TlTransform *tf )
{
	float transf[12];
	Int32 bytesRead;
	int i;
		
	bytesRead = ReadData( transf, 12 * sizeof( Int32 ) );
	for ( i = 0; i < 12; i++ ) 
		SwapInt32( (Int32 *)&transf[ i ] );
		
	tf->SetVal( 0, 0, transf[ 0 ] );
	tf->SetVal( 0, 1, transf[ 1 ] );
	tf->SetVal( 0, 2, transf[ 2 ] );
		
	tf->SetVal( 1, 0, transf[ 3 ] );
	tf->SetVal( 1, 1, transf[ 4 ] );
	tf->SetVal( 1, 2, transf[ 5 ] );
	
	tf->SetVal( 2, 0, transf[ 6 ] );
	tf->SetVal( 2, 1, transf[ 7 ] );
	tf->SetVal( 2, 2, transf[ 8 ] );
	
	tf->SetVal( 3, 0, transf[ 9 ] );
	tf->SetVal( 3, 1, transf[ 10 ] );
	tf->SetVal( 3, 2, transf[ 11 ] );
	
	// tf->WriteSDF( cout );
	*tf = tf->Invert();
}

/*
** A3dsfile::ParseColor()
** Processes various color chunk formats
*/

void		
A3dsfile::ParseColor(
TlColor	*color )
{
	a3dChunk		chunk;
	Byte			r, g, b;
	float tmp;

	ScanChunk( &chunk );
	switch( chunk.header.ID )
	{
		case chunkid_ColorFloat:
			ScanFloat32( &tmp );
			color->r = tmp;
			ScanFloat32( &tmp );
			color->g = tmp;
			ScanFloat32( &tmp );
			color->b = tmp;
#if 0
			color->r =(double) ((float)ScanInt32());
			color->g =(double) ((float)ScanInt32());
			color->b =(double) ((float)ScanInt32());
			fprintf( stderr, "Color%g %g %g\n", color->r, color->g, color->b  );
#endif
			break;
			
		case chunkid_ColorByte:
			r = ScanByte();
			g = ScanByte();
			b = ScanByte();
			
			color->r = (float)r / 255.;
			color->g = (float)g / 255.;
			color->b = (float)b / 255.;
			break;
		default:	
			break;
	}
	SkipChunk( &chunk );
}

/*
** A3dsfile::ParseTransparency()
** Extract transparency data
*/

void
A3dsfile::ParseTransparency( TlMaterial *mat )
{
	a3dChunk		chunk;
	float			transp;

	ScanChunk( &chunk );
	switch( chunk.header.ID )
	{
		case chunkid_IntPercentage:
			transp = ( (float)ScanInt16() ) / 100.0;
			mat->SetTransparency( transp ); 
			break;
			
		default:	
			break;
	}
	SkipChunk( &chunk );
}

void
A3dsfile::ParsePercentage( float *percent )
{
  a3dChunk		chunk;
  float			pct;
  
  ScanChunk( &chunk );
  switch( chunk.header.ID )
    {
    case chunkid_IntPercentage:
      pct = ( (float)ScanInt16() ) / 100.0;
      *percent = pct; 
      break;
      
    default:	
      break;
    }
  SkipChunk( &chunk );
}
			
/*
** A3dsfile::ParseFaceMaterials()
** Derives facet material assignments
*/

void
A3dsfile::ParseFaceMaterials(
TlSurface *surf,
TlSurface **surfs )
{
	int				i;
	const char		*nom;
	Int16			nAssigned, facet;
	TlMaterial		*mat;
	TlTexture			*tex;
	TlVtxList			*vlist = surf->GetFacetList()->GetVtxList();
	TlSurface *cur;
	char buf[100];
	
	nom = ScanString();
	nAssigned = ScanInt16();
	
	// Share the vlist, mat, tex in a new surface with group of 
	// facets having same texture and material properties
	// If this group does not have any facets do not create any
	// surface entity. This will happen in case of box mappinf in 3DS.
	
	if ( nAssigned > 0 )
	{
		if ( prefix ) sprintf( buf, "%s%s", prefix, nom );
		else sprintf( buf, "%s", nom );
		
		// get the material
		mat = GetMaterial( buf );
		
		// if the texture needs to be written out then get it
		if ( writeTexRefs == TRUE ) tex = GetTexture( buf );
		else tex = NULL;
		
		cur = new TlSurface( vlist, mat, tex );
		PRINT_ERROR( (cur==NULL), "Out of memory" );
		
		surf->AppendSurface( cur );
		for ( i = 0; i < nAssigned; i++ )
		{
			facet = ScanInt16();
			surfs[ facet ] = cur;
		}
	}
}

/*
** Caveat : even if the facet belongs to multiple smoothing groups
** only the one ( the first one ) is used to put it in a group
*/
static UInt32 
FindSmoothingGroup(
UInt32 smootingBits )
{
	UInt32 i, result = 0;
	UInt32 mask = 0x0001;
	UInt32 found = 0;
	
	for ( i = 0; (i < 32 ) && ( found == 0 ); i++ )
	{
		if ( ( smootingBits >> i ) & mask )
		{
			result = i;
			found = 1;
		}
	}
	
	return result;
}

/*
** A3dsfile::ParseFaceMaterials()
** Derives facet material assignments
*/

void
A3dsfile::ParseSmoothingGroups(
TlSurface *surf,
TlSurface **surfs,
Int16 nThings )
{	
	Int32 bytesRead, i;  
	Int32 bytesToRead = nThings * sizeof( UInt32 );  
	MemPtr	mp( (unsigned int)nThings * sizeof( UInt32 ) );
	UInt32  *facet_indices = (UInt32  *)mp.mPtr;
	MemPtr	sf( (unsigned int)nThings * sizeof( SmoothFacet ) );
	SmoothFacet  *smooth_facets = (SmoothFacet  *)sf.mPtr;
	TlSurface *cur_surf, *prev_surf;
	UInt32 prev_group;
	TlMaterial *mat;
	TlTexture *tex;
	TlVtxList *vlist; 
	
	// Acquire the quad info
	bytesRead = ReadData( facet_indices, bytesToRead );
	
	for ( i = 0; i < nThings; i++ )
	{
		SwapInt32( (Int32*)&facet_indices[i] );
		
		smooth_facets[i].surf = surfs[i];
		smooth_facets[i].smoothingGroup = FindSmoothingGroup ( facet_indices[i] );;
		smooth_facets[i].facetIndex = i;
	}
	
	/* sort based on surface ( with same material ) and smoothing Group of the facet */
	qsort( smooth_facets, nThings, sizeof( SmoothFacet ), CompareSmoothFacets );
		
	cur_surf = prev_surf = smooth_facets[0].surf;
	prev_group = smooth_facets[0].smoothingGroup;
	
	for ( i = 1; i < nThings; i++ )
	{
		if ( prev_surf == smooth_facets[i].surf )
		{
			if( prev_group != smooth_facets[i].smoothingGroup )
			{
#if 0	
				fprintf( stderr, "Prv G[%d] = %d, Cur G = %d\n",
					 i, prev_group, smooth_facets[i].smoothingGroup );
#endif
				mat = prev_surf->GetMaterial();
				tex = prev_surf->GetTexture();
				vlist = prev_surf->GetFacetList()->GetVtxList();
				
				cur_surf = new TlSurface( vlist, mat, tex );
				PRINT_ERROR( (cur_surf==NULL), "Out of memory" );
				surf->AppendSurface( cur_surf );
				prev_group = smooth_facets[i].smoothingGroup;
			}
		} else {
			cur_surf = prev_surf = smooth_facets[i].surf;
			prev_group = smooth_facets[i].smoothingGroup;
		}
		surfs[smooth_facets[i].facetIndex] = cur_surf;
#if 0	
		fprintf( stderr, "--------- Assigned Surf = 0x%x\n",
			surfs[smooth_facets[i].facetIndex] );
			
		fprintf( stderr, "Surf = 0x%x, fi = %d, sg = %d\n",
			smooth_facets[i].surf,
			smooth_facets[i].facetIndex,
			smooth_facets[i].smoothingGroup );
#endif
	}
}

/*
** A3dsfile::ParseFacetList()
** Scans the contents of a facet list
*/

void
A3dsfile::ParseFacetList(
a3dChunk *chunk,
TlSurface *surf )
{
	int i, j;
	Int32 bytesRead;
	Int16 nThings = ScanInt16();
	Int32 bytesToRead = nThings * sizeof( a3dFacet ); 	
	TlFacetList *flist = surf->GetFacetList();
	TlVtxList *vlist = flist->GetVtxList();
	TlFacet *fct;
	TlVertex *vtx;
	Int16 indx;
	int ccw_poly = 0;
	
	// Allocate enough surface pointers as the number of
	// facets in a trimesh. Later allocate surfaces equal
	// to the number of TlMaterial groups
	MemPtr	mp( (unsigned int)nThings * sizeof( TlSurface* ) );
	MemPtr	ap( (unsigned int)nThings * sizeof( a3dFacet ) );

	// Allocate space for all materials & triangles info	
	a3dFacet *a3dqs = (a3dFacet *)ap.mPtr;
	TlSurface **surfs = (TlSurface **)mp.mPtr;

	// Initialize the facet with surface it belongs to
	for ( i = 0; i < nThings; i++ ) surfs[ i ] = surf;
	
	// Acquire the quad info
	bytesRead = ReadData( a3dqs, bytesToRead );
	
	// Initialise all the surfs with TlMaterial and TlTexture info	
	ParseMaterialList( chunk, surf, surfs, nThings );

	int nverts = (int)flist->GetVtxList()->GetVtxCount();
	// Confine out-of-range indices to the 0th vertex
	// Create a triangle polygon and add it to TlFacetList
	for ( i = 0; i < nThings; i++ )
	{
		// check if it is a degenarate polygon 
		if( (a3dqs[i].vtx[0] == a3dqs[i].vtx[1]) || 
			(a3dqs[i].vtx[0] == a3dqs[i].vtx[2]) ||
			(a3dqs[i].vtx[1] == a3dqs[i].vtx[2]) )
			fprintf( stderr, "WARNING: Degenerate polygon ( V%d, V%d, V%d ) \n",
				a3dqs[i].vtx[0], a3dqs[i].vtx[1], a3dqs[i].vtx[2] );
		else {
			fct = new TlFacet();
			PRINT_ERROR( (fct==NULL), "Out of memory" );
			
						
			// 3DS poly is clock-wise so invert the vertex order
			if ( ccw_poly )
			{
				for ( j = 2; j >= 0; j-- )
				{
					indx = a3dqs[ i ].vtx[ j ];
					SwapInt16( &indx );
					if ( indx >= nverts ) {
						cout << "WARNING: Out-of-range triangle index:" << indx << endl;
						indx = 0;
					}
					vtx = new TlVertex( *vlist, (Int32)indx );
					PRINT_ERROR( (vtx==NULL), "Out of memory" );
					fct->AppendVertex( vtx );
				}
			} else {
				for ( j = 0; j < 3; j++ )
				{
					indx = a3dqs[ i ].vtx[ j ];
					SwapInt16( &indx );
					if ( indx >= nverts ) {
						cout << "WARNING: Out-of-range triangle index:" << indx << endl;
						indx = 0;
					}
					vtx = new TlVertex( *vlist, (Int32)indx );
					PRINT_ERROR( (vtx==NULL), "Out of memory" );
			
					fct->AppendVertex( vtx );
				}
			}
			
			surfs[ i ]->GetFacetList()->AppendFacet( fct );
		}
	}
}

/*
** A3dsfile::AddMaterial()
** Add material to the global list
*/ 

void
A3dsfile::AddMaterial( TlMaterial *mat )
{
	static TlMaterial *last = materials;
		
	if ( materials == NULL ) materials = mat;
	else last->SetNextMaterial( mat );
	last = mat;
}

/*
** A3dsfile::GetMaterial() - dumb lookup
** Lookup material name in the global list
*/ 

TlMaterial*
A3dsfile::GetMaterial( const char* name )
{
	int result;
	TlMaterial *mat = materials;

	while ( mat )
	{
		result = strcmp( mat->GetName(), name );
		if ( result == 0 ) return mat;
		mat = mat->GetNextMaterial();
	}
		
	return ( (TlMaterial *)NULL );
}

/*
** A3dsfile::AddTexture()
** Add texture to the global list
*/ 

void
A3dsfile::AddTexture( TlTexture *tex )
{
	static TlTexture *last = textures;
		
	if ( textures == NULL ) textures = tex;
	else last->SetNextTexture( tex );
	last = tex;
}

/*
** A3dsfile::GetTexture() - dumb lookup
** Lookup texture name in the global list
*/ 

TlTexture*
A3dsfile::GetTexture( const char* name )
{
	int result;
	TlTexture *tex = textures;

	while ( tex )
	{
		result = strcmp( tex->GetName(), name );
		if ( result == 0 ) return tex;
		tex = tex->GetNextTexture();
	}
	return ( (TlTexture *)NULL );
}

/*
** A3dsfile::AddKFObject()
** Add KFObject to the global list
*/ 

void
A3dsfile::AddKFObject( KFObject *obj )
{
	static KFObject *last = kfObjs;
		
	if ( kfObjs == NULL ) kfObjs = obj;
	else last->next = obj;
	last = obj;
}

/*
** A3dsfile::GetKFObject - dumb lookup
** Lookup KFObject name in the global list
*/ 

KFObject*
A3dsfile::GetKFObject( Int16 ID )
{
	KFObject *obj = kfObjs;
	
	if ( ID >= 0 )
	{
		while ( obj )
		{
			if ( obj->nodeID == ID ) return obj;
			obj = obj->next;
		}
	}
	return ( (KFObject *)NULL );
}

/*		
** A3dsfile::ParseMaterialEntry()
** Parses chunks subordinate to a material definition
*/

void		
A3dsfile::ParseMaterialEntry(
a3dChunk *headmaster )

{
	a3dChunk		chunk;
	TlColor			color;
	TlMaterial		*mat;
	float                   percent;
	int 			shadeType;
	char			mat_name[100];
		
	do
	{
		ScanChunk( &chunk );
		if ( chunk.offset.end <= headmaster->offset.end )
		{ 
			switch( chunk.header.ID )
			{
			case chunkid_MatName:
				if ( prefix ) sprintf( mat_name, "%s%s", prefix, ScanString() );
				else sprintf( mat_name, "%s", ScanString() );
				mat = new TlMaterial( mat_name );
				PRINT_ERROR( (mat==NULL), "Out of memory" );
				//printf( " Material NAME : %s\n", mat->GetName() );
				break;
			
			case chunkid_MatAmbient:
				ParseColor( &color );
				mat->SetAmbient( color );
				break;
				
			case chunkid_MatDiffuse:
				ParseColor( &color );
				mat->SetDiffuse( color );
				break;
				
			case chunkid_MatSpecular: 
			  ParseColor( &color );
			  mat->SetSpecular( color );
			  break;
				
			case chunkid_MatShininess: 
			  ParsePercentage (&percent);
			  mat->SetShininess(percent);
			  break;

			case chunkid_MatShinePct:
			  ParsePercentage (&percent);
			  mat->SetShineStrength(percent);
			  break;
				
			case chunkid_MatTransparency:
				ParseTransparency( mat );
				break;

			case chunkid_MatShading:
				shadeType = ScanInt16();
				mat->SetShading( shadeType );
				break;
				
			case chunkid_MatTwoSided:
				mat->SetTwoSided( TRUE );
				break;
				
			case chunkid_MatDecal:
				//fprintf( stderr, "Decal flag set on : %s\n", mat->GetName() );
				break;
				
			case chunkid_MatAdditive:
				//fprintf( stderr, "Additive flag set on : %s\n", mat->GetName() );
				break;
				
			case chunkid_MatTexMap: // First texture map
				ParseTextureMap( &chunk, mat->GetName() );
				break;	

			default: 
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );

	// Add the material info to the global list
	AddMaterial( mat );
}

/*
** A3dsfile::ParseMaterialList()
** Parses a facet material list
*/

void
A3dsfile::ParseMaterialList(
a3dChunk *headmaster,
TlSurface *surf,
TlSurface **surfs,
Int16 nThings )
{
	a3dChunk		chunk;
		
	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_MaterialList:
				ParseFaceMaterials( surf, surfs );
				break;

			case chunkid_SmoothList:
				if( writeSmoothingGroups == TRUE ) 
					ParseSmoothingGroups( surf, surfs, nThings );
				break;
			default:
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
}

/*
** A3dsfile::ParseModel()
** Parses chunks subordinate to a model chunk
*/

void		
A3dsfile::ParseModel(
a3dChunk *headmaster,
TlGroup *prnt )
{
	a3dChunk		chunk;
	
	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_Material:
				ParseMaterialEntry( &chunk );
				break;
			
			case chunkid_NamedObject:
				ParseNamedObject( &chunk, prnt );
				break;
								
			case 0x2100: // Global ambient color
			case 0x1200: // Background color
			case 0x1201: // Write color info
			case 0x2200: // Fog
			case 0x2210: // Fog background
			case 0x2201: // Write fog info
			     break;
			
			default:
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
}

/*
** A3dsfile::ParseNamedObject()
** Parses a named chunk type and its subordinates
*/

void		
A3dsfile::ParseNamedObject(
a3dChunk *headmaster,
TlGroup *parent )
{
	a3dChunk		chunk;
	const char		*nom;
	
	// Scan off object name
	nom = AddModelName( ScanString() );
	do
	{
		ScanChunk( &chunk );
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_Trimesh:
				ParseTrimesh( &chunk, parent, nom );
				break;
			
			case chunkid_Light:
				if( writeLights == TRUE ) ParseLight( &chunk, parent, nom );
				break;
				
			case 0x4700: // Camera
			case 0x4010: // Hidden mesh
			case 0x4012: // Shadow mesh
			     break;
			
			default:
				break;
			}
		}
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
}

/*
** A3dsfile::ParseLight()
** Parse 3DS Light object
*/

void		
A3dsfile::ParseSpotLight( 
a3dChunk *headmaster,
TlLight *lgt )
{
	a3dChunk		chunk;
	float tmpFloat;

	// create a TlLight
	lgt->m->spotLght = new TlSpotLight;
	PRINT_ERROR( (lgt->m->spotLght==NULL), "Out of memory" );
			
	ScanPoint( &(lgt->m->spotLght->target) );
	ScanFloat32( &tmpFloat);		
	lgt->m->spotLght->coneAngle = tmpFloat;		
	ScanFloat32( &tmpFloat );		
	lgt->m->spotLght->fallOff = tmpFloat;		

	do
	{
		// Get the next chunk from the stream
		ScanChunk( &chunk );
		// Proceed if we're still within the bounds 
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			default:
				break;
			}
		}
		// Discard any unprocessed portion of the current chunk
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
}

/*
** A3dsfile::ParseLight()
** Parse 3DS Light object
*/

void		
A3dsfile::ParseLight( 
a3dChunk *headmaster,
TlGroup *parent,
const char *nom )
{
	a3dChunk		chunk;
	
	// create a TlLight
	TlLight *mdl = new TlLight( nom );
	PRINT_ERROR( (mdl==NULL), "Out of memory" );
			
	ScanPoint( &(mdl->m->pos) );		
	ParseColor( &(mdl->m->lgtColor) );

	do
	{
		// Get the next chunk from the stream
		ScanChunk( &chunk );
		// Proceed if we're still within the bounds 
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{			
			case chunkid_LightOff:
				mdl->m->lgtOff = TRUE;
				break;
				
			case chunkid_SoftLight:
				ParseSpotLight( &chunk, mdl );
				break;
							 	
			default:
				break;
			}
		}
		// Discard any unprocessed portion of the current chunk
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
	
	parent->AddChild( ( TlCharacter * )mdl );	
}

/*
** Slap the model transform to the vertices and make it unit matrix
*/
static void 
TransfModelVertices( TlModel *mdl )
{
	int i;
	Point3D  *p;
	TlSurface *surf = mdl->GetSurface();
	if ((surf->GetFacetList()) != NULL)
	  {
	    TlVtxList *vlist = surf->GetFacetList()->GetVtxList();
	    int vsize = (int)vlist->GetVertexSize();
	    int vcount = (int)vlist->GetVtxCount();	
	    double *vd = vlist->GetVtxData();
	    
	    // update the texture coordinates to new sub-texture region
	    for ( i = 0; i < vcount; i++, vd += vsize )	
	      {
		p = vlist->GetPosition( vd );
		*p = *p * mdl->transf;
	      }
	  }
	
	/* set the model transform to unit matrix */
	TlTransform tr;
	mdl->transf = tr;
}


/*
** A3dsfile::ParseTrimesh()
** Parse 3DS  TRI_MESH object
*/
void		
A3dsfile::ParseTrimesh( 
a3dChunk *headmaster,
TlGroup *parent,
const char *nom )
{
	a3dChunk		chunk;
	Boolean			texVertsDefined = FALSE;
	TlSurface 		*next_surf;
	a3dTexInfo 		tex_info;
	
	// start with planar mapping
	tex_info.type = 0;
	
	// create a TlModel
	TlModel *mdl = new TlModel( nom );
	TlSurface *surf = new TlSurface();
	
	PRINT_ERROR( (mdl==NULL), "Out of memory" );
	PRINT_ERROR( (surf==NULL), "Out of memory" );
	
	mdl->SetSurface( surf );
		
	do
	{
		// Get the next chunk from the stream
		ScanChunk( &chunk );
		// Proceed if we're still within the bounds of 
		// the trimesh chunk
		if ( chunk.offset.end <= headmaster->offset.end )
		{
			switch( chunk.header.ID )
			{
			case chunkid_VertexList:
				ParseVertexList( &chunk, surf );
				break;
			
			case chunkid_FacetList:
				ParseFacetList( &chunk, surf );
				break;
				
			case chunkid_4by3Matrix:
				ParseMeshMatrix( &mdl->transf );
				break;
			
			case chunkid_TexVerts:	// TlTexture vertices
				texVertsDefined = TRUE;
				ParseTextureVertices( &chunk, surf );
				break;
					
			case chunkid_MeshTextureInfo:	// Mesh texture info
				ParseMeshTextureInfo( &chunk, &tex_info );
				break;
			 	
			default:
				break;
			}
		}
		// Discard any unprocessed portion of the current chunk
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
	
	// slap the transform to the vertices
	TransfModelVertices( mdl );
	
	// Number of facets will be zero if no facets exist in default
	// (for the first surface "surf" ). So remove this surface from
	// TlModel data and attach the next surface. This avoids surfaces
	// with null facet list
	if ( surf->HasNullData() == TRUE )
	{
		next_surf = surf->GetNextSurface();
		delete surf;
		mdl->SetSurface( next_surf );
	}

	// Stitch the textures
	if( writeTextureStitiching == TRUE )
	{
		surf = mdl->GetSurface();
		if( surf != NULL ) StitchTextureSeams( surf,&tex_info ); 
	}
	
	// Til now the individual surfaces point to the same vertex list
	// but after this the surfaces without texturing may contain their
	// own local vetex list without texture coordinates
	// Set the tiling mode looking at the texture coordinates
	surf = mdl->GetSurface();
	while ( surf )
	{
		next_surf = surf->GetNextSurface();
		DeleteTextureVertices( surf );
		SetTlSurfaceWrapMode( surf );		
		surf = next_surf;
	}
	
	parent->AddChild( ( TlCharacter * )mdl );	
}

/*
** A3dsfile::ParseVertexList()
** Scans the contents of a vertex list
*/
void
A3dsfile::ParseVertexList(
a3dChunk *chunk,
TlSurface *surf )
{
	Int32 bytesRead;
	Int16 nThings = ScanInt16();
	Int32 bytesToRead = (Int32)nThings * sizeof( Point3DS );
	MemPtr		ap( (unsigned int)bytesToRead );
	
	// Allocate space for the vertex info and acquire it
	Point3DS *a3dvs = ( Point3DS *) ap.mPtr;
	
	bytesRead = ReadData( a3dvs, bytesToRead );
	VData1 *vd = ( VData1 *) new char[ sizeof(VData1)* nThings ];
	PRINT_ERROR( (vd==NULL), "Out of memory" );

	(void)&chunk;
	
	for ( int i = 0; i < nThings; i++ )
	{
		SwapInt32( (Int32 *)&a3dvs[ i ].x );
		SwapInt32( (Int32 *)&a3dvs[ i ].y );
		SwapInt32( (Int32 *)&a3dvs[ i ].z );
		vd[ i ].indx = i;
		vd[ i ].pnt.x = a3dvs[ i ].x;
		vd[ i ].pnt.y = a3dvs[ i ].y;
		vd[ i ].pnt.z = a3dvs[ i ].z;	

		//		fprintf(stderr,"PV%d:%g %g %g \n", i, vd[i].pnt.x, 
		//	vd[i].pnt.y, vd[i].pnt.z);
	}
	
	// Create a vertex template and stuff it into TlSurface
	TlVtxList *vlist = new TlVtxList( TlVtxList::LOCATIONS, 
	                              (Int32)nThings, 
	                              (double *)vd );
	PRINT_ERROR( (vlist==NULL), "Out of memory" );
	
	TlFacetList *flist = new TlFacetList( vlist );
	PRINT_ERROR( (flist==NULL), "Out of memory" );
	
	delete vlist;
	surf->SetFacetList( flist );
}

/*
** PrintKFObject()
** Print Keyframe Data
*/

ostream& 
A3dsfile::PrintKFObjects( ostream& os )
{
	KFObject *obj = kfObjs;
	KFObject *pobj;
	int i, j, cframe;
	Boolean hasData, hasObj;
	const char *objName;
	Point3D *po, *pi;
	Point3D ppi;
	double *rot;

	if ( kfObjs == NULL ) return os;
	
	for ( cframe = 0; cframe <= maxFrames; cframe++ )
	{
		obj = kfObjs;
		hasObj = FALSE;
		while ( obj )
		{
 			// Print KFObject data
 			hasData = FALSE;
			pi = &obj->pivot;
			
			// Get it's parent KFObject and it's pivot point
			pobj = GetKFObject( obj->parentID );
			if ( pobj == NULL ) {
				ppi.x = 0.0; ppi.y = 0.0; ppi.z = 0.0;
			} else ppi = pobj->pivot; 				
			
			if ( obj->grp != NULL ) objName = obj->grp->GetName();
			else  objName = obj->meshObject;

			for ( i=0; i < obj->pt->nKeys; i++ ) 
			{	
				po = &obj->pt->pnts[i].pnt;
				if ( obj->pt->pnts[i].frame == cframe )
				{
					hasData = TRUE; hasObj = TRUE;
					break;	
				}
			}
			TlTransform trInit;
			Vector3D raxis; 
			double ang;
			
			for ( i=0; i <  obj->rt->nKeys; i++ )
			{
				rot = obj->rt->rots[i].rot;
				ang = rot[0];
				raxis.x = rot[1];
				raxis.y = rot[2];
				raxis.z = rot[3];
				// compound the current rotation with previous keys
				// because the rotations are ralative to the previous key
				trInit = trInit * TlTransform( raxis, -ang );
				if ( obj->rt->rots[i].frame == cframe )
				{
					hasData = TRUE; hasObj = TRUE; 	
					break;
				}
			}	
			if 	( hasData == TRUE )
			{	
				// Position
				os << "p" << '\t' << objName << '\t' 
				   <<po->x <<' '
				   <<po->y <<' '
				   <<po->z <<' ';
				// Pivot
				os <<pi->x <<' '
				   <<pi->y <<' '
				   <<pi->z <<' ' << '\t';
				// Parent's Pivot
				os <<ppi.x <<' '
				   <<ppi.y <<' '
				   <<ppi.z <<' ' << '\t';
				// Rotation  
				for( i=0; i < 3; i++ )
					for( j=0; j < 3; j++ )
							os << trInit.GetVal(i,j) <<' ';
						os << '\n';
			}
			// Get next object in the list
			obj = obj->next;
		}
		if ( hasObj == TRUE ) {
			os << "; End of frame " << cframe << '\n';
			os << 'd' << '\n';
		}
	}
	return os; 
}

/*
** Print all the material and texture defines up-front
** and then refer to them in the surfaces
*/
ostream&
A3dsfile::PrintDefineAttributes( ostream& os, char *fileName, Boolean newFile,
                                char *scrFile, int scrType)
{
    TlMaterial *mat = materials;
    TlTexture *tex = textures;
    ostream *mat_file = NULL;
    ostream *tex_file = NULL;
    ostream *scr_file = NULL;
    int indx;
    char buf[120];
	const char *tmp;
	char temp[120], fl_name[120];

	/* if needed write textures and materials in to separate files */
	if( newFile == TRUE )
	{
		tmp = ChopStringLast( fileName, '.' );
		sprintf( temp, "%s", tmp );
		if( mat )
		{
			sprintf( fl_name, "%s.mat", temp );
			mat_file = new ofstream( fl_name );
			
			if( mat_file == NULL )
			{
				fprintf( stderr,"ERROR: Could not open file %s\n", fl_name );
				exit( 0 );
			}
	        sprintf( buf, "SDFVersion 0.1\n" );
	        WRITE_SDF( *mat_file, buf );
	        sprintf( buf, "include \"%s\"\n", fl_name );
	        WRITE_SDF( os, buf );
		}
		if( ( writeTexRefs == TRUE) && tex )
		{
			sprintf( fl_name, "%s.tex", temp );
			tex_file = new ofstream( fl_name );
			if( tex_file == NULL )
			{
				fprintf( stderr,"ERROR: Could not open file %s\n", fl_name );
				exit( 0 );
			}
	        sprintf( buf, "SDFVersion 0.1\n" );
	        WRITE_SDF( *tex_file, buf );
	        sprintf( buf, "include \"%s\"\n", fl_name );
	        WRITE_SDF( os, buf );
		}
	} else {
		mat_file = tex_file = &os;
	}

	/* write the texture conversion script file */
	if( scrType != 0 )
	{
		if ( scrType == 1 )
			scr_file = new ofstream( scrFile , ios::out );
		else scr_file = new ofstream( scrFile , ios::out | ios::app );
	}
	
    if ( mat )
    {
        sprintf( buf, "Define MatArray \"%s_materials\" {", GetFileName3DS() );
        BEGIN_SDF( *mat_file, buf );

        indx = 0;
        while ( mat )
        {
            if ( mat && ( mat->HasNullData() == FALSE ) )
            {
                mat->WriteSDF1( *mat_file );
                mat->SetRefID( -1 );   // clear and
                mat->SetRefID( indx ); // set the index
                indx++;
            }
            mat = mat->GetNextMaterial();
        }
        END_SDF( *mat_file, "} " );
    }

    if ( ( writeTexRefs == TRUE) && tex )
    {
        sprintf( buf, "Define TexArray \"%s_textures\" {", GetFileName3DS() );
        BEGIN_SDF( *tex_file, buf );

        indx = 0;
        while ( tex )
        {
            if ( tex && ( tex->HasNullData() == FALSE ) )
            {
                tex->WriteSDF1( *tex_file );
                tex->SetRefID( -1 );    // clear and
                tex->SetRefID( indx );  // set the index
                indx++;

				if( ( scrType != 0 ) && ( scr_file != NULL )  )
				{               
					// Write texture object
					strcpy( temp, tex->GetFileName() );
					sprintf( buf, " process_texture \"%s\" \"%s\"", 
						tex->GetOrigFileName(), tex->GetFileName() );
					WRITE_SDF( *scr_file, buf );
				}
            }
            tex = tex->GetNextTexture();
        }
        END_SDF( *tex_file, "} " );
    }
    
    if( newFile == TRUE )
	{
		if( mat_file ) delete mat_file;
		if( tex_file ) delete tex_file;
	}

	if( ( scrType != 0 ) && ( scr_file != NULL ) ) delete scr_file;

    return os;
}

/*
** Multiply two quaternions together
*/
static void
Quat_Mul(
    double *qL,
    double *qR,
    double *qq
    )
{
    qq[3] = qL[3]*qR[3] - qL[0]*qR[0] - qL[1]*qR[1] - qL[2]*qR[2];
    qq[0] = qL[3]*qR[0] + qL[0]*qR[3] + qL[1]*qR[2] - qL[2]*qR[1];
    qq[1] = qL[3]*qR[1] + qL[1]*qR[3] + qL[2]*qR[0] - qL[0]*qR[2];
    qq[2] = qL[3]*qR[2] + qL[2]*qR[3] + qL[0]*qR[1] - qL[1]*qR[0];
}

/*
** SDF_WriteKfData()
** Print Keyframe Data needed for SDF and Framework
*/


#define PI 3.14159265359
ostream& 
A3dsfile::SDF_WriteKfData( ostream& os )
{
	char buf[100], tbuf[100];
	KFObject *obj = kfObjs;
	KFObject *pobj;
	int i, j;
	int wholeDegrees, extraSpins;
	Boolean hasData;
	const char *objName;
	Point3D *pi;
	Point3D ppi;
	KFPosTrack *pt;
	KFRotTrack *rt;
	KFSclTrack *st;
	double duration;
	
	if ( kfObjs == NULL ) return os;

	WRITE_SDF( os, "SDFVersion 1.0\n" );

	// Define the KeyFrame object
	BEGIN_SDF( os, "define class KF_Object from Engine {" );
	WRITE_SDF( os, "character	Target" );
	WRITE_SDF( os, "point		ObjPivot" );
	WRITE_SDF( os, "point		PrntPivot\n" );

	WRITE_SDF( os, "floatarray	PosFrames" );
	WRITE_SDF( os, "floatarray	PosData" );
	WRITE_SDF( os, "floatarray	PosSplData\n" );

	WRITE_SDF( os, "floatarray	RotFrames" );
	WRITE_SDF( os, "floatarray	RotData" );
	WRITE_SDF( os, "floatarray	RotSplData\n" );

	WRITE_SDF( os, "floatarray	SclFrames" );
	WRITE_SDF( os, "floatarray	SclData" );
	WRITE_SDF( os, "floatarray	SclSplData" );
	WRITE_SDF( os, "pad 3" );
	END_SDF( os, "}\n" );

	WRITE_SDF( os, "Define array kfobjarray of KF_Object" );

	sprintf( buf, "Define kfobjarray \"%s_kfengines\" {", GetFileName3DS() );
	BEGIN_SDF( os, buf );
	while ( obj )
	{
		duration = 0.0;
		
		BEGIN_SDF( os, "KF_Object {" );
		
		// print object keyframe data only if the target object exist
		// this avoids writing rogue objects like - _$$$DUMMY
		if ( obj->grp != NULL ) objName = obj->grp->GetName();
		else  objName = obj->meshObject;
		
		sprintf( buf, "Target Use \"%s\"", objName );
		WRITE_SDF( os, buf );

#if 0
		fprintf( stderr, "********* *bj = %s\n", objName );
#endif
		
		pi = &obj->pivot;
		
		// Get it's parent KFObject and it's pivot point
		pobj = GetKFObject( obj->parentID );
		if ( pobj == NULL ) {
			ppi.x = 0.0; ppi.y = 0.0; ppi.z = 0.0;
		} else ppi = pobj->pivot; 				

		// objects pivot
		sprintf( buf, "ObjPivot { %g %g %g }", pi->x, pi->y, pi->z );
		WRITE_SDF( os, buf );
		// parents pivot
		sprintf( buf, "PrntPivot { %g %g %g }", ppi.x, ppi.y, ppi.z ); 
		WRITE_SDF( os, buf );

		// POSITION keys
		if ( obj->pt ) {
			pt = obj->pt;
			
			if ( duration < pt->pnts[pt->nKeys-1].frame ) 
				duration = pt->pnts[pt->nKeys-1].frame;
				
			// write time keys
			BEGIN_SDF( os, "PosFrames { " );
			for( i = 0; i < pt->nKeys; )
			{
				buf[ 0 ] = '\0';
				j = i;
				for( ; ( i < pt->nKeys ) && ( ( i - j ) < 5 ); i++ )
				{
					sprintf( tbuf, " %g", (float)pt->pnts[i].frame );	 
					strcat( buf, tbuf );
				}
				strcat( buf, "" );
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );
			
			// write position keys
			BEGIN_SDF( os, "PosData { " );
			for( i = 0; i < pt->nKeys; i++ )
			{
				sprintf( buf, "%g %g %g ",  pt->pnts[i].pnt.x,
				                            pt->pnts[i].pnt.y,
				                            pt->pnts[i].pnt.z );	 
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );
			
			// write spline data
			hasData = FALSE;
			for( i = 0; ( ( i < pt->nKeys ) && ( !hasData ) ); i++ )
				if ( pt->spl[i].mUseFlag ) hasData = TRUE;
			if ( hasData )
			{
				BEGIN_SDF( os, "PosSplData { " );
				for( i = 0; i < pt->nKeys; i++ )
				{
					sprintf( buf, "%g %g %g %g %g", pt->spl[i].mTension,
					                            pt->spl[i].mBias,
					                            pt->spl[i].mContinuity,
					                            pt->spl[i].mEaseIn,
					                            pt->spl[i].mEaseOut );	 
					WRITE_SDF( os, buf );
				}
				END_SDF( os, "}" );
			}			
		}
		
		// ROTATION keys
		if ( obj->rt ) {
			rt = obj->rt;

			if ( duration < rt->rots[rt->nKeys-1].frame ) 
				duration = rt->rots[rt->nKeys-1].frame;
		
			// write time keys
			BEGIN_SDF( os, "RotFrames { " );
			for( i = 0; i < rt->nKeys; )
			{
				buf[ 0 ] = '\0';
				j = i;
				for( ; ( i < rt->nKeys ) && ( ( i - j ) < 5 ); i++ )
				{
					sprintf( tbuf, " %g", (float)rt->rots[i].frame );	 
					strcat( buf, tbuf );
				}
				strcat( buf, "" );
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );

			// write rotation keys
			TlTransform trInit;
			Vector3D raxis; 
			double ang, cang = 0.0;
			double cquat[4], tquat[4], fquat[4];
			double len;

			/* initialize to a unit quaternion */
			cquat[0] = 0.0; cquat[1] = 0.0; cquat[2] = 0.0; cquat[3] = 1.0;
			BEGIN_SDF( os, "RotData { " );
			for( i = 0; i < rt->nKeys; i++ )
			{
				/*
				** convert the incremental rotation at each to quaternion and
				** then concatenate the these quaternions to get cumulative
				** rotation til that specific key.
				** by using quaternions you will automatically get minimum angular
				** displacement between adjacent keys
				*/
			        fquat[0] = rt->rots[i].rot[1];
			        
				fquat[0] = rt->rots[i].rot[1] * sin( (double)((double)(-rt->rots[i].rot[0])/(double)2.0) );
				fquat[1] = rt->rots[i].rot[2] * sin( -rt->rots[i].rot[0]/2.0 );
				fquat[2] = rt->rots[i].rot[3] * sin( -rt->rots[i].rot[0]/2.0 );

#if 1
				wholeDegrees = (int)(fabs(PI*rt->rots[i].rot[0]));
				extraSpins = wholeDegrees/360;
#endif
				fquat[3] = cos( -rt->rots[i].rot[0]/2.0 );
				/* Do the concatenation to get cumulative rotation */
				Quat_Mul( fquat, cquat, tquat );
				cquat[0] = tquat[0];
				cquat[1] = tquat[1];
				cquat[2] = tquat[2];
				cquat[3] = tquat[3];
				/* normalize the axis */
				len = sqrt( cquat[0]*cquat[0] +
				            cquat[1]*cquat[1] +
				            cquat[2]*cquat[2] );
				if ( !AEQUAL( len, 0.0 ) )
				{
				  ang = 2.0 * acos( cquat[3] );
#if 1
				  ang += (double)extraSpins*PI;
#endif
				  raxis.x = (cquat[0] / len);
				  raxis.y = (cquat[1] / len);
				  raxis.z = (cquat[2] / len);
				} else {
					raxis.x = 1.0;
					raxis.y = 0.0;
					raxis.z = 0.0;
					ang = 0.0;
				}
				
				sprintf( buf, "%g %g %g %g", raxis.x, raxis.y, raxis.z, ang );				
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );
			
			// write spline data
			hasData = FALSE;
			for( i = 0; ( ( i < rt->nKeys ) && ( !hasData ) ); i++ )
				if ( rt->spl[i].mUseFlag ) hasData = TRUE;
			if ( hasData )
			{
				BEGIN_SDF( os, "RotSplData { " );
				for( i = 0; i < rt->nKeys; i++ )
				{
					sprintf( buf, "%g %g %g %g %g", rt->spl[i].mTension,
					                            rt->spl[i].mBias,
					                            rt->spl[i].mContinuity,
					                            rt->spl[i].mEaseIn,
					                            rt->spl[i].mEaseOut );	 
					WRITE_SDF( os, buf );
				}
				END_SDF( os, "}" );
			}			
		}

		// SCALE keys
		if ( obj->st ) {
			st = obj->st;

			if ( duration < st->scls[st->nKeys-1].frame ) 
				duration = st->scls[st->nKeys-1].frame;
			
			// write time keys
			BEGIN_SDF( os, "SclFrames { " );
			for( i = 0; i < st->nKeys; )
			{
				buf[ 0 ] = '\0';
				j = i;
				for( ; ( i < st->nKeys ) && ( ( i - j ) < 5 ); i++ )
				{
					sprintf( tbuf, " %g", (float)st->scls[i].frame );	 
					strcat( buf, tbuf );
				}
				strcat( buf, "" );
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );

			// write scale keys
			BEGIN_SDF( os, "SclData { " );
			for( i = 0; i < st->nKeys; i++ )
			{
				sprintf( buf, "%g %g %g ", st->scls[i].pnt.x,
				                            st->scls[i].pnt.y,
				                            st->scls[i].pnt.z );	 
				WRITE_SDF( os, buf );
			}
			END_SDF( os, "}" );
			
			// write spline data
			hasData = FALSE;
			for( i = 0; ( ( i < st->nKeys ) && ( !hasData ) ); i++ )
				if ( st->spl[i].mUseFlag ) hasData = TRUE;
			if ( hasData )
			{
				BEGIN_SDF( os, "SclSplData { " );
				for( i = 0; i < st->nKeys; i++ )
				{
					sprintf( buf, "%g %g %g %g %g", st->spl[i].mTension,
					                            st->spl[i].mBias,
					                            st->spl[i].mContinuity,
					                            st->spl[i].mEaseIn,
					                            st->spl[i].mEaseOut );	 
					WRITE_SDF( os, buf );
				}
				END_SDF( os, "}" );
			}			
		}
		
		// write animation mode and 
		sprintf( buf, "Control cycle" );
		WRITE_SDF( os, buf );
		
		/* assuming the frame rate is 30 fps */
	        sprintf( buf, "Duration %g", duration /30.0 );
		WRITE_SDF( os, buf );

		END_SDF( os, "}" );
		
		obj = obj->next;
	}
	END_SDF( os, "}" );		
	return os; 
}

/*
** A3dsfile::ParseKFData()
** Import Keyframe Data
*/

void		
A3dsfile::ParseKFData(
a3dChunk *headmaster )
{
	a3dChunk		chunk;
	
	maxFrames = 0;
	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )	
		// Proceed if we're still within the bounds of the model chunk
		{
			switch( chunk.header.ID )
			{
			case chunkid_KFHeader:
				ScanInt16();  // skip
				ScanString(); // skip
				maxFrames = ScanInt32();
				break;
			case chunkid_KFObject:
				ParseKFObject( &chunk );
				break;
								
			default:
				break;
			}
		}
		
		// Discard any unprocessed portion of the current chunk
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
}

/*
** A3dsfile::ParseKFObject()
** Parse keyframe object 
*/

void		
A3dsfile::ParseKFObject(
a3dChunk *headmaster )
{
	a3dChunk chunk;
	const char *nom;
	
	// Allocate memory for KeyFrame Object & initialize
	KFObject *obj 		= new KFObject;
	obj->nodeID 		= -1;
	obj->parentID		= -1;
	obj->meshObject[0] 	= '\0';
	obj->instName		= NULL;
	obj->mt				= NULL;
	obj->pt				= NULL;
	obj->rt				= NULL;
	obj->st				= NULL;
	obj->grp			= NULL;
	obj->next			= NULL;
	obj->pivot.x = 0.0; obj->pivot.y = 0.0; obj->pivot.z = 0.0;

	PRINT_ERROR( (obj==NULL), "Out of memory" );

	do
	{
		ScanChunk( &chunk );
		
		if ( chunk.offset.end <= headmaster->offset.end )	
		{
			switch( chunk.header.ID )
			{
			case chunkid_KFNodeID:
				obj->nodeID = ScanInt16();
				break;
								
			case chunkid_KFNodeHdr:
				ParseKFNodeHdr( &chunk, obj );
				break;
				
			case chunkid_KFInstName:
				nom = ScanString();
				obj->instName = new char[ strlen( nom ) + 1 ];
				PRINT_ERROR( (obj->instName==NULL), "Out of memory" );
				strcpy( obj->instName, nom );
				break;
			
			case chunkid_KFObjPivot:
				ScanPoint( &obj->pivot );
				break;

			case chunkid_KFObjBbox:
				break;
					
			case chunkid_KFPosTrack:
				ParseKFPosTrack( &chunk, &obj->pt );
				break;
				
			case chunkid_KFRotTrack:
				ParseKFRotTrack( &chunk, &obj->rt );
				break;
				
			case chunkid_KFSclTrack:
				ParseKFSclTrack( &chunk, &obj->st );
				break;
																
			case chunkid_KFMorphTrack:
				ParseKFMorphData( &chunk, &obj->mt );
				break;
												
			default:
				break;
			}
		}
		
		// Discard any unprocessed portion of the current chunk
		SkipChunk( &chunk );
	}
	while ( chunk.offset.end < headmaster->offset.end );
	
#if 0
	fprintf( stderr, "Object ID = %d, Parent ID = %d\tObject Name = %s\n",
			obj->nodeID, obj->parentID, obj->meshObject );
#endif

	// Add the object to the linked list
	AddKFObject( obj );
}

/*
** A3dsfile::ParseKFNodeHdr()
** Parse Keyframe node header info
*/

void		
A3dsfile::ParseKFNodeHdr(
a3dChunk *headmaster,
KFObject *obj )
{
	Int16			data[3];
	
	(void)&headmaster;

	// Object name
	strcpy( obj->meshObject, ScanString() );
	// Flags
	data[ 0 ] = ScanInt16();
	data[ 1 ] = ScanInt16();
	// Parent indx
	obj->parentID = ScanInt16();
}

/*
** A3dsfile::ParseKFMorphData()
** Parse Keyframe morhing data
*/

void		
A3dsfile::ParseKFMorphData(
a3dChunk *headmaster,
KFMorphTrack **mt )
{
	Int16			flags;
	Int32			dummy[2], numKeys;
	int				i;
	
	(void)&headmaster;

	// read track header
	flags = ScanInt16();
	ReadData( dummy, 2*sizeof( Int32 ) );
	numKeys = ScanInt32();

	// Allocate the tracks
	*mt = new KFMorphTrack;
	PRINT_ERROR( ((*mt)==NULL), "Out of memory" );
	(*mt)->nKeys = numKeys;
	(*mt)->morphObjs = new KFMorphKey[ numKeys ];
	PRINT_ERROR( ((*mt)->morphObjs==NULL), "Out of memory" );
		
	// read key header and data
	for ( i = 0; i < numKeys; i++ ) {
		(*mt)->morphObjs[ i ].frame = ScanInt32();
		flags = ScanInt16();
		if ( flags & 0x01 ) ReadData( dummy, 4 );
		if ( flags & 0x02 ) ReadData( dummy, 4 );
		if ( flags & 0x04 ) ReadData( dummy, 4 );
		if ( flags & 0x08 ) ReadData( dummy, 4 );
		if ( flags & 0x10 ) ReadData( dummy, 4 ); 
		
		strcpy( (*mt)->morphObjs[ i ].obj, ScanString() );
	}	 
}

/*
** A3dsfile::ParseKFPosTrack()
** Parse Keyframe position data
*/

void		
A3dsfile::ParseKFPosTrack(
a3dChunk *headmaster,
KFPosTrack **pt )
{
	Int16			flags;
	Int32			dummy[2], numKeys;
	Point3D			pnt;
	int				i;
	
	(void)&headmaster;

	// read track header
	flags = ScanInt16();
	ReadData( dummy, 2*sizeof( Int32 ) );
	numKeys = ScanInt32();

	// Allocate the tracks
	*pt = new KFPosTrack;
	PRINT_ERROR( ((*pt)==NULL), "Out of memory" );
	(*pt)->nKeys = numKeys;
	(*pt)->pnts = new KFPosKey[ numKeys ];
	PRINT_ERROR( ((*pt)->pnts==NULL), "Out of memory" );
	(*pt)->spl = new SplineProps[ numKeys ];
	PRINT_ERROR( ((*pt)->spl==NULL), "Out of memory" );
		
	// read key header and data
	for ( i = 0; i < numKeys; i++ ) {
		(*pt)->pnts[ i ].frame = ScanInt32();
		
		ScanSplineKey( &(*pt)->spl[i] ); 
		
		ScanPoint( &pnt );
		(*pt)->pnts[ i ].pnt = pnt;
	}	
}

/*
** A3dsfile::ParseKFRotTrack()
** Parse Keyframe position data
*/

void		
A3dsfile::ParseKFRotTrack(
a3dChunk *headmaster,
KFRotTrack **rt )
{
	Int16			flags;
	Int32			dummy[2], numKeys;
	float 			rot[4];
	int				i, j;
	
	(void)&headmaster;

	// read track header
	flags = ScanInt16();
	ReadData( dummy, 2*sizeof( Int32 ) );
	numKeys = ScanInt32();

	// Allocate the tracks
	*rt = new KFRotTrack;
	PRINT_ERROR( ((*rt)==NULL), "Out of memory" );
	(*rt)->nKeys = numKeys;
	(*rt)->rots = new KFRotKey[ numKeys ];
	PRINT_ERROR( ((*rt)->rots==NULL), "Out of memory" );
	(*rt)->spl = new SplineProps[ numKeys ];
	PRINT_ERROR( ((*rt)->spl==NULL), "Out of memory" );
		
	// read key header and data
	for ( i = 0; i < numKeys; i++ ) {
		(*rt)->rots[ i ].frame = ScanInt32();
		
		ScanSplineKey( &(*rt)->spl[i] ); 
				
		ReadData( rot, 4 * sizeof( Int32 ) );
		for ( j = 0; j < 4; j++ ) 
		{
			SwapInt32( (Int32 *)&rot[j] );
			(*rt)->rots[ i ].rot[ j ] = rot[ j ];
		}
	}	
}

/*
** A3dsfile::ParseKFSclTrack()
** Parse Keyframe position data
*/

void		
A3dsfile::ParseKFSclTrack(
a3dChunk *headmaster,
KFSclTrack **st )
{
	Int16			flags;
	Int32			dummy[2], numKeys;
	Point3D			pnt;
	int 			i;
	
	(void)&headmaster;

	// read track header
	flags = ScanInt16();
	ReadData( dummy, 2*sizeof( Int32 ) );
	numKeys = ScanInt32();
	
	// Allocate the tracks
	*st = new KFSclTrack;
	PRINT_ERROR( ((*st)==NULL), "Out of memory" );
	(*st)->nKeys = numKeys;
	(*st)->scls = new KFPosKey[ numKeys ];
	PRINT_ERROR( ((*st)->scls==NULL), "Out of memory" );
	(*st)->spl = new SplineProps[ numKeys ];
	PRINT_ERROR( ((*st)->spl==NULL), "Out of memory" );
	
	// read key header and data
	for ( i = 0; i < numKeys; i++ ) {
		(*st)->scls[ i ].frame = ScanInt32();
		
		ScanSplineKey( &(*st)->spl[i] ); 
		
		ScanPoint( &pnt );
		(*st)->scls[ i ].pnt = pnt;
	}
}

/*
** A3dsfile::BuildKFObjHierarchy()
** Build KeyFrame object hierarchy and attach it to "parent".
** Parent group contains all characters from mesh section
*/

void			
A3dsfile::BuildKFObjHierarchy( TlGroup *parent )
{
	KFObject *obj = kfObjs;
	TlCharacter *instChar, *origChar;
	KFObject *pobj;
	char buf[ 80 ];
	int i;
	Point3D ppi;
	// keep track of all the refered mesh objects
	Int32 ntotal = 0;
	Int32 ncur = 0;
	TlCharacter **refered;
	static Int32 dummy_obj_count = 0;
	
	if ( kfObjs == NULL ) return;
	
	// calculate the number of objects
	while ( obj )
	{
		ntotal++;
		obj = obj->next;
	}
	obj = kfObjs;
	
	refered = new TlCharacter*[ ntotal ]; 
	PRINT_ERROR( (refered==NULL), "Out of memory" );

	// Create "group" character corresponding to each KFObject
	while ( obj )
	{		
		if ( strlen( obj->meshObject ) != 0 ) 
		{
			origChar = parent->LookUp( GetModelName( obj->meshObject ) );
			if ( origChar != NULL ) {
			
				refered[ ncur++ ] = origChar;
				
				// create instance of the original character
				instChar = origChar->Instance();
				
				// create a group and add this character instance
				// in some cases the instance name is not-unique ( same )
				// so generate a unique name even if the instance name exists
				/* if ( obj->instName != NULL ) sprintf( buf, "%s", obj->instName );
				else */
				sprintf( buf, "Grp_%s", instChar->GetName() );
				obj->grp = new TlGroup( buf );
				PRINT_ERROR( (obj->grp==NULL), "Out of memory" );
				obj->grp->AddChild( instChar );
				
			/* you can have rogue models with names like - _$$$DUMMY so ignore them */
			} else {
				/* cout << "Could not find TlCharacter " << obj->meshObject << endl; */
				
			    /* create an empty group for this dummy object */
			    sprintf( buf, "Grp_Dummy_%d", dummy_obj_count++ );
			    obj->grp = new TlGroup( buf );
				PRINT_ERROR( (obj->grp==NULL), "Out of memory" );
			}
		}		
		// Get Next KFObject		
		obj = obj->next;
	}

	// Remove all refered mesh objects from parent group
	for ( i = 0; i < ncur; i++ )
	{
		origChar = parent->RemoveChild( refered[ i ] );
		if ( origChar ) delete origChar;
	}
	
	// Create the hierarchy from the KFObject list
	obj = kfObjs;
	while ( obj )
	{
		// get parent KFObject and connect children appropriately
		pobj = GetKFObject( obj->parentID );
		if ( obj->grp != NULL ) 
		{
			if ( ( pobj != NULL ) && ( pobj->grp != NULL ) ) {
				ppi = pobj->pivot;
				pobj->grp->AddChild( obj->grp );
			// if parent not found add it to the root group
			} else {
				ppi.x = 0.0; ppi.y = 0.0; ppi.z = 0.0; 
				parent->AddChild( (TlCharacter *)obj->grp ); 
			}
			
			// Calculate the transformation of the current child group
			TlTransform tr( obj->pt->pnts[0].pnt, obj->pivot, ppi,
				              obj->st->scls[0].pnt, obj->rt->rots[0].rot );
			obj->grp->transf = tr;
			// obj->grp->transf.WriteSDF( cout );
		}	
		
		// Get Next KFObject
		obj = obj->next;	
	}
	delete[] refered;
}

/*
** Calculate the surface wrap mode based on the texture coordinates
*/
void 
A3dsfile::SetTlSurfaceWrapMode( TlSurface *surf )
{
	int i;
	Point2D *uvp;
	TlVtxList *vlist = surf->GetFacetList()->GetVtxList();
	int vsize = (int)vlist->GetVertexSize();
	int vcount = (int)vlist->GetVtxCount();	
	double *vd = vlist->GetVtxData();
	TlTexture *tex = surf->GetTexture();
	Point2D min, max;

	if ( !( vlist->GetVertexFormat() & TlVtxList::TEXTURES ) )
		return;
	
	uvp = (Point2D *) vlist->GetTexCoord( vd );
	vd += vsize;
	min.x = uvp->x; min.y = uvp->y;
	max.x = uvp->x; max.y = uvp->y;
	
	// update the texture coordinates to new sub-texture region
	for ( i = 1; i < vcount; i++, vd += vsize )	
	{
		uvp = (Point2D *) vlist->GetTexCoord( vd );
		
		if (min.x > uvp->x ) min.x = uvp->x;	
		if (min.y > uvp->y ) min.y = uvp->y;
		if (max.x < uvp->x ) max.x = uvp->x;	
		if (max.y < uvp->y ) max.y = uvp->y;
	}
	
	if ( tex ) 
	{
		// if the texture mode is "tiled" and UVs are within 0 to 1 then make it "clamp"
		if ( ( !tex->GetxWrapMode() ) &&  ( (min.x+EPSILON) >= 0.0 ) && ( (max.x-EPSILON) <= 1.0 ) )
			tex->SetxWrapMode( TRUE );
			
		if ( ( !tex->GetyWrapMode() ) &&  ( (min.y+EPSILON) >= 0.0 ) && ( (max.y-EPSILON) <= 1.0 ) )
			tex->SetyWrapMode( TRUE );	

#if 0 
		printf( "%s.utf : minu=%f, maxu=%f; minv=%f, maxv=%f\n",
			tex->t->fileName, min.x, max.x,min.y, max.y  );		
#endif
	}
}

/*
** set prefix to generate unique model, texture and material names
*/
void			
A3dsfile::SetPrefix( 
	const char *pfx 
	)
{
	if ( prefix == NULL ) 
	{
		prefix = new char[ strlen( pfx ) + 1 ];
		PRINT_ERROR( (prefix==NULL), "Out of memory" );
		strcpy( prefix, pfx );
	} else if ( strlen( prefix ) < strlen( pfx ) ) {
		delete[] prefix;
		prefix = new char[ strlen( pfx ) + 1 ];
		PRINT_ERROR( (prefix==NULL), "Out of memory" );
		strcpy( prefix, pfx );
	} else strcpy( prefix, pfx );
}

/*
**	Set the flag to output/suppress texture refences into SDF file
*/
void
A3dsfile::SetTexRefOutput( 
	Boolean texRefs
	)
{
	writeTexRefs = texRefs;
}

/*
**	Set the flag to output/suppress light refences into SDF file
*/
void
A3dsfile::SetLightsOutput( 
	Boolean lgtRefs
	)
{
	writeLights = lgtRefs;
}

/*
**	Set the flag to output/suppress smooting groups into SDF file
*/
void
A3dsfile::SetSmootGroupsOutput( 
	Boolean sGroups
	)
{
	writeSmoothingGroups = sGroups;
}

/*
**	Enable/disable texture stitiching for cylindrical and spherical mapping
*/
void
A3dsfile::SetTexStitchOutput( 
	Boolean stitch
	)
{
	writeTextureStitiching = stitch;
}

