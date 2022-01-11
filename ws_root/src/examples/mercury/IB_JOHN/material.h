/*
 *
 *
 */
#ifndef _material_h_
#define _material_h_

#include "mercury.h"
#include "lighting.h"
#include "misc.h"

/*
 *
 */
#define mMaterial_SetBaseColor( s, r, g, b ) \
	mColor4_SetColor( &((s)->base), (r), (g), (b) )

#define mMaterial_SetAlpha( s, a ) \
	mColor4_SetAlpha( &((s)->base), (a) )

#define mMaterial_SetDiffuseColor( s, r, g, b ) \
	mColor3_Set( &((s)->diffuse), (r), (g), (b) )

#define mMaterial_SetSpecularColor( s, r, g, b ) \
	mColor3_Set( &((s)->specular), (r), (g), (b) )

#define mMaterial_SetShine( s, v ) \
	(s)->shine = (v)

/*
 *
 */
Material* Material_Construct( void );
void Material_Destruct( Material *self );
void Material_Print( Material *self );

#include "misc.h"

#endif
/* End of File */
