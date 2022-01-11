#include "TlMaterial.h"

TlMaterial::TlMaterial( const char *name )
{
#ifdef DEBUG
	Material_numObjs++;
#endif
	m = new MaterialEntry;
	PRINT_ERROR( (m==NULL), "Out of memory" );
	m->refID = -1;
	m->matName = new char[ strlen( name ) + 1 ];
	PRINT_ERROR( (m->matName==NULL), "Out of memory" );
	strcpy( m->matName, name );
		
	m->ambient = NULL;
	m->diffuse = NULL;
	m->specular = NULL;
	m->emission = NULL;
	m->shininess = -1.0;	// use default
	m->shineStrength = -1.0;	// use default
	m->transparency = -1.0;	// use default
	m->shading = 2;       // default is smooth shading
	m->twoSided = FALSE;
	nextMaterial = NULL;
}

TlMaterial::TlMaterial( const TlMaterial& mat )
{
#ifdef DEBUG
	Material_numObjs++;
#endif
	mat.m->refCount++;
	m = mat.m;
	nextMaterial = NULL;
}

TlMaterial::~TlMaterial()
{
#ifdef DEBUG
	Material_numObjs--;
#endif
	if ( --m->refCount == 0 ) 
	{
		delete[] m->matName;
		if ( m->ambient != NULL ) delete m->ambient;
		if ( m->diffuse != NULL ) delete m->diffuse;
		if ( m->specular != NULL ) delete m->specular;
		if ( m->emission != NULL ) delete m->emission;
		delete m;
	}
}

void 
TlMaterial::SetNextMaterial( TlMaterial *mat )
{
	if ( this && mat ) nextMaterial = mat;
}

// Data setting functions

// Set the material name 
void 
TlMaterial::SetName( char *name )
{
	if ( m->matName == NULL ) {
		m->matName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (m->matName==NULL), "Out of memory" );
		strcpy( m->matName, name );
	} else if ( strlen( m->matName ) < strlen( name ) ) {
		delete[] m->matName;
		m->matName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (m->matName==NULL), "Out of memory" );
		strcpy( m->matName, name );
	} else strcpy( m->matName, name );
}	


void 
TlMaterial::SetAmbient( const TlColor& clr )
{	
	if ( m->ambient == NULL ) 
	{
		m->ambient = new TlColor;
		PRINT_ERROR( (m->ambient==NULL), "Out of memory" );
	}
	
	if ( (clr.r == 0.0) && 
	     (clr.g == 0.0) &&
	     (clr.g == 0.0) ) {
	     	m->ambient->r = 1.0;
	     	m->ambient->g = 1.0;
	     	m->ambient->b = 1.0;
	} else *m->ambient = clr;
}

void 
TlMaterial::SetDiffuse( const TlColor& clr )
{	
	if ( m->diffuse == NULL ) 
	{
		m->diffuse = new TlColor;
		PRINT_ERROR( (m->diffuse==NULL), "Out of memory" );
	}
	
	if ( (clr.r == 0.0) && 
	     (clr.g == 0.0) &&
	     (clr.g == 0.0) ) {
	     	m->diffuse->r = 1.0;
	     	m->diffuse->g = 1.0;
	     	m->diffuse->b = 1.0;
	} else *m->diffuse = clr;
}

void 
TlMaterial::SetSpecular( const TlColor& clr )
{	
	if ( m->specular == NULL )
	{
		 m->specular = new TlColor;
		 PRINT_ERROR( (m->specular==NULL), "Out of memory" );
	}
	*m->specular = clr;
}

void
TlMaterial::SetEmission( const TlColor& clr )
{	
	if ( m->emission == NULL ) 
	{
		m->emission = new TlColor;
		PRINT_ERROR( (m->emission==NULL), "Out of memory" );
	}
	*m->emission = clr;
}

void
TlMaterial::SetShininess( float factor )
{	
	m->shininess = factor;
} 


void
TlMaterial::SetShineStrength( float factor )
{	
	m->shineStrength = factor;
} 

void 
TlMaterial::SetTransparency( float factor )
{	
	m->transparency = factor;
}

void 
TlMaterial::SetShading( int typ )
{	
	m->shading = typ;
}

void 
TlMaterial::SetTwoSided( Boolean flag )
{	
	m->twoSided = flag;
}
  	
