#ifndef _AlPolyset
#define _AlPolyset

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
//
//	.NAME AlPolyset - Interface to Alias polyset geometry.
//
//	.SECTION Description
//		
//		AlPolyset is the interface to the geometric data of Alias'
//		polyset objects.  To create a polyset, first instantiate and
//		then create an AlPolyset. The AlPolysetNode for the polyset
//		is created automatically.
//	
//		For more information on how to create the polyset geometry, see the
//		description for the create() method.
//
//		A polyset is a collection of polygons which use a collection of
//		vertices.
//
//		There are two ways to delete an AlPolyset.
//		.br
//		If the AlPolyset::deleteObject() is called, the attached AlPolysetNode
//		is deleted.  If the AlPolysetNode::deleteObject() is called, the
//		attached AlPolyset is deleted.
//
//		All AlPolyset objects have render information attached to them.
//		The AlRenderInfo structure can be used to query the polyset's
//		render parameters.
//
//      In addition, polysets may contains more than one list of shaders.
//      These shaders are used to specify per-polygon shading.
//
//		Polysets are made of polygons and vertices each of which you
//		traverse as a list by using the polygon() and vertex()
//		methods. The AlPolygon and AlPolysetVertex methods allow to work
//		with the individual polygons and vertices. You can also pack the
//		polyset information into an array using methods in this class.
//

#include <AlIterator.h>
#include <AlObject.h>
#include <AlShader.h>
#include <AlRenderInfo.h>
#include <AlTM.h>

struct Dag_node;
struct PS_Polyset;

#define AL_MAX_SHADERS_PER_POLYSET 64


//
// These constants may be 'or'd together to specify what merging
// criterial are appropriate for the mergeVertices call.
//
const int AL_MERGE_RESPECT_ST       =0x00000001;
const int AL_MERGE_RESPECT_NORMAL   =0x00000002;
const int AL_MERGE_RESPECT_DISTANCE =0x00000004;
const int AL_MERGE_RESPECT_SHADER   =0x00000008;


class AlPolyset : public AlObject {
	friend class			AlFriend;
public:
							AlPolyset();
	virtual					~AlPolyset();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	statusCode 				create(); 

	virtual AlObjectType	type() const;
	virtual AlPolyset* 		asPolysetPtr();
	AlPolysetNode*			polysetNode() const;

	int						newVertex( double, double, double );
	int						newPolygon();

	statusCode				deleteVertex( int );
	statusCode				deletePolygon( int );
	statusCode				deletePolygons( int, const int[] );

	AlPolysetVertex*		vertex( int ) const;
	statusCode				vertexD( int, AlPolysetVertex& ) const;
	AlPolygon*				polygon( int ) const;
	statusCode				polygonD( int, AlPolygon& ) const;

	int						numberOfVertices() const;
	int						numberOfPolygons() const;

	statusCode				calcNormals( boolean );

	statusCode				verticesWorldPosition( double* ) const;
	statusCode				verticesAffectedPosition( AlTM&, double* ) const;
	statusCode				verticesUnaffectedPosition( double* ) const;
	statusCode				setVerticesUnaffectedPosition( double* );

	// Modify vertex sharing information
	//
	statusCode				mergeVertices( int flags,
										   double distanceTolerance,
										   double normalTolerance,
										   double textureTolerance );

	statusCode				splitVertices();

    int                     numberOfShaderLists() const;

    // For backwards compatibility, these first routines always deal with
    // the first shader list
	AlShader*				firstShader() const;
	AlShader*				nextShader( AlShader* ) const;
	statusCode				nextShaderD( AlShader *) const;
	statusCode				assignShader( AlShader* );
	statusCode				layerShader( AlShader* );

    // The following routines allow full access to potentially multiple
    // shader lists per polyset. The first parameter is the list number.
	AlShader*				firstShader( int ) const;
	AlShader*				nextShader( int, AlShader* ) const;
	statusCode				nextShaderD( int, AlShader *) const;
	statusCode				assignShader( int, AlShader* );

	statusCode				renderInfo( AlRenderInfo & ) const;
	statusCode				setRenderInfo( AlRenderInfo& ) const;

	boolean					isDisplayModeSet( AlDisplayModeType ) const;
	statusCode				setDisplayMode( AlDisplayModeType, boolean );

	boolean                 isFrozenNormals() const;
	statusCode              freezeNormals();
	statusCode              unFreezeNormals();

	statusCode				calcSTs();

	statusCode				blindData( int, long&, const char *& );
	statusCode 				setBlindData( int, long, const char * );
	statusCode          	removeBlindData( int );


	statusCode				applyIteratorToPolygons( AlIterator*, int& ) const;
	statusCode				applyIteratorToVertices( AlIterator*, int& ) const;

	statusCode              doUpdates( boolean newState = TRUE );

protected:

							AlPolyset( Dag_node* );
	// Pointer to its AlPolysetNode
	//
	Dag_node				*fParent;

private:
    boolean                 updateOn;
    boolean                 updateNeeded;
    void                    updatePerform();

	static void				initMessages();
	static void				finiMessages();

	virtual boolean			extractType( int&, void*&, void*& ) const;
};
#endif
