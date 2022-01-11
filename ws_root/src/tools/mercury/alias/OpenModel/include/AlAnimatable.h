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
//	.NAME AlAnimatable - Basic Interface to Alias objects which support
//						 animation.
//
//	.SECTION Description
//		This class encapsulates the functionality of Alias objects
//		which have the capacity to be animated.  It provides access
//		to channels.
//

#ifndef _AlAnimatable
#define _AlAnimatable

#include <AlObject.h>
#include <AlTM.h>
#include <AlIterator.h>

struct AI_Parm_ctrl;

class AlAnimatable
{
	friend			class AlFriend;

public:
	virtual AlChannel*		firstChannel() const;

	virtual AlChannel*		nextChannel( const AlChannel* ) const;
	virtual AlChannel*		prevChannel( const AlChannel* ) const;

	virtual statusCode		nextChannelD( AlChannel* ) const;
	virtual statusCode		prevChannelD( AlChannel* ) const;

	virtual statusCode		applyIteratorToChannels( AlIterator*, int &);

	virtual statusCode		deleteAnimation();

	statusCode				globalParam( const char *param, boolean& );
	AlList*					globalParamList();

	statusCode				localParam( const char *param, boolean& );
	AlList*					localParamList();

	statusCode				setLocalParam( const char *paramName, boolean state );
	statusCode				setGlobalParam( const char *paramName, boolean state );

protected:
							AlAnimatable();
	virtual 				~AlAnimatable();
	AlAnimatable*			animatablePtr();

	virtual boolean			extractType( int&, void *&, void *& ) const;

private:
	static AlList*			buildParamList( AI_Parm_ctrl* );
};

class AlParamItem : public AlLinkItem
{
	friend class	AlAnimatable;

public:
	AlParamItem*	nextItem() { return (AlParamItem *) next(); }

	char*			name;
	boolean			value;

protected:
					AlParamItem( const char*, boolean);
	virtual			~AlParamItem();
};

#endif // _AlAnimatable
