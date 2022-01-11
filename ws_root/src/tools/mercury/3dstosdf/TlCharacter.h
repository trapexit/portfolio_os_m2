#include "TlBasicTypes.h"
#include "TlTransform.h"

#ifndef CHARACTER
#define CHARACTER


class TlCharacter
{
  private:
  	char *charName;
  	TlCharacter *nextCharacter;
  	TlCharacter *parent;
  	
  public:
	TlTransform	transf;
  
  public:
  	TlCharacter();
	TlCharacter( const char *name );
	virtual ~TlCharacter(); 
	virtual TlCharacter* Instance();
	
  	void SetCharacterName( const char *name ); 
	void SetNextCharacter( TlCharacter *chr )
		{ nextCharacter = chr; }
	void SetParent( TlCharacter *prnt )
		{ parent = prnt; }
	virtual int SetRefID( int id )
		{ return id; }

	TlCharacter *GetParent()
		{ return parent; }
	virtual TlCharacter *GetChildren();	
	TlCharacter *GetNextCharacter()
		{ return nextCharacter; }
	const char* GetName()
		{ return charName; }
	virtual int GetType()
		{ return CHAR_TYPE; }
	// Write unique object SDF define
	virtual ostream& WriteSDF1( ostream& os )
		{ return os; }	
	// Write object instance SDF use
	virtual ostream& WriteSDF( ostream& os )
		{ return os; }
};
  
#endif	// CHARACTER
