#include <math.h>
#include "TlLight.h"
#ifdef SPLIT_TG
#include "TlGeomGroup.h"
#endif

#ifndef SPLIT_TG
// get the global texture & material array name
extern char *GetFileName3DS();
#endif

static void ComputeLightTransform( Point3D src, Point3D dst, TlTransform *t )
{
	float	U[3], V[3], N[3];
	Point3D up;
	float	dot;
	float len;

	up.x = 0.0; up.y = 1.0; up.z = 0.0;
	/*
	 * Compute vector  N = dst - src  and normalize  N
	 */
	N[0] = dst.x - src.x; N[1] = dst.y - src.y; N[2] = dst.z - src.z;
	/* normalize the vector */
	len = (float)sqrt( (double) (N[0] * N[0] + N[1] * N[1] + N[2] * N[2]) );
	if (len != 0.0) { N[0] /= len;  N[1] /= len; N[2] /= len; } 

	/*
	 * Compute vector  V = UP - VRP
	 * Make vector  V  orthogonal to  N  and normalize  V
	 */
l1:
	V[0] = up.x - dst.x; V[1] = up.y - dst.y; V[2] = up.z - dst.z;
	dot = V[0] * N[0] + V[1] * N[1] + V[2] * N[2];
	if( AEQUAL( dot, 0.0 ) ) 
	{
		up.x = 1.0; up.y = 0.0; up.z = 0.0;
		goto l1;
	}
	V[0] -= dot * N[0]; V[1] -= dot * N[1]; V[2] -= dot * N[2];
	
	/* normalize the vector */
	len = (float)sqrt( (double)( V[0] * V[0] + V[1] * V[1] + V[2] * V[2]) );
	if (len != 0.0) { V[0] /= len;  V[1] /= len; V[2] /= len; }


	/*
	 * Compute vector  U = V x N  (cross product)
	 */
	U[0] = (V[1]*N[2]) - (V[2]*N[1]);
	U[1] = (V[2]*N[0]) - (V[0]*N[2]);
	U[2] = (V[0]*N[1]) - (V[1]*N[0]);
	t->SetVal( 0, 0, U[0] ); t->SetVal( 0, 1, U[1] ); t->SetVal( 0, 2, U[2] );
	t->SetVal( 1, 0, V[0] ); t->SetVal( 1, 1, V[1] ); t->SetVal( 1, 2, V[2] );
	t->SetVal( 2, 0, N[0] ); t->SetVal( 2, 1, N[1] ); t->SetVal( 2, 2, N[2] );
}

TlLight::TlLight( const char *name ) : TlCharacter( name )
{
#ifdef DEBUG
	Light_numObjs++;
#endif
	m = new LightEntry;
	PRINT_ERROR( (m==NULL), "Out of memory" );
	m->refID = -1;
	m->lgtName = new char[ strlen( name ) + 1 ];
	PRINT_ERROR( (m->lgtName==NULL), "Out of memory" );
	strcpy( m->lgtName, name );	
	m->spotLght = NULL;
	m->lgtOff = FALSE;
}

TlLight::TlLight( const TlLight& lgt )
{
#ifdef DEBUG
	Light_numObjs++;
#endif
	// Set the instance name
	char buf[100];
	
	sprintf( buf, "%s.%d", lgt.m->lgtName, lgt.m->refCount );
	this->SetCharacterName( buf );
	
	// Share the light description
	lgt.m->refCount++;
	m = lgt.m; 
	
	// Initialise the transform
	this->transf = lgt.transf;
}

TlLight::~TlLight()
{
#ifdef DEBUG
	Light_numObjs--;
#endif
	if ( --m->refCount == 0 ) 
	{
		delete[] m->lgtName;
		
		if( m->spotLght ) delete m->spotLght;
		delete m;
	}		
}

TlCharacter*
TlLight::Instance()
{	
	TlCharacter *tmp;
	
	tmp = (TlCharacter*)new TlLight( *this );
	PRINT_ERROR( (tmp==NULL), "Out of memory" );
	
	return ( tmp );
}

int
TlLight::SetRefID( int id )
{
	// clear the ID
	if ( id < 0 ) m->refID = id;
	// mark with unique ID
	else if ( m->refID < 0 ) m->refID = id;
	return m->refID;
}

ostream& 
TlLight::WriteSDF1( ostream& os )
{
	char buf[100];
	
	
	// if not defined before do it now
	if ( m->refID < 0 ) 
	{		
		m->refID = 1; // flag it as defined
		sprintf( buf, "Define Light \"%s\" {", m->lgtName );
		BEGIN_SDF( os, buf );
		
		// light kind
		if( m->spotLght == NULL )
		{ 
			sprintf( buf, "kind point" );
			WRITE_SDF( os, buf );
		} else {
			if( m->spotLght )
			{
				// angle
				sprintf( buf, "angle %0.6g", m->spotLght->coneAngle   );		
				WRITE_SDF( os, buf );
				
				if ( AEQUAL ( (m->spotLght->coneAngle+0.5), m->spotLght->fallOff ) )
				{
					sprintf( buf, "kind spot" );
					WRITE_SDF( os, buf );
				} else {
					sprintf( buf, "kind softspot" );
					WRITE_SDF( os, buf );

					// falloff
					sprintf( buf, "falloff %0.6g", 
						1.0 - ( m->spotLght->coneAngle/ m->spotLght->fallOff )   );		
					WRITE_SDF( os, buf );
				}
			}
		}
		
		if( m->lgtOff )
		{
			sprintf( buf, "enabled false"  );		
			WRITE_SDF( os, buf );
		}
	
		// light color
		sprintf( buf, "color { %0.5f %0.5f %0.5f }", 
				m->lgtColor.r, m->lgtColor.g, m->lgtColor.b );		
		WRITE_SDF( os, buf );
		
		END_SDF( os, "}" );
	}

	return os;
}

ostream& 
TlLight::WriteSDF( ostream& os )
{
	char buf[100];
	float ang = 0.0;

	/* calculate the transform to reflect the light direction */
	if( m->spotLght )
	{	
		TlTransform tr;
		
		ComputeLightTransform( m->pos, m->spotLght->target, &tr ); 
		transf = tr;
	}
	
	// Already defined use it !, write character as a group
	if ( m->refID >= 0 ) 
	{
		sprintf( buf, "Define Group \"Chr_%s\" {", GetName() );
		BEGIN_SDF( os, buf );	
		
		// Write the transform if it is not "unit" transform
		if ( transf.UnitTransform() == FALSE ) transf.WriteSDF( os );
				
		sprintf( buf, "Use Light \"%s\"", m->lgtName );
		WRITE_SDF( os, buf );
		END_SDF( os, "}" );
	}
	
	return os;
}
