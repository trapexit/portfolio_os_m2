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
//  .NAME AlClusterable - Encapsulates methods common to Alias objects which 
//		  can belong to clusters.
//
//  .SECTION Description
//
//		This class is a base class for all objects which can be
//		contained in a cluster. It provides the methods necessary
//		to access the cluster methods of these objects.
//

#ifndef _AlClusterable
#define _AlClusterable

#include <AlObject.h>
#include <AlTM.h>
#include <AlIterator.h>

//-
//	Note that this class relies on the fact the classes derived from it 
//	overload the extractType function in AlObject.  In theory, we should be
//	introducing a new base class, AlTypeable, which has a pure virtual 
//	extractType method.  But this introduces a lot of extra code.  Let's just
//	assume for now that if AlFoo inherits AlClusterable, then AlFoo provides
//	a working extractType method.
//+

class AlCluster;
class AlClusterMember;

class AlClusterable {
	friend class		AlFriend;
public:

	virtual AlCluster*	firstCluster() const;

	virtual AlCluster*	nextCluster( const AlCluster* ) const;
	virtual AlCluster*	prevCluster( const AlCluster* ) const;

	virtual statusCode	nextClusterD( AlCluster* ) const;
	virtual statusCode	prevClusterD( AlCluster* ) const;

	virtual statusCode	applyIteratorToClusters( AlIterator*, int& );

	statusCode          addToCluster( AlCluster*, double = 1.0 );
    statusCode          removeFromCluster( AlCluster* );
    statusCode			removeFromAllClusters();
    AlClusterMember*    isClusterMember( AlCluster* ) const;
    double              percentEffect( AlCluster* ) const;
    statusCode          setPercentEffect( AlCluster*, double );

protected:
						AlClusterable();
	virtual				~AlClusterable();
	AlClusterable*		clusterablePtr();	

	virtual boolean		extractType( int&, void *&, void *& ) const;
};
#endif
