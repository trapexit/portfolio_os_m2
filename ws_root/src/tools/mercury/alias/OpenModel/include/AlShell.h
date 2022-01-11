#ifndef _AlShell
#define _AlShell

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
//	.NAME AlShell - Interface to Alias nurbs surface geometry.
//
//	.SECTION Description
//		A shell is a collection of surfaces which behave as a single object.
//		Each surface can have its own set of trim curves.   A cube with six
//		square sides is a shell.
//		The cube can be assigned a single shader and rendering formation.
//		The cube is made of a collection of 6 spline surfaces. 
//
//		The AlTrimRegions are inaccurately named.
//		They actually represent the surfaces in the shell.
//		

#include <AlObject.h>
#include <AlTM.h>
#include <AlRenderInfo.h>

struct Spline_surface;
struct Dag_node;

class AlTrimRegion;

class AlShell : public AlObject
{
	friend class			AlFriend;
public:
							AlShell();
	virtual					~AlShell();
	virtual AlObject*		copyWrapper() const;

	virtual AlObjectType	type() const;
	virtual AlShell*		asShellPtr();
	AlShellNode*			shellNode() const;

	statusCode				create( AlTrimRegion*, boolean );
	statusCode 				addToShell( AlTrimRegion*, boolean );
	boolean					isInShell( AlTrimRegion* );
	AlTrimRegion*			firstTrimRegion() const;

	AlGroupNode*			unstitchShell( void );

	AlShader*				firstShader() const;
	AlShader*				nextShader( const AlShader* ) const;
	statusCode				nextShaderD( AlShader* ) const;

	statusCode				assignShader( AlShader* );
	statusCode				layerShader( AlShader* );

	statusCode				renderInfo( AlRenderInfo& ) const;
	statusCode				setRenderInfo( const AlRenderInfo& ) const;

	boolean 				isDisplayModeSet( AlDisplayModeType ) const;
	statusCode 				setDisplayMode( AlDisplayModeType, boolean );

	statusCode 				area( double &, boolean = TRUE, double tolerance=0.001 );
	statusCode 				circumference( double &, boolean = TRUE, double tolerance=0.001 );

protected:
private:
	static void				initMessages();
	static void				finiMessages();
};

#endif
