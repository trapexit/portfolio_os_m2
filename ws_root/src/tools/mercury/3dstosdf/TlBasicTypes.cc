#include "TlBasicTypes.h"
#include <ctype.h>

// For proper indentation of SDF output
static int indentLevel = 0;

// Convenient functions		
void 
WRITE_SDF( ostream& os, char *sdump )														
{	
	char	indent[ 200 ];
	
	indent[ 0 ] = '\0';																	
	for ( int ind = 0; ind < indentLevel; ind++ ) {
		strcat(indent,"\t");
	}
	strcat( indent, sdump );
	os << indent << '\n';
	
	PRINT_ERROR( os.fail(), "Unable to write SDF data to file\n" );
}

void 
BEGIN_SDF( ostream& os, char *sdump )										
{																	
	WRITE_SDF(os, sdump );						
	indentLevel += 1;	 												
}

void 
END_SDF( ostream& os, char *sdump )										
{																		
	indentLevel -= 1;													
	WRITE_SDF( os, sdump );						
}

// Memory class methods

MemPtr::MemPtr( size_t numBytes ) 
{
	if ( numBytes > 0 ) mPtr = malloc( numBytes );
	else mPtr = NULL;
}
	
MemPtr::~MemPtr() 
{
	if (mPtr != NULL ) free( mPtr );
}

const char 
*ChopString( char *in, char ch )
{
	static char out[ 80 ];
	int i = 0;
	
	while ( ( in[i] != '\0' ) && ( in[i] != ch ) )
	{
		out[ i ] = in[ i ];
		i++;
	}
	out[ i ] = '\0';
	return out;
}

const char 
*ChopStringLast( char *in, char ch )
{
	static char out[ 80 ];
	int i = strlen(in) - 1;
	
	strcpy( out, in );
	while ( ( out[i] != '\0' ) && ( out[i] != ch ) && ( i != 0 ) )
	{
		out[ i ] = in[ i ];
		i--;
	}
	
	if( i != 0 ) out[ i ] = '\0';
	
	return out;
}

const char 
*LowerCase( char *in )
{
	static char out[ 80 ];
	int i = 0;
	
	while ( in[i] != '\0' )
	{
		out[ i ] = tolower( in[ i ] );
		i++;
	}
	out[ i ] = '\0';
	return out;
}

