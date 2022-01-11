#include "TlCharacter.h"

TlCharacter::TlCharacter()
{
	charName = NULL;
	nextCharacter = NULL;
	parent = NULL;
}

TlCharacter::TlCharacter( const char *name )
{
	charName = new char[ strlen( name ) + 1 ];
	PRINT_ERROR( (charName==NULL), "Out of memory" );
	strcpy( charName, name );
	nextCharacter = NULL;
	parent = NULL;
}

TlCharacter::~TlCharacter()
{
	delete[] charName;	
}

TlCharacter* 
TlCharacter::Instance()
{ 
	TlCharacter *tmp;
	
	tmp = new TlCharacter(); 
	PRINT_ERROR( (tmp==NULL), "Out of memory" );
	
	return tmp;
}

TlCharacter*
TlCharacter::GetChildren()
{ 
	return (TlCharacter *)NULL; 
}	
				
void 
TlCharacter::SetCharacterName( const char* name )
{	
	if ( charName == NULL ) {
		charName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (charName==NULL), "Out of memory" );
		strcpy( charName, name );
	} else if ( strlen( charName ) < strlen( name ) ) {
		delete[] charName;
		charName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (charName==NULL), "Out of memory" );
		strcpy( charName, name );
	} else strcpy( charName, name );
}

