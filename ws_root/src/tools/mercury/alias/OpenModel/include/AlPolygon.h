/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions, statements and computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	protected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties or copied or duplicated, in whole or
//	in part, without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+
*/

/*
//
//	.NAME AlPolygon - Interface to Alias polyset polygons.
//
//	.SECTION Description
//		
//		AlPolygon is the interface to the polygon data of Alias'
//		polyset objects.  To create a polygon, first instantiate and create
//		an AlPolyset and then use the AlPolyset::newPolygon() method to
//		create a new polygon. It is not possible to create a polygon
//		without a polyset.
//	
//		A polygon is a collection of vertices from a particular polyset.
//
//		The only way to delete a polygon is with the AlPolyset::deletePolygon()
//		method.
//
//		Polygons are made of lists of vertices which you traverse using
//		an index through the vertex() method. You can also pack the
//		vertex information into an array using methods in this class.
//
*/

#ifndef _AlPolygon
#define _AlPolygon

#ifdef __cplusplus
#include <AlObject.h>
#include <AlIterator.h>
#include <AlTM.h>
//
// These are to be used to pass to the fliping and rotating methods.
#endif

const unsigned int kPOLY_FLIP_NONE      =0x00000000;
const unsigned int kPOLY_FLIP_HORIZ     =0x00000001;
const unsigned int kPOLY_FLIP_VERT      =0x00000002;
const unsigned int kPOLY_FLIP_BOTH      =0x00000003;

const unsigned int kPOLY_ROT_0          =0x00000000;
const unsigned int kPOLY_ROT_90         =0x00000001;
const unsigned int kPOLY_ROT_180        =0x00000002;
const unsigned int kPOLY_ROT_270        =0x00000003;

#ifdef __cplusplus
extern "C"
{
	struct Dag_node;
}

class PS_Polygon;

class AlPolygon : public AlObject {
	friend					class AlFriend;

public:

							AlPolygon();
	virtual 				~AlPolygon();
	virtual AlObject*		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlPolygon* 		asPolygonPtr();

	int						numberOfVertices() const;
	AlPolysetVertex*		vertex( int ) const;
	statusCode				vertexD( int, AlPolysetVertex& ) const;

	statusCode				addVertex( int );
	statusCode				removeVertex( int );
	statusCode				verticesWorldPosition( double[] )const;
	statusCode				verticesAffectedPosition( const AlTM&, double[] )const;
	statusCode				verticesUnaffectedPosition( double[] )const;

	statusCode				normal( double&, double&, double& ) const;
	statusCode				setNormal( double, double, double );
	statusCode				calcNormal();

	int						index();
	AlPolyset*				polyset() const;

	int						findVertexIndex( AlPolysetVertex* );
	int						vertexPolysetIndex( int vertexIndex );

    int						shaderIndex();
    statusCode				setShaderIndex( int );

	statusCode				setSt( int vertex, float, float );
	statusCode				st( int vertex, float &, float & );

	statusCode				normal( int vertex, float &, float &, float & );
	statusCode				setNormal( int vertex, float, float, float );

	boolean					queryPerPolyTextures() const;
	statusCode				setPerPolyTextures( boolean state );

	statusCode				queryPerPolyTextureOther1( unsigned short &index ) const;
	statusCode				setPerPolyTextureOther1( unsigned short index );

	statusCode				queryPerPolyTextureOther2( unsigned short &index ) const;
	statusCode				setPerPolyTextureOther2( unsigned short index );

	statusCode				queryPerPolyTextureIndex( unsigned short &index ) const;
	statusCode				setPerPolyTextureIndex( unsigned short index );

	statusCode				queryPerPolyTextureRotation( int &flag ) const;
	statusCode				setPerPolyTextureRotation( int flag );

	statusCode				queryPerPolyTextureFlip( int &flag ) const;
	statusCode				setPerPolyTextureFlip( int flag );

	statusCode				blindData( int, long&, const char *& );
	statusCode 				setBlindData( int, long, const char * );
	statusCode 				removeBlindData( int );

	statusCode 				applyIteratorToVertices( AlIterator*, int& ) const;

	statusCode              doUpdates( boolean newState = TRUE );

protected:

							AlPolygon( Dag_node* );
	statusCode 				create( AlPolyset*, int ); 
	statusCode				create( AlPolyset*, PS_Polygon* );

	Dag_node				*fParent;
private:
	boolean					updateOn;
	boolean					updateNeeded;
	void					updatePerform();

	static void				initMessages();
	static void				finiMessages();
	
	boolean					extractType( int&, void*&, void*& ) const;
};
#endif	/* __cplusplus */

#endif	/* _AlPolygon_h */
