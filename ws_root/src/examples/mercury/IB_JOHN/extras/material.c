/*
 *
 *
 */
#include "material.h"

/*
 *
 */
Material* Material_Construct()
{
	Material	*self;

	self = (Material*)AllocMem( sizeof(Material), MEMTYPE_ANY );
	assert( self );

	mMaterial_SetAlpha( self, 0.0f );
	mMaterial_SetBaseColor( self, 0.1f, 0.1f, 0.1f );
	mMaterial_SetDiffuseColor( self, 0.5f, 0.5f, 0.5f );
	mMaterial_SetSpecularColor( self, 0.75f, 0.75f, 0.75f );
	mMaterial_SetShine( self, 0.4f );

	return self;
}

/*
 *
 */
void Material_Destruct( Material *self )
{
	assert( self );
	free( self );
}

/*
 *
 */
void Material_Print( Material *self)
{
	printf( "Material:\n" );
	printf( "base       ->\n" );
	printf( "\n" );
	Color4_Print( &self->base );
	printf( "\n" );
	printf( "diffuse    ->\n" );
	printf( "\n" );
	Color3_Print( &self->diffuse );
	printf( "\n" );
	printf( "shine      %3.3f\n", self->shine );
	printf( "specular   ->\n" );
	printf( "\n" );
	Color3_Print( &self->specular );
	printf( "\n" );
}


/* End of File */
