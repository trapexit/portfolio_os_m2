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
//	.NAME AlEnvironment - Base object for representing shader environment data
//
//	.SECTION Description
//		This class encapsulates the basic functionality for checking and
//		setting the name of an environment.  It also encapsulates accessing the
//		textures that a particular environment refers to, and the animation
//		on the environment.  When the wire file is read, the environment
//		contained therein are created as an AlEnvironment class object.
//		This environment object is accessible through the AlUniverse class.
//
//		An environment object may reference textures.  The firstTexture and
//		nextTexture methods are used to access these textures.
//
//		firstTexture() returns the first texture that the environment object
//		references.  nextTexture() moves from a given referenced texture
//		to the next texture in order, as related to the environment object.
//		(See the similar methods for the AlTexture/AlShader classes.)
//
//		The animation on the environment can be accessed through the
//		firstChannel() and nextChannel() methods.  All the channels on the
//		environment can be deleted by calling deleteAnimation().
//
//		The environment parameters can be accessed through the parameter() and
//		setParameter() methods.  Each shader has a specific set of parameters
//		that are valid for it that depend on its type.  The full list of
//		environment parameters can be seen in the file AlAnim.h.  For example,
//		all parameters specific to the Blinn shader have names of the form
//		kFLD_SHADING_BLINN_*.  Parameters common to all shaders have the form
//		kFLD_SHADING_COMMON_*.  All parameters are treated as doubles even
//		though this may not necessarily be what they are.  This is done to
//		make the interface as simple and consistent as possible.
//
//		The user can neither create nor destroy an AlEnvironment class object
//		at this time.
//

#ifndef _AlEnvironment
#define _AlEnvironment

#include <AlObject.h>
#include <AlShader.h>
#include <AlAnimatable.h>

class AlEnvironment : public AlObject,
	public AlAnimatable {
	friend class			AlFriend;

public:
	virtual					~AlEnvironment();
	virtual AlAnimatable*	asAnimatablePtr();

	virtual AlObjectType	type() const;
	virtual AlEnvironment*	asEnvironmentPtr();
	virtual const char*		name() const;
	virtual statusCode		deleteObject();
    virtual AlObject*		copyWrapper() const;

	AlTexture*				firstTexture() const;
	AlTexture*				nextTexture( AlTexture* ) const;
	statusCode				nextTextureD( AlTexture* ) const;

	statusCode				parameter( const AlShadingFields, double* ) const;

	statusCode				parameter( const AlShadingFields, double& ) const;
	statusCode				setParameter( const AlShadingFields, const double );

	AlList*					fields() const;
	AlList*					mappedFields() const;

	statusCode				addTexture( const char*, const char*, AlTexture** = NULL );
	statusCode				removeTexture( const char* );
	statusCode				applyIteratorToTextures( AlIterator *, int& );

protected:
							AlEnvironment();
	statusCode				create(const struct IR_ShaderEntry* );

	boolean					extractType( int&, void*&, void*& ) const;
};

#endif // _AlEnvironment
