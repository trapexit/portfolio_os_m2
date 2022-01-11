#include "TlCharacter.h"

#ifndef GROUP
#define GROUP

// TlGroup class
class TlGroup : public TlCharacter
{
  public :
	TlGroup( const char *name );
	~TlGroup();

	// Get the data
	inline TlCharacter* GetFirstChild() 
		{ return firstChild; }
	inline TlCharacter* GetLastChild()
		{ return lastChild; }
	inline Int32 GetNumChildren()
		{ return numChildren; }
	TlCharacter *GetChildren()
		{ return firstChild; }
	int GetType() { return GROUP_TYPE; }
	void AddChild( TlCharacter* chld );
	TlCharacter *RemoveChild( TlCharacter *child );
  	int SetRefID( int id );
  	TlCharacter* LookUp( const char* name );
  	
  	ostream& WriteSDF1( ostream& os );
  	ostream& WriteSDF( ostream& os );
  	
  private:
	TlCharacter *firstChild;
	TlCharacter *lastChild;
	Int32 numChildren;
};

#endif // GROUP
