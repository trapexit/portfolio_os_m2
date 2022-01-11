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
//	.NAME AlSet - Basic Interface to Alias set structures.
//
//	.SECTION Description
//		A set can be described as a storable list.  A set can contain
//		any combination of cameras, dag nodes, curve CV's and surface CV's, 
//      or polyset vertices.
//		A set can be exclusive or multi - exclusive means that a member 
//		of a set can be in no other set; multi means that a member of the
//		set can be in any other non-exclusive set.
//
//		You can access the members of a set by traversing a set's list of
//		members. 
//
//		If you remove all members of a set, the empty AlSet object must be
//		explicitly deleted.  If you store an empty set, it will be lost
//		when you retrieve your wire file into the interactive Alias package.
//
//		The following classes can be grouped into sets: AlDagNode, AlCamera,
//		AlCurveCV, AlPolysetVertex and AlSurfaceCV.
//

#ifndef _AlSet
#define _AlSet

#include <AlSettable.h>
#include <AlObject.h>

struct Set;
struct Dag_node;

class AlSetMember;


class AlSet : public AlObject {
	friend class			AlFriend;

public:
							AlSet();
	virtual					~AlSet();
	virtual statusCode		deleteObject();
	virtual AlObject		*copyWrapper() const;

	statusCode				create( const boolean );

	virtual AlObjectType	type() const;
	virtual AlSet*			asSetPtr();

	virtual const char*		name() const;
	virtual statusCode		setName( const char* );

	AlSet*			nextSet() const;
	AlSet*			prevSet() const;

	statusCode		nextSetD();
	statusCode		prevSetD();

	boolean			isEmpty() const;
	boolean			isExclusive() const;
	int				numberOfMembers() const;

	AlSetMember		*firstMember() const;
	statusCode		applyIteratorToMembers( AlIterator* iter, int& );

	//	Obsolete .. Moved to AlSettable, AlSetMember
	statusCode		addMember( const AlSettable * );
	statusCode		removeMember( AlSettable *);
	statusCode		removeMember( AlSetMember *);
	AlSetMember		*hasMember( const AlSettable* ) const;
	static statusCode	removeFromAllSets( AlSettable* );

private:
	static void 	initMessages();
	static void		finiMessages();
};

#endif // _AlSet
