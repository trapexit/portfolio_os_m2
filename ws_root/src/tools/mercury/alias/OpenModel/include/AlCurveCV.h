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
//  .NAME AlCurveCV - Encapsulates methods common to curve CVs.
//
//  .SECTION Description
//
//      AlCurveCV is the class used to access and manipulate
//      curve CV's (also referred to as Control Points). 
//		There is one AlCurveCV object for each CV of an AlCurve as you see
//		it in the interactive Alias package.  You cannot create or delete 
//		an AlCurveCV.  AlCurveCV's are only created through AlCurve's.
//
//		There are methods to query and set the multiplicity of a CV,
//		and method to query the world position and the "unaffected"
//		position of a CV.  The "unaffected" position of a CV is its
//		position BEFORE any transformations (from its parent dag node
//		or from affecting clusters) have been applied to it.
//		There is also a method to access the clusters that affect a CV.
//
//		For example, this is one way to print out all CV's of a
//		curve:
//
//:	AlCurve     curve;
//:	AlCurveCV  *cvPtr;
//:	int         i = 0;
//:	double      x, y, z, w;
//:
//:	if ( cvPtr = curve->firstCV() )
//:	{
//:		do {
//:			cvPtr->worldPosition( x, y, z, w );
//:			printf("CV %d is at %G, %G, %G with weight %G.\n", i, x, y, z, w );
//:			i++;
//:		} while( cvPtr->nextD() == sSuccess );
//:	}
//:	delete cvPtr;
//:
//:      Example Result:
//:          CV 1 is at 0.0, 1.0, 0.0 with weight 1.0
//:          CV 2 is at 0.0, 2.0, 0.0 with weight 1.0
//:          CV 3 is at 0.0, 3.0, 0.0 with weight 1.0
//:          CV 4 is at 0.0, 4.0, 0.0 with weight 1.0
//:
//
//	Multiplicity:
//
//		When an AlCurveCV has a multiplicity > 1, then it means that CV's
//		are internally piled up at that position.  In this case, an AlCurveCV 
//		actually represents more than one CV internally.  To access ALL CV's,
//		use the methods in the AlCurve class to query values "including
//		multiplicity", or use the example code below.  By representing 
//		AlCurveCV's this way, then if an AlCurveCV is moved, the corresponding 
//		multiple CV's at that position are moved as well, acting exactly 
//		like the Alias interactive package. 
//
//		For example, to print all internal CV's of a curve with 4 AlCurveCV's
//		where the second AlCurveCV has multiplicity = 2, you would do this:
//
//:	AlCurve		curve;
//:	AlCurveCV	*cvPtr;
//:	int			i,j = 0;
//:	double		x, y, z, w;
//:
//:	if ( cvPtr = curve->firstCV() )
//:	{
//:		do {
//:			for( j = 0; j <= cvPtr->multiplicity(); j++, i++ )
//:			{
//:				cvPtr->worldPosition( x, y, z, w );
//:				printf("CV %d is at %G, %G, %G with weight %G.\n", i, x, y, z, w );
//:			}
//:			i++;
//:		} while( cvPtr->nextD() == sSuccess );
//:	}
//:	delete cvPtr;
//:
//:      Example Result:
//:          CV 1 is at 0.0, 1.0, 0.0 with weight 1.0
//:          CV 2 is at 0.0, 2.0, 0.0 with weight 1.0
//:          CV 3 is at 0.0, 2.0, 0.0 with weight 1.0
//:          CV 4 is at 0.0, 3.0, 0.0 with weight 1.0
//:          CV 5 is at 0.0, 4.0, 0.0 with weight 1.0
//
//	Creation:
//
//		Note that certain routines require that the given CV be installed
//		in the Dag (i.e., that the curve belongs to a AlCurveNode).  If
//		any of these are called on a CV that's not part of the Dag, they
//		will simply fail.
//
//		Routines that require the CV to be in the Dag:
//	.nf
//			next
//			nextD
//			worldPosition
//			setMultiplicity
//			setUnaffectedPosition
//	.fi
//			


#ifndef _AlCurveCV
#define _AlCurveCV

#include <AlClusterable.h>
#include <AlAnimatable.h>
#include <AlSettable.h>
#include <AlPickable.h>
#include <AlTM.h>

struct ag_cnode;
struct Dag_node;
struct Spline_surface;

class AlCurveCV : public AlObject
				, public AlClusterable
				, public AlAnimatable
				, public AlSettable
				, public AlPickable
{
	friend					class AlFriend;
public:

	virtual AlAnimatable*	asAnimatablePtr();
	virtual AlSettable*		asSettablePtr();
	virtual AlClusterable*	asClusterablePtr();
	virtual AlPickable*		asPickablePtr();
	virtual					~AlCurveCV();
	virtual AlObject*		copyWrapper() const;

	AlObjectType		type() const;
	AlCurveCV*			asCurveCVPtr();	

	AlCurveCV*			next() const;
	AlCurveCV*			prev() const;
	statusCode			nextD();
	statusCode			prevD();

	int					multiplicity() const;
	statusCode			worldPosition( double&, double&, double&, double& ) const;
	statusCode			affectedPosition( const AlTM&, double&, double&, double&, double& ) const;
	statusCode			unaffectedPosition( double&, double&, double&, double& ) const;
	statusCode			setMultiplicity( int );
	statusCode			setUnaffectedPosition( double, double, double, double );

	AlCurve*			curve() const;

	statusCode			blindData( int, long&, const char *& );
	statusCode 			setBlindData( int, long, const char * );
	statusCode 			removeBlindData( int );

	statusCode			doUpdates( boolean newState = TRUE );

protected:
						AlCurveCV( Spline_surface* );

private:
	boolean				updateOn;
	boolean				updateNeeded;
	void				updatePerform();

	virtual	boolean 	extractType( int&, void*&, void*& ) const;

	static void			initMessages();
	static void			finiMessages();

	Spline_surface*		fParent;
};
#endif
