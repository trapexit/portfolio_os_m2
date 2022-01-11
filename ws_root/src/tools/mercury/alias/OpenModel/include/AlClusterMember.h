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
//	.NAME AlClusterMember - Basic Interface to the object representing the relationship between an item and a cluster that it belongs to.
//
//	.SECTION Description
//
//		This class encapsulates the relationship that is cluster membership.
//		Whenever an object is placed into a cluster an AlClusterMember object
//		is created to represent the relationship.
//
//		Each AlClusterMember object knows the associated cluster object as well
//		as the geometry object which represents the item in the cluster.
//		Presently, this object can be an AlDagNode, AlSurfaceCV, AlCurveCV, or
//		AlPolysetVertex.  To determine an AlClusterMember's type can use the
//		following method:
//
//	.nf
//
//	%@ AlClusterMember* clusterMember;
//	%@ AlObject* objectMember;
//	%@ objectMember = clusterMember->object();
//	%@
//	%@ if( objectMember->asDagNodePtr() )
//	%@%@ ;// This member is an AlDagNode object
//	%@ else if( asCurveCVPtr( objectMember ) )
//	%@%@ ;// This member is an AlCurveCV object
//	%@ else if( asSurfaceCVPtr( objectMember ) )
//	%@%@ ;// This member is an AlSurfaceCV object
//	%@ else if( asPolysetVertexPtr( objectMember ) )
//	%@%@ ;// This member id an AlPolysetVertex object
//
//		Alternatively, the type() method in the AlObject class can be
//		used to determine the object type:
//
//	%@ AlClusterMember* clusterMember;
//	%@ AlObject* objectMember;
//	%@ objectMember = clusterMember->object();
//	%@ 
//	%@ switch( objectMember->type() )
//	%@ {
//	%@%@ case kDagNodeType:
//	%@%@ {
//	%@%@%@ AlDagNode *dagNode = objectMember->asDagNodePtr();
//	%@%@%@ ....
//	%@%@ }
//	%@ }
//
//	.fi
//
//		If an AlDagNode is a member of an AlCluster then every AlSurfaceCV,
//		AlCurveCV, or AlPolysetVertex that appears in the DAG hierarchy
//		underneath the AlDagNode is affected by the AlCluster.
//	.br
//		For example, if you wanted to set the percentage effects of all CVs
//		that were affected by an AlCluster, you would use the firstMember()
//		method from the AlCluster object, then walk along the AlClusterMember
//		list using the method nextClusterMember() in the AlClusterMember object.//		Whenever you encountered an AlDagNode member (as determined by the code
//		fragment above) you would recursively walk down the dag node to	find
//		all AlSurfaceCVs and AlCurveCVs below it in the dag.
//
//		The AlClusterMember object may not be created or destroyed directly.
//		The AlCluster object creates or destroys the AlClusterMember object
//		when the memberships of the AlCluster object change.
//
//		The print() method is an aid to debugging code.  It prints the
//		current contents of the cluster member object.
//

#ifndef _AlClusterMember
#define _AlClusterMember

#include <AlObject.h>
#include <AlClusterable.h>
#include <AlAnimatable.h>

extern "C" {
	struct Cl_Cluster_node;
	struct Cl_Subcluster;
	struct Cl_Cluster;
}

class AlClusterMember : public AlObject 
					  , public AlAnimatable 
{
	friend						class AlFriend;
public:

	virtual						~AlClusterMember();
	virtual AlObject*			copyWrapper() const;

	virtual AlObjectType		type() const;
	virtual AlClusterMember*	asClusterMemberPtr();
	virtual AlAnimatable*		asAnimatablePtr();

	AlClusterMember*			nextClusterMember() const;
	AlClusterMember*			prevClusterMember() const;

	statusCode					nextClusterMemberD();
	statusCode					prevClusterMemberD();

	AlObject*					object()  const;
	AlCluster*					cluster() const;
	
	statusCode					removeFromCluster( AlCluster* );

private:
								AlClusterMember( Cl_Cluster*, Cl_Subcluster* );

	Cl_Cluster*					fCluster;
	Cl_Subcluster*				fSubcluster;	// Used for internal ID purposes

	virtual	boolean extractType( int&, void*&, void*& ) const;

	static void					initMessages();
	static void					finiMessages();
};

#endif // _AlClusterMember
