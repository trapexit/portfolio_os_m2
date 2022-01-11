#ifndef _AlCurveAttributes
#define _AlCurveAttributes

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
//	.NAME AlCurveAttributes - Interface to Alias curve attributes.
//
//	.SECTION Description
//
//		AlCurveAttributes is a class derived from the AlAttributes class.
//		This class allows access to a curve.
//
//		This class provides information similar to that of the AlCurve
//		class but without the ability to modify the curve or its CVs.
//
//		NOTE: When an AlCurve has just an AlCurveAttribute attribute
//		the AlCurveAttribute provides no information that the AlCurve
//		doesn't. For this reason, in this case, no information is
//		provided by the AlCurveAttribute, all methods return null
//		values.
//

#include <AlAttributes.h>
#include <AlCurve.h>

class AlCurveAttributes : public AlAttributes {
	friend class AlFriend;

public:
	virtual AlObjectType			type() const;
	virtual AlCurveAttributes* 		asCurveAttributesPtr();
	AlObject*						copyWrapper() const;

	curveFormType			form() const;
	int						degree() const;
	int						numberOfSpans() const;
	int						numberOfCVs() const;
	statusCode				CVsUnaffectedPosition( double[][4], int[] ) const;
	int						numberOfKnots() const;
	statusCode				knotVector( double[] ) const;

protected:
							AlCurveAttributes( struct Spline_surface* );
	virtual					~AlCurveAttributes();

private:
	boolean					isPeriodic() const;
	boolean					isClosed() const;
	struct ag_cnode*		firstCV() const;
	struct ag_cnode*		nextCV( struct ag_cnode* ) const;
	statusCode				unaffectedPosition( struct ag_cnode*, double&, double&, double&, double& ) const;
	int						multiplicity(struct ag_cnode*) const;
};
#endif
