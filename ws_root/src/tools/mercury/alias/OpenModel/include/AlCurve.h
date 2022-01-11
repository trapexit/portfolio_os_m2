#ifndef _AlCurve
#define _AlCurve

/*
//-
//	Copyright (C) 1995, Alias|Wavefront
//
//	These coded instructions,  statements and  computer programs contain
//	unpublished information proprietary to Alias|Wavefront  and are
// 	rotected by the Canadian and US Federal copyright law. They may not
//	be disclosed to third parties  or copied  or duplicated, in whole or
//	in part,  without the prior written consent of Alias|Wavefront
//
//	Unpublished-rights reserved under the Copyright Laws of the United States.
//+
//
//	.NAME AlCurve - Interface to Alias nurbs curves geometry.
//
//	.SECTION Description
//
//		AlCurve is the interface to the geometric data of Alias' NURBS
//		curve objects.  To create a curve, first instantiate and create
//		an AlCurve and then instantiate and create an AlCurveNode.  You
//		can't do much with an AlCurve that doesn't have an AlCurveNode.
//
//		For more information on how to create the curve geometry, see the
//		description for the create() method.
//
//		A curve form can be one of three types: periodic, closed or open.
//		If a curve is "kPeriodic", it is tangent continuous at all points
//		on the curve and its endpoints are coincident.  If a curve is 
//		"kClosed", it is not periodic but its 
//		endpoints are coincident.  If the curve is neither closed nor 
//		periodic, it is considered to be "kOpen".
//
//		There are two ways to delete an AlCurve.  If the AlCurve destructor
//		is called, the attached AlCurveNode is deleted.  If the AlCurveNode
//		destructor is called, the attached AlCurve is deleted.
//
//		You should always create a curve node for a curve.  If you create
//		a curve with no curve node, the curve is not added to the
//		universe.  If you should lose the pointer to the curve, it will 
//		become lost memory.
//
//		There is one limitation to this class: you cannot add or remove
//		individual curve control points (except by changing multiplicity).
//
//		Curves are made of curve control points (or CV's) which you can 
//		traverse as a list by using AlCurve::firstCV() plus the AlCurveCV
//		methods.  You can also pack the curve information into an array
//		using methods in this class.
//
//		What is multiplicity?
//
//		An AlCurveCV object can actually represent multiple CV's, depending
//		on the AlCurveCV's multiplicity and periodicity.  Notice that in
//		this class there are two sets of methods - some "InclMultiples"
//		and some not (namely "numberOfCVs", "CVsWorldPosition", 
//		"CVsUnaffectedPosition", etc).  The set of methods without
//		multiplicity lets you get all curve CV's where a curve CV can have
//		multiplicity of 1, 2 or 3.  The set of methods "InclMultiples"
//		lets you get ALL curve CV's including multiples due to a multiplicity
//		> 1 and due to periodicity.
//
//		Example 1:
//
//		If you create a curve in the interactive Alias package with
//		4 CV's and change the multiplicity of the second CV to 2 (using
//		the "multiplicity" menu item in the Curve Tools menu), then:
// .br
//		1.  numberOfCVs() will return 4.
// .br
//		2.  CVsWorldPosition must be passed a 4x4 CVs matrix, and a
//			multiplicity vector of length 4.
// .br
//		3.  CVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		4.  setCVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		5.  numberOfCVsInclMultiples() will return 5.
// .br
//		6.  CVsWorldPositionInclMultiples() must be passed a 5x4 CVs matrix.
//		    You will notice that the 2nd and 3rd points in this matrix are
//		    the same.
// .br
//		7.  CVsUnaffectedPositionInclMultiples() must be passed the 
//		    same as item 6.
//		    You will notice that the 2nd and 3rd points in this matrix are
//		    the same.
// .br
//		8.  setCVsUnaffectedPositionInclMultiples() must be passed the 
//		    same as item 6.
//			The matrix you pass in should have the 2nd and 3rd points the
//			same.  What really happens in this method is that the 3rd 
//			point is given the same value as the 2nd point.
//			(What you give as the 3rd point is ignored).
//
//		Example 2:
//
//		If you create a curve in the interactive Alias
//		package with 4 CV's and "close" the curve (using the
//		"close" menu item in the Object Tools menu), you create a periodic
//		curve.  Then:
// .br
//		1.  numberOfCVs() will return 4.
// .br
//		2.  CVsWorldPosition must be passed a 4x4 CVs matrix, and a
//			multiplicity vector of length 4.
// .br
//		3.  CVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		4.  setCVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		5.  numberOfCVsInclMultiples() will return 7.
// .br
//		6.  CVsWorldPositionInclMultiples() must be passed a 7x4 CVs matrix.
//		    You will notice that in this matrix the 5th, 6th and 7th points
//			are the same as the 1st, 2nd and 3rd points respectively.
// .br
//		7.  CVsUnaffectedPositionInclMultiples() must be passed the 
//		    same as item 6.
//		    You will notice that in this matrix the 5th, 6th and 7th points
//			are the same as the 1st, 2nd and 3rd points respectively.
// .br
//		8.  setCVsUnaffectedPositionInclMultiples() must be passed the 
//		    same as item 6.  
//		    The matrix you pass in should have the 5th, 6th and 7th points 
//			the same as the 1st, 2nd and 3rd points respectively.
//			What really happens in this method is that the 5th, 6th and 7th
//			points are assigned the same values as the 1st, 2nd and 3rd
//			points respectively.  (What you give for the 5th, 6th and 7th 
//			points are ignored).
//
*/

