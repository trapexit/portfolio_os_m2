//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  rotected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//	.NAME AlIterator - A base class used to derive iterators for performing tasks on elements of a list.
//
//	.SECTION Description
//		Many classes return the first element of a list, which is then
//		traversed with some operation performed on each element of the
//		list. This class encapsulates this functionality making it very
//		easy to write code which performs operations on members of a list.
//
//		To use this class the method "func" should be overloaded. On
//		success func() should return zero which will cause the iteration
//		to continue with the next element in the list. A non-zero return
//		value will cause the iteration to stop. The returned value will
//		be returned through the second reference argument in the
//		applyIterator() method. In general the applyIterator() methods
//		return sSuccess even when func() returns non-zero. A return
//		other than sSuccess indicates that the applyIterator() method
//		failed to function properly.
//
//		Iterators should be used to examine or set data in the visited
//		objects, but should not be used to delete the objects.
//
//		For example:
//
//	.nf
// %@ class countIterator : public AlIterator {
// %@%@ public:
// %@%@%@ countIterator() : count( 0 );
// %@%@%@ ~countIterator() {};
// %@%@%@ virtual int func( AlObject* ) { count++; };
// %@%@%@ int result() { return count; };
// %@%@ private:
// %@%@%@ int count;
// %@ };
//
// code which creates a complete polyset ...
//
// %@ countIterator* iter = new countIterator;
// %@ status = pset->applyIteratorToVertices( iter, returnCode );
// %@ numVertices = iter->result();
//	.fi
//

#ifndef _AlIterator
#define _AlIterator

class AlObject;

class AlIterator {
public:
	virtual int func( AlObject* ) = 0;
};

#endif