// Data getting fuctions
Boolean 
TlMaterial::HasNullData() const
{
	if ( m->ambient != NULL ||
	     m->diffuse != NULL ||
	     m->specular != NULL ||
	     m->emission != NULL ||
	     m->shininess >= 0.0 ||
	     m->transparency >= 0.0 ) return FALSE;
	else return TRUE;
}

TlMaterial& 
TlMaterial::operator=( const TlMaterial& mat )
{
	mat.m->refCount++;
	if ( --m->refCount == 0 ) 
	{
		delete[] m->matName;
		if ( m->ambient != NULL ) delete m->ambient;
		if ( m->diffuse != NULL ) delete m->diffuse;
		if ( m->specular != NULL ) delete m->specular;
		if ( m->emission != NULL ) delete m->emission;
		delete m;
	}
	m = mat.m;
	return *this;
}

Boolean 
TlMaterial::operator==(const TlMaterial& mat ) const
{
	if ( m == mat.m ) return TRUE;
	else return FALSE;
}

ostream& 
TlMaterial::WriteSDF( ostream& os )
{
		char buf[100];
		float alpha = 1.0;
		
		sprintf( buf, "Use Material \"Mat_%s\"", GetName() );
		WRITE_SDF( os, buf );
		
		return os;
}

ostream& 
TlMaterial::WriteSDF1( ostream& os )
{
		char buf[100];
		char senbuff[100], senbuff1[100];
		int senable = 0;
		float alpha = 1.0;
		
		sprintf( buf, "# Material Mat_%s", GetName() );
		
		WRITE_SDF( os, buf );
		BEGIN_SDF( os, "{" );
		
		sprintf( senbuff, "shadeenable( " );
		
		if ( m->ambient != NULL ) {
			sprintf( buf, "ambient { %0.5f %0.5f %0.5f %0.5f }",
	  	    m->ambient->r, m->ambient->g, m->ambient->b, alpha);
			WRITE_SDF( os, buf );
			senable = 1;
			sprintf( senbuff1, "ambient" );
			strcat( senbuff,  senbuff1 );
		} 
		if ( m->diffuse != NULL ) {
			sprintf( buf, "diffuse { %0.5f %0.5f %0.5f %0.5f }",
	    	m->diffuse->r, m->diffuse->g, m->diffuse->b, (1.0 - m->transparency) );
			WRITE_SDF( os, buf );
			
			if( senable ) sprintf( senbuff1, " | diffuse" );
			else sprintf( senbuff1, "diffuse" );
			senable = 1;
			strcat( senbuff, senbuff1 );
		} 
		if (( m->specular != NULL ) && (m->shineStrength>0.0)) {
		  sprintf( buf, "specular { %0.5f %0.5f %0.5f %0.5f }",
			   m->specular->r*m->shineStrength,
			   m->specular->g*m->shineStrength, 
			   m->specular->b*m->shineStrength,
			   alpha );
		  WRITE_SDF( os, buf );
		  
		  if( senable ) sprintf( senbuff1, " | specular" );
		  else sprintf( senbuff1, "specular" );
		  senable = 1;
		  strcat( senbuff, senbuff1 );
		}
		if ( m->emission != NULL ) {
			sprintf( buf, "emission { %0.5f %0.5f %0.5f %0.5f }",
	 	    m->emission->r, m->emission->g, m->emission->b, alpha );
			WRITE_SDF( os, buf );
			
			if( senable ) sprintf( senbuff1, " | emission" );
			else sprintf( senbuff1, "emission" );
			senable = 1;
			strcat( senbuff, senbuff1 );
		} 
		if (( m->shininess > 0.0 ) && (m->shineStrength>0.0)) {
			sprintf( buf, "shine  %0.5f ", m->shininess );
			WRITE_SDF( os, buf );
		} 	
		
		if( senable ) 
		{
			sprintf( senbuff1, " )" );
			strcat( senbuff, senbuff1 );
			WRITE_SDF( os, senbuff );
		}
		
		END_SDF( os, "}" );
	return os;
}

int
TlMaterial::SetRefID( int id )
{
	// clear the ID
	if ( id < 0 ) m->refID = id;
	// mark with unique ID
	else if ( m->refID < 0 ) m->refID = id;
	return m->refID;
}