#ifdef __cplusplus

#include <AlModel.h>
#include <AlObject.h>
#include <AlCurveNode.h>
#include <AlStyle.h>
#include <AlTM.h>

#include <AlIterator.h>

struct Spline_surface;

class AlCurveCV;

class AlCurve : public AlObject {
	friend class			AlFriend;
public:
							AlCurve();
	virtual					~AlCurve();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( int, curveFormType, int, const double[], int, const double[][4], const int[]);
	statusCode				createLine( const double[3], const double[3] );
	statusCode				createArc( const double[3], const double[3], const double[3], boolean );
	statusCode				createConic( double, double, double, double, double, double, const double[3], const double[3] );
	statusCode				createParabola( const double[3], const double[3], const double[3], const double[3] );
	statusCode				createEllipse( const double[3], const double[3], const double[3], const double[3], const double[3] );
	statusCode				createHyperbola( const double[3], const double[3], const double[3], const double[3], const double[3] );

	virtual AlObjectType	type() const;
	virtual AlCurve* 		asCurvePtr();
	AlCurveNode* 			curveNode() const;

	curveFormType			form() const;
	int						degree() const;
	int						numberOfSpans() const;
	int						numberOfCVs() const;
	AlCurveCV*				firstCV() const;
	AlAttributes*			firstAttribute() const;
	statusCode				CVsWorldPosition( double[][4], int[] ) const;
	statusCode				CVsAffectedPosition( const AlTM&, double[][4], int[] ) const;
	statusCode				CVsUnaffectedPosition( double[][4], int[] ) const;
	virtual statusCode		setCVsUnaffectedPosition( const double[][4] );

	statusCode				length( double& len, boolean worldCoords = TRUE, double tolerance=0.001 );
	statusCode				eval(double, boolean, double P[3] = NULL, double dP[3] = NULL ) const;
	int						numberOfKnots() const;
	statusCode				knotVector( double[] ) const;
	statusCode				setKnotVector( const double[] );

	int						numberOfCVsInclMultiples() const;
	statusCode				CVsWorldPositionInclMultiples( double[][4] )const;
	statusCode				CVsAffectedPositionInclMultiples( const AlTM&, double[][4] ) const;
	statusCode				CVsUnaffectedPositionInclMultiples( double[][4] ) const;
	virtual statusCode		setCVsUnaffectedPositionInclMultiples( const double[][4] );

	int						realNumberOfKnots() const;
	statusCode				realKnotVector( double[] ) const;
	statusCode				setRealKnotVector( const double[] );

	boolean					isDisplayModeSet( AlDisplayModeType ) const;
	statusCode				setDisplayMode( AlDisplayModeType, boolean );

	statusCode				applyIteratorToCVs( AlIterator*, int& );

	statusCode				doUpdates( boolean newState = TRUE );

protected:

	static curveFormType	getForm( Spline_surface* );
	static boolean			isPeriodic( Spline_surface* );
	static boolean			isClosed( Spline_surface* );

	// Method to get the Dag_node * of the parent curve Node
	//
	Dag_node*				curveDagNode() const;

private:
	boolean					updateOn;
	boolean					updateNeeded;
	void					updatePerform();

	static void				initMessages();
	static void				finiMessages();
};

#endif /* __cplusplus */ 

#endif
