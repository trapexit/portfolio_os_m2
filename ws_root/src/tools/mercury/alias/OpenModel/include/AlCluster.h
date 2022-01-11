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
//  .NAME AlCluster - Encapsulates creation, deletion and manipulation of clusters.
//
//  .SECTION Description
//
//		This class encapsulates the functionality for creating,
//		manipulating and deleting a cluster.  A cluster is a group of
//		dag nodes and/or curve and surface control points, which
//		are grouped as such for deformation purposes.  Every cluster
//		has a cluster dag node which is in the universe's dag.  Adding and
//		removing nodes and control points to and from a cluster
//		does not affect the topology of the universe's dag.
//		Transforming the cluster dag node affects the transformations
//		of the objects in the cluster.
//
//		Empty clusters are allowed.  An object can be in more
//		than one cluster at a time, provided that those clusters are
//		of type kMultiCluster.  When an object is added to
//		a cluster, it is given a weight that indicates how much
//		of the cluster's leaf transformation is applied to the object.  
//		The default weight is 100%.  If a dag node is added to a
//		cluster the percentages of each individual CV may be manipulated
//		separately without actually adding the CVs themselves to the cluster.
//		
//		To create a cluster, the user must instantiate and call
//		create on an AlCluster object.  This also creates an AlClusterNode
//		which gets attached to the AlCluster and which is inserted 
//		into the universe's dag.  The user may not instantiate an
//		AlClusterNode or an AlClusterMember directly. 
//
//		There are two ways to delete a cluster object.  When a cluster
//		is deleted, its attached cluster node is deleted.  Alternatively,
//		when AlClusterNode::deleteObject() is used, its cluster is deleted.
//		The dag nodes and control points in a cluster are not deleted,
//		however the AlClusterMember objects that represented the "in a
//		cluster" relation are invalidated.
//
//		Clusters don't have names.  Any attempts to query for a name
//		will return NULL.
//

#ifndef _AlCluster
#define _AlCluster

#include <AlClusterable.h>
#include <AlModel.h>

extern "C" {
	struct Cl_Cluster;
	struct Cl_Subcluster;
	struct Cl_Cluster_node;
	struct CL_Pw;
}

class AlCluster : public AlObject {
	friend class		AlFriend;

public:
						AlCluster();
	virtual				~AlCluster();
	virtual statusCode	deleteObject();
	virtual AlObject*	copyWrapper() const;

	statusCode			create();

	AlObjectType		type() const;
	AlCluster* 			asClusterPtr();

	AlCluster*			nextCluster() const;
	statusCode			nextClusterD();

	AlCluster*			prevCluster() const;
	statusCode			prevClusterD();

	AlClusterNode*		clusterNode() const;
	boolean				isEmpty() const;

	int					numberOfMembers() const;
	AlClusterMember*	firstMember() const;

	statusCode			applyIteratorToMembers( AlIterator*, int& ) const;

	statusCode 			clusterRestrict( AlClusterRestrict& ) const;
	statusCode			setClusterRestrict( AlClusterRestrict );

	// Obsolete .. moved to AlClusterable, AlClusterMember
		statusCode			addMember( const AlClusterable*, double = 1.0 );
		statusCode			removeMember( AlClusterable* );
		statusCode			removeMember( AlClusterMember* );
		AlClusterMember*	hasMember( const AlClusterable* ) const;
		double				percentEffect( AlClusterable* ) const;
		statusCode			setPercentEffect( AlClusterable*, double );
		static statusCode	removeFromAllClusters( AlClusterable* );

private:
	static void			initMessages();
	static void			finiMessages();

	virtual boolean		extractType( int&, void *&, void *& ) const;
};

#endif
