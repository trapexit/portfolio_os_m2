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
//  .NAME AlContact
//
//  .SECTION Description
//

#ifndef _AlContact
#define _AlContact

#include <AlStyle.h>
#include <AlObject.h>

class AlCommandRef : public AlObject
{
	friend class			AlFriend;
	friend class			AlCommandFriend;
public:
							AlCommandRef();
	virtual					~AlCommandRef();
	virtual AlObjectType	type() const;
	AlCommandRef*			asCommandRefPtr();

	AlCommandRef*			nextRef();
	AlCommandRef*			prevRef();
	statusCode				nextRefD();
	statusCode				prevRefD();

	AlDagNode*				dagNode();
	AlCurveOnSurface*		curveOnSurface();
	AlContact*				contact();

private:
    static void             initMessages();
    static void             finiMessages();
};

enum AlContactType	{	kContactInvalid, kContactIsoparamU, kContactIsoparamV,
						kContactCurveOnSurface, kContactTrimEdge,
						kContactFreeCurve };

class AlContact : public AlObject
{
	friend class			AlFriend;
	friend class			AlCommandFriend;

public:
							AlContact();
	virtual					~AlContact();

	statusCode				deleteObject();
	virtual AlObjectType	type() const;
	AlContact*				asContactPtr();

	statusCode				create();

	statusCode				appendContact( AlContact * );
	int						numberContacts() const;
	AlContact*				nextContact();
	AlContact*				prevContact();
	statusCode				nextContactD();
	statusCode				prevContactD();

	AlContactType			contactType() const;

	// These are different values common to the various contact types
	// derived classes are not used since this would result in about 6
	// new classes, each with a single method.
	// These could be added later
	//
	statusCode			calculate( AlDagNode *dagNode, double tolerance = 0, boolean adjustTolerance = FALSE );
	boolean				areEqual( AlDagNode *dagNodeThis, AlContact *contactOther, AlDagNode *dagNodeOther) const;


	AlTrimCurve*			trimCurve();
	AlCurveOnSurface*		curveOnSurface();
	int						freeCurveIndex();
	double					curveOnSurfaceParam() const;
	double					paramValue() const;
	double					nonisoparamMin() const;
	double					nonisoparamMax() const;

	// currently unimplemented - return NULL
	AlCurve*				curve();
	AlCurve*				curveUV();

private:
    static void             initMessages();
    static void             finiMessages();
};

#endif	// _AlContact

