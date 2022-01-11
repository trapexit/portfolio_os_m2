#include "TlModel.h"
#ifdef SPLIT_TG
#include "TlGeomGroup.h"
#endif

#ifndef SPLIT_TG
// get the global texture & material array name
extern char *GetFileName3DS();
#endif

TlModel::TlModel( const char *name ) : TlCharacter( name )
{
#ifdef DEBUG
	Model_numObjs++;
#endif
	m = new ModelEntry;
	PRINT_ERROR( (m==NULL), "Out of memory" );
	m->refID = -1;
	m->mdlName = new char[ strlen( name ) + 1 ];
	PRINT_ERROR( (m->mdlName==NULL), "Out of memory" );
	strcpy( m->mdlName, name );	
	m->surface = NULL;
}

TlModel::TlModel( const TlModel& mdl )
{
#ifdef DEBUG
	Model_numObjs++;
#endif
	// Set the instance name
	char buf[100];
	
	sprintf( buf, "%s.%d", mdl.m->mdlName, mdl.m->refCount );
	this->SetCharacterName( buf );
	
	// Share the model description
	mdl.m->refCount++;
	m = mdl.m; 
	
	// Initialise the transform
	this->transf = mdl.transf;
}

TlModel::~TlModel()
{
#ifdef DEBUG
	Model_numObjs--;
#endif
	if ( --m->refCount == 0 ) 
	{
		delete[] m->mdlName;
		
		TlSurface *cur = m->surface;
		TlSurface *next;

		while ( cur )
		{
 			next = cur->GetNextSurface();
			delete cur;
			cur = next;
		}
		delete m;
	}		
}

TlCharacter*
TlModel::Instance()
{	
	TlCharacter *tmp;
	
	tmp = (TlCharacter*)new TlModel( *this );
	PRINT_ERROR( (tmp==NULL), "Out of memory" );
	
	return ( tmp );
}

void 
TlModel::AppendSurface( TlSurface *surf )
{
	if ( m->surface ) 
	{
		TlSurface *cur = m->surface;

		while ( cur->GetNextSurface() )
		{
 			cur = cur->GetNextSurface();
		}
		cur->SetNextSurface( surf );
	} else m->surface = surf;
}

int
TlModel::SetRefID( int id )
{
	// clear the ID
	if ( id < 0 ) m->refID = id;
	// mark with unique ID
	else if ( m->refID < 0 ) m->refID = id;
	return m->refID;
}

ostream& 
TlModel::WriteSDF1( ostream& os )
{
	char buf[100];
	
	
	if ( m->surface )
	{
		// if not defined before do it now
		if ( m->refID < 0 ) 
		{
			TlSurface *cur;
			TlMaterial	*mat;
			TlTexture	*tex;
			Boolean texs_exist = FALSE;
			Boolean mats_exist = FALSE;
			
			m->refID = 1; // flag it as defined
			sprintf( buf, "Define Model \"%s\" {", m->mdlName );
			BEGIN_SDF( os, buf );

			// See if textures exist, material must exist for each surface
			cur = m->surface;
			while ( cur )
			{
				tex = cur->GetTexture();
				mat = cur->GetMaterial();
				if ( ( texs_exist == FALSE ) && tex && ( tex->HasNullData() == FALSE ) )
					texs_exist = TRUE;
				if ( ( mats_exist == FALSE ) && mat && ( mat->HasNullData() == FALSE ) )
					mats_exist = TRUE;
				if ( ( texs_exist == TRUE ) && ( mats_exist == TRUE ) ) break;
				
				cur = cur->GetNextSurface();
			}

			// Write  materials array
			if( mats_exist == TRUE )
			{ 
#ifdef SPLIT_TG
				sprintf( buf, "Use MatArray materials" );
#else
				sprintf( buf, "Use MatArray \"%s_materials\"", GetFileName3DS() );
#endif
				WRITE_SDF( os, buf );
			}
			// Write textures array
			if( texs_exist == TRUE ) 
			{
#ifdef SPLIT_TG
				sprintf( buf, "Use TexArray textures" );
#else
				sprintf( buf, "Use TexArray \"%s_textures\"", GetFileName3DS() );
#endif
				WRITE_SDF( os, buf );
			}
			
			sprintf( buf, "Surface {" );
			BEGIN_SDF( os, buf );
		
			// Write surface elements
			cur = m->surface;
			while ( cur )
			{
				cur->WriteSDF( os );
				cur = cur->GetNextSurface();
			}
				
			END_SDF( os, "}" );
			
			END_SDF( os, "}" );
		}
	}
	return os;
}

ostream& 
TlModel::WriteSDF( ostream& os )
{
	char buf[100];
	Boolean unit_transf = transf.UnitTransform();
	
	if ( m->surface )
	{
		// Already defined use it !, write character as a group
		// if the model has a non - unit matrix
		if ( m->refID >= 0 ) 
		{
			if( unit_transf == FALSE )
			{
				sprintf( buf, "Define Group \"Chr_%s\" {", GetName() );
				BEGIN_SDF( os, buf );
				transf.WriteSDF( os );
			}	
			
			sprintf( buf, "Use Model \"%s\"", m->mdlName );
			WRITE_SDF( os, buf );

			if( unit_transf == FALSE )
				END_SDF( os, "}" );
		}
	}	
	return os;
}
