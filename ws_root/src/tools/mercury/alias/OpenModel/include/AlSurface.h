#ifndef _AlSurface
#define _AlSurface

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
//
//+
//
//	.NAME AlSurface - Interface to Alias nurbs surface geometry.
//
//	.SECTION Description
//		
//		AlSurface is the interface to the geometric data of Alias' NURBS
//		surface objects.  To create a surface, first instantiate and create
//		an AlSurface and then instantiate and create an AlSurfaceNode.  There
//		is only a limited amount that you can do with an AlSurface that does
//		not have an AlSurfaceNode.
//	
//		For more information on how to create the surface geometry, see the
//		description for the create() method.
//
//		NOTE: In V5.0 multifillet operations may be animated. This can
//		cause geometry to be deleted and recreated during a viewframe
//		operation. The method isAffectedByViewFrame() will return TRUE
//		if the viewframe may delete the surface CVs, and FALSE if it will not.
//		See the firstCV() method for additional information.
//
//		A NURBS surface is described as having two dimensions u and v.
//		Many of the surface properties (form, multiplicity, etc.) are the
//		same as a NURBS curve but are expressed in both the u and
//		v direction.
//
//		The form of a surface in the u or v direction can be one of three types:
//		periodic, closed or open.  If a surface is "kPeriodic" in u, then
//		it is tangent continuous in u at all points in that direction.
//		If a surface is "kClosed" in u, it is not periodic but its 
//		edges in the u direction  are coincident.  If the surface 
//		is neither closed nor periodic in u, it is considered to be "kOpen".
//		The same applies in the v direction.
//
//		There are two ways to delete an AlSurface.  If the AlSurface 
//		deleteObject() is called, the attached AlSurfaceNode is
//		deleted.  If the AlSurfaceNode deleteObject() is called, the attached
//		AlSurface is deleted.
//
//		You should always create a surface node for a surface.  If you
//		create a surface with no surface node, then the surface is not
//		added to the universe.  If you should lose the pointer to the
//		the surface, it will become lost memory.
//
//		There is one limitation to this class:  you cannot add or remove
//		individual CV's (except by changing multiplicity).
//	
//		If an AlSurface has curves-on-surface, then you can get a pointer
//		to the first curve-on-surface and then traverse the curves using
//		methods in the class AlCurveOnSurface.
//
//		If an AlSurface is trimmed, the trim regions on the surface can be
//		accessed using the firstTrimRegion() method and then traversed
//		using methods in the class AlTrimRegion.
//
//		All AlSurface objects will have at least one shader attached to them.
//		These can be accessed through the firstShader() and nextShader() methods.
//
//		All AlSurface objects have render information attached to them.
//		The AlRenderInfo structure can be used to query the surface's
//		render parameters.
//
//		Surfaces are made of surface control points (or CV's) which you 
//		traverse as a list by using the firstCV() in conjunction with the
//		AlSurfaceCV methods.  You can also pack the surface information
//		into an array using methods in this class.
//
//		What is multiplicity?
//
//		An AlSurfaceCV object can actually represent multiple CV's, depending
//		on the AlSurfaceCV's multiplicity and periodicity.  Notice that in
//		this class there are two sets of methods - some "InclMultiples"
//		and some not (namely "numberOfCVs", "CVsWorldPosition",
//		"CVsUnaffectedPosition", etc).  The set of methods without
//		multiplicity lets you get all surface CV's where a surface CV can have
//		multiplicity of 1, 2 or 3.  The set of methods "InclMultiples"
//		lets you get ALL surface CV's including multiples due to a multiplicity
//		> 1 and due to periodicity.
//
//		Example 1:
//
//		Create a surface in the interactive Alias package with
//		16 CV's, and pick the CV which is 2nd in the
//		u direction and 2nd in the v direction.  Set the multiplicity
//		of this CV to 3 in the u direction and 2 in the v direction (using
//		the "multiplicity" menu item in the Curve Tools menu). Then:
// .br
//		1. uNumberOfCVs() will return 4. uNumberOfCVs() will return 4.
// .br
//		2. CVsWorldPosition must be passed a 4x4x4 CVs matrix, and 
//			the u and v multiplicity vectors must each be of length 4.
// .br
//		3. CVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		4. setCVsUnaffectedPosition() must be passed the same as item 2.
// .br
//
//		5. uNumberOfCVsInclMultiples() will return 6. 
//			vNumberOfCvsInclMultiples() will return 5.
// .br
//		6. CVsWorldPositionInclMultiples() must be passed a 6x5x4 CVs matrix.
//			You will notice that in this matrix there are duplicate CVs
//			to indicate multiples due to mutiplicity > 1 and periodicity.
// .br
//		7. CVsUnaffectedPositionInclMultiples() must be passed the same as item 6.
//			You will notice that in this matrix there are duplicate CVs
//			to indicate multiples due to mutiplicity > 1 and periodicity.
// .br
//		8. setCVsUnaffectedPositionInclMultiples() must be passed the
//			same as item 6.
//			Similar to items 6 and 7, you should put duplicate CVs
//			to account for multiples although this is not mandatory.  
//			You may want to refer to the examples in the AlCurve class 
//			description.  The way CVs are ignored also applies to this method.
//
//	  Example 2:
//
//		If you create a surface in the interactive Alias
//		package with 16 CV's and "close" the surface in the u direction
//		(using the "close" menu item in the Object Tools menu), you 
//		create a periodic surface.  Then:
// .br
//		1. uNumberOfCVs() will return 4. uNumberOfCVs() will return 4.
// .br
//		2. CVsWorldPosition must be passed a 4x4x4 CVs matrix, and 
//			the u and v multiplicity vectors must each be of length 4.
// .br
//		3. CVsUnaffectedPosition() must be passed the same as item 2.
// .br
//		4. setCVsUnaffectedPosition() must be passed the same as item 2.
// .br
//
//		5. uNumberOfCVsInclMultiples() will return 7.
//			uNumberOfCVsInclMultiples() will return 4.
// .br
//		6. CVsWorldPositionInclMultiples() must be passed a 7x4x4 CVs matrix.
// .br
//		7. CVsUnaffectedPositionInclMultiples() must be passed the
//			same as item 6.
// .br
//		8. setCVsUnaffectedPositionInclMultiples() must be passed the
//			same as item 6.
//			Similar to items 6 and 7, you should put duplicate CVs
//			to account for multiples although this is not mandatory.  
//			You may want to refer to the examples in the AlCurve class 
//			description.  The way CVs are ignored also applies to this method.
//
// .br
//		How do I process a matrix of CV's?
//
//		Methods in this class store CV's in the V direction first, then
//		in the U direction.  Here's an example of how to get the world
//		positions of the CV's of a surface and print them out in
//		V direction then U direction.  (Notice that the outer "for" loop
//		uses the number of CV's in the U direction).
//
//	.nf
// %@ int  numUCvs = surface->uNumberOfCVs();
// %@ int  numVCvs = surface->vNumberOfCVs();
// %@ double *cvs = new double [numUCvs * numVCvs * 4];
// %@ int  *uMult = new int [numUCvs];
// %@ int  *vMult = new int [numVCvs];
// %@ int  u, v, index;
//
// %@ surface->CVsWorldPosition( cvs, uMult, vMult );
// %@ for( index = 0, u = 0; u < numUCvs; u++ ) {
// %@%@ for( v = 0; v < numVCvs; v++, index+=4 ) {
// %@%@%@ printf(" %g, %g, %g, %g\n",
// %@%@%@ cvs[index], cvs[index+1], cvs[index+2], cvs[index+3] );
// %@%@ }
// %@ }
//	.fi
//
//

