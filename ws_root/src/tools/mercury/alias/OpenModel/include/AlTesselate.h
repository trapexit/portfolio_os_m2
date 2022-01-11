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
//
//+
*/

/*
//
//	.NAME AlTesselate - All the methods to tesselate geometry.
//
//	.SECTION Description
//		This is a static class in that all of its member functions are
//		static.  It provides the functionality to tesselate geometry.
//
//		The methods in this library will generate polysets from the
//		geometry below the AlDagNode, composed of either triangles
//		or quadrilaterals based on the type argument to the method
//		(either kTESSELATE_TRIANGLE or kTESSELATE_QUADRILATERAL).
//
//		Note that when tesselating a trimmed surface some triangles
//		may be created along the trim edge even though quadrilateral
//		tesselation was selected.
*/

#ifndef _AlTesselate
#define _AlTesselate

typedef enum 
	{ kTESSELATE_TRIANGLE, kTESSELATE_QUADRILATERAL } 
AlTesselateTypes;

#ifdef __cplusplus 

#include <AlObject.h>

struct PS_ConversionParams;
struct Dag_node;

class AlDagNode;


class AlTesselate {
public:
	static statusCode	uniform(AlDagNode* &outdag, const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 2, int = 2);
	static statusCode	adaptive(AlDagNode* &outdag, const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 2, int = 4, double = 0.6, int = 2);
	static statusCode	number(AlDagNode* &outdag, const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 512, int = 32, double = 0.001);

	// OBSOLETE - do not use
	static AlDagNode*	uniform( const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 2, int = 2);
	static AlDagNode*	adaptive( const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 2, int = 4, double = 0.96);
	static AlDagNode*	number( const AlDagNode*, AlTesselateTypes = kTESSELATE_TRIANGLE, int = 512, int = 32, double = 0.01);
};

#endif /* __cplusplus */

#endif /* _AlTesselate */
