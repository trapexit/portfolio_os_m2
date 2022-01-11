#ifndef _AlPolysetVertex
#define _AlPolysetVertex

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
//	.NAME AlPolysetVertex - Interface to Alias polyset vertices.
//
//	.SECTION Description
//		
//		AlPolysetVertex is the interface to the vertex data of Alias'
//		polyset objects. An AlPolysetVertex can only be created through
//		AlPolyset::newVertex() method, and it can be deleted only through
//		the AlPolyset::deleteVertex() method.
// .br
//		An AlPolysetVertex defines a location in space for a corner of
//		an AlPolygon.
//

#include <AlObject.h>
#include <AlClusterable.h>
#include <AlAnimatable.h>
#include <AlSettable.h>
#include <AlPickable.h>
#include <AlTM.h>

struct Dag_node;

class PS_Vertex;

class AlPolysetVertex	: public AlObject
					 	, public AlClusterable
					 	, public AlAnimatable
						, public AlSettable
						, public AlPickable
{
	friend class				AlFriend;
public:

								AlPolysetVertex();
	virtual						~AlPolysetVertex();
	virtual AlObject*			copyWrapper() const;

	virtual AlAnimatable*		asAnimatablePtr();
	virtual AlSettable*			asSettablePtr();
	virtual AlClusterable*		asClusterablePtr();
	virtual AlPickable*			asPickablePtr();

	virtual AlObjectType		type() const;
	virtual AlPolysetVertex* 	asPolysetVertexPtr();

	statusCode					worldPosition( double&, double&, double& )const;
	statusCode					affectedPosition( AlTM&, double&, double&, double& )const;
	statusCode					unaffectedPosition( double&, double&, double& )const;
	statusCode					setUnaffectedPosition( double, double, double );

	statusCode					normal( double&, double&, double& ) const;
	statusCode					setNormal( double, double, double );
	statusCode					freezeNormalFlag( boolean& ) const;
	statusCode					setFreezeNormalFlag( boolean );

	statusCode					st( double&, double& ) const;
	statusCode					setSt( double, double );

	statusCode					color( double&, double&, double&, double& ) const;
	statusCode					setColor( double, double, double, double );

	int							index() const;
	AlPolyset*					polyset() const;

	statusCode					blindData( int, long&, const char *& );
	statusCode 					setBlindData( int, long, const char * );
	statusCode					removeBlindData( int );

	statusCode            		doUpdates( boolean newState = TRUE );

protected:
								AlPolysetVertex( Dag_node* );
	statusCode					create( AlPolyset*, int );
	statusCode					create( AlPolyset*, PS_Vertex* );

	Dag_node*					fParent;

private:
    boolean               		updateOn;
    boolean               		updateNeeded;
    void                  		updatePerform();

	static void					initMessages();
	static void					finiMessages();

	virtual boolean				extractType( int&, void*&, void*& ) const;
};

#endif
