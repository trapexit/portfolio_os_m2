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
//	.NAME AlTexture - Base object for representing texture data
//
//	.SECTION Description
//		This class encapsulates the basic functionality for checking and
//		setting the name of a texture as well as accessing the textures that
//		this texture refers to, and the animation on this texture.
//		These texture objects can be obtained from the AlShader class and the
//		AlEnvironment class.
//
//		A texture object itself may also reference other textures.  For
//		this reason the firstTexture() and nextTexture() methods are used.
//
//		firstTexture() returns the first texture that the texture object
//		references.  nextTexture() moves from a given referenced texture
//		to the next texture in order, as related to the texture object.
//		(See the similar methods for the AlShader/AlEnvironment classes.)
//
//		The animation on a texture can be accessed through the
//		firstChannel() and nextChannel() methods.  All the channels on the
//		texture can be deleted by calling deleteAnimation().
//
//		The texture parameters can be accessed through the parameter() and
//		setParameter() methods.  Each texture has a specific set of parameters
//		that are valid for it that depend on its type.  The full list of
//		texture parameters can be seen in the file AlShading.h.  For example,
//		all parameters specific to the water texture have names of the form
//		kFLD_SHADING_WATER_*.  Parameters common to all textures have the form
//		kFLD_SHADING_COMMON_TEXTURE_*.  All parameters are treated as doubles
//		even though this may not necessarily be what they are.  This was done
//		to make the interface as simple and consistent as possible.
//
//		The user can neither create nor destroy an AlTexture class object
//		at this time.
//

#ifndef _AlTexture
#define _AlTexture

#include <AlIterator.h>
#include <AlAnimatable.h>
#include <AlObject.h>
#include <AlShader.h>

struct UI_Widget;
struct IR_ShaderEntry;

class AlTexture : public AlObject,
				public AlAnimatable {
	friend class			AlFriend;

public:

	virtual					~AlTexture();
	virtual statusCode		deleteObject();
	virtual AlObject		*copyWrapper() const;

	virtual AlAnimatable*	asAnimatablePtr();

	virtual AlObjectType	type() const;
	virtual AlTexture*		asTexturePtr();
	virtual const char*		name() const;
	virtual statusCode		setName(const char *);

	AlTexture*				firstTexture() const;
	AlTexture*				nextTexture( const AlTexture* ) const;
	statusCode				nextTextureD( AlTexture* );

	const char*				textureType() const;
	const char*				fieldType() const;
	statusCode				parameter( const AlShadingFields, double* ) const;
	statusCode				parameter( const AlShadingFields, double& ) const;

	statusCode				setParameter( const AlShadingFields, const double );

	/* this section is only for file textures */
	const char*				filename() const;
	statusCode				setFilename( const char* );
	const char*				firstPerObjectPixEntry();
	const char*				nextPerObjectPixEntry( const char *);
	const char*				getPerObjectPixFilename( const char*) const;
	statusCode				setPerObjectPixFilename( const char*, const char*);
	statusCode				addPerObjectPixEntry( const char*, const char*);
	statusCode				removePerObjectPixEntry( const char* );
	
	boolean					isParametric( void ) const;
    statusCode				eval( double, double, double, double, double, double, double, double, double, double, double*, double*, double*, double*, boolean = TRUE, double = 1.0, double = 0.0, char *objectname = NULL ) const;

	AlList*					fields() const;
	AlList*					mappedFields() const;

	statusCode				addTexture( const char*, const char*, AlTexture** returnedTexture = NULL );
	statusCode				removeTexture( const char* );

	AlTextureNode*			textureNode() const;

	statusCode				applyIteratorToTextures( AlIterator*, int& );

protected:
							AlTexture();
	static statusCode		deleteObject(IR_ShaderEntry*);

private:
	boolean					extractType( int&, void*&, void *& ) const;
};

#endif // _AlTexture