#include <AlObject.h>
#include <AlModel.h>
#include <AlTM.h>

#define AL_UNPILE_START_U	1
#define AL_UNPILE_START_V	2
#define AL_UNPILE_END_U		4
#define AL_UNPILE_END_V		8

struct Spline_surface;
struct ag_snode;
struct Dag_node;

class AlTrimRegion;
class AlCurveOnSurface;
struct AlRenderInfo;

class AlSurface : public AlObject {
	friend class			AlFriend;

public:
							AlSurface();
	virtual					~AlSurface();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( int, int, curveFormType, curveFormType, int, int, const double[], const double[], int, int, const double[], const int[], const int[] );
	statusCode				createRevolvedSurface(const double[3], const double[3], double, double, const AlCurve* );

	virtual AlObjectType	type() const;
	virtual AlSurface*		asSurfacePtr();
	AlSurfaceNode*			surfaceNode() const;

	curveFormType			uForm() const;
	curveFormType			vForm() const;
	int						uDegree() const;
	int						vDegree() const;
	int						uNumberOfSpans() const;
	int						vNumberOfSpans() const;
	int						uNumberOfCVs() const;
	int						vNumberOfCVs() const;

	AlSurfaceCV*			firstCV() const;
	AlAttributes*			firstAttribute() const;

