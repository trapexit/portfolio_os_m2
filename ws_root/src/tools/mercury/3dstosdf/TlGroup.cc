/*
**	File:		TlGroup.cp
**
**	Contains:	 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.
**
**	Change History (most recent first):
**
**		<1+>	 1/26/95	RRR		Common boolean definition
**
**	To Do:
*/

#include "TlGroup.h"

TlGroup::TlGroup( const char *name ) : TlCharacter( name )
{
	firstChild = NULL;
	lastChild = NULL;
	numChildren = 0;
}

TlGroup::~TlGroup()
{
	TlCharacter *cur = firstChild;
	TlCharacter *next;

	while ( cur )
	{
 		next = cur->GetNextCharacter();
		delete cur;
		cur = next;
	}
}

void 
TlGroup::AddChild( TlCharacter *child )
{
	if ( this && child )
	{
		if ( lastChild ) lastChild->SetNextCharacter( child );
		else firstChild = child;
		lastChild = child;
		child->SetParent( (TlCharacter*) this );
		numChildren++;
	}
}

/*
** TlGroup::RemoveChild()
** Find and remove the TlCharacter from children list 
** and then return it
*/
TlCharacter * 
TlGroup::RemoveChild( TlCharacter *child )
{
	TlCharacter *prev, *cur;

	cur = firstChild;
	prev = NULL;
	while ( cur )
	{
		if ( cur == child ) 
		{
			if ( cur == firstChild ) {
				// cout << "----- First" << endl;
				firstChild = cur->GetNextCharacter();
			}
			if ( cur == lastChild ) {
				// cout << "----- Last" << endl;
				lastChild = prev; 
			}
			
			if ( prev ) prev->SetNextCharacter( cur->GetNextCharacter() );
			
			// disconnect child
			child->SetParent( (TlCharacter*) NULL );
			child->SetNextCharacter( (TlCharacter*) NULL );
			numChildren--;
			// cout << "++++++ TlGroup::RemoveChild :" << cur->GetName() << endl;
			
			return child;
		}
		prev = cur;
 		cur = cur->GetNextCharacter();
	}
	// cout << "++++++ TlGroup::RemoveChild : not found" << endl;
	return ( (TlCharacter*) NULL );
}

/*
** TlGroup::LookUp()
** Find and return a TlCharacter with the given name
** 
*/
TlCharacter* 
TlGroup::LookUp( const char* name )
{
	TlCharacter *ret = NULL;
	TlCharacter *tmp = GetChildren();
	
	if ( name == NULL ) return ret;
		
	while ( tmp ) 
	{
		if ( !strcmp( tmp->GetName(), name ) )  return tmp;
		else {
			if (tmp->GetType() == GROUP_TYPE) 
				ret = ((TlGroup *)tmp)->LookUp(name);
			tmp = tmp->GetNextCharacter();
		}
	}

	return ret;
}

/*
** TlGroup::SetRefID()
** Set all the component ref ids
*/
int
TlGroup::SetRefID( int id )
{
	TlCharacter *tmp = GetChildren();
	
	while ( tmp ) 
	{
		tmp->SetRefID( -1 );
		tmp = tmp->GetNextCharacter();
	}
	return id;
}

ostream& 
TlGroup::WriteSDF1( ostream& os )
{
	TlCharacter *tmp = GetChildren();
	
	while ( tmp ) 
	{
		tmp->WriteSDF1( os );
		tmp = tmp->GetNextCharacter();
	}
	return os;
}

ostream& 
TlGroup::WriteSDF( ostream& os )
{
	char buf[ 80 ];
	TlCharacter *tmp = GetChildren();
	
	sprintf( buf, "Define Group \"%s\" {", GetName() );
	BEGIN_SDF( os, buf );			

	// Write the transform if it is not "unit" transform
	if ( transf.UnitTransform() == FALSE ) transf.WriteSDF( os );
				
	while ( tmp ) 
	{
		tmp->WriteSDF( os );
		tmp = tmp->GetNextCharacter();
	}

	END_SDF( os, "}" );
	
	return os;	
}
