/*
 *
 *
 */
#include "lights.h"



Lights* Lights_Construct(void)
{
	Lights*		self;

	return self;
}

/*
 *
 */
void Lights_Print_old(Lights *self)
{
	uint32		*lights;

	lights = (uint32*)self;

	while ( *lights != 0 ) {
		printf( "%x\n", *lights );
		lights++;
	}

}

/*
 *
 */
void Lights_Print(Lights *self)
{
	uint32		nLights;

	printf( "Lights:\n" );
	nLights = 0;

	while ( self->lightProcess != 0 ) {

		if ( self->lightProcess == (uint32*)M_LightFog ) {
				printf( "M_LightFog ...\n" );
		}
		if ( self->lightProcess == (uint32*)M_LightDir ) {

				LightDir	*light = (LightDir*)self->lightData;

				printf( "M_LightDir ...\n" );
				Vector3D_Print( (Vector3D*)(&light->nx) );
				Color3_Print( &(light->lightcolor) );
		}
		if ( self->lightProcess == (uint32*)M_LightPoint ) {
				printf( "M_LightPoint ...\n" );
		}
		if ( self->lightProcess == (uint32*)M_LightSoftSpot ) {
				printf( "M_LightSoftSpot ...\n" );
		}
		if ( self->lightProcess == (uint32*)M_LightFogTrans ) {
				printf( "M_LightFogTrans ...\n" );
		}
		if ( self->lightProcess == (uint32*)M_LightDirSpec ) {
				printf( "M_LightDirSpec ...\n" );
		}
		self++;
		nLights++;
	}
	printf( "Number of lights = %i\n", nLights );
}

/* End of File */