	boolean					isAffectedByViewFrame() const;

	// specific to AlSurface
	statusCode				setCVsUnaffectedPosition( const double[]);
	statusCode				setuKnotVector( const double[] );
	statusCode				setvKnotVector( const double[] );
	statusCode				setCVsUnaffectedPositionInclMultiples( const double[] );
	statusCode				setRealuKnotVector( const double[] );
	statusCode				setRealvKnotVector( const double[] );
	statusCode				unpileEndKnots( int, const double[] );

	boolean					isPointActive( double u, double v ) const;

	// Common to AlSurface, AlTrimRegion
	statusCode				CVsWorldPosition( double[], int[], int[])const;
	statusCode				CVsAffectedPosition( const AlTM&, double[], int[], int[]) const;
	statusCode				CVsUnaffectedPosition( double[], int[], int[])const;

	int						uNumberOfKnots() const;
	int						vNumberOfKnots() const;
	statusCode				uKnotVector( double[] ) const;
	statusCode				vKnotVector( double[] ) const;

	int						uNumberOfCVsInclMultiples() const;
	int						vNumberOfCVsInclMultiples() const;
	statusCode				CVsWorldPositionInclMultiples( double[] ) const;
	statusCode				CVsAffectedPositionInclMultiples( const AlTM&, double[] ) const;
	statusCode				CVsUnaffectedPositionInclMultiples( double[] )const;

	int						realuNumberOfKnots() const;
	int						realvNumberOfKnots() const;
	statusCode				realuKnotVector( double[] ) const;
	statusCode				realvKnotVector( double[] ) const;

	statusCode				area( double& area, boolean worldCoords = TRUE, double tolerance=0.001 );
	statusCode				circumference( double& circ, boolean worldCoords = TRUE, double tolerance=0.001 );

	statusCode				eval(double,double,boolean,double P[3], double Pu[3]=NULL, double Pv[3]=NULL, double n[3]=NULL,boolean=FALSE,boolean=FALSE ) const;

	// Specific to AlSurface
	AlShader*				firstShader() const;
	AlShader*				nextShader( const AlShader* ) const;
	statusCode				nextShaderD( AlShader* ) const;

	statusCode				assignShader( AlShader* );
	statusCode				layerShader( AlShader* );

	statusCode				renderInfo( AlRenderInfo& ) const;
	statusCode				setRenderInfo( const AlRenderInfo& ) const;

	boolean					trimmed() const;
	boolean					isTargetSurface() const;

	statusCode				project( AlCurveNode*, double[3], boolean );
	statusCode				trim( int, const double[], const double[] );
	statusCode				trim( int, const AlCurveOnSurface*[] ); 

	statusCode				uniformRebuild( AlSurfaceNode* &newSurfaceNode, int nu, int nv, boolean inU, boolean inV, boolean=TRUE );

	AlTrimRegion*			firstTrimRegion() const;
	AlCurveOnSurface*		firstCurveOnSurface() const;
	statusCode				addCurveOnSurface( AlCurveOnSurface* );
	statusCode				removeCurveOnSurface( AlCurveOnSurface* );

	boolean					isDisplayModeSet( AlDisplayModeType ) const;
	statusCode				setDisplayMode( AlDisplayModeType, boolean );

	statusCode				doUpdates( boolean newState = TRUE );

protected:
	boolean					isClosedInU() const;
	boolean					isClosedInV() const;

private:
	boolean					updateOn;
	boolean					updateNeeded;
	void					updatePerform();

	static void				initMessages();
	static void				finiMessages();
};

#endif
