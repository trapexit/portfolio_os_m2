#ifndef _AlFaceNode
#define _AlFaceNode

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
//	.NAME AlFaceNode - Interface to the dag node that gives access to faces.
//
//	.SECTION Description
//		AlFaceNode is the class used to access faces in the dag.  Faces
//		can be created from scratch or read in from a wire file and
//		accessed via the firstFace() method.  Faces are a collection of
//		curves that all lie in the same plane.  An AlFaceNode points to
//		an AlFace, which points to the next face in the collection etc...
// .br
//		Add faces to the collection with the addFace() method and
//		remove them with the removeFace() method.  In order to access the
//		list of faces, you walk through the face list with AlFace
//		methods nextFace() and prevFace().
// .br
//		There are two ways to delete an AlFaceNode.  If the
//		AlFaceNode::deleteObject() method is called, then this node's AlFace
//		objects are all deleted.  If this node only has one face and its
//		deleteObject() method is called, then this node is deleted as well.
//


#include <AlCurveNode.h>

class AlFriend;

class AlFaceNode : public AlCurveNode {
	friend	class			AlFriend;
public:
							AlFaceNode();
	virtual					~AlFaceNode();
	virtual AlObject*		copyWrapper() const;

	statusCode				create( AlFace * );

	virtual AlObjectType	type() const;
	virtual AlFaceNode*		asFaceNodePtr();

	AlFace*					firstFace() const;
	AlFace*					firstFace(AlTM&) const;

	statusCode				addFace( AlFace* );
	statusCode				removeFace( AlFace* );

	AlSurface*				convertToTrimmedSurface( boolean worldSpace =FALSE )const;
	statusCode				normal( double &x, double &y, double &z) const;
protected:
};

#endif
