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
//	.NAME AlShader - Base object for representing shader data
//
//	.SECTION Description
//		This class encapsulates the basic functionality for checking and
//		setting the name of a shader as well as accessing the textures that
//		a particular shader refers to, and the animation on the shader.
//		Shader objects are accessible through both the AlUniverse class and
//		the objects that reference them (AlSurface and AlFace classes).
//
//		A shader object may reference textures.  The firstTexture() and
//		nextTexture() methods are used to access these textures.
//
//		firstTexture() returns the first texture that the shader object
//		references.  nextTexture() moves from a given referenced texture
//		to the next texture in order, as related to the shader object.
//		(See the similar methods for the AlTexture/AlEnvironment classes.)
//
//		The animation on a shader can be accessed through the
//		firstChannel() and nextChannel() methods.  All the channels on the
//		shader can be deleted by calling deleteAnimation().
//
//		The shader parameters can be accessed through the parameter() and
//		setParameter() methods.  Each shader has a specific set of parameters
//		that are valid for it that depend on its type.  The full list of
//		shader parameters can be seen in the file AlAnim.h.  For example,
//		all parameters specific to the Blinn shader have names of the form
//		kFLD_SHADING_BLINN_*.  Parameters common to all shaders have the form
//		kFLD_SHADING_COMMON_*.  All parameters are treated as doubles even
//		though this may not necessarily be what they are.  This is done to
//		make the interface as simple and consistent as possible.
//

#ifndef _AlShader
#define _AlShader

#include <AlObject.h>
#include <AlAnimatable.h>
#include <AlAnim.h>
#include <AlList.h>
#include <AlIterator.h>

class AlShader : public AlObject
			   , public AlAnimatable {
	friend class			AlFriend;
public:
							AlShader();
	virtual					~AlShader();
	virtual statusCode		deleteObject();
	virtual AlObject		*copyWrapper() const;

	statusCode				create();

	virtual AlAnimatable*	asAnimatablePtr();

	virtual AlObjectType	type() const;
	virtual AlShader*		asShaderPtr();

	virtual const char*		name() const;
	virtual statusCode		setName( const char * );

	statusCode				parameter( const AlShadingFields, double* ) const;
	statusCode				parameter( const AlShadingFields, double& ) const;
	statusCode				setParameter( const AlShadingFields, const double );

	const char*				shadingModel() const;
	statusCode				setShadingModel( const char* );

	AlTexture*				firstTexture() const;

	AlTexture*				nextTexture( const AlTexture* ) const;
	statusCode				nextTextureD( AlTexture* ) const;

	AlList*					fields() const;
	AlList*					mappedFields() const;

	statusCode				addTexture( const char*, const char*, AlTexture** returnedTexture = NULL );
	statusCode				removeTexture( const char* );

	statusCode				applyIteratorToTextures( AlIterator*, int& );

private:
	static void				initMessages();
	static void				finiMessages();

	boolean					extractType( int&, void *&, void *& ) const;
};

#endif // _AlShader
