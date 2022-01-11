//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//  .NAME AlSurfaceCV - Encapsulates methods common to surface CVs.
//
//  .SECTION Description
//
//      AlSurfaceCV is the class used to access and manipulate
//      surface CV's (also referred to as Control Points). 
//		There is one AlSurfaceCV object for each CV of an AlSurface as you see
//		it in the interactive Alias package.  You cannot create or delete 
//		an AlSurfaceCV.  AlSurfaceCV's are only created through AlSurface's.
//
//		There are methods to query and set the multiplicity of a CV,
//		and method to query the world position and the "unaffected"
//		position of a CV.  The "unaffected" position of a CV is its
//		position BEFORE any transformations (from its parent dag node
//		or from affecting clusters) have been applied to it.
//		There is also a method to access the clusters that affect a CV.
//
//		For more information on CVs please see AlCurveCV.
//

#ifndef _AlSurfaceCV
#define _AlSurfaceCV

#include <AlClusterable.h>
#include <AlAnimatable.h>
#include <AlSettable.h>
#include <AlPickable.h>
#include <AlTM.h>

struct ag_snode;
typedef ag_snode AG_SNODE;
struct Spline_surface;

class AlSurfaceCV	: public AlObject
					, public AlClusterable
					, public AlAnimatable
					, public AlSettable
					, public AlPickable
{
	friend class		AlFriend;
	friend class		AlSurface;
public:

	virtual				~AlSurfaceCV();
	virtual AlObject*	copyWrapper() const;

	virtual AlAnimatable*	asAnimatablePtr();
	virtual AlSettable*		asSettablePtr();
	virtual AlClusterable*	asClusterablePtr();
	virtual AlPickable*		asPickablePtr();

	AlObjectType		type() const;
	AlSurfaceCV*		asSurfaceCVPtr();	

	AlSurfaceCV*		nextInU() const;
	AlSurfaceCV*		nextInV() const;
	AlSurfaceCV*		prevInU() const;
	AlSurfaceCV*		prevInV() const;

	statusCode			nextInUD();
	statusCode			nextInVD();
	statusCode			prevInUD();
	statusCode			prevInVD();

	int					multiplicityInU() const;
	int					multiplicityInV() const;
	statusCode			worldPosition( double&, double&, double&, double& ) const;
	statusCode			affectedPosition( const AlTM&, double&, double&, double&, double& ) const;
	statusCode			unaffectedPosition( double&, double&, double&, double& ) const;

	statusCode			setMultiplicity( int, int );
	statusCode			setUnaffectedPosition( double, double, double, double );

	AlSurface*			surface() const;

	statusCode			blindData( int, long&, const char *& );
	statusCode 			setBlindData( int, long, const char * );
	statusCode          removeBlindData( int );

	statusCode			applyIteratorToCVsInU( AlIterator* iter, int &rc );
	statusCode			applyIteratorToCVsInV( AlIterator* iter, int &rc );

	statusCode			doUpdates( boolean newState = TRUE );

protected:
						AlSurfaceCV( Spline_surface* );
private:
	boolean				updateOn;
	boolean				updateNeeded;
	void				updatePerform();

	virtual boolean		extractType( int&, void*&, void*& ) const;

	static void         initMessages();
	static void         finiMessages();

	Spline_surface*		fParent;
};
#endif
