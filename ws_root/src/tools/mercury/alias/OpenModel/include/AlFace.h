#ifndef _AlFace
#define _AlFace

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
//	.NAME AlFace - Interface to Alias face curves.
//
//	.SECTION Description
//		An AlFace is derived from AlCurve and inherits AlCurve's public methods.
//
//		AlFace objects are created independently from the AlFaceNode, and
//		then added to the AlFaceNode afterwards.  An AlFaceNode requires
//		one valid AlFace object in order for it to be created.  Other
//		faces can be added to the list of faces under the AlFaceNode
//		afterwards using AlFaceNode::addFace().
//
//		Deleting a face can cause one of two things to happen.  If the AlFace
//		is not under an AlFaceNode, or it is one of several faces under an
//		AlFaceNode, then only the face will be deleted.  If the AlFace is
//		the only face under the AlFaceNode, then the AlFaceNode will also
//		be deleted.
//
//		Each face curve must be planar and each face curve must lie in the same
//		plane as all the others.  In addition to the parent class (AlCurve)
//		methods this class allows you to walk through the list of the face
//		curves that make up the face.
//
//		All AlFace objects will have at least one shader attached to them.
//		These can be accessed through the firstShader and nextShader methods.
//

#include <AlCurve.h>
#include <AlShader.h>
#include <AlRenderInfo.h>

class AlFriend;
class AlFaceNode;

class AlFace : public AlCurve {
	friend class			AlFriend;
public:

							AlFace();
	virtual					~AlFace();
	virtual statusCode		deleteObject();
	virtual AlObject*		copyWrapper() const;

	statusCode          	create( int, curveFormType, int, const double[], int, const double[][4], const int[] );

	virtual AlObjectType	type() const;
	virtual AlFace* 		asFacePtr();

	AlFaceNode*				faceNode() const;

	AlFace*					prevFace() const;
	AlFace*					nextFace() const;

	statusCode				prevFaceD();
	statusCode				nextFaceD();

	statusCode				area( double&, boolean worldCoords=TRUE, double otlerance=0.001 );
	AlShader*				firstShader() const;
	AlShader*				nextShader( AlShader* ) const;
	statusCode				nextShaderD( AlShader* ) const;

	statusCode				renderInfo( AlRenderInfo& ) const;
	statusCode				setRenderInfo( const AlRenderInfo& ) const;

	statusCode				normal( double[3] ) const;
};

#endif
